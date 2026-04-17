# Mac Language Reference

**Mac** (Meme as Code) is a programming language where memes are first-class citizens. It combines a general-purpose scripting language with a built-in rendering pipeline for creating memes, GIFs, and animated timelines.

## Quick Example

```
// Create a meme and save it
@two_panel "Writes code" "It works first try" => "miracle.png";

// Or use the full syntax with styles and effects
style bold_red { color: "#FF0000", fontWeight: "bold" }

@blank 720x720 bold_red {
    top: "Mac Language"
    bottom: "Meme as Code"
} |> sepia |> border(3) => "styled.png";
```

## Language Features

| Feature | Description |
|---------|-------------|
| Dynamic typing | Values are strings, numbers, booleans, nil, arrays, or maps |
| First-class functions | Functions are values, with closures and lambdas |
| Classes | Single inheritance, constructors, methods, fields |
| Pipe operator | `value \|> func` for pipeline-style composition |
| Meme literals | `@template "text"` creates renderable meme objects |
| Effects | `blur(5)`, `sepia`, `grayscale` — composable image transforms |
| Grid layout | `grid 2x2 { ... }` for multi-panel compositions |
| GIF animation | `gif { ... }` with per-frame timing |
| Timeline | `timeline { ... }` with transitions and easing curves |
| Save operator | `expr => "file.png"` writes output to disk |

## Version

This reference documents Mac v0.2.3.
