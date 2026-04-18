# Mac Development Guide

## Project Overview

Mac (Meme as Code) is a C++23 tree-walk interpreter where memes are first-class citizens. The project has three components:

- **Interpreter** - C++ in `src/` and `include/`
- **VS Code Extension + LSP** - TypeScript in `mac-lang/`
- **Language Reference** - mdBook in `docs/reference/`

The Web GIF Studio and Code Playground live in a monorepo: [mac-studio-meme/mac-studio](https://github.com/mac-studio-meme/mac-studio)

## Build

```bash
cmake -S . -B build && cmake --build build
```

## Test

```bash
bash tests/run_tests.sh                        # 83 runtime tests
python3 tests/analyzer/run_analyzer_tests.py   # 41 analyzer tests
cd mac-lang && npx tsc --noEmit                # LSP typecheck
```

All three must pass before pushing.

## Language Features

| Feature | Syntax | Since |
|---------|--------|-------|
| Dynamic typing | strings, numbers, booleans, nil, arrays, maps | v0.1 |
| Functions | `fun name(params) { body }` | v0.1 |
| Classes | `class Name { init() {} method() {} }` | v0.1 |
| Closures | Functions capture enclosing scope | v0.1 |
| Meme literals | `@template "text"` or `@template { top: "..." bottom: "..." }` | v0.1 |
| Positional strings | `@two_panel "top" "bottom"` (1=center, 2=top+bottom, 3=top+bottom+center) | v0.7 |
| Effects | `blur(5)`, `sepia`, composable with `>>` | v0.1 |
| Grid layout | `grid 2x2 { ... }` with mismatch warnings | v0.1 |
| GIF animation | `gif loop { @tmpl "text" : 500ms }` with transitions | v0.3 |
| Pipe operator | `value \|> func` | v0.4 |
| String interpolation | `"Hello, {name}!"` | v0.4 |
| Array destructuring | `val [a, b] = expr` | v0.4 |
| Match expressions | `match expr { pattern -> result }` | v0.4 |
| Partial application | `partial(fn, arg)` | v0.5 |
| Immutable bindings | `val x = 42` prevents reassignment | v0.5 |
| Enum sum types | `enum Name { Variant(fields) }` with match destructuring | v0.6 |
| Expression blocks | `{ stmts; tail_expr }` - last expression is block's value | v0.7 |
| Implicit returns | Functions return last expression without `return` | v0.7 |
| Trailing commas | Allowed in arrays, maps, and function calls | v0.7 |
| Escape sequences | `\n`, `\t`, `\\`, `\"`, `\r`, `\{` in strings | v0.7 |
| Save operator | `expr => "file.png"` writes output to disk | v0.1 |

## Development Checklist

Every change to the language - new syntax, new native functions, modified behavior - must update all affected layers. Use this checklist:

### Language Changes (new syntax, modified semantics)

- [ ] **Scanner** (`src/Scanner.cpp`, `include/Scanner.h`) - new keywords, tokens, escape sequences
- [ ] **Parser** (`src/Parser.cpp`, `include/Parser.h`) - parsing rules
- [ ] **AST** (`include/Expr.h`, `include/Stmt.h`) - new expression/statement nodes
- [ ] **Interpreter** (`include/Interpreter.h`) - evaluation logic
- [ ] **Resolver** (`include/Resolver.h`) - variable resolution for new nodes
- [ ] **Runtime tests** - add `.mac` test files in `tests/` with `// expect:` annotations
- [ ] **Analyzer** (`include/AnalyzerWalk.h`, `include/AnalyzerInference.h`) - semantic analysis, type inference
- [ ] **Analyzer tests** - update `tests/analyzer/run_analyzer_tests.py`, regenerate snapshots with `--update`
- [ ] **Docs** - update `docs/reference/src/` pages, rebuild with `cd docs/reference && mdbook build`
- [ ] **AstPrinter** (`include/AstPrinter.h`) - add visitor stub for new nodes
- [ ] **tmLanguage** - update `mac-lang/syntaxes/mac.tmLanguage.json` and sync to `mac-studio/playground/static/mac.tmLanguage.json`
- [ ] **Shared tokens** - update `mac-studio/shared/js/mac-tokens.js` if keywords/types/constants changed

### New Native Functions

- [ ] **Implementation** (`include/NativeFunctions.h`) - new `MacCallable` subclass
- [ ] **Registration** (`include/NativeRegistry.h`) - name, visibility, type, description, overloads
- [ ] **Runtime tests** - test the function in `tests/`
- [ ] **Docs** - add to `docs/reference/src/stdlib/native_functions.md`

The analyzer and LSP pick up native functions automatically from the registry.

### New Effects

- [ ] **Pixel transform** (`include/MemeEffects.h`) - implement the effect
- [ ] **Dispatch** (`include/NativeFunctions.h`) - add to `PartialEffect::call()` or `DirectEffect::call()`
- [ ] **Registration** (`include/NativeRegistry.h`)
- [ ] **Runtime tests**
- [ ] **Docs** - add to `docs/reference/src/meme/effects.md`

### New Templates

- [ ] **Image** - add to `assets/templates/`
- [ ] **Registration** (`include/MacMeme.h`) - add to `templateMap()`
- [ ] **Docs** - add to `docs/reference/src/stdlib/templates.md`

### Prelude Changes

- [ ] **Stdlib** (`stdlib/prelude.mac`) - class/function changes
- [ ] **Docs** - update `docs/reference/src/stdlib/prelude_classes.md`

## Versioning

Version lives in the `VERSION` file (single source of truth). CMake injects it as `MAC_VERSION` at build time. CI validates the tag matches on release.

- **Patch** (0.x.1) - bug fixes, internal refactors, test/doc/CI changes
- **Minor** (0.x.0) - new language features, syntax, native functions, analyzer capabilities
- **Major** (x.0.0) - breaking changes: removed syntax, changed semantics, incompatible stdlib

## Release Process

1. Update `VERSION` file
2. Commit
3. `git tag v<version> && git push origin v<version>`
4. CI builds binaries (macOS arm64, macOS x86_64, Linux x86_64), publishes GitHub Release
5. Docs deploy runs automatically on push to main (docs.macstudio.meme)

## Architecture Notes

- `print` is a language keyword, not a native function
- `gif`, `grid` are contextual keywords (parsed as identifiers, checked in parser)
- Most implementation lives in headers due to C++ templates
- `MacValue` is a `std::variant` with 13 alternatives (string, double, bool, monostate, MacCallable, MacInstance, MacArray, MacMap, RenderSurface, MacMeme, MacGif, MacTimeline, MacEnum)
- The analyzer outputs JSON via `nlohmann/json` for LSP consumption
- `MacTimeline` (C++ class) is the internal engine for gif transitions - not exposed to users
- `_timeline_render` is the only surviving internal timeline native (used by `Gif.save()`)
- `Environment` tracks `immutables` set for `val` enforcement
- `pendingTailExpr_` parser field communicates tail expressions for implicit returns
- `MAC_OUTPUT_DIR` env var overrides default output directory (`~/mac/output/`)

## Code Style

- Every `#include` line has a comment explaining what it imports
- Prefer header-only implementation for new analyzer/interpreter features
- Follow existing patterns - check how similar features are implemented before adding new ones
- No `Co-Authored-By` trailers in commits

## Key Files

| File | Purpose |
|------|---------|
| `src/main.cpp` | CLI entry point |
| `src/Scanner.cpp` | Lexer, string scanning, escape sequences |
| `include/Scanner.h` | Token types, keywords map |
| `src/Parser.cpp` | Recursive descent parser |
| `include/Expr.h` | Expression AST nodes |
| `include/Stmt.h` | Statement AST nodes |
| `include/Interpreter.h` | Tree-walk evaluator |
| `include/Resolver.h` | Variable resolution pass |
| `include/Environment.h` | Scopes, variable storage, `val` immutability |
| `include/NativeFunctions.h` | All native function implementations |
| `include/NativeRegistry.h` | Native function registry (name, type, description) |
| `include/MacEnum.h` | Enum sum type (MacEnumDef, MacEnum) |
| `include/MacLambda.h` | Lambda/closure with tail expression support |
| `include/MacMeme.h` | Template map, resolveTemplate, custom templates |
| `include/MacAnalyzer.h` | Semantic analyzer entry point |
| `include/AnalyzerWalk.h` | AST walk for analysis |
| `include/AnalyzerInference.h` | Type inference |
| `include/AnalyzerTypes.h` | Analysis output structs + JSON serialization |
| `stdlib/prelude.mac` | Standard library (loaded before user code) |
| `docs/reference/` | mdBook language reference |
| `mac-lang/syntaxes/mac.tmLanguage.json` | TextMate grammar for VS Code + Monaco |
| `VERSION` | Version source of truth |
