# Native Functions

Built-in functions available in every Mac program without imports.

## Time & Introspection

| Function | Signature | Returns | Description |
|----------|-----------|---------|-------------|
| `clock` | `clock()` | `number` | Wall-clock time in seconds |
| `type` | `type(value)` | `string` | Runtime type name |

## String Operations

| Function | Signature | Returns | Description |
|----------|-----------|---------|-------------|
| `len` | `len(str)` | `number` | String length (also works on arrays) |
| `substr` | `substr(str, start, length)` | `string` | Extract substring |
| `split` | `split(str, delimiter)` | `[string]` | Split into array |
| `join` | `join(array, separator)` | `string` | Join array into string |
| `upper` | `upper(str)` | `string` | Convert to uppercase |
| `lower` | `lower(str)` | `string` | Convert to lowercase |
| `trim` | `trim(str)` | `string` | Remove surrounding whitespace |
| `replace` | `replace(str, from, to)` | `string` | Replace all occurrences |

## Math

| Function | Signature | Returns | Description |
|----------|-----------|---------|-------------|
| `sqrt` | `sqrt(n)` | `number` | Square root |
| `abs` | `abs(n)` | `number` | Absolute value |
| `pow` | `pow(base, exp)` | `number` | Exponentiation |
| `floor` | `floor(n)` | `number` | Round down |
| `ceil` | `ceil(n)` | `number` | Round up |

## Array Mutation

| Function | Signature | Returns | Description |
|----------|-----------|---------|-------------|
| `push` | `push(array, value)` | `nil` | Append element (mutates) |
| `pop` | `pop(array)` | `T` | Remove and return last element |

## Array Transforms

| Function | Signature | Returns | Description |
|----------|-----------|---------|-------------|
| `map` | `map(array, fn)` | `[T]` | Transform each element |
| `filter` | `filter(array, fn)` | `[T]` | Keep matching elements |
| `reduce` | `reduce(array, fn, init)` | `T` | Fold with accumulator |
| `find` | `find(array, fn)` | `T` | First matching element |
| `flatMap` | `flatMap(array, fn)` | `[T]` | Map and flatten one level |
| `flatten` | `flatten(array)` | `[T]` | Flatten one nesting level |
| `reverse` | `reverse(array)` | `[T]` | Reversed copy |
| `sort` | `sort(array)` | `[T]` | Sorted copy |
| `sort` | `sort(array, cmp)` | `[T]` | Sort with comparator |
| `take` | `take(array, n)` | `[T]` | First n elements |
| `drop` | `drop(array, n)` | `[T]` | Skip first n elements |
| `zip` | `zip(a, b)` | `[[A, B]]` | Pair elements from two arrays |
| `enumerate` | `enumerate(array)` | `[[number, T]]` | Index-value pairs |
| `each` | `each(array, fn)` | `nil` | Side-effect iteration |

## Array Predicates

| Function | Signature | Returns | Description |
|----------|-----------|---------|-------------|
| `any` | `any(array, fn)` | `bool` | Any element matches |
| `all` | `all(array, fn)` | `bool` | All elements match |

## Generators

| Function | Signature | Returns | Description |
|----------|-----------|---------|-------------|
| `range` | `range(end)` | `[number]` | `[0, 1, ..., end-1]` |
| `range` | `range(start, end)` | `[number]` | `[start, ..., end-1]` |
| `range` | `range(start, end, step)` | `[number]` | With custom step |

## User Input

| Function | Signature | Returns | Description |
|----------|-----------|---------|-------------|
| `input` | `input(prompt)` | `string` | Read line from stdin |

## Image Effects

See [Effects](../meme/effects.md) for the full effect reference.

**Parameterized** (return a meme transform function):

`blur(r)`, `pixelate(s)`, `noise(a)`, `saturate(f)`, `contrast(f)`, `brightness(f)`, `hueShift(d)`, `glow(r)`, `posterize(l)`, `chromatic(o)`, `threshold(l)`, `tint(hex)`, `jpeg(q)`

**Direct** (take a meme, return a meme):

`grayscale`, `sepia`, `invert`, `sharpen`, `vignette`

## Layout & Composition

| Function | Signature | Returns | Description |
|----------|-----------|---------|-------------|
| `beside` | `beside(left, right)` | `Meme` | Side-by-side |
| `stack` | `stack(top, bottom)` | `Meme` | Vertical stack |
| `grid` | `grid(cols, memes)` | `Meme` | Grid arrangement |
| `pad` | `pad(meme, px)` | `Meme` | Add white padding |
| `border` | `border(meme, px)` | `Meme` | Add black border |

## Animation

| Function | Signature | Returns | Description |
|----------|-----------|---------|-------------|
| `animate` | `animate(memes, ms)` | `Gif` | Create GIF with uniform frame duration |
| `toGrid` | `toGrid(memes, cols, rows)` | `Meme` | Render array to grid |

## I/O

| Function | Signature | Returns | Description |
|----------|-----------|---------|-------------|
| `save` | `save(target, path)` | `bool` | Save meme/GIF to file |
