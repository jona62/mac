# Control Flow

## if / else

```
if (condition) {
    // then branch
} else {
    // else branch
}
```

The else branch is optional. Parentheses around the condition are required.

```
var x = 10;
if (x > 5) {
    print "big";
} else {
    print "small";
}
// Output: big
```

## while

```
while (condition) {
    // body
}
```

```
var i = 0;
while (i < 3) {
    print i;
    i = i + 1;
}
// Output: 0  1  2
```

## for

The C-style `for` loop with initializer, condition, and increment:

```
for (var i = 0; i < 5; i = i + 1) {
    print i;
}
```

## for-in

Iterate over arrays or maps:

```
var items = ["a", "b", "c"];
for (var item in items) {
    print item;
}
// Output: a  b  c
```

### for-in Destructuring

When iterating over arrays of arrays, you can destructure each element directly in the loop variable:

```
for (var [i, val] in enumerate(["a", "b", "c"])) {
    print "{i}: {val}";
}
// Output: 0: a  1: b  2: c
```

```
for (var [name, score] in zip(["Alice", "Bob"], [95, 87])) {
    print "{name}: {score}";
}
// Output: Alice: 95  Bob: 87
```

This is equivalent to accessing each element by index inside the loop body but more concise.

## match

A `match` expression evaluates a value against a series of patterns and returns the result of the matching arm. It can be used anywhere an expression is valid.

```
var label = match score {
    100 -> "Perfect"
    0 -> "Zero"
    _ -> "Other"
};
```

Each arm is a pattern followed by `->` and a result expression. The `_` wildcard matches any value. If no arm matches and there is no wildcard, the result is `nil`.

```
var x = match 42 {
    0 -> "zero"
    42 -> "forty-two"
    _ -> "other"
};
print x;
// Output: forty-two
```

Patterns are compared using `==`. Any expression can be used as a pattern:

```
var n = 5;
var result = match n {
    2 + 3 -> "five"
    _ -> "not five"
};
// Output: five
```

Match works with any value type -- numbers, strings, booleans, and function return values:

```
var icon = match type(value) {
    "number" -> "#"
    "string" -> "abc"
    _ -> "?"
};
```

Since `match` is an expression, it can be used inline:

```
print match 1 + 1 {
    2 -> "correct"
    _ -> "wrong"
};
// Output: correct
```

### Enum Destructuring

Match can destructure [enum](./enums.md) variants and bind their fields to local variables:

```
enum Result { Ok(value), Error(message) }

val msg = match Result.Ok(42) {
    Result.Ok(v) -> "got {v}"
    Result.Error(e) -> "err: {e}"
};
// Output: got 42
```

Simple enum variants match by identity:

```
enum Color { Red, Green, Blue }
val label = match Color.Red {
    Color.Red -> "danger"
    Color.Green -> "go"
    _ -> "other"
};
// Output: danger
```

## break

Exit the innermost loop immediately.

```
for (var i = 0; i < 10; i = i + 1) {
    if (i == 3) break;
    print i;
}
// Output: 0  1  2
```

## continue

Skip to the next iteration of the innermost loop.

```
for (var i = 0; i < 5; i = i + 1) {
    if (i == 2) continue;
    print i;
}
// Output: 0  1  3  4
```
