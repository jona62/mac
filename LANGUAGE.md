# Mac Language Reference

## Types

| Type | Example |
|------|---------|
| Number | `42`, `3.14` |
| String | `"hello"` |
| Boolean | `true`, `false` |
| Nil | `nil` |
| Array | `[1, 2, 3]` |
| Map | `{name: "Mac", version: 1}` |

## Variables

```mac
var x = 10;
var y;          // nil
x = 20;
```

## Operators

`+` `-` `*` `/` `%` `==` `!=` `<` `>` `<=` `>=` `!` `and` `or`

## Control Flow

```mac
if (condition) { ... } else { ... }
while (condition) { ... }
for (var i = 0; i < 10; i = i + 1) { ... }
for (var x in [1, 2, 3]) { ... }
break;
continue;
```

## Functions

```mac
fun greet(name) {
    return "Hello, " + name;
}

// Lambdas
var double = fun(x) { return x * 2; };

// Closures
fun counter() {
    var n = 0;
    return fun() { n = n + 1; return n; };
}
```

## Classes

```mac
class Animal {
    init(name) { this.name = name; }
    speak() { return "..."; }
}

class Dog < Animal {
    speak() { return "Woof!"; }
    parent() { return super.speak(); }
}

print Dog("Rex").speak();
```

## Operator Overloading

Any class can define dunder methods:

```mac
class Vec {
    init(x, y) { this.x = x; this.y = y; }
    __add__(other) { return Vec(this.x + other.x, this.y + other.y); }
    __mul__(n) { return Vec(this.x * n, this.y * n); }
    __neg__() { return Vec(-this.x, -this.y); }
    __eq__(other) { return this.x == other.x and this.y == other.y; }
}
```

| Operator | Method | Operator | Method |
|----------|--------|----------|--------|
| `+` | `__add__` | `==` | `__eq__` |
| `-` | `__sub__` | `!=` | `__ne__` |
| `*` | `__mul__` | `<` | `__lt__` |
| `/` | `__div__` | `>` | `__gt__` |
| `%` | `__mod__` | `-x` | `__neg__` |

## Built-in Functions

| Function | Description |
|----------|-------------|
| `clock()` | Unix timestamp |
| `type(x)` | Type name as string |
| `len(x)` | Length of string or array |
| `input(prompt)` | Read line from stdin |
| `substr(s, start, len)` | Substring |
| `split(s, delim)` | Split string into array |
| `sqrt(n)` `abs(n)` `pow(b, e)` | Math |
| `floor(n)` `ceil(n)` | Rounding |
| `push(arr, val)` `pop(arr)` | Array mutation |
| `map(arr, fn)` `filter(arr, fn)` | Functional |

## Arrays and Maps

```mac
var a = [1, 2, 3];
print a[0];         // 1
a[1] = 99;
push(a, 4);

var m = {name: "Mac"};
print m.name;       // Mac
print m["name"];    // Mac
m.version = 1;

for (var x in a) print x;
for (var key in m) print key;
```
