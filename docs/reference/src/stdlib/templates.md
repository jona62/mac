# Templates

Built-in meme templates available by name with the `@` prefix.

## Built-in Templates

| Template | Description | Text Slots |
|----------|-------------|-----------|
| `blank` | White canvas | top, bottom, center |
| `dark` | Dark/black canvas | top, bottom, center |
| `two_panel` | Two-panel vertical split | top, bottom |
| `three_panel` | Three-panel vertical | top, center, bottom |
| `four_panel` | 2x2 grid | (4 entries) |
| `bottom_text` | Image with bottom caption | bottom |
| `caption_bar` | White bar caption above image | top, bottom |
| `wide` | Wide aspect ratio canvas | top, bottom, center |
| `tall` | Tall aspect ratio canvas | top, bottom, center |
| `square` | Square canvas | top, bottom, center |

## Usage

```
@blank "Hello World";
@two_panel "Expectation" "Reality";
@four_panel "A" "B" "C" "D";
```

## Custom Templates

Use a file path to any image as a template:

```
@"photos/cat.jpg" "Caption";
@"assets/background.png" { top: "Title" bottom: "Subtitle" }
```

String template paths resolve relative to the script's directory.

## Meme Templates (assets)

Additional templates from `assets/templates/meme/` are available with the `meme.` prefix:

```
@meme.shrek_smirk "When the code compiles";
@meme.kid_crying "Production is down";
@meme.distracted_boyfriend "The old plan";
@meme.cat_explosion "Deploying on Friday";
```

Bundled meme templates include the original reaction pack plus additional
templates such as `distracted_boyfriend`, `will_smith_presenting`,
`thousand_yard_stare`, `cat_explosion`, `drake_reaction_grid`,
`cow_dolphin_jump`, `evil_cat_throne`, `shaq_timeout`,
`this_is_fine_empty_room`, `stonks_arrow`, and `empty_oval_office`,
plus newer additions such as `windows_xp_bliss`,
`spongebob_group_stare`, `squidward_window_stare`, `rock_bliss_peek`,
`mona_lisa_side_eye`, `krusty_krab_kitchen`, `doge_windows_hill`, and
`tube_dogs_hill`. Available templates depend on the installed asset pack.

## See Also

- [Meme Literals](../meme/meme_literal.md) -- using templates
- [Styles](../meme/styles.md) -- text customization
