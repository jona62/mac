# Contributing to Mac

## Setup

```bash
git clone https://github.com/jona62/mac.git
cd mac
cmake -S . -B build && cmake --build build
bash tests/run_tests.sh                        # 69 runtime tests
python3 tests/analyzer/run_analyzer_tests.py   # 41 analyzer tests
```

Requires CMake 3.20+ and a C++23 compiler. See the [language reference](docs/reference/) for full documentation (`cd docs/reference && mdbook serve`).

## Project Layout

- `src/` — C++ source files (entry point in `main.cpp`)
- `include/` — C++ headers (most implementation lives here due to templates)
- `stdlib/prelude.mac` — Standard library loaded before user code
- `tests/` — Runtime tests (`run_tests.sh`) and analyzer tests (`analyzer/`)
- `mac-lang/` — VS Code extension + LSP (thin adapter over `mac --analyze`)
- `webapp/` — Web GIF studio (Python)
- `docs/reference/` — mdBook language reference

## Writing Tests

### Runtime Tests

Tests are `.mac` files with `// expect:` annotations:

```mac
print 2 + 2;              // expect: 4
print "hi" |> upper;       // expect: HI
print type(@blank "x");    // expect: instance
```

Place tests in `tests/<category>/`.

Run: `bash tests/run_tests.sh`

### Analyzer Tests

Python tests that verify `--analyze` JSON output (symbols, references, diagnostics, type inference, etc.). Includes snapshot regression tests.

Run: `python3 tests/analyzer/run_analyzer_tests.py`

Update snapshots after intentional changes: `python3 tests/analyzer/run_analyzer_tests.py --update`

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
3. `bash tests/run_tests.sh` — all runtime tests must pass
4. `python3 tests/analyzer/run_analyzer_tests.py` — all analyzer tests must pass
5. `cd mac-lang && npx tsc --noEmit` — LSP must typecheck
6. Commit and open a PR against `main`

CI runs runtime + analyzer tests on Ubuntu and macOS, plus LSP typechecking.
