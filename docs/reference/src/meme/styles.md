# Styles

Style blocks define reusable text rendering configurations.

## Syntax

```
style name {
    property: value
    property: value
}
```

## Properties

| Property | Type | Default | Description |
|----------|------|---------|-------------|
| `color` | hex string | `"#FFFFFF"` | Text color (`#RRGGBB` or `#RRGGBBAA`) |
| `outline` | number | `3` | Outline stroke width in pixels |
| `outlineWidth` | number | `3` | Alias for `outline` |
| `outlineColor` | hex string | `"#000000"` | Outline color |
| `shadow` | number | `0` | Drop shadow offset in pixels |
| `shadowColor` | hex string | `"#000000"` | Shadow color |
| `fontSize` | number or string | `"md"` | Font size: pixels or `"sm"`, `"md"`, `"lg"`, `"xlg"` |
| `fontWeight` | string | `"bold"` | `"normal"` or `"bold"` |
| `textTransform` | string | `"uppercase"` | `"none"` or `"uppercase"` |
| `background` | hex string | none | Background fill color (with alpha) |

## Example

```
style impact {
    color: "#FFFFFF"
    outline: 4
    outlineColor: "#000000"
    fontWeight: "bold"
    textTransform: "uppercase"
}

style caption {
    color: "#000000"
    outline: 0
    fontWeight: "normal"
    textTransform: "none"
    background: "#FFFFFFCC"
}

@blank 720x720 impact { top: "Impact Style" } => "impact.png";
@blank 720x720 caption { bottom: "Caption Style" } => "caption.png";
```

## Font Sizes

| Preset | Approximate Size |
|--------|-----------------|
| `"sm"` | Small |
| `"md"` | Medium (default) |
| `"lg"` | Large |
| `"xlg"` | Extra large |

Or specify exact pixel size:

```
style small_text { fontSize: 24 }
style huge_text { fontSize: 72 }
```

## Background Color

The `background` property fills the area behind text. Use alpha for transparency.

```
style subtitle {
    color: "#FFFFFF"
    background: "#00000099"
    outline: 0
    textTransform: "none"
}
```

## Inline Style

Styles can also be applied using keyword modifiers after the template:

```
@blank cinematic { top: "Movie Style" }
```

Built-in style keywords:

| Keyword | Effect |
|---------|--------|
| `impact` | White text, black outline, bold, uppercase |
| `cinematic` | Letter-boxed look |
| `shout` | Large bold uppercase |
| `whisper` | Small, normal weight |
| `panic` | Red text, heavy outline |

## See Also

- [Meme Literals](./meme_literal.md) -- using styles with memes
- [Effects](./effects.md) -- image-level transforms
