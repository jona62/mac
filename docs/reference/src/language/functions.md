# Functions

## Declaration

```
fun name(param1, param2) {
    // body
    return value;
}
```

Functions are first-class values. They can be stored in variables, passed as arguments, and returned from other functions.

```
fun add(a, b) {
    return a + b;
}
print add(2, 3);          // 5
```

## Return

The `return` statement exits the function and produces a value. If omitted, the function returns `nil`.

```
fun greet(name) {
    return "Hello, " + name + "!";
}
print greet("Mac");       // Hello, Mac!
```

## Closures

Functions capture variables from their enclosing scope.

```
fun makeCounter() {
    var count = 0;
    fun increment() {
        count = count + 1;
        return count;
    }
    return increment;
}

var counter = makeCounter();
print counter();              // 1
print counter();              // 2
```

## Functions as Values

```
fun apply(f, x) {
    return f(x);
}

fun double(n) { return n * 2; }

print apply(double, 5);      // 10
```

## Recursion

Functions can call themselves.

```
fun factorial(n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}
print factorial(5);           // 120
```

## See Also

- [Lambdas](./lambdas.md) -- anonymous functions
- [Pipe operator](./operators.md#pipe) -- `value |> func`
- [Compose operator](./operators.md#compose) -- `func1 >> func2`
