# Contributing to Mac

## Setup

See [docs/BUILDING.md](docs/BUILDING.md) for build prerequisites and instructions.

```bash
git clone https://github.com/jona62/mac.git
cd mac
cmake -S . -B build && cmake --build build
bash tests/run_tests.sh  # should pass all 57 tests
```

## Project Layout

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the full architecture overview.

- `src/` — C++ source files (entry point in `main.cpp`)
- `include/` — C++ headers (most implementation lives here due to templates)
- `stdlib/prelude.mac` — Standard library loaded before user code
- `tests/` — 57 tests across 20 categories
- `examples/` — 9 progressive examples (01-07 + 02b, 03b)
- `mac-lang/` — VS Code extension + LSP (thin adapter over `mac --analyze`)
- `webapp/` — Web GIF studio (Python)
- `.claude/skills/` — Claude Code agent skill for Mac language

## Writing Tests

Tests are `.mac` files with `// expect:` annotations:

```mac
print 2 + 2;              // expect: 4
print "hi" |> upper;       // expect: HI
print type(@blank "x");    // expect: instance
```

Place tests in `tests/<category>/`. Categories include: arrays, classes, control_flow, effects, expressions, extensions, functional, functions, lambdas, layout, maps, memes, operators, pipes, scoping, statements, stdlib, syntax, timeline.

Run the full suite: `bash tests/run_tests.sh`

## Adding a Native Function

1. Define in `include/NativeRegistry.h`:
   ```cpp
   {"my_func", NativeVisibility::Public, "fun(1)", "Description.",
       {{{"arg1"}, "ReturnType", "Overload description."}},
       [] { return std::make_shared<callable::MyFunction>(); }},
   ```

2. Implement the class in `include/NativeFunctions.h` extending `MacCallable`.

3. Write a test in `tests/` to verify.

The analyzer and LSP pick up the function automatically from `NativeRegistry.h`.

## Adding an Effect

1. Implement the pixel transform in `include/MemeEffects.h`:
   ```cpp
   inline void myEffect(unsigned char* pixels, int w, int h, float param) {
       // process RGBA pixels in-place
   }
   ```

2. Add dispatch in `PartialEffect::call()` or `DirectEffect::call()` in `NativeFunctions.h`.

3. Register in `NativeRegistry.h`:
   ```cpp
   {"myEffect", NativeVisibility::Public, "Meme -> Meme", "Description.",
       {{{"param"}, "Meme -> Meme", "..."}},
       [] { return std::make_shared<callable::ParamEffectCreator>("myEffect"); }},
   ```

## Adding a Template

1. Add the generator function in `tools/gen_templates.cpp`.
2. Register the name→path mapping in `MacMeme.h`'s `templateMap()`.
3. Run `cd tools && ./gen_templates` to regenerate images.

## Pull Request Workflow

1. Create a branch from `main`
2. Make changes
3. `bash tests/run_tests.sh` — all 57 tests must pass
4. `cd mac-lang && npx tsc --noEmit` — LSP must typecheck
5. Commit and open a PR against `main`

CI runs tests on Ubuntu + macOS, plus LSP typechecking.
