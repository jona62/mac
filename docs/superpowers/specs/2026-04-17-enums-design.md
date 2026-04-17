# Enums: Exhaustive Sum Types

## Goal

Add `enum` as a new keyword for defining closed, tagged variant types. Enums integrate with `match` for destructuring and exhaustiveness checking. This is the foundation for Result/Error handling and user-defined algebraic data types.

## Syntax

### Simple enums (no data)

```mac
enum Color { Red, Green, Blue }

val c = Color.Red;
print c;                          // Red

val label = match c {
    Color.Red -> "danger"
    Color.Green -> "go"
    Color.Blue -> "info"
};
```

### Data-carrying enums

```mac
enum Result {
    Ok(value)
    Error(message)
}

fun divide(a, b) {
    if (b == 0) return Result.Error("division by zero");
    return Result.Ok(a / b);
}

match divide(10, 0) {
    Result.Ok(v) -> print "got {v}"
    Result.Error(e) -> print "failed: {e}"
};
```

### Mixed variants

```mac
enum Shape {
    Circle(radius)
    Rect(width, height)
    Point
}

fun area(s) {
    return match s {
        Shape.Circle(r) -> 3.14 * r * r
        Shape.Rect(w, h) -> w * h
        Shape.Point -> 0
    };
}
```

## Semantics

### Constructing variants

Each variant is a constructor function namespaced under the enum name:

- `Color.Red` — simple variant, evaluates to a tagged value
- `Result.Ok(42)` — data variant, call with arguments
- `Shape.Rect(3, 4)` — multi-field data variant

### Runtime representation

An enum value is a tagged object with:
- A `tag` — the variant name (e.g., `"Ok"`, `"Red"`)
- An `enumName` — the enum name (e.g., `"Result"`, `"Color"`)
- Optional `fields` — an array of the data values

Internally this can be a `MacInstance` of a special enum class, or a new variant in `MacValue`. Recommendation: new lightweight C++ class `MacEnum` added to the `MacValue` variant — avoids conflating with class instances.

### Match destructuring

`match` currently does value equality (`==`). For enums, it needs pattern destructuring:

```mac
match expr {
    EnumName.Variant(binding1, binding2) -> result_expr
    EnumName.SimpleVariant -> result_expr
    _ -> default_expr
}
```

When the match arm is a variant pattern:
1. Check if the subject's tag matches the variant name and enum matches the enum name
2. If the variant has fields, bind them to the named variables in the arm's scope
3. Evaluate the result expression with those bindings in scope

### Exhaustiveness

When the analyzer can determine the subject is an enum type (via type inference), it checks that all variants are covered. If not, and there's no `_` wildcard, emit a warning:

```mac
enum Result { Ok(value), Error(message) }

match result {
    Result.Ok(v) -> v
};
// Analyzer warning: non-exhaustive match — missing Result.Error
```

This is an analyzer warning, not a runtime error. The runtime returns `nil` for unmatched values (same as today).

### Printing

Enum values print as their construction form:

```mac
print Result.Ok(42);              // Ok(42)
print Color.Red;                  // Red
print Shape.Rect(3, 4);           // Rect(3, 4)
```

### Equality

Two enum values are equal if they have the same enum name, tag, and field values:

```mac
print Result.Ok(1) == Result.Ok(1);     // true
print Result.Ok(1) == Result.Ok(2);     // false
print Result.Ok(1) == Result.Error(1);  // false
print Color.Red == Color.Red;           // true
```

### type() function

```mac
print type(Color.Red);            // Color
print type(Result.Ok(42));        // Result
```

## Prelude additions

```mac
enum Result {
    Ok(value)
    Error(message)
}

enum Option {
    Some(value)
    None
}
```

These are defined in `stdlib/prelude.mac` using the new enum syntax, making them available in every program.

## What changes

| Layer | Change |
|-------|--------|
| Scanner | Add `enum` keyword |
| Parser | Parse `enum Name { Variant, Variant(fields) }` into `EnumStmt` |
| AST | New `EnumStmt` in `Stmt.h`, new `EnumAccessExpr` in `Expr.h` for `Enum.Variant` |
| Interpreter | `visitEnumStmt` registers enum in environment, `visitEnumAccessExpr` returns constructor or simple value, match gains variant pattern destructuring |
| MacValue | Add `MacEnum` to the variant (tag, enumName, fields vector) |
| Resolver | Walk new nodes |
| Analyzer | Type inference for enums, exhaustiveness checking in match |
| Tests | Enum tests, match destructuring tests, exhaustiveness tests |
| Docs | New enum page in reference |

## What does NOT change

- `class` keyword — untouched
- Existing `match` behavior for non-enum values — still does equality
- No `impl` blocks (future)
- No generics (Result holds `value` as a dynamic type, not `Result<T, E>`)
- No traits

## Match pattern grammar (extended)

Current: `pattern -> result` where pattern is any expression (compared with `==`) or `_` (wildcard).

New: pattern can also be `EnumName.Variant` or `EnumName.Variant(binding1, binding2)`.

The parser distinguishes these from regular expressions by recognizing the `Identifier.Identifier` or `Identifier.Identifier(args)` form in match arms.

## Examples

### Error handling

```mac
fun readFile(path) {
    if (path == "") return Result.Error("empty path");
    return Result.Ok("file contents");
}

val content = match readFile("test.txt") {
    Result.Ok(data) -> data
    Result.Error(msg) -> "fallback: {msg}"
};
print content;
```

### State machines

```mac
enum State { Idle, Loading, Done(data), Failed(error) }

fun render(state) {
    return match state {
        State.Idle -> "Click to start"
        State.Loading -> "Loading..."
        State.Done(d) -> "Got: {d}"
        State.Failed(e) -> "Error: {e}"
    };
}
```

### Simple enums as labels

```mac
enum Direction { Up, Down, Left, Right }

fun move(pos, dir) {
    return match dir {
        Direction.Up -> pos + 1
        Direction.Down -> pos - 1
        Direction.Left -> pos - 10
        Direction.Right -> pos + 10
    };
}
```
