# Architecture

Mac has three components: a **C++ interpreter**, a **VS Code extension** with LSP, and a **web GIF studio**.

## Directory Structure

```
mac-cpp/
├── src/                   # C++ source files (entry point + implementations)
│   ├── main.cpp           # CLI entry point and REPL
│   ├── Scanner.cpp        # Lexer implementation
│   ├── Parser.cpp         # Parser implementation
│   ├── Interpreter.cpp    # Interpreter implementation
│   └── stb_impl.cpp       # stb library linkage
├── include/               # C++ headers
│   ├── Token.h            # Token types and lexemes
│   ├── Scanner.h          # Lexer (source -> tokens)
│   ├── Parser.h           # Parser (tokens -> AST)
│   ├── Expr.h / Stmt.h    # AST node definitions (template-parameterized)
│   ├── Resolver.h         # Variable resolution pass
│   ├── Interpreter.h      # Tree-walk interpreter + visitor
│   ├── NativeFunctions.h  # All 40+ built-in function classes
│   ├── MacValue.h         # Value type (std::variant with 10 alternatives)
│   ├── MacCallable.h      # Base class for callable objects
│   ├── MacFunction.h      # User-defined functions
│   ├── MacLambda.h        # Lambda / arrow function wrapper
│   ├── MacClass.h         # Class definitions
│   ├── MacInstance.h       # Class instances
│   ├── MacArray.h         # Array type
│   ├── MacMap.h           # Map type
│   ├── MacMeme.h          # Meme type (wraps pixel data)
│   ├── MacGif.h           # GIF builder type
│   ├── MacTimeline.h      # Timeline animation type
│   ├── MemeRenderer.h     # Text-on-image rendering (stb_truetype)
│   ├── MemeEffects.h      # Pixel-level image effects
│   ├── MemeLayout.h       # Layout combinators (beside, stack, grid)
│   ├── GifEncoder.h       # GIF89a encoder with LZW compression
│   ├── Environment.h      # Scope / variable binding
│   ├── AstPrinter.h       # Debug AST printer
│   ├── RuntimeError.h     # Runtime exception
│   ├── Return.h           # Return control flow exception
│   ├── ParserError.h      # Parse error exception
│   └── stb/               # Vendored stb headers (image I/O, fonts)
├── stdlib/prelude.mac     # Standard library (loaded before user code)
├── assets/
│   ├── templates/         # Built-in meme template images
│   └── fonts/             # Meme font (TTF)
├── tests/                 # Test suite (46 tests across 19 categories)
├── examples/              # Example programs
├── mac-lang/              # VS Code extension + LSP (TypeScript)
├── webapp/                # Web GIF studio (Python)
├── tools/                 # Build tools (template image generator)
└── .github/workflows/     # CI/CD pipeline
```

## Interpreter Pipeline

```
Source Code -> Scanner -> Parser -> Resolver -> Interpreter -> Output
               (tokens)   (AST)    (scopes)    (execution)
```

### Scanner (`include/Scanner.h` + `src/Scanner.cpp`)

Tokenizes source code. The scanner is an iterator — it yields tokens lazily. Handles all Mac operators including `|>` (pipe), `>>` (compose), and `->` (arrow).

### Parser (`include/Parser.h` + `src/Parser.cpp`)

Recursive descent parser producing a template-parameterized AST (`Expr<T>`, `Stmt<T>`). The template parameter is the value type used during interpretation (`MacValue`).

Key parsing decisions:
- Arrow functions are parsed in `primary()` with lookahead: single-param `IDENTIFIER ARROW` and multi-param `(ids) ARROW` with backtracking.
- Precedence chain: expression -> pipe -> compose -> assignment -> logicalOr -> ... -> primary.
- Explicit template instantiations at the bottom of `Parser.cpp` for all parser methods.

### Resolver (`include/Resolver.h`)

Static analysis pass that resolves variable scopes before execution. Walks the AST and records how many scopes deep each variable reference is. Header-only — includes `Interpreter.h` at line 292 (after closing its own namespace) to break a circular dependency.

### Interpreter (`include/Interpreter.h` + `src/Interpreter.cpp`)

Tree-walk interpreter implementing the Visitor pattern over all expression and statement types. Key behaviors:
- **Pipe operator** (`|>`): prepends the left-hand value as the first argument to the right-hand function call.
- **Compose operator** (`>>`): creates a `ComposedFunction` that chains two callables.
- **Operator overloading**: `visitBinaryExpr` checks for dunder methods (`__add__`, `__mul__`, etc.) on `MacInstance` before falling back to built-in operators.
- **Variable arity**: arity check is skipped when `arity() == -1` (used by `range`, `sort`).

The constructor registers all 40+ native functions via the `defn()` lambda.

## Meme Subsystem

### Templates and Rendering

`MemeRenderer.h` loads template images via `stb_image`, renders text using `stb_truetype` (white text with black outline, auto-sizing), and writes output via `stb_image_write`. Templates are PNG images in `assets/templates/`.

### Effects (`MemeEffects.h`)

Pixel-level transforms operating on RGBA buffers in-place: saturate, contrast, brightness, blur, sharpen, pixelate, invert, sepia, noise, vignette, jpeg quality simulation.

Effects are exposed as native functions in two patterns:
- `ParamEffectCreator` — takes a parameter, returns a `PartialEffect` callable (e.g., `blur(5)`)
- `DirectEffect` — no parameter, directly callable (e.g., `sepia`)

Both work with `|>` and `>>`.

### Layout (`MemeLayout.h`)

Compositing functions: `beside` (horizontal), `stack` (vertical), `grid` (cols x rows), `pad`, `border`. All operate on rendered pixel buffers.

### GIF Encoding (`GifEncoder.h`)

GIF89a format writer with:
- 6x6x6 color cube + 40 grayscale palette (256 entries)
- LZW compression with dictionary reset on table full
- Netscape looping extension for animated GIFs
- Per-frame delay in centiseconds

### Timeline (`MacTimeline.h`)

Keyframe-based animation with transitions (crossfade, slide, wipe). `renderFrames()` generates intermediate frames at ~15fps for smooth transitions between keyframes.

## Standard Library (`stdlib/prelude.mac`)

Loaded automatically before user code. Defines Mac-language classes:
- `Size`, `Duration` — value types with operator overloading (`__add__`, `__mul__`, `__eq__`)
- `Position` — `Top`, `Bottom`, `Center` constants
- `Format` — `PNG`, `JPG`, `GIF` constants
- `Template`, `Meme`, `Frame`, `Gif` — meme API types
- `deepfry` — composed effect preset
- Timeline API wrappers (`Timeline`, `at`, `transition`, `hold`, `render`, `loop`)
- Transition constants (`crossfade`, `slideLeft`, etc.)

These Mac-level classes call native C++ functions prefixed with `_` (e.g., `_resolve_template`, `_meme_save`, `_gif_save`).

## Native Functions (`include/NativeFunctions.h`)

All built-in functions are classes extending `MacCallable`. Organized by domain:
- **Core**: `clock`, `type`, `len`, `input`
- **Math**: `sqrt`, `abs`, `pow`, `floor`, `ceil`
- **Strings**: `substr`, `split`, `upper`, `lower`, `trim`, `replace`
- **Arrays**: `push`, `pop`, `map`, `filter`, `reduce`, `find`, `any`, `all`, `sort`, `reverse`, `flatten`, `flatMap`, `zip`, `enumerate`, `take`, `drop`, `join`, `each`, `range`
- **Effects**: `ParamEffectCreator` and `DirectEffect` wrappers
- **Layout**: `beside`, `stack`, `grid`, `pad`, `border`
- **Timeline**: `timeline`, `_timeline_keyframe`, `_timeline_transition`, `_timeline_hold`, `_timeline_loop`, `_timeline_render`
- **Meme bridge**: `animate`, `toGrid`

## VS Code Extension (`mac-lang/`)

Self-contained TypeScript project providing:
- Syntax highlighting via TextMate grammar (`syntaxes/mac.tmLanguage.json`)
- LSP server with hover, go-to-definition, autocomplete, diagnostics
- Code snippets and file icons

The LSP mirrors the C++ scanner/parser in TypeScript for real-time analysis. See `mac-lang/LSP.md` for details.

## Web GIF Studio (`webapp/`)

Python HTTP server (`server.py`) that:
1. Serves a browser UI for building animated memes
2. Accepts frame data via REST API (`POST /api/generate`)
3. Generates Mac source code from the request
4. Executes it via `subprocess.run([mac_binary, script_path])`
5. Returns the generated GIF

Launch with `PORT=9001 ./webapp/run.sh`.
