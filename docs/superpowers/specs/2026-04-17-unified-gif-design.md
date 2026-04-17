# Unified GIF: Memes as the Single Building Block

## Goal

Simplify Mac's visual type system by making the meme the only building block. Remove the `timeline` keyword and `Frame`/`Timeline` prelude classes. The `gif { }` block absorbs transition syntax. Duration becomes an optional property on memes. Transitions remain grammar between entries inside `gif { }`, not data on memes.

## Type Hierarchy

```
Template (atom)
  = path to an image (built-in name or user file)
  = what @blank, @two_panel, @"photo.jpg" resolve to

Meme (the building block)
  = Template + text + style + effects + optional duration
  = @blank "Hello" |> sepia : 500ms
  = saveable as static image (PNG/JPG) — duration ignored on save
  = usable as a gif frame when given duration

Gif (the output)
  = ordered sequence of memes → animated GIF
  = transitions between entries rendered when present
  = replaces both gif {} and timeline {}
```

## Syntax

### Static memes (unchanged)

```
@blank "Hello";
@blank "Hello" |> sepia;
@blank 720x720 myStyle { top: "Hi" };
```

### Memes with duration

Duration is optional metadata on a meme. Does not change the type.

```
@blank "Hello" : 500ms
@blank "Hello" |> sepia : 1s
```

A meme with duration saved as PNG ignores the duration silently.

### GIF sequences (unified)

```
// Simple GIF — no transitions
gif {
    @blank "Frame 1" : 500ms
    @blank "Frame 2" : 500ms
    @blank "Frame 3" : 1s
} => "simple.gif";

// GIF with transitions (replaces timeline)
gif {
    @blank "Scene 1" : 2s
    --- crossfade 500ms ---
    @blank "Scene 2" : 2s
    --- fadeBlack 300ms easeInOut ---
    @blank "Scene 3" : 2s
} => "cinematic.gif";

// Mixed — some with transitions, some without
gif {
    @blank "A" : 500ms
    @blank "B" : 500ms
    --- crossfade 300ms ---
    @blank "C" : 1s
} => "mixed.gif";
```

### Transitions

Transitions are syntax between gif entries, not properties of memes. A transition describes how to get from the previous frame to the next. They only exist inside `gif { }`.

**Types:** crossfade, slideLeft, slideRight, slideUp, slideDown, wipe, fadeBlack, zoom

**Easing curves:** linear (default), easeIn, easeOut, easeInOut, bounce

**Loop-back:** The last `---` transition applies when looping from the final frame back to the first.

```
gif {
    @blank "A" : 1s
    --- slideLeft 300ms ---
    @blank "B" : 1s
    --- crossfade 500ms ---
}
// crossfade transitions B back to A on loop
```

### Variables and composition

Memes are reusable. Duration travels with the meme.

```
var intro = @blank "Hello" : 2s;
var outro = @blank "Bye" : 2s;

gif {
    intro
    --- crossfade 500ms ---
    outro
} => "greeting.gif";
```

### animate() function

Accepts an array of memes and an optional uniform duration.

- If a uniform duration is provided, all frames use it (existing behavior).
- If no uniform duration is provided, each meme must carry its own `: duration`. Memes without duration default to 500ms.

```
// Per-meme durations
var scenes = [@blank "A" : 500ms, @blank "B" : 1s, @blank "C" : 500ms];
animate(scenes) => "varied.gif";

// Uniform duration (existing behavior)
animate([@blank "A", @blank "B"], 500) => "uniform.gif";
```

## What Gets Removed

### Language

- `timeline` keyword removed from scanner
- `TimelineBlockExpr` AST node removed; `GifBlockExpr` extended with transition support
- `visitTimelineBlockExpr` interpreter visitor removed; logic merged into `visitGifBlockExpr`

### Prelude

- `Timeline` class removed
- `Frame` class removed
- `_timeline_keyframe`, `_timeline_transition`, `_timeline_hold`, `_timeline_loop`, `_timeline_render` native functions removed

### Composition rules

- "Sequence type" category simplified — only Gif remains
- `getRenderSurface()` error messages updated (no more "Cannot use Timeline as a frame")

## What Changes

### Parser (src/Parser.cpp)

`GifBlockExpr` parsing extended to accept `---` transition syntax between entries. The parser currently handles this in `TimelineBlockExpr` — that logic moves into `GifBlockExpr`.

### AST (include/Expr.h)

`GifBlockExpr` gains transition data:
```cpp
struct Entry {
    std::shared_ptr<Expr<T>> meme;
    double durationMs;
    // New: optional transition to NEXT frame
    std::string transitionType;    // "" if none
    double transitionDurationMs;   // 0 if none
    std::string transitionEasing;  // "" = linear
};
```

### Interpreter (include/Interpreter.h)

`visitGifBlockExpr` checks if any entry has transitions:
- **No transitions:** Use `MacGif` engine (simple frame sequence, existing behavior)
- **Any transitions:** Use `MacTimeline` engine (interpolated rendering)

### Analyzer

- Remove `TimelineBlockExpr` handling from `AnalyzerWalk.h`
- Update `GifBlockExpr` handling to emit semantic tokens for transition keywords
- Update `AnalyzerInference.h` — `GifBlockExpr` always infers type "Gif"
- Remove "Timeline" from composition error messages

### Natives

- Remove `TimelineRenderFunction`, `TimelineKeyframeFunction`, `TimelineTransitionFunction`, `TimelineHoldFunction`, `TimelineLoopFunction`
- Remove their registrations from `NativeRegistry.h`
- Update `animate()` to respect per-meme durations

## What Stays

- `MacTimeline` C++ class — internal rendering engine, now accessed only through `gif { }` with transitions
- `MacGif` C++ class — simple frame-sequence engine
- All effects, styles, grid, pipe, compose, save syntax
- `@template` meme literal syntax
- All transition types and easing curves
- Loop-back transition behavior

## Files Affected

| Layer | Files | Change |
|-------|-------|--------|
| Scanner | `include/Scanner.h` | Remove `timeline` keyword |
| AST | `include/Expr.h` | Remove `TimelineBlockExpr`, extend `GifBlockExpr` with transitions |
| Parser | `src/Parser.cpp` | Merge timeline parsing into gif block parser |
| Interpreter | `include/Interpreter.h` | Merge `visitTimelineBlockExpr` into `visitGifBlockExpr` |
| Natives | `include/NativeFunctions.h` | Remove 5 timeline native classes |
| Registry | `include/NativeRegistry.h` | Remove timeline native registrations |
| Prelude | `stdlib/prelude.mac` | Remove Timeline, Frame classes |
| Analyzer walk | `include/AnalyzerWalk.h` | Remove timeline expr handling, update gif handling |
| Analyzer inference | `include/AnalyzerInference.h` | Remove timeline type inference |
| Analyzer types | `include/AnalyzerTypes.h` | No structural change |
| Composition check | `include/NativeFunctions.h` | Update `getRenderSurface()` error messages |
| Tests | `tests/syntax/`, `tests/timeline/` | Rewrite timeline tests as gif tests |
| Analyzer tests | `tests/analyzer/` | Update snapshots and assertions |
| Docs | `docs/reference/src/` | Remove timeline page, update gif page |
| Webapp backend | `webapp/server.py` | Generate gif blocks instead of timeline blocks |
| Webapp frontend | `webapp/static/js/` | Remove timeline UI, transitions move into gif editor |

## Backward Compatibility

**Breaking change.** This is a major version bump (v1.0.0 or v0.3.0 depending on policy).

- `timeline { }` syntax stops parsing — hard error
- `Frame` and `Timeline` prelude classes no longer exist
- Timeline native functions no longer callable
- Existing `.mac` files using `timeline` must be rewritten as `gif` with transitions

## Verification

1. `@blank "Hello" => "test.png"` — static meme saves as PNG
2. `@blank "Hello" : 500ms => "test.png"` — meme with duration saves as PNG (duration ignored)
3. `gif { @blank "A" : 500ms  @blank "B" : 500ms } => "simple.gif"` — simple GIF works
4. `gif { @blank "A" : 2s --- crossfade 500ms --- @blank "B" : 2s } => "trans.gif"` — transitions work
5. `gif { ... --- fadeBlack 300ms --- }` — loop-back transition works
6. `timeline { ... }` — produces a parse error
7. All 69 runtime tests pass (after rewriting timeline tests)
8. All 41 analyzer tests pass (after updating)
9. Webapp generates correct gif blocks with transitions
10. mdBook reference updated — no mention of timeline
