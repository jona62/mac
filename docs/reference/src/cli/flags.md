# Command-Line Flags

## Usage

```
mac [script]
mac --analyze <file>
mac --catalog[=<selector[,selector]>]
mac --uninstall
mac --version
```

## Modes

### Run Script

```bash
mac script.mac
```

Execute a `.mac` file. Bare filenames save to `~/mac/output/` by default, while explicit relative and absolute paths are respected. Temporary effect files are cleaned up after execution.

### Environment

#### `MAC_OUTPUT_DIR`

```bash
MAC_OUTPUT_DIR=/tmp/mac-out mac script.mac
```

Redirect all saves into a single directory. When this is set, Mac strips any path components from the requested save path and writes the file into `MAC_OUTPUT_DIR` using only its filename.

This is primarily intended for hosts like web apps and editors that need to control where generated files land. The saved confirmation still prints the actual absolute path so users can find the output immediately.

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

### Catalog

```bash
mac --catalog
mac --catalog=assets:meme
mac --catalog=layouts,style_presets,allowed_names
```

Output runtime catalog metadata as JSON. `mac --catalog` returns the full catalog; `--catalog=<selector>` returns one or more comma-separated subsets. Supported selectors include `templates`, `assets`, `assets:<category>`, `effects`, `effect_definitions`, `layouts`, `style_presets`, `limits`, and `allowed_names`.

Asset entries include stable IDs such as `meme.distracted_boyfriend` and relative asset paths such as `assets/templates/meme/distracted_boyfriend.jpg`. Image bytes are not embedded.

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

## See Also

- [Saving Output](../meme/save.md)
