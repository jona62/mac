# Animation Showcase Design

## Purpose

Five flagship animations for `playground/main.mac` that demonstrate Mac's full range as an animation tool. Each targets a distinct use case and teaches different techniques. Together they serve as both developer documentation and proof that Mac produces real, shareable content.

## Animations

### 1. DEEP - Cinematic Storytelling (existing)

A 4-chapter narrative about ocean descent, discovery, and return. Single GIF.

**Techniques demonstrated:**
- All 8 transition types (crossfade, slideDown/Up/Left/Right, wipe, fadeBlack, zoom)
- All 5 easing curves (linear, easeIn, easeOut, easeInOut, bounce)
- Reusable frame-builder functions (`make_title`, `make_split`, `make_wide`)
- Style factory function building styles from palette maps
- If/else expressions inside style definitions
- 10 named effect pipelines with distinct visual identities per chapter
- Pacing arc: long holds for emotion, rapid cuts for tension

**Status:** Complete. No changes needed.

### 2. LAUNCH - Brand/Marketing Announcement

A product feature reveal sequence. User customizes 3 variables at the top (product name, tagline, features array) and gets a complete announcement GIF.

**Structure:**
1. Dark silence (600ms) -> crossfade
2. Product name reveal, letter-by-letter build (5 frames, 200ms each) with glow effect
3. Tagline fade-in below (1.5s hold) -> wipe
4. Feature cards: each slides in from alternating directions. Data-driven from features array of maps `{ title, detail }`. Each gets a color from a palette cycle via index modulo.
5. Date/CTA card with zoom bounce (1.5s) -> fadeBlack
6. Closing tagline with corona glow (2s)

**Techniques demonstrated:**
- Customizable template (change variables, get different output)
- Data-driven feature cards from array of maps
- Index-based palette cycling (`palette[i % len(palette)]`)
- Alternating transition directions via conditional (`if i % 2 == 0 slideLeft else slideRight` - done manually since gif blocks can't branch, but the frames themselves alternate visual weight)
- Letter-by-letter text reveal using `range() |> map(i -> substr(name, 0, i))`

### 3. PRISM - Generative/Algorithmic Art

A procedural color rotation loop. Zero narrative content - text ("PRISM") is a visual anchor while effects do the storytelling.

**Structure:**
1. Generate 24 frames: `range(0, 360, 15) |> map(angle -> base_frame |> hueShift(angle))`
2. Each frame holds 150ms with crossfade 80ms between = smooth rotation
3. Build via reduce + Gif + .frame() for precise duration control
4. Second layer: reverse the sequence and apply increasing chromatic aberration
5. Concatenate forward + reverse with array spread for seamless bounce loop

**Techniques demonstrated:**
- Pure math-driven generation (no handcrafted frames)
- Effect parameterization from computed values
- Array spread for loop construction `[...forward, ...reverse]`
- reduce + Gif pattern for programmatic animation building
- Content where effects ARE the content, not decoration

### 4. WITNESS - Social/Reaction GIF Template

A reusable reaction GIF format. User sets `setup_text`, `punch_text`, and picks a `reaction_asset`, gets a complete shareable GIF.

**Structure:**
1. Setup text on dark template, clean style (1.5s hold) -> fadeBlack 300ms
2. Meme asset with punch_text, bold style with glow (2s hold)
3. Zoom bounce emphasis on the reaction (re-render same frame with intensified effects)
4. Quick fadeBlack out (400ms)

**Techniques demonstrated:**
- Template pattern: change 3 variables, get different output
- Meme assets as first-class visual elements
- Effect intensification (same frame, progressively stronger effects)
- Minimal code, maximum shareability
- Shows Mac as practical content tool, not just demo engine

### 5. SCOREBOARD - Data Visualization

An animated stat reveal that turns numbers into a visual story. Data array of entries drives the entire animation.

**Structure:**
1. Title card ("THE NUMBERS") with wipe transition
2. Each stat entry slides in one at a time (slideRight, 800ms each)
   - Frame shows label prominently with value, styled by the entry's color
   - Effect intensity scales with value (higher number = more glow)
3. After all stats: grid composite of all entries as a summary dashboard
4. Winner/highlight card with zoom bounce + corona effect

**Techniques demonstrated:**
- Data-driven animation from structured arrays
- Effect intensity computed from data values
- Grid spread for composite dashboard (`grid { ...stat_frames }`)
- Transition sequencing for reveal timing
- String interpolation for dynamic values in frames

## File Structure

All five animations live in `playground/main.mac`, separated by clear section headers. Each section is self-contained (no shared styles/effects between animations except `mono` and `whisper` utility styles).

**Output:** 5 GIFs total
- `deep.gif` - Cinematic story (~10MB, 30+ scenes)
- `launch.gif` - Product announcement (~3MB, ~15 scenes)
- `prism.gif` - Color study loop (~2MB, ~48 frames)
- `witness.gif` - Reaction GIF (~1MB, 4-5 scenes)
- `scoreboard.gif` - Stat reveal (~3MB, ~12 scenes)

## Guide Section

The file opens with the existing 7-technique guide (unchanged). Each animation's section header notes which techniques it demonstrates.

## Constraints

- Each animation must produce exactly one GIF
- No standalone PNG saves (GIFs only)
- Total file should run within the playground's 45s CPU limit
- Each section should be understandable in isolation
- Variables that users would customize should be at the top of each section
