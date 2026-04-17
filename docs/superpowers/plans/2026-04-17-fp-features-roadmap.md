# Functional Programming Features Roadmap

## Phase 1 (Next)

### 1. String Interpolation

Lowest risk, highest everyday impact. Replace `"Hello, " + name + "!"` with `"Hello, {name}!"`.

**Syntax:** `"text {expression} more text"`

```mac
var name = "Mac";
print "Hello, {name}!";              // Hello, Mac!
print "2 + 2 = {2 + 2}";            // 2 + 2 = 4
print "{upper(name)} LANG";          // MAC LANG
@blank "{name} v{version}" => "banner.png";
```

**Scope:** Scanner (tokenize interpolated strings into string parts + expressions), Parser (build concatenation AST), Interpreter (evaluate normally via desugared concatenation). No new runtime types needed.

### 2. Destructuring

Unlock clean iteration over zip/enumerate results and multi-return patterns.

**Syntax:** `var [a, b] = expr;` and `for [k, v] in pairs { }`

```mac
var [x, y] = [10, 20];
var [first, ...rest] = [1, 2, 3, 4];   // rest = [2, 3, 4]

for [i, item] in enumerate(["a", "b", "c"]) {
    print "{i}: {item}";
}

for [name, score] in zip(names, scores) {
    print "{name} scored {score}";
}
```

**Scope:** Parser (new VarStmt/ForInStmt variants), Interpreter (array index extraction at assignment). Optional rest element (`...rest`) is a stretch goal.

### 3. Pattern Matching

Replace if/else chains with expressive match expressions.

**Syntax:** `match expr { pattern -> result }`

```mac
var label = match score {
    100 -> "Perfect"
    0 -> "Zero"
    _ -> "Score: {score}"
};

var icon = match type(value) {
    "number" -> "123"
    "string" -> "abc"
    "bool" -> "T/F"
    _ -> "?"
};

// Use with memes
var meme = match mood {
    "happy" -> @blank "All good!"
    "sad" -> @blank "Not great..." |> grayscale
    _ -> @blank mood
};
meme => "mood.png";
```

**Scope:** New `MatchExpr` AST node, parser for `match { }` blocks, interpreter evaluates each arm until a match. Wildcard `_` always matches. Value comparison only (no structural patterns in phase 1).

---

## Phase 2 (Later)

### 4. More Array Operations

| Function | Signature | Description |
|----------|-----------|-------------|
| `takeWhile(arr, fn)` | `[T] -> [T]` | Take while predicate true |
| `dropWhile(arr, fn)` | `[T] -> [T]` | Drop while predicate true |
| `partition(arr, fn)` | `[T] -> [[T], [T]]` | Split by predicate |
| `groupBy(arr, fn)` | `[T] -> {[T]}` | Group by key function |
| `unique(arr)` | `[T] -> [T]` | Remove duplicates |
| `chunk(arr, n)` | `[T] -> [[T]]` | Split into groups of n |
| `scan(arr, fn, init)` | `[T] -> [T]` | Like reduce but keeps intermediates |

### 5. General Partial Application

```mac
var greet = (greeting, name) -> greeting + " " + name;
var hello = partial(greet, "Hello");
hello("World");                       // "Hello World"
```

Or auto-curry when fewer args are passed than arity.

### 6. Immutable Bindings

```mac
val name = "Mac";      // cannot be reassigned
name = "other";        // compile error
```

### 7. Try/Catch

```mac
var result = try {
    save(meme, "out.png")
} catch (err) {
    print "Failed: {err}";
    false
};
```

### 8. Tail Call Optimization

Optimize recursive calls in tail position to avoid stack overflow. Low priority for a meme scripting language.
