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
[1, 2, 3] |> filter(x -> x > 1) |> reverse;  // [3, 2]
"hello" |> upper;                        // HELLO
```

Pipes chain naturally for multi-step transformations:

```mac
"a,b,c" |> split(",") |> map(upper) |> join("-");  // A-B-C
```

### Compose `>>`

Creates a new function by chaining two functions together. The output of the first becomes the input of the second.

```mac
var pipeline = (x -> x * 2) >> (x -> x + 1);
print pipeline(5);  // 11
```

Compose is used heavily for effect presets:

```mac
var glitch = pixelate(6) >> contrast(1.8) >> noise(0.2);
var vintage = sepia >> vignette >> brightness(0.9);
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

Lightweight syntax for single-expression functions.

```mac
// Single parameter
var double = x -> x * 2;
print double(5);  // 10

// Multiple parameters
var add = (a, b) -> a + b;
print add(3, 4);  // 7

// Inline with higher-order functions
[1, 2, 3] |> map(x -> x * 2);           // [2, 4, 6]
[1, 2, 3] |> filter(x -> x > 1);        // [2, 3]
[1, 2, 3] |> reduce((a, b) -> a + b, 0); // 6

// Nested arrows (currying)
var mul = x -> y -> x * y;
var triple = mul(3);
print triple(4);  // 12
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

print Dog("Rex").speak();
```

## Operator Overloading

Any class can define dunder methods:

```mac
class Vec {
    init(x, y) { this.x = x; this.y = y; }
    __add__(other) { return Vec(this.x + other.x, this.y + other.y); }
    __mul__(n) { return Vec(this.x * n, this.y * n); }
    __neg__() { return Vec(-this.x, -this.y); }
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

## Built-in Functions

### Core

| Function             | Description               |
| -------------------- | ------------------------- |
| `clock()`            | Unix timestamp            |
| `type(x)`            | Type name as string       |
| `len(x)`             | Length of string or array  |
| `input(prompt)`      | Read line from stdin       |
| `print x`            | Print to stdout            |

### Math

| Function            | Description          |
| ------------------- | -------------------- |
| `sqrt(n)`           | Square root          |
| `abs(n)`            | Absolute value       |
| `pow(b, e)`         | Exponentiation       |
| `floor(n)`          | Round down           |
| `ceil(n)`           | Round up             |

### Strings

| Function                   | Description                        |
| -------------------------- | ---------------------------------- |
| `substr(s, start, len)`    | Substring                          |
| `split(s, delim)`          | Split string into array            |
| `upper(s)`                 | Uppercase                          |
| `lower(s)`                 | Lowercase                          |
| `trim(s)`                  | Strip leading/trailing whitespace  |
| `replace(s, from, to)`     | Replace all occurrences            |
| `join(arr, sep)`           | Join array elements into string    |

```mac
"hello" |> upper;                              // HELLO
"  hi  " |> trim;                             // hi
"hello world" |> replace("world", "mac");      // hello mac
"a,b,c" |> split(",") |> map(upper) |> join("-"); // A-B-C
```

### Arrays

| Function               | Description                                   |
| ---------------------- | --------------------------------------------- |
| `push(arr, val)`       | Append value (mutates)                        |
| `pop(arr)`             | Remove and return last element (mutates)      |
| `map(arr, fn)`         | Transform each element                        |
| `filter(arr, fn)`      | Keep elements where fn returns true           |
| `reduce(arr, fn, init)`| Fold left with accumulator                    |
| `find(arr, fn)`        | First element matching predicate, or nil      |
| `any(arr, fn)`         | True if any element satisfies predicate       |
| `all(arr, fn)`         | True if all elements satisfy predicate        |
| `sort(arr)`            | Sort copy (numbers/strings)                   |
| `sort(arr, fn)`        | Sort copy with comparator                     |
| `reverse(arr)`         | Reversed copy                                 |
| `flatten(arr)`         | Flatten one level of nesting                  |
| `flatMap(arr, fn)`     | Map then flatten                              |
| `zip(a, b)`            | Pair elements into `[[a0, b0], [a1, b1], ...]`|
| `enumerate(arr)`       | Returns `[[0, el0], [1, el1], ...]`           |
| `take(arr, n)`         | First n elements                              |
| `drop(arr, n)`         | Skip first n elements                         |
| `each(arr, fn)`        | Call fn for side effects, returns nil          |
| `range(end)`           | `[0, 1, ..., end-1]`                          |
| `range(start, end)`    | `[start, ..., end-1]`                         |
| `range(start, end, step)` | `[start, start+step, ...]`                 |

```mac
print range(5);                              // [0, 1, 2, 3, 4]
print range(2, 5);                           // [2, 3, 4]
print range(0, 10, 3);                       // [0, 3, 6, 9]
print [1,2,3] |> reduce((a,b) -> a + b, 0); // 6
print zip([1,2], ["a","b"]);                 // [[1, a], [2, b]]
print ["x","y"] |> enumerate;               // [[0, x], [1, y]]
print [3,1,2] |> sort;                       // [1, 2, 3]
print [1,2,3] |> reverse;                   // [3, 2, 1]
print [1,2,3,4] |> find(x -> x > 2);        // 3
print [1,2,3] |> any(x -> x > 2);           // true
print [1,2,3] |> all(x -> x > 0);           // true
print [1,2,3] |> take(2);                   // [1, 2]
print [1,2,3] |> drop(1);                   // [2, 3]
print ["a","b","c"] |> join("-");            // a-b-c
print [[1,2],[3],[4,5]] |> flatten;          // [1, 2, 3, 4, 5]
```

## Arrays and Maps

```mac
var a = [1, 2, 3];
print a[0];         // 1
a[1] = 99;
push(a, 4);

var m = {name: "Mac"};
print m.name;       // Mac
print m["name"];    // Mac
m.version = 1;

for (var x in a) print x;
for (var key in m) print key;
```

## Effects

Effects are pure functions that transform meme pixel data. They work with the pipe operator and can be composed with `>>`.

### Parameterized Effects

These return a partial effect when called with a parameter, ready to be piped or composed.

| Effect              | Description                          |
| ------------------- | ------------------------------------ |
| `blur(radius)`      | Gaussian blur                        |
| `pixelate(size)`    | Pixelation at given block size       |
| `noise(amount)`     | Random noise (0.0 - 1.0)            |
| `saturate(factor)`  | Color saturation multiplier          |
| `contrast(factor)`  | Contrast multiplier                  |
| `brightness(factor)`| Brightness multiplier                |
| `jpeg(quality)`     | JPEG compression artifact simulation |

### Direct Effects

These take no parameters and can be used directly.

| Effect      | Description          |
| ----------- | -------------------- |
| `invert`    | Invert colors        |
| `sepia`     | Sepia tone           |
| `sharpen`   | Sharpen              |
| `vignette`  | Dark vignette border |

### Presets

Compose effects into reusable presets:

```mac
var glitch = pixelate(6) >> contrast(1.8) >> noise(0.2);
var vintage = sepia >> vignette >> brightness(0.9);
var deepfry = saturate(3.0) >> contrast(2.0) >> jpeg(10) >> noise(0.1);

Meme(t).text(Top, "hello") |> glitch;
```

## Layout

Layout functions combine multiple rendered memes into composite images.

| Function            | Description                            |
| ------------------- | -------------------------------------- |
| `beside(a, b)`      | Place two memes side by side           |
| `stack(a, b)`       | Stack two memes vertically             |
| `grid(memes, c, r)` | Arrange memes in a cols x rows grid    |
| `toGrid(arr, c, r)` | Pipeline-friendly grid from array      |
| `pad(meme, px)`     | Add padding around a meme              |
| `border(meme, px)`  | Add a border around a meme             |

```mac
var a = Meme(t).text(Top, "Left");
var b = Meme(t).text(Top, "Right");
beside(a, b) |> pad(5) |> border(2);
```

## Timeline & Animation

The Timeline API builds animated GIFs with keyframes, transitions, and timing control.

```mac
var t = Template("two_panel");

Timeline()
    |> at(0, Meme(t).text(Top, "Frame 1") |> clean)
    |> transition(400, crossfade)
    |> at(800, Meme(t).text(Top, "Frame 2") |> vintage)
    |> transition(400, slideLeft)
    |> at(1600, Meme(t).text(Top, "Frame 3") |> glitch)
    |> hold(800)
    |> loop(0)
    |> render("animation.gif");
```

### Timeline Functions

| Function                    | Description                           |
| --------------------------- | ------------------------------------- |
| `Timeline()`                | Create a new timeline                 |
| `at(tl, timeMs, meme)`     | Add a keyframe at a time offset       |
| `transition(tl, ms, type)` | Add transition between keyframes      |
| `hold(tl, ms)`             | Hold the current frame                |
| `loop(tl, count)`          | Set loop count (0 = infinite)         |
| `render(tl, path)`         | Render timeline to GIF                |

### Transition Types

`crossfade` `slideLeft` `slideRight` `slideUp` `slideDown` `wipe`

### Simple GIF Builder

For straightforward frame-by-frame GIFs without transitions:

```mac
Gif()
    .frame(Meme(t).text(Top, "1"), Duration(500))
    .frame(Meme(t).text(Top, "2"), Duration(500))
    .save("countdown.gif");
```

### Animate Helper

Convert an array of memes into a GIF with uniform frame duration:

```mac
["Mon", "Tue", "Wed"]
    |> map(d -> Meme(t).text(Top, d))
    |> animate(Duration(400));
```
