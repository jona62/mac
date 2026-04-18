#!/bin/bash
# Test template path resolution: relative paths, symlinks, traversal blocking
set -euo pipefail

MAC="$1"
TMP="$(mktemp -d "${TMPDIR:-/tmp}/mac-tpl-test.XXXXXX")"
trap 'rm -rf "$TMP"' EXIT

PASS=0
FAIL=0

ok() { PASS=$((PASS + 1)); echo "  PASS  $1"; }
fail() { FAIL=$((FAIL + 1)); echo "  FAIL  $1: $2"; }

# Create a tiny valid PNG for tests
python3 -c "
import struct, zlib
w, h = 2, 2
raw = b''
for y in range(h):
    raw += b'\\x00' + b'\\xff\\x00\\x00\\xff' * w
compressed = zlib.compress(raw)
def chunk(name, data):
    c = zlib.crc32(name + data) & 0xffffffff
    return struct.pack('>I', len(data)) + name + data + struct.pack('>I', c)
sig = b'\\x89PNG\\r\\n\\x1a\\n'
ihdr = chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 6, 0, 0, 0))
idat = chunk(b'IDAT', compressed)
iend = chunk(b'IEND', b'')
open('$TMP/test.png', 'wb').write(sig + ihdr + idat + iend)
"

# --- Test 1: Relative path in script dir ---
cp "$TMP/test.png" "$TMP/my_image.png"
cat > "$TMP/t1.mac" <<'EOF'
@"my_image.png" "hello" => "t1_out.png";
EOF
if MAC_OUTPUT_DIR="$TMP" "$MAC" "$TMP/t1.mac" >/dev/null 2>&1 && [ -f "$TMP/t1_out.png" ]; then
    ok "relative path in script dir"
else
    fail "relative path in script dir" "render failed"
fi

# --- Test 2: Subdirectory relative path ---
mkdir -p "$TMP/images"
cp "$TMP/test.png" "$TMP/images/photo.png"
cat > "$TMP/t2.mac" <<'EOF'
@"images/photo.png" "hello" => "t2_out.png";
EOF
if MAC_OUTPUT_DIR="$TMP" "$MAC" "$TMP/t2.mac" >/dev/null 2>&1 && [ -f "$TMP/t2_out.png" ]; then
    ok "subdirectory relative path"
else
    fail "subdirectory relative path" "render failed"
fi

# --- Test 3: Symlinked directory ---
mkdir -p "$TMP/real_uploads"
cp "$TMP/test.png" "$TMP/real_uploads/bg.png"
ln -sf "$TMP/real_uploads" "$TMP/uploads"
cat > "$TMP/t3.mac" <<'EOF'
@"uploads/bg.png" "hello" => "t3_out.png";
EOF
if MAC_OUTPUT_DIR="$TMP" "$MAC" "$TMP/t3.mac" >/dev/null 2>&1 && [ -f "$TMP/t3_out.png" ]; then
    ok "symlinked directory"
else
    fail "symlinked directory" "render failed"
fi

# --- Test 4: Path traversal blocked ---
cat > "$TMP/t4.mac" <<'EOF'
@"../../../etc/passwd" "hello" => "t4_out.png";
EOF
if MAC_OUTPUT_DIR="$TMP" "$MAC" "$TMP/t4.mac" >/dev/null 2>&1; then
    fail "path traversal blocked" "should have failed but succeeded"
else
    ok "path traversal blocked"
fi

# --- Test 5: Dotted template (meme asset) ---
cat > "$TMP/t5.mac" <<'EOF'
@meme.shrek_smirk "hello" => "t5_out.png";
EOF
if MAC_OUTPUT_DIR="$TMP" "$MAC" "$TMP/t5.mac" >/dev/null 2>&1 && [ -f "$TMP/t5_out.png" ]; then
    ok "dotted meme asset"
else
    fail "dotted meme asset" "render failed"
fi

# --- Test 6: Built-in template ---
cat > "$TMP/t6.mac" <<'EOF'
@blank "hello" => "t6_out.png";
EOF
if MAC_OUTPUT_DIR="$TMP" "$MAC" "$TMP/t6.mac" >/dev/null 2>&1 && [ -f "$TMP/t6_out.png" ]; then
    ok "built-in template"
else
    fail "built-in template" "render failed"
fi

# --- Test 7: Output path sanitized (absolute stripped to filename) ---
cat > "$TMP/t7.mac" <<'EOF'
@blank "hello" => "/tmp/evil/absolute.png";
EOF
if MAC_OUTPUT_DIR="$TMP" "$MAC" "$TMP/t7.mac" >/dev/null 2>&1 && [ -f "$TMP/absolute.png" ]; then
    ok "absolute output path sanitized"
else
    fail "absolute output path sanitized" "file not in output dir"
fi

# --- Test 8: Output traversal stripped ---
cat > "$TMP/t8.mac" <<'EOF'
@blank "hello" => "../../evil.png";
EOF
if MAC_OUTPUT_DIR="$TMP" "$MAC" "$TMP/t8.mac" >/dev/null 2>&1 && [ -f "$TMP/evil.png" ]; then
    ok "output traversal stripped to filename"
else
    fail "output traversal stripped to filename" "file not in output dir"
fi

# --- Test 9: MAC_OUTPUT_DIR override ---
mkdir -p "$TMP/custom_out"
cat > "$TMP/t9.mac" <<'EOF'
@blank "hello" => "t9_out.png";
EOF
if MAC_OUTPUT_DIR="$TMP/custom_out" "$MAC" "$TMP/t9.mac" >/dev/null 2>&1 && [ -f "$TMP/custom_out/t9_out.png" ]; then
    ok "MAC_OUTPUT_DIR override"
else
    fail "MAC_OUTPUT_DIR override" "file not in custom output dir"
fi

# --- Test 10: Dimension clamping (no crash on huge values) ---
cat > "$TMP/t10.mac" <<'EOF'
@blank 99999x99999 "hello" => "t10_out.png";
EOF
if MAC_OUTPUT_DIR="$TMP" "$MAC" "$TMP/t10.mac" >/dev/null 2>&1 && [ -f "$TMP/t10_out.png" ]; then
    ok "dimension clamping (no crash)"
else
    fail "dimension clamping" "crashed or no output"
fi

echo ""
echo "  Template paths: $PASS passed, $FAIL failed"
[ "$FAIL" -eq 0 ]
