# Mac Language Server Protocol

The Mac VS Code extension provides an LSP server with diagnostics, completions, hover info, and go-to-definition for `.mac` files.

## Setup

```bash
cd mac-lang
npm install
npx tsc
```

Then in VS Code, reload the window (`Cmd+Shift+P` > "Reload Window") to activate the extension.

## Features

### Diagnostics

Errors and warnings appear inline as you type. The analysis pipeline runs on every document change:

1. **Scanner** — catches unterminated strings and unexpected characters
2. **Parser** — catches syntax errors (missing semicolons, unmatched brackets, etc.)
3. **Analyzer** — catches semantic warnings (undefined variables, unknown functions)

| Diagnostic | Severity | Example |
| --- | --- | --- |
| Unterminated string | Error | `"hello` |
| Unexpected character | Error | `@` |
| Parse error | Error | `var x = ;` |
| Undefined variable | Warning | `print y;` (when `y` is not declared) |

The analyzer knows all 62 native functions, prelude types (`Meme`, `Gif`, `Template`, `Duration`, `Size`, `Position`, `Format`, `Frame`), and prelude constants (`Top`, `Bottom`, `Center`, `PNG`, `JPG`, `GIF`, `crossfade`, `slideLeft`, etc.). References to these will not produce false "undefined variable" warnings.

### Completions

Triggered automatically and on `.` (dot):

- **Keywords** — `var`, `fun`, `class`, `if`, `while`, `for`, `return`, `print`, `true`, `false`, `nil`, `and`, `or`, `break`, `continue`, `this`, `super`, `else`, `in`
- **Symbols** — all variables, functions, and classes visible in the current scope
- **Dot completions** — type-aware property/method suggestions:

| Type | Completions |
| --- | --- |
| `Meme` | `.text`, `.save`, `.resize` |
| `Gif` | `.frame`, `.save` |
| `Template` | `.name`, `.path` |
| `Size` | `.width`, `.height` |
| `Duration` | `.ms` |

### Hover

Hover over any symbol to see its type and description:

- **Native functions** — shows signature and description (e.g., `map(arr, fn)` — "Transform each element")
- **User functions** — shows `fun name(params)`
- **Classes** — shows `class name(params)`
- **Variables** — shows `var name: type`
- **Properties** — shows `.name` with owner type

### Go to Definition

`Cmd+Click` (or `F12`) on any user-defined symbol jumps to its definition. Works for variables, functions, classes, and method calls. Native built-in functions are excluded (they have no source location).

## Type Inference

The analyzer infers types to power hover info and completions:

| Expression | Inferred Type |
| --- | --- |
| `42`, `3.14` | `number` |
| `"hello"` | `string` |
| `true`, `false` | `bool` |
| `nil` | `nil` |
| `[1, 2, 3]` | `array<number>` |
| `{a: 1}` | `map<number>` |
| `Meme(t)` | `instance Meme` |
| `x + y` (numbers) | `number` |
| `x + y` (strings) | `string` |
| `x == y` | `bool` |
| `blur(5)` | `function` (partial effect) |
| `meme \|> sepia` | `instance Meme` |

Type information flows through pipes, compose chains, and method calls.

## Supported Syntax

The scanner and parser handle the full Mac language grammar:

- Literals: numbers, strings, booleans, nil, arrays, maps
- Operators: arithmetic, comparison, logical, pipe (`|>`), compose (`>>`), arrow (`->`)
- Statements: `var`, `print`, `if`/`else`, `while`, `for`/`for-in`, `break`, `continue`, `return`
- Functions: `fun` declarations, lambdas, arrow functions, closures
- Classes: `class` with `init`, methods, inheritance (`<`), `super`, `this`, operator overloading
- Indexing: `arr[i]`, `map["key"]`, `obj.field`

## Architecture

```
extension.ts    VS Code client — activates on .mac files, starts LSP
    |
server.ts       LSP server — registers capabilities, routes requests
    |
analyzer.ts     Semantic analysis — scope resolution, type inference, diagnostics
    |
parser.ts       AST construction — expressions, statements, classes
    |
scanner.ts      Tokenization — keywords, operators, literals, comments
```

All communication uses LSP over IPC. The server runs as a Node.js process managed by the VS Code language client.

## Extension Metadata

| Field | Value |
| --- | --- |
| Extension ID | `mac-lang` |
| File extension | `.mac` |
| VS Code version | >= 1.75.0 |
| Syntax grammar | `source.mac` (TextMate) |
| Includes | Syntax highlighting, snippets, file icon theme |
