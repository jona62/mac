# Contributing to Mac

## Setup

See [docs/BUILDING.md](docs/BUILDING.md) for build prerequisites and instructions.

```bash
git clone https://github.com/jona62/mac.git
cd mac
cmake -S . -B build && cmake --build build
bash tests/run_tests.sh  # should pass all 46 tests
```

## Project Layout

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for the full architecture overview.

- `src/` — C++ source files (entry point in `main.cpp`)
- `include/` — C++ headers (most implementation lives here due to templates)
- `stdlib/prelude.mac` — Standard library loaded before user code
- `tests/` — Test suite organized by category
- `mac-lang/` — VS Code extension and LSP server (TypeScript)
- `webapp/` — Web GIF studio (Python)

## Writing Tests

Tests are `.mac` files with `// expect:` annotations on output lines:

```mac
print 2 + 2;           // expect: 4
print "hi" |> upper;   // expect: HI
```

Place tests in the appropriate `tests/<category>/` subdirectory. The test harness (`tests/run_tests.sh`) runs each file and compares stdout against expected output.

Run the full suite: `bash tests/run_tests.sh`

## Adding a Native Function

1. Define a class in `include/NativeFunctions.h` extending `MacCallable`:
   ```cpp
   class MyFunction : public MacCallable {
   public:
       value::MacValue call(std::shared_ptr<interpreter::Interpreter> interp,
                            std::vector<value::MacValue> args) override {
           // implementation
       }
       int arity() override { return 1; }  // use -1 for variable arity
       std::string toString() override { return "<native fn my_func>"; }
   };
   ```

2. Register it in the `Interpreter` constructor in `include/Interpreter.h`:
   ```cpp
   defn("my_func", make_shared<callable::MyFunction>());
   ```

3. Add hover documentation in `mac-lang/src/analyzer.ts` (the `NATIVE_FUNCTIONS` array).

4. Write a test in `tests/` to verify the function works.

## Adding an Effect

Effects use two patterns in `include/NativeFunctions.h`:

- **Parameterized** (e.g., `blur(5)`): use `ParamEffectCreator`
- **Direct** (e.g., `sepia`): use `DirectEffect`

Register in `include/Interpreter.h`:
```cpp
defn("my_effect", make_shared<callable::ParamEffectCreator>("my_effect"));
// or
defn("my_effect", make_shared<callable::DirectEffect>("my_effect"));
```

Then implement the pixel transform in `include/MemeEffects.h`.

## Pull Request Workflow

1. Create a branch from `main`
2. Make your changes
3. Run `bash tests/run_tests.sh` — all tests must pass
4. Run `cd mac-lang && npx tsc --noEmit` — LSP must typecheck (if you touched TypeScript)
5. Commit with a descriptive message
6. Open a PR against `main`

CI runs tests on Ubuntu and macOS, plus LSP typechecking.

## Code Style

- Follow existing patterns — the codebase is consistent
- Headers are template-heavy and contain most implementation; this is deliberate
- Native functions follow the `MacCallable` class pattern
- Tests use the `// expect:` annotation convention
