#!/bin/bash
# Mac Language Installer
# Usage: curl -fsSL https://raw.githubusercontent.com/jona62/mac/main/install.sh | bash

set -e

REPO="jona62/mac"
INSTALL_DIR="${MAC_INSTALL_DIR:-$HOME/.mac}"
BIN_DIR="${MAC_BIN_DIR:-$HOME/.local/bin}"

# ── Colors ──
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
BOLD='\033[1m'
DIM='\033[2m'
RESET='\033[0m'

# ── Spinner ──
spin() {
    local pid=$1
    local msg=$2
    local frames='⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏'
    local i=0
    while kill -0 "$pid" 2>/dev/null; do
        printf "\r  ${CYAN}${frames:$i:1}${RESET} ${msg}"
        i=$(( (i + 1) % ${#frames} ))
        sleep 0.08
    done
    wait "$pid" 2>/dev/null
    local exit_code=$?
    if [ $exit_code -eq 0 ]; then
        printf "\r  ${GREEN}✓${RESET} ${msg}\n"
    else
        printf "\r  ${RED}✗${RESET} ${msg}\n"
        return $exit_code
    fi
}

# ── Banner ──
echo ""
printf "  ${BOLD}${MAGENTA}┌─────────────────────────────┐${RESET}\n"
printf "  ${BOLD}${MAGENTA}│${RESET}  ${BOLD}Mac${RESET} ${DIM}— Meme as Code${RESET}         ${BOLD}${MAGENTA}│${RESET}\n"
printf "  ${BOLD}${MAGENTA}└─────────────────────────────┘${RESET}\n"
echo ""

# ── Detect platform ──
OS="$(uname -s)"
ARCH="$(uname -m)"

case "$OS" in
    Darwin)
        case "$ARCH" in
            arm64) TARGET="mac-macos-arm64" ;;
            x86_64) TARGET="mac-macos-x86_64" ;;
            *) printf "  ${RED}✗${RESET} Unsupported architecture: ${BOLD}$ARCH${RESET}\n"; exit 1 ;;
        esac
        ;;
    Linux)
        case "$ARCH" in
            x86_64) TARGET="mac-linux-x86_64" ;;
            *) printf "  ${RED}✗${RESET} Unsupported architecture: ${BOLD}$ARCH${RESET}\n"; exit 1 ;;
        esac
        ;;
    *)
        printf "  ${RED}✗${RESET} Unsupported OS: ${BOLD}$OS${RESET}\n"
        exit 1
        ;;
esac

if [ -f "$BIN_DIR/mac" ]; then
    printf "  ${YELLOW}↻${RESET} Updating ${DIM}($TARGET)${RESET}\n"
else
    printf "  ${BLUE}↓${RESET} Installing ${DIM}($TARGET)${RESET}\n"
fi

# ── Fetch latest release ──
TMPDIR=$(mktemp -d)

printf "  ${CYAN}⠹${RESET} Fetching latest release\r"
RELEASE_JSON=$(curl -fsSL "https://api.github.com/repos/$REPO/releases/latest" </dev/null)
LATEST=$(echo "$RELEASE_JSON" | grep "browser_download_url.*$TARGET" | cut -d '"' -f 4)
VERSION=$(echo "$RELEASE_JSON" | grep '"tag_name"' | head -1 | cut -d '"' -f 4)
printf "  ${GREEN}✓${RESET} Fetching latest release\n"

if [ -z "$LATEST" ]; then
    printf "  ${RED}✗${RESET} No release found for ${BOLD}$TARGET${RESET}\n"
    printf "  ${DIM}Build from source: cmake -S . -B build && cmake --build build${RESET}\n"
    rm -rf "$TMPDIR"
    exit 1
fi

printf "  ${GREEN}✓${RESET} Found ${BOLD}$VERSION${RESET}\n"

# ── Download ──
curl -fsSL "$LATEST" -o "$TMPDIR/mac.tar.gz" </dev/null &
spin $! "Downloading binary"

# ── Extract ──
tar xzf "$TMPDIR/mac.tar.gz" -C "$TMPDIR" &
spin $! "Extracting archive"

# ── Install ──
rm -rf "$INSTALL_DIR"
mv "$TMPDIR/$TARGET" "$INSTALL_DIR"
rm -rf "$TMPDIR"
mkdir -p "$BIN_DIR"
ln -sf "$INSTALL_DIR/mac" "$BIN_DIR/mac"
printf "  ${GREEN}✓${RESET} Installed to ${DIM}$INSTALL_DIR${RESET}\n"
printf "  ${GREEN}✓${RESET} Linked at ${DIM}$BIN_DIR/mac${RESET}\n"

# ── PATH check ──
if ! echo "$PATH" | grep -q "$BIN_DIR"; then
    echo ""
    printf "  ${YELLOW}!${RESET} Add to your shell profile:\n"
    printf "    ${DIM}export PATH=\"$BIN_DIR:\$PATH\"${RESET}\n"
fi

# ── Done ──
echo ""
printf "  ${BOLD}${GREEN}Ready!${RESET} Run ${CYAN}mac${RESET} to start the REPL.\n"
echo ""
printf "  ${DIM}|>${RESET} print \"Hello, Mac!\"\n"
printf "  ${DIM}Hello, Mac!${RESET}\n"
echo ""
