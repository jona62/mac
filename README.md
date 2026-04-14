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

## Quick Start

```mac
// Create a meme with @template
@two_panel {
    top: "Before Mac"
    bottom: "After Mac"
} => "before_after.png"

// One-liner
@blank "Hello from Mac!" => "hello.png"

// Specify size
@square 800x800 {
    top: "High res"
    bottom: "Square format"
} => "square.png"

// Use any image as a template
@"path/to/photo.png" {
    top: "Custom template"
} => "custom.png"
```

## Effects

Compose effect presets with `>>`, apply with `|>`.

```mac
effect glitch = pixelate(4) >> contrast(1.8) >> noise(0.2)
effect vintage = sepia >> brightness(0.9)
effect cyberpunk = hueShift(180) >> contrast(1.5) >> chromatic(3) >> glow(4)

@two_panel {
    top: "Normal"
    bottom: "Glitched"
} |> glitch => "glitched.png"
```

18 built-in effects: `blur`, `pixelate`, `noise`, `saturate`, `contrast`, `brightness`, `jpeg`, `invert`, `sepia`, `sharpen`, `vignette`, `grayscale`, `hueShift`, `glow`, `posterize`, `chromatic`, `threshold`, `tint`

## Animation

```mac
// GIF with frame timing
gif loop {
    @blank "3" : 500ms
    @blank "2" : 500ms
    @blank "1" : 500ms
    @blank "GO!" : 1s
} => "countdown.gif"

// Timeline with transitions and easing
timeline loop {
    @two_panel { top: "Act 1" } : 3s
    --- crossfade 200ms ease ---
    @two_panel { top: "Act 2" } : 3s
    --- fadeBlack 300ms ---
    @two_panel { top: "Act 3" } : 3s
} => "story.gif"
```

8 transitions: `crossfade`, `slideLeft`, `slideRight`, `slideUp`, `slideDown`, `wipe`, `fadeBlack`, `zoom`

4 easing curves: `ease`, `easeIn`, `easeOut`, `easeInOut`

## Functional Pipelines

```mac
var quotes = [
    ["Debugging", "print('here')"],
    ["Testing", "works on my machine"],
    ["Deploying", "YOLO"]
]

quotes
    |> map(q -> @two_panel { top: q[0] bottom: q[1] })
    |> reduce((tl, m) -> tl
        .frame(m, Duration(3000))
        .transition(crossfade, Duration(200)), Timeline())
    |> save("lifecycle.gif")
```

## Layout

```mac
// Effects comparison grid
grid 2x2 {
    @blank "Clean" |> sharpen
    @blank "Vintage" |> vintage
    @blank "Glitch" |> glitch
    @blank "Deepfry" |> deepfry
} |> pad(5) |> border(2) => "comparison.png"
```

## Templates

10 built-in: `two_panel`, `three_panel`, `bottom_text`, `blank`, `caption_bar`, `four_panel`, `wide` (16:9), `tall` (9:16), `square` (1:1), `dark`

Or use any image: `@"path/to/image.png" { ... }`

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
