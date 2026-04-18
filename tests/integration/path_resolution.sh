#!/bin/bash
set -euo pipefail

MAC="$1"
TMP_ROOT="$(mktemp -d "${TMPDIR:-/tmp}/mac-path-test.XXXXXX")"
trap 'rm -rf "$TMP_ROOT"' EXIT

UNIQUE_ID="$(date +%s)-$$"
FILE_NAME="path_resolution_${UNIQUE_ID}.png"
SCRIPT_PATH="$TMP_ROOT/path_resolution.mac"
WORK_DIR="$TMP_ROOT/work"
OVERRIDE_DIR="$TMP_ROOT/override"

mkdir -p "$WORK_DIR"

cat > "$SCRIPT_PATH" <<EOF
@blank "Path Resolution" => "nested/$FILE_NAME";
EOF

STDOUT_FILE="$TMP_ROOT/stdout.txt"
STDERR_FILE="$TMP_ROOT/stderr.txt"

extract_saved_path() {
    python3 - "$1" <<'PY'
import pathlib
import re
import sys

text = pathlib.Path(sys.argv[1]).read_text()
text = re.sub(r'\x1b\[[0-9;]*m', '', text)
match = re.search(r'Saved\s+([^\n]+)', text)
if match:
    print(match.group(1).strip())
PY
}

same_file() {
    python3 - "$1" "$2" <<'PY'
import os
import sys

left, right = sys.argv[1], sys.argv[2]
ok = os.path.exists(left) and os.path.exists(right) and os.path.samefile(left, right)
sys.exit(0 if ok else 1)
PY
}

(cd "$WORK_DIR" && "$MAC" "$SCRIPT_PATH" >"$STDOUT_FILE" 2>"$STDERR_FILE")

EXPECTED_RELATIVE="$WORK_DIR/nested/$FILE_NAME"
REPORTED_RELATIVE="$(extract_saved_path "$STDERR_FILE")"
if [ ! -f "$EXPECTED_RELATIVE" ]; then
    echo "explicit relative save path was not created: $EXPECTED_RELATIVE" >&2
    exit 1
fi
if [ -z "$REPORTED_RELATIVE" ] || ! same_file "$REPORTED_RELATIVE" "$EXPECTED_RELATIVE"; then
    echo "saved path notice did not mention explicit relative output path" >&2
    cat "$STDERR_FILE" >&2
    exit 1
fi

rm -f "$EXPECTED_RELATIVE"
mkdir -p "$OVERRIDE_DIR"
(cd "$WORK_DIR" && MAC_OUTPUT_DIR="$OVERRIDE_DIR" "$MAC" "$SCRIPT_PATH" >"$STDOUT_FILE" 2>"$STDERR_FILE")

EXPECTED_OVERRIDE="$OVERRIDE_DIR/$FILE_NAME"
REPORTED_OVERRIDE="$(extract_saved_path "$STDERR_FILE")"
if [ ! -f "$EXPECTED_OVERRIDE" ]; then
    echo "MAC_OUTPUT_DIR override did not capture output: $EXPECTED_OVERRIDE" >&2
    exit 1
fi
if [ -f "$EXPECTED_RELATIVE" ]; then
    echo "explicit relative path should not be used when MAC_OUTPUT_DIR is set" >&2
    exit 1
fi
if [ -z "$REPORTED_OVERRIDE" ] || ! same_file "$REPORTED_OVERRIDE" "$EXPECTED_OVERRIDE"; then
    echo "saved path notice did not mention MAC_OUTPUT_DIR output path" >&2
    cat "$STDERR_FILE" >&2
    exit 1
fi
