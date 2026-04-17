# GIF Animation

GIF blocks create animated GIFs from a sequence of meme frames with individual timing.

## Syntax

```
gif {
    frame1 : duration
    frame2 : duration
    ...
}
```

Duration is specified in milliseconds (`ms`) or seconds (`s`).

## Example

```
gif {
    @blank "Frame 1" : 500ms
    @blank "Frame 2" : 500ms
    @blank "Frame 3" : 1s
} => "animation.gif";
```

## Duration Units

| Unit | Example | Description |
|------|---------|-------------|
| `ms` | `500ms` | Milliseconds |
| `s` | `1.5s` | Seconds (converted to ms) |

## With Effects

Each frame can be processed independently:

```
gif {
    @blank "Normal" : 400ms
    @blank "Sepia" |> sepia : 400ms
    @blank "Inverted" |> invert : 400ms
} => "effects.gif";
```

## With Grids

Grid layouts can be used as GIF frames:

```
gif {
    grid 2x1 { @blank "A" @blank "B" } : 500ms
    grid 2x1 { @blank "C" @blank "D" } : 500ms
} => "grid_anim.gif";
```

## Programmatic GIFs

Use the `animate()` function to create GIFs from arrays:

```
var frames = [
    @blank "One",
    @blank "Two",
    @blank "Three"
];
animate(frames, 500) => "uniform.gif";
```

## Looping

GIFs loop infinitely by default.

## Composition Type

`gif` produces a sequence type (Gif). Sequence types:
- Can be saved directly
- Cannot be placed inside grids or other frames
- Cannot be nested inside other gifs or timelines

## See Also

- [Timeline](./timeline.md) -- transitions between frames
- [Saving Output](./save.md)
- [Grid Layout](./grid.md) -- grid frames in GIFs
