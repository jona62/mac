# Mac Language Reference

**Mac** (Meme as Code) is a programming language where memes are first-class citizens. It combines a general-purpose scripting language with a built-in rendering pipeline for creating memes and animated GIFs.

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
| First-class functions | Functions are values, with closures, lambdas, and implicit returns |
| Classes | Single inheritance, constructors, methods, fields |
| Enums | Exhaustive sum types with match destructuring (`Result`, `Option`) |
| Pattern matching | `match expr { Pattern -> result }` with enum destructuring |
| String interpolation | `"Hello, {name}!"` embeds expressions in strings |
| Destructuring | `var [a, b] = expr` and `for (var [k, v] in pairs)` |
| Immutable bindings | `val x = 42` prevents reassignment |
| Pipe operator | `value \|> func` for pipeline-style composition |
| Partial application | `partial(fn, arg)` creates pre-filled functions |
| Expression blocks | `{ statements; tail_expr }` — last expression is the block's value |
| Meme literals | `@template "text"` creates renderable meme objects |
| Effects | `blur(5)`, `sepia`, `grayscale` — composable image transforms |
| Grid layout | `grid 2x2 { ... }` for multi-panel compositions |
| GIF animation | `gif { ... }` with per-frame timing, transitions, and easing curves |
| Save operator | `expr => "file.png"` writes output to disk |

## Version

This reference documents Mac v0.13.0.
