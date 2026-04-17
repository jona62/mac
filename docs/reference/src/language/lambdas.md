# Lambdas

Lambdas are anonymous functions, useful for short callbacks and inline transforms.

## Syntax

```
// No parameters
fun() { body }

// With parameters
fun(a, b) { body }

// Arrow shorthand (single expression, implicit return)
(params) -> expression
```

## Examples

```
var double = fun(x) { return x * 2; };
print double(5);                  // 10

// Arrow form
var triple = (x) -> x * 3;
print triple(5);                  // 15
```

## With Higher-Order Functions

Lambdas are commonly used with `map`, `filter`, and `reduce`:

```
var nums = [1, 2, 3, 4, 5];

var doubled = map(nums, (x) -> x * 2);
print doubled;                    // [2, 4, 6, 8, 10]

var evens = filter(nums, (x) -> x % 2 == 0);
print evens;                      // [2, 4]

var sum = reduce(nums, (acc, x) -> acc + x, 0);
print sum;                        // 15
```

## With Pipes

Lambdas combine naturally with the pipe operator:

```
[1, 2, 3, 4, 5]
    |> filter((x) -> x > 2)
    |> map((x) -> x * 10)
    |> reduce((a, b) -> a + b, 0);
// Result: 120
```

## Closures

Lambdas capture variables from their enclosing scope, just like named functions.

```
var factor = 3;
var scale = (x) -> x * factor;
print scale(10);                  // 30
```

## Meme Factories

Arrow lambdas returning meme expressions create reusable meme generators:

```
var meme_factory = (text) -> @blank text;
meme_factory("Hello!") => "hello.png";
```

## See Also

- [Functions](./functions.md) -- named function declarations
- [Native Functions](../stdlib/native_functions.md) -- `map`, `filter`, `reduce`
