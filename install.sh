#!/bin/bash
# Mac Language Installer
# Usage: curl -fsSL https://raw.githubusercontent.com/jona62/mac/main/install.sh | bash

set -e

REPO="jona62/mac"
INSTALL_DIR="${MAC_INSTALL_DIR:-$HOME/.mac}"
BIN_DIR="${MAC_BIN_DIR:-$HOME/.local/bin}"

# Detect platform
OS="$(uname -s)"
ARCH="$(uname -m)"

case "$OS" in
    Darwin)
        case "$ARCH" in
            arm64) TARGET="mac-macos-arm64" ;;
            x86_64) TARGET="mac-macos-x86_64" ;;
            *) echo "Unsupported architecture: $ARCH"; exit 1 ;;
        esac
        ;;
    Linux)
        case "$ARCH" in
            x86_64) TARGET="mac-linux-x86_64" ;;
            *) echo "Unsupported architecture: $ARCH"; exit 1 ;;
        esac
        ;;
    *)
        echo "Unsupported OS: $OS"
        exit 1
        ;;
esac

echo "Installing Mac language ($TARGET)..."

# Get latest release URL
LATEST=$(curl -fsSL "https://api.github.com/repos/$REPO/releases/latest" | grep "browser_download_url.*$TARGET" | cut -d '"' -f 4)

if [ -z "$LATEST" ]; then
    echo "No release found for $TARGET."
    echo "Build from source: cmake -S . -B build && cmake --build build"
    exit 1
fi

# Download and extract
echo "Downloading $LATEST..."
TMPDIR=$(mktemp -d)
curl -fsSL "$LATEST" -o "$TMPDIR/mac.tar.gz"
tar xzf "$TMPDIR/mac.tar.gz" -C "$TMPDIR"

# Install
rm -rf "$INSTALL_DIR"
mv "$TMPDIR/$TARGET" "$INSTALL_DIR"
rm -rf "$TMPDIR"

# Create symlink
mkdir -p "$BIN_DIR"
ln -sf "$INSTALL_DIR/mac" "$BIN_DIR/mac"

echo ""
echo "Mac installed to $INSTALL_DIR"
echo "Binary linked at $BIN_DIR/mac"
echo ""

# Check if BIN_DIR is in PATH
if ! echo "$PATH" | grep -q "$BIN_DIR"; then
    echo "Add this to your shell profile:"
    echo "  export PATH=\"$BIN_DIR:\$PATH\""
    echo ""
fi

echo "Run 'mac' to start the REPL, or 'mac script.mac' to run a file."
echo ""
echo "  |> print \"Hello, Mac!\";"
echo "  Hello, Mac!"
