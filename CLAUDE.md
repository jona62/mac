# Mac Development Guide

## Project Overview

Mac (Meme as Code) is a C++23 tree-walk interpreter where memes are first-class citizens. The project has four components:

- **Interpreter** — C++ in `src/` and `include/`
- **VS Code Extension + LSP** — TypeScript in `mac-lang/`
- **Web GIF Studio** — Python backend + JS frontend in `webapp/`
- **Language Reference** — mdBook in `docs/reference/`

## Build

```bash
cmake -S . -B build && cmake --build build
```

## Test

```bash
bash tests/run_tests.sh                        # 69 runtime tests
python3 tests/analyzer/run_analyzer_tests.py   # 41 analyzer tests
cd mac-lang && npx tsc --noEmit                # LSP typecheck
```

All three must pass before pushing.

## Development Checklist

Every change to the language — new syntax, new native functions, modified behavior — must update all affected layers. Use this checklist:

### Language Changes (new syntax, modified semantics)

- [ ] **Scanner** (`include/Scanner.h`) — new keywords or tokens
- [ ] **Parser** (`src/Parser.cpp`, `include/Parser.h`) — parsing rules
- [ ] **AST** (`include/Expr.h`, `include/Stmt.h`) — new expression/statement nodes
- [ ] **Interpreter** (`include/Interpreter.h`) — evaluation logic
- [ ] **Resolver** (`include/Resolver.h`) — variable resolution for new nodes
- [ ] **Runtime tests** — add `.mac` test files in `tests/` with `// expect:` annotations
- [ ] **Analyzer** (`include/AnalyzerWalk.h`, `include/AnalyzerInference.h`) — semantic analysis, type inference
- [ ] **Analyzer tests** — update `tests/analyzer/run_analyzer_tests.py`, regenerate snapshots with `--update`
- [ ] **Docs** — update `docs/reference/src/` pages, rebuild with `cd docs/reference && mdbook build`
- [ ] **AstPrinter** (`include/AstPrinter.h`) — add visitor stub for new nodes

### New Native Functions

- [ ] **Implementation** (`include/NativeFunctions.h`) — new `MacCallable` subclass
- [ ] **Registration** (`include/NativeRegistry.h`) — name, visibility, type, description, overloads
- [ ] **Runtime tests** — test the function in `tests/`
- [ ] **Docs** — add to `docs/reference/src/stdlib/native_functions.md`

The analyzer and LSP pick up native functions automatically from the registry.

### New Effects

- [ ] **Pixel transform** (`include/MemeEffects.h`) — implement the effect
- [ ] **Dispatch** (`include/NativeFunctions.h`) — add to `PartialEffect::call()` or `DirectEffect::call()`
- [ ] **Registration** (`include/NativeRegistry.h`)
- [ ] **Runtime tests**
- [ ] **Docs** — add to `docs/reference/src/meme/effects.md`

### New Templates

- [ ] **Image** — add to `assets/templates/`
- [ ] **Registration** (`include/MacMeme.h`) — add to `templateMap()`
- [ ] **Docs** — add to `docs/reference/src/stdlib/templates.md`

### Webapp Changes

- [ ] **Backend** (`webapp/server.py`) — API/script generation changes
- [ ] **Frontend** (`webapp/static/`) — UI changes
- [ ] **Syntax highlighting** (`webapp/static/js/highlight.js`) — new keywords

### Prelude Changes

- [ ] **Stdlib** (`stdlib/prelude.mac`) — class/function changes
- [ ] **Docs** — update `docs/reference/src/stdlib/prelude_classes.md`

## Versioning

Version lives in the `VERSION` file (single source of truth). CMake injects it as `MAC_VERSION` at build time. CI validates the tag matches on release.

- **Patch** (0.x.1) — bug fixes, internal refactors, test/doc/CI changes
- **Minor** (0.x.0) — new language features, syntax, native functions, analyzer capabilities
- **Major** (x.0.0) — breaking changes: removed syntax, changed semantics, incompatible stdlib

## Release Process

1. Update `VERSION` file
2. Commit
3. `git tag v<version> && git push origin v<version>`
4. CI builds binaries (macOS arm64, macOS x86_64, Linux x86_64), publishes GitHub Release
5. Deploy runs automatically on push to main (webapp + docs at macstudio.meme and docs.macstudio.meme)

## Architecture Notes

- `print` is a language keyword, not a native function
- `gif`, `grid` are contextual keywords (parsed as identifiers, checked in parser)
- Most implementation lives in headers due to C++ templates
- `MacValue` is a `std::variant` with 12+ alternatives
- The analyzer outputs JSON via `nlohmann/json` for LSP consumption
- `MacTimeline` (C++ class) is the internal engine for gif transitions — not exposed to users
- `_timeline_render` is the only surviving internal timeline native (used by `Gif.save()`)

## Code Style

- Every `#include` line has a comment explaining what it imports
- Prefer header-only implementation for new analyzer/interpreter features
- Follow existing patterns — check how similar features are implemented before adding new ones
- No `Co-Authored-By` trailers in commits

## Key Files

| File | Purpose |
|------|---------|
| `src/main.cpp` | CLI entry point |
| `include/Scanner.h` | Lexer, keywords |
| `src/Parser.cpp` | Recursive descent parser |
| `include/Expr.h` | Expression AST nodes |
| `include/Stmt.h` | Statement AST nodes |
| `include/Interpreter.h` | Tree-walk evaluator |
| `include/Resolver.h` | Variable resolution pass |
| `include/NativeFunctions.h` | All native function implementations |
| `include/NativeRegistry.h` | Native function registry (name, type, description) |
| `include/MacAnalyzer.h` | Semantic analyzer entry point |
| `include/AnalyzerWalk.h` | AST walk for analysis |
| `include/AnalyzerInference.h` | Type inference |
| `include/AnalyzerTypes.h` | Analysis output structs + JSON serialization |
| `stdlib/prelude.mac` | Standard library (loaded before user code) |
| `docs/reference/` | mdBook language reference |
| `webapp/server.py` | Web GIF studio backend |
| `VERSION` | Version source of truth |
