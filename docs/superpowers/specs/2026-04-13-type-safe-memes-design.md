# Type-Safe Memes + Operator Overloading

## Purpose

Replace the string-based meme API with a type-safe system where Templates, Positions, Formats, Sizes, and Durations are first-class types. Add general operator overloading to the Mac language so any class can define `__add__`, `__mul__`, etc. — the meme types use this same mechanism.

## Part 1: General Operator Overloading

When a binary operator is applied and either operand is a class instance, the interpreter checks for a dunder method on the left operand.

| Operator | Method | Unary | Method |
|----------|--------|-------|--------|
| `+` | `__add__(other)` | `-x` | `__neg__()` |
| `-` | `__sub__(other)` | `!x` | `__not__()` |
| `*` | `__mul__(other)` | | |
| `/` | `__div__(other)` | | |
| `%` | `__mod__(other)` | | |
| `==` | `__eq__(other)` | | |
| `!=` | `__ne__(other)` | | |
| `<` | `__lt__(other)` | | |
| `>` | `__gt__(other)` | | |

Implementation: In `visitBinaryExpr`, after the existing type switch, check if left is a `MacInstance` with the dunder method. Call it with right as argument. Same for `visitUnaryExpr`.

Works for ALL user classes:
```mac
class Vec {
  init(x, y) { this.x = x; this.y = y; }
  __add__(other) { return Vec(this.x + other.x, this.y + other.y); }
  __mul__(n) { return Vec(this.x * n, this.y * n); }
}
var v = Vec(1, 2) + Vec(3, 4); // Vec(4, 6)
```

## Part 2: Type-Safe Meme Types

### Types

| Type | Constructor | Operators | Properties |
|------|------------|-----------|------------|
| `Template` | `Template("name_or_path")` | `+ Position` → partial Meme | `.path`, `.width`, `.height` |
| `Position` | Constants: `Top`, `Bottom`, `Center` | — | `.name` |
| `Format` | Constants: `PNG`, `JPG`, `GIF` | — | `.name` |
| `Size` | `Size(w, h)` | `* n`, `+ Size` | `.width`, `.height` |
| `Duration` | `Duration(ms)` | `+ Duration`, `* n` | `.ms` |
| `Meme` | `Meme(template)` | `+ Meme` (side-by-side), `/ Meme` (stack), `+ Duration` → Frame | `.text(pos, str)`, `.save(fmt, path)`, `.resize(size)` |
| `Frame` | `Meme + Duration` | — | `.meme`, `.duration` |
| `Gif` | `Gif()` | `+ Frame` | `.frame(meme, dur)`, `.save(path)`, `.frameCount` |

### API Examples

```mac
// Create
var drake = Template("drake");
var m = Meme(drake)
  .text(Top, "Writing Java")
  .text(Bottom, "Writing Mac");

// Export
m.save(PNG, "output.png");
m.save(JPG, "output.jpg");

// Resize
var small = m.resize(Size(200, 200));

// Composition
var combo = m + m2;           // side-by-side
var stack = m / m2;           // vertical stack

// GIF — method chaining
var g = Gif()
  .frame(m, Duration(300))
  .frame(m2, Duration(500));
g.save("out.gif");

// GIF — operator composition
var f1 = m + Duration(300);   // Frame
var g2 = Gif() + f1 + f2;
g2.save("out.gif");
```

### Removed API

The old `meme()`, `addTemplate()`, `gifMeme()`, `saveGif()` functions are removed. Replaced by:
- `meme()` → `Meme(Template("name"))`
- `addTemplate()` → just use `Template("path/to/image.png")`
- `gifMeme()` → `Gif()`
- `saveGif()` → `Gif().frame(...).save()`

### Implementation

All meme types are implemented as native Mac classes registered at interpreter startup. Their constructors are `MacCallable` objects in the global environment. Methods (including dunder methods) are native callables returned via property access.

Position constants (`Top`, `Bottom`, `Center`) and Format constants (`PNG`, `JPG`, `GIF`) are singleton instances pre-defined in the global environment.

The existing `MemeRenderer` and `GifEncoder` are reused for actual image I/O.

## Files

| Change | Files |
|--------|-------|
| Operator overloading | `Interpreter.h` (visitBinaryExpr, visitUnaryExpr) |
| Meme type classes | New: `include/MemeTypes.h` — all native meme class definitions |
| Remove old API | `NativeFunctions.h`, `Interpreter.h`, `MacMeme.h` |
| MacValue variant | `MacValue.h` — may not need changes if types use MacInstance |
| Tests | `tests/memes/`, `tests/operators/` |
| LSP/extension | `mac-lang/src/analyzer.ts` — register new type names |
