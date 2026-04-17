# Command-Line Flags

## Usage

```
mac [script]
mac --analyze <file>
mac --uninstall
mac --version
```

## Modes

### Run Script

```bash
mac script.mac
```

Execute a `.mac` file. Output files go to `~/mac/output/`. Temporary effect files are cleaned up after execution.

### Interactive REPL

```bash
mac
```

Start the interactive prompt. See [REPL](./repl.md).

### Analyze (LSP)

```bash
mac --analyze file.mac
```

Output a JSON analysis of the file to stdout. Used by the VS Code extension for IDE features (diagnostics, completions, hover info, semantic highlighting).

The JSON contains 11 categories: `symbols`, `references`, `diagnostics`, `properties`, `foldingRanges`, `semanticTokens`, `paramHints`, `chainHints`, `signatures`, `classes`, `templates`.

### Version

```bash
mac --version
mac -v
```

Print the version string (e.g., `Mac v0.2.3`).

### Uninstall

```bash
mac --uninstall
```

Remove the Mac installation. See [Installation](./install.md#uninstall).
