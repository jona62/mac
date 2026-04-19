# Meme Literals

Meme literals create renderable meme images using the `@` prefix.

## Syntax

```
// Quick form — template name and a single text string
@template "text"

// Full form — with dimensions, style, and named text slots
@template [WIDTHxHEIGHT] [styleName] {
    top: "Top text"
    bottom: "Bottom text"
    center: "Center text"
}
```

## Quick Form

A template name followed by a string creates a meme with that text as the top line.

```
@blank "Hello, World!";
@two_panel "Before coffee" "After coffee";
@bottom_text "When the code works";
```

Multi-string memes fill top, bottom, center in order depending on the template.

## Full Form

Use `{ }` for explicit text slot assignment.

```
@blank {
    top: "Mac Language"
    bottom: "Meme as Code"
}
```

## Dimensions

Specify output size with `WIDTHxHEIGHT` before the text block.

```
@blank 1080x1080 {
    top: "HD Meme"
}
```

Default dimensions depend on the template (typically 720x720).

## Custom Images

Use a file path instead of a template name.

```
@"photos/cat.jpg" "Caption text";
```

String template paths (`@"path"`) resolve relative to the script's directory.

## Saving

Memes are saved with the `=>` operator or `save()` function.

```
@blank "Hello" => "hello.png";

var m = @two_panel "Top" "Bottom";
save(m, "meme.jpg");
```

Output files are written to `~/mac/output/` by default.
Explicit relative and absolute save paths are respected unless `MAC_OUTPUT_DIR` is set by the host.

## With Styles

Apply a named style for text customization.

```
style red_bold {
    color: "#FF0000"
    fontWeight: "bold"
}

@blank 720x720 red_bold {
    top: "Styled Text"
}
```

## With Effects

Pipe memes through effect functions.

```
@blank "Vintage" |> sepia |> vignette => "vintage.png";
```

## With Positioned Text

Use `text:` followed by `x:` and `y:` to place text at absolute pixel
coordinates. Multiple positioned entries are allowed and can mix with
the named positions (`top:`, `bottom:`, `center:`).

```
@blank 720x720 {
    top: "HEADER"
    text: "label A" x: 120 y: 200 fontSize: "sm"
    text: "label B" x: 520 y: 200 fontSize: 64
    bottom: "footer"
}
```

`fontSize` accepts either a pixel number or one of the size tier strings
`"sm"`, `"md"`, `"lg"`, `"xlg"`. If omitted, the meme's default sizing
applies.

All positioned text on a single meme shares the meme's outer style
(color, outline, weight, etc.). Per-entry colors are not currently
supported — use `beside`, `stack`, or `grid` to combine memes with
different styles.

## Meme Assets

The `@meme.<name>` syntax resolves to a built-in meme asset image.

```
@meme.shrek_smirk "When your code compiles" => "shrek.png";
@meme.girl_side_eye { bottom: "QA finds a bug" } => "qa.png";
```

Available assets: `shrek_smirk`, `shrek_side_eye`, `girl_side_eye`,
`king_bach_stare`, `jordan_crying`, `kid_crying`, `window_despair`,
`guy_crying`, `idk_about_that`.

## See Also

- [Styles](./styles.md) -- text color, outline, font
- [Effects](./effects.md) -- image transforms
- [Templates](../stdlib/templates.md) -- built-in template list
- [Saving Output](./save.md) -- `=>` and `save()`
