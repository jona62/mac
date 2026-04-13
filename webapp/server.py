#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import mimetypes
import re
import socketserver
import subprocess
import tempfile
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
MAC_BINARY = ROOT_DIR / "build" / "mac"

MAX_REQUEST_BYTES = 256 * 1024
MAX_FRAME_COUNT = 24
MAX_FILE_AGE_SECONDS = 60 * 60 * 24
MAX_GENERATED_FILES = 200

TEMPLATE_CATALOG = [
    {
        "id": "two_panel",
        "name": "Two Panel",
        "description": "Classic split-screen rhythm for before-and-after, expectation-vs-reality, or Monday-to-Friday arcs.",
        "bestFor": "Pairing two ideas with a fast payoff.",
        "previewUrl": "/assets/templates/two_panel.png",
    },
    {
        "id": "three_panel",
        "name": "Three Panel",
        "description": "Three beats with escalating energy when you want the joke to land in stages.",
        "bestFor": "Setups that need a beginning, middle, and spike.",
        "previewUrl": "/assets/templates/three_panel.png",
    },
    {
        "id": "bottom_text",
        "name": "Bottom Text",
        "description": "Poster-style composition with a giant image and a heavy caption block.",
        "bestFor": "One-liners, announcements, and dramatic reveals.",
        "previewUrl": "/assets/templates/bottom_text.png",
    },
    {
        "id": "blank",
        "name": "Blank",
        "description": "Minimal empty canvas for countdowns, title cards, or stark graphic loops.",
        "bestFor": "Simple text-driven GIFs and punchy hard cuts.",
        "previewUrl": "/assets/templates/blank.png",
    },
]
TEMPLATE_IDS = {template["id"] for template in TEMPLATE_CATALOG}


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


def mac_string_literal(text: str) -> str:
    return f'"{text}"'


def build_script(payload: dict[str, object], output_path: Path) -> tuple[str, dict[str, object]]:
    template_id = str(payload.get("template") or "two_panel").strip()
    if template_id not in TEMPLATE_IDS:
        raise ValueError("Unknown template selected.")

    width = clamp_int(payload.get("width"), 240, 1200, 640)
    height = clamp_int(payload.get("height"), 240, 1200, 640)

    raw_frames = payload.get("frames")
    if not isinstance(raw_frames, list) or not raw_frames:
        raise ValueError("At least one frame is required.")

    if len(raw_frames) > MAX_FRAME_COUNT:
        raise ValueError(f"Frame count exceeds the limit of {MAX_FRAME_COUNT}.")

    normalized_frames: list[dict[str, object]] = []
    script_lines = [
        f"var template = Template({mac_string_literal(template_id)});",
        "var gif = Gif();",
        "",
    ]

    for raw_frame in raw_frames:
        if not isinstance(raw_frame, dict):
            raise ValueError("Each frame must be an object.")
        top = normalize_caption(raw_frame.get("top"))
        bottom = normalize_caption(raw_frame.get("bottom"))
        duration = clamp_int(raw_frame.get("duration"), 80, 2500, 400)
        normalized_frames.append(
            {
                "top": top,
                "bottom": bottom,
                "duration": duration,
            }
        )
        script_lines.append(
            "gif.frame("
            f"Meme(template).text(Top, {mac_string_literal(top)}).text(Bottom, {mac_string_literal(bottom)}).resize(Size({width}, {height})), "
            f"Duration({duration})"
            ");"
        )

    script_lines.extend(
        [
            "",
            f'gif.save("{output_path.as_posix()}");',
            'print "GIF saved.";'
        ]
    )

    return "\n".join(script_lines) + "\n", {
        "template": template_id,
        "width": width,
        "height": height,
        "frames": normalized_frames,
    }


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


class GifStudioHandler(BaseHTTPRequestHandler):
    server_version = "MacGifStudio/1.0"

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
                    "templates": TEMPLATE_CATALOG,
                    "limits": {
                        "frameCount": MAX_FRAME_COUNT,
                        "width": {"min": 240, "max": 1200},
                        "height": {"min": 240, "max": 1200},
                        "duration": {"min": 80, "max": 2500},
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

        if path != "/api/generate":
            self.send_error(HTTPStatus.NOT_FOUND, "Route not found.")
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

            output_name = f"gif-{uuid.uuid4().hex[:12]}.gif"
            output_path = GENERATED_DIR / output_name
            script_text, normalized_payload = build_script(payload, output_path)

            with tempfile.TemporaryDirectory(prefix="mac-gif-studio-") as temp_dir:
                script_path = Path(temp_dir) / "request.mac"
                script_path.write_text(script_text, encoding="utf-8")

                result = subprocess.run(
                    [str(MAC_BINARY), str(script_path)],
                    cwd=ROOT_DIR,
                    capture_output=True,
                    text=True,
                    timeout=20,
                    check=False,
                )

            if result.returncode != 0 or not output_path.exists():
                stderr = (result.stderr or result.stdout or "Unknown error").strip()
                raise RuntimeError(stderr or "GIF generation failed.")

            json_response(
                self,
                {
                    "ok": True,
                    "previewUrl": f"/generated/{output_name}",
                    "downloadUrl": f"/generated/{output_name}",
                    "script": script_text,
                    "summary": {
                        "frameCount": len(normalized_payload["frames"]),
                        "durationMs": sum(frame["duration"] for frame in normalized_payload["frames"]),
                        "width": normalized_payload["width"],
                        "height": normalized_payload["height"],
                        "template": normalized_payload["template"],
                        "fileSizeBytes": output_path.stat().st_size,
                    },
                },
            )
        except ValueError as exc:
            json_response(self, {"error": str(exc)}, status=HTTPStatus.BAD_REQUEST)
        except subprocess.TimeoutExpired:
            json_response(
                self,
                {"error": "GIF generation timed out after 20 seconds."},
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
    parser = argparse.ArgumentParser(description="Mac GIF Studio web server")
    parser.add_argument("--host", default="0.0.0.0", help="Host interface to bind")
    parser.add_argument("--port", type=int, default=9001, help="Port to listen on")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    GENERATED_DIR.mkdir(parents=True, exist_ok=True)
    cleanup_generated_dir()

    server = FastThreadingHTTPServer((args.host, args.port), GifStudioHandler)
    print(f"Mac GIF Studio running at http://{args.host}:{args.port}")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()
