# Saving Output

## Save Operator

The `=>` operator saves a meme or GIF to a file.

```
expression => "filename.ext"
```

```
@blank "Hello" => "hello.png";
@blank "Photo" => "photo.jpg";
gif { @blank "A" : 500ms @blank "B" : 500ms } => "anim.gif";
```

The save operator returns `true` on success.

## save() Function

Equivalent to `=>` but as a function call.

```
var m = @blank "Hello";
save(m, "hello.png");
```

## Output Paths

When `MAC_OUTPUT_DIR` is not set, Mac resolves save paths like this:

| Input | Output path |
|-------|-------------|
| `"hello.png"` | `~/mac/output/hello.png` |
| `"sub/hello.png"` | `./sub/hello.png` (relative to cwd) |
| `"/tmp/hello.png"` | `/tmp/hello.png` (absolute) |

Any missing parent directories are created automatically on save.

## MAC_OUTPUT_DIR

Set `MAC_OUTPUT_DIR` to force all saves into a specific directory:

```bash
MAC_OUTPUT_DIR=/tmp/mac-out mac script.mac
```

When this override is set, Mac keeps only the filename portion of the requested path:

| Requested path | Actual output path |
|----------------|--------------------|
| `"hello.png"` | `/tmp/mac-out/hello.png` |
| `"sub/hello.png"` | `/tmp/mac-out/hello.png` |
| `"/tmp/hello.png"` | `/tmp/mac-out/hello.png` |

This is mainly useful for hosts like playgrounds, editors, and web apps that need all generated files to stay in one managed directory.

## Supported Formats

| Extension | Format |
|-----------|--------|
| `.png` | PNG (lossless, default) |
| `.jpg` / `.jpeg` | JPEG (lossy) |
| `.gif` | GIF (animated or static) |

## Confirmation

On every successful save, the actual absolute output path is printed to stderr:

```
  Saved /Users/you/mac/output/hello.png
```

This appears in REPL, file execution, and piped modes without interfering with stdout.

## See Also

- [Meme Literals](./meme_literal.md)
- [GIF Animation](./gif.md)
- [Command-Line Flags](../cli/flags.md)
