# Mac Development Guide

## Skills

Read `.agent/skills/` for available skills. The `mac-language` skill covers all Mac syntax, templates, effects, and examples - use it when writing or explaining Mac programs.

## Project Overview

Mac (Meme as Code) is a C++23 tree-walk interpreter where memes are first-class citizens. The project has three components:

- **Interpreter** - C++ in `src/` and `include/`
- **VS Code Extension + LSP** - TypeScript in `mac-lang/`
- **Language Reference** - mdBook in `docs/reference/`

The Web GIF Studio and Code Playground live in a monorepo: [mac-studio-meme/mac-studio](https://github.com/mac-studio-meme/mac-studio). If developing locally, look for a sibling `mac-studio/` directory next to this repo. It contains `studio/` (GIF Studio), `playground/` (Code Playground), `shared/` (common JS/Python), `deploy/`, and `tests/`.

## Web Properties

| Property | URL | Purpose |
|----------|-----|---------|
| GIF Studio | [macstudio.meme](https://macstudio.meme) | Visual meme/GIF editor |
| Playground | [playground.macstudio.meme](https://playground.macstudio.meme) | Browser code editor with Monaco, live analysis |
| Docs | [docs.macstudio.meme](https://docs.macstudio.meme) | Language reference (mdBook) |

### Playground Link Generation

Code only:
```bash
curl -s -X POST https://playground.macstudio.meme/api/share \
  -H 'Content-Type: application/json' \
  -d '{"code": "print \"Hello, Mac!\";"}'
# Returns: {"ok": true, "url": "https://playground.macstudio.meme/#code=..."}
```

Code with images (multipart):
```bash
curl -s -X POST https://playground.macstudio.meme/api/share \
  -F 'code=@meme.user "caption" => "out.png";' \
  -F 'image=@photo.jpg'
# Returns: {"ok": true, "url": "...#code=...&images=abc123", "images": [...]}
```

Limits (defined in `mac-studio/shared/python/mac_shared/limits.py`):
- Max code: 50 KB
- Max image: 5 MB per file, .png/.jpg/.jpeg/.gif only
- Max 3 images per share request

## Build

```bash
cmake -S . -B build && cmake --build build
```

## Test

```bash
bash tests/run_tests.sh                        # runtime tests
python3 tests/analyzer/run_analyzer_tests.py   # analyzer tests
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
| Positional strings | `@two_panel "top" "bottom"` (1=center, 2=top+bottom, 3=top+bottom+center) | v0.8 |
| Custom images | `@"path/to/image.jpg" "text"` resolves relative to script dir | v0.1 |
| Meme assets | `@meme.shrek_smirk "text"` - dotted name resolves from assets/templates/ | v0.1 |
| Effects | `blur(5)`, `sepia`, `grayscale`, composable with `>>` | v0.1 |
| Grid layout | `grid 2x2 { ... }` with mismatch warnings (truncate or fill blank) | v0.1 |
| GIF animation | `gif loop { @tmpl "text" : 500ms }` with transitions and easing | v0.3 |
| Pipe operator | `value \|> func` | v0.4 |
| String interpolation | `"Hello, {name}!"` - any expression inside `{}` | v0.4 |
| Array destructuring | `val [a, b] = expr` and `for (var [k, v] in pairs)` | v0.4 |
| Match expressions | `match expr { pattern -> result }` with enum destructuring | v0.4 |
| Partial application | `partial(fn, arg)` creates pre-filled functions | v0.5 |
| Immutable bindings | `val x = 42` prevents reassignment | v0.5 |
| Enum sum types | `enum Name { Variant(fields) }` with match destructuring | v0.6 |
| Expression blocks | `{ stmts; tail_expr }` - last expression is block's value | v0.7 |
| Implicit returns | Functions return last expression without `return` | v0.7 |
| Trailing commas | Allowed in arrays, maps, and function calls | v0.8 |
| Escape sequences | `\n`, `\t`, `\\`, `\"`, `\r`, `\{` in strings | v0.8 |
| If/else expressions | `if (cond) expr else expr` usable anywhere an expression is expected | v0.9 |
| Save operator | `expr => "file.png"` writes output to `~/mac/output/` (or `MAC_OUTPUT_DIR`) | v0.1 |

### Available Templates

Built-in: `blank`, `dark`, `two_panel`, `three_panel`, `four_panel`, `bottom_text`, `caption_bar`, `square`, `wide`, `tall`

Meme assets (via `@meme.name`): `shrek_smirk`, `shrek_side_eye`, `girl_side_eye`, `king_bach_stare`, `jordan_crying`, `kid_crying`, `window_despair`, `guy_crying`, `idk_about_that`

### Available Effects

Direct (no args): `sepia`, `grayscale`, `invert`, `sharpen`, `vignette`

Parameterized: `blur(radius)`, `pixelate(size)`, `noise(amount)`, `contrast(factor)`, `brightness(factor)`, `hueShift(degrees)`, `glow(radius)`, `chromatic(offset)`

Layout: `pad(pixels)`, `border(width)`

Compose: `effect name = sepia >> contrast(1.5) >> vignette;`

### GIF Transitions

Types: `crossfade`, `slideLeft`, `slideRight`, `slideUp`, `slideDown`, `wipe`, `fadeBlack`, `zoom`

Easing: `linear`, `easeIn`, `easeOut`, `easeInOut`, `bounce`

Syntax: `--- crossfade 300ms easeInOut ---` between gif entries

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
- [ ] **Examples** - update `mac-studio/shared/js/mac-examples.js` if showcasing the feature

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

### New Templates / Meme Assets

- [ ] **Image** - add to `assets/templates/` (built-in) or `assets/templates/meme/` (meme asset)
- [ ] **Registration** (`include/MacMeme.h`) - add to `templateMap()` (built-in only; meme assets auto-resolve via dotted name)
- [ ] **Docs** - add to `docs/reference/src/stdlib/templates.md`

### Prelude Changes

- [ ] **Stdlib** (`stdlib/prelude.mac`) - class/function changes
- [ ] **Docs** - update `docs/reference/src/stdlib/prelude_classes.md`

## Versioning

Version lives in the `VERSION` file (single source of truth). CMake injects it as `MAC_VERSION` at build time. CI validates the tag matches on release.

- **Patch** (0.x.1) - bug fixes, internal refactors, test/doc/CI changes, security hardening
- **Minor** (0.x.0) - new language features, syntax, native functions, analyzer capabilities
- **Major** (x.0.0) - breaking changes: removed syntax, changed semantics, incompatible stdlib

## Release Process

1. Update `VERSION` file
2. Update `docs/reference/src/introduction.md` version
3. Update `mac-studio/*/static/index.html` fallback version badges
4. Commit: `release: vX.Y.Z`
5. `git tag v<version> && git push origin v<version>`
6. CI builds binaries (macOS arm64, macOS x86_64, Linux x86_64), publishes GitHub Release
7. Deploy runs automatically (docs.macstudio.meme via mac-cpp CI, apps via mac-studio CI)

## Architecture Notes

- `print` is a language keyword, not a native function
- `gif`, `grid` are contextual keywords (parsed as identifiers, checked in parser)
- Most implementation lives in headers due to C++ templates
- `MacValue` is a `std::variant` with 13 alternatives (string, double, bool, monostate, MacCallable, MacInstance, MacArray, MacMap, RenderSurface, MacMeme, MacGif, MacTimeline, MacEnum)
- The analyzer outputs JSON via `nlohmann/json` for LSP consumption
- `MacTimeline` is an internal C++ class for gif transition rendering - not exposed to users, used automatically by `gif { }` blocks with `--- transition ---` syntax
- `Environment` tracks `immutables` set for `val` enforcement
- `pendingTailExpr_` parser field communicates tail expressions for implicit returns
- `MAC_OUTPUT_DIR` env var overrides default output directory (`~/mac/output/`)
- Output paths are sanitized (directory components stripped) to prevent path traversal
- Template resolution validates canonical paths stay under script/binary directories
- Max image dimensions clamped to 4096x4096, max GIF frames capped at 200

## Code Style

- Every `#include` line has a comment explaining what it imports
- Prefer header-only implementation for new analyzer/interpreter features
- Follow existing patterns - check how similar features are implemented before adding new ones
- No `Co-Authored-By` trailers in commits
- No emdashes - use regular dashes

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
| `include/MacMeme.h` | Template map, resolveTemplate, path traversal protection |
| `include/MacGif.h` | GIF frame storage, frame limit (200 max) |
| `include/GifLimits.h` | `MAX_GIF_FRAMES` constant |
| `include/MacAnalyzer.h` | Semantic analyzer entry point |
| `include/AnalyzerWalk.h` | AST walk for analysis (match binding scope fix) |
| `include/AnalyzerInference.h` | Type inference |
| `include/AnalyzerTypes.h` | Analysis output structs + JSON serialization |
| `stdlib/prelude.mac` | Standard library (loaded before user code) |
| `docs/reference/` | mdBook language reference |
| `mac-lang/syntaxes/mac.tmLanguage.json` | TextMate grammar for VS Code + Monaco |
| `VERSION` | Version source of truth |
