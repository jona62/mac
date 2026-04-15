#!/usr/bin/env python3
from __future__ import annotations

import argparse
import collections
import json
import mimetypes
import re
import socketserver
import subprocess
import tempfile
import threading
import time
import uuid
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import unquote, urlparse

ROOT_DIR = Path(__file__).resolve().parent.parent
WEBAPP_DIR = ROOT_DIR / "webapp"
STATIC_DIR = WEBAPP_DIR / "static"
GENERATED_DIR = WEBAPP_DIR / "generated"
UPLOADS_DIR = WEBAPP_DIR / "uploads"
MAC_BINARY = ROOT_DIR / "build" / "mac"

MAX_REQUEST_BYTES = 256 * 1024
MAX_UPLOAD_BYTES = 5 * 1024 * 1024   # 5 MB max image upload
MAX_UPLOADS = 50                      # max uploaded images retained
MAX_SCENE_COUNT = 24
MAX_FILE_AGE_SECONDS = 60 * 60 * 24
MAX_GENERATED_FILES = 200
RATE_LIMIT_WINDOW = 60          # seconds
RATE_LIMIT_MAX_REQUESTS = 30    # max renders per IP per window


class RateLimiter:
    """Thread-safe sliding-window rate limiter keyed by IP."""

    def __init__(self, window: int = RATE_LIMIT_WINDOW, max_reqs: int = RATE_LIMIT_MAX_REQUESTS):
        self._window = window
        self._max = max_reqs
        self._lock = threading.Lock()
        self._hits: dict[str, collections.deque] = {}

    def allow(self, ip: str) -> bool:
        now = time.monotonic()
        with self._lock:
            q = self._hits.setdefault(ip, collections.deque())
            while q and q[0] < now - self._window:
                q.popleft()
            if len(q) >= self._max:
                return False
            q.append(now)
            return True


_rate_limiter = RateLimiter()

WIDTH_LIMITS = {"min": 240, "max": 1200}
HEIGHT_LIMITS = {"min": 240, "max": 1200}
DURATION_LIMITS = {"min": 80, "max": 2500}
PADDING_LIMITS = {"min": 0, "max": 60}
BORDER_LIMITS = {"min": 0, "max": 24}
OUTLINE_LIMITS = {"min": 0, "max": 10}
SHADOW_LIMITS = {"min": 0, "max": 10}
FONT_SIZE_PX_LIMITS = {"min": 8, "max": 240}

TEMPLATE_CATALOG = [
    {
        "id": "two_panel",
        "name": "Two Panel",
        "description": "Classic split-screen for before-and-after or expectation-vs-reality.",
        "bestFor": "Pairing two ideas with a fast payoff.",
        "previewUrl": "/assets/templates/two_panel.png",
        "category": "canvas",
    },
    {
        "id": "three_panel",
        "name": "Three Panel",
        "description": "Three beats with escalating energy.",
        "bestFor": "Setups that need a beginning, middle, and spike.",
        "previewUrl": "/assets/templates/three_panel.png",
        "category": "canvas",
    },
    {
        "id": "bottom_text",
        "name": "Bottom Text",
        "description": "Poster-style with a heavy caption block.",
        "bestFor": "One-liners, announcements, and dramatic reveals.",
        "previewUrl": "/assets/templates/bottom_text.png",
        "category": "canvas",
    },
    {
        "id": "blank",
        "name": "Blank",
        "description": "Plain white canvas.",
        "bestFor": "Simple text-led stills and punchy cut-ins.",
        "previewUrl": "/assets/templates/blank.png",
        "category": "canvas",
    },
    {
        "id": "dark",
        "name": "Dark",
        "description": "Dark background for neon and high-contrast text.",
        "bestFor": "Night-mode cards, neon captions, dramatic overlays.",
        "previewUrl": "/assets/templates/dark.png",
        "category": "canvas",
    },
    {
        "id": "wide",
        "name": "Wide (16:9)",
        "description": "Landscape format for thumbnails and banners.",
        "bestFor": "Video covers and wide-format studio cards.",
        "previewUrl": "/assets/templates/wide.png",
        "category": "canvas",
    },
    {
        "id": "tall",
        "name": "Tall (9:16)",
        "description": "Portrait format for stories and reels.",
        "bestFor": "Storyboards, vertical promos, and mobile-first ideas.",
        "previewUrl": "/assets/templates/tall.png",
        "category": "canvas",
    },
    {
        "id": "square",
        "name": "Square (1:1)",
        "description": "Square format for social posts.",
        "bestFor": "Feed cards and balanced editorial compositions.",
        "previewUrl": "/assets/templates/square.png",
        "category": "canvas",
    },
    {
        "id": "four_panel",
        "name": "Four Panel",
        "description": "2x2 grid with dividers.",
        "bestFor": "Comparison sets and multi-beat punchlines.",
        "previewUrl": "/assets/templates/four_panel.png",
        "category": "canvas",
    },
    {
        "id": "caption_bar",
        "name": "Caption Bar",
        "description": "Image-heavy composition with a clean caption zone.",
        "bestFor": "Poster-like cards and tidy social callouts.",
        "previewUrl": "/assets/templates/caption_bar.png",
        "category": "canvas",
    },
]

MEME_TEMPLATE_DIR = ROOT_DIR / "assets" / "templates" / "meme"
if MEME_TEMPLATE_DIR.is_dir():
    for img in sorted(MEME_TEMPLATE_DIR.iterdir()):
        if img.suffix.lower() in {".jpg", ".jpeg", ".png", ".gif"}:
            TEMPLATE_CATALOG.append(
                {
                    "id": f"meme.{img.stem}",
                    "name": img.stem.replace("_", " ").title(),
                    "description": f"Meme template: {img.stem}",
                    "bestFor": "Reaction shots and recognizable meme beats.",
                    "previewUrl": f"/assets/templates/meme/{img.name}",
                    "category": "meme",
                }
            )

TEMPLATE_IDS = {template["id"] for template in TEMPLATE_CATALOG}

LAYOUT_CATALOG = [
    {
        "id": "single",
        "name": "Single",
        "slotCount": 1,
        "description": "One full-canvas scene.",
    },
    {
        "id": "beside",
        "name": "Beside",
        "slotCount": 2,
        "description": "Two slots side by side.",
    },
    {
        "id": "stack",
        "name": "Stack",
        "slotCount": 2,
        "description": "Two slots stacked vertically.",
    },
    {
        "id": "grid2x2",
        "name": "Grid 2x2",
        "slotCount": 4,
        "description": "Four slots in a 2x2 comparison grid.",
    },
]
LAYOUT_LOOKUP = {layout["id"]: layout for layout in LAYOUT_CATALOG}

EFFECT_CATALOG = [
    {"id": "none", "name": "None", "description": "Leave the scene clean.", "expression": None},
    {"id": "sepia", "name": "Sepia", "description": "Warm sepia tone.", "expression": "sepia"},
    {"id": "vintage", "name": "Vintage", "description": "Sepia with reduced brightness.", "expression": "sepia >> brightness(0.9)"},
    {"id": "glitch", "name": "Glitch", "description": "Pixelation, contrast, and noise.", "expression": "pixelate(4) >> contrast(1.8) >> noise(0.2)"},
    {"id": "deepfry", "name": "Deep Fry", "description": "High saturation and compression chaos.", "expression": "saturate(3.0) >> contrast(2.0) >> jpeg(10) >> noise(0.1)"},
    {"id": "cyberpunk", "name": "Cyberpunk", "description": "Hue shift, chromatic aberration, glow.", "expression": "hueShift(180) >> contrast(1.5) >> chromatic(3) >> glow(4)"},
    {"id": "comic", "name": "Comic", "description": "Posterize, contrast, sharpen.", "expression": "posterize(5) >> contrast(1.4) >> sharpen"},
    {"id": "retro", "name": "Retro", "description": "Grayscale with texture and contrast.", "expression": "grayscale >> contrast(1.3) >> noise(0.1)"},
    {"id": "grayscale", "name": "Grayscale", "description": "Convert to grayscale.", "expression": "grayscale"},
    {"id": "invert", "name": "Invert", "description": "Invert all colors.", "expression": "invert"},
    {"id": "vignette", "name": "Vignette", "description": "Darken the outer edges.", "expression": "vignette"},
    {"id": "sharpen", "name": "Sharpen", "description": "Sharpen the scene.", "expression": "sharpen"},
]
EFFECT_LOOKUP = {effect["id"]: effect for effect in EFFECT_CATALOG}

STYLE_PRESET_CATALOG = [
    {
        "id": "cinematic",
        "name": "Cinematic",
        "description": "Soft white type with a restrained shadow.",
        "style": {
            "color": "#FFFFFF",
            "outline": 2,
            "outlineColor": "#111111",
            "shadow": 4,
            "shadowColor": "#00000088",
            "fontSize": "md",
        },
    },
    {
        "id": "panic",
        "name": "Panic",
        "description": "Red alert styling with heavier edges.",
        "style": {
            "color": "#FF0000",
            "outline": 5,
            "outlineColor": "#440000",
            "shadow": 3,
            "shadowColor": "#00000099",
            "fontSize": "lg",
        },
    },
    {
        "id": "chill",
        "name": "Chill",
        "description": "Terminal green with crisp outline.",
        "style": {
            "color": "#00FF41",
            "outline": 3,
            "outlineColor": "#003300",
            "fontSize": "md",
        },
    },
    {
        "id": "shout",
        "name": "Shout",
        "description": "Loud white all-caps energy.",
        "style": {
            "color": "#FFFFFF",
            "outline": 4,
            "outlineColor": "#000000",
            "shadow": 3,
            "shadowColor": "#00000099",
            "fontSize": "xlg",
        },
    },
    {
        "id": "whisper",
        "name": "Whisper",
        "description": "Muted grey with a fine outline.",
        "style": {
            "color": "#CCCCCC",
            "outline": 1,
            "outlineColor": "#333333",
            "fontSize": "sm",
        },
    },
]
STYLE_PRESET_LOOKUP = {preset["id"]: preset for preset in STYLE_PRESET_CATALOG}

DEFAULT_STYLE = {
    "color": "#FFFFFF",
    "outline": 3,
    "outlineColor": "#000000",
    "shadow": 0,
    "shadowColor": "#00000080",
    "fontSize": "auto",
}
FONT_SIZE_MODES = {"auto", "sm", "md", "lg", "xlg", "custom"}

DISPLAY_OUTPUT_NAME = {
    "png": "studio-output.png",
    "gif": "studio-output.gif",
}


def clamp_int(value: object, minimum: int, maximum: int, fallback: int) -> int:
    try:
        parsed = int(value)
    except (TypeError, ValueError):
        parsed = fallback
    return max(minimum, min(maximum, parsed))


def normalize_caption(value: object, limit: int = 120) -> str:
    text = str(value or "")
    text = text.replace("\r\n", "\n").replace("\r", "\n")
    text = text.replace("\\", "\\\\")
    text = text.replace('"', "'")
    text = re.sub(r"\s+", " ", text).strip()
    return text[:limit]


def normalize_hex_color(value: object, fallback: str, allow_alpha: bool = False) -> str:
    text = str(value or "").strip().upper()
    pattern = r"^#[0-9A-F]{6}$" if not allow_alpha else r"^#[0-9A-F]{6}([0-9A-F]{2})?$"
    if re.fullmatch(pattern, text):
        return text
    return fallback


def mac_string_literal(text: str) -> str:
    return f'"{text}"'


def style_signature(resolved: dict[str, object]) -> tuple[object, ...]:
    return (
        resolved["color"],
        resolved["outline"],
        resolved["outlineColor"],
        resolved["shadow"],
        resolved["shadowColor"],
        resolved["fontSize"],
    )


def slot_count_for_layout(kind: str) -> int:
    return int(LAYOUT_LOOKUP[kind]["slotCount"])


def normalize_font_size(style_payload: dict[str, object], base_style: dict[str, object]) -> object:
    mode = str(style_payload.get("fontSizeMode") or "").strip().lower()
    base_font_size = base_style["fontSize"]

    if mode not in FONT_SIZE_MODES:
        if isinstance(base_font_size, str) and base_font_size in FONT_SIZE_MODES:
            mode = "custom" if base_font_size not in {"auto", "sm", "md", "lg", "xlg"} else base_font_size
        elif isinstance(base_font_size, int):
            mode = "custom"
        else:
            mode = "auto"

    if mode == "custom":
        default_px = base_font_size if isinstance(base_font_size, int) else 64
        return clamp_int(
            style_payload.get("fontSizePx"),
            FONT_SIZE_PX_LIMITS["min"],
            FONT_SIZE_PX_LIMITS["max"],
            int(default_px),
        )

    if mode == "auto":
        return "auto"
    return mode


def normalize_style(raw_style: object) -> dict[str, object]:
    style_payload = raw_style if isinstance(raw_style, dict) else {}
    preset_id = str(style_payload.get("preset") or "").strip()
    preset = STYLE_PRESET_LOOKUP.get(preset_id)
    base_style = dict(DEFAULT_STYLE)
    if preset:
        base_style.update(preset["style"])

    resolved = {
        "color": normalize_hex_color(style_payload.get("color"), str(base_style["color"])),
        "outline": clamp_int(style_payload.get("outline"), OUTLINE_LIMITS["min"], OUTLINE_LIMITS["max"], int(base_style["outline"])),
        "outlineColor": normalize_hex_color(style_payload.get("outlineColor"), str(base_style["outlineColor"])),
        "shadow": clamp_int(style_payload.get("shadow"), SHADOW_LIMITS["min"], SHADOW_LIMITS["max"], int(base_style["shadow"])),
        "shadowColor": normalize_hex_color(style_payload.get("shadowColor"), str(base_style["shadowColor"]), allow_alpha=True),
        "fontSize": normalize_font_size(style_payload, base_style),
    }

    return {
        "preset": preset_id if preset else "",
        "color": resolved["color"],
        "outline": resolved["outline"],
        "outlineColor": resolved["outlineColor"],
        "shadow": resolved["shadow"],
        "shadowColor": resolved["shadowColor"],
        "fontSize": resolved["fontSize"],
        "resolved": resolved,
    }


def normalize_slot(raw_slot: object) -> dict[str, object]:
    if not isinstance(raw_slot, dict):
        raise ValueError("Each slot must be an object.")

    template_id = str(raw_slot.get("templateId") or "blank").strip()
    if template_id not in TEMPLATE_IDS and not template_id.startswith("user."):
        raise ValueError("Unknown template selected.")

    text_payload = raw_slot.get("text")
    text_payload = text_payload if isinstance(text_payload, dict) else {}

    return {
        "templateId": template_id,
        "text": {
            "top": normalize_caption(text_payload.get("top")),
            "center": normalize_caption(text_payload.get("center")),
            "bottom": normalize_caption(text_payload.get("bottom")),
        },
        "style": normalize_style(raw_slot.get("style")),
    }


def normalize_scene(raw_scene: object) -> dict[str, object]:
    if not isinstance(raw_scene, dict):
        raise ValueError("Each scene must be an object.")

    layout_payload = raw_scene.get("layout")
    layout_payload = layout_payload if isinstance(layout_payload, dict) else {}
    layout_kind = str(layout_payload.get("kind") or "single").strip()
    if layout_kind not in LAYOUT_LOOKUP:
        raise ValueError("Unknown layout selected.")

    effect_id = str(layout_payload.get("effect") or "none").strip()
    if effect_id not in EFFECT_LOOKUP:
        raise ValueError("Unknown effect selected.")

    raw_slots = raw_scene.get("slots")
    if not isinstance(raw_slots, list):
        raise ValueError("Each scene must include a slots array.")

    expected_slots = slot_count_for_layout(layout_kind)
    if len(raw_slots) != expected_slots:
        raise ValueError(f"Layout '{layout_kind}' expects {expected_slots} slot(s).")

    slots = [normalize_slot(slot) for slot in raw_slots]
    duration = clamp_int(
        raw_scene.get("durationMs"),
        DURATION_LIMITS["min"],
        DURATION_LIMITS["max"],
        400,
    )

    transition = None
    raw_trans = raw_scene.get("transition")
    if isinstance(raw_trans, dict) and raw_trans.get("type"):
        valid_types = {"crossfade", "slideLeft", "slideRight", "slideUp", "slideDown", "wipe", "fadeBlack", "zoom"}
        valid_easings = {"linear", "ease", "easeIn", "easeOut", "easeInOut"}
        t_type = str(raw_trans["type"]).strip()
        if t_type in valid_types:
            transition = {
                "type": t_type,
                "durationMs": clamp_int(raw_trans.get("durationMs"), 50, 1000, 150),
                "easing": str(raw_trans.get("easing", "linear")).strip() if str(raw_trans.get("easing", "")).strip() in valid_easings else "linear",
            }

    return {
        "durationMs": duration,
        "layout": {
            "kind": layout_kind,
            "padding": clamp_int(layout_payload.get("padding"), PADDING_LIMITS["min"], PADDING_LIMITS["max"], 0),
            "border": clamp_int(layout_payload.get("border"), BORDER_LIMITS["min"], BORDER_LIMITS["max"], 0),
            "effect": effect_id,
        },
        "slots": slots,
        "transition": transition,
    }


def normalize_document(payload: dict[str, object]) -> dict[str, object]:
    canvas_payload = payload.get("canvas")
    canvas_payload = canvas_payload if isinstance(canvas_payload, dict) else {}
    scenes_payload = payload.get("scenes")
    if not isinstance(scenes_payload, list) or not scenes_payload:
        raise ValueError("At least one scene is required.")
    if len(scenes_payload) > MAX_SCENE_COUNT:
        raise ValueError(f"Scene count exceeds the limit of {MAX_SCENE_COUNT}.")

    scenes = [normalize_scene(scene) for scene in scenes_payload]

    output_payload = payload.get("output")
    output_payload = output_payload if isinstance(output_payload, dict) else {}
    output_format = str(output_payload.get("format") or ("gif" if len(scenes) > 1 else "png")).strip().lower()
    if output_format not in {"png", "gif"}:
        raise ValueError("Output format must be png or gif.")
    if output_format == "png" and len(scenes) > 1:
        raise ValueError("PNG export only supports a single scene.")

    preview_scene_index = payload.get("previewSceneIndex")
    if preview_scene_index is None:
        preview_index = None
    else:
        preview_index = clamp_int(preview_scene_index, 0, len(scenes) - 1, 0)

    return {
        "canvas": {
            "width": clamp_int(canvas_payload.get("width"), WIDTH_LIMITS["min"], WIDTH_LIMITS["max"], 640),
            "height": clamp_int(canvas_payload.get("height"), HEIGHT_LIMITS["min"], HEIGHT_LIMITS["max"], 640),
        },
        "output": {"format": output_format},
        "scenes": scenes,
        "previewSceneIndex": preview_index,
    }


def slot_dimensions(canvas: dict[str, int], layout_kind: str) -> tuple[int, int]:
    width = int(canvas["width"])
    height = int(canvas["height"])
    if layout_kind == "beside":
        return max(1, width // 2), height
    if layout_kind == "stack":
        return width, max(1, height // 2)
    if layout_kind == "grid2x2":
        return max(1, width // 2), max(1, height // 2)
    return width, height


def render_style_props(style: dict[str, object], full: bool = False) -> list[str]:
    resolved = style["resolved"] if "resolved" in style else style
    lines: list[str] = []

    if full or resolved["color"] != DEFAULT_STYLE["color"]:
        lines.append(f'    color: {mac_string_literal(str(resolved["color"]))}')
    if full or resolved["outline"] != DEFAULT_STYLE["outline"]:
        lines.append(f"    outline: {int(resolved['outline'])}")
    if full or resolved["outlineColor"] != DEFAULT_STYLE["outlineColor"]:
        lines.append(f'    outlineColor: {mac_string_literal(str(resolved["outlineColor"]))}')
    if full or resolved["shadow"] != DEFAULT_STYLE["shadow"]:
        lines.append(f"    shadow: {int(resolved['shadow'])}")
    if full or (
        int(resolved["shadow"]) > 0 and resolved["shadowColor"] != DEFAULT_STYLE["shadowColor"]
    ):
        lines.append(f'    shadowColor: {mac_string_literal(str(resolved["shadowColor"]))}')
    if full or resolved["fontSize"] != DEFAULT_STYLE["fontSize"]:
        font_size = resolved["fontSize"]
        if isinstance(font_size, int):
            lines.append(f"    fontSize: {font_size}")
        else:
            lines.append(f'    fontSize: {mac_string_literal(str(font_size))}')

    return lines


def render_style_block(name: str, style: dict[str, object], full: bool = False) -> str:
    props = render_style_props(style, full=full)
    if not props:
        return ""
    return "\n".join([f"style {name} {{", *props, "}"])


def build_slot_expr(slot: dict[str, object], scene_index: int, slot_index: int, canvas: dict[str, int], layout_kind: str,
                    used_presets: list[str], custom_style_defs: list[str], custom_style_cache: dict[tuple[object, ...], str]) -> str:
    slot_width, slot_height = slot_dimensions(canvas, layout_kind)
    style = slot["style"]
    resolved = style["resolved"]
    style_ref = ""

    if style["preset"]:
        preset = STYLE_PRESET_LOOKUP[style["preset"]]
        preset_resolved = normalize_style({"preset": preset["id"]})["resolved"]
        if style_signature(resolved) == style_signature(preset_resolved):
            style_ref = preset["id"]
            if preset["id"] not in used_presets:
                used_presets.append(preset["id"])

    if not style_ref and style_signature(resolved) != style_signature(DEFAULT_STYLE):
        key = style_signature(resolved)
        style_ref = custom_style_cache.get(key, "")
        if not style_ref:
            style_ref = f"scene_{scene_index + 1}_slot_{slot_index + 1}_style"
            block = render_style_block(style_ref, style)
            if block:
                custom_style_defs.append(block)
            custom_style_cache[key] = style_ref

    style_suffix = f" {style_ref}" if style_ref else ""
    entries = []
    for position in ("top", "center", "bottom"):
        value = slot["text"][position]
        if value:
            entries.append(f"    {position}: {mac_string_literal(value)}")

    # User uploads use @"absolute/path" syntax; built-in templates use @identifier
    tid = slot["templateId"]
    if tid.startswith("user."):
        # Resolve to absolute path
        stem = tid.removeprefix("user.")
        upload_path = None
        if UPLOADS_DIR.is_dir():
            for f in UPLOADS_DIR.iterdir():
                if f.stem == stem:
                    upload_path = str(f.resolve())
                    break
        tmpl_ref = f'@"{upload_path or tid}"' if upload_path else f"@{tid}"
    else:
        tmpl_ref = f"@{tid}"

    if not entries:
        return f"{tmpl_ref} {slot_width}x{slot_height}{style_suffix} {{}}"

    return "\n".join(
        [
            f"{tmpl_ref} {slot_width}x{slot_height}{style_suffix} {{",
            *entries,
            "}",
        ]
    )


def layout_expression(layout_kind: str, slot_var_names: list[str]) -> str:
    if layout_kind == "single":
        return slot_var_names[0]
    if layout_kind == "beside":
        return f"beside({slot_var_names[0]}, {slot_var_names[1]})"
    if layout_kind == "stack":
        return f"stack({slot_var_names[0]}, {slot_var_names[1]})"
    if layout_kind == "grid2x2":
        return "\n".join(
            [
                "grid 2x2 {",
                *(f"    {name}" for name in slot_var_names),
                "}",
            ]
        )
    raise ValueError(f"Unsupported layout kind: {layout_kind}")


def _animation_block(scenes: list[dict], save_target: str) -> list[str]:
    """Generate gif loop { } or timeline loop { } lines depending on transitions."""
    has_transitions = any(
        scene.get("transition") and scene["transition"].get("type") not in (None, "cut")
        for scene in scenes
    )
    lines: list[str] = []
    block_type = "timeline" if has_transitions else "gif"
    lines.append(f"{block_type} loop {{")
    for i, scene in enumerate(scenes):
        trans = scene.get("transition") if i > 0 else None
        if trans and trans.get("type") and trans["type"] != "cut" and has_transitions:
            t_type = trans["type"]
            t_dur = int(trans.get("durationMs", 150))
            t_ease = trans.get("easing", "linear")
            ease_part = f" {t_ease}" if t_ease != "linear" else ""
            lines.append(f"    --- {t_type} {t_dur}ms{ease_part} ---")
        lines.append(f"    scene_{i + 1} : {scene['durationMs']}ms")
    lines.append(f"}} => {save_target};")
    return lines


def build_script_bundle(document: dict[str, object], output_path: Path, preview_scene_index: int | None = None) -> dict[str, str]:
    used_presets: list[str] = []
    custom_style_defs: list[str] = []
    custom_style_cache: dict[tuple[object, ...], str] = {}
    effect_defs: list[str] = []
    effect_cache: dict[str, str] = {}
    scene_defs: list[str] = []

    for scene_index, scene in enumerate(document["scenes"]):
        slot_var_names: list[str] = []
        scene_lines: list[str] = []
        layout_kind = str(scene["layout"]["kind"])

        for slot_index, slot in enumerate(scene["slots"]):
            slot_var_name = f"scene_{scene_index + 1}_slot_{slot_index + 1}"
            slot_expr = build_slot_expr(
                slot,
                scene_index,
                slot_index,
                document["canvas"],
                layout_kind,
                used_presets,
                custom_style_defs,
                custom_style_cache,
            )
            scene_lines.append(f"var {slot_var_name} = {slot_expr};")
            slot_var_names.append(slot_var_name)

        scene_expr = layout_expression(layout_kind, slot_var_names)

        padding = int(scene["layout"]["padding"])
        border = int(scene["layout"]["border"])
        effect_id = str(scene["layout"]["effect"])
        pipes: list[str] = []

        if padding > 0:
            pipes.append(f"pad({padding})")
        if border > 0:
            pipes.append(f"border({border})")
        if effect_id != "none":
            effect_ref = effect_cache.get(effect_id, "")
            if not effect_ref:
                effect_ref = f"fx_{effect_id}"
                effect_defs.append(f"effect {effect_ref} = {EFFECT_LOOKUP[effect_id]['expression']};")
                effect_cache[effect_id] = effect_ref
            pipes.append(effect_ref)

        if pipes:
            scene_expr = scene_expr + "".join(f" |> {pipe}" for pipe in pipes)

        scene_lines.append(f"var scene_{scene_index + 1} = {scene_expr};")
        scene_defs.append("\n".join(scene_lines))

    sections: list[str] = []
    for preset_id in used_presets:
        block = render_style_block(preset_id, normalize_style({"preset": preset_id}))
        if block:
            sections.append(block)
    sections.extend(custom_style_defs)
    sections.extend(effect_defs)
    sections.extend(scene_defs)

    export_lines: list[str] = []
    display_output_name = DISPLAY_OUTPUT_NAME[str(document["output"]["format"])]

    if document["output"]["format"] == "png":
        export_lines.append(f'scene_1 => "{display_output_name}";')
    else:
        export_lines.extend(_animation_block(document["scenes"], f'"{display_output_name}"'))

    document_script = "\n\n".join([*sections, "\n".join(export_lines)]).strip() + "\n"

    run_output_path = output_path.as_posix()
    run_export_lines: list[str] = []
    if preview_scene_index is not None:
        run_export_lines.append(f'scene_{preview_scene_index + 1} => "{run_output_path}";')
    elif document["output"]["format"] == "png":
        run_export_lines.append(f'scene_1 => "{run_output_path}";')
    else:
        run_export_lines.extend(_animation_block(document["scenes"], f'"{run_output_path}"'))

    run_script = "\n\n".join([*sections, "\n".join(run_export_lines)]).strip() + "\n"

    return {
        "documentScript": document_script,
        "runScript": run_script,
    }


def cleanup_mac_temp_files() -> None:
    """Remove /tmp/mac_effects_* dirs for dead processes only."""
    import glob
    import os
    import shutil
    for d in glob.glob("/tmp/mac_effects_*"):
        try:
            pid = int(d.rsplit("_", 1)[-1])
            # Only delete if the process no longer exists
            try:
                os.kill(pid, 0)
            except OSError:
                shutil.rmtree(d, ignore_errors=True)
        except (ValueError, Exception):
            pass


ALLOWED_IMAGE_EXTS = {".png", ".jpg", ".jpeg", ".gif", ".webp", ".bmp", ".tiff", ".tif", ".svg", ".ico", ".heic", ".heif", ".avif"}


def handle_upload(handler: BaseHTTPRequestHandler) -> dict[str, object]:
    """Accept an image upload, save to uploads/, return template info."""
    content_length = int(handler.headers.get("Content-Length", 0))
    if content_length <= 0:
        raise ValueError("Upload body is empty.")
    if content_length > MAX_UPLOAD_BYTES:
        raise ValueError(f"File too large. Maximum is {MAX_UPLOAD_BYTES // (1024 * 1024)} MB.")

    content_type = handler.headers.get("Content-Type", "")
    if "multipart/form-data" not in content_type:
        raise ValueError("Expected multipart/form-data upload.")

    raw = handler.rfile.read(content_length)

    # Parse the multipart boundary
    boundary = content_type.split("boundary=")[-1].strip()
    parts = raw.split(f"--{boundary}".encode())

    file_data = None
    filename = "upload.png"
    for part in parts:
        if b"Content-Disposition" not in part:
            continue
        header_end = part.find(b"\r\n\r\n")
        if header_end < 0:
            continue
        headers = part[:header_end].decode("utf-8", errors="replace")
        body = part[header_end + 4:]
        if body.endswith(b"\r\n"):
            body = body[:-2]

        if 'name="image"' not in headers and 'name="file"' not in headers:
            continue
        file_data = body
        # Extract filename via regex
        fn_match = re.search(r'filename="([^"]*)"', headers)
        if fn_match and fn_match.group(1):
            filename = fn_match.group(1)
        break

    if not file_data or len(file_data) < 100:
        raise ValueError("No image file found in the upload.")

    # Detect extension from magic bytes if filename has none
    ext = Path(filename).suffix.lower()
    if not ext or ext not in ALLOWED_IMAGE_EXTS:
        if file_data[:8].startswith(b"\x89PNG"):
            ext = ".png"
        elif file_data[:3] in (b"\xff\xd8\xff", b"\xff\xd8"):
            ext = ".jpg"
        elif file_data[:6] in (b"GIF87a", b"GIF89a"):
            ext = ".gif"
        elif file_data[:4] == b"RIFF" and file_data[8:12] == b"WEBP":
            ext = ".webp"
        else:
            ext = ".png"
        filename = Path(filename).stem + ext

    ext = Path(filename).suffix.lower()
    if ext not in ALLOWED_IMAGE_EXTS:
        raise ValueError(f"Unsupported format '{ext}'. Use PNG, JPG, GIF, or WebP.")

    # Save with unique name
    safe_name = f"upload-{uuid.uuid4().hex[:10]}{ext}"
    UPLOADS_DIR.mkdir(parents=True, exist_ok=True)
    dest = UPLOADS_DIR / safe_name
    dest.write_bytes(file_data)

    cleanup_uploads_dir()

    template_id = f"user.{dest.stem}"
    return {
        "ok": True,
        "templateId": template_id,
        "name": Path(filename).stem.replace("_", " ").title(),
        "previewUrl": f"/uploads/{safe_name}",
    }


def get_uploaded_templates() -> list[dict[str, str]]:
    """Return uploaded images as template catalog entries."""
    if not UPLOADS_DIR.is_dir():
        return []
    templates = []
    for img in sorted(UPLOADS_DIR.iterdir()):
        if img.suffix.lower() not in ALLOWED_IMAGE_EXTS:
            continue
        templates.append({
            "id": f"user.{img.stem}",
            "name": img.stem.replace("upload-", "").replace("_", " ").title(),
            "description": "User uploaded image",
            "bestFor": "Custom templates and personal images.",
            "previewUrl": f"/uploads/{img.name}",
            "category": "uploads",
        })
    return templates


def cleanup_uploads_dir() -> None:
    """Keep only the newest MAX_UPLOADS files."""
    if not UPLOADS_DIR.is_dir():
        return
    files = []
    for p in UPLOADS_DIR.iterdir():
        if not p.is_file():
            continue
        try:
            files.append((p.stat().st_mtime, p))
        except FileNotFoundError:
            continue
    if len(files) <= MAX_UPLOADS:
        return
    files.sort(key=lambda item: item[0], reverse=True)
    for _, old in files[MAX_UPLOADS:]:
        old.unlink(missing_ok=True)


def cleanup_generated_dir() -> None:
    GENERATED_DIR.mkdir(parents=True, exist_ok=True)
    now = time.time()
    files = []
    for path in GENERATED_DIR.iterdir():
        if not path.is_file() or path.name == ".gitkeep":
            continue
        try:
            stat = path.stat()
        except FileNotFoundError:
            continue
        if now - stat.st_mtime > MAX_FILE_AGE_SECONDS:
            path.unlink(missing_ok=True)
            continue
        files.append((stat.st_mtime, path))

    if len(files) <= MAX_GENERATED_FILES:
        return

    files.sort(key=lambda item: item[0], reverse=True)
    for _, old_path in files[MAX_GENERATED_FILES:]:
        old_path.unlink(missing_ok=True)


def json_response(
    handler: BaseHTTPRequestHandler,
    payload: dict[str, object],
    status: HTTPStatus = HTTPStatus.OK,
    send_body: bool = True,
) -> None:
    body = json.dumps(payload).encode("utf-8")
    handler.send_response(status)
    handler.send_header("Content-Type", "application/json; charset=utf-8")
    handler.send_header("Content-Length", str(len(body)))
    handler.send_header("Cache-Control", "no-store")
    handler.end_headers()
    if send_body:
        handler.wfile.write(body)


def read_json_body(handler: BaseHTTPRequestHandler) -> dict[str, object]:
    content_length = clamp_int(handler.headers.get("Content-Length"), 0, MAX_REQUEST_BYTES, 0)
    if content_length <= 0:
        raise ValueError("Request body is empty.")
    raw_body = handler.rfile.read(content_length)
    if len(raw_body) != content_length:
        raise ValueError("Request body is incomplete.")
    try:
        parsed = json.loads(raw_body)
    except json.JSONDecodeError as exc:
        raise ValueError("Request body must be valid JSON.") from exc
    if not isinstance(parsed, dict):
        raise ValueError("Top-level JSON payload must be an object.")
    return parsed


def serve_file(
    handler: BaseHTTPRequestHandler,
    file_path: Path,
    cache_control: str = "public, max-age=300",
    send_body: bool = True,
) -> None:
    if not file_path.exists() or not file_path.is_file():
        handler.send_error(HTTPStatus.NOT_FOUND, "File not found.")
        return

    content_type = mimetypes.guess_type(file_path.name)[0] or "application/octet-stream"
    data = file_path.read_bytes()
    handler.send_response(HTTPStatus.OK)
    handler.send_header("Content-Type", content_type)
    handler.send_header("Content-Length", str(len(data)))
    handler.send_header("Cache-Control", cache_control)
    handler.end_headers()
    if send_body:
        handler.wfile.write(data)


def safe_child_path(base: Path, requested_path: str) -> Path | None:
    try:
        candidate = (base / requested_path).resolve()
        candidate.relative_to(base.resolve())
    except (ValueError, RuntimeError):
        return None
    return candidate


def run_raw_script(script: str) -> dict[str, object]:
    """Execute a raw Mac script and return the generated artifact."""
    # Determine output format from the script's save expression
    output_format = "gif" if ".gif" in script else "png"
    output_name = f"raw-{uuid.uuid4().hex[:12]}.{output_format}"
    output_path = GENERATED_DIR / output_name

    # Rewrite => "..." save targets to point to our generated dir
    patched = re.sub(
        r'=>\s*"[^"]*"',
        f'=> "{output_path}"',
        script,
    )

    with tempfile.TemporaryDirectory(prefix="mac-raw-") as temp_dir:
        script_path = Path(temp_dir) / "raw.mac"
        script_path.write_text(patched, encoding="utf-8")

        result = subprocess.run(
            [str(MAC_BINARY), str(script_path)],
            cwd=ROOT_DIR,
            capture_output=True,
            text=True,
            timeout=20,
            check=False,
        )

    cleanup_mac_temp_files()

    if result.returncode != 0 or not output_path.exists():
        stderr = (result.stderr or result.stdout or "Unknown error").strip()
        raise RuntimeError(stderr or "Raw script execution failed.")

    file_size = output_path.stat().st_size if output_path.exists() else 0

    return {
        "ok": True,
        "previewUrl": f"/generated/{output_name}",
        "downloadUrl": f"/generated/{output_name}",
        "script": script,
        "summary": {
            "format": output_format,
            "sceneCount": 1,
            "frameCount": 1,
            "durationMs": 0,
            "width": 0,
            "height": 0,
            "fileSizeBytes": file_size,
        },
    }


def build_render_response(payload: dict[str, object]) -> tuple[dict[str, object], Path]:
    document = normalize_document(payload)
    preview_scene_index = document["previewSceneIndex"]
    output_format = "png" if preview_scene_index is not None else str(document["output"]["format"])

    output_name = f"{'preview' if preview_scene_index is not None else 'artifact'}-{uuid.uuid4().hex[:12]}.{output_format}"
    output_path = GENERATED_DIR / output_name

    script_bundle = build_script_bundle(document, output_path, preview_scene_index=preview_scene_index)

    with tempfile.TemporaryDirectory(prefix="mac-studio-") as temp_dir:
        script_path = Path(temp_dir) / "request.mac"
        script_path.write_text(script_bundle["runScript"], encoding="utf-8")

        result = subprocess.run(
            [str(MAC_BINARY), str(script_path)],
            cwd=ROOT_DIR,
            capture_output=True,
            text=True,
            timeout=20,
            check=False,
        )

    cleanup_mac_temp_files()

    if result.returncode != 0 or not output_path.exists():
        stderr = (result.stderr or result.stdout or "Unknown error").strip()
        raise RuntimeError(stderr or "Studio render failed.")

    duration_ms = sum(int(scene["durationMs"]) for scene in document["scenes"])
    summary_format = output_format
    summary_frame_count = 1 if output_format == "png" else len(document["scenes"])

    response = {
        "ok": True,
        "previewUrl": f"/generated/{output_name}",
        "downloadUrl": f"/generated/{output_name}",
        "script": script_bundle["documentScript"],
        "summary": {
            "format": summary_format,
            "sceneCount": len(document["scenes"]),
            "frameCount": summary_frame_count,
            "durationMs": duration_ms,
            "width": document["canvas"]["width"],
            "height": document["canvas"]["height"],
            "fileSizeBytes": output_path.stat().st_size,
        },
    }
    return response, output_path


class GifStudioHandler(BaseHTTPRequestHandler):
    server_version = "MacStudio/2.0"

    def do_GET(self) -> None:
        self.handle_get_request(send_body=True)

    def do_HEAD(self) -> None:
        self.handle_get_request(send_body=False)

    def handle_get_request(self, send_body: bool) -> None:
        parsed = urlparse(self.path)
        path = unquote(parsed.path)

        if path == "/api/templates":
            json_response(
                self,
                {
                    "templates": TEMPLATE_CATALOG + get_uploaded_templates(),
                    "effects": [
                        {key: value for key, value in effect.items() if key != "expression"}
                        for effect in EFFECT_CATALOG
                    ],
                    "layouts": LAYOUT_CATALOG,
                    "stylePresets": STYLE_PRESET_CATALOG,
                    "limits": {
                        "sceneCount": MAX_SCENE_COUNT,
                        "width": WIDTH_LIMITS,
                        "height": HEIGHT_LIMITS,
                        "duration": DURATION_LIMITS,
                        "padding": PADDING_LIMITS,
                        "border": BORDER_LIMITS,
                        "outline": OUTLINE_LIMITS,
                        "shadow": SHADOW_LIMITS,
                        "fontSizePx": FONT_SIZE_PX_LIMITS,
                    },
                },
                send_body=send_body,
            )
            return

        if path == "/health":
            json_response(
                self,
                {
                    "ok": True,
                    "binaryReady": MAC_BINARY.exists(),
                    "generatedDir": str(GENERATED_DIR),
                },
                send_body=send_body,
            )
            return

        if path.startswith("/generated/"):
            relative_path = path.removeprefix("/generated/").lstrip("/")
            candidate = safe_child_path(GENERATED_DIR, relative_path)
            if candidate is None:
                self.send_error(HTTPStatus.NOT_FOUND, "File not found.")
                return
            serve_file(self, candidate, cache_control="no-store", send_body=send_body)
            return

        if path.startswith("/uploads/"):
            relative_path = path.removeprefix("/uploads/").lstrip("/")
            candidate = safe_child_path(UPLOADS_DIR, relative_path)
            if candidate is None:
                self.send_error(HTTPStatus.NOT_FOUND, "File not found.")
                return
            serve_file(self, candidate, send_body=send_body)
            return

        if path.startswith("/assets/"):
            relative_path = path.removeprefix("/assets/").lstrip("/")
            candidate = safe_child_path(ROOT_DIR / "assets", relative_path)
            if candidate is None:
                self.send_error(HTTPStatus.NOT_FOUND, "File not found.")
                return
            serve_file(self, candidate, send_body=send_body)
            return

        static_rel = "index.html" if path in {"", "/"} else path.lstrip("/")
        candidate = safe_child_path(STATIC_DIR, static_rel)
        if candidate is None or not candidate.exists():
            candidate = STATIC_DIR / "index.html"
        serve_file(self, candidate, send_body=send_body)

    def do_POST(self) -> None:
        parsed = urlparse(self.path)
        path = unquote(parsed.path)

        if path == "/api/upload/delete":
            try:
                body = read_json_body(self)
                template_id = str(body.get("templateId", ""))
                if not template_id.startswith("user."):
                    raise ValueError("Can only delete user uploads.")
                stem = template_id.removeprefix("user.")
                deleted = False
                if UPLOADS_DIR.is_dir():
                    for f in UPLOADS_DIR.iterdir():
                        if f.stem == stem:
                            f.unlink(missing_ok=True)
                            deleted = True
                            break
                json_response(self, {"ok": True, "deleted": deleted})
            except ValueError as exc:
                json_response(self, {"error": str(exc)}, status=HTTPStatus.BAD_REQUEST)
            except Exception as exc:
                json_response(self, {"error": str(exc)}, status=HTTPStatus.INTERNAL_SERVER_ERROR)
            return

        if path == "/api/upload":
            try:
                client_ip = self.client_address[0]
                if not _rate_limiter.allow(client_ip):
                    json_response(self, {"error": "Too many requests."}, status=HTTPStatus.TOO_MANY_REQUESTS)
                    return
                result = handle_upload(self)
                json_response(self, result)
            except ValueError as exc:
                json_response(self, {"error": str(exc)}, status=HTTPStatus.BAD_REQUEST)
            except Exception as exc:
                json_response(self, {"error": str(exc)}, status=HTTPStatus.INTERNAL_SERVER_ERROR)
            return

        if path != "/api/generate":
            self.send_error(HTTPStatus.NOT_FOUND, "Route not found.")
            return

        client_ip = self.client_address[0]
        if not _rate_limiter.allow(client_ip):
            json_response(
                self,
                {"error": "Too many requests. Try again in a minute."},
                status=HTTPStatus.TOO_MANY_REQUESTS,
            )
            return

        if not MAC_BINARY.exists():
            json_response(
                self,
                {
                    "error": "The Mac binary is missing. Run `cmake -S . -B build && cmake --build build` first.",
                },
                status=HTTPStatus.SERVICE_UNAVAILABLE,
            )
            return

        try:
            payload = read_json_body(self)
            cleanup_generated_dir()
            raw_script = payload.get("rawScript")
            if raw_script and isinstance(raw_script, str) and raw_script.strip():
                response = run_raw_script(raw_script)
            else:
                response, _ = build_render_response(payload)
            json_response(self, response)
        except ValueError as exc:
            json_response(self, {"error": str(exc)}, status=HTTPStatus.BAD_REQUEST)
        except subprocess.TimeoutExpired:
            json_response(
                self,
                {"error": "Studio render timed out after 20 seconds."},
                status=HTTPStatus.GATEWAY_TIMEOUT,
            )
        except Exception as exc:  # noqa: BLE001
            json_response(
                self,
                {"error": str(exc)},
                status=HTTPStatus.INTERNAL_SERVER_ERROR,
            )

    def log_message(self, fmt: str, *args: object) -> None:
        print(f"[{self.log_date_time_string()}] {self.address_string()} {fmt % args}")


class FastThreadingHTTPServer(ThreadingHTTPServer):
    allow_reuse_address = True
    daemon_threads = True

    def server_bind(self) -> None:
        socketserver.TCPServer.server_bind(self)
        host, port = self.server_address[:2]
        self.server_name = host
        self.server_port = port


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Mac Studio web server")
    parser.add_argument("--host", default="0.0.0.0", help="Host interface to bind")
    parser.add_argument("--port", type=int, default=9001, help="Port to listen on")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    GENERATED_DIR.mkdir(parents=True, exist_ok=True)
    cleanup_generated_dir()

    server = FastThreadingHTTPServer((args.host, args.port), GifStudioHandler)
    print(f"Mac Studio running at http://{args.host}:{args.port}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
