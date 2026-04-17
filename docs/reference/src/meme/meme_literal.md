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

Use `x` and `y` for absolute text positioning.

```
@blank 720x720 {
    text: "Anywhere" x: 100 y: 200
}
```

## See Also

- [Styles](./styles.md) -- text color, outline, font
- [Effects](./effects.md) -- image transforms
- [Templates](../stdlib/templates.md) -- built-in template list
- [Saving Output](./save.md) -- `=>` and `save()`
