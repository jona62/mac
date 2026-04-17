# Types

Mac is dynamically typed. Every value belongs to one of the following types at runtime.

## Scalar Types

| Type | Example | Description |
|------|---------|-------------|
| `number` | `42`, `3.14` | IEEE 754 double-precision floating point |
| `string` | `"hello"` | Double-quoted UTF-8 string |
| `bool` | `true`, `false` | Boolean |
| `nil` | `nil` | Absence of a value |

### Numbers

All numbers are 64-bit floating point. There is no separate integer type.

```
var x = 42;
var pi = 3.14159;
var neg = -7;
```

### Strings

Strings are delimited by double quotes.

```
var greeting = "Hello, World!";
```

#### String Interpolation

Any double-quoted string can embed expressions inside `{` and `}`. The expression is evaluated at runtime and its result is converted to a string and concatenated in place.

```
var name = "Mac";
print "Hello, {name}!";           // Hello, Mac!
print "2 + 2 = {2 + 2}";         // 2 + 2 = 4
print "{upper(name)} LANG";       // MAC LANG
```

To include a literal `{` in a string, escape it with a backslash:

```
print "literal \{brace}";         // literal {brace}
```

The `+` operator also auto-converts non-string values to strings when the other operand is a string:

```
print "count: " + 42;             // count: 42
```

### String Templates

The `@"path"` syntax resolves a file path relative to the script's directory.

```
var img = @"assets/photo.png";
```

## Collection Types

| Type | Example | Description |
|------|---------|-------------|
| `[T]` | `[1, 2, 3]` | Ordered, mutable array |
| `{T}` | `{"a": 1}` | Key-value map (string keys) |

See [Arrays](./arrays.md) and [Maps](./maps.md) for details.

## Meme Types

| Type | Produced by | Description |
|------|------------|-------------|
| `Meme` | `@template "text"` | Renderable meme image |
| `Gif` | `gif { ... }` | Animated GIF (supports transitions) |
| `RenderSurface` | Effects pipeline | In-memory pixel buffer |

## Type Checking

Use `type()` to inspect a value's type at runtime.

```
print type(42);          // number
print type("hi");        // string
print type(true);        // bool
print type(nil);         // nil
print type([1, 2]);      // array
print type({"a": 1});    // map
```
