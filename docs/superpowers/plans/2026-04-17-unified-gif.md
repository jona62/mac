# Unified GIF Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Merge `timeline` into `gif` so that the `gif { }` block supports transitions, and remove the `timeline` keyword, `TimelineBlockExpr`, and prelude `Timeline`/`Frame` classes.

**Architecture:** Extend `GifBlockExpr` AST node to carry optional transition data (currently only in `TimelineBlockExpr`). Merge timeline parsing into the gif parser. In the interpreter, `visitGifBlockExpr` uses `MacGif` when no transitions are present and `MacTimeline` when they are. Remove all timeline-specific code paths.

**Tech Stack:** C++23, Mac language (custom interpreter), Python (webapp backend), JavaScript (webapp frontend)

---

## File Structure

| File | Responsibility | Change |
|------|---------------|--------|
| `include/Expr.h` | AST node definitions | Extend `GifBlockExpr` with transitions, remove `TimelineBlockExpr` |
| `src/Parser.cpp` | Parsing | Merge timeline parsing into `gifBlock()`, remove `timelineBlock()`, remove `timeline` from contextual check |
| `include/Interpreter.h` | Evaluation | Merge `visitTimelineBlockExpr` into `visitGifBlockExpr`, remove timeline visitor |
| `include/Resolver.h` | Variable resolution | Remove `visitTimelineBlockExpr` |
| `include/NativeFunctions.h` | Native function classes | Remove 6 timeline native classes |
| `include/NativeRegistry.h` | Native function registry | Remove 6 timeline registrations |
| `stdlib/prelude.mac` | Standard library | Remove `Frame`, `Timeline` classes; remove transition constants; update `Gif` class |
| `include/AnalyzerWalk.h` | Analyzer AST walk | Remove timeline handling, update gif handling |
| `include/AnalyzerInference.h` | Type inference | Remove timeline type case |
| `tests/` | Test suite | Rewrite timeline tests as gif tests |
| `tests/analyzer/` | Analyzer tests | Update snapshots |
| `docs/reference/` | Language reference | Remove timeline page, update gif page |
| `webapp/server.py` | Backend | Change `_animation_block` to always use `gif` |
| `webapp/static/js/highlight.js` | Syntax highlighting | Remove `timeline` from keywords |

---

### Task 1: Extend GifBlockExpr and remove TimelineBlockExpr

**Files:**
- Modify: `include/Expr.h`

- [ ] **Step 1: Read current Expr.h to confirm line numbers**

Run: `head -420 include/Expr.h | tail -100`

- [ ] **Step 2: Replace GifBlockExpr with extended version and remove TimelineBlockExpr**

In `include/Expr.h`, replace the `GifBlockExpr` class (lines 336-348) with:

```cpp
template <typename T>
class GifBlockExpr : public Expr<T> {
public:
    struct Transition { std::string type; double durationMs; std::string easing; };
    struct Entry {
        shared_ptr<Expr<T>> meme;
        double durationMs;
        std::shared_ptr<Transition> transition; // transition to NEXT frame, nullptr if none
    };
    GifBlockExpr(Token keyword, bool loop, std::vector<Entry> entries, Token loopToken = Token())
        : keyword(keyword), loop(loop), entries(std::move(entries)), loopToken(loopToken) {}
    T visit(shared_ptr<Visitor<T>> visitor) override {
        return visitor->visitGifBlockExpr(this);
    }
    Token keyword;
    bool loop;
    std::vector<Entry> entries;
    Token loopToken;
};
```

Remove the entire `TimelineBlockExpr` class (lines 351-366).

Remove `visitTimelineBlockExpr` from the `Visitor` abstract class (line 407):
```cpp
// DELETE THIS LINE:
virtual T visitTimelineBlockExpr(TimelineBlockExpr<T>* expr) = 0;
```

- [ ] **Step 3: Build to see what breaks**

Run: `cmake -S . -B build 2>&1 | tail -1 && cmake --build build 2>&1 | head -30`

Expected: Compilation errors in Parser.cpp, Interpreter.h, Resolver.h, AnalyzerWalk.h — all referencing the removed `TimelineBlockExpr` and `visitTimelineBlockExpr`. This confirms all call sites.

- [ ] **Step 4: Commit AST change**

```bash
git add include/Expr.h
git commit -m "refactor: extend GifBlockExpr with transitions, remove TimelineBlockExpr"
```

---

### Task 2: Merge timeline parsing into gif parser

**Files:**
- Modify: `src/Parser.cpp`
- Modify: `include/Parser.h`

- [ ] **Step 1: Remove `timeline` from contextual keyword check**

In `src/Parser.cpp` (line 401), change:
```cpp
if (s && (*s == "gif" || *s == "timeline" || *s == "grid")) {
```
to:
```cpp
if (s && (*s == "gif" || *s == "grid")) {
```

Remove the `if (*s == "timeline") return timelineBlock<T>();` line (around line 411).

- [ ] **Step 2: Rewrite gifBlock() to support transitions**

Replace the `gifBlock()` method (lines 834-856) with:

```cpp
template <typename T>
shared_ptr<Expr<T>> Parser::gifBlock() {
    Token keyword = previous();
    bool loop = false;
    Token loopToken;

    if (peek().type == TokenType::IDENTIFIER) {
        auto* s = std::get_if<std::string>(&peek().lexeme);
        if (s && *s == "loop") { advance(); loop = true; loopToken = previous(); }
    }

    consume(TokenType::LEFT_BRACE, "Expected '{' after gif.");

    std::vector<typename expr::GifBlockExpr<T>::Entry> entries;
    while (peek().type != TokenType::RIGHT_BRACE && !isAtEnd()) {
        // Check for transition: --- type duration [easing] ---
        if (match(TokenType::TRIPLE_DASH)) {
            consume(TokenType::IDENTIFIER, "Expected transition type after '---'.");
            std::string transType = std::get<std::string>(previous().lexeme);
            double transMs = parseDuration();
            std::string easing = "linear";
            if (peek().type == TokenType::IDENTIFIER) {
                auto* s = std::get_if<std::string>(&peek().lexeme);
                if (s && (*s == "ease" || *s == "easeIn" || *s == "easeOut"
                          || *s == "easeInOut" || *s == "bounce" || *s == "linear")) {
                    advance();
                    easing = *s;
                }
            }
            consume(TokenType::TRIPLE_DASH, "Expected '---' after transition.");
            if (!entries.empty()) {
                auto trans = std::make_shared<typename expr::GifBlockExpr<T>::Transition>();
                trans->type = transType;
                trans->durationMs = transMs;
                trans->easing = easing;
                entries.back().transition = trans;
            }
            continue;
        }

        auto meme = expression<T>();
        consume(TokenType::COLON, "Expected ':' after meme in gif frame.");
        double ms = parseDuration();
        entries.push_back({meme, ms, nullptr});
    }
    consume(TokenType::RIGHT_BRACE, "Expected '}' after gif block.");

    return make_shared<expr::GifBlockExpr<T>>(keyword, loop, std::move(entries), loopToken);
}
```

- [ ] **Step 3: Remove timelineBlock() method entirely**

Delete the `timelineBlock()` method (lines 861-909).

Remove its declaration from `include/Parser.h` — find and delete:
```cpp
template <typename T> shared_ptr<expr::Expr<T>> timelineBlock();
```

- [ ] **Step 4: Build to verify parser compiles**

Run: `cmake --build build 2>&1 | head -20`

Expected: Still errors from Interpreter.h and Resolver.h (next tasks), but parser should compile.

- [ ] **Step 5: Commit parser change**

```bash
git add src/Parser.cpp include/Parser.h
git commit -m "refactor: merge timeline parsing into gifBlock(), remove timelineBlock()"
```

---

### Task 3: Merge interpreter visitors

**Files:**
- Modify: `include/Interpreter.h`

- [ ] **Step 1: Rewrite visitGifBlockExpr to handle transitions**

Replace `visitGifBlockExpr` (lines 715-741) with:

```cpp
MacValue visitGifBlockExpr(expr::GifBlockExpr<MacValue>* expr) override {
    // Check if any entry has a transition
    bool hasTransitions = false;
    for (auto& entry : expr->entries) {
        if (entry.transition) { hasTransitions = true; break; }
    }

    if (hasTransitions) {
        // Use MacTimeline engine for interpolated transitions
        auto rawTl = std::make_shared<meme::MacTimeline>();
        for (auto& entry : expr->entries) {
            auto meme = evaluate(entry.meme);
            int holdMs = static_cast<int>(entry.durationMs);
            rawTl->addKeyframe(callable::getRenderSurface(meme));
            rawTl->addHold(holdMs);
            if (entry.transition) {
                std::string easing = entry.transition->easing.empty()
                    ? "linear" : entry.transition->easing;
                rawTl->setTransition(static_cast<int>(entry.transition->durationMs),
                                     entry.transition->type, easing);
            }
        }
        if (expr->loop) rawTl->setLoop(0);

        // Wrap in Gif prelude instance with _tl for timeline rendering
        auto gifClass = env->get(token::Token(token::TokenType::IDENTIFIER,
            token::TokenValue(std::string("Gif")), 0));
        auto gifFn = std::get<shared_ptr<callable::MacCallable>>(gifClass);
        auto gif = gifFn->call(shared_from_this(), {});
        auto inst = std::get<shared_ptr<instance::MacInstance>>(gif);
        token::Token tlTok(token::TokenType::IDENTIFIER,
            token::TokenValue(std::string("_tl")), 0);
        inst->set(tlTok, MacValue(rawTl));
        return gif;
    }

    // No transitions — use simple MacGif engine
    auto rawGif = std::make_shared<meme::MacGif>();
    for (auto& entry : expr->entries) {
        auto meme = evaluate(entry.meme);
        int durationMs = static_cast<int>(entry.durationMs);
        rawGif->addFrame(callable::getRenderSurface(meme), durationMs);
    }
    if (expr->loop) rawGif->setLoop(0);

    auto gifClass = env->get(token::Token(token::TokenType::IDENTIFIER,
        token::TokenValue(std::string("Gif")), 0));
    auto gifFn = std::get<shared_ptr<callable::MacCallable>>(gifClass);
    auto gif = gifFn->call(shared_from_this(), {});
    auto inst = std::get<shared_ptr<instance::MacInstance>>(gif);
    token::Token gifTok(token::TokenType::IDENTIFIER,
        token::TokenValue(std::string("_gif")), 0);
    inst->set(gifTok, MacValue(rawGif));
    token::Token fcTok(token::TokenType::IDENTIFIER,
        token::TokenValue(std::string("frameCount")), 0);
    inst->set(fcTok, MacValue(static_cast<double>(expr->entries.size())));
    return gif;
}
```

- [ ] **Step 2: Remove visitTimelineBlockExpr entirely**

Delete `visitTimelineBlockExpr` (lines 743-776).

- [ ] **Step 3: Commit interpreter change**

```bash
git add include/Interpreter.h
git commit -m "refactor: visitGifBlockExpr handles transitions, remove visitTimelineBlockExpr"
```

---

### Task 4: Update Resolver

**Files:**
- Modify: `include/Resolver.h`

- [ ] **Step 1: Find and remove visitTimelineBlockExpr from Resolver**

Search for `visitTimelineBlockExpr` in `include/Resolver.h`. It should be a visitor method that walks the timeline's sub-expressions. Remove it.

Update `visitGifBlockExpr` to walk the new `entries` structure instead of `frames`:

```cpp
MacValue visitGifBlockExpr(expr::GifBlockExpr<MacValue>* expr) override {
    for (auto& entry : expr->entries) {
        resolve(entry.meme);
    }
    return {};
}
```

- [ ] **Step 2: Build**

Run: `cmake --build build 2>&1 | head -20`

- [ ] **Step 3: Commit**

```bash
git add include/Resolver.h
git commit -m "refactor: update Resolver for unified GifBlockExpr, remove timeline visitor"
```

---

### Task 5: Remove timeline native functions

**Files:**
- Modify: `include/NativeFunctions.h`
- Modify: `include/NativeRegistry.h`

- [ ] **Step 1: Remove timeline native classes from NativeFunctions.h (keep TimelineRenderFunction)**

Delete these classes (lines 1103-1195), but **keep `TimelineRenderFunction`** — it's needed by `Gif.save()` when the gif has transitions:
- `TimelineCreateFunction` — delete
- `TimelineKeyframeFunction` — delete
- `TimelineTransitionFunction` — delete
- `TimelineHoldFunction` — delete
- `TimelineLoopFunction` — delete
- `TimelineRenderFunction` — **keep**

- [ ] **Step 2: Remove timeline registrations from NativeRegistry.h (keep _timeline_render)**

Delete these entries from the `all()` function, but **keep `_timeline_render`**:
- `"timeline"` (TimelineCreateFunction) — delete
- `"_timeline_keyframe"` (TimelineKeyframeFunction) — delete
- `"_timeline_transition"` (TimelineTransitionFunction) — delete
- `"_timeline_hold"` (TimelineHoldFunction) — delete
- `"_timeline_loop"` (TimelineLoopFunction) — delete
- `"_timeline_render"` (TimelineRenderFunction) — **keep** (used by Gif.save for transition rendering)

- [ ] **Step 3: Update getRenderSurface() error messages**

In `include/NativeFunctions.h`, find the `getRenderSurface()` function. Remove the Timeline check:

```cpp
// DELETE:
if (std::holds_alternative<std::shared_ptr<meme::MacTimeline>>(val)) {
    throw std::runtime_error("Cannot use Timeline as a frame — Timeline is a sequence type...");
}
```

- [ ] **Step 4: Keep SaveFunction MacTimeline handling**

In `include/NativeFunctions.h`, the `SaveFunction::call()` method handles `MacTimeline` directly. **Keep this** — it's still needed as the internal engine. No changes required here.

- [ ] **Step 5: Build**

Run: `cmake --build build 2>&1 | head -20`

- [ ] **Step 6: Commit**

```bash
git add include/NativeFunctions.h include/NativeRegistry.h
git commit -m "refactor: remove timeline native functions and registry entries"
```

---

### Task 6: Update prelude

**Files:**
- Modify: `stdlib/prelude.mac`

- [ ] **Step 1: Remove Frame class**

Delete the `Frame` class (lines 70-75):
```mac
class Frame {
    init(memeObj, duration) {
        this.meme = memeObj;
        this.duration = duration;
    }
}
```

- [ ] **Step 2: Remove Timeline class**

Delete the entire `Timeline` class (lines 192-235).

- [ ] **Step 3: Remove transition constants**

Delete these lines (178-186):
```mac
var crossfade = "crossfade";
var slideLeft = "slideLeft";
var slideRight = "slideRight";
var slideUp = "slideUp";
var slideDown = "slideDown";
var wipe = "wipe";
var fadeBlack = "fadeBlack";
var zoom = "zoom";
```

- [ ] **Step 4: Update Gif class to handle _tl**

The `Gif.save()` method currently checks for `this._gif`. It needs to also check for `this._tl` (set by the interpreter when the gif block has transitions):

```mac
class Gif {
    init() {
        this._gif = nil;
        this._tl = nil;
        this.frameCount = 0;
    }

    frame(memeObj, duration) {
        if (this._gif == nil) {
            this._gif = _gif_create();
        }
        _gif_add_frame(this._gif, memeObj, duration.ms);
        this.frameCount = this.frameCount + 1;
        return this;
    }

    save(path) {
        if (this._tl != nil) {
            _timeline_render(this._tl, path);
        } else if (this._gif != nil) {
            _gif_save_raw(this._gif, path);
        } else {
            print "No frames to save.";
        }
    }

    __add__(other) {
        this.frame(other.meme, other.duration);
        return this;
    }
}
```

Wait — we removed `_timeline_render` in Task 5. The Gif class needs a way to save when it has a `_tl`. We need to keep `_timeline_render` as an internal native, or have the interpreter handle it differently.

**Resolution:** Keep `_timeline_render` as the one surviving timeline native (it's internal, user never calls it). Go back to Task 5 and keep `TimelineRenderFunction` and its registry entry.

- [ ] **Step 5: Remove Meme.__add__ Frame operator**

In the `Meme` class, remove the `__add__` method that creates Frames (since Frame class is gone):

```mac
// DELETE from Meme class:
__add__(other) {
    return Frame(this, other);
}
```

- [ ] **Step 6: Build and test prelude loads**

Run: `cmake --build build 2>&1 | tail -3 && echo 'print "prelude ok";' | ./build/mac`

Expected output: `prelude ok`

- [ ] **Step 7: Commit**

```bash
git add stdlib/prelude.mac
git commit -m "refactor: remove Frame/Timeline classes, update Gif to handle transitions"
```

---

### Task 7: Update analyzer

**Files:**
- Modify: `include/AnalyzerWalk.h`
- Modify: `include/AnalyzerInference.h`

- [ ] **Step 1: Update AnalyzerWalk.h**

Find `TimelineBlockExpr` handling in `analyzeExpr()`. Remove it. Update `GifBlockExpr` handling to walk the new `entries` structure:

```cpp
} else if (auto* p = dynamic_cast<expr::GifBlockExpr<MV>*>(e)) {
    for (auto& entry : p->entries) {
        analyzeExpr(entry.meme.get());
        checkFrameType(entry.meme.get(), "gif");
    }
    // Emit semantic tokens for the gif keyword
    if (p->keyword.line > 0 && currentSource == "user") {
        result.semanticTokens.push_back({
            p->keyword.line, safeCol(p->keyword),
            static_cast<int>(tokName(p->keyword).size()),
            "keyword", currentSource});
    }
    if (p->loop && p->loopToken.line > 0 && currentSource == "user") {
        result.semanticTokens.push_back({
            p->loopToken.line, safeCol(p->loopToken),
            4, "keyword", currentSource});
    }
}
```

- [ ] **Step 2: Update AnalyzerInference.h**

Find where `TimelineBlockExpr` returns `"Timeline"` type. Change the `GifBlockExpr` case to always return `"Gif"`. Remove the timeline case.

- [ ] **Step 3: Build**

Run: `cmake --build build 2>&1 | tail -5`

- [ ] **Step 4: Commit**

```bash
git add include/AnalyzerWalk.h include/AnalyzerInference.h
git commit -m "refactor: update analyzer for unified GifBlockExpr, remove timeline handling"
```

---

### Task 8: Rewrite tests

**Files:**
- Modify: `tests/timeline/basic.mac`
- Modify: `tests/syntax/transitions.mac`
- Modify: `tests/syntax/sequence_errors_tl.mac`
- Modify: `tests/syntax/composition.mac`
- Modify: `tests/syntax/easing_curves.mac`

- [ ] **Step 1: Rewrite tests/timeline/basic.mac**

Rename to test gif with transitions (or rewrite in place):

```mac
// Gif with transitions (unified — replaces timeline)
var g = gif {
    @blank "frame 1" : 500ms
    --- crossfade 300ms ---
    @blank "frame 2" : 500ms
};
print type(g);              // expect: instance

// Simple gif without transitions still works
var g2 = gif {
    @blank "A" : 400ms
    @blank "B" : 400ms
};
print type(g2);             // expect: instance

print "gif transitions ok"; // expect: gif transitions ok
```

- [ ] **Step 2: Rewrite tests/syntax/transitions.mac**

```mac
// Transition syntax inside gif blocks
var g = gif {
    @blank "A" : 1s
    --- crossfade 200ms ease ---
    @blank "B" : 1s
    --- fadeBlack 300ms easeOut ---
    @blank "C" : 1s
};
print type(g);              // expect: instance

// Mixed — some frames with transitions, some without
var g2 = gif {
    @blank "X" : 500ms
    @blank "Y" : 500ms
    --- slideLeft 200ms ---
    @blank "Z" : 500ms
};
print type(g2);             // expect: instance

print "transitions ok";     // expect: transitions ok
```

- [ ] **Step 3: Rewrite tests/syntax/sequence_errors_tl.mac**

Update to use gif instead of timeline:

```mac
// Gif with transitions inside grid should error
var g = gif loop {
    @blank "A" : 500ms
    --- crossfade 150ms ---
    @blank "B" : 500ms
};

grid 2x1 { g @blank "ok" } => "fail.png";
// expect runtime error: Cannot use Gif as a frame
```

- [ ] **Step 4: Update tests/syntax/composition.mac**

Replace `timeline` with `gif`:

```mac
// Grid inside gif with transitions
var g = gif loop {
    grid 1x2 {
        @blank "Left"
        @blank "Right"
    } : 500ms
    --- crossfade 150ms ---
    @blank "Full" : 500ms
};
print type(g);              // expect: instance
```

- [ ] **Step 5: Update tests/syntax/easing_curves.mac**

Replace `timeline` with `gif` in any easing curve tests.

- [ ] **Step 6: Remove tests referencing Timeline/Frame prelude classes**

Check `tests/syntax/sequence_errors.mac` and any other test that uses `Timeline()`, `Frame()`, or timeline variables. Update or remove as needed.

- [ ] **Step 7: Run all tests**

Run: `bash tests/run_tests.sh`

Expected: All tests pass (the exact count may change if tests were removed).

- [ ] **Step 8: Commit**

```bash
git add tests/
git commit -m "test: rewrite timeline tests as unified gif tests"
```

---

### Task 9: Update analyzer tests and snapshots

**Files:**
- Modify: `tests/analyzer/run_analyzer_tests.py`
- Modify: `tests/analyzer/snapshots/`

- [ ] **Step 1: Update any analyzer test that references Timeline**

Search for "Timeline" or "timeline" in `tests/analyzer/run_analyzer_tests.py`. Update assertions — there should be no Timeline class in the output, no timeline type inference.

- [ ] **Step 2: Regenerate snapshots**

Run: `python3 tests/analyzer/run_analyzer_tests.py --update`

- [ ] **Step 3: Verify**

Run: `python3 tests/analyzer/run_analyzer_tests.py`

Expected: All 41 tests pass.

- [ ] **Step 4: Commit**

```bash
git add tests/analyzer/
git commit -m "test: update analyzer tests for unified gif"
```

---

### Task 10: Update webapp

**Files:**
- Modify: `webapp/server.py`
- Modify: `webapp/static/js/highlight.js`

- [ ] **Step 1: Update _animation_block in server.py**

In `webapp/server.py`, find the `_animation_block` function (around line 743). Change it to always use `gif` instead of conditionally choosing `timeline`:

```python
def _animation_block(scenes: list[dict], save_target: str) -> list[str]:
    """Generate gif loop { } lines with optional transitions."""
    lines: list[str] = []
    lines.append("gif loop {")
    for i, scene in enumerate(scenes):
        trans = scene.get("transition") if i > 0 else None
        if trans and trans.get("type") and trans["type"] != "cut":
            t_type = trans["type"]
            t_dur = int(trans.get("durationMs", 150))
            t_ease = trans.get("easing", "linear")
            ease_part = f" {t_ease}" if t_ease != "linear" else ""
            lines.append(f"    --- {t_type} {t_dur}ms{ease_part} ---")
        lines.append(f"    scene_{i + 1} : {scene['durationMs']}ms")
    # Loop-back transition
    if len(scenes) > 1:
        first_trans = next(
            (s.get("transition") for s in scenes[1:]
             if s.get("transition") and s["transition"].get("type") not in (None, "cut")),
            None,
        )
        if first_trans:
            t_type = first_trans["type"]
            t_dur = int(first_trans.get("durationMs", 150))
            t_ease = first_trans.get("easing", "linear")
            ease_part = f" {t_ease}" if t_ease != "linear" else ""
            lines.append(f"    --- {t_type} {t_dur}ms{ease_part} ---")
    lines.append(f"}} => {save_target};")
    return lines
```

- [ ] **Step 2: Search for any other "timeline" references in server.py**

Run: `grep -n timeline webapp/server.py`

Update any remaining references.

- [ ] **Step 3: Update highlight.js**

In `webapp/static/js/highlight.js`, remove `timeline` from the keywords set:

Change:
```javascript
const HL_KEYWORDS = new Set("var,fun,effect,style,class,for,while,if,else,return,print,in,break,continue,and,or,gif,timeline,grid,loop".split(","));
```
To:
```javascript
const HL_KEYWORDS = new Set("var,fun,effect,style,class,for,while,if,else,return,print,in,break,continue,and,or,gif,grid,loop".split(","));
```

- [ ] **Step 4: Search all JS files for timeline references**

Run: `grep -rn timeline webapp/static/js/`

Update any remaining references (document.js, render.js, controls.js, state.js).

- [ ] **Step 5: Commit**

```bash
git add webapp/
git commit -m "refactor: webapp uses gif for all animations, remove timeline references"
```

---

### Task 11: Update docs

**Files:**
- Modify: `docs/reference/src/SUMMARY.md`
- Delete: `docs/reference/src/meme/timeline.md`
- Modify: `docs/reference/src/meme/gif.md`
- Modify: `docs/reference/src/introduction.md`
- Modify: `docs/reference/src/stdlib/prelude_classes.md`
- Modify: `docs/reference/src/stdlib/native_functions.md`
- Modify: `docs/reference/src/language/types.md`

- [ ] **Step 1: Remove timeline from SUMMARY.md**

Delete the `- [Timeline](./meme/timeline.md)` line.

- [ ] **Step 2: Delete timeline.md**

Run: `rm docs/reference/src/meme/timeline.md`

- [ ] **Step 3: Update gif.md with transition syntax**

Add the `---` transition syntax, easing curves, and loop-back behavior to the gif page. This page now covers everything that was on the timeline page.

- [ ] **Step 4: Update prelude_classes.md**

Remove `Timeline` and `Frame` class documentation. Update `Gif` class to show `_tl` handling.

- [ ] **Step 5: Update other pages**

Remove "Timeline" from types.md, introduction.md, native_functions.md. Change any mention of "gif or timeline" to just "gif".

- [ ] **Step 6: Build docs**

Run: `cd docs/reference && mdbook build 2>&1`

- [ ] **Step 7: Commit**

```bash
git add docs/reference/
git commit -m "docs: update reference for unified gif, remove timeline page"
```

---

### Task 12: Final verification and version bump

**Files:**
- Modify: `VERSION`

- [ ] **Step 1: Run full test suite**

```bash
bash tests/run_tests.sh
python3 tests/analyzer/run_analyzer_tests.py
```

Expected: All tests pass.

- [ ] **Step 2: Verify timeline is fully gone**

```bash
grep -rn "timeline" include/ src/ stdlib/ --include="*.h" --include="*.cpp" --include="*.mac" | grep -v "MacTimeline" | grep -v "//"
```

Expected: No hits (MacTimeline is the internal C++ class, which stays).

- [ ] **Step 3: Test gif with transitions end-to-end**

```bash
echo 'gif { @blank "A" : 500ms --- crossfade 200ms --- @blank "B" : 500ms } => "unified_test.gif";' | ./build/mac
```

Expected: `Saved /path/to/mac/output/unified_test.gif`

- [ ] **Step 4: Test simple gif still works**

```bash
echo 'gif { @blank "X" : 300ms @blank "Y" : 300ms } => "simple_test.gif";' | ./build/mac
```

Expected: `Saved /path/to/mac/output/simple_test.gif`

- [ ] **Step 5: Test timeline keyword produces error**

```bash
echo 'timeline { @blank "A" : 500ms };' | ./build/mac 2>&1
```

Expected: Parse error (timeline is no longer recognized as a block keyword).

- [ ] **Step 6: Bump version**

This is a breaking change. Update `VERSION` to `0.3.0`.

- [ ] **Step 7: Commit and tag**

```bash
git add VERSION
git commit -m "feat!: unified gif — merge timeline into gif, memes as single building block

BREAKING CHANGE: timeline keyword removed. Use gif { } with --- transitions instead.
Frame and Timeline prelude classes removed."
```
