# Mac Language Reference

## Types

| Type    | Example                       |
| ------- | ----------------------------- |
| Number  | `42`, `3.14`                  |
| String  | `"hello"`                     |
| Boolean | `true`, `false`               |
| Nil     | `nil`                         |
| Array   | `[1, 2, 3]`                   |
| Map     | `{name: "Mac", version: 1}`   |

## Variables

```mac
var x = 10;
var y;          // nil
x = 20;
```

## Operators

### Arithmetic & Comparison

`+` `-` `*` `/` `%` `==` `!=` `<` `>` `<=` `>=` `!` `and` `or`

### Pipe `|>`

Passes the left-hand value as the first argument to the right-hand function.

```mac
[1, 2, 3] |> map(x -> x * 2);           // [2, 4, 6]
"hello" |> upper;                        // HELLO
"a,b,c" |> split(",") |> map(upper) |> join("-");  // A-B-C
```

### Compose `>>`

Creates a new function by chaining two functions together.

```mac
var pipeline = (x -> x * 2) >> (x -> x + 1);
print pipeline(5);  // 11

effect glitch = pixelate(4) >> contrast(1.8) >> noise(0.2);
```

### Save `=>`

Exports any meme, GIF, timeline, or rendered result to a file. Auto-detects format from extension.

```mac
@blank "hello" => "hello.png"
gif { ... } => "animation.gif"
```

## Control Flow

```mac
if (condition) { ... } else { ... }
while (condition) { ... }
for (var i = 0; i < 10; i = i + 1) { ... }
for (var x in [1, 2, 3]) { ... }
break;
continue;
```

## Functions

```mac
fun greet(name) {
    return "Hello, " + name;
}

// Lambdas
var double = fun(x) { return x * 2; };

// Closures
fun counter() {
    var n = 0;
    return fun() { n = n + 1; return n; };
}
```

### Arrow Functions

```mac
var double = x -> x * 2;
var add = (a, b) -> a + b;

[1, 2, 3] |> map(x -> x * 2);
[1, 2, 3] |> reduce((a, b) -> a + b, 0);

// Currying
var mul = x -> y -> x * y;
var triple = mul(3);
```

## Classes

```mac
class Animal {
    init(name) { this.name = name; }
    speak() { return "..."; }
}

class Dog < Animal {
    speak() { return "Woof!"; }
    parent() { return super.speak(); }
}
```

## Operator Overloading

```mac
class Vec {
    init(x, y) { this.x = x; this.y = y; }
    __add__(other) { return Vec(this.x + other.x, this.y + other.y); }
    __mul__(n) { return Vec(this.x * n, this.y * n); }
    __eq__(other) { return this.x == other.x and this.y == other.y; }
}
```

| Operator | Method      | Operator | Method      |
| -------- | ----------- | -------- | ----------- |
| `+`      | `__add__`   | `==`     | `__eq__`    |
| `-`      | `__sub__`   | `!=`     | `__ne__`    |
| `*`      | `__mul__`   | `<`      | `__lt__`    |
| `/`      | `__div__`   | `>`      | `__gt__`    |
| `%`      | `__mod__`   | `-x`     | `__neg__`   |

## Meme Literals

The `@template` syntax creates memes as language-level literals.

```mac
// Block syntax with named positions
@two_panel {
    top: "Hello"
    bottom: "World"
} => "meme.png"

// One-liner (center text)
@blank "Just this" => "simple.png"

// With size
@two_panel 800x600 {
    top: "High res"
    bottom: "800 by 600"
} => "hires.png"

// Custom image as template
@"path/to/photo.png" {
    top: "Custom template"
} => "custom.png"
```

Positions: `top`, `bottom`, `center`

### Templates

10 built-in templates:

| Template       | Size      | Description                            |
| -------------- | --------- | -------------------------------------- |
| `two_panel`    | 600x600   | Top and bottom panels with divider     |
| `three_panel`  | 800x500   | Three vertical panels                  |
| `bottom_text`  | 600x400   | Image area on top, text area at bottom |
| `blank`        | 600x600   | Plain white canvas                     |
| `caption_bar`  | 600x500   | 70% image, 30% caption bar            |
| `four_panel`   | 600x600   | 2x2 grid with dividers                |
| `wide`         | 1200x675  | 16:9 landscape (YouTube)              |
| `tall`         | 675x1200  | 9:16 portrait (stories/reels)         |
| `square`       | 800x800   | 1:1 (Instagram)                       |
| `dark`         | 600x600   | Dark background                       |

Or use any image: `@"path/to/image.png"`

## Effects

Effects are pure functions that transform meme pixel data. Apply with `|>`, compose with `>>`.

### Effect Keyword

```mac
effect glitch = pixelate(4) >> contrast(1.8) >> noise(0.2);
effect vintage = sepia >> brightness(0.9);
effect cyberpunk = hueShift(180) >> contrast(1.5) >> chromatic(3) >> glow(4);
effect comic = posterize(5) >> contrast(1.4) >> sharpen;

@two_panel { top: "hello" } |> glitch => "out.png"
```

### Parameterized Effects

| Effect              | Description                          |
| ------------------- | ------------------------------------ |
| `blur(radius)`      | Box blur                             |
| `pixelate(size)`    | Pixelation at given block size       |
| `noise(amount)`     | Random noise (0.0 - 1.0)            |
| `saturate(factor)`  | Color saturation multiplier          |
| `contrast(factor)`  | Contrast multiplier                  |
| `brightness(factor)`| Brightness multiplier                |
| `jpeg(quality)`     | JPEG compression artifact simulation |
| `hueShift(degrees)` | HSL hue rotation (0-360)             |
| `glow(radius)`      | Bloom via blur + screen blend        |
| `posterize(levels)`  | Reduce to N color levels            |
| `chromatic(offset)` | RGB channel displacement             |
| `threshold(level)`  | Black/white binarize                 |
| `tint(hexColor)`    | Color overlay blend                  |

### Direct Effects

| Effect      | Description          |
| ----------- | -------------------- |
| `invert`    | Invert colors        |
| `sepia`     | Sepia tone           |
| `sharpen`   | Sharpen              |
| `vignette`  | Dark vignette border |
| `grayscale` | Convert to grayscale |

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

// Apply between template name and block
@dark neon {
    top: "NEON"
    bottom: "Styled text"
} => "neon.png"

// One-liner with style
@blank neon "Green text" => "green.png"

// Size + style
@dark 800x800 neon "Big neon" => "big.png"
```

### Style Properties

| Property       | Type   | Default       | Description                 |
| -------------- | ------ | ------------- | --------------------------- |
| `color`        | hex    | `"#FFFFFF"`   | Text fill color             |
| `outline`      | number | `3`           | Outline width in pixels     |
| `outlineColor` | hex    | `"#000000"`   | Outline color               |
| `shadow`       | number | `0`           | Shadow offset (x and y)     |
| `shadowColor`  | hex    | `"#00000080"` | Shadow color (supports alpha)|
| `fontSize`     | number | `0` (auto)    | Override auto font sizing   |

Hex colors: `#RRGGBB` or `#RRGGBBAA` (with alpha channel).

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

Duration units: `ms` (milliseconds), `s` (seconds)

### Timeline Blocks

```mac
timeline loop {
    @two_panel { top: "Scene 1" } : 3s
    --- crossfade 200ms ease ---
    @two_panel { top: "Scene 2" } : 3s
    --- fadeBlack 300ms ---
    @two_panel { top: "Scene 3" } : 3s
} => "story.gif"
```

### Transitions

`crossfade` `slideLeft` `slideRight` `slideUp` `slideDown` `wipe` `fadeBlack` `zoom`

### Easing

Add an easing keyword after the duration in `--- transition duration [easing] ---`:

`ease` `easeIn` `easeOut` `easeInOut`

Default is linear.

## Layout

```mac
// Side by side
beside(meme1, meme2) => "row.png"

// Vertical stack
stack(meme1, meme2) => "col.png"

// Grid
grid 2x2 {
    @blank "A" |> sepia
    @blank "B" |> vintage
    @blank "C" |> glitch
    @blank "D" |> deepfry
} |> pad(5) |> border(2) => "grid.png"
```

## Built-in Functions

### Core

| Function         | Description               |
| ---------------- | ------------------------- |
| `clock()`        | Unix timestamp            |
| `type(x)`        | Type name as string       |
| `len(x)`         | Length of string or array  |
| `input(prompt)`  | Read line from stdin       |
| `print x`        | Print to stdout            |
| `save(x, path)`  | Save any exportable type   |

### Math

`sqrt(n)` `abs(n)` `pow(b, e)` `floor(n)` `ceil(n)`

### Strings

`substr(s, start, len)` `split(s, delim)` `upper(s)` `lower(s)` `trim(s)` `replace(s, from, to)` `join(arr, sep)`

### Arrays

| Function               | Description                                   |
| ---------------------- | --------------------------------------------- |
| `push(arr, val)`       | Append value (mutates)                        |
| `pop(arr)`             | Remove and return last element                |
| `map(arr, fn)`         | Transform each element                        |
| `filter(arr, fn)`      | Keep elements where fn returns true           |
| `reduce(arr, fn, init)`| Fold left with accumulator                    |
| `find(arr, fn)`        | First match or nil                            |
| `any(arr, fn)`         | True if any element satisfies predicate       |
| `all(arr, fn)`         | True if all elements satisfy predicate        |
| `sort(arr)`            | Sort copy                                     |
| `reverse(arr)`         | Reversed copy                                 |
| `flatten(arr)`         | Flatten one level                             |
| `flatMap(arr, fn)`     | Map then flatten                              |
| `zip(a, b)`            | Pair elements into tuples                     |
| `enumerate(arr)`       | Returns `[(index, element)]`                  |
| `take(arr, n)`         | First n elements                              |
| `drop(arr, n)`         | Skip first n elements                         |
| `each(arr, fn)`        | Call fn for side effects                      |
| `range(end)`           | `[0, 1, ..., end-1]`                          |
| `range(start, end)`    | `[start, ..., end-1]`                         |
| `range(start, end, step)` | `[start, start+step, ...]`                 |

## Arrays and Maps

```mac
var a = [1, 2, 3];
print a[0];
push(a, 4);

var m = {name: "Mac"};
print m.name;
m.version = 1;

for (var x in a) print x;
for (var key in m) print key;
```
