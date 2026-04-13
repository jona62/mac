# Mac

A dynamically typed scripting language with memes as first-class citizens.

## Install

```bash
curl -fsSL https://raw.githubusercontent.com/jona62/mac/main/install.sh | bash
```

Or build from source:

```bash
cmake -S . -B build && cmake --build build
cd build && ./mac
```

## Usage

```bash
mac script.mac    # run a file
mac               # start the REPL
```

## Language

```mac
// Variables and types
var name = "Mac";
var nums = [1, 2, 3];
var config = {host: "localhost", port: 8080};

// Functions and closures
fun greet(who) { return "Hello, " + who + "!"; }
print greet(name);

// Lambdas
var double = fun(x) { return x * 2; };
print map([1, 2, 3], double);  // [2, 4, 6]

// Classes with inheritance
class Animal {
    init(name) { this.name = name; }
    speak() { return "..."; }
}
class Dog < Animal {
    speak() { return "Woof!"; }
}
print Dog("Rex").speak();

// Operator overloading
class Vec {
    init(x, y) { this.x = x; this.y = y; }
    __add__(other) { return Vec(this.x + other.x, this.y + other.y); }
}
var v = Vec(1, 2) + Vec(3, 4);

// Control flow
for (var i = 0; i < 5; i = i + 1) print i;
for (var x in [10, 20, 30]) print x;
while (true) { break; }

// Memes
var m = Meme(Template("two_panel"))
    .text(Top, "Writing Java")
    .text(Bottom, "Writing Mac");
m.save(PNG, "meme.png");

// Animated GIFs
Gif()
    .frame(m, Duration(300))
    .frame(m.text(Top, "Frame 2"), Duration(300))
    .save("animated.gif");
```

## Standard Library

### Built-in Functions

| Function | Description |
|----------|-------------|
| `print x;` | Print a value (statement) |
| `clock()` | Unix timestamp in seconds |
| `type(x)` | Type name as string |
| `len(x)` | Length of string or array |
| `input(prompt)` | Read line from stdin |

### Strings

`substr(str, start, len)` `split(str, delim)`

### Math

`sqrt(n)` `abs(n)` `pow(base, exp)` `floor(n)` `ceil(n)`

### Arrays

`push(arr, val)` `pop(arr)` `map(arr, fn)` `filter(arr, fn)`

### Types (from stdlib prelude)

| Type | Example | Operators |
|------|---------|-----------|
| `Size` | `Size(400, 300)` | `+`, `*`, `==` |
| `Duration` | `Duration(300)` | `+`, `*`, `==` |
| `Position` | `Top`, `Bottom`, `Center` | `==` |
| `Format` | `PNG`, `JPG`, `GIF` | `==` |
| `Template` | `Template("two_panel")` | — |
| `Meme` | `Meme(tmpl).text(Top, "hi")` | `+ Duration` = Frame |
| `Frame` | `meme + Duration(300)` | — |
| `Gif` | `Gif().frame(m, dur)` | `+ Frame` |

### Built-in Templates

`two_panel` `three_panel` `bottom_text` `blank`

## VS Code Extension

The `mac-lang/` directory contains a VS Code extension with:

- Syntax highlighting
- Autocomplete, hover, and go-to-definition (LSP)
- Code snippets (`fun`, `class`, `for`, `if`, `var`, etc.)
- File icon for `.mac` files

Install: symlink `mac-lang/` into `~/.vscode/extensions/` and reload VS Code.

## Development

```bash
cmake -S . -B build
cmake --build build
bash tests/run_tests.sh          # 37 tests
```

CI runs on every push — builds and tests on macOS and Linux. Tagged releases (`v*`) produce downloadable binaries for macOS ARM64, macOS x86_64, and Linux x86_64.

## License

MIT
