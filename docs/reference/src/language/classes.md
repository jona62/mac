# Classes

Mac supports single-inheritance classes with constructors, methods, and fields.

## Declaration

```
class Name {
    init(params) {
        this.field = value;
    }
    method(params) {
        return this.field;
    }
}
```

## Constructor

The `init` method is called automatically when a class is invoked as a function.

```
class Point {
    init(x, y) {
        this.x = x;
        this.y = y;
    }
}

var p = Point(3, 4);
print p.x;                   // 3
```

## Methods

Methods are functions that have access to `this`.

```
class Circle {
    init(radius) {
        this.radius = radius;
    }
    area() {
        return 3.14159 * this.radius * this.radius;
    }
}

var c = Circle(5);
print c.area();               // 78.53975
```

## Fields

Fields are created by assigning to `this.name` inside any method. There are no field declarations outside methods.

```
class Config {
    init() {
        this.debug = false;
        this.verbose = true;
    }
}
```

## Inheritance

Use `<` to extend a superclass.

```
class Animal {
    init(name) { this.name = name; }
    speak() { return this.name + " makes a sound"; }
}

class Dog < Animal {
    init(name) { super.init(name); }
    speak() { return this.name + " barks"; }
}

var d = Dog("Rex");
print d.speak();              // Rex barks
```

## super

The `super` keyword calls the superclass version of a method.

```
class Cat < Animal {
    init(name) { super.init(name); }
    describe() {
        return super.speak() + " (it's a cat)";
    }
}
```

## Operator Overloading

Methods named `__add__`, `__mul__`, `__eq__` override `+`, `*`, `==` respectively.

```
class Vec {
    init(x, y) { this.x = x; this.y = y; }
    __add__(other) {
        return Vec(this.x + other.x, this.y + other.y);
    }
}

var a = Vec(1, 2);
var b = Vec(3, 4);
var c = a + b;
print c.x;                    // 4
```

## Visibility

Fields and methods prefixed with `_` are treated as internal by the analyzer (LSP warnings for external access).

```
class Secret {
    init() {
        this._hidden = 42;    // internal
        this.visible = true;  // public
    }
    _helper() { }             // internal
    api() { }                 // public
}
```

## See Also

- [Prelude Classes](../stdlib/prelude_classes.md) -- built-in Size, Duration, Meme, Gif, Timeline
