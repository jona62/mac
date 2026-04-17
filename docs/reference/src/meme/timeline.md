# Timeline

Timeline blocks create animated GIFs with transitions between keyframes.

## Syntax

```
timeline {
    keyframe1 : hold_duration
    --- transition_type duration ---
    keyframe2 : hold_duration
    ...
}
```

## Example

```
timeline {
    @blank "Scene 1" : 2s
    --- crossfade 500ms ---
    @blank "Scene 2" : 2s
    --- fadeBlack 300ms ---
    @blank "Scene 3" : 2s
} => "movie.gif";
```

## Transition Types

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

## Easing Curves

Add an easing curve after the transition duration:

```
timeline {
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
| `bounce` | Bouncing effect at end |

## Loop-Back Transition

The last `---` transition applies when the GIF loops back from the final frame to the first, creating seamless loops.

```
timeline {
    @blank "A" : 1s
    --- slideLeft 300ms ---
    @blank "B" : 1s
    --- slideLeft 300ms ---
    @blank "C" : 1s
    --- crossfade 500ms ---
} => "seamless.gif";
// The crossfade at the end transitions C back to A on loop
```

## With Effects

Keyframes can have effects applied:

```
timeline {
    @blank "Day" : 2s
    --- crossfade 500ms ---
    @blank "Night" |> tint("#000044AA") |> vignette : 2s
} => "day_night.gif";
```

## With Grids

```
timeline {
    grid 2x1 { @blank "A" @blank "B" } : 1s
    --- wipe 300ms ---
    grid 2x1 { @blank "C" @blank "D" } : 1s
} => "grid_timeline.gif";
```

## Composition Type

`timeline` produces a sequence type (Timeline). Like GIFs, timelines cannot be nested inside frames or other sequences.

## See Also

- [GIF Animation](./gif.md) -- simpler frame-by-frame animation
- [Effects](./effects.md)
- [Saving Output](./save.md)
