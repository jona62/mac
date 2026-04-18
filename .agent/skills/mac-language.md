---
name: mac-language
description: Use when writing, debugging, or explaining Mac (Meme as Code) programs. Covers all syntax including meme literals, effects, animations, styles, enums, pattern matching, functional pipelines, and the playground API.
---

# Mac Language Skill

Mac is a dynamically typed programming language where memes are first-class citizens. Current version: v0.8.1.

## When to Use

- User asks to write a Mac program or `.mac` file
- User asks about meme generation, GIF creation, or image effects
- User asks about Mac syntax (`@template`, `=>`, `gif`, `effect`, `style`, `|>`, `>>`, `enum`, `match`, `val`)
- User is debugging a Mac script
- User wants to generate or share memes programmatically

## Running Mac

```bash
mac script.mac         # Run a .mac file
mac                    # Interactive REPL
mac --analyze file.mac # JSON analysis for LSP
mac --version          # Print version
```

Output files are written to `~/mac/output/` by default (override with `MAC_OUTPUT_DIR` env var).

## Sharing Code via Playground

Generate a shareable playground link with optional image uploads:

```bash
# Code only
curl -s -X POST https://playground.macstudio.meme/api/share \
  -H 'Content-Type: application/json' \
  -d '{"code": "print \"Hello!\";"}'

# Code + image (field name = reference in code)
curl -s -X POST https://playground.macstudio.meme/api/share \
  -F 'code=@"bg" "caption" => "out.png";' \
  -F 'bg=@photo.jpg'
# Server rewrites @"bg" to @"user.<generated_id>" in the URL
```

## Language Basics

### Variables

```mac
var x = 42;           // mutable
val name = "Mac";     // immutable (cannot reassign)
val items = [1, 2, 3];
val config = { key: "value" };
```

### Control Flow

```mac
if (x > 10) { print "big"; } else { print "small"; }
while (x > 0) { x = x - 1; }
for (var i = 0; i < 5; i = i + 1) { print i; }
for (var item in items) { print item; }
```

### Functions and Implicit Returns

```mac
fun greet(name) { return "Hello, " + name; }

// Implicit return (last expression without semicolon)
fun add(a, b) { a + b }

// Arrow functions
val double = x -> x * 2;
val add = (a, b) -> a + b;
```

### String Interpolation

```mac
val name = "Mac";
print "Hello, {name}!";
print "2 + 2 = {2 + 2}";
```

### Escape Sequences

`\n` (newline), `\t` (tab), `\\` (backslash), `\"` (quote), `\r` (return), `\{` (literal brace)

### Trailing Commas

Allowed in arrays, maps, and function calls:

```mac
val items = [1, 2, 3,];
val config = { a: 1, b: 2, };
print add(3, 4,);
```

### Destructuring

```mac
val [a, b, c] = [1, 2, 3];
for (var [key, value] in zip(keys, values)) { print "{key}: {value}"; }
```

### Enums and Pattern Matching

```mac
enum Shape {
    Circle(radius)
    Rect(w, h)
}

val area = (s) -> match s {
    Shape.Circle(r) -> 3.14159 * r * r
    Shape.Rect(w, h) -> w * h
};

print area(Shape.Circle(5));
```

### Expression Blocks

```mac
val result = {
    val x = compute();
    val y = transform(x);
    x + y  // last expression is the block's value
};
```

### Classes

```mac
class Vec {
    init(x, y) { this.x = x; this.y = y; }
    __add__(other) { return Vec(this.x + other.x, this.y + other.y); }
}
```

## Meme Creation

### @template Literal

```mac
// One-liner (center text)
@blank "Hello!" => "hello.png";

// Positional strings: 1=center, 2=top+bottom, 3=top+bottom+center
@two_panel "Top caption" "Bottom caption" => "meme.png";

// Block syntax with named positions
@two_panel {
    top: "Top caption"
    bottom: "Bottom caption"
} => "meme.png";

// With dimensions
@square 800x800 { top: "High res" } => "square.png";

// Custom image as template (resolved relative to script dir)
@"photos/cat.jpg" { top: "Custom" } => "custom.png";

// Meme assets (from assets/templates/meme/)
@meme.shrek_smirk "When your code compiles" => "shrek.png";
```

### Available Templates (10)

`blank`, `dark`, `two_panel`, `three_panel`, `four_panel`, `bottom_text`, `caption_bar`, `square`, `wide`, `tall`

### Meme Assets (9)

Access via `@meme.<name>`: `shrek_smirk`, `shrek_side_eye`, `girl_side_eye`, `king_bach_stare`, `jordan_crying`, `kid_crying`, `window_despair`, `guy_crying`, `idk_about_that`

### Text Positions

`top:`, `bottom:`, `center:` (default for one-liners)

## Save Operator `=>`

```mac
@blank "hello" => "hello.png";
gif { ... } => "animation.gif";
grid 2x2 { ... } |> pad(5) => "grid.png";

// Variable save paths work too
val filename = "output.png";
@blank "test" => filename;
```

## Effects

### Defining Effects

```mac
effect glitch = pixelate(4) >> contrast(1.8) >> noise(0.2);
effect vintage = sepia >> brightness(0.9) >> vignette;
effect cyberpunk = hueShift(180) >> contrast(1.5) >> chromatic(3) >> glow(4);
```

### Applying Effects

```mac
@two_panel { top: "A" bottom: "B" } |> sepia => "sepia.png";
@blank "Max" |> glitch |> border(2) => "max.png";
```

### All Effects

**Parameterized:** `blur(r)`, `pixelate(s)`, `noise(a)`, `saturate(f)`, `contrast(f)`, `brightness(f)`, `jpeg(q)`, `hueShift(deg)`, `glow(r)`, `posterize(l)`, `chromatic(o)`, `threshold(l)`, `tint(hex)`

**Direct:** `invert`, `sepia`, `sharpen`, `vignette`, `grayscale`

**Layout:** `pad(px)`, `border(w)`

**Built-in preset:** `deepfry` = `saturate(3.0) >> contrast(2.0) >> jpeg(10) >> noise(0.1)`

## Styles

```mac
style neon {
    color: "#00FF41"
    outline: 4
    outlineColor: "#003300"
    shadow: 3
    shadowColor: "#00FF4180"
}

// Apply between template and text
@dark neon { top: "NEON" bottom: "Styled" } => "neon.png";
@blank alert "WARNING" => "alert.png";
@dark 800x800 neon "Big neon" => "big.png";
```

## Animation

### GIF Blocks

```mac
gif loop {
    @blank 480x480 "3" : 500ms
    @blank 480x480 "2" : 500ms
    @blank 480x480 "1" : 500ms
    @blank 480x480 "GO!" : 1s
} => "countdown.gif";
```

### GIF Transitions

Use `--- type duration [easing] ---` between frames:

```mac
gif loop {
    @two_panel { top: "Act 1" bottom: "The setup" } : 3s
    --- crossfade 300ms easeInOut ---
    @two_panel { top: "Act 2" bottom: "The conflict" } |> sepia : 3s
    --- fadeBlack 400ms ---
    @dark "fin." : 2s
} => "story.gif";
```

**Transition types:** `crossfade`, `slideLeft`, `slideRight`, `slideUp`, `slideDown`, `wipe`, `fadeBlack`, `zoom`

**Easing:** `linear` (default), `easeIn`, `easeOut`, `easeInOut`, `bounce`

### GIF from Data

```mac
val memes = data |> map(d -> @two_panel { top: d[0] bottom: d[1] });
val gif = memes |> reduce((g, m) -> g.frame(m, Duration(1500)), Gif());
gif.save("output.gif");
```

## Grid Layout

```mac
grid 2x2 {
    @blank "A" |> sepia
    @blank "B" |> vintage
    @blank "C" |> glitch
    @blank "D" |> deepfry
} |> pad(5) |> border(2) => "grid.png";
```

Warns if items don't match slots (truncates extra, fills missing with blank).

## Functional Programming

### Pipe `|>` and Compose `>>`

```mac
[1, 2, 3] |> map(x -> x * 2) |> filter(x -> x > 3) |> reverse;

val pipeline = (x -> x * 2) >> (x -> x + 1);
print pipeline(5);  // 11
```

### Partial Application

```mac
val multiply = (a, b) -> a * b;
val double = partial(multiply, 2);
print [1, 2, 3] |> map(double);  // [2, 4, 6]
```

### Array Operations

```mac
map(arr, fn)      filter(arr, fn)    reduce(arr, fn, init)
find(arr, fn)     any(arr, fn)       all(arr, fn)
sort(arr)         reverse(arr)       flatten(arr)
flatMap(arr, fn)  zip(a, b)          enumerate(arr)
take(arr, n)      drop(arr, n)       each(arr, fn)
join(arr, sep)    range(end)         range(start, end, step)
takeWhile(arr, fn)  dropWhile(arr, fn)  partition(arr, fn)
groupBy(arr, fn)    unique(arr)         chunk(arr, n)
scan(arr, fn, init)
```

### String Operations

```mac
upper(s)  lower(s)  trim(s)  split(s, delim)
replace(s, from, to)  substr(s, start, len)
```

## Stdlib Types

`Result` and `Option` enums (defined in prelude):

```mac
val ok = Result.Ok(42);
val err = Result.Err("failed");
val value = match ok {
    Result.Ok(v) -> v
    Result.Err(e) -> 0
};
```

## Complete Example

```mac
style neon {
    color: "#00FF41"
    outline: 4
    outlineColor: "#003300"
}

style bold {
    color: "#FFFFFF"
    outline: 3
    outlineColor: "#000000"
}

effect glitch = pixelate(4) >> contrast(1.8) >> noise(0.2);

// Hero meme with style and effect
@dark 800x800 neon {
    top: "HACK THE PLANET"
    bottom: "sudo rm -rf /"
} |> glitch => "hero.png";

// Meme assets
@meme.shrek_smirk bold {
    top: "When your code compiles"
    bottom: "On the first try"
} => "shrek.png";

// Animated countdown
gif loop {
    @dark 480x480 neon "3" : 500ms
    @dark 480x480 neon "2" : 500ms
    @dark 480x480 neon "1" : 500ms
    @dark 480x480 neon "GO!" : 1s
} => "countdown.gif";

// GIF with transitions
gif loop {
    @two_panel bold { top: "Act 1" bottom: "git init" } : 3s
    --- crossfade 300ms easeInOut ---
    @two_panel bold { top: "Act 2" bottom: "merge conflict" } : 3s
    --- fadeBlack 400ms ---
    @dark neon "fin." : 2s
} => "story.gif";

// Grid with meme assets
grid 2x2 {
    @meme.shrek_smirk bold "Works on my machine"
    @meme.girl_side_eye bold "QA finds a bug"
    @meme.king_bach_stare bold "Prod is down"
    @meme.jordan_crying bold "Rollback failed"
} |> pad(8) |> border(3) => "four_stages.png";

// Data-driven GIF
val days = [
    ["MONDAY", "Fresh deploy"],
    ["WEDNESDAY", "Why is CI failing?"],
    ["FRIDAY", "Ship it anyway"],
];
val week = days
    |> map(d -> @two_panel bold { top: d[0] bottom: d[1] })
    |> reduce((g, m) -> g.frame(m, Duration(1500)), Gif());
week.save("dev_week.gif");

print "Done! Check ~/mac/output/";
```
