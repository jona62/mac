# Type-Safe Memes + Operator Overloading — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add general operator overloading to Mac (dunder methods), then implement type-safe meme types (Template, Position, Format, Size, Duration, Meme, Frame, Gif) as Mac classes defined in a standard library prelude.

**Architecture:** Operator overloading is a small interpreter change — `visitBinaryExpr` and `visitUnaryExpr` check for `__add__`, `__neg__`, etc. on class instances via `findMethod`. The meme types are written in Mac itself as a prelude file loaded at startup, backed by low-level native C++ functions for image I/O. This keeps the types in Mac's own class system with full operator overloading support.

**Tech Stack:** C++ interpreter changes, Mac language standard library, existing stb/MemeRenderer/GifEncoder infrastructure.

---

### Task 1: Operator Overloading

**Files:**
- Modify: `include/Interpreter.h` (visitBinaryExpr ~lines 89-137, visitUnaryExpr ~lines 74-87)
- Create: `tests/operators/overloading.mac`

- [ ] **Step 1: Create the test file**

```mac
// tests/operators/overloading.mac
class Vec {
    init(x, y) { this.x = x; this.y = y; }
    __add__(other) { return Vec(this.x + other.x, this.y + other.y); }
    __sub__(other) { return Vec(this.x - other.x, this.y - other.y); }
    __mul__(n) { return Vec(this.x * n, this.y * n); }
    __eq__(other) { return this.x == other.x and this.y == other.y; }
    __neg__() { return Vec(-this.x, -this.y); }
    str() { return "(" + this.x + ", " + this.y + ")"; }
}
var a = Vec(1, 2);
var b = Vec(3, 4);
var c = a + b;
print c.x;              // expect: 4
print c.y;              // expect: 6
var d = a - b;
print d.x;              // expect: -2
var e = a * 3;
print e.x;              // expect: 3
print e.y;              // expect: 6
print a == Vec(1, 2);   // expect: true
print a == b;           // expect: false
var f = -a;
print f.x;              // expect: -1
print f.y;              // expect: -2

// Operators still work normally for non-instances
print 1 + 2;            // expect: 3
print "a" + "b";        // expect: ab
```

- [ ] **Step 2: Modify visitBinaryExpr in Interpreter.h**

After evaluating `left` and `right` (line ~91), before the switch statement, add an instance check:

```cpp
MacValue visitBinaryExpr(expr::Binary<MacValue>* expr) override {
    MacValue left = evaluate(expr->left);
    MacValue right = evaluate(expr->right);

    // Check for operator overloading on class instances
    if (std::holds_alternative<shared_ptr<instance::MacInstance>>(left)) {
        auto inst = std::get<shared_ptr<instance::MacInstance>>(left);
        std::string dunder;
        switch (expr->operatorToken.type) {
            case token::TokenType::PLUS:          dunder = "__add__"; break;
            case token::TokenType::MINUS:         dunder = "__sub__"; break;
            case token::TokenType::STAR:          dunder = "__mul__"; break;
            case token::TokenType::SLASH:         dunder = "__div__"; break;
            case token::TokenType::PERCENT:       dunder = "__mod__"; break;
            case token::TokenType::EQUAL_EQUAL:   dunder = "__eq__"; break;
            case token::TokenType::BANG_EQUAL:     dunder = "__ne__"; break;
            case token::TokenType::LESS:          dunder = "__lt__"; break;
            case token::TokenType::GREATER:       dunder = "__gt__"; break;
            case token::TokenType::LESS_EQUAL:    dunder = "__le__"; break;
            case token::TokenType::GREATER_EQUAL: dunder = "__ge__"; break;
            default: break;
        }
        if (!dunder.empty()) {
            auto method = inst->klass->findMethod(dunder);
            if (method) {
                auto bound = method->bind(inst);
                return bound->call(shared_from_this(), {right});
            }
        }
    }

    // Existing switch statement follows unchanged...
    switch (expr->operatorToken.type) {
```

Note: `inst->klass` is private in MacInstance. Need to make it accessible — add a public getter or make it public. Check MacInstance.h — `klass` is private. Add: `std::shared_ptr<callable::MacClass> getClass() const { return klass; }` or make the field public.

- [ ] **Step 3: Modify visitUnaryExpr in Interpreter.h**

Before the switch statement, add an instance check:

```cpp
MacValue visitUnaryExpr(expr::Unary<MacValue>* expr) override {
    MacValue right = evaluate(expr->right);

    // Check for operator overloading on class instances
    if (std::holds_alternative<shared_ptr<instance::MacInstance>>(right)) {
        auto inst = std::get<shared_ptr<instance::MacInstance>>(right);
        std::string dunder;
        switch (expr->operatorToken.type) {
            case token::TokenType::MINUS: dunder = "__neg__"; break;
            case token::TokenType::BANG:  dunder = "__not__"; break;
            default: break;
        }
        if (!dunder.empty()) {
            auto method = inst->getClass()->findMethod(dunder);
            if (method) {
                auto bound = method->bind(inst);
                return bound->call(shared_from_this(), {});
            }
        }
    }

    // Existing switch statement follows unchanged...
```

- [ ] **Step 4: Make MacInstance::klass accessible**

In `include/MacInstance.h`, add a public getter:

```cpp
std::shared_ptr<callable::MacClass> getClass() const { return klass; }
```

- [ ] **Step 5: Build and test**

```bash
cmake --build build --clean-first && bash tests/run_tests.sh
```

Expected: 32 existing tests pass + new overloading test passes.

- [ ] **Step 6: Commit**

```bash
git add include/Interpreter.h include/MacInstance.h tests/operators/overloading.mac
git commit -m "feat: add general operator overloading via dunder methods"
```

---

### Task 2: Prelude System (Load Mac Stdlib at Startup)

**Files:**
- Create: `stdlib/prelude.mac`
- Modify: `main.cpp`
- Modify: `CMakeLists.txt` (copy stdlib to build)

- [ ] **Step 1: Create minimal prelude to test the system**

```mac
// stdlib/prelude.mac
// Mac Standard Library — loaded before user code

class Size {
    init(w, h) {
        this.width = w;
        this.height = h;
    }
    __add__(other) {
        return Size(this.width + other.width, this.height + other.height);
    }
    __mul__(n) {
        return Size(this.width * n, this.height * n);
    }
    __eq__(other) {
        return this.width == other.width and this.height == other.height;
    }
}

class Duration {
    init(ms) {
        this.ms = ms;
    }
    __add__(other) {
        return Duration(this.ms + other.ms);
    }
    __mul__(n) {
        return Duration(this.ms * n);
    }
    __eq__(other) {
        return this.ms == other.ms;
    }
}
```

- [ ] **Step 2: Update CMakeLists.txt to copy stdlib**

Add after the existing assets copy:

```cmake
file(COPY ${CMAKE_SOURCE_DIR}/stdlib DESTINATION ${CMAKE_BINARY_DIR})
```

- [ ] **Step 3: Modify main.cpp to load prelude**

In the `run()` function or at startup, load and execute `stdlib/prelude.mac` before user code. Add a `loadPrelude()` function:

```cpp
void loadPrelude() {
    // Try to load stdlib/prelude.mac relative to executable
    std::ifstream prelude("stdlib/prelude.mac");
    if (!prelude.is_open()) return; // silently skip if not found
    std::ostringstream ss;
    std::string line;
    while (std::getline(prelude, line)) ss << line << '\n';
    prelude.close();
    run(ss.str());
}
```

Call `loadPrelude()` in `main()` after creating the interpreter but before `run_file()` or `run_prompt()`.

- [ ] **Step 4: Create test for prelude types**

```mac
// tests/stdlib/prelude_types.mac
var s = Size(100, 200);
print s.width;           // expect: 100
print s.height;          // expect: 200
var s2 = s * 2;
print s2.width;          // expect: 200
print s2.height;         // expect: 400
var s3 = s + Size(50, 50);
print s3.width;          // expect: 150

var d = Duration(300);
print d.ms;              // expect: 300
var d2 = d + Duration(200);
print d2.ms;             // expect: 500
```

- [ ] **Step 5: Build and test**

```bash
cmake -S . -B build && cmake --build build --clean-first && bash tests/run_tests.sh
```

- [ ] **Step 6: Commit**

```bash
git add stdlib/prelude.mac main.cpp CMakeLists.txt tests/stdlib/prelude_types.mac
git commit -m "feat: add prelude system for Mac standard library"
```

---

### Task 3: Position and Format Constants + Template Type

**Files:**
- Modify: `stdlib/prelude.mac`
- Modify: `include/NativeFunctions.h` (add low-level `_meme_render` native)
- Modify: `include/Interpreter.h` (register new native)
- Create: `tests/memes/typed_basic.mac`

- [ ] **Step 1: Add Position, Format, Template to prelude.mac**

Append to `stdlib/prelude.mac`:

```mac
// Position constants
class Position {
    init(name) { this.name = name; }
    __eq__(other) { return this.name == other.name; }
}
var Top = Position("top");
var Bottom = Position("bottom");
var Center = Position("center");

// Format constants
class Format {
    init(name) { this.name = name; }
    __eq__(other) { return this.name == other.name; }
}
var PNG = Format("png");
var JPG = Format("jpg");
var GIF = Format("gif");

// Template
class Template {
    init(nameOrPath) {
        this.name = nameOrPath;
        this.path = _resolve_template(nameOrPath);
    }
}
```

- [ ] **Step 2: Add `_resolve_template` native function**

In `include/NativeFunctions.h`:

```cpp
class ResolveTemplateFunction : public MacCallable {
public:
    value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                         std::vector<value::MacValue> args) override {
        auto name = std::get<std::string>(args[0]);
        return meme::MacMeme::resolveTemplate(name);
    }
    int arity() override { return 1; }
    std::string toString() override { return "<native fn>"; }
};
```

Register in Interpreter.h constructor:
```cpp
defn("_resolve_template", make_shared<callable::ResolveTemplateFunction>());
```

- [ ] **Step 3: Add `_meme_save` native function**

In `include/NativeFunctions.h`:

```cpp
class MemeRenderSaveFunction : public MacCallable {
public:
    value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                         std::vector<value::MacValue> args) override {
        auto templatePath = std::get<std::string>(args[0]);
        auto topText = std::get<std::string>(args[1]);
        auto bottomText = std::get<std::string>(args[2]);
        int width = static_cast<int>(std::get<double>(args[3]));
        int height = static_cast<int>(std::get<double>(args[4]));
        auto format = std::get<std::string>(args[5]);
        auto outputPath = std::get<std::string>(args[6]);

        auto m = std::make_shared<meme::MacMeme>("", topText, bottomText);
        m->imagePath = templatePath;
        m->width = width;
        m->height = height;
        return m->save(outputPath);
    }
    int arity() override { return 7; }
    std::string toString() override { return "<native fn>"; }
};
```

Register: `defn("_meme_save", make_shared<callable::MemeRenderSaveFunction>());`

- [ ] **Step 4: Add `_gif_save` native function**

```cpp
class GifRenderSaveFunction : public MacCallable {
public:
    value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                         std::vector<value::MacValue> args) override {
        // args[0] = array of maps [{path, top, bottom, w, h, duration}, ...]
        // args[1] = output path
        auto framesArr = std::get<std::shared_ptr<collection::MacArray>>(args[0]);
        auto outputPath = std::get<std::string>(args[1]);

        auto gif = std::make_shared<meme::MacGif>();
        for (auto& frameVal : framesArr->elements) {
            auto frameMap = std::get<std::shared_ptr<collection::MacMap>>(frameVal);
            auto path = std::get<std::string>(frameMap->get("path"));
            auto top = std::get<std::string>(frameMap->get("top"));
            auto bottom = std::get<std::string>(frameMap->get("bottom"));
            int w = static_cast<int>(std::get<double>(frameMap->get("width")));
            int h = static_cast<int>(std::get<double>(frameMap->get("height")));
            int dur = static_cast<int>(std::get<double>(frameMap->get("duration")));

            auto m = std::make_shared<meme::MacMeme>("", top, bottom);
            m->imagePath = path;
            m->width = w;
            m->height = h;
            gif->addFrame(m, dur);
        }
        return gif->save(outputPath);
    }
    int arity() override { return 2; }
    std::string toString() override { return "<native fn>"; }
};
```

Register: `defn("_gif_save", make_shared<callable::GifRenderSaveFunction>());`

- [ ] **Step 5: Write test**

```mac
// tests/memes/typed_basic.mac
print type(Top);             // expect: instance
print Top.name;              // expect: top
print PNG.name;              // expect: png
print Top == Top;            // expect: true
print Top == Bottom;         // expect: false

var drake = Template("drake");
print drake.name;            // expect: drake

var s = Size(400, 400);
print s.width;               // expect: 400
print (s * 2).width;         // expect: 800

var d = Duration(300);
print d.ms;                  // expect: 300
print (d + Duration(200)).ms; // expect: 500
```

- [ ] **Step 6: Build and test**

```bash
cmake -S . -B build && cmake --build build --clean-first && bash tests/run_tests.sh
```

- [ ] **Step 7: Commit**

```bash
git add stdlib/prelude.mac include/NativeFunctions.h include/Interpreter.h tests/memes/typed_basic.mac
git commit -m "feat: add Position, Format, Template, Size, Duration types to stdlib"
```

---

### Task 4: Meme, Frame, and Gif Builder Types

**Files:**
- Modify: `stdlib/prelude.mac`
- Create: `tests/memes/typed_meme.mac`
- Create: `tests/memes/typed_gif.mac`

- [ ] **Step 1: Add Meme class to prelude.mac**

```mac
class Frame {
    init(memeObj, duration) {
        this.meme = memeObj;
        this.duration = duration;
    }
}

class Meme {
    init(template) {
        this._template = template;
        this._top = "";
        this._bottom = "";
        this._width = 0;
        this._height = 0;
    }
    text(position, str) {
        var m = Meme(this._template);
        m._top = this._top;
        m._bottom = this._bottom;
        m._width = this._width;
        m._height = this._height;
        if (position == Top) m._top = str;
        if (position == Bottom) m._bottom = str;
        if (position == Center) m._bottom = str;
        return m;
    }
    save(format, path) {
        _meme_save(this._template.path, this._top, this._bottom,
                   this._width, this._height, format.name, path);
    }
    resize(size) {
        var m = Meme(this._template);
        m._top = this._top;
        m._bottom = this._bottom;
        m._width = size.width;
        m._height = size.height;
        return m;
    }
    __add__(other) {
        if (type(other) == "instance") {
            // Meme + Duration = Frame
            if (other.ms) return Frame(this, other);
            // Meme + Meme = side-by-side (future: composite)
        }
        return nil;
    }
    __div__(other) {
        // Meme / Meme = stacked (future: composite)
        return nil;
    }
}

class Gif {
    init() {
        this._frames = [];
    }
    frame(memeObj, duration) {
        push(this._frames, {
            path: memeObj._template.path,
            top: memeObj._top,
            bottom: memeObj._bottom,
            width: memeObj._width,
            height: memeObj._height,
            duration: duration.ms
        });
        return this;
    }
    save(path) {
        _gif_save(this._frames, path);
    }
    __add__(other) {
        // Gif + Frame
        if (other.meme) {
            this.frame(other.meme, other.duration);
        }
        return this;
    }
}
```

- [ ] **Step 2: Write Meme test**

```mac
// tests/memes/typed_meme.mac
var drake = Template("drake");
var m = Meme(drake)
    .text(Top, "Writing Java")
    .text(Bottom, "Writing Mac");
print m._top;                    // expect: Writing Java
print m._bottom;                 // expect: Writing Mac

m.save(PNG, "typed_meme.png");
print "saved";                   // expect: saved

var small = m.resize(Size(200, 200));
small.save(JPG, "typed_small.jpg");
print "resized";                 // expect: resized
```

- [ ] **Step 3: Write Gif test**

```mac
// tests/memes/typed_gif.mac
var drake = Template("drake");
var m1 = Meme(drake).text(Top, "Frame 1");
var m2 = Meme(drake).text(Top, "Frame 2");

// Method chaining
var g = Gif()
    .frame(m1, Duration(300))
    .frame(m2, Duration(300));
g.save("typed_anim.gif");
print "gif saved";               // expect: gif saved

// Operator composition
var f1 = m1 + Duration(400);
print type(f1);                  // expect: instance
var g2 = Gif() + f1;
g2.save("typed_op.gif");
print "op gif saved";            // expect: op gif saved
```

- [ ] **Step 4: Build and test**

```bash
cmake -S . -B build && cmake --build build --clean-first && bash tests/run_tests.sh
```

- [ ] **Step 5: Commit**

```bash
git add stdlib/prelude.mac tests/memes/typed_meme.mac tests/memes/typed_gif.mac
git commit -m "feat: add Meme, Frame, Gif builder types to stdlib"
```

---

### Task 5: Remove Old API + Clean Up

**Files:**
- Modify: `include/NativeFunctions.h` (remove old meme/gif classes)
- Modify: `include/Interpreter.h` (remove old registrations + visitGetExpr meme/gif blocks)
- Modify: `tests/memes/basic.mac` (update to new API)
- Modify: `tests/memes/images.mac` (update to new API)
- Modify: `tests/memes/gif.mac` (update to new API)
- Modify: `mac-lang/src/analyzer.ts` (add new type names to native symbols)

- [ ] **Step 1: Remove old native functions from NativeFunctions.h**

Remove these classes: `MemeFunction`, `AddTemplateFunction`, `GifMemeFunction`, `SaveGifFunction`, `MemeRemixCallable`, `MemeSaveCallable`, `MemeResizeCallable`, `GifAddFrameCallable`, `GifSaveCallable`.

Keep: `ResolveTemplateFunction`, `MemeRenderSaveFunction`, `GifRenderSaveFunction` (from Task 3).

- [ ] **Step 2: Update Interpreter.h**

Remove from constructor: `defn("meme", ...)`, `defn("addTemplate", ...)`, `defn("gifMeme", ...)`, `defn("saveGif", ...)`.

Remove from `visitGetExpr`: the `MacMeme` property block (lines ~199-223) and the `MacGif` property block (lines ~225-237). The meme/gif objects are now handled as regular MacInstance objects through the class system.

- [ ] **Step 3: Update tests/memes/basic.mac**

```mac
// tests/memes/basic.mac — updated to typed API
var drake = Template("drake");
var m = Meme(drake).text(Top, "Writing Java").text(Bottom, "Writing Mac");
print m._top;               // expect: Writing Java
print m._bottom;            // expect: Writing Mac
print m._template.name;     // expect: drake

// Position/Format types
print Top.name;             // expect: top
print PNG.name;             // expect: png
```

- [ ] **Step 4: Update tests/memes/images.mac**

```mac
// tests/memes/images.mac — updated to typed API
var drake = Template("drake");
var m = Meme(drake).text(Top, "Writing Java").text(Bottom, "Writing Mac");
m.save(PNG, "test_meme.png");
print "png saved";          // expect: png saved

var small = m.resize(Size(200, 200));
small.save(JPG, "test_small.jpg");
print "jpg saved";          // expect: jpg saved
```

- [ ] **Step 5: Update tests/memes/gif.mac**

```mac
// tests/memes/gif.mac — updated to typed API
var drake = Template("drake");
var m1 = Meme(drake).text(Top, "Frame 1");
var m2 = Meme(drake).text(Top, "Frame 2");

var g = Gif()
    .frame(m1, Duration(300))
    .frame(m2, Duration(300));
g.save("test_anim.gif");
print "gif saved";              // expect: gif saved
```

- [ ] **Step 6: Update LSP analyzer (mac-lang/src/analyzer.ts)**

Add the new stdlib types and constants to the native symbols registration:

```typescript
// In registerNatives(), add:
this.defineSymbol({ name: "Template", kind: "native", ... description: "Template(nameOrPath) — create meme template" });
this.defineSymbol({ name: "Meme", kind: "native", ... description: "Meme(template) — create meme builder" });
this.defineSymbol({ name: "Gif", kind: "native", ... description: "Gif() — create GIF builder" });
this.defineSymbol({ name: "Frame", kind: "native", ... description: "Frame(meme, duration) — animation frame" });
this.defineSymbol({ name: "Size", kind: "native", ... description: "Size(w, h) — pixel dimensions" });
this.defineSymbol({ name: "Duration", kind: "native", ... description: "Duration(ms) — time in milliseconds" });
this.defineSymbol({ name: "Position", kind: "native", ... description: "Position(name) — text position" });
this.defineSymbol({ name: "Format", kind: "native", ... description: "Format(name) — image format" });
this.defineSymbol({ name: "Top", kind: "native", ... description: "Position constant: top" });
this.defineSymbol({ name: "Bottom", kind: "native", ... description: "Position constant: bottom" });
this.defineSymbol({ name: "Center", kind: "native", ... description: "Position constant: center" });
this.defineSymbol({ name: "PNG", kind: "native", ... description: "Format constant: PNG" });
this.defineSymbol({ name: "JPG", kind: "native", ... description: "Format constant: JPG" });
this.defineSymbol({ name: "GIF", kind: "native", ... description: "Format constant: GIF" });
```

Remove old symbols: `meme`, `addTemplate`, `gifMeme`, `saveGif`.

- [ ] **Step 7: Build and run full test suite**

```bash
cmake -S . -B build && cmake --build build --clean-first && bash tests/run_tests.sh
```

All tests must pass.

- [ ] **Step 8: Rebuild LSP**

```bash
cd mac-lang && npx tsc
```

- [ ] **Step 9: Commit and push**

```bash
git add -A && git commit -m "feat: replace string-based meme API with type-safe builder pattern"
git push origin main
```
