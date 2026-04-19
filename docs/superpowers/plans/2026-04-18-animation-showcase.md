# Animation Showcase Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add four new flagship animations (LAUNCH, PRISM, WITNESS, SCOREBOARD) to `playground/main.mac` alongside the existing DEEP animation.

**Architecture:** Each animation is a self-contained section appended to `playground/main.mac`. Shared utility styles (`mono`, `whisper`) are defined once in the DEEP section. Each new section defines its own styles, effects, and data. All output is GIF-only.

**Tech Stack:** Mac language (`.mac` files), run with `./build/mac`

---

### Task 1: LAUNCH - Brand/Marketing Announcement

**Files:**
- Modify: `playground/main.mac` (append after DEEP section)

- [ ] **Step 1: Append the LAUNCH animation to main.mac**

Add this after line 195 (`print "deep.gif saved";`):

```mac


// ============================================================
// "LAUNCH" - product feature announcement
// Techniques: customizable template, data-driven cards,
//   letter-by-letter reveal, alternating transitions
//
// Customize: product_name, tagline, launch_date, features
// ============================================================

val product_name = "NEXUS";
val tagline = "the future, shipped";
val launch_date = "06.01.26";

val features = [
    { title: "INSTANT",  detail: "zero latency" },
    { title: "INFINITE", detail: "no limits" },
    { title: "SECURE",   detail: "trust nothing" },
];

val brand_colors = ["#00EEFF", "#FF00AA", "#AAFF00", "#FF8800"];

effect reveal = glow(8) >> contrast(1.3) >> brightness(1.1);
effect card_pop = glow(6) >> contrast(1.5) >> saturate(1.6);
effect grand = saturate(3.0) >> glow(14) >> brightness(1.2);

style brand_whisper {
    color: "#555555"
    outline: 0
    bold: false
    uppercase: false
}

// Letter-by-letter name reveal
val name_frames = range(1, len(product_name) + 1) |> map(i -> {
    style build {
        color: "#FFFFFF"
        outline: 4
        outlineColor: "#111111"
        shadow: 6
        shadowColor: "#FFFFFF44"
    }
    @dark 800x450 build { center: substr(product_name, 0, i) } |> reveal
});

// Feature cards from data
val feature_frames = range(len(features)) |> map(i -> {
    val f = features[i];
    val c = brand_colors[i % len(brand_colors)];
    style feat {
        color: c
        outline: 3
        outlineColor: "#111111"
        shadow: 5
        shadowColor: "{c}66"
    }
    @dark 800x450 feat { top: f["title"] bottom: f["detail"] } |> card_pop
});

// Build the full animation
val launch_intro = name_frames
    |> reduce((g, f) -> g.frame(f, Duration(200)), Gif());

// Add tagline hold
val with_tagline = launch_intro
    .frame(@dark 800x450 brand_whisper { center: tagline } |> glow(4), Duration(1500));

// Add feature cards
val with_features = feature_frames
    |> reduce((g, f) -> g.frame(f, Duration(1200)), with_tagline);

// Date reveal + closing
style date_style {
    color: "#FFD700"
    outline: 4
    outlineColor: "#332b00"
    shadow: 8
    shadowColor: "#FFD70077"
}

with_features
    .frame(@dark 800x450 date_style { center: launch_date } |> grand, Duration(1500))
    .frame(@dark 800x450 brand_whisper { center: tagline } |> glow(3) >> brightness(0.6), Duration(2000))
    .save("launch.gif");

print "launch.gif saved";
```

- [ ] **Step 2: Run and verify**

Run: `./build/mac playground/main.mac`
Expected: `deep.gif saved` then `launch.gif saved`, both in `~/mac/output/`

- [ ] **Step 3: Visually verify launch.gif**

Check that the GIF shows: letter-by-letter "NEXUS" build, tagline, 3 colored feature cards, gold date, closing tagline. No text overlap.

- [ ] **Step 4: Commit**

```bash
git add playground/main.mac
git commit -m "feat: add LAUNCH animation - brand/marketing announcement template"
```

---

### Task 2: PRISM - Generative/Algorithmic Art

**Files:**
- Modify: `playground/main.mac` (append after LAUNCH section)

- [ ] **Step 1: Append the PRISM animation to main.mac**

```mac


// ============================================================
// "PRISM" - procedural color study
// Techniques: math-driven generation, effect parameterization,
//   array spread for bounce loops, reduce+Gif pattern
//
// No customization needed - pure algorithmic output
// ============================================================

style prism_anchor {
    color: "#FFFFFF"
    outline: 5
    outlineColor: "#222222"
    shadow: 3
    shadowColor: "#FFFFFF33"
}

effect prism_base = glow(8) >> contrast(1.3) >> saturate(1.8);

val base_frame = @dark 600x600 prism_anchor { center: "PRISM" } |> prism_base;

// Generate 24 hue-shifted frames across the full spectrum
val forward = range(0, 360, 15) |> map(angle ->
    base_frame |> hueShift(angle) >> glow(4)
);

// Reverse pass with increasing chromatic aberration
val backward = range(24) |> map(i ->
    forward[23 - i] |> chromatic(1 + i / 4)
);

// Bounce loop: forward + reverse
val prism_loop = [...forward, ...backward]
    |> reduce((g, f) -> g.frame(f, Duration(130)), Gif());

prism_loop.save("prism.gif");

print "prism.gif saved";
```

- [ ] **Step 2: Run and verify**

Run: `./build/mac playground/main.mac`
Expected: `deep.gif saved`, `launch.gif saved`, `prism.gif saved`

- [ ] **Step 3: Visually verify prism.gif**

Check that the GIF shows smooth color rotation through the full spectrum, then reverses back with chromatic aberration. Should feel like a seamless loop.

- [ ] **Step 4: Commit**

```bash
git add playground/main.mac
git commit -m "feat: add PRISM animation - procedural color study loop"
```

---

### Task 3: WITNESS - Social/Reaction GIF Template

**Files:**
- Modify: `playground/main.mac` (append after PRISM section)

- [ ] **Step 1: Append the WITNESS animation to main.mac**

```mac


// ============================================================
// "WITNESS" - reaction GIF template
// Techniques: customizable template, meme assets,
//   effect intensification, minimal code
//
// Customize: setup_text, punch_text, reaction asset
// ============================================================

val setup_text = "when the code compiles";
val punch_text = "on the first try";

style setup_style {
    color: "#FFFFFF"
    outline: 3
    outlineColor: "#000000"
    shadow: 2
    shadowColor: "#FFFFFF33"
}

style punch_style {
    color: "#FFD700"
    outline: 4
    outlineColor: "#332b00"
    shadow: 6
    shadowColor: "#FFD70088"
}

effect punch_hit = saturate(2.0) >> glow(8) >> contrast(1.4);
effect punch_hard = saturate(3.0) >> glow(14) >> contrast(1.8) >> chromatic(3);

gif {
    // Setup
    @dark 600x600 setup_style { center: setup_text } : 1.5s
    --- fadeBlack 300ms ---

    // Reaction
    @meme.shrek_smirk punch_style { bottom: punch_text } |> punch_hit : 1.5s
    --- zoom 200ms bounce ---

    // Emphasis - same reaction, harder effects
    @meme.shrek_smirk punch_style { bottom: punch_text } |> punch_hard : 1s
    --- fadeBlack 400ms ---
} => "witness.gif";

print "witness.gif saved";
```

- [ ] **Step 2: Run and verify**

Run: `./build/mac playground/main.mac`
Expected: all four `saved` messages

- [ ] **Step 3: Visually verify witness.gif**

Check: dark setup text, shrek reaction with gold punchline, then intensified version of the same frame. Should feel like a shareable reaction GIF.

- [ ] **Step 4: Commit**

```bash
git add playground/main.mac
git commit -m "feat: add WITNESS animation - reaction GIF template"
```

---

### Task 4: SCOREBOARD - Data Visualization

**Files:**
- Modify: `playground/main.mac` (append after WITNESS section)

- [ ] **Step 1: Append the SCOREBOARD animation to main.mac**

```mac


// ============================================================
// "SCOREBOARD" - animated stat reveal
// Techniques: data-driven frames, effect intensity from values,
//   grid spread dashboard, string interpolation
//
// Customize: stats array and title
// ============================================================

val board_title = "Q4 RESULTS";

val stats = [
    { label: "USERS",    value: "12.4K", color: "#00EEFF", intensity: 4 },
    { label: "REVENUE",  value: "$840K", color: "#00FF88", intensity: 6 },
    { label: "GROWTH",   value: "+37%",  color: "#AAFF00", intensity: 8 },
    { label: "UPTIME",   value: "99.9%", color: "#FFD700", intensity: 10 },
];

effect board_base = contrast(1.3) >> glow(4);

style title_bar {
    color: "#FFFFFF"
    outline: 3
    outlineColor: "#111111"
    shadow: 4
    shadowColor: "#FFFFFF44"
}

// Generate a styled frame per stat
val stat_frames = stats |> map(s -> {
    style stat {
        color: s["color"]
        outline: 3
        outlineColor: "#111111"
        shadow: s["intensity"]
        shadowColor: "{s[\"color\"]}88"
    }
    @dark 700x400 stat {
        top: s["label"]
        bottom: s["value"]
    } |> board_base >> glow(s["intensity"])
});

// Find the winner (last entry = highest intensity)
val winner = stats[len(stats) - 1];

style winner_style {
    color: winner["color"]
    outline: 5
    outlineColor: "#222222"
    shadow: 12
    shadowColor: "{winner[\"color\"]}99"
}

effect winner_glow = saturate(3.0) >> glow(16) >> contrast(1.5) >> chromatic(2);

gif {
    // Title
    @dark 700x400 title_bar { center: board_title } |> board_base : 1.5s
    --- wipe 400ms ---

    // Reveal each stat
    stat_frames[0] : 1s
    --- slideRight 300ms easeOut ---
    stat_frames[1] : 1s
    --- slideRight 300ms easeOut ---
    stat_frames[2] : 1s
    --- slideRight 300ms easeOut ---
    stat_frames[3] : 1s
    --- fadeBlack 400ms ---

    // Winner highlight
    @dark 700x400 winner_style { top: winner["label"] bottom: winner["value"] } |> winner_glow : 2s
    --- zoom 300ms bounce ---
    @dark 700x400 winner_style { center: winner["value"] } |> winner_glow : 1.5s
    --- fadeBlack 600ms ---
} => "scoreboard.gif";

print "scoreboard.gif saved";
```

- [ ] **Step 2: Run and verify**

Run: `./build/mac playground/main.mac`
Expected: all five `saved` messages: `deep.gif`, `launch.gif`, `prism.gif`, `witness.gif`, `scoreboard.gif`

- [ ] **Step 3: Visually verify scoreboard.gif**

Check: title card, 4 stat cards sliding in with increasing glow intensity, winner highlight with zoom bounce. Colors match the data.

- [ ] **Step 4: Commit**

```bash
git add playground/main.mac
git commit -m "feat: add SCOREBOARD animation - data visualization stat reveal"
```

---

### Task 5: Final verification and push

**Files:**
- Verify: `playground/main.mac`

- [ ] **Step 1: Run the full file and check timing**

Run: `time ./build/mac playground/main.mac`
Expected: all 5 GIFs saved, total CPU time under 45 seconds

- [ ] **Step 2: Run test suite to ensure no regressions**

Run: `bash tests/run_tests.sh`
Expected: all tests pass

- [ ] **Step 3: Push**

```bash
git push origin main
```
