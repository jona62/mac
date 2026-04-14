# Building Mac

## Prerequisites

- C++23 compiler (Clang 16+, GCC 13+)
- CMake 3.20+
- Python 3 (web GIF studio, optional)
- Node.js 22+ (VS Code extension, optional)

## Build

```bash
cmake -S . -B build
cmake --build build
```

The post-build step copies `assets/` and `stdlib/` to the build directory on every build.

## Run

```bash
./build/mac                        # Interactive REPL (|> prompt)
./build/mac script.mac             # Execute a file
./build/mac examples/01_hello.mac  # Run an example
```

Output files are written to `~/mac/output/` regardless of working directory.

The binary resolves `stdlib/prelude.mac` and `assets/` relative to its own location, so it works from any directory after installation.

## Tests

```bash
bash tests/run_tests.sh
```

57 tests across 20 categories. Tests use `// expect:` annotations:

```mac
print 2 + 2;                    // expect: 4
print "hello" |> upper;         // expect: HELLO
print type(@blank "hi");        // expect: instance
```

Test categories: arrays, classes, control_flow, effects, expressions, extensions, functional, functions, lambdas, layout, maps, memes, operators, pipes, scoping, statements, stdlib, syntax, timeline.

## Examples

9 progressive examples from basic to advanced:

```
examples/
├── 01_hello.mac          # Language basics
├── 02_first_meme.mac     # @template, =>, positions
├── 02b_classic_api.mac   # Builder pattern (Meme, Gif, Timeline classes)
├── 03_effects.mac        # Pipes, compose, effect presets
├── 03b_styles.mac        # Text color, outline, shadow
├── 04_animation.mac      # gif/timeline blocks, transitions, easing
├── 05_functional.mac     # Arrows, map/filter/reduce, zip
├── 06_layout.mac         # beside, stack, grid blocks
└── 07_showcase.mac       # Everything combined
```

## VS Code Extension

```bash
cd mac-lang && npm install && npx tsc && cd ..
ln -sf "$(pwd)/mac-lang" ~/.vscode/extensions/mac-lang
```

Reload VS Code. Features: syntax highlighting, hover, go-to-definition, inlay type hints, semantic tokens, signature help, find references, document symbols, folding ranges, completion.

The LSP runs `mac --analyze` as a backend — no language logic in TypeScript.

## Web GIF Studio

```bash
PORT=9001 ./webapp/run.sh
```

Opens at `http://localhost:9001`. 10 templates, 12 effect presets, style customization, live Mac v2 script preview.

## Pre-built Binaries

```bash
curl -fsSL https://raw.githubusercontent.com/jona62/mac/main/install.sh | bash
```

Installs to `~/.mac/` with symlink at `~/.local/bin/mac`. Includes binary, assets, and stdlib.

Supported platforms: macOS arm64, macOS x86_64, Linux x86_64.

## Release

Tag a version to trigger CI release:

```bash
git tag v0.0.2
git push origin v0.0.2
```

CI builds binaries for 3 platforms, runs all tests, and publishes a GitHub release.
