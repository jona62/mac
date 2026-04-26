#!/usr/bin/env python3
"""
Mac Analyzer Test Suite

Tests the --analyze JSON output for correctness, position accuracy,
cross-reference integrity, and regression detection.

Usage:
    python3 tests/analyzer/run_analyzer_tests.py                # run all tests
    python3 tests/analyzer/run_analyzer_tests.py --update        # update snapshots
    python3 tests/analyzer/run_analyzer_tests.py -k symbols      # filter by name
"""

import json
import os
import subprocess
import sys
from pathlib import Path
from typing import Any

PROJECT_DIR = Path(__file__).resolve().parent.parent.parent
MAC = PROJECT_DIR / "build" / "mac"
SNAPSHOT_DIR = Path(__file__).resolve().parent / "snapshots"

# ── Helpers ───────────────────────────────────────────────

def analyze(source: str) -> dict:
    """Write source to a temp file, run --analyze, return parsed JSON."""
    tmp = PROJECT_DIR / "build" / "_analyzer_test.mac"
    tmp.write_text(source)
    try:
        result = subprocess.run(
            [str(MAC), "--analyze", str(tmp)],
            capture_output=True, text=True, timeout=10
        )
        return json.loads(result.stdout)
    finally:
        tmp.unlink(missing_ok=True)


def catalog(selector: str | None = None) -> dict:
    """Run --catalog and return parsed JSON."""
    args = [str(MAC), "--catalog" if selector is None else f"--catalog={selector}"]
    result = subprocess.run(args, capture_output=True, text=True, timeout=10)
    if result.returncode != 0:
        raise AssertionError(
            f"--catalog failed with {result.returncode}: {result.stdout} {result.stderr}"
        )
    return json.loads(result.stdout)


def user_items(data: dict, category: str) -> list[dict]:
    """Filter to source='user' items (except templates which have no source)."""
    items = data.get(category, [])
    if category == "templates":
        return items
    return [i for i in items if i.get("source") == "user"]


def find(items: list[dict], **kwargs) -> dict | None:
    """Find first item matching all key=value pairs."""
    for item in items:
        if all(item.get(k) == v for k, v in kwargs.items()):
            return item
    return None


def find_all(items: list[dict], **kwargs) -> list[dict]:
    """Find all items matching all key=value pairs."""
    return [
        item for item in items
        if all(item.get(k) == v for k, v in kwargs.items())
    ]


def assert_eq(actual, expected, msg=""):
    if actual != expected:
        raise AssertionError(f"{msg}: expected {expected!r}, got {actual!r}")


def assert_true(cond, msg=""):
    if not cond:
        raise AssertionError(msg)


def assert_none(val, msg=""):
    if val is not None:
        raise AssertionError(f"{msg}: expected None, got {val!r}")

# ── Test runner ───────────────────────────────────────────

TESTS: list[tuple[str, callable]] = []

def test(fn):
    TESTS.append((fn.__name__, fn))
    return fn

# ── Symbol tests ──────────────────────────────────────────

@test
def symbols_variable_kinds():
    """Variables, functions, parameters, classes, fields, methods get correct kinds."""
    data = analyze("""
var x = 42;
fun add(a, b) { return a + b; }
class Point {
    init(px, py) { this.x = px; this.y = py; }
    dist() { return this.x + this.y; }
}
""")
    syms = user_items(data, "symbols")
    # Variable
    s = find(syms, name="x", kind="variable")
    assert_true(s is not None, "var x should be a variable symbol")
    assert_eq(s["type"], "number", "x type")
    assert_eq(s["line"], 2, "x line")

    # Function
    s = find(syms, name="add", kind="function")
    assert_true(s is not None, "add should be a function symbol")
    assert_eq(s["line"], 3, "add line")

    # Parameters
    for name in ("a", "b"):
        s = find(syms, name=name, kind="parameter")
        assert_true(s is not None, f"param {name} should exist")
        assert_eq(s["line"], 3, f"{name} line")

    # Class
    s = find(syms, name="Point", kind="class")
    assert_true(s is not None, "Point should be a class symbol")
    assert_eq(s["type"], "class Point", "Point type")

    # Fields
    for name in ("x", "y"):
        s = find(syms, name=name, kind="field", ownerType="Point")
        assert_true(s is not None, f"field {name} should exist on Point")

    # Method
    s = find(syms, name="dist", kind="method", ownerType="Point")
    assert_true(s is not None, "dist should be a method on Point")


@test
def symbols_position_accuracy():
    """Symbol col and endCol match the identifier's exact span."""
    #       1234567890
    data = analyze("var name = 1;\n")
    syms = user_items(data, "symbols")
    s = find(syms, name="name")
    assert_true(s is not None, "name should exist")
    assert_eq(s["col"], 5, "col should be 5 (after 'var ')")
    assert_eq(s["endCol"], 9, "endCol should be 5 + len('name') = 9")


@test
def symbols_visibility_internal():
    """Underscore-prefixed members get internal visibility."""
    data = analyze("""
class Cfg {
    init() {
        this.pub = 1;
        this._priv = 2;
    }
    _secret() { return this._priv; }
}
""")
    syms = user_items(data, "symbols")
    assert_eq(find(syms, name="pub", ownerType="Cfg")["visibility"], "public")
    assert_eq(find(syms, name="_priv", ownerType="Cfg")["visibility"], "internal")
    assert_eq(find(syms, name="_secret", ownerType="Cfg")["visibility"], "internal")


# ── Reference tests ───────────────────────────────────────

@test
def references_point_to_definitions():
    """Each reference's defLine/defCol matches the actual definition location."""
    data = analyze("""
var target = 42;
print target;
""")
    refs = user_items(data, "references")
    r = find(refs, defName="target")
    assert_true(r is not None, "reference to target should exist")
    # Definition is on line 2 col 5
    assert_eq(r["defLine"], 2, "defLine")
    assert_eq(r["defCol"], 5, "defCol")
    assert_eq(r["defEndCol"], 11, "defEndCol = 5 + len('target')")
    # Usage is on line 3
    assert_eq(r["line"], 3, "ref line")


@test
def references_cross_scope():
    """References inside functions resolve to outer scope definitions."""
    data = analyze("""
var outer = 10;
fun useit() {
    return outer;
}
""")
    refs = user_items(data, "references")
    r = find(refs, defName="outer")
    assert_true(r is not None, "reference to outer should exist")
    assert_eq(r["defLine"], 2, "defLine should be 2")
    assert_eq(r["line"], 4, "usage line should be 4")


@test
def references_parameter_inside_body():
    """References to parameters resolve to the parameter definition on the function line."""
    data = analyze("""
fun greet(who) {
    return "Hi " + who;
}
""")
    refs = user_items(data, "references")
    r = find(refs, defName="who")
    assert_true(r is not None, "reference to who should exist")
    assert_eq(r["defLine"], 2, "param defined on function line")
    assert_eq(r["line"], 3, "used inside body")


@test
def references_multiple_usages():
    """A variable used multiple times produces multiple reference entries."""
    data = analyze("""
var x = 1;
var a = x;
var b = x;
var c = x;
""")
    refs = find_all(user_items(data, "references"), defName="x")
    assert_eq(len(refs), 3, "x should have 3 references")
    lines = sorted(r["line"] for r in refs)
    assert_eq(lines, [3, 4, 5], "references on lines 3, 4, 5")


# ── Diagnostic tests ─────────────────────────────────────

@test
def diagnostics_undefined_variable():
    """Undefined variables produce a warning with the correct name and position."""
    data = analyze("print unknownVar;\n")
    diags = user_items(data, "diagnostics")
    d = find(diags, severity="warning")
    assert_true(d is not None, "should have a warning")
    assert_true("unknownVar" in d["message"], f"message should mention unknownVar: {d['message']}")
    assert_eq(d["line"], 1, "line")
    assert_eq(d["col"], 7, "col")
    assert_eq(d["endCol"], 17, "endCol = 7 + len('unknownVar')")


@test
def diagnostics_no_false_positives_natives():
    """Native functions don't trigger undefined variable warnings."""
    data = analyze("""
var a = len("hi");
var b = type(42);
var c = abs(-1);
var d = floor(3.5);
push([], 1);
""")
    diags = user_items(data, "diagnostics")
    assert_eq(len(diags), 0, f"no diagnostics expected, got: {[d['message'] for d in diags]}")


@test
def diagnostics_no_false_positives_defined():
    """Defined variables don't trigger warnings."""
    data = analyze("""
var x = 1;
var y = x + 2;
fun f() { return 1; }
var z = f();
""")
    diags = user_items(data, "diagnostics")
    assert_eq(len(diags), 0, f"no diagnostics expected, got: {[d['message'] for d in diags]}")


@test
def diagnostics_multiple_undefined():
    """Multiple undefined variables each produce their own diagnostic."""
    data = analyze("var x = aaa + bbb + ccc;\n")
    diags = user_items(data, "diagnostics")
    names = sorted(d["message"].split("'")[1] for d in diags)
    assert_eq(names, ["aaa", "bbb", "ccc"], "three undefined variables")


# ── Class tests ───────────────────────────────────────────

@test
def classes_structure():
    """ClassInfo contains correct members with kinds and types."""
    data = analyze("""
class Shape {
    init(sides) {
        this.sides = sides;
    }
    area() {
        return this.sides * 10;
    }
}
""")
    classes = user_items(data, "classes")
    c = find(classes, name="Shape")
    assert_true(c is not None, "Shape class should exist")
    members = c["members"]

    field = find(members, name="sides", kind="field")
    assert_true(field is not None, "sides field should exist")

    ctor = find(members, name="init", kind="constructor")
    assert_true(ctor is not None, "init constructor should exist")
    assert_eq(ctor["params"], ["sides"], "constructor params")
    assert_eq(ctor["returnType"], "Shape", "constructor returns Shape")

    method = find(members, name="area", kind="method")
    assert_true(method is not None, "area method should exist")
    assert_eq(method["returnType"], "number", "area returns number")


@test
def classes_inheritance_superclass():
    """Subclass records the superclass name."""
    data = analyze("""
class Base { init() {} }
class Child < Base { init() { super.init(); } }
""")
    classes = data["classes"]
    child = find(classes, name="Child")
    assert_true(child is not None, "Child class should exist")
    assert_eq(child["superclass"], "Base", "superclass should be Base")


@test
def classes_inherited_property_resolution():
    """Accessing a superclass method on a subclass instance resolves to the superclass."""
    data = analyze("""
class Animal {
    init(name) { this.name = name; }
    speak() { return this.name; }
}
class Dog < Animal {
    init(name) { super.init(name); }
}
var d = Dog("Rex");
print d.speak();
""")
    props = user_items(data, "properties")
    p = find(props, name="speak", line=10)
    assert_true(p is not None, "speak property ref should exist on line 10")
    assert_eq(p["ownerType"], "Dog", "ownerType resolved through Dog")


# ── Property tests ────────────────────────────────────────

@test
def properties_link_to_definitions():
    """PropertyRef entries link field access to the definition site."""
    data = analyze("""
class User {
    init(name) { this.name = name; }
}
var u = User("Alice");
print u.name;
""")
    props = user_items(data, "properties")
    p = find(props, name="name", line=6)
    assert_true(p is not None, "u.name property ref should exist")
    assert_eq(p["ownerType"], "User", "ownerType")
    assert_eq(p["kind"], "field", "kind")
    # Definition should point to `this.name = name` in init
    assert_eq(p["defLine"], 3, "defLine")


@test
def properties_method_calls():
    """Method calls produce PropertyRef entries with kind=method."""
    data = analyze("""
class Calc {
    init() { this.val = 0; }
    add(n) { return this.val + n; }
}
var c = Calc();
print c.add(5);
""")
    props = user_items(data, "properties")
    p = find(props, name="add", line=7)
    assert_true(p is not None, "c.add property ref should exist")
    assert_eq(p["kind"], "method", "kind should be method")
    assert_eq(p["ownerType"], "Calc", "ownerType")


# ── Folding range tests ──────────────────────────────────

@test
def folding_ranges_functions_and_classes():
    """Functions and classes produce folding ranges spanning their bodies."""
    data = analyze("""
fun hello() {
    var x = 1;
    return x;
}
class Foo {
    init() { this.x = 1; }
    bar() { return this.x; }
}
""")
    folds = user_items(data, "foldingRanges")
    # Function fold: line 2, ends at last body statement (line 4)
    f = find(folds, startLine=2)
    assert_true(f is not None, "function fold should start at line 2")
    assert_eq(f["endLine"], 4, "function fold should end at last body line")

    # Class fold: line 6, ends at last method (line 9)
    f = find(folds, startLine=6)
    assert_true(f is not None, "class fold should start at line 6")
    assert_eq(f["endLine"], 9, "class fold should end at last member line")


# ── Semantic token tests ─────────────────────────────────

@test
def semantic_tokens_categories():
    """Different symbol kinds produce the correct tokenType."""
    data = analyze("""
var count = 0;
fun helper(arg) { return arg; }
class Widget { init() {} }
var w = Widget();
var r = helper(count);
""")
    tokens = user_items(data, "semanticTokens")
    # Definition tokens
    assert_true(find(tokens, tokenType="variable", line=2) is not None, "variable token")
    assert_true(find(tokens, tokenType="function", line=3) is not None, "function token")
    assert_true(find(tokens, tokenType="parameter", line=3) is not None, "parameter token")
    assert_true(find(tokens, tokenType="class", line=4) is not None, "class token")
    # Reference tokens
    assert_true(find(tokens, tokenType="class", line=5) is not None, "Widget ref token")
    assert_true(find(tokens, tokenType="function", line=6) is not None, "helper ref token")
    assert_true(find(tokens, tokenType="variable", line=6) is not None, "count ref token")


@test
def semantic_tokens_no_prelude_leak():
    """Semantic tokens only contain user-source items, no prelude leaks."""
    data = analyze("var x = 1;\n")
    tokens = data["semanticTokens"]
    for t in tokens:
        assert_eq(t["source"], "user", f"token should be user-source: {t}")


@test
def semantic_tokens_position_accuracy():
    """Token col and length match the identifier span exactly."""
    #       1234567890123
    data = analyze("var myVar = 1;\n")
    tokens = user_items(data, "semanticTokens")
    t = find(tokens, tokenType="variable")
    assert_true(t is not None, "variable token should exist")
    assert_eq(t["col"], 5, "col after 'var '")
    assert_eq(t["length"], 5, "length of 'myVar'")


# ── Signature tests ───────────────────────────────────────

@test
def signatures_function_params_and_return():
    """Signatures capture params and inferred return type."""
    data = analyze("""
fun multiply(a, b) {
    return a * b;
}
fun concat(s1, s2) {
    return s1 + s2;
}
""")
    sigs = user_items(data, "signatures")
    s = find(sigs, name="multiply")
    assert_true(s is not None, "multiply signature should exist")
    assert_eq(s["params"], ["a", "b"], "params")
    assert_eq(s["kind"], "function", "kind")

    s = find(sigs, name="concat")
    assert_true(s is not None, "concat signature should exist")
    assert_eq(s["params"], ["s1", "s2"], "params")


@test
def signatures_constructor_and_methods():
    """Class constructors and methods produce signatures with ownerType."""
    data = analyze("""
class Stack {
    init(capacity) { this.cap = capacity; }
    push(item) { return true; }
    size() { return 0; }
}
""")
    sigs = user_items(data, "signatures")
    ctor = find(sigs, name="init", ownerType="Stack")
    assert_true(ctor is not None, "Stack.init signature should exist")
    assert_eq(ctor["kind"], "constructor", "init kind")
    assert_eq(ctor["params"], ["capacity"], "init params")
    assert_eq(ctor["returnType"], "Stack", "constructor returns Stack")

    push = find(sigs, name="push", ownerType="Stack")
    assert_true(push is not None, "Stack.push signature should exist")
    assert_eq(push["kind"], "method", "push kind")
    assert_eq(push["returnType"], "bool", "push returns bool")


@test
def signatures_native_functions():
    """Native functions have signatures with kind=native."""
    data = analyze("var x = 1;\n")
    sigs = data["signatures"]
    s = find(sigs, name="len", kind="native")
    assert_true(s is not None, "len native signature should exist")
    s = find(sigs, name="push", kind="native")
    assert_true(s is not None, "push native signature should exist")


# ── Param hint tests ─────────────────────────────────────

@test
def param_hints_at_call_sites():
    """Param hints appear at argument positions when args are variables."""
    data = analyze("""
fun add(left, right) { return left + right; }
var a = 1;
var b = 2;
var r = add(a, b);
""")
    hints = user_items(data, "paramHints")
    h = find(hints, name="left")
    assert_true(h is not None, "left param hint should exist")
    assert_eq(h["line"], 5, "line")

    h = find(hints, name="right")
    assert_true(h is not None, "right param hint should exist")
    assert_eq(h["line"], 5, "line")


@test
def param_hints_method_calls():
    """Param hints work for method calls on typed objects."""
    data = analyze("""
class Vec {
    init() { this.items = []; }
    set(index, value) { return true; }
}
var v = Vec();
var i = 0;
var num = 42;
v.set(i, num);
""")
    hints = user_items(data, "paramHints")
    h = find(hints, name="index")
    assert_true(h is not None, "index param hint should exist")
    h = find(hints, name="value")
    assert_true(h is not None, "value param hint should exist")


# ── Chain hint tests ──────────────────────────────────────

@test
def chain_hints_return_type():
    """Chain hints show the return type at the closing paren of method calls."""
    data = analyze("""
class Fmt {
    init() {}
    bold() { return "**bold**"; }
    count() { return 42; }
}
var f = Fmt();
print f.bold();
print f.count();
""")
    hints = user_items(data, "chainHints")
    h = find(hints, type="string")
    assert_true(h is not None, "string chain hint should exist")
    assert_eq(h["line"], 8, "bold() on line 8")

    h = find(hints, type="number")
    assert_true(h is not None, "number chain hint should exist")
    assert_eq(h["line"], 9, "count() on line 9")


# ── Type inference tests ──────────────────────────────────

@test
def type_inference_literals():
    """Literal types are inferred correctly on variables."""
    data = analyze("""
var n = 42;
var s = "hello";
var b = true;
var nothing = nil;
""")
    syms = user_items(data, "symbols")
    assert_eq(find(syms, name="n")["type"], "number")
    assert_eq(find(syms, name="s")["type"], "string")
    assert_eq(find(syms, name="b")["type"], "bool")
    assert_eq(find(syms, name="nothing")["type"], "nil")


@test
def type_inference_collections():
    """Array and map literals get element-typed inference."""
    data = analyze("""
var nums = [1, 2, 3];
var strs = ["a", "b"];
var m = {"key": 42};
""")
    syms = user_items(data, "symbols")
    assert_eq(find(syms, name="nums")["type"], "[number]")
    assert_eq(find(syms, name="strs")["type"], "[string]")
    assert_eq(find(syms, name="m")["type"], "{number}")


@test
def type_inference_function_return():
    """Function return types are inferred from the return statement."""
    data = analyze("""
fun getNum() { return 42; }
fun getStr() { return "hi"; }
fun getBool() { return false; }
""")
    sigs = user_items(data, "signatures")
    assert_eq(find(sigs, name="getNum")["returnType"], "number")
    assert_eq(find(sigs, name="getStr")["returnType"], "string")
    assert_eq(find(sigs, name="getBool")["returnType"], "bool")


# ── Template tests ────────────────────────────────────────

@test
def templates_builtins_registered():
    """Built-in meme templates are present in templates output."""
    data = analyze("var x = 1;\n")
    tmpls = data["templates"]
    for name in ("two_panel", "three_panel", "bottom_text", "blank", "dark"):
        assert_true(find(tmpls, name=name) is not None, f"template {name} should exist")


# ── Catalog tests ─────────────────────────────────────────

@test
def catalog_full_registered():
    """Full catalog includes the stable top-level sections."""
    data = catalog()
    for key in (
        "schema_version", "mac_version", "catalog_fingerprint", "included",
        "templates", "assets", "effects", "effect_definitions",
        "layouts", "style_presets", "limits", "allowed_names",
    ):
        assert_true(key in data, f"catalog should include {key}")
    assert_eq(data["schema_version"], 1)


@test
def catalog_comma_selectors_registered():
    """Comma selectors return only requested sections plus metadata."""
    data = catalog("layouts,style_presets,allowed_names")
    assert_eq(data["included"], ["layouts", "style_presets", "allowed_names"])
    assert_true("layouts" in data)
    assert_true("style_presets" in data)
    assert_true("allowed_names" in data)
    assert_true("effects" not in data)


@test
def catalog_asset_selector_registered():
    """Asset category selector exposes meme templates by stable id."""
    data = catalog("assets:meme")
    assets = data["assets"]["meme"]
    assert_true(find(assets, id="meme.distracted_boyfriend") is not None)
    template = find(assets, id="meme.distracted_boyfriend")
    assert_true(template["assetPath"].endswith("assets/templates/meme/distracted_boyfriend.jpg"))


@test
def catalog_unknown_selector_errors():
    """Unknown selectors fail loudly with JSON guidance."""
    result = subprocess.run(
        [str(MAC), "--catalog=missing"],
        capture_output=True,
        text=True,
        timeout=10,
    )
    assert_eq(result.returncode, 2)
    data = json.loads(result.stdout)
    assert_eq(data["ok"], False)
    assert_eq(data["error"], "unknown_catalog_selector")
    assert_true("available" in data)


@test
def analyzer_templates_match_catalog_templates():
    """Analyzer template metadata is sourced from the catalog."""
    analyzed = analyze("var x = 1;\n")
    cataloged = catalog("templates")
    analyzer_names = sorted(t["name"] for t in analyzed["templates"])
    catalog_names = sorted(t["id"] for t in cataloged["templates"])
    assert_eq(analyzer_names, catalog_names)


# ── Scoping tests ─────────────────────────────────────────

@test
def scoping_block_isolation():
    """Variables defined in blocks are not visible outside."""
    data = analyze("""
{
    var inner = 1;
}
print inner;
""")
    diags = user_items(data, "diagnostics")
    d = find(diags, severity="warning")
    assert_true(d is not None, "accessing inner outside block should warn")
    assert_true("inner" in d["message"], "should mention inner")


@test
def scoping_closure_capture():
    """Closures can reference variables from their enclosing scope."""
    data = analyze("""
fun outer() {
    var captured = 1;
    fun inner() {
        return captured;
    }
    return inner;
}
""")
    refs = user_items(data, "references")
    r = find(refs, defName="captured")
    assert_true(r is not None, "reference to captured should exist")
    assert_eq(r["defLine"], 3, "captured defined on line 3")
    assert_eq(r["line"], 5, "captured used on line 5")


@test
def scoping_for_loop_variable():
    """For loop variables are defined in the loop scope."""
    data = analyze("""
for (var i = 0; i < 3; i = i + 1) {
    print i;
}
print i;
""")
    # i inside loop → reference, i outside → diagnostic
    diags = user_items(data, "diagnostics")
    d = find(diags, severity="warning")
    assert_true(d is not None, "i outside loop should warn")
    assert_eq(d["line"], 5, "warning on line 5")


# ── JSON output structure ─────────────────────────────────

@test
def json_all_categories_present():
    """Output JSON contains all 11 required top-level keys."""
    data = analyze("var x = 1;\n")
    expected_keys = {
        "symbols", "references", "diagnostics", "properties",
        "foldingRanges", "semanticTokens", "paramHints",
        "chainHints", "signatures", "classes", "templates"
    }
    assert_eq(set(data.keys()), expected_keys, "top-level keys")


@test
def json_empty_file():
    """Analyzing an empty-ish file produces valid JSON with empty user arrays."""
    data = analyze("// just a comment\n")
    for cat in ("diagnostics", "foldingRanges", "paramHints", "chainHints"):
        user = user_items(data, cat)
        assert_eq(len(user), 0, f"{cat} should have 0 user items")


# ── Snapshot tests ────────────────────────────────────────

SNAPSHOT_SOURCES = {
    "basic_program": """
var x = 42;
var greeting = "hello";
fun double(n) { return n * 2; }
print double(x);
""",
    "class_hierarchy": """
class Animal {
    init(name) { this.name = name; }
    speak() { return this.name; }
}
class Dog < Animal {
    init(name) { super.init(name); }
    bark() { return this.name + " barks"; }
}
var d = Dog("Rex");
print d.bark();
print d.speak();
""",
    "diagnostics_mix": """
var good = 1;
print good;
print bad1;
print bad2;
var result = good + bad3;
""",
}


def normalize_snapshot(data: dict) -> dict:
    """Strip prelude/native items to keep snapshots focused on user code."""
    out = {}
    for cat in ("symbols", "references", "diagnostics", "properties",
                "foldingRanges", "semanticTokens", "paramHints",
                "chainHints", "signatures", "classes"):
        out[cat] = [i for i in data.get(cat, []) if i.get("source") == "user"]
    # Sort templates by name — directory_iterator order varies across platforms
    out["templates"] = sorted(data.get("templates", []), key=lambda t: t.get("name", ""))
    return out


def run_snapshot_tests(update: bool) -> tuple[int, int]:
    """Run snapshot tests, return (pass, fail) counts."""
    SNAPSHOT_DIR.mkdir(exist_ok=True)
    passed = 0
    failed = 0

    for name, source in SNAPSHOT_SOURCES.items():
        data = analyze(source)
        actual = normalize_snapshot(data)
        snap_file = SNAPSHOT_DIR / f"{name}.json"

        if update:
            snap_file.write_text(json.dumps(actual, indent=2, sort_keys=True) + "\n")
            print(f"  SNAP  {name} (updated)")
            passed += 1
            continue

        if not snap_file.exists():
            snap_file.write_text(json.dumps(actual, indent=2, sort_keys=True) + "\n")
            print(f"  SNAP  {name} (created)")
            passed += 1
            continue

        expected = json.loads(snap_file.read_text())
        if actual == expected:
            print(f"  SNAP  {name}")
            passed += 1
        else:
            print(f"  DIFF  {name}")
            failed += 1
            # Show what changed
            for cat in sorted(set(list(actual.keys()) + list(expected.keys()))):
                a = actual.get(cat, [])
                e = expected.get(cat, [])
                if a != e:
                    print(f"        {cat}: expected {len(e)} items, got {len(a)}")
                    # Show first diff
                    for i, (ai, ei) in enumerate(zip(a, e)):
                        if ai != ei:
                            for k in set(list(ai.keys()) + list(ei.keys())):
                                if ai.get(k) != ei.get(k):
                                    print(f"          [{i}].{k}: {ei.get(k)!r} -> {ai.get(k)!r}")
                            break
                    if len(a) != len(e):
                        print(f"          length: {len(e)} -> {len(a)}")

    return passed, failed


# ── Integration tests ─────────────────────────────────────

@test
def integration_cross_reference_integrity():
    """Every reference's defLine/defCol points to an actual symbol definition."""
    data = analyze("""
var alpha = 1;
var beta = 2;
fun combine(a, b) { return a + b; }
var result = combine(alpha, beta);
""")
    syms = data["symbols"]
    refs = user_items(data, "references")

    for r in refs:
        # Find the symbol this reference points to
        matching = [s for s in syms
                    if s["name"] == r["defName"]
                    and s["line"] == r["defLine"]
                    and s["col"] == r["defCol"]]
        assert_true(
            len(matching) > 0,
            f"ref to '{r['defName']}' at line {r['line']} col {r['col']} "
            f"points to line {r['defLine']} col {r['defCol']} but no symbol exists there"
        )


@test
def integration_endcol_consistency():
    """endCol - col equals the length of the name for all symbols."""
    data = analyze("""
var counter = 0;
fun increment(step) { return step; }
class Manager { init(id) { this.id = id; } }
""")
    for sym in user_items(data, "symbols"):
        name = sym["name"]
        span = sym["endCol"] - sym["col"]
        assert_eq(span, len(name),
                  f"symbol '{name}' at line {sym['line']}: endCol - col = {span}, len = {len(name)}")


@test
def integration_full_program():
    """A complex program exercises all categories without errors."""
    data = analyze("""
class Stack {
    init() { this._items = []; }
    push(item) { push(this._items, item); }
    pop() { return pop(this._items); }
    size() { return len(this._items); }
    isEmpty() { return this.size() == 0; }
}
fun reverseStack(s) {
    var out = [];
    return out;
}
var stack = Stack();
stack.push(1);
stack.push(2);
var r = reverseStack(stack);
""")
    # All 11 categories present
    assert_eq(set(data.keys()), {
        "symbols", "references", "diagnostics", "properties",
        "foldingRanges", "semanticTokens", "paramHints",
        "chainHints", "signatures", "classes", "templates"
    })

    # No diagnostics (everything well-defined)
    assert_eq(len(user_items(data, "diagnostics")), 0,
              f"no diagnostics: {[d['message'] for d in user_items(data, 'diagnostics')]}")

    # Class exists with members
    cls = find(user_items(data, "classes"), name="Stack")
    assert_true(cls is not None, "Stack class")
    member_names = {m["name"] for m in cls["members"]}
    assert_true({"_items", "init", "push", "pop", "size", "isEmpty"} <= member_names,
                f"expected all members, got {member_names}")

    # Internal field
    assert_eq(find(cls["members"], name="_items")["visibility"], "internal")

    # Properties from method calls on typed instance
    props = user_items(data, "properties")
    assert_true(find(props, name="push", ownerType="Stack") is not None, "stack.push property")

    # Signatures
    sigs = user_items(data, "signatures")
    assert_true(find(sigs, name="push", ownerType="Stack") is not None, "Stack.push signature")
    assert_true(find(sigs, name="reverseStack", kind="function") is not None, "reverseStack signature")


# ── Main ──────────────────────────────────────────────────

def main():
    update = "--update" in sys.argv
    filter_name = None
    for i, arg in enumerate(sys.argv):
        if arg == "-k" and i + 1 < len(sys.argv):
            filter_name = sys.argv[i + 1]

    if not MAC.exists():
        print(f"Binary not found at {MAC}. Build first: cmake --build build")
        sys.exit(1)

    print("")
    passed = 0
    failed = 0
    errors = []

    # Unit tests
    for name, fn in TESTS:
        if filter_name and filter_name not in name:
            continue
        try:
            fn()
            print(f"  PASS  {name}")
            passed += 1
        except AssertionError as e:
            print(f"  FAIL  {name}")
            print(f"        {e}")
            failed += 1
            errors.append((name, str(e)))
        except Exception as e:
            print(f"  ERR   {name}")
            print(f"        {type(e).__name__}: {e}")
            failed += 1
            errors.append((name, f"{type(e).__name__}: {e}"))

    # Snapshot tests
    if not filter_name or "snapshot" in filter_name:
        print("")
        sp, sf = run_snapshot_tests(update)
        passed += sp
        failed += sf

    print(f"\nResults: {passed} passed, {failed} failed, {passed + failed} total")

    if errors:
        print("\nFailures:")
        for name, msg in errors:
            print(f"  {name}: {msg}")

    sys.exit(1 if failed > 0 else 0)


if __name__ == "__main__":
    main()
