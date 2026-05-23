# Error Handling

Mac reports errors at two stages: **parse time** (syntax errors) and **runtime** (logic errors). All errors print to stderr and cause the process to exit with code 1.

## Parse Errors

Parse errors occur when the source code is syntactically invalid. The parser reports the line number and a description of what it expected.

```
[line 3] Error at '}': Expected ';' after expression.
```

Common causes:

- Missing semicolons after statements
- Unmatched braces, brackets, or parentheses
- Invalid token sequences (e.g. `var = 5` without a name)

## Runtime Errors

Runtime errors occur during execution when an operation is applied to the wrong type or an invalid state is reached. Each error prints `Runtime Error:` followed by a description.

### Type Errors

```
// Arithmetic on non-numbers
"hello" - 5;
// Runtime Error: Operands must be numbers.

// String + non-string (only + between two strings or two numbers)
"hello" + 5;
// Runtime Error: Operands must be two numbers or two strings.

// Negating a non-number
-"text";
// Runtime Error: Operand must be a number.
```

### Call Errors

```
// Calling a non-function
var x = 42;
x();
// Runtime Error: Can only call functions and classes.

// Wrong number of arguments
fun greet(name) { print "Hi, {name}"; }
greet("A", "B");
// Runtime Error: Expected 1 arguments but got 2.
```

### Variable Errors

```
// Undefined variable
print x;
// Runtime Error: Undefined variable 'x'.

// Reassigning an immutable binding
val x = 10;
x = 20;
// Runtime Error: Cannot reassign 'val' binding 'x'.
```

### Index and Property Errors

```
// Out-of-bounds array access
var a = [1, 2, 3];
print a[10];
// Runtime Error: Array index out of bounds.

// Non-numeric index
print a["key"];
// Runtime Error: Array index must be a number.

// Indexing a non-collection
var n = 42;
n[0];
// Runtime Error: Only arrays and maps support indexing.

// Missing map key
var m = { name: "Mac" };
print m["age"];
// Runtime Error: Undefined map key 'age'.

// Property access on a non-instance
var n = 42;
n.field;
// Runtime Error: Only instances and maps have properties.
```

### Arithmetic Errors

```
// Division by zero
10 / 0;
// Runtime Error: Division by zero.

// Modulo by zero
10 % 0;
// Runtime Error: Modulo by zero.
```

### Pipe and Compose Errors

```
// Piping into a non-function
5 |> 10;
// Runtime Error: Pipe target must be callable.

// Composing non-functions
val f = 5 >> 10;
// Runtime Error: Compose (>>) requires two callable values.
```

### Iteration Errors

```
// Iterating over a non-collection
for (var x in 42) { print x; }
// Runtime Error: Can only iterate over arrays and maps.
```

### Class Errors

```
// Non-class as superclass
var x = 42;
class Child < x {}
// Runtime Error: Superclass must be a class.
```

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Parse error or runtime error |

When running a `.mac` file, the process exits with code 1 on the first error. In the REPL, errors are reported but execution continues for the next input.
