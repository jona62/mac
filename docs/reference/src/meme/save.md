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

## Output Directory

Filenames without a path are written to `~/mac/output/`:

| Input | Output path |
|-------|-------------|
| `"hello.png"` | `~/mac/output/hello.png` |
| `"sub/hello.png"` | `./sub/hello.png` (relative to cwd) |
| `"/tmp/hello.png"` | `/tmp/hello.png` (absolute) |

The output directory is created automatically on first save.

## Supported Formats

| Extension | Format |
|-----------|--------|
| `.png` | PNG (lossless, default) |
| `.jpg` / `.jpeg` | JPEG (lossy) |
| `.gif` | GIF (animated or static) |

## Confirmation

On every successful save, the absolute output path is printed to stderr:

```
  Saved /Users/you/mac/output/hello.png
```

This appears in REPL, file execution, and piped modes without interfering with stdout.

## See Also

- [Meme Literals](./meme_literal.md)
- [GIF Animation](./gif.md)
