#!/bin/bash
# Mac Language Uninstaller
# Usage: curl -fsSL https://macstudio.meme/uninstall.sh | bash

set -e

INSTALL_DIR="${MAC_INSTALL_DIR:-$HOME/.mac}"
BIN_DIR="${MAC_BIN_DIR:-$HOME/.local/bin}"
OUTPUT_DIR="$HOME/mac/output"

# ── Colors ──
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
DIM='\033[2m'
RESET='\033[0m'

echo ""
printf "  ${BOLD}${MAGENTA}┌─────────────────────────────┐${RESET}\n"
printf "  ${BOLD}${MAGENTA}│${RESET}  ${BOLD}Mac${RESET} ${DIM}— Uninstaller${RESET}          ${BOLD}${MAGENTA}│${RESET}\n"
printf "  ${BOLD}${MAGENTA}└─────────────────────────────┘${RESET}\n"
echo ""

removed=0

if [ -d "$INSTALL_DIR" ]; then
    rm -rf "$INSTALL_DIR"
    printf "  ${GREEN}✓${RESET} Removed ${DIM}$INSTALL_DIR${RESET}\n"
    removed=1
fi

if [ -L "$BIN_DIR/mac" ]; then
    rm -f "$BIN_DIR/mac"
    printf "  ${GREEN}✓${RESET} Removed symlink ${DIM}$BIN_DIR/mac${RESET}\n"
    removed=1
elif [ -f "$BIN_DIR/mac" ]; then
    rm -f "$BIN_DIR/mac"
    printf "  ${GREEN}✓${RESET} Removed binary ${DIM}$BIN_DIR/mac${RESET}\n"
    removed=1
fi

if [ -f "$HOME/.mac_history" ]; then
    rm -f "$HOME/.mac_history"
    printf "  ${GREEN}✓${RESET} Removed ${DIM}~/.mac_history${RESET}\n"
fi

if [ -d "$OUTPUT_DIR" ]; then
    if [ "$1" = "--purge" ]; then
        count=$(find "$OUTPUT_DIR" -type f 2>/dev/null | wc -l | tr -d ' ')
        rm -rf "$OUTPUT_DIR"
        printf "  ${GREEN}✓${RESET} Removed output directory ${DIM}($count files)${RESET}\n"
    else
        printf "  ${DIM}-${RESET} Kept ${DIM}$OUTPUT_DIR${RESET} (use --purge to remove)\n"
    fi
    removed=1
fi

if [ $removed -eq 0 ]; then
    printf "  ${YELLOW}!${RESET} Mac is not installed.\n"
else
    echo ""
    printf "  ${BOLD}${GREEN}Done.${RESET} Mac has been uninstalled.\n"
fi

echo ""
