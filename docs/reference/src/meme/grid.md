# Grid Layout

Grid blocks compose multiple memes into a single image arranged in a grid.

## Syntax

```
grid COLSxROWS {
    entry1
    entry2
    ...
}
```

Entries are meme expressions. The grid fills left-to-right, top-to-bottom.

## Example

```
grid 2x2 {
    @blank "Top Left"
    @blank "Top Right"
    @blank "Bottom Left"
    @blank "Bottom Right"
} => "quad.png";
```

## With Effects

Each entry can have its own effects:

```
grid 2x1 {
    @blank "Normal"
    @blank "Sepia" |> sepia
} => "comparison.png";
```

The entire grid can also be piped through effects:

```
grid 2x2 {
    @blank "A"
    @blank "B"
    @blank "C"
    @blank "D"
} |> border(3) => "bordered_grid.png";
```

## With Padding and Borders

```
grid 2x2 {
    @blank "1"
    @blank "2"
    @blank "3"
    @blank "4"
} |> pad(10) |> border(2) => "framed.png";
```

## Nested Grids

Grids can contain other grids:

```
grid 1x2 {
    grid 2x1 {
        @blank "A"
        @blank "B"
    }
    @blank "Footer"
} => "nested.png";
```

## Spread from an Array

When the entries are already in an array, use `...` to spread them into
the grid body, or omit the body entirely and let `grid` derive the shape
from the array's length.

```
var memes = data |> map(d -> @two_panel { top: d[0] bottom: d[1] });

grid 2x2 { ...memes } => "explicit.png";  // explicit shape
grid memes => "auto.png";                  // auto-dimensioned
```

If an explicit `COLSxROWS` is given and the array size doesn't match,
extras are truncated and missing cells are filled with blank.

## Programmatic Composition

For cases where a literal `grid { }` block is awkward, three native
functions compose memes without syntax sugar:

| Function | Description |
|----------|-------------|
| `beside(a, b)` | Place two memes side by side |
| `stack(a, b)` | Stack two memes vertically |
| `toGrid(arr, cols, rows)` | Arrange an array of memes into a grid |

```
beside(@blank "before", @blank "after") => "compare.png";
stack(@blank "setup", @blank "punchline") => "bit.png";
toGrid(memes, 3, 2) => "dashboard.png";
```

## Composition Type

`grid` produces a frame type (Meme), which means it can be used inside:
- Other grids
- GIF frames
- Effect pipelines

But it cannot directly contain sequence types (Gif).

## See Also

- [Meme Literals](./meme_literal.md)
- [GIF Animation](./gif.md)
- [Effects](./effects.md)
