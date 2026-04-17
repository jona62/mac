# Variables

## Declaration

Variables are declared with `var` and an optional initializer.

```
var x = 42;
var name = "Mac";
var uninitialized;        // value is nil
```

Variables must be declared before use. Referencing an undefined variable is a runtime error.

## Immutable Bindings

Use `val` to declare a variable that cannot be reassigned.

```
val name = "Mac";
print name;                       // Mac
name = "other";                   // Runtime Error: Cannot reassign 'val' binding 'name'.
```

A `val` declaration must always have an initializer:

```
val x = 42;
val y = x + 1;                    // Fine — val doesn't freeze the value, just the binding
print y;                          // 43
```

Destructuring also works with `val`:

```
val [a, b] = [1, 2];
print a;                          // 1
```

## Assignment

```
var count = 0;
count = count + 1;
```

## Scoping

Variables are block-scoped. A variable declared inside `{ }` is not visible outside it.

```
var outer = "visible";
{
    var inner = "hidden";
    print outer;          // visible
    print inner;          // hidden
}
// print inner;           // Runtime Error: Undefined variable 'inner'.
```

Inner scopes can read and assign to variables from enclosing scopes.

```
var x = 1;
{
    x = 2;                // assigns to outer x
}
print x;                  // 2
```

## Array Destructuring

Unpack array elements into individual variables using a destructuring pattern.

```
var [a, b, c] = [1, 2, 3];
print a;                  // 1
print b;                  // 2
print c;                  // 3
```

This works with any expression that evaluates to an array:

```
var [first, second] = split("hello-world", "-");
print first;              // hello
print second;             // world
```

Destructuring also works in `for-in` loops — see [Control Flow](control_flow.md#for-in-destructuring).

## Shadowing

A new `var` declaration in an inner scope creates a separate variable that shadows the outer one.

```
var x = "outer";
{
    var x = "inner";
    print x;              // inner
}
print x;                  // outer
```
