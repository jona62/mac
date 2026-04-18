#!/usr/bin/env python3
"""Generate emoji sprite atlas PNG + JSON metadata from Twemoji 72x72 PNGs.

Usage:
    python3 tools/gen_emoji_atlas.py /path/to/twemoji-14.0.2/assets/72x72/

Outputs:
    assets/emoji/emoji_atlas.png
    assets/emoji/emoji_meta.json
"""
import json
import math
import os
import sys
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    print("Error: Pillow required. Install with: pip3 install Pillow", file=sys.stderr)
    sys.exit(1)

CELL = 72
OUT_DIR = Path(__file__).resolve().parent.parent / "assets" / "emoji"


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <twemoji-72x72-dir>", file=sys.stderr)
        sys.exit(1)

    src = Path(sys.argv[1])
    if not src.is_dir():
        print(f"Error: {src} is not a directory", file=sys.stderr)
        sys.exit(1)

    pngs = sorted(src.glob("*.png"))
    if not pngs:
        print(f"Error: no PNGs found in {src}", file=sys.stderr)
        sys.exit(1)

    print(f"Found {len(pngs)} emoji PNGs")

    cols = math.ceil(math.sqrt(len(pngs)))
    rows = math.ceil(len(pngs) / cols)
    atlas_w = cols * CELL
    atlas_h = rows * CELL

    print(f"Atlas: {cols}x{rows} grid = {atlas_w}x{atlas_h}px")

    atlas = Image.new("RGBA", (atlas_w, atlas_h), (0, 0, 0, 0))
    sprites = {}

    for idx, png_path in enumerate(pngs):
        col = idx % cols
        row = idx // cols
        key = png_path.stem.upper()

        img = Image.open(png_path).convert("RGBA")
        if img.size != (CELL, CELL):
            img = img.resize((CELL, CELL), Image.LANCZOS)

        atlas.paste(img, (col * CELL, row * CELL))
        sprites[key] = [col, row]

    OUT_DIR.mkdir(parents=True, exist_ok=True)

    atlas_path = OUT_DIR / "emoji_atlas.png"
    atlas.save(atlas_path, optimize=True)
    print(f"Saved atlas: {atlas_path} ({os.path.getsize(atlas_path) / 1024:.0f} KB)")

    meta = {"cell": CELL, "cols": cols, "sprites": sprites}
    meta_path = OUT_DIR / "emoji_meta.json"
    meta_path.write_text(json.dumps(meta, separators=(",", ":")))
    print(f"Saved metadata: {meta_path} ({os.path.getsize(meta_path) / 1024:.0f} KB)")
    print(f"Total emoji: {len(sprites)}")


if __name__ == "__main__":
    main()
