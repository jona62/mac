# Mac Language VS Code Extension

## Purpose

Provide syntax highlighting, language configuration, and code snippets for `.mac` files in VS Code, making it easier to write and read Mac programs.

## Location

`mac-lang/` subdirectory inside the `mac-cpp` repo root.

## Structure

```
mac-lang/
├── package.json
├── language-configuration.json
├── syntaxes/
│   └── mac.tmLanguage.json
├── snippets/
│   └── mac.snippets.json
└── README.md
```

## Syntax Highlighting

TextMate grammar (`mac.tmLanguage.json`) with these scopes:

| Token | TextMate Scope |
|-------|---------------|
| `var`, `fun`, `class`, `for`, `while`, `if`, `else`, `return`, `print` | `keyword.control.mac` |
| `and`, `or` | `keyword.operator.logical.mac` |
| `super`, `this` | `variable.language.mac` |
| `true`, `false` | `constant.language.boolean.mac` |
| `nil` | `constant.language.nil.mac` |
| `123`, `3.14` | `constant.numeric.mac` |
| `"..."` | `string.quoted.double.mac` |
| `// ...` | `comment.line.double-slash.mac` |
| `+`, `-`, `*`, `/`, `!`, `=`, `==`, `!=`, `<`, `>`, `<=`, `>=` | `keyword.operator.mac` |
| `(`, `)`, `{`, `}` | `punctuation.mac` |
| Function name after `fun` | `entity.name.function.mac` |
| Function calls `name(` | `entity.name.function.call.mac` |
| Class name after `class` | `entity.name.type.class.mac` |

## Language Configuration

- Comment toggling: `//`
- Auto-closing pairs: `()`, `{}`, `""`
- Surrounding pairs: `()`, `{}`, `""`
- Bracket matching: `()`, `{}`
- Indentation: increase after `{`, decrease after `}`
- Folding: brace-based

## Snippets

| Prefix | Description | Body |
|--------|-------------|------|
| `fun` | Function declaration | `fun name(params) { ... }` |
| `class` | Class with init | `class Name { init(params) { ... } }` |
| `for` | For loop | `for (var i = 0; i < count; i = i + 1) { ... }` |
| `while` | While loop | `while (condition) { ... }` |
| `if` | If statement | `if (condition) { ... }` |
| `ife` | If-else | `if (condition) { ... } else { ... }` |
| `print` | Print statement | `print expr;` |
| `var` | Variable declaration | `var name = value;` |

## Installation

Run `code --install-extension mac-lang/` from the repo root, or use the "Install from VSIX" option after packaging.

For development: symlink or copy `mac-lang/` into `~/.vscode/extensions/`.
