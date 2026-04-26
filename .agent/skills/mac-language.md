---
name: mac-language
description: Use when writing, debugging, or explaining Mac (Meme as Code) programs. Covers all syntax, meme literals, positioned text, effects, animations, styles, grids, functional pipelines, and operational limits.
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

## Reach For More Than Basics

Before writing a trivial `@blank "text" => "out.png"`, consider whether the request supports any of these — it usually does, and the output is dramatically more interesting:

- **Positioned text** at specific `x, y` coordinates for labeled diagrams, poster layouts, comic panels
- **Named effects** composed with `>>` (e.g. `effect vintage = sepia >> vignette >> noise(0.05)`) rather than raw inline effects
- **Data-driven frames** — `range` / `map` / `reduce(Gif())` to generate animations from an array
- **GIFs with transitions** — a 3-beat story with `crossfade`/`fadeBlack` transitions is usually more shareable than a static meme
- **Grids** to compose multiple memes into a single image

Default to animated output over static when a request mentions time, sequence, reveal, build-up, or emphasis.

---

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

### If/Else Expressions

`if`/`else` can be used as an expression anywhere. The `else` branch is required.

```mac
val x = if (10 > 5) "big" else "small";
val max = (a, b) -> if (a > b) a else b;
val grade = (s) -> if (s >= 90) "A" else if (s >= 80) "B" else "F";
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

### Duration

Animation timings use `Duration(ms)`. GIF frames accept it in several places:

```mac
Duration(500)                  // 500ms
Duration(500) + Duration(100)  // 600ms
Duration(500) * 3              // 1500ms
```

---

## Meme Creation

### @template Literal

```mac
// One-liner (text fills the primary slot per template)
@blank "Hello!" => "hello.png";

// Positional strings: 1=center (blank/bottom_text), 2=top+bottom (two_panel), 3=top+bottom+center
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

### Templates (10 built-in)

`blank`, `dark`, `two_panel`, `three_panel`, `four_panel`, `bottom_text`, `caption_bar`, `square`, `wide`, `tall`

### Meme Assets (66)

Access via `@meme.<n>`: `shrek_smirk`, `shrek_side_eye`, `girl_side_eye`, `king_bach_stare`, `jordan_crying`,
`kid_crying`, `window_despair`, `guy_crying`, `idk_about_that`, `distracted_boyfriend`, `will_smith_presenting`,
`thousand_yard_stare`, `height_chart_lineup`, `horse_head_wedding`, `bliss_ferret_stare`, `cat_explosion`,
`drake_reaction_grid`, `cow_dolphin_jump`, `kcat_stare`, `evil_cat_throne`, `shrek_sun_donkey`, `shaq_timeout`,
`cat_hill_peek`, `dog_earbuds_bliss`, `breaking_news_alert`, `shrek_xp_hill`, `fallon_staring`,
`bubbles_chopsticks`, `blossom_angry_fire`, `this_is_fine_empty_room`, `window_bars`,
`empty_stage_spotlight`, `kpop_shush`, `cardi_blue_hair`, `t_rex_car_window`, `stonks_arrow`,
`breaking_news_mic`, `bathroom_throne`, `empty_press_conference`, `bright_sun_day`, `many_bald_men`, `pepe_clouds`,
`empty_oval_office`, `windows_xp_bliss`, `spongebob_flower_hills`, `krusty_krab_red_chair`,
`barbie_car_fire`, `guy_holding_rose`, `sid_closeup`, `spongebob_group_stare`, `squidward_window_stare`,
`rock_bliss_peek`, `bubbles_unimpressed`, `bubbles_scheming`, `gary_disguise`, `spongebob_houses_sunset`,
`spongebob_war_room`, `beach_dog_sitting`, `mona_lisa_side_eye`, `three_matching_guys`, `lonely_desk_worker`,
`peanuts_crosswalk`, `angry_alarm_clock`, `krusty_krab_kitchen`, `doge_windows_hill`, `tube_dogs_hill`

### Named Text Positions

`top:`, `bottom:`, `center:` — default for one-liners depends on the template.

### Positioned Text (x, y)

Place text at arbitrary pixel coordinates. Use `text:` followed by the content, then `x:`, `y:`, and optionally `fontSize:`. Multiple positioned entries are allowed and can mix with `top:` / `bottom:` / `center:`.

```mac
@blank 800x600 {
    top: "HEADER"
    text: "label A" x: 120 y: 200 fontSize: "sm"
    text: "label B" x: 520 y: 200 fontSize: "sm"
    text: "BIG"     x: 400 y: 400 fontSize: 96
    bottom: "footer"
};
```

`fontSize` accepts a numeric pixel size **or** a tier string: `"sm"`, `"md"`, `"lg"`, `"xlg"`.

All positioned text on a single meme shares the meme's outer style (color, outline, etc.). Per-positioned-text colors are **not** supported — if you need two colors in one image, generate two memes and `beside`/`stack` them, or use `grid`.

### Meme Blocks with a Style Require Dimensions (parser quirk)

These two forms are accepted:

```mac
// Quick form (string text) with a style — dimensions optional
@blank alert "PRODUCTION IS DOWN" => "alert.png";

// Block form with a style — dimensions REQUIRED
@blank 720x720 alert { top: "PRODUCTION" bottom: "IS DOWN" } => "alert.png";
```

This form **fails to parse**:

```mac
@blank alert { top: "oops" } => "x.png";  // Error: Expected ';'
```

Always include `WIDTHxHEIGHT` when combining a style with a `{ ... }` block.

---

## Styles

### Syntax

```mac
style name {
    color: "#FFFFFF"            // hex, supports alpha (#RRGGBBAA)
    outline: 3                  // pixels; or `outlineWidth:` (alias)
    outlineColor: "#000000"
    shadow: 3                   // drop shadow offset in pixels
    shadowColor: "#00000088"    // alpha allowed
    fontSize: "md"              // "sm" | "md" | "lg" | "xlg" or a pixel number
    fontWeight: "bold"          // "normal" | "bold"
    textTransform: "uppercase"  // "none" | "uppercase" (default: uppercase)
    background: "#00000099"     // optional fill behind text; use alpha for translucency
}
```

### Example: subtitle bar

```mac
style subtitle {
    color: "#FFFFFF"
    outline: 0
    fontWeight: "normal"
    textTransform: "none"
    background: "#00000099"
    fontSize: 28
}
@blank 720x720 subtitle { bottom: "A low-key caption" } => "sub.png";
```

### Applying Styles

```mac
// Quick form
@dark neon "HACK THE PLANET" => "neon.png";

// Block form (MUST include dimensions)
@dark 800x800 neon {
    top: "HACK THE PLANET"
    bottom: "sudo rm -rf /"
} => "hero.png";
```

### Hex Colors with Alpha

`#RRGGBB` and `#RRGGBBAA` both work everywhere a color is accepted (`color`, `outlineColor`, `shadowColor`, `background`).

---

## Save Operator `=>` and `save()`

```mac
@blank "hello" => "hello.png";
gif { ... } => "animation.gif";
grid 2x2 { ... } |> pad(5) => "grid.png";

// Variable save paths
val filename = "output.png";
@blank "test" => filename;

// save() function — equivalent to =>
val m = @blank "hi";
save(m, "hi.png");
```

### Path resolution

| Input | Output |
|---|---|
| `"hello.png"` | `~/mac/output/hello.png` |
| `"sub/hello.png"` | `./sub/hello.png` (relative to cwd) |
| `"/tmp/hello.png"` | `/tmp/hello.png` (absolute) |

If `MAC_OUTPUT_DIR` is set by the host, only the filename portion is kept — all saves land in that directory. Use bare filenames unless the user explicitly wants a specific location.

### Supported formats

`.png` (default, lossless), `.jpg` / `.jpeg` (lossy), `.gif` (animated or static).

---

## Effects

### Direct effects (no parameters)

`invert`, `sepia`, `grayscale`, `sharpen`, `vignette`

### Parameterized effects

Parameter ranges below are the useful sweet spots — values outside these are accepted but produce low-quality output.

| Effect | Parameter | Range | Notes |
|---|---|---|---|
| `blur(r)` | pixels | 1–20 | box blur (not gaussian) |
| `pixelate(s)` | block size | 2–50 | |
| `noise(a)` | amount | 0.0–1.0 | **non-deterministic across frames — see warning below** |
| `saturate(f)` | factor | 0.0–5.0 | 1.0 = unchanged |
| `contrast(f)` | factor | 0.0–5.0 | 1.0 = unchanged |
| `brightness(f)` | factor | 0.0–3.0 | 1.0 = unchanged |
| `jpeg(q)` | quality | 1–100 | lower = more artifacts |
| `hueShift(deg)` | degrees | 0–360 | |
| `glow(r)` | bloom radius | 1–20 | screen-blend bloom |
| `posterize(l)` | color levels | 2–32 | |
| `chromatic(o)` | RGB offset | 1–20 | |
| `threshold(l)` | luma level | 0–255 | black/white binarize |

**`tint` is currently broken.** It's documented as accepting a hex string but the implementation requires a decimal integer AND hardcodes alpha at 0.5. Avoid it — use `hueShift`, `saturate`, or a `background` color in a style instead.

### Layout wrappers (applied via pipe)

`pad(px)`, `border(w)` — added around a rendered meme, grid, or gif frame.

### Applying effects

```mac
@blank "Vintage" |> sepia |> vignette => "vintage.png";
@blank 800x800 "Max" |> glitch |> border(2) => "max.png";
```

### Naming and composing effects

Always name effects for non-trivial work. This is the single biggest quality signal in Mac code.

```mac
effect glitch    = pixelate(4) >> contrast(1.8) >> noise(0.2);
effect vintage   = sepia >> brightness(0.9) >> vignette;
effect cyberpunk = hueShift(180) >> contrast(1.5) >> chromatic(3) >> glow(4);
effect deepfry   = saturate(3.0) >> contrast(2.0) >> jpeg(10) >> noise(0.1);
```

### Effect ordering intuition

Effect ordering matters — `contrast(2) >> saturate(2)` is not the same as `saturate(2) >> contrast(2)`. A rule of thumb:

1. **Shape / tonal first**: `brightness`, `contrast`, `blur`, `sharpen`, `pixelate`
2. **Color second**: `saturate`, `hueShift`, `sepia`, `grayscale`, `posterize`, `threshold`
3. **Atmosphere / bloom third**: `glow`, `vignette`, `chromatic`
4. **Artifacts last**: `jpeg`, `noise`

### Noise and GIF flicker

`noise()` uses a non-seeded random source. In a GIF where every frame has `|> noise(0.1)`, each frame's grain pattern is different, which reads as flicker. Prefer one of:

- Apply noise to a *single base frame*, then derive animation frames from it with other effects (`hueShift`, `chromatic`) that are deterministic.
- Keep noise amounts small (`0.02` to `0.05`) if used per-frame.
- Skip noise in short, slow GIFs where the flicker is most visible.

---

## Grids

### Literal form

```mac
grid 2x2 {
    @blank "A" |> sepia
    @blank "B" |> vintage
    @blank "C" |> glitch
    @blank "D" |> deepfry
} |> pad(5) |> border(2) => "grid.png";
```

If the entry count doesn't match slots, the grid truncates extras and fills missing cells with blank.

### Spread form (data-driven)

```mac
val memes = data |> map(d -> @two_panel { top: d[0] bottom: d[1] });

grid 2x2 { ...memes } => "spread.png";  // explicit shape
grid memes => "auto.png";                // auto-dimensions from array length
```

### Nested grids

```mac
grid 1x2 {
    grid 2x1 { @blank "A" @blank "B" }
    @blank "Footer"
} => "nested.png";
```

### Piping a grid through effects

```mac
grid 2x2 { @blank "a" @blank "b" @blank "c" @blank "d" }
    |> vignette
    |> border(3)
    => "framed.png";
```

### Programmatic composition

`beside(a, b)` — side-by-side, `stack(a, b)` — top/bottom, `toGrid(arr, cols, rows)` — array to grid. Useful when building layouts programmatically where a literal `grid { }` block won't fit.

```mac
beside(@blank "before", @blank "after") => "compare.png";
stack(@blank "setup", @blank "punchline") => "bit.png";
toGrid(memes, 3, 2) => "dashboard.png";
```

---

## Animation

### GIF blocks

```mac
gif {
    @blank 480x480 "3" : 500ms
    @blank 480x480 "2" : 500ms
    @blank 480x480 "1" : 500ms
    @blank 480x480 "GO!" : 1s
} => "countdown.gif";
```

### Transitions

Use `--- type duration [easing] ---` between frames. Transitions run at **15 fps internally**, so a 200ms transition is only 3 frames and will look stepped. **Target 500ms or more** for anything described as "smooth".

```mac
gif {
    @two_panel { top: "Act 1" bottom: "The setup" } : 3s
    --- crossfade 500ms easeInOut ---
    @two_panel { top: "Act 2" bottom: "The conflict" } |> sepia : 3s
    --- fadeBlack 600ms ---
    @dark "fin." : 2s
} => "story.gif";
```

**Transition types:** `crossfade`, `slideLeft`, `slideRight`, `slideUp`, `slideDown`, `wipe`, `fadeBlack`, `zoom`

**Easing:** `linear` (default), `easeIn`, `easeOut`, `easeInOut`. Do not use `bounce` — the parser accepts it but the interpreter silently falls through to `linear`.

### Loop-back transition

A `---` separator at the *end* of the block defines how the GIF transitions from its last frame back to the first when it loops. Use this for seamless loops.

```mac
gif {
    @blank "A" : 1s
    --- slideLeft 500ms ---
    @blank "B" : 1s
    --- crossfade 500ms ---
} => "seamless.gif";
// The trailing crossfade transitions B back to A on loop
```

### Mixed mode

Transitions are optional between any pair of frames. Frames without a `---` separator simply hard-cut.

```mac
gif {
    @blank "Intro" : 1s
    --- crossfade 500ms ---
    @blank "Main" : 2s
    @blank "Quick Cut" : 500ms
    --- fadeBlack 400ms ---
    @blank "Outro" : 1s
} => "mixed.gif";
```

### Programmatic GIFs

#### `animate(frames, duration)` — one-liner

Simplest path for a data-driven GIF. **Duration must be a `Duration` instance, not a raw number** — `animate(frames, 500)` will crash.

```mac
val frames = [@blank "One", @blank "Two", @blank "Three"];
animate(frames, Duration(500)) => "simple.gif";
```

#### `reduce(Gif())` — per-frame durations

Use this when frames need varying durations or the GIF is built up from data.

```mac
val days = [
    ["MONDAY",    "Fresh deploy"],
    ["WEDNESDAY", "Why is CI failing?"],
    ["FRIDAY",    "Ship it anyway"],
];

val week = days
    |> map(d -> @two_panel { top: d[0] bottom: d[1] })
    |> reduce((g, m) -> g.frame(m, Duration(1500)), Gif());

week.save("dev_week.gif");
```

### Grids as GIF frames

A whole grid can be a single frame of a gif:

```mac
gif {
    grid 2x1 { @blank "a" @blank "b" } : 600ms
    --- crossfade 300ms ---
    grid 2x1 { @blank "c" @blank "d" } : 600ms
} => "grid_anim.gif";
```

### Effects per frame

```mac
gif {
    @blank "Day" : 2s
    --- crossfade 500ms ---
    @blank "Night" |> vignette : 2s
} => "day_night.gif";
```

---

## Operational Limits

These are hard caps in the runtime. Plan animations around them.

- **GIF frame cap: 500 frames total.** Includes interpolated transition frames (15 fps × transition duration in seconds). A 40-character letter-by-letter reveal with 300ms crossfades between each is ≈ 40 + 40×4.5 = 220 frames — fine. Pushing to 100 characters overflows.
- **Transition frame rate: 15 fps.** A 200ms transition is 3 frames. Aim for 400–800ms for smoothness.
- **Image dimensions clamped to 4096×4096 max.**
- **Playground code size limit: 50 KB.** Images: 5 MB each, max 3 per share request, `.png` / `.jpg` / `.jpeg` / `.gif` only.

---

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

### Array operations

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

### String operations

```mac
upper(s)  lower(s)  trim(s)  split(s, delim)
replace(s, from, to)  substr(s, start, len)
```

### Stdlib types

`Result` and `Option` enums (defined in prelude):

```mac
val ok = Result.Ok(42);
val err = Result.Err("failed");
val value = match ok {
    Result.Ok(v) -> v
    Result.Err(e) -> 0
};
```

---

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

---

## Complete Example — Data-Driven Animated Story

```mac
style chapter {
    color: "#FFD700"
    outline: 4
    outlineColor: "#332b00"
    shadow: 6
    shadowColor: "#FFD70066"
}

style body {
    color: "#FFFFFF"
    outline: 2
    outlineColor: "#000000"
    textTransform: "none"
    fontSize: "md"
}

effect cinematic = contrast(1.2) >> saturate(1.1) >> vignette;
effect climax    = saturate(1.8) >> glow(8) >> chromatic(2) >> contrast(1.4);

val beats = [
    { kind: "title",  text: "THE DEPLOY" },
    { kind: "body",   text: "git push origin main" },
    { kind: "body",   text: "CI goes green" },
    { kind: "body",   text: "smoke tests pass" },
    { kind: "climax", text: "PROD IS DOWN" },
];

val frames = beats |> map(b ->
    if (b["kind"] == "title")
        @dark 800x450 chapter { center: b["text"] } |> cinematic
    else if (b["kind"] == "climax")
        @dark 800x450 chapter { center: b["text"] } |> climax
    else
        @dark 800x450 body { center: b["text"] } |> cinematic
);

val story = frames |> reduce((g, f) -> g.frame(f, Duration(1500)), Gif());
story.save("deploy.gif");

print "Saved deploy.gif";
```
