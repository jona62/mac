# Building Mac

## Prerequisites

- C++23 compiler (Clang 16+, GCC 13+, MSVC 19.36+)
- CMake 3.20+
- Python 3 (for the web GIF studio, optional)
- Node.js 20+ (for the VS Code extension, optional)

## Build

```bash
cmake -S . -B build
cmake --build build
```

The build copies `assets/` and `stdlib/` into the build directory so the binary can find templates and the standard library at runtime.

## Run

```bash
./build/mac                    # Interactive REPL (prompt: |>)
./build/mac script.mac         # Execute a .mac file
./build/mac examples/hello.mac # Run an example
```

The REPL loads `stdlib/prelude.mac` on startup, then accepts one-line expressions. Type `exit` or press Ctrl-D to quit.

## Tests

```bash
bash tests/run_tests.sh
```

The test harness finds all `.mac` files under `tests/`, runs each through the interpreter, and checks stdout against `// expect:` annotations in the source.

Example test file:
```mac
print 2 + 2;           // expect: 4
print "hello" |> upper; // expect: HELLO
```

There are 46 tests across 19 categories (arrays, classes, control flow, effects, expressions, extensions, functional, functions, lambdas, layout, maps, memes, operators, pipes, scoping, statements, stdlib, timeline).

## VS Code Extension

Build the LSP server:
```bash
cd mac-lang
npm install
npx tsc
```

Install locally:
```bash
ln -sf "$(pwd)/mac-lang" ~/.vscode/extensions/mac-lang
```

Provides syntax highlighting, hover, go-to-definition, autocomplete, and diagnostics for `.mac` files.

## Web GIF Studio

```bash
PORT=9001 ./webapp/run.sh
```

This builds the `mac` binary (if needed) and starts a Python HTTP server on port 9001. Open `http://localhost:9001` to use the browser-based meme builder.

## Pre-built Binaries

Install the latest release without building:
```bash
curl -fsSL https://raw.githubusercontent.com/jona62/mac/main/install.sh | bash
```

Installs to `~/.mac` with a symlink at `~/.local/bin/mac`. Override with `MAC_INSTALL_DIR` and `MAC_BIN_DIR` environment variables.

## Release Builds

Tagged releases (`v*`) trigger CI to build binaries for:
- macOS arm64
- macOS x86_64
- Linux x86_64

Each release package contains the `mac` binary, `assets/`, `stdlib/`, and `README.md`.
