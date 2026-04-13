# Mac

Meme as Code. A programming language where memes are first-class citizens.

## Install

```bash
curl -fsSL https://raw.githubusercontent.com/jona62/mac/main/install.sh | bash
```

Or build from source:

```bash
cmake -S . -B build && cmake --build build
```

## Memes

```mac
var m = Meme(Template("two_panel"))
    .text(Top, "Writing Java")
    .text(Bottom, "Writing Mac");

m.save(PNG, "meme.png");
```

### Templates

`two_panel` `three_panel` `bottom_text` `blank` — or use any image path: `Template("path/to/image.png")`

### Animated GIFs

```mac
var t = Template("two_panel");

Gif()
    .frame(Meme(t).text(Top, "Monday").text(Bottom, "Coding"), Duration(400))
    .frame(Meme(t).text(Top, "Friday").text(Bottom, "Deploying"), Duration(400))
    .save("week.gif");
```

### Operator Composition

```mac
// Meme + Duration = Frame
var f1 = Meme(t).text(Top, "Frame 1") + Duration(300);
var f2 = Meme(t).text(Top, "Frame 2") + Duration(300);

// Gif + Frame = Gif
(Gif() + f1 + f2).save("composed.gif");
```

### Meme Types

| Type | Example |
|------|---------|
| `Template` | `Template("two_panel")` |
| `Meme` | `Meme(tmpl).text(Top, "hi")` |
| `Size` | `Size(400, 300)` — supports `+`, `*` |
| `Duration` | `Duration(300)` — supports `+`, `*` |
| `Position` | `Top`, `Bottom`, `Center` |
| `Format` | `PNG`, `JPG`, `GIF` |
| `Frame` | `meme + Duration(300)` |
| `Gif` | `Gif().frame(m, dur).save(path)` |

## Language

Mac is a dynamically typed language with functions, closures, classes, arrays, maps, lambdas, and operator overloading.

See [LANGUAGE.md](LANGUAGE.md) for the full reference.

## VS Code Extension

The `mac-lang/` directory provides syntax highlighting, LSP (autocomplete, hover, go-to-definition, diagnostics), snippets, and file icons.

```bash
ln -sf "$(pwd)/mac-lang" ~/.vscode/extensions/mac-lang
```

## Development

```bash
cmake --build build && bash tests/run_tests.sh  # 37 tests
```

## License

MIT
