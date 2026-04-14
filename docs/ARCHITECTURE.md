# Architecture

Mac has three components: a **C++ interpreter**, a **VS Code extension** with LSP, and a **web GIF studio**.

## Directory Structure

```
mac-cpp/
├── src/                       # C++ source files
│   ├── main.cpp               # CLI entry point, REPL, --analyze mode
│   ├── Scanner.cpp            # Lexer
│   ├── Parser.cpp             # Recursive descent parser
│   ├── Interpreter.cpp        # Interpreter (thin — most logic in headers)
│   └── stb_impl.cpp           # stb library linkage
├── include/                   # C++ headers
│   ├── Token.h                # Token types (~55 types incl. AT, FAT_ARROW, EFFECT, STYLE)
│   ├── Scanner.h              # Lexer with column tracking
│   ├── Parser.h               # Parser declarations
│   ├── Expr.h                 # 24 expression AST nodes (incl. MemeLiteralExpr, SaveExpr, GifBlockExpr, TimelineBlockExpr, GridBlockExpr)
│   ├── Stmt.h                 # 14 statement AST nodes (incl. EffectStmt, StyleStmt)
│   ├── Resolver.h             # Variable scope resolution
│   ├── Interpreter.h          # Tree-walk interpreter + all visitors
│   ├── NativeRegistry.h       # Centralized native function definitions
│   ├── NativeFunctions.h      # Native function implementations
│   ├── MacValue.h             # Value variant (string, double, bool, nil, callable, instance, array, map, meme, gif, timeline)
│   ├── MacCallable.h          # Base callable interface
│   ├── MacFunction.h          # User-defined functions
│   ├── MacLambda.h            # Lambda / arrow functions
│   ├── MacClass.h             # Class definitions
│   ├── MacInstance.h           # Class instances
│   ├── MacArray.h / MacMap.h  # Collection types
│   ├── MacMeme.h              # Meme type + TextStyle struct + template registry
│   ├── MacGif.h               # GIF builder
│   ├── MacTimeline.h          # Timeline with 8 transitions + 4 easing curves
│   ├── MemeRenderer.h         # Text rendering (stb_truetype) with style support
│   ├── MemeEffects.h          # 18 pixel-level effects
│   ├── MemeLayout.h           # Layout combinators (beside, stack, grid)
│   ├── GifEncoder.h           # GIF89a encoder with LZW compression
│   ├── MacAnalyzer.h          # C++ analyzer for --analyze mode (LSP backend)
│   ├── AnalyzerTypes.h        # Analysis result structs + JSON serialization
│   ├── AnalyzerRegistry.h     # Native function registration for analyzer
│   ├── AnalyzerWalk.h         # AST walking + semantic token/hint collection
│   ├── AnalyzerInference.h    # Type inference engine
│   └── stb/                   # Vendored stb headers
├── stdlib/prelude.mac         # Standard library (classes, constants, presets)
├── assets/
│   ├── templates/             # 10 built-in template images
│   └── fonts/                 # Meme font (TTF)
├── tests/                     # 57 tests across 20 categories
├── examples/                  # 9 progressive examples (01-07 + 02b, 03b)
├── mac-lang/                  # VS Code extension + LSP
│   ├── src/server.ts          # LSP server (thin adapter over mac --analyze)
│   ├── src/extension.ts       # VS Code extension entry point
│   ├── src/legacy/            # Archived TS scanner/parser/analyzer
│   └── syntaxes/              # TextMate grammar
├── webapp/                    # Web GIF studio (Python + HTML/CSS/JS)
├── tools/gen_templates.cpp    # Template image generator
├── .claude/skills/            # Claude Code agent skill
└── .github/workflows/ci.yml   # CI/CD (test + lint + release + publish)
```

## Interpreter Pipeline

```
Source Code → Scanner → Parser → Resolver → Interpreter → Output
               tokens    AST      scopes     execution
```

### Scanner

Tokenizes source with column tracking. Handles `@`, `=>`, `---`, `|>`, `>>`, `->`, and keywords `effect`, `style`. Contextual keywords `gif`, `timeline`, `grid` remain as identifiers (detected by lookahead in parser).

### Parser

Recursive descent with template-parameterized AST. Precedence chain:

```
expression → save (=>) → pipe (|>) → compose (>>) → assignment → logicalOr → ... → call → primary
```

Primary parses: literals, variables, lambdas, arrows, `@template` meme literals, `gif`/`timeline`/`grid` blocks.

### Interpreter

Visitor pattern over 24 expression types and 14 statement types. v2 syntax visitors (`visitMemeLiteralExpr`, `visitGifBlockExpr`, etc.) desugar to the same runtime operations as the classic builder API.

The binary resolves `stdlib/` and `assets/` relative to its own location (not cwd) so it works from any directory after installation.

## Meme Subsystem

### Templates (10 built-in)

`two_panel`, `three_panel`, `bottom_text`, `blank`, `dark`, `wide` (16:9), `tall` (9:16), `square` (1:1), `four_panel`, `caption_bar`. Custom templates via file path.

### Text Rendering (`MemeRenderer.h`)

Renders text with stb_truetype. Supports `TextStyle` for customizable color, outline width/color, shadow offset/color, and font size override. Auto-sizes text, word-wraps at 90% width, uppercase by default.

### Effects (`MemeEffects.h`) — 18 total

**Parameterized**: blur, pixelate, noise, saturate, contrast, brightness, jpeg, hueShift, glow, posterize, chromatic, threshold, tint

**Direct**: invert, sepia, sharpen, vignette, grayscale

Exposed via `ParamEffectCreator` (returns `PartialEffect`) and `DirectEffect`. All composable with `>>` and pipeable with `|>`.

### Animations (`MacTimeline.h`)

8 transitions: crossfade, slideLeft, slideRight, slideUp, slideDown, wipe, fadeBlack, zoom

4 easing curves: ease (smoothstep), easeIn (cubic), easeOut (inverse cubic), easeInOut

### Output

All user saves go to `~/mac/output/` via `toOutputPath()`. The directory is created automatically.

## LSP Architecture

The LSP uses **Architecture B**: the C++ binary is the single source of truth.

```
VS Code ←LSP→ server.ts (thin adapter) ←JSON→ mac --analyze file.mac
```

`mac --analyze` runs Scanner + Parser + MacAnalyzer and outputs JSON with: symbols, references, diagnostics, properties, folding ranges, semantic tokens, param hints, chain hints, signatures, classes.

The TS server (322 lines) maps JSON to LSP responses. No TS scanner/parser — zero language logic in TypeScript.

### LSP Features

Hover, go-to-definition (prelude + user code), inlay type hints, semantic tokens, document symbols, folding ranges, signature help, find all references, completion.

## Web GIF Studio (`webapp/`)

Python HTTP server generating v2 Mac scripts from browser UI. 10 templates, 12 effect presets, style customization, live script preview.

## CI/CD

GitHub Actions: test (Ubuntu + macOS), lint LSP (TypeScript), release (3 platforms), publish. Tagged releases (`v*`) produce downloadable binaries.
