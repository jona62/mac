---
name: mac-language
description: Use when writing, debugging, or explaining Mac (Meme as Code) programs. Covers all syntax from basic to advanced including meme literals, effects, animations, styles, and functional pipelines.
---

# Mac Language Skill

Mac is a dynamically typed programming language where memes are first-class citizens. It compiles from C++ and has its own VS Code extension with LSP.

## When to Use

- User asks to write a Mac program or `.mac` file
- User asks about meme generation, GIF creation, or image effects
- User asks about Mac syntax (`@template`, `=>`, `gif`, `timeline`, `effect`, `style`, `|>`, `>>`)
- User is debugging a Mac script
- User wants to generate memes or animated GIFs programmatically

## Running Mac

```bash
mac script.mac         # Run a .mac file
mac                    # Interactive REPL (|> prompt)
mac --analyze file.mac # JSON analysis for LSP
```

All output files are written to `~/mac/output/` regardless of where the command is run.

## Language Basics

### Variables, Types, Control Flow

```mac
var x = 42;
var name = "Mac";
var items = [1, 2, 3];
var config = { key: "value" };

if (x > 10) { print "big"; } else { print "small"; }
while (x > 0) { x = x - 1; }
for (var i = 0; i < 5; i = i + 1) { print i; }
for (var item in items) { print item; }
```

### Functions and Arrow Functions

```mac
fun greet(name) { return "Hello, " + name; }

var double = x -> x * 2;
var add = (a, b) -> a + b;
var mul = x -> y -> x * y;  // currying
```

### Classes and Operator Overloading

```mac
class Vec {
    init(x, y) { this.x = x; this.y = y; }
    __add__(other) { return Vec(this.x + other.x, this.y + other.y); }
}
```

## Meme Creation (v2 Syntax)

### @template Literal — The Core Syntax

```mac
// Block syntax with named positions
@two_panel {
    top: "Top caption"
    bottom: "Bottom caption"
} => "meme.png"

// One-liner (center text)
@blank "Just this" => "simple.png"

// With dimensions
@square 800x800 {
    top: "High res"
    bottom: "Square"
} => "square.png"

// Custom image as template
@"path/to/photo.png" {
    top: "Custom"
} => "custom.png"
```

### Available Templates (10)

| Template       | Size      | Use Case                    |
|---------------|-----------|------------------------------|
| `two_panel`   | 600×600   | Before/after, comparison     |
| `three_panel` | 800×500   | Three-beat jokes             |
| `bottom_text` | 600×400   | Image + caption              |
| `blank`       | 600×600   | Text-only, countdowns        |
| `dark`        | 600×600   | Neon text, dark mode         |
| `wide`        | 1200×675  | YouTube thumbnails (16:9)    |
| `tall`        | 675×1200  | Stories/reels (9:16)         |
| `square`      | 800×800   | Instagram (1:1)              |
| `four_panel`  | 600×600   | 2×2 grids                    |
| `caption_bar` | 600×500   | 70% image, 30% caption       |

### Text Positions

- `top:` — top half of the image
- `bottom:` — bottom half
- `center:` — center (used in one-liners by default)

## Save Operator `=>`

Exports any meme, GIF, timeline, or layout result. Auto-detects format from extension.

```mac
@blank "hello" => "hello.png"
gif { ... } => "animation.gif"
grid 2x2 { ... } |> pad(5) => "grid.png"
```

## Effects

### Effect Keyword

Define reusable effect presets with `>>` composition:

```mac
effect glitch = pixelate(4) >> contrast(1.8) >> noise(0.2);
effect vintage = sepia >> brightness(0.9);
effect cyberpunk = hueShift(180) >> contrast(1.5) >> chromatic(3) >> glow(4);
effect comic = posterize(5) >> contrast(1.4) >> sharpen;
effect retro = grayscale >> contrast(1.3) >> noise(0.1);
```

### Apply with Pipe `|>`

```mac
@two_panel { top: "Normal" bottom: "Glitched" } |> glitch => "out.png"

// Chain multiple
@blank "Max" |> saturate(3.0) |> pixelate(3) |> noise(0.3) => "max.png"
```

### All Effects (18)

**Parameterized** (take a number, return Meme → Meme):
`blur(radius)`, `pixelate(size)`, `noise(amount)`, `saturate(factor)`, `contrast(factor)`, `brightness(factor)`, `jpeg(quality)`, `hueShift(degrees)`, `glow(radius)`, `posterize(levels)`, `chromatic(offset)`, `threshold(level)`, `tint(hexColor)`

**Direct** (Meme → Meme):
`invert`, `sepia`, `sharpen`, `vignette`, `grayscale`

**Built-in preset:**
`deepfry` = `saturate(3.0) >> contrast(2.0) >> jpeg(10) >> noise(0.1)`

## Styles

Customize text color, outline, and shadow with reusable `style` blocks:

```mac
style neon {
    color: "#00FF41"
    outline: 4
    outlineColor: "#003300"
    shadow: 3
    shadowColor: "#00FF4180"
}

style alert {
    color: "#FF0000"
    outline: 5
    outlineColor: "#440000"
}

// Apply between template name and block
@dark neon {
    top: "NEON"
    bottom: "Styled text"
} => "neon.png"

// One-liner with style
@blank alert "WARNING" => "alert.png"

// Size + style
@dark 800x800 neon "Big neon" => "big.png"
```

### Style Properties

| Property       | Type    | Default     | Description                      |
|---------------|---------|-------------|----------------------------------|
| `color`       | hex     | `"#FFFFFF"` | Text fill color                  |
| `outline`     | number  | `3`         | Outline width in pixels          |
| `outlineColor`| hex     | `"#000000"` | Outline color                    |
| `shadow`      | number  | `0`         | Shadow offset (x and y)          |
| `shadowColor` | hex     | `"#00000080"`| Shadow color (supports alpha)   |
| `fontSize`    | number  | `0` (auto)  | Override auto font sizing        |

Hex colors: `#RRGGBB` or `#RRGGBBAA` (with alpha).

## Animation

### GIF Blocks

```mac
gif loop {
    @blank "3" : 500ms
    @blank "2" : 500ms
    @blank "1" : 500ms
    @blank "GO!" : 1s
} => "countdown.gif"
```

- `loop` keyword is optional (omit for single-play)
- Duration: `400ms` or `2s`
- Each line: `memeExpr : duration`
- Meme expressions can include `|>` effects

### Timeline Blocks

Timelines add transitions between frames:

```mac
timeline loop {
    @two_panel { top: "Scene 1" } : 3s
    --- crossfade 300ms ease ---
    @two_panel { top: "Scene 2" } : 3s
    --- fadeBlack 400ms ---
    @two_panel { top: "Scene 3" } : 3s
} => "cinematic.gif"
```

### Transitions (8)

`crossfade`, `slideLeft`, `slideRight`, `slideUp`, `slideDown`, `wipe`, `fadeBlack`, `zoom`

### Easing

Add after duration in `--- transition duration [easing] ---`:

`ease`, `easeIn`, `easeOut`, `easeInOut`

Default is linear.

## Layout

```mac
// Side by side
beside(meme1, meme2) |> save("row.png")

// Vertical stack
stack(meme1, meme2) |> save("col.png")

// Grid block
grid 2x2 {
    @blank "A" |> sepia
    @blank "B" |> vintage
    @blank "C" |> glitch
    @blank "D" |> deepfry
} |> pad(5) |> border(2) => "grid.png"
```

## Functional Programming

### Pipe `|>` and Compose `>>`

```mac
// Pipe: passes left as first arg to right
[1, 2, 3] |> map(x -> x * 2) |> filter(x -> x > 3) |> reverse

// Compose: chain functions
var pipeline = (x -> x * 2) >> (x -> x + 1);
print pipeline(5);  // 11
```

### Higher-Order Functions

```mac
// Arrays
map(arr, fn)          filter(arr, fn)       reduce(arr, fn, init)
find(arr, fn)         any(arr, fn)          all(arr, fn)
sort(arr)             reverse(arr)          flatten(arr)
flatMap(arr, fn)      zip(a, b)             enumerate(arr)
take(arr, n)          drop(arr, n)          each(arr, fn)
join(arr, sep)        range(end)            range(start, end, step)

// Strings
upper(s)  lower(s)  trim(s)  split(s, d)  replace(s, from, to)  substr(s, i, n)
```

### Data-Driven Meme Generation

```mac
var quotes = [
    ["Debugging", "print('here')"],
    ["Testing", "works on my machine"],
    ["Deploying", "YOLO"]
];

// Map data to memes
var memes = quotes |> map(q -> @two_panel {
    top: q[0]
    bottom: q[1]
});

// Fold into a timeline
var tl = memes |> reduce((tl, m) -> tl
    .frame(m, Duration(3000))
    .transition(crossfade, Duration(200)), Timeline());
tl.loop(0).render("lifecycle.gif");
```

## Classic Builder API

The original class-based API still works alongside v2 syntax:

```mac
// Builder pattern
var t = Template("two_panel");
var m = Meme(t).text(Top, "Hello").text(Bottom, "World");
m.save(PNG, "meme.png");
m.resize(Size(400, 300)).save(PNG, "small.png");

// GIF builder
Gif()
    .frame(Meme(t).text(Top, "1"), Duration(500))
    .frame(Meme(t).text(Top, "2"), Duration(500))
    .save("countdown.gif");

// Timeline builder
Timeline()
    .frame(m, Duration(2000))
    .transition(crossfade, Duration(150))
    .frame(m2, Duration(2000))
    .loop(0)
    .render("timeline.gif");

// Operator overloading
var f = Meme(t).text(Top, "A") + Duration(300);  // Frame
var g = Gif() + f;                                // Gif + Frame
```

## Complete Example

```mac
// Styles
style neon {
    color: "#00FF41"
    outline: 4
    outlineColor: "#003300"
    shadow: 3
}

// Effects
effect glitch = pixelate(4) >> contrast(1.8) >> noise(0.2);
effect vintage = sepia >> brightness(0.9);

// Hero meme
@dark 800x800 neon {
    top: "HACK THE PLANET"
    bottom: "sudo rm -rf /"
} |> glitch => "hero.png"

// Animated countdown
gif loop {
    @dark 480x480 neon "3" : 500ms
    @dark 480x480 neon "2" : 500ms
    @dark 480x480 neon "1" : 500ms
    @dark 480x480 neon "GO!" : 1s
} => "countdown.gif"

// Cinematic timeline
timeline loop {
    @two_panel { top: "Act 1" bottom: "The setup" } : 3s
    --- crossfade 300ms ease ---
    @two_panel { top: "Act 2" bottom: "The conflict" } |> vintage : 3s
    --- fadeBlack 400ms ---
    @two_panel { top: "Act 3" bottom: "The resolution" } |> glitch : 3s
} => "story.gif"

// Effects comparison grid
grid 2x2 {
    @blank "Clean" |> sharpen
    @blank "Vintage" |> vintage
    @blank "Glitch" |> glitch
    @blank "Deepfry" |> deepfry
} |> pad(5) |> border(2) => "grid.png"

// Data pipeline
["Alpha", "Beta", "Gamma"]
    |> map(s -> @blank s)
    |> reduce((g, m) -> g.frame(m, Duration(800)), Gif())
    |> save("sequence.gif")
```

## Key Files

- `stdlib/prelude.mac` — Standard library (classes, constants, presets)
- `include/MemeRenderer.h` — Text rendering with stb_truetype
- `include/MemeEffects.h` — 18 pixel-level effects
- `include/MacTimeline.h` — Timeline with 8 transitions + easing
- `include/GifEncoder.h` — GIF89a encoder with LZW compression
- `include/NativeRegistry.h` — All native function definitions
- `examples/` — Progressive examples from basic to advanced

## Common Patterns

### Quick meme
```mac
@blank "Hello!" => "hello.png"
```

### Styled meme on dark background
```mac
style s { color: "#FF0" outline: 3 }
@dark s "Warning!" => "warn.png"
```

### Apply effect to meme
```mac
@two_panel { top: "A" bottom: "B" } |> sepia => "sepia.png"
```

### GIF from array
```mac
["A", "B", "C"] |> map(s -> @blank s)
    |> reduce((g, m) -> g.frame(m, Duration(500)), Gif())
    |> save("abc.gif")
```

### Grid comparison
```mac
grid 2x2 { @blank "1" @blank "2" @blank "3" @blank "4" } => "grid.png"
```
