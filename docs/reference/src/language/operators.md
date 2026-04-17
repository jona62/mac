# Operators

## Arithmetic

| Operator | Description | Example |
|----------|-------------|---------|
| `+` | Addition / string concatenation | `2 + 3` &rarr; `5`, `"a" + "b"` &rarr; `"ab"` |
| `-` | Subtraction / negation | `5 - 3` &rarr; `2`, `-x` |
| `*` | Multiplication | `4 * 3` &rarr; `12` |
| `/` | Division | `10 / 4` &rarr; `2.5` |
| `%` | Modulo | `7 % 3` &rarr; `1` |

## Comparison

| Operator | Description |
|----------|-------------|
| `==` | Equal |
| `!=` | Not equal |
| `<` | Less than |
| `<=` | Less than or equal |
| `>` | Greater than |
| `>=` | Greater than or equal |

Comparisons return `true` or `false`.

## Logical

| Operator | Description |
|----------|-------------|
| `and` | Logical AND (short-circuit) |
| `or` | Logical OR (short-circuit) |
| `!` | Logical NOT |

```
print true and false;     // false
print true or false;      // true
print !true;              // false
```

## Pipe

The pipe operator passes the left-hand value as the first argument to the right-hand function.

```
value |> func
// equivalent to: func(value)
```

Pipes chain naturally for multi-step processing:

```
@blank "Hello"
    |> sepia
    |> blur(3)
    |> border(2)
    => "processed.png";
```

## Compose

The compose operator creates a new function from two existing functions.

```
var transform = sepia >> blur(3) >> border(2);
@blank "Hello" |> transform => "composed.png";
```

## Save

The fat arrow `=>` saves a meme or GIF to a file.

```
@blank "Hello" => "hello.png";
```

See [Saving Output](../meme/save.md) for details.

## Precedence

From lowest to highest:

| Precedence | Operators |
|-----------|-----------|
| 1 | `=>` (save) |
| 2 | `\|>` (pipe) |
| 3 | `>>` (compose) |
| 4 | `or` |
| 5 | `and` |
| 6 | `==` `!=` |
| 7 | `<` `<=` `>` `>=` |
| 8 | `+` `-` |
| 9 | `*` `/` `%` |
| 10 | `!` `-` (unary) |
| 11 | `.` `()` `[]` (call, access) |
