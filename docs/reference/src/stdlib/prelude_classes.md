# Prelude Classes

Classes defined in `stdlib/prelude.mac`, automatically loaded at startup.

## Size

Pixel dimensions with arithmetic operators.

```
var s = Size(720, 480);
print s.width;                // 720
print s.height;               // 480

var doubled = s * 2;          // Size(1440, 960)
var combined = s + Size(100, 0); // Size(820, 480)
print s == Size(720, 480);    // true
```

| Method | Returns | Description |
|--------|---------|-------------|
| `init(w, h)` | `Size` | Constructor |
| `__add__(other)` | `Size` | Add two sizes |
| `__mul__(n)` | `Size` | Multiply by scalar |
| `__eq__(other)` | `bool` | Equality check |

## Duration

Time duration in milliseconds with arithmetic.

```
var d = Duration(500);
print d.ms;                   // 500

var longer = d * 3;           // Duration(1500)
var total = d + Duration(200); // Duration(700)
```

| Method | Returns | Description |
|--------|---------|-------------|
| `init(ms)` | `Duration` | Constructor |
| `__add__(other)` | `Duration` | Add durations |
| `__mul__(n)` | `Duration` | Multiply by scalar |
| `__eq__(other)` | `bool` | Equality check |

## Meme

Builder-pattern meme construction (alternative to meme literals).

```
var m = Meme(Template("blank"));
m.text(Position("top"), "Hello");
m.text(Position("bottom"), "World");
m.save("png", "hello.png");
```

| Method | Returns | Description |
|--------|---------|-------------|
| `init(template)` | `Meme` | Constructor with Template |
| `text(position, str)` | `Meme` | Add text at position (chainable) |
| `resize(size)` | `Meme` | Create resized copy |
| `save(format, path)` | `nil` | Save to file |
| `__add__(duration)` | `Frame` | Create animation Frame |

## Gif

Animated GIF builder (alternative to `gif { }` blocks).

```
var g = Gif();
g.frame(@blank "One", Duration(500));
g.frame(@blank "Two", Duration(500));
g.save("output.gif");
```

| Method | Returns | Description |
|--------|---------|-------------|
| `init()` | `Gif` | Constructor |
| `frame(meme, duration)` | `Gif` | Add frame (chainable) |
| `save(path)` | `nil` | Save as animated GIF |
| `__add__(frame)` | `Gif` | Add Frame object |

## Timeline

Animation with transitions (alternative to `timeline { }` blocks).

```
var t = Timeline();
t.frame(@blank "Start", Duration(2000));
t.transition("crossfade", Duration(500));
t.frame(@blank "End", Duration(2000));
t.save("timeline.gif");
```

| Method | Returns | Description |
|--------|---------|-------------|
| `init()` | `Timeline` | Constructor |
| `frame(meme, duration)` | `Timeline` | Add keyframe (chainable) |
| `transition(type, duration)` | `Timeline` | Add transition (chainable) |
| `loop(count)` | `Timeline` | Set loop count (0 = infinite) |
| `render(path)` | `nil` | Render to GIF |
| `save(path)` | `nil` | Alias for render |
| `__add__(frame)` | `Timeline` | Add Frame |

## Helper Classes

### Position

Text position constants.

```
var top = Position("top");
var bottom = Position("bottom");
var center = Position("center");
```

### Format

Output format constants.

```
var png = Format("png");
var jpg = Format("jpg");
var gif = Format("gif");
```

### Template

Meme template reference.

```
var t = Template("two_panel");
var custom = Template("path/to/image.png");
```

### Frame

A meme + duration pair for animation.

```
var f = Frame(@blank "Hello", Duration(500));
```

## See Also

- [Meme Literals](../meme/meme_literal.md) -- `@template` shorthand
- [GIF Animation](../meme/gif.md) -- `gif { }` block syntax
- [Timeline](../meme/timeline.md) -- `timeline { }` block syntax
