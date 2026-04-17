# Effects

Effects are image transforms applied to memes via the pipe operator or composition.

## Usage

```
// Pipe a meme through an effect
@blank "Hello" |> sepia => "sepia.png";

// Chain multiple effects
@blank "Vintage" |> sepia |> vignette |> blur(2) => "vintage.png";

// Compose effects into a reusable transform
var retro = sepia >> vignette >> noise(0.05);
@blank "Retro" |> retro => "retro.png";
```

## Non-Parameterized Effects

These take a meme directly and return a transformed meme.

| Effect | Description |
|--------|-------------|
| `grayscale` | Convert to grayscale |
| `sepia` | Warm sepia tone |
| `invert` | Invert all colors |
| `sharpen` | Sharpen edges |
| `vignette` | Darken edges |

```
@blank "Test" |> grayscale => "gray.png";
```

## Parameterized Effects

These take a numeric argument and return a function that transforms a meme.

| Effect | Parameter | Description |
|--------|-----------|-------------|
| `blur(radius)` | 1-20 | Gaussian blur |
| `pixelate(blockSize)` | 2-50 | Pixelation |
| `noise(amount)` | 0.0-1.0 | Random noise overlay |
| `saturate(factor)` | 0.0-5.0 | Color saturation (1.0 = normal) |
| `contrast(factor)` | 0.0-5.0 | Contrast (1.0 = normal) |
| `brightness(factor)` | 0.0-3.0 | Brightness (1.0 = normal) |
| `hueShift(degrees)` | 0-360 | Rotate hue |
| `glow(radius)` | 1-20 | Bloom/glow effect |
| `posterize(levels)` | 2-32 | Reduce color levels |
| `chromatic(offset)` | 1-20 | RGB channel displacement |
| `threshold(level)` | 0-255 | Black/white binarization |
| `tint(hexColor)` | hex string | Color tint overlay |
| `jpeg(quality)` | 1-100 | JPEG compression artifacts |

```
@blank "Blurry" |> blur(5) => "blurred.png";
@blank "Warm" |> tint("#FF880044") => "tinted.png";
```

## Named Effects

Define reusable effect compositions with the `effect` keyword.

```
effect deepfry = saturate(3.0) >> contrast(2.0) >> jpeg(10) >> noise(0.1);
effect lofi = grayscale >> noise(0.08) >> vignette;

@blank "Deep Fried" |> deepfry => "fried.png";
@blank "Lo-Fi" |> lofi => "lofi.png";
```

## Effect Composition

Use `>>` to compose effects into a pipeline.

```
var dramatic = contrast(1.5) >> saturate(1.3) >> vignette;
var subtle = blur(1) >> brightness(1.1);

// Composed effects can be further composed
var full = dramatic >> subtle;
```

## See Also

- [Pipe operator](../language/operators.md#pipe)
- [Compose operator](../language/operators.md#compose)
- [Meme Literals](./meme_literal.md)
