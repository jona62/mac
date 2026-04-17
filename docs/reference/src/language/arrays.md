# Arrays

Ordered, mutable collections of values.

## Literal

```
var empty = [];
var nums = [1, 2, 3];
var mixed = ["hello", 42, true, nil];
```

## Indexing

Zero-based. Use `[]` to read or write elements.

```
var a = [10, 20, 30];
print a[0];                   // 10
a[1] = 99;
print a;                      // [10, 99, 30]
```

## Length

```
print len([1, 2, 3]);         // 3
```

## Mutation

```
var a = [1, 2];
push(a, 3);                   // [1, 2, 3]
var last = pop(a);             // last = 3, a = [1, 2]
```

## Iteration

```
var colors = ["red", "green", "blue"];
for color in colors {
    print color;
}
```

Or with `each`:

```
each(colors, (c) -> print(c));
```

## Functional Operations

```
var nums = [1, 2, 3, 4, 5];

map(nums, (x) -> x * 2);              // [2, 4, 6, 8, 10]
filter(nums, (x) -> x > 3);           // [4, 5]
reduce(nums, (acc, x) -> acc + x, 0); // 15
find(nums, (x) -> x > 3);             // 4
any(nums, (x) -> x > 4);              // true
all(nums, (x) -> x > 0);              // true
```

## Transformation

```
reverse([1, 2, 3]);                    // [3, 2, 1]
sort([3, 1, 2]);                       // [1, 2, 3]
flatten([[1, 2], [3, 4]]);             // [1, 2, 3, 4]
take([1, 2, 3, 4], 2);                 // [1, 2]
drop([1, 2, 3, 4], 2);                 // [3, 4]
zip([1, 2], ["a", "b"]);              // [[1, "a"], [2, "b"]]
enumerate(["a", "b"]);                 // [[0, "a"], [1, "b"]]
join(["a", "b", "c"], ", ");           // "a, b, c"
```

## Nested Arrays

```
var matrix = [[1, 2], [3, 4]];
print matrix[0][1];                    // 2
```

## See Also

- [Native Functions](../stdlib/native_functions.md) -- full list of array operations
