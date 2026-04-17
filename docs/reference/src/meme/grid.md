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
