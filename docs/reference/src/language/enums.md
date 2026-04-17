# Enums

Enums define a closed set of named variants. Variants can optionally carry data.

## Declaration

```
enum Color { Red, Green, Blue }
```

This creates an enum type `Color` with three simple variants. Access variants via `EnumName.Variant`:

```
var c = Color.Red;
print c;           // Red
print type(c);     // Color
```

## Data Variants

Variants can carry named fields:

```
enum Shape { Circle(radius), Rect(width, height), Point }
```

Data variants are constructed by calling them like functions:

```
var circle = Shape.Circle(5);
var rect = Shape.Rect(3, 4);
print circle;      // Circle(5)
print rect;        // Rect(3, 4)
print Shape.Point; // Point
```

Simple variants (no fields) are values, not constructors. Data variants are callable constructors.

## Equality

Enum values support `==` and `!=`. Two enum values are equal if they have the same variant and the same field values:

```
print Color.Red == Color.Red;    // true
print Color.Red == Color.Blue;   // false
```

## Match Destructuring

The `match` expression can destructure enum variants and bind their fields to variables:

```
enum Result { Ok(value), Error(message) }

val msg = match Result.Ok(42) {
    Result.Ok(v) -> "got {v}"
    Result.Error(e) -> "err: {e}"
};
print msg;         // got 42
```

Each match arm uses `EnumName.Variant(bindings)` syntax. The bound variables are available in the result expression.

The wildcard `_` matches any value:

```
val x = match color {
    Color.Red -> "danger"
    _ -> "other"
};
```

If no arm matches and there is no wildcard, the result is `nil`.

## Built-in Enums

The prelude defines two commonly-used enum types:

### Result

```
enum Result { Ok(value), Error(message) }
```

Use `Result` for operations that can succeed or fail:

```
fun divide(a, b) {
    if (b == 0) return Result.Error("division by zero");
    return Result.Ok(a / b);
}

val answer = match divide(10, 2) {
    Result.Ok(v) -> "result: {v}"
    Result.Error(e) -> "error: {e}"
};
print answer;      // result: 5
```

### Option

```
enum Option { Some(value), None }
```

Use `Option` for values that may or may not exist:

```
fun find_first(arr, pred) {
    for (var item in arr) {
        if (pred(item)) return Option.Some(item);
    }
    return Option.None;
}

val found = match find_first([1, 2, 3], (x) -> x > 2) {
    Option.Some(v) -> "found: {v}"
    Option.None -> "not found"
};
print found;       // found: 3
```
