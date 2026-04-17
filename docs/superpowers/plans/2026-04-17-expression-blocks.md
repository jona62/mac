# Expression Blocks Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `{ }` blocks expression-oriented — the last item without a trailing semicolon becomes the block's value, enabling implicit function returns, multi-statement match arms, and if/else expressions.

**Architecture:** Add a `tailExpr` field to `BlockStmt`. The parser detects when the last item in a block is an expression without a semicolon and stores it separately. `executeBlock()` changes from returning void to returning `MacValue` — the evaluated tail expression or nil. Function/lambda `call()` methods use this return value when no `Return` exception was thrown.

**Tech Stack:** C++23, Mac language interpreter

---

## File Structure

| File | Change |
|------|--------|
| `include/Stmt.h` | Add `tailExpr` to `BlockStmt` |
| `src/Parser.cpp` | Detect tail expression in `block()` |
| `include/Interpreter.h` | `executeBlock()` returns `MacValue`, functions use tail value |
| `include/MacFunction.h` | `call()` uses `executeBlock()` return value |
| `include/MacLambda.h` | `call()` uses `executeBlock()` return value |
| `include/Resolver.h` | Resolve `tailExpr` |
| `include/AnalyzerWalk.h` | Analyze `tailExpr` |
| `include/AnalyzerInference.h` | Infer block type from tail |
| Tests | New test file |
| Docs | Update reference |

---

### Task 1: Add tailExpr to BlockStmt

**Files:**
- Modify: `include/Stmt.h`

- [ ] **Step 1: Extend BlockStmt with tailExpr**

Change `BlockStmt` (lines 66-76) from:

```cpp
template <typename T>
class BlockStmt : public Stmt<T> {
public:
    BlockStmt(vector<shared_ptr<Stmt<T>>> statements)
        : statements(statements) {}

    void accept(shared_ptr<StmtVisitor<T>> visitor) override {
        visitor->visitBlockStmt(this);
    }

    vector<shared_ptr<Stmt<T>>> statements;
};
```

To:

```cpp
template <typename T>
class BlockStmt : public Stmt<T> {
public:
    BlockStmt(vector<shared_ptr<Stmt<T>>> statements,
              shared_ptr<expr::Expr<T>> tailExpr = nullptr)
        : statements(statements), tailExpr(std::move(tailExpr)) {}

    void accept(shared_ptr<StmtVisitor<T>> visitor) override {
        visitor->visitBlockStmt(this);
    }

    vector<shared_ptr<Stmt<T>>> statements;
    shared_ptr<expr::Expr<T>> tailExpr; // last expression without ; — block's value
};
```

- [ ] **Step 2: Build to verify no breakage**

Run: `cmake --build build 2>&1 | head -10`

Expected: Compiles — the new parameter has a default value of nullptr.

- [ ] **Step 3: Commit**

```bash
git add include/Stmt.h
git commit -m "refactor: add tailExpr field to BlockStmt for expression blocks"
```

---

### Task 2: Parser — detect tail expressions in blocks

**Files:**
- Modify: `src/Parser.cpp`

- [ ] **Step 1: Modify block() to detect tail expressions**

The current `block()` method (lines 413-436) calls `declaration()` for each item and wraps bare expressions in `ExpressionStmt` (which requires `;`). The change: when parsing the last item, if it's an expression followed by `}` (not `;`), capture it as the tail expression instead.

Replace the `block()` method with:

```cpp
template <typename T>
std::vector<shared_ptr<stmt::Stmt<T>>> Parser::block() {
    std::vector<shared_ptr<stmt::Stmt<T>>> statements;

    while (!isAtEnd() && peek().type != TokenType::RIGHT_BRACE) {
        // Array destructuring: var [a, b] = expr; or val [a, b] = expr;
        if ((peek().type == TokenType::VAR || peek().type == TokenType::VAL)
            && current + 1 < tokens.size()
            && tokens[current + 1].type == TokenType::LEFT_BRACKET) {
            bool isVal = peek().type == TokenType::VAL;
            advance(); // consume VAR/VAL
            auto stmts = varDestructuring<T>(isVal);
            for (auto& s : stmts) statements.push_back(s);
            continue;
        }
        auto decl = declaration<T>();
        if (decl != nullptr) {
            statements.push_back(decl);
        }
    }

    consume(TokenType::RIGHT_BRACE, "Expected '}' after block.");
    return statements;
}
```

Wait — the problem is that `declaration()` → `statement()` → `expressionStatement()` already consumes the `;`. By the time we know the expression lacks a `;`, the parser has already errored.

The fix needs to happen in `expressionStatement()` or in `statement()`. The approach:

In `statement()`, when we're about to call `expressionStatement()`, check if the expression is followed by `}` instead of `;`. If so, don't consume the `;` and instead return a special marker.

Actually, the cleanest approach: modify `block()` itself. Before calling `declaration()`, peek ahead. If the next construct looks like an expression and the token after it is `}`, parse it as a tail expression.

But we can't easily "peek ahead past an expression" without parsing it.

**Better approach:** Add a `blockTailExpr` field to the Parser class. In `expressionStatement()`, when the next token is `}` instead of `;`, set `blockTailExpr` instead of consuming `;`:

```cpp
// Add to Parser class:
std::shared_ptr<expr::Expr<value::MacValue>> blockTailExpr = nullptr;
```

Modify `expressionStatement()`:

```cpp
template <typename T>
shared_ptr<stmt::Stmt<T>> Parser::expressionStatement() {
    auto expr = expression<T>();
    // If next token is } (block end), this is a tail expression
    if (peek().type == TokenType::RIGHT_BRACE) {
        blockTailExpr = expr;
        return nullptr; // signal to block() that we have a tail
    }
    consume(TokenType::SEMICOLON, "Expected ';' after expression.");
    return make_shared<stmt::ExpressionStmt<T>>(expr);
}
```

Modify `block()` to use this:

```cpp
template <typename T>
std::vector<shared_ptr<stmt::Stmt<T>>> Parser::block() {
    std::vector<shared_ptr<stmt::Stmt<T>>> statements;
    blockTailExpr = nullptr;

    while (!isAtEnd() && peek().type != TokenType::RIGHT_BRACE) {
        // Array destructuring
        if ((peek().type == TokenType::VAR || peek().type == TokenType::VAL)
            && current + 1 < tokens.size()
            && tokens[current + 1].type == TokenType::LEFT_BRACKET) {
            bool isVal = peek().type == TokenType::VAL;
            advance();
            auto stmts = varDestructuring<T>(isVal);
            for (auto& s : stmts) statements.push_back(s);
            continue;
        }
        auto decl = declaration<T>();
        if (decl != nullptr) {
            statements.push_back(decl);
        }
        // If blockTailExpr was set, we hit a tail expression — stop
        if (blockTailExpr) break;
    }

    consume(TokenType::RIGHT_BRACE, "Expected '}' after block.");
    return statements;
}
```

But `block()` returns just statements — it doesn't return the tail. The caller constructs BlockStmt. Let me check all callers of `block()`.

The callers construct `BlockStmt` from the returned vector. We need to change the callers to also pass `blockTailExpr`. Since `blockTailExpr` is a field on Parser, callers can read it after `block()` returns.

Find every place where `BlockStmt` is constructed from `block()` results and pass `blockTailExpr`:

Typical pattern:
```cpp
consume(TokenType::LEFT_BRACE, "...");
auto body = block<T>();
// Change from:
auto blockStmt = make_shared<stmt::BlockStmt<T>>(body);
// To:
auto blockStmt = make_shared<stmt::BlockStmt<T>>(body, blockTailExpr);
blockTailExpr = nullptr;
```

Search the parser for all `BlockStmt` constructions and `block<T>()` calls.

- [ ] **Step 2: Add blockTailExpr field to Parser.h**

```cpp
// In Parser class private section:
std::shared_ptr<expr::Expr<value::MacValue>> blockTailExpr = nullptr;
```

Wait — Parser is templated. The field type needs to work with the template. Actually, Parser is not templated — its methods are. But Expr is templated. Since the Parser only ever gets instantiated with `MacValue`, using a concrete type is fine. But to be safe, use `void*` or store it differently.

Actually, looking at the code, Parser already stores non-templated state. And `blockTailExpr` is set inside `expressionStatement<T>()` which knows the type. The issue is that `expressionStatement` is templated but Parser's fields are not.

**Simplest solution:** Don't use a field. Instead, have `block()` return a struct:

```cpp
template <typename T>
struct BlockResult {
    std::vector<shared_ptr<stmt::Stmt<T>>> statements;
    shared_ptr<expr::Expr<T>> tailExpr;
};

template <typename T>
BlockResult<T> Parser::block();
```

Then every caller gets both pieces. This is cleaner.

- [ ] **Step 3: Implement and update all callers**

Change `block()` return type and update every caller to handle the tail expression. The key callers are:
- `statement()` — for standalone `{ }` blocks
- `ifStatement()` — for if/else bodies
- `whileStatement()`, `forStatement()` — for loop bodies
- `functionDeclaration()` — for function bodies
- `classDeclaration()` — for class method bodies

For loop bodies and class method bodies, the tail expression should be ignored (loops don't return values, class methods use explicit return). For function/lambda bodies and standalone blocks, propagate it.

- [ ] **Step 4: Build**

Run: `cmake --build build 2>&1 | head -20`

- [ ] **Step 5: Commit**

```bash
git add src/Parser.cpp include/Parser.h
git commit -m "feat: parser detects tail expressions in blocks"
```

---

### Task 3: Interpreter — executeBlock returns MacValue

**Files:**
- Modify: `include/Interpreter.h`

- [ ] **Step 1: Change executeBlock signature**

Change from:
```cpp
void executeBlock(const std::vector<shared_ptr<stmt::Stmt<MacValue>>>& statements,
                  shared_ptr<environment::Environment> blockEnv)
```

To:
```cpp
MacValue executeBlock(const std::vector<shared_ptr<stmt::Stmt<MacValue>>>& statements,
                      shared_ptr<environment::Environment> blockEnv,
                      expr::Expr<MacValue>* tailExpr = nullptr)
```

Implementation:
```cpp
MacValue executeBlock(const std::vector<shared_ptr<stmt::Stmt<MacValue>>>& statements,
                      shared_ptr<environment::Environment> blockEnv,
                      expr::Expr<MacValue>* tailExpr = nullptr) {
    auto previousEnv = env;
    MacValue result = std::monostate{};
    try {
        env = blockEnv;
        for (auto& statement : statements) {
            execute(statement);
        }
        if (tailExpr) {
            result = evaluate(tailExpr);
        }
        env = previousEnv;
    } catch (...) {
        env = previousEnv;
        throw;
    }
    return result;
}
```

- [ ] **Step 2: Update visitBlockStmt to propagate tail value**

The challenge: `visitBlockStmt` returns void (StmtVisitor pattern). But we need the block's value to be accessible. Store it in a field:

```cpp
// Add to Interpreter class:
MacValue lastBlockValue = std::monostate{};
```

```cpp
void visitBlockStmt(stmt::BlockStmt<MacValue>* stm) override {
    auto blockEnv = make_shared<environment::Environment>(env);
    lastBlockValue = executeBlock(stm->statements, blockEnv, stm->tailExpr.get());
}
```

For cases where blocks are used as expressions (match arms, if/else, assignments), the caller reads `lastBlockValue`.

- [ ] **Step 3: Update function call to use tail value**

In `MacFunction::call()` (Interpreter.h lines 1069-1085):

```cpp
inline value::MacValue callable::MacFunction::call(
    std::shared_ptr<interpreter::Interpreter> interpreter,
    std::vector<value::MacValue> arguments)
{
    auto funcEnv = std::make_shared<environment::Environment>(closure);
    for (size_t i = 0; i < declaration->params.size(); i++) {
        funcEnv->define(std::get<std::string>(declaration->params[i].lexeme), arguments[i]);
    }

    try {
        auto result = interpreter->executeBlock(declaration->body, funcEnv, declaration->tailExpr.get());
        return result; // implicit return from tail expression
    } catch (const errors::Return& returnValue) {
        return returnValue.returnValue;
    }

    return std::monostate{};
}
```

Wait — `declaration->body` is a `vector<shared_ptr<Stmt>>`, and `declaration->tailExpr` doesn't exist because `FunctionStmt` stores the body directly, not as a `BlockStmt`. Let me check.

Actually, `FunctionStmt` stores `vector<shared_ptr<Stmt<T>>> body` directly (not a BlockStmt). So the function body is just a flat list of statements. The tail expression needs to be stored on `FunctionStmt` too.

**Add to FunctionStmt in Stmt.h:**
```cpp
shared_ptr<expr::Expr<T>> tailExpr; // implicit return expression
```

When the parser parses a function body, it calls `block()` which now returns both statements and tailExpr. Store the tail in the FunctionStmt.

Similarly for `MacLambda` — it stores `body` as a flat vector. Add `tailExpr` field.

- [ ] **Step 4: Update MacLambda::call()**

Same pattern as MacFunction — use the tail expression value when no Return exception is thrown.

- [ ] **Step 5: Build and test**

```bash
cmake --build build && echo 'fun add(a, b) { a + b } print add(2, 3);' | ./build/mac
```

Expected: `5`

- [ ] **Step 6: Commit**

```bash
git add include/Interpreter.h include/Stmt.h include/MacFunction.h include/MacLambda.h
git commit -m "feat: executeBlock returns tail expression value, implicit function returns"
```

---

### Task 4: Resolver and analyzer updates

**Files:**
- Modify: `include/Resolver.h`
- Modify: `include/AnalyzerWalk.h`
- Modify: `include/AnalyzerInference.h`

- [ ] **Step 1: Resolve tailExpr in blocks**

In `include/Resolver.h`, `visitBlockStmt()`:

```cpp
void visitBlockStmt(stmt::BlockStmt<MV>* stm) override {
    beginScope();
    resolve(stm->statements);
    if (stm->tailExpr) resolveExpr(stm->tailExpr);
    endScope();
}
```

Also resolve tailExpr in FunctionStmt body handling (wherever the function body is resolved — check `visitFunctionStmt`).

- [ ] **Step 2: Analyze tailExpr in blocks**

In `include/AnalyzerWalk.h`, block handling:

```cpp
} else if (auto* p = dynamic_cast<stmt::BlockStmt<MV>*>(s)) {
    beginScope();
    for (auto& st : p->statements) analyzeStmt(st.get());
    if (p->tailExpr) analyzeExpr(p->tailExpr.get());
    endScope();
```

- [ ] **Step 3: Build**

Run: `cmake --build build 2>&1 | head -10`

- [ ] **Step 4: Commit**

```bash
git add include/Resolver.h include/AnalyzerWalk.h include/AnalyzerInference.h
git commit -m "feat: resolve and analyze tail expressions in blocks"
```

---

### Task 5: Tests

**Files:**
- Create: `tests/expressions/expression_blocks.mac`

- [ ] **Step 1: Create test file**

```mac
// Block as expression
val x = {
    val a = 1;
    val b = 2;
    a + b
};
print x;                              // expect: 3

// Semicolon suppresses value
val y = {
    42;
};
print y;                              // expect: nil

// Function implicit return
fun add(a, b) {
    a + b
}
print add(2, 3);                      // expect: 5

// Explicit return still works
fun sub(a, b) {
    return a - b;
}
print sub(10, 3);                     // expect: 7

// Semicolon in function = nil return
fun noop() {
    42;
}
print noop();                         // expect: nil

// Nested blocks
val z = {
    val inner = {
        10 + 20
    };
    inner * 2
};
print z;                              // expect: 60

// Match with expression block arms
enum Result { Ok(value), Error(message) }
val msg = match Result.Ok(5) {
    Result.Ok(v) -> {
        val doubled = v * 2;
        "result: {doubled}"
    }
    Result.Error(e) -> {
        "error: {e}"
    }
};
print msg;                            // expect: result: 10

// Lambda implicit return
val mul = (a, b) -> {
    val result = a * b;
    result
};
print mul(3, 4);                      // expect: 12

// if/else as expression
val label = if (true) { "yes" } else { "no" };
print label;                          // expect: yes

// Mixed: some arms block, some inline
val v = match 2 {
    1 -> "one"
    2 -> {
        val s = "tw";
        s + "o"
    }
    _ -> "other"
};
print v;                              // expect: two

print "expression blocks ok";         // expect: expression blocks ok
```

- [ ] **Step 2: Run tests**

Run: `bash tests/run_tests.sh`

Expected: All tests pass.

- [ ] **Step 3: Run analyzer tests**

Run: `python3 tests/analyzer/run_analyzer_tests.py --update && python3 tests/analyzer/run_analyzer_tests.py`

- [ ] **Step 4: Commit**

```bash
git add tests/expressions/expression_blocks.mac tests/analyzer/
git commit -m "test: expression blocks — implicit returns, block values, match arms"
```

---

### Task 6: Docs and version bump

**Files:**
- Modify: `docs/reference/src/language/functions.md`
- Modify: `docs/reference/src/language/control_flow.md`
- Modify: `docs/reference/src/language/variables.md`
- Modify: `VERSION`

- [ ] **Step 1: Update functions.md**

Add an "Implicit Return" section explaining that the last expression without a semicolon is the return value.

- [ ] **Step 2: Update control_flow.md**

Add note that if/else can be used as an expression when branches are blocks with tail expressions. Update match section to show multi-statement block arms.

- [ ] **Step 3: Update variables.md**

Add example of block expressions in variable binding: `val x = { compute(); result };`

- [ ] **Step 4: Build docs**

Run: `cd docs/reference && mdbook build`

- [ ] **Step 5: Bump version to 0.7.0**

```bash
echo "0.7.0" > VERSION
```

- [ ] **Step 6: Run full verification**

```bash
bash tests/run_tests.sh
python3 tests/analyzer/run_analyzer_tests.py
cd mac-lang && npx tsc --noEmit
```

- [ ] **Step 7: Commit**

```bash
git add docs/reference/ VERSION
git commit -m "feat: expression blocks — implicit returns, block-as-expression"
```
