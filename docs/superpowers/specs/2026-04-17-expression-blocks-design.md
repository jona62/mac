# Expression Blocks

## Goal

Make `{ }` blocks expression-oriented: the last item without a trailing semicolon becomes the block's value. This enables implicit function returns, multi-statement match arms, if/else as expressions, and block-scoped computations that produce values.

## The Rule

In any `{ }` block, if the last item is an expression without a trailing semicolon, that expression is the block's value. A trailing semicolon suppresses the value (block returns nil).

## Syntax

### Block as expression

```mac
val x = {
    val a = 1;
    val b = 2;
    a + b           // no semicolon → block value is 3
};
print x;            // 3
```

### Semicolon suppresses value

```mac
val y = {
    compute();      // semicolon → value discarded
};
print y;            // nil
```

### Function implicit return

```mac
fun add(a, b) {
    a + b           // last expression → return value
}
print add(2, 3);    // 5

// Explicit return still works
fun divide(a, b) {
    if (b == 0) return Result.Error("zero");
    Result.Ok(a / b)    // implicit return
}

// Semicolon means no implicit return
fun doStuff() {
    compute();      // returns nil
}
```

### Match with expression blocks

```mac
val result = match doStuff() {
    Result.Ok(v) -> {
        val processed = v * 2;
        processed + 1
    }
    Result.Error(e) -> {
        print "error: {e}";
        -1
    }
};
```

### if/else as expression

```mac
val msg = if (x > 0) {
    "positive"
} else {
    "non-positive"
};
```

## Implementation

### Parser

`BlockStmt` currently holds `vector<shared_ptr<Stmt<T>>> statements`. Change it to also hold an optional tail expression:

```cpp
class BlockStmt {
    vector<shared_ptr<Stmt<T>>> statements;
    shared_ptr<expr::Expr<T>> tailExpr; // nullptr if last item has semicolon
};
```

When parsing a block, if the last item is a bare expression (no semicolon, no `}` immediately after a statement keyword), capture it as `tailExpr` instead of wrapping it in `ExpressionStmt`.

The parser already distinguishes between `ExpressionStmt` (expression + `;`) and other statements. The change: when parsing inside a block and the next token after an expression is `}` (not `;`), treat it as the tail expression.

### Interpreter

`visitBlockStmt` currently returns void. It needs to propagate the tail expression value. Since `StmtVisitor::visitBlockStmt` returns void, the mechanism is: store the tail value somewhere accessible.

Approach: `visitBlockStmt` evaluates all statements, then evaluates `tailExpr` if present. The tail value is stored in a field on the interpreter (e.g., `lastBlockValue`) that callers can read.

For functions: `visitFunctionStmt` / `MacFunction::call` wraps the body execution. If no `Return` exception was thrown and the block has a tail expression, use the tail value as the return value.

For match arms: `visitMatchExpr` already evaluates the arm's result expression. If the result is a block expression, its tail value flows naturally through `evaluate()`.

Wait — blocks are statements, not expressions. To use a block as an expression (in `val x = { ... }`), we need a `BlockExpr` or we need to make `BlockStmt` usable as an expression context.

Simplest approach: add a `BlockExpr` expression node that wraps a `BlockStmt` and evaluates to its tail value. The parser emits `BlockExpr` when it sees `{` in expression position (after `=`, after `->`, etc.) and `BlockStmt` in statement position.

Actually, even simpler: keep `BlockStmt` as the only block type. When evaluating a `BlockStmt`, if it has a `tailExpr`, evaluate and store the result. The interpreter has a `MacValue blockResult` field. After `executeBlock()`, the caller can read it.

### Scope of changes

| Layer | Change |
|-------|--------|
| `include/Stmt.h` | Add `tailExpr` field to `BlockStmt` |
| `src/Parser.cpp` | Detect tail expression in `block()` when last item lacks `;` |
| `include/Interpreter.h` | `executeBlock()` returns `MacValue` (the tail value or nil) |
| `include/Interpreter.h` | `MacFunction::call()` uses block tail value as implicit return |
| `include/Interpreter.h` | `visitMatchExpr` — works naturally if match arm blocks flow through |
| `include/Resolver.h` | Resolve `tailExpr` in blocks |
| `include/AnalyzerWalk.h` | Analyze `tailExpr` |
| Tests | New tests for expression blocks |
| Docs | Update reference |

### REPL behavior

The REPL auto-appends `;` when the input doesn't end with `}`. So:
- `1 + 2` → appends `;` → expression statement, value discarded
- `{ val a = 1; a + 2 }` → no `;` appended → block with tail expression

This means blocks in the REPL naturally work without changes to REPL logic.

## What stays the same

- `return` keyword works for early returns
- Semicolons required for all non-tail statements
- All existing code continues to work (adding `tailExpr` is additive)
- `print`, `var`, `val`, `for`, `while` are statements
- The auto-semicolon behavior in the REPL is unchanged

## Backward compatibility

Fully backward compatible. Existing code with explicit `return` and semicolons everywhere works identically. The new behavior only activates when the last item in a block lacks a semicolon.

## Verification

1. `val x = { val a = 1; a + 2 }; print x;` → `3`
2. `fun add(a, b) { a + b } print add(2, 3);` → `5`
3. `fun f() { compute(); } print f();` → `nil` (semicolon suppresses)
4. `fun f() { return 42; } print f();` → `42` (explicit return still works)
5. `val x = match Result.Ok(1) { Result.Ok(v) -> { val d = v * 2; d } Result.Error(e) -> -1 }; print x;` → `2`
6. `val msg = if (true) { "yes" } else { "no" }; print msg;` → `yes`
7. All 79 runtime tests pass
8. All 41 analyzer tests pass
