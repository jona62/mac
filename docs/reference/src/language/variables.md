# Variables

## Declaration

Variables are declared with `var` and an optional initializer.

```
var x = 42;
var name = "Mac";
var uninitialized;        // value is nil
```

Variables must be declared before use. Referencing an undefined variable is a runtime error.

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
