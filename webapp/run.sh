#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
HOST="${HOST:-0.0.0.0}"
PORT="${PORT:-9001}"

mkdir -p "$ROOT_DIR/webapp/generated"
mkdir -p "$ROOT_DIR/build"

if command -v cmake >/dev/null 2>&1; then
  cmake -S "$ROOT_DIR" -B "$ROOT_DIR/build"
  cmake --build "$ROOT_DIR/build"
else
  g++ \
    -std=c++23 \
    -O2 \
    -I"$ROOT_DIR/include" \
    "$ROOT_DIR/src/main.cpp" \
    "$ROOT_DIR/src/Scanner.cpp" \
    "$ROOT_DIR/src/Parser.cpp" \
    "$ROOT_DIR/src/Interpreter.cpp" \
    "$ROOT_DIR/src/stb_impl.cpp" \
    -o "$ROOT_DIR/build/mac"
fi

exec python3 -u "$ROOT_DIR/webapp/server.py" --host "$HOST" --port "$PORT"
