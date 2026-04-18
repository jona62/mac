# Mac

Meme as Code. A programming language where memes are first-class citizens.

```mac
@two_panel {
    top: "Me: it works on my machine"
    bottom: "Prod: lol no"
} |> glitch => "meme.png"
```

## Install

```bash
curl -fsSL https://raw.githubusercontent.com/jona62/mac/main/install.sh | bash
```

Or build from source:

```bash
cmake -S . -B build && cmake --build build
```

## Grammar

Every meme expression follows a composable grammar. Here's how all the pieces fit together:

### Meme Literal

```
@<template> [WxH] [style] { top: "..." bottom: "..." }  [|> effect]  [=> "path"]
@<template> [WxH] [style] "one-liner text"               [|> effect]  [=> "path"]
@"path/to/image.png" [WxH] [style] { ... }               [|> effect]  [=> "path"]
```

```mac
@blank "Hello"                                    // minimal
@two_panel { top: "A" bottom: "B" }               // block with positions
@square 800x800 { top: "Sized" }                  // with dimensions
@dark neon { top: "Styled" }                      // with style
@dark 800x800 neon { top: "All" } |> glitch       // size + style + effect
@"photo.png" { top: "Custom" } => "out.png"       // custom template + save
```

### Effect Preset

```
effect <name> = <effect> >> <effect> >> ... ;
```

```mac
effect glitch = pixelate(4) >> contrast(1.8) >> noise(0.2);
effect vintage = sepia >> brightness(0.9);
effect cyberpunk = hueShift(180) >> contrast(1.5) >> chromatic(3) >> glow(4);
```

### Style Block

```
style <name> {
    color: "#RRGGBB"
    outline: <pixels>
    outlineColor: "#RRGGBB"
    shadow: <offset>
    shadowColor: "#RRGGBBAA"
}
```

```mac
style neon {
    color: "#00FF41"
    outline: 4
    outlineColor: "#003300"
    shadow: 3
    shadowColor: "#00FF4180"
}
```

### GIF Block

```
gif [loop] {
    <meme> [|> effect] : <duration>
    <meme> [|> effect] : <duration>
    ...
} [=> "path"]
```

```mac
gif loop {
    @blank "3" : 500ms
    @blank "2" : 500ms
    @blank "GO!" : 1s
} => "countdown.gif"
```

### Timeline Block

```
timeline [loop] {
    <meme> [|> effect] : <duration>
    --- <transition> <duration> [easing] ---
    <meme> [|> effect] : <duration>
    ...
} [=> "path"]
```

```mac
timeline loop {
    @two_panel { top: "Act 1" } |> vintage : 3s
    --- crossfade 300ms ease ---
    @two_panel { top: "Act 2" } |> glitch : 3s
    --- fadeBlack 400ms ---
    @blank "fin." : 2s
} => "story.gif"
```

### Grid Block

```
grid <cols>x<rows> {
    <meme> [|> effect]
    <meme> [|> effect]
    ...
} [|> pad(n)] [|> border(n)] [=> "path"]
```

```mac
grid 2x2 {
    @blank "A" |> sepia
    @blank "B" |> glitch
    @blank "C" |> vintage
    @blank "D" |> deepfry
} |> pad(5) |> border(2) => "grid.png"
```

### Layout Operators

```
beside(<meme>, <meme>)     // side by side
stack(<meme>, <meme>)      // vertical
<expr> |> pad(<px>)        // padding
<expr> |> border(<px>)     // border
```

### Pipe & Compose

```
<expr> |> <fn>                          // pass as first arg
<expr> |> <fn>(args) |> <fn> ...        // chain
<fn> >> <fn> >> <fn>                    // compose into new function
```

### Functional Pipelines

```
<array> |> map(<fn>)                    // transform each
<array> |> filter(<fn>)                 // keep matches
<array> |> reduce(<fn>, <init>)         // fold to value
<array> |> zip(<array>)                 // pair elements
```

```mac
["Mon", "Tue", "Wed"]
    |> map(d -> @two_panel { top: d })
    |> reduce((g, m) -> g.frame(m, Duration(500)), Gif())
    |> save("week.gif")
```

### Save Operator

```
<expr> => "filename.png"       // save to ~/mac/output/
<expr> => "filename.gif"       // auto-detects format
<expr> => "subdir/file.png"    // explicit relative path
<expr> => "/tmp/file.png"      // explicit absolute path
```

If `MAC_OUTPUT_DIR` is set, saves are redirected there and only the filename portion is kept.

## Reference

| Element | Options |
|---------|---------|
| **Templates** | `two_panel` `three_panel` `bottom_text` `blank` `dark` `wide` `tall` `square` `four_panel` `caption_bar` or `@"path"` |
| **Positions** | `top` `bottom` `center` |
| **Effects (param)** | `blur(n)` `pixelate(n)` `noise(n)` `saturate(n)` `contrast(n)` `brightness(n)` `jpeg(n)` `hueShift(n)` `glow(n)` `posterize(n)` `chromatic(n)` `threshold(n)` `tint(n)` |
| **Effects (direct)** | `invert` `sepia` `sharpen` `vignette` `grayscale` `deepfry` |
| **Transitions** | `crossfade` `slideLeft` `slideRight` `slideUp` `slideDown` `wipe` `fadeBlack` `zoom` |
| **Easing** | `ease` `easeIn` `easeOut` `easeInOut` |
| **Durations** | `400ms` `2s` `Duration(400)` |
| **Sizes** | `800x600` `Size(800, 600)` |

## Language

Mac is a dynamically typed language with functions, closures, classes, arrays, maps, arrow functions, pipes, and operator overloading.

See [LANGUAGE.md](LANGUAGE.md) for the full reference.

## VS Code Extension

Syntax highlighting, LSP (hover, go-to-definition, inlay hints, semantic tokens, signature help, find references, document symbols), snippets, and file icons.

```bash
cd mac-lang && npm install && npx tsc && cd ..
ln -sf "$(pwd)/mac-lang" ~/.vscode/extensions/mac-lang
```

## Development

```bash
cmake --build build && bash tests/run_tests.sh
```

See [docs/BUILDING.md](docs/BUILDING.md) and [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## License

MIT
