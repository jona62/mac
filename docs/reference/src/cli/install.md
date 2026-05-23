# Installation

## Quick Install

```bash
curl -fsSL https://macstudio.meme/install.sh | bash
```

This detects your platform, downloads the latest release binary, and installs to `~/.mac/` with a symlink at `~/.local/bin/mac`.

## Supported Platforms

| Platform | Architecture |
|----------|-------------|
| macOS | arm64 (Apple Silicon) |
| macOS | x86_64 (Intel) |
| Linux | x86_64 |

## Updating

Run the install script again. It detects existing installations and updates in place.

Mac also checks for updates on startup (once per 24 hours) and prints a message if a newer version is available.

## Build from Source

```bash
git clone https://github.com/jona62/mac.git
cd mac
cmake -S . -B build
cmake --build build
./build/mac --version
```

Requires CMake 3.20+ and a C++23 compiler.

## Uninstall

```bash
mac --uninstall
```

Removes `~/.mac/` (binary, assets, stdlib), `~/.local/bin/mac` (symlink), and `~/.mac_history` (REPL history). Your generated output in `~/mac/output/` is kept by default.

To also remove generated output files:

```bash
mac --uninstall --purge
```

## File Locations

| Path | Contents |
|------|----------|
| `~/.mac/` | Binary, assets, stdlib |
| `~/.local/bin/mac` | Symlink to binary |
| `~/.mac_history` | REPL command history |
| `~/.mac/update_check` | Update check cache |
| `~/mac/output/` | Default generated output for bare filenames |
