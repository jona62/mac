#!/usr/bin/env python3
"""Check and ingest meme template assets without adding duplicates.

The duplicate check hashes decoded pixels instead of file bytes, so metadata,
EXIF, and PNG/JPEG container differences do not create false "new" assets.

Examples:
    python3 tools/meme_asset_intake.py scan
    python3 tools/meme_asset_intake.py check /tmp/meme-candidates --json
    python3 tools/meme_asset_intake.py ingest /tmp/meme.jpg --slug distracted_boyfriend
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

try:
    from PIL import Image, ImageOps, ImageSequence, UnidentifiedImageError
except ImportError:  # pragma: no cover - exercised only on machines without Pillow
    Image = None
    ImageOps = None
    ImageSequence = None
    UnidentifiedImageError = OSError


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ASSET_DIR = ROOT / "assets" / "templates" / "meme"
IMAGE_EXTENSIONS = {".png", ".jpg", ".jpeg", ".gif", ".bmp", ".webp"}
VALID_SLUG = re.compile(r"^[a-z][a-z0-9]*(?:_[a-z0-9]+)*$")
DEFAULT_NEAR_THRESHOLD = 5


@dataclass(frozen=True)
class ImageFingerprint:
    path: Path
    width: int
    height: int
    file_sha256: str
    pixel_sha256: str
    dhash: str

    @property
    def slug(self) -> str:
        return self.path.stem

    def to_json(self) -> dict[str, object]:
        return {
            "path": str(self.path),
            "slug": self.slug,
            "width": self.width,
            "height": self.height,
            "file_sha256": self.file_sha256,
            "pixel_sha256": self.pixel_sha256,
            "dhash": self.dhash,
        }


@dataclass(frozen=True)
class ReportItem:
    path: Path
    proposed_slug: str
    fingerprint: ImageFingerprint
    exact_duplicates: tuple[str, ...]
    near_duplicates: tuple[dict[str, object], ...]
    slug_conflicts: tuple[str, ...]

    @property
    def status(self) -> str:
        if self.exact_duplicates:
            return "duplicate"
        if self.slug_conflicts:
            return "slug_conflict"
        if self.near_duplicates:
            return "near_duplicate"
        return "new"

    def to_json(self) -> dict[str, object]:
        data = self.fingerprint.to_json()
        data.update(
            {
                "status": self.status,
                "proposed_slug": self.proposed_slug,
                "exact_duplicates": list(self.exact_duplicates),
                "near_duplicates": list(self.near_duplicates),
                "slug_conflicts": list(self.slug_conflicts),
            }
        )
        return data


def require_pillow() -> None:
    if Image is None:
        raise SystemExit("Pillow is required. Install it with: python3 -m pip install Pillow")


def file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def first_frame_rgba(path: Path):
    require_pillow()
    try:
        with Image.open(path) as source:
            if getattr(source, "is_animated", False):
                frame = next(ImageSequence.Iterator(source)).copy()
            else:
                frame = source.copy()
            frame = ImageOps.exif_transpose(frame)
            return frame.convert("RGBA")
    except (OSError, UnidentifiedImageError) as exc:
        raise ValueError(f"{path} is not a readable image: {exc}") from exc


def pixel_sha256(image) -> str:
    digest = hashlib.sha256()
    digest.update(f"{image.width}x{image.height}:RGBA\n".encode("ascii"))
    digest.update(image.tobytes())
    return digest.hexdigest()


def difference_hash(image) -> str:
    resampling = getattr(Image, "Resampling", Image)
    grayscale = image.convert("L").resize((9, 8), resampling.LANCZOS)
    pixels = list(grayscale.getdata())
    value = 0
    for row in range(8):
        offset = row * 9
        for column in range(8):
            value <<= 1
            if pixels[offset + column] > pixels[offset + column + 1]:
                value |= 1
    return f"{value:016x}"


def hamming_distance(left: str, right: str) -> int:
    return (int(left, 16) ^ int(right, 16)).bit_count()


def fingerprint_image(path: Path) -> ImageFingerprint:
    image = first_frame_rgba(path)
    return ImageFingerprint(
        path=path,
        width=image.width,
        height=image.height,
        file_sha256=file_sha256(path),
        pixel_sha256=pixel_sha256(image),
        dhash=difference_hash(image),
    )


def is_image_path(path: Path) -> bool:
    return path.suffix.lower() in IMAGE_EXTENSIONS


def collect_image_files(inputs: Iterable[Path]) -> list[Path]:
    collected: list[Path] = []
    seen: set[Path] = set()
    for raw_path in inputs:
        path = raw_path.expanduser()
        if path.is_dir():
            paths = sorted(child for child in path.rglob("*") if child.is_file() and is_image_path(child))
        elif path.is_file() and is_image_path(path):
            paths = [path]
        elif path.is_file():
            raise SystemExit(f"Unsupported image extension: {path}")
        else:
            raise SystemExit(f"Path does not exist: {path}")

        for candidate in paths:
            resolved = candidate.resolve()
            if resolved not in seen:
                seen.add(resolved)
                collected.append(candidate)

    return collected


def scan_assets(asset_dir: Path) -> list[ImageFingerprint]:
    if not asset_dir.exists():
        return []
    return [fingerprint_image(path) for path in collect_image_files([asset_dir])]


def slugify(value: str) -> str:
    slug = re.sub(r"[^a-z0-9]+", "_", value.lower()).strip("_")
    slug = re.sub(r"_+", "_", slug)
    if not slug:
        slug = "meme_asset"
    if slug[0].isdigit():
        slug = f"meme_{slug}"
    return slug


def validate_slug(slug: str) -> str:
    if not VALID_SLUG.fullmatch(slug):
        raise SystemExit(
            f"Invalid slug `{slug}`. Use lowercase snake_case, starting with a letter."
        )
    return slug


def load_slug_map(path: Path | None) -> dict[str, str]:
    if path is None:
        return {}
    with path.open("r", encoding="utf-8") as handle:
        raw = json.load(handle)
    if not isinstance(raw, dict):
        raise SystemExit("--slug-map must be a JSON object of source path/name to slug")
    slug_map = {str(key): validate_slug(str(value)) for key, value in raw.items()}
    return slug_map


def slug_for_candidate(path: Path, slug: str | None, slug_map: dict[str, str]) -> str:
    if slug is not None:
        return validate_slug(slug)

    for key in (str(path), str(path.resolve()), path.name, path.stem):
        if key in slug_map:
            return slug_map[key]
    return validate_slug(slugify(path.stem))


def build_report(
    candidate_paths: list[Path],
    existing_assets: list[ImageFingerprint],
    slugs: dict[Path, str],
    near_threshold: int,
) -> list[ReportItem]:
    pixel_index: dict[str, list[str]] = {}
    slug_index: dict[str, list[str]] = {}
    phash_index: list[tuple[str, str]] = []

    for asset in existing_assets:
        label = f"{asset.slug} ({asset.path.name})"
        pixel_index.setdefault(asset.pixel_sha256, []).append(label)
        slug_index.setdefault(asset.slug, []).append(label)
        phash_index.append((asset.dhash, label))

    report: list[ReportItem] = []
    for path in candidate_paths:
        fingerprint = fingerprint_image(path)
        proposed_slug = slugs[path]
        exact_duplicates = tuple(pixel_index.get(fingerprint.pixel_sha256, ()))
        slug_conflicts = tuple(slug_index.get(proposed_slug, ()))
        near_duplicates = tuple(
            {"asset": label, "distance": distance}
            for known_hash, label in phash_index
            for distance in [hamming_distance(fingerprint.dhash, known_hash)]
            if distance <= near_threshold and label not in exact_duplicates
        )

        report.append(
            ReportItem(
                path=path,
                proposed_slug=proposed_slug,
                fingerprint=fingerprint,
                exact_duplicates=exact_duplicates,
                near_duplicates=near_duplicates,
                slug_conflicts=slug_conflicts,
            )
        )

        candidate_label = f"{proposed_slug} ({path.name})"
        pixel_index.setdefault(fingerprint.pixel_sha256, []).append(candidate_label)
        slug_index.setdefault(proposed_slug, []).append(candidate_label)
        phash_index.append((fingerprint.dhash, candidate_label))

    return report


def build_slug_plan(paths: list[Path], single_slug: str | None, slug_map: dict[str, str]) -> dict[Path, str]:
    if single_slug is not None and len(paths) != 1:
        raise SystemExit("--slug can only be used with one candidate image")
    return {path: slug_for_candidate(path, single_slug, slug_map) for path in paths}


def destination_for(asset_dir: Path, slug: str, source: Path) -> Path:
    suffix = source.suffix.lower()
    if suffix == ".jpeg":
        suffix = ".jpg"
    return asset_dir / f"{slug}{suffix}"


def find_existing_slug_files(asset_dir: Path, slug: str) -> list[Path]:
    if not asset_dir.exists():
        return []
    return sorted(path for path in asset_dir.iterdir() if path.is_file() and is_image_path(path) and path.stem == slug)


def format_matches(item: ReportItem) -> str:
    if item.exact_duplicates:
        return ", ".join(item.exact_duplicates)
    if item.slug_conflicts:
        return ", ".join(item.slug_conflicts)
    if item.near_duplicates:
        return ", ".join(f"{match['asset']} d={match['distance']}" for match in item.near_duplicates[:3])
    return ""


def print_table(report: list[ReportItem]) -> None:
    print(f"{'Status':<15} {'Slug':<28} {'Size':<12} Source")
    print("-" * 88)
    for item in report:
        size = f"{item.fingerprint.width}x{item.fingerprint.height}"
        matches = format_matches(item)
        suffix = f" -> {matches}" if matches else ""
        print(f"{item.status:<15} {item.proposed_slug:<28} {size:<12} {item.path}{suffix}")


def print_scan(fingerprints: list[ImageFingerprint]) -> None:
    print(f"{'Slug':<28} {'Size':<12} {'Pixel SHA-256':<16} {'dHash':<16} File")
    print("-" * 104)
    for fingerprint in fingerprints:
        size = f"{fingerprint.width}x{fingerprint.height}"
        print(
            f"{fingerprint.slug:<28} {size:<12} "
            f"{fingerprint.pixel_sha256[:16]:<16} {fingerprint.dhash:<16} {fingerprint.path.name}"
        )


def summarize(report: list[ReportItem]) -> dict[str, int]:
    counts = {"new": 0, "duplicate": 0, "near_duplicate": 0, "slug_conflict": 0}
    for item in report:
        counts[item.status] += 1
    return counts


def command_scan(args: argparse.Namespace) -> int:
    fingerprints = scan_assets(args.asset_dir)
    if args.json:
        print(json.dumps([fingerprint.to_json() for fingerprint in fingerprints], indent=2))
    else:
        print_scan(fingerprints)
        print(f"\nScanned {len(fingerprints)} asset(s).")
    return 0


def command_check(args: argparse.Namespace) -> int:
    candidates = collect_image_files(args.candidates)
    slug_map = load_slug_map(args.slug_map)
    slugs = build_slug_plan(candidates, args.slug, slug_map)
    existing_assets = scan_assets(args.asset_dir)
    report = build_report(candidates, existing_assets, slugs, args.near_threshold)

    if args.json:
        print(json.dumps({"summary": summarize(report), "items": [item.to_json() for item in report]}, indent=2))
    else:
        print_table(report)
        print(f"\nSummary: {summarize(report)}")
    return 0


def command_ingest(args: argparse.Namespace) -> int:
    candidates = collect_image_files(args.candidates)
    slug_map = load_slug_map(args.slug_map)
    slugs = build_slug_plan(candidates, args.slug, slug_map)
    existing_assets = scan_assets(args.asset_dir)
    report = build_report(candidates, existing_assets, slugs, args.near_threshold)
    actions: list[dict[str, object]] = []

    if not args.json:
        print_table(report)
        print()

    for item in report:
        target = destination_for(args.asset_dir, item.proposed_slug, item.path)
        skipped_reason = ""
        existing_slug_files = find_existing_slug_files(args.asset_dir, item.proposed_slug)

        if item.exact_duplicates and not args.allow_duplicates:
            skipped_reason = "exact pixel duplicate"
        elif item.slug_conflicts and not args.replace:
            skipped_reason = "slug already exists"
        elif item.near_duplicates and args.fail_near:
            skipped_reason = "near duplicate"
        elif existing_slug_files and not args.replace:
            skipped_reason = "slug already exists"

        action = {
            "source": str(item.path),
            "target": str(target),
            "status": item.status,
            "dry_run": args.dry_run,
        }

        if skipped_reason:
            action["action"] = "skip"
            action["reason"] = skipped_reason
            actions.append(action)
            if not args.json:
                print(f"skip {item.path}: {skipped_reason}")
            continue

        action["action"] = "copy"
        actions.append(action)
        if args.dry_run:
            if not args.json:
                print(f"would copy {item.path} -> {target}")
            continue

        args.asset_dir.mkdir(parents=True, exist_ok=True)
        if args.replace:
            for existing in existing_slug_files:
                existing.unlink()
        shutil.copy2(item.path, target)
        if not args.json:
            print(f"copied {item.path} -> {target}")

    if args.json:
        print(json.dumps({"summary": summarize(report), "actions": actions}, indent=2))
    return 0


def add_shared_candidate_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("candidates", nargs="+", type=Path, help="Image files or directories to inspect.")
    parser.add_argument(
        "--asset-dir",
        type=Path,
        default=DEFAULT_ASSET_DIR,
        help=f"Existing meme asset directory. Defaults to {DEFAULT_ASSET_DIR}.",
    )
    parser.add_argument("--slug", help="Slug for a single candidate, for example distracted_boyfriend.")
    parser.add_argument("--slug-map", type=Path, help="JSON object mapping source path/name/stem to slug.")
    parser.add_argument(
        "--near-threshold",
        type=int,
        default=DEFAULT_NEAR_THRESHOLD,
        help="dHash Hamming distance that counts as a near duplicate.",
    )
    parser.add_argument("--json", action="store_true", help="Emit machine-readable JSON.")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)

    scan = subparsers.add_parser("scan", help="Fingerprint the current meme asset library.")
    scan.add_argument(
        "--asset-dir",
        type=Path,
        default=DEFAULT_ASSET_DIR,
        help=f"Existing meme asset directory. Defaults to {DEFAULT_ASSET_DIR}.",
    )
    scan.add_argument("--json", action="store_true", help="Emit machine-readable JSON.")
    scan.set_defaults(func=command_scan)

    check = subparsers.add_parser("check", help="Report duplicates and naming conflicts for candidates.")
    add_shared_candidate_args(check)
    check.set_defaults(func=command_check)

    ingest = subparsers.add_parser("ingest", help="Copy non-duplicate candidates into assets/templates/meme.")
    add_shared_candidate_args(ingest)
    ingest.add_argument("--dry-run", action="store_true", help="Show copy actions without writing files.")
    ingest.add_argument("--allow-duplicates", action="store_true", help="Allow exact pixel duplicates.")
    ingest.add_argument("--fail-near", action="store_true", help="Skip near duplicates as well as exact duplicates.")
    ingest.add_argument("--replace", action="store_true", help="Replace an existing asset with the same slug.")
    ingest.set_defaults(func=command_ingest)

    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    if hasattr(args, "near_threshold") and args.near_threshold < 0:
        parser.error("--near-threshold must be >= 0")
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
