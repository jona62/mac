#!/usr/bin/env python3
from __future__ import annotations

import contextlib
import importlib.util
import io
import json
import sys
import tempfile
import unittest
from pathlib import Path

from PIL import Image, PngImagePlugin


ROOT = Path(__file__).resolve().parents[2]
TOOL_PATH = ROOT / "tools" / "meme_asset_intake.py"
SPEC = importlib.util.spec_from_file_location("meme_asset_intake", TOOL_PATH)
assert SPEC is not None and SPEC.loader is not None
meme_asset_intake = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = meme_asset_intake
SPEC.loader.exec_module(meme_asset_intake)


def write_png(path: Path, color: tuple[int, int, int, int], metadata: dict[str, str] | None = None) -> None:
    image = Image.new("RGBA", (16, 16), color)
    png_info = PngImagePlugin.PngInfo()
    for key, value in (metadata or {}).items():
        png_info.add_text(key, value)
    image.save(path, pnginfo=png_info)


class MemeAssetIntakeTests(unittest.TestCase):
    def test_pixel_hash_ignores_png_metadata(self) -> None:
        with tempfile.TemporaryDirectory(prefix="mac-meme-intake-") as temp_dir:
            root = Path(temp_dir)
            first = root / "first.png"
            second = root / "second.png"
            write_png(first, (20, 40, 60, 255), {"source": "one"})
            write_png(second, (20, 40, 60, 255), {"source": "two"})

            first_fingerprint = meme_asset_intake.fingerprint_image(first)
            second_fingerprint = meme_asset_intake.fingerprint_image(second)

            self.assertEqual(first_fingerprint.pixel_sha256, second_fingerprint.pixel_sha256)
            self.assertNotEqual(first_fingerprint.file_sha256, second_fingerprint.file_sha256)

    def test_report_marks_existing_pixel_duplicate(self) -> None:
        with tempfile.TemporaryDirectory(prefix="mac-meme-intake-") as temp_dir:
            root = Path(temp_dir)
            asset_dir = root / "assets"
            asset_dir.mkdir()
            existing = asset_dir / "known_meme.png"
            candidate = root / "candidate.png"
            write_png(existing, (100, 20, 30, 255), {"asset": "original"})
            write_png(candidate, (100, 20, 30, 255), {"asset": "downloaded"})

            existing_assets = meme_asset_intake.scan_assets(asset_dir)
            report = meme_asset_intake.build_report(
                [candidate],
                existing_assets,
                {candidate: "downloaded_meme"},
                near_threshold=0,
            )

            self.assertEqual(report[0].status, "duplicate")
            self.assertIn("known_meme", report[0].exact_duplicates[0])

    def test_report_marks_slug_conflict(self) -> None:
        with tempfile.TemporaryDirectory(prefix="mac-meme-intake-") as temp_dir:
            root = Path(temp_dir)
            asset_dir = root / "assets"
            asset_dir.mkdir()
            existing = asset_dir / "known_meme.png"
            candidate = root / "different.png"
            write_png(existing, (100, 20, 30, 255))
            write_png(candidate, (30, 80, 120, 255))

            existing_assets = meme_asset_intake.scan_assets(asset_dir)
            report = meme_asset_intake.build_report(
                [candidate],
                existing_assets,
                {candidate: "known_meme"},
                near_threshold=0,
            )

            self.assertEqual(report[0].status, "slug_conflict")
            self.assertIn("known_meme", report[0].slug_conflicts[0])

    def test_cli_check_json(self) -> None:
        with tempfile.TemporaryDirectory(prefix="mac-meme-intake-") as temp_dir:
            root = Path(temp_dir)
            asset_dir = root / "assets"
            asset_dir.mkdir()
            existing = asset_dir / "known_meme.png"
            candidate = root / "candidate.png"
            write_png(existing, (10, 30, 50, 255))
            write_png(candidate, (10, 30, 50, 255), {"download": "candidate"})

            output = io.StringIO()
            with contextlib.redirect_stdout(output):
                result = meme_asset_intake.main(
                    ["check", str(candidate), "--asset-dir", str(asset_dir), "--json"]
                )

            payload = json.loads(output.getvalue())
            self.assertEqual(result, 0)
            self.assertEqual(payload["summary"]["duplicate"], 1)
            self.assertEqual(payload["items"][0]["status"], "duplicate")


if __name__ == "__main__":
    unittest.main()
