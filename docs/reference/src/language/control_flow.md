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
for item in items {
    print item;
}
// Output: a  b  c
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
