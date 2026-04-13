# Mac Language Server Protocol (LSP)

## Purpose

Provide IDE intelligence for `.mac` files in VS Code — diagnostics, go-to-definition, hover info, and autocomplete — via an LSP server integrated into the existing `mac-lang` extension.

## Architecture

A TypeScript LSP server inside `mac-lang/`. The VS Code extension acts as the client, launching the server process on activation. The server re-implements a lightweight scanner and parser in TypeScript (analysis only, no interpreter).

## Components

| Component | File | Purpose |
|-----------|------|---------|
| Client | `src/extension.ts` | Launches LSP server, connects via stdio |
| Server | `src/server.ts` | LSP lifecycle, document sync, dispatches requests |
| Scanner | `src/scanner.ts` | Tokenizes Mac source into token stream |
| Parser | `src/parser.ts` | Builds AST from tokens, collects parse errors |
| Analyzer | `src/analyzer.ts` | Walks AST to build symbol table, resolve references, produce diagnostics |

## Features

### Diagnostics
- Run scanner + parser on every document change (debounced ~300ms)
- Report parse errors with line/column positions
- Report undeclared variable usage where detectable

### Go to Definition
- Analyzer builds a scope-aware symbol table: each identifier maps to its declaration location (file, line, column)
- Handles: variables (`var`), functions (`fun`), classes (`class`), parameters
- Walks scope chain for resolution (inner scopes shadow outer)

### Hover
- Shows symbol kind and context on hover
- Variables: `var name` with inferred info if available
- Functions: `fun name(param1, param2)` with parameter names
- Classes: `class Name` or `class Name < SuperName`
- Native functions: name + brief description (e.g., `len(x) — returns length of string or array`)

### Autocomplete
- Keywords: `var`, `fun`, `class`, `if`, `else`, `for`, `while`, `return`, `print`, `true`, `false`, `nil`, `and`, `or`, `this`, `super`, `break`, `continue`, `in`
- In-scope symbols: variables, functions, classes visible from cursor position
- Native stdlib: `len`, `substr`, `split`, `type`, `sqrt`, `abs`, `pow`, `floor`, `ceil`, `push`, `pop`, `map`, `filter`, `input`, `clock`, `meme`
- After `.`: suggest known properties/methods based on context (class methods, meme properties, map keys)

## File Structure

```
mac-lang/
├── package.json              (updated: LSP deps, activation, main entry)
├── tsconfig.json             (new: TypeScript config)
├── src/
│   ├── extension.ts          (new: client entry point)
│   ├── server.ts             (new: LSP server)
│   ├── scanner.ts            (new: Mac tokenizer in TS)
│   ├── parser.ts             (new: Mac parser in TS, AST types)
│   └── analyzer.ts           (new: symbol table, diagnostics)
├── language-configuration.json
├── syntaxes/mac.tmLanguage.json
├── snippets/mac.snippets.json
├── icons/
└── mac-icon-theme.json
```

## Dependencies

- `vscode-languageclient` — client-side (VS Code extension)
- `vscode-languageserver` — server-side
- `vscode-languageserver-textdocument` — document management
- `typescript` — build tool

## Build

- `npm install` in `mac-lang/`
- `npx tsc` compiles `src/*.ts` → `out/*.js`
- Extension loads `out/extension.js` as entry point

## Token/AST Design (TypeScript)

The TypeScript scanner and parser mirror the C++ versions but simplified for analysis:
- Scanner produces the same token types as `Token.h`
- Parser builds a typed AST (discriminated unions, not visitor pattern)
- No need for an interpreter — just structural analysis

## Scope

- Single-file analysis only (no cross-file imports since Mac has no module system)
- No runtime type inference (Mac is dynamically typed)
- No formatting or code actions in v1
