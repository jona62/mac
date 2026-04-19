# GIF Animation

GIF blocks create animated GIFs from a sequence of frames with individual timing and optional transitions between them.

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

## Transitions

Use `---` separators between frames to add transitions. The syntax is:

```
gif {
    frame1 : hold_duration
    --- transition_type duration ---
    frame2 : hold_duration
    ...
}
```

### Example

```
gif {
    @blank "Scene 1" : 2s
    --- crossfade 500ms ---
    @blank "Scene 2" : 2s
    --- fadeBlack 300ms ---
    @blank "Scene 3" : 2s
} => "movie.gif";
```

### Transition Types

| Transition | Description |
|-----------|-------------|
| `crossfade` | Blend between frames |
| `slideLeft` | New frame slides in from right |
| `slideRight` | New frame slides in from left |
| `slideUp` | New frame slides in from bottom |
| `slideDown` | New frame slides in from top |
| `wipe` | Horizontal wipe reveal |
| `fadeBlack` | Fade through black |
| `zoom` | Zoom into new frame |

### Easing Curves

Add an easing curve after the transition duration:

```
gif {
    @blank "Start" : 1s
    --- crossfade 500ms easeInOut ---
    @blank "End" : 1s
} => "eased.gif";
```

| Easing | Description |
|--------|-------------|
| `linear` | Constant speed (default) |
| `easeIn` | Accelerate from rest |
| `easeOut` | Decelerate to rest |
| `easeInOut` | Accelerate then decelerate |

### Loop-Back Transition

The last `---` transition applies when the GIF loops back from the final frame to the first, creating seamless loops.

```
gif {
    @blank "A" : 1s
    --- slideLeft 300ms ---
    @blank "B" : 1s
    --- slideLeft 300ms ---
    @blank "C" : 1s
    --- crossfade 500ms ---
} => "seamless.gif";
// The crossfade at the end transitions C back to A on loop
```

### Mixed Mode

Transitions are optional between any pair of frames. Frames without a `---` separator simply cut to the next frame with no transition:

```
gif {
    @blank "Intro" : 1s
    --- crossfade 500ms ---
    @blank "Main" : 2s
    @blank "Quick Cut" : 500ms
    --- fadeBlack 300ms ---
    @blank "Outro" : 1s
} => "mixed.gif";
```

## With Effects

Each frame can be processed independently:

```
gif {
    @blank "Normal" : 400ms
    @blank "Sepia" |> sepia : 400ms
    @blank "Inverted" |> invert : 400ms
} => "effects.gif";
```

Frames with transitions can also have effects:

```
gif {
    @blank "Day" : 2s
    --- crossfade 500ms ---
    @blank "Night" |> vignette |> brightness(0.5) : 2s
} => "day_night.gif";
```

## With Grids

Grid layouts can be used as GIF frames:

```
gif {
    grid 2x1 { @blank "A" @blank "B" } : 500ms
    --- wipe 300ms ---
    grid 2x1 { @blank "C" @blank "D" } : 500ms
} => "grid_anim.gif";
```

## Programmatic GIFs

Use the `animate()` function to create GIFs from an array of frames where
every frame has the same duration. The duration argument must be a
`Duration` instance, not a raw number.

```
var frames = [
    @blank "One",
    @blank "Two",
    @blank "Three"
];
animate(frames, Duration(500)) => "uniform.gif";
```

For GIFs with per-frame durations, build up a `Gif` with `reduce`:

```
var gif = frames
    |> reduce((g, m) -> g.frame(m, Duration(500)), Gif());
gif.save("uniform.gif");
```

## Looping

GIFs loop infinitely by default.

## Composition Type

`gif` produces a sequence type (Gif). Sequence types:
- Can be saved directly
- Cannot be placed inside grids or other frames
- Cannot be nested inside other gifs

## Operational Limits

- **Frame cap: 500 total frames per GIF.** This includes transition
  frames, which are rendered at 15 fps. A 500ms `crossfade` between two
  keyframes contributes ~7 interpolated frames on top of the keyframes
  themselves. Exceeding 500 frames raises `GIF frame limit exceeded`.
- **Transition frame rate: 15 fps.** A 200ms transition is only 3 frames
  and will look stepped; target 400-800ms for smooth transitions.
- **Image dimensions capped at 4096x4096.** Larger values are clamped.

## See Also

- [Saving Output](./save.md)
- [Grid Layout](./grid.md) -- grid frames in GIFs
- [Effects](./effects.md)
