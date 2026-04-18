# Mac Playground Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a browser-based coding playground for the Mac language with Monaco Editor, real-time analysis, code execution, and opt-in meme/GIF rendering.

**Architecture:** Python stdlib HTTP server (same pattern as studio) shells out to the `mac` binary for execution and `mac --analyze` for diagnostics. Monaco Editor in the browser with TextMate grammar for syntax highlighting. Single-page app, no build step, no framework.

**Tech Stack:** Python 3 (stdlib only), Monaco Editor (CDN), monaco-textmate + vscode-oniguruma (CDN/WASM), lz-string (CDN)

**Spec:** `docs/superpowers/specs/2026-04-17-playground-design.md`

---

## File Structure

```
mac-studio-meme/playground/
├── server.py                  # Python HTTP server — all API endpoints
├── run.sh                     # Startup script — builds mac binary, starts server
├── loadtest.py                # Load test — 30 TPS sustained for 60s
├── .gitignore                 # Ignore generated/, uploads/, __pycache__/
├── static/
│   ├── index.html             # SPA — layout, Monaco loader, top bar, panels
│   ├── styles.css             # Design tokens, layout grid, component styles
│   ├── app.js                 # Initialization — boot Monaco, wire modules
│   ├── js/
│   │   ├── editor.js          # Monaco config — theme, grammar, language registration
│   │   ├── analyzer.js        # Debounced /api/analyze calls, Monaco provider wiring
│   │   ├── runner.js          # Run/Render button handlers, output display
│   │   ├── sharing.js         # LZ-string URL encode/decode, share button
│   │   ├── uploads.js         # Image upload, gallery, TTL display
│   │   └── examples.js        # 8 example definitions, dropdown loader
│   └── mac.tmLanguage.json    # TextMate grammar (copied from mac-lang/)
├── generated/                 # Rendered images (gitignored)
├── uploads/                   # User uploads (gitignored)
└── .github/
    └── workflows/
        └── deploy.yml         # SSH deploy to Rigbox on push to main
```

---

### Task 1: Repo Setup + Server Skeleton

**Files:**
- Create: `server.py`
- Create: `run.sh`
- Create: `.gitignore`
- Create: `.github/workflows/deploy.yml`

- [ ] **Step 1: Create the GitHub repo**

```bash
gh repo create mac-studio-meme/playground --private --description "Mac Playground — browser-based coding environment for Mac (Meme as Code)"
```

- [ ] **Step 2: Initialize local repo and create .gitignore**

```bash
mkdir -p ~/workspace/playground && cd ~/workspace/playground
git init
```

Create `.gitignore`:

```
__pycache__/
*.pyc
generated/
uploads/
.DS_Store
```

- [ ] **Step 3: Create server.py with /api/status endpoint**

```python
#!/usr/bin/env python3
from __future__ import annotations

import argparse
import collections
import json
import mimetypes
import os
import re
import subprocess
import tempfile
import threading
import time
import uuid
from http import HTTPStatus
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from urllib.parse import unquote, urlparse

WEBAPP_DIR = Path(__file__).resolve().parent
STATIC_DIR = WEBAPP_DIR / "static"
GENERATED_DIR = WEBAPP_DIR / "generated"
UPLOADS_DIR = WEBAPP_DIR / "uploads"
MAC_ROOT = Path(os.environ.get("MAC_ROOT", str(WEBAPP_DIR.parent / "mac")))
MAC_BINARY = MAC_ROOT / "build" / "mac"

VERSION = "0.1.0"
MAX_REQUEST_BYTES = 256 * 1024
MAX_CODE_LENGTH = 50_000

# ── Helpers ──────────────────────────────────────────────

def json_response(
    handler: BaseHTTPRequestHandler,
    payload: dict,
    status: HTTPStatus = HTTPStatus.OK,
    send_body: bool = True,
) -> None:
    body = json.dumps(payload).encode("utf-8")
    handler.send_response(status)
    handler.send_header("Content-Type", "application/json; charset=utf-8")
    handler.send_header("Content-Length", str(len(body)))
    handler.send_header("Cache-Control", "no-store")
    handler.send_header("X-Content-Type-Options", "nosniff")
    handler.end_headers()
    if send_body:
        handler.wfile.write(body)


def read_json_body(handler: BaseHTTPRequestHandler) -> dict:
    length = int(handler.headers.get("Content-Length", 0))
    if length > MAX_REQUEST_BYTES:
        raise ValueError("Request too large.")
    raw = handler.rfile.read(length)
    return json.loads(raw)


def serve_file(
    handler: BaseHTTPRequestHandler,
    path: Path,
    cache_control: str = "public, max-age=300",
    send_body: bool = True,
) -> None:
    if not path.exists() or not path.is_file():
        handler.send_error(HTTPStatus.NOT_FOUND)
        return
    content_type, _ = mimetypes.guess_type(str(path))
    body = path.read_bytes()
    handler.send_response(HTTPStatus.OK)
    handler.send_header("Content-Type", content_type or "application/octet-stream")
    handler.send_header("Content-Length", str(len(body)))
    handler.send_header("Cache-Control", cache_control)
    handler.end_headers()
    if send_body:
        handler.wfile.write(body)


def safe_child_path(base: Path, relative: str) -> Path | None:
    candidate = (base / relative).resolve()
    if not str(candidate).startswith(str(base.resolve())):
        return None
    if not candidate.exists():
        return None
    return candidate


# ── Handler ──────────────────────────────────────────────

class PlaygroundHandler(BaseHTTPRequestHandler):
    def log_message(self, fmt, *args):
        pass  # suppress default logging

    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        path = unquote(parsed.path)

        if path == "/api/status":
            json_response(self, {
                "ok": True,
                "version": VERSION,
                "binaryReady": MAC_BINARY.exists(),
            })
            return

        # Static file serving
        static_rel = "index.html" if path in {"", "/"} else path.lstrip("/")
        candidate = safe_child_path(STATIC_DIR, static_rel)
        if candidate is None or not candidate.exists():
            candidate = STATIC_DIR / "index.html"
        serve_file(self, candidate)

    def do_POST(self) -> None:
        json_response(
            self,
            {"error": "Not implemented."},
            status=HTTPStatus.NOT_FOUND,
        )

    def do_HEAD(self) -> None:
        self.do_GET()

    def do_OPTIONS(self) -> None:
        self.send_response(HTTPStatus.NO_CONTENT)
        self.send_header("Allow", "GET, POST, HEAD, OPTIONS")
        self.end_headers()


# ── Main ─────────────────────────────────────────────────

def main() -> None:
    parser = argparse.ArgumentParser(description="Mac Playground Server")
    parser.add_argument("--host", default="0.0.0.0")
    parser.add_argument("--port", type=int, default=9002)
    args = parser.parse_args()

    GENERATED_DIR.mkdir(parents=True, exist_ok=True)
    UPLOADS_DIR.mkdir(parents=True, exist_ok=True)

    server = ThreadingHTTPServer((args.host, args.port), PlaygroundHandler)
    print(f"Playground running on http://{args.host}:{args.port}")
    print(f"MAC_ROOT: {MAC_ROOT}")
    print(f"Binary:   {MAC_BINARY} ({'ready' if MAC_BINARY.exists() else 'NOT FOUND'})")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        print("\nShutting down.")
        server.shutdown()


if __name__ == "__main__":
    main()
```

- [ ] **Step 4: Create run.sh**

```bash
#!/usr/bin/env bash
set -euo pipefail

PLAYGROUND_DIR="$(cd "$(dirname "$0")" && pwd)"
MAC_ROOT="${MAC_ROOT:-$PLAYGROUND_DIR/../mac}"
HOST="${HOST:-0.0.0.0}"
PORT="${PORT:-9002}"

mkdir -p "$PLAYGROUND_DIR/generated"
mkdir -p "$PLAYGROUND_DIR/uploads"
mkdir -p "$MAC_ROOT/build"

if command -v cmake >/dev/null 2>&1; then
  cmake -S "$MAC_ROOT" -B "$MAC_ROOT/build"
  cmake --build "$MAC_ROOT/build"
else
  g++ \
    -std=c++23 \
    -O2 \
    -I"$MAC_ROOT/include" \
    "$MAC_ROOT/src/main.cpp" \
    "$MAC_ROOT/src/Scanner.cpp" \
    "$MAC_ROOT/src/Parser.cpp" \
    "$MAC_ROOT/src/Interpreter.cpp" \
    "$MAC_ROOT/src/stb_impl.cpp" \
    -o "$MAC_ROOT/build/mac"
fi

export MAC_ROOT
exec python3 -u "$PLAYGROUND_DIR/server.py" --host "$HOST" --port "$PORT"
```

```bash
chmod +x run.sh
```

- [ ] **Step 5: Create deploy workflow**

Create `.github/workflows/deploy.yml`:

```yaml
name: Deploy

on:
  push:
    branches: [main]

env:
  FORCE_JAVASCRIPT_ACTIONS_TO_NODE24: true

jobs:
  deploy:
    name: Deploy to Rigbox
    runs-on: ubuntu-latest
    steps:
      - name: Deploy via SSH
        uses: appleboy/ssh-action@v1
        with:
          host: eu-west-1.rigbox.dev
          username: base-uvvrblcc-1yvx9yo7
          key: ${{ secrets.RIGBOX_SSH_KEY }}
          script: bash ~/deploy.sh
```

- [ ] **Step 6: Create a minimal index.html for testing**

Create `static/index.html`:

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Mac Playground</title>
</head>
<body>
    <h1>Mac Playground</h1>
    <p>Coming soon.</p>
</body>
</html>
```

- [ ] **Step 7: Test the server locally**

```bash
MAC_ROOT=~/workspace/mac-cpp python3 server.py --port 9002
# In another terminal:
curl -s http://localhost:9002/api/status | python3 -m json.tool
```

Expected:
```json
{
    "ok": true,
    "version": "0.1.0",
    "binaryReady": true
}
```

- [ ] **Step 8: Commit and push**

```bash
git add -A
git commit -m "feat: server skeleton with /api/status endpoint"
git remote add origin git@github.com:mac-studio-meme/playground.git
git push -u origin main
```

---

### Task 2: Frontend Layout + Styles

**Files:**
- Create: `static/styles.css`
- Modify: `static/index.html`

- [ ] **Step 1: Create styles.css with design tokens and layout**

```css
/* ── Fonts ─────────────────────────────────────────── */
@import url('https://fonts.googleapis.com/css2?family=Bricolage+Grotesque:wght@400;500;600;700;800&family=Cormorant+Garamond:wght@500;600;700&family=IBM+Plex+Mono:wght@400;500;600&display=swap');

/* ── Design Tokens ─────────────────────────────────── */
:root {
    --bg: #1a1918;
    --surface: #211f1d;
    --surface-hover: #292724;
    --surface-active: #332f2c;
    --surface-active-strong: #3d3935;
    --ink: #e8e0d4;
    --ink-muted: rgba(232, 224, 212, 0.5);
    --ink-faint: rgba(232, 224, 212, 0.25);
    --accent: #c44b2b;
    --accent-hover: #d4593a;
    --accent-soft: rgba(196, 75, 43, 0.15);
    --teal: #3aaa8a;
    --teal-hover: #4dbfa0;
    --gold: #c8992a;
    --green: #98c379;
    --red: #c44b2b;
    --divider: rgba(232, 224, 212, 0.1);
    --divider-strong: rgba(232, 224, 212, 0.18);
    --shadow: rgba(0, 0, 0, 0.3);
    --radius: 10px;
    --radius-sm: 6px;
    --font-display: 'Bricolage Grotesque', sans-serif;
    --font-serif: 'Cormorant Garamond', serif;
    --font-mono: 'IBM Plex Mono', monospace;
    --topbar-h: 48px;
    --statusbar-h: 28px;
}

/* ── Reset ─────────────────────────────────────────── */
*, *::before, *::after { box-sizing: border-box; margin: 0; padding: 0; }
html, body { height: 100%; overflow: hidden; }
body {
    font-family: var(--font-display);
    background: var(--bg);
    color: var(--ink);
    font-size: 13px;
}

/* ── Top Bar ───────────────────────────────────────── */
.topbar {
    height: var(--topbar-h);
    background: var(--surface);
    border-bottom: 1px solid var(--divider);
    display: flex;
    align-items: center;
    padding: 0 16px;
    gap: 12px;
    z-index: 10;
}

.topbar-brand {
    display: flex;
    align-items: baseline;
    gap: 6px;
    margin-right: auto;
}

.topbar-brand-name {
    font-family: var(--font-serif);
    font-weight: 700;
    font-size: 20px;
    color: var(--accent);
}

.topbar-brand-sub {
    font-family: var(--font-mono);
    font-size: 11px;
    color: var(--ink-muted);
}

.topbar-group {
    display: flex;
    align-items: center;
    gap: 8px;
}

/* ── Buttons ───────────────────────────────────────── */
.btn {
    font-family: var(--font-display);
    font-size: 12px;
    font-weight: 600;
    padding: 6px 14px;
    border: none;
    border-radius: var(--radius-sm);
    cursor: pointer;
    transition: background 0.15s, transform 0.1s;
    display: flex;
    align-items: center;
    gap: 6px;
}

.btn:active { transform: scale(0.97); }

.btn-run {
    background: var(--accent);
    color: #fff;
}
.btn-run:hover { background: var(--accent-hover); }

.btn-render {
    background: var(--teal);
    color: #fff;
}
.btn-render:hover { background: var(--teal-hover); }

.btn-ghost {
    background: var(--surface-hover);
    color: var(--ink);
}
.btn-ghost:hover { background: var(--surface-active); }

.btn-share {
    background: var(--surface-hover);
    color: var(--ink);
}
.btn-share:hover { background: var(--surface-active); }

.btn:disabled {
    opacity: 0.5;
    cursor: not-allowed;
    transform: none;
}

/* ── Select / Dropdown ─────────────────────────────── */
.select {
    font-family: var(--font-mono);
    font-size: 12px;
    padding: 5px 10px;
    background: var(--surface-hover);
    color: var(--ink);
    border: 1px solid var(--divider-strong);
    border-radius: var(--radius-sm);
    cursor: pointer;
    appearance: none;
    -webkit-appearance: none;
    background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='10' height='6'%3E%3Cpath d='M0 0l5 6 5-6z' fill='%23e8e0d4' opacity='0.5'/%3E%3C/svg%3E");
    background-repeat: no-repeat;
    background-position: right 8px center;
    padding-right: 24px;
}

.select:hover { border-color: var(--ink-muted); }

/* ── Main Layout ───────────────────────────────────── */
.main {
    display: flex;
    height: calc(100vh - var(--topbar-h) - var(--statusbar-h));
}

/* ── Editor Panel ──────────────────────────────────── */
.panel-editor {
    flex: 6;
    min-width: 0;
    display: flex;
    flex-direction: column;
    border-right: 1px solid var(--divider);
}

.editor-container {
    flex: 1;
    min-height: 0;
}

/* ── Output Panel ──────────────────────────────────── */
.panel-output {
    flex: 4;
    min-width: 0;
    display: flex;
    flex-direction: column;
}

.output-console {
    flex: 1;
    min-height: 0;
    display: flex;
    flex-direction: column;
    overflow: hidden;
}

.output-header {
    display: flex;
    align-items: center;
    padding: 8px 12px;
    gap: 8px;
    border-bottom: 1px solid var(--divider);
    flex-shrink: 0;
}

.output-label {
    font-family: var(--font-mono);
    font-size: 10px;
    font-weight: 600;
    text-transform: uppercase;
    letter-spacing: 0.08em;
    color: var(--ink-muted);
}

.output-badge {
    font-family: var(--font-mono);
    font-size: 10px;
    padding: 1px 6px;
    border-radius: 3px;
    background: var(--surface-active);
    color: var(--ink-muted);
    margin-left: auto;
}

.output-body {
    flex: 1;
    overflow-y: auto;
    padding: 10px 12px;
    font-family: var(--font-mono);
    font-size: 12px;
    line-height: 1.6;
    white-space: pre-wrap;
    word-break: break-word;
}

.output-stdout { color: var(--green); }
.output-stderr { color: var(--red); }

.output-empty {
    color: var(--ink-faint);
    font-style: italic;
}

/* ── Render Area ───────────────────────────────────── */
.render-area {
    border-top: 1px solid var(--divider);
    padding: 12px;
    display: none;
    flex-direction: column;
    align-items: center;
    gap: 8px;
    max-height: 50%;
    overflow-y: auto;
}

.render-area.visible { display: flex; }

.render-area img {
    max-width: 100%;
    max-height: 300px;
    border-radius: var(--radius-sm);
    border: 1px solid var(--divider);
}

.render-download {
    font-family: var(--font-mono);
    font-size: 11px;
    color: var(--teal);
    text-decoration: none;
}
.render-download:hover { text-decoration: underline; }

/* ── Upload Gallery ────────────────────────────────── */
.upload-gallery {
    display: none;
    border-top: 1px solid var(--divider);
    padding: 8px 12px;
    gap: 6px;
    flex-wrap: wrap;
    max-height: 80px;
    overflow-y: auto;
}

.upload-gallery.visible { display: flex; }

.upload-thumb {
    width: 40px;
    height: 40px;
    border-radius: 4px;
    object-fit: cover;
    border: 1px solid var(--divider);
    cursor: pointer;
    transition: border-color 0.15s;
}

.upload-thumb:hover { border-color: var(--teal); }

/* ── Status Bar ────────────────────────────────────── */
.statusbar {
    height: var(--statusbar-h);
    background: var(--surface);
    border-top: 1px solid var(--divider);
    display: flex;
    align-items: center;
    padding: 0 12px;
    gap: 12px;
    font-family: var(--font-mono);
    font-size: 10px;
    color: var(--ink-muted);
}

.status-dot {
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: var(--teal);
    display: inline-block;
}

.status-dot.warning { background: var(--gold); }
.status-dot.error { background: var(--red); }

/* ── Toast ─────────────────────────────────────────── */
.toast {
    position: fixed;
    bottom: 40px;
    left: 50%;
    transform: translateX(-50%) translateY(20px);
    background: var(--surface-active-strong);
    color: var(--ink);
    font-family: var(--font-mono);
    font-size: 12px;
    padding: 8px 16px;
    border-radius: var(--radius-sm);
    box-shadow: 0 4px 12px var(--shadow);
    opacity: 0;
    transition: opacity 0.2s, transform 0.2s;
    pointer-events: none;
    z-index: 100;
}

.toast.show {
    opacity: 1;
    transform: translateX(-50%) translateY(0);
}

/* ── Spinner ───────────────────────────────────────── */
@keyframes spin { to { transform: rotate(360deg); } }
.spinner {
    width: 14px;
    height: 14px;
    border: 2px solid rgba(255,255,255,0.3);
    border-top-color: #fff;
    border-radius: 50%;
    animation: spin 0.6s linear infinite;
}
```

- [ ] **Step 2: Update index.html with full layout structure**

```html
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Mac Playground</title>
    <meta name="description" content="Write, run, and share Mac (Meme as Code) programs in your browser.">
    <link rel="icon" href="/favicon.svg" type="image/svg+xml">
    <link rel="stylesheet" href="/styles.css">
</head>
<body>
    <!-- Top Bar -->
    <div class="topbar">
        <div class="topbar-brand">
            <span class="topbar-brand-name">Mac</span>
            <span class="topbar-brand-sub">Playground</span>
        </div>

        <div class="topbar-group">
            <select class="select" id="examples-select">
                <option value="">Examples...</option>
            </select>

            <button class="btn btn-ghost" id="btn-upload" title="Upload image">Upload</button>

            <button class="btn btn-run" id="btn-run" title="Run code (stdout only)">
                <span class="btn-label">Run</span>
            </button>

            <button class="btn btn-render" id="btn-render" title="Run and render images">
                <span class="btn-label">Run &amp; Render</span>
            </button>

            <button class="btn btn-share" id="btn-share" title="Copy share link">Share</button>

            <a class="btn btn-ghost" href="https://docs.macstudio.meme" target="_blank" rel="noopener">Docs</a>
        </div>
    </div>

    <!-- Main Panels -->
    <div class="main">
        <!-- Editor -->
        <div class="panel-editor">
            <div class="editor-container" id="editor-container"></div>
            <div class="upload-gallery" id="upload-gallery"></div>
        </div>

        <!-- Output -->
        <div class="panel-output">
            <div class="output-console">
                <div class="output-header">
                    <span class="output-label">Output</span>
                    <span class="output-badge" id="output-duration" style="display:none"></span>
                </div>
                <div class="output-body" id="output-body">
                    <span class="output-empty">Press Run to execute your code.</span>
                </div>
            </div>
            <div class="render-area" id="render-area">
                <div class="output-header" style="width:100%">
                    <span class="output-label">Render</span>
                </div>
            </div>
        </div>
    </div>

    <!-- Status Bar -->
    <div class="statusbar">
        <span class="status-dot" id="status-dot"></span>
        <span id="status-text">Ready</span>
        <span style="margin-left:auto">Mac v<span id="status-version">0.7.0</span></span>
    </div>

    <!-- Toast -->
    <div class="toast" id="toast"></div>

    <!-- Hidden file input for uploads -->
    <input type="file" id="file-input" accept="image/png,image/jpeg,image/gif" style="display:none">

    <!-- Scripts loaded after DOM -->
    <script src="/js/examples.js"></script>
    <script src="/js/sharing.js"></script>
    <script src="/js/runner.js"></script>
    <script src="/js/uploads.js"></script>
    <script src="/js/analyzer.js"></script>
    <script src="/js/editor.js"></script>
    <script src="/app.js"></script>
</body>
</html>
```

- [ ] **Step 3: Test the layout renders in browser**

```bash
MAC_ROOT=~/workspace/mac-cpp python3 server.py --port 9002
# Open http://localhost:9002 — should see the top bar, empty editor area, output panel, status bar
```

- [ ] **Step 4: Commit**

```bash
git add static/index.html static/styles.css
git commit -m "feat: frontend layout with design tokens and panel structure"
```

---

### Task 3: Monaco Editor Setup

**Files:**
- Create: `static/js/editor.js`
- Create: `static/app.js`
- Copy: `static/mac.tmLanguage.json` (from `mac-lang/syntaxes/`)

- [ ] **Step 1: Copy the TextMate grammar**

```bash
cp ~/workspace/mac-cpp/mac-lang/syntaxes/mac.tmLanguage.json static/mac.tmLanguage.json
```

- [ ] **Step 2: Create editor.js — Monaco configuration and theme**

```javascript
// editor.js — Monaco Editor setup: language registration, theme, grammar
'use strict';

const MacEditor = (() => {
    let _editor = null;
    let _model = null;

    const MAC_THEME = {
        base: 'vs-dark',
        inherit: false,
        rules: [
            { token: '',                foreground: 'e8e0d4', background: '1a1918' },
            { token: 'comment',         foreground: '5c6370', fontStyle: 'italic' },
            { token: 'string',          foreground: '98c379' },
            { token: 'number',          foreground: 'd19a66' },
            { token: 'keyword',         foreground: 'e06040' },
            { token: 'operator',        foreground: 'c678dd' },
            { token: 'identifier',      foreground: 'e8e0d4' },
            { token: 'type',            foreground: 'e5c07b' },
            { token: 'function',        foreground: '61afef' },
            { token: 'constant',        foreground: '56b6c2' },
            { token: 'delimiter',       foreground: '8a8070' },
            { token: 'tag',             foreground: 'e06040', fontStyle: 'bold' },
        ],
        colors: {
            'editor.background':                '#1a1918',
            'editor.foreground':                '#e8e0d4',
            'editor.lineHighlightBackground':   '#211f1d',
            'editor.selectionBackground':       '#3d393580',
            'editorCursor.foreground':          '#e8e0d4',
            'editorLineNumber.foreground':      '#5c6370',
            'editorLineNumber.activeForeground':'#e8e0d4',
            'editorIndentGuide.background':     '#292724',
            'editorWidget.background':          '#211f1d',
            'editorSuggestWidget.background':   '#211f1d',
            'editorSuggestWidget.border':       '#3d3935',
            'editorSuggestWidget.selectedBackground': '#332f2c',
            'editorHoverWidget.background':     '#211f1d',
            'editorHoverWidget.border':         '#3d3935',
            'input.background':                 '#292724',
            'input.border':                     '#3d3935',
            'focusBorder':                      '#c44b2b',
            'list.hoverBackground':             '#292724',
            'list.focusBackground':             '#332f2c',
            'scrollbar.shadow':                 '#00000000',
            'scrollbarSlider.background':       '#3d393560',
            'scrollbarSlider.hoverBackground':  '#3d393590',
        },
    };

    // Simple monarch tokenizer as fallback / primary tokenizer
    const MAC_LANGUAGE = {
        defaultToken: '',
        tokenPostfix: '.mac',

        keywords: [
            'val', 'var', 'fun', 'class', 'enum', 'return', 'if', 'else',
            'for', 'while', 'break', 'continue', 'print', 'in', 'match',
            'style', 'effect', 'grid', 'gif', 'loop', 'this', 'super',
            'init', 'import', 'nil', 'true', 'false',
        ],

        typeKeywords: [
            'Template', 'Meme', 'Gif', 'Size', 'Duration', 'Position', 'Format',
            'Result', 'Option',
        ],

        constants: ['Top', 'Bottom', 'Center', 'PNG', 'JPG', 'GIF'],

        operators: [
            '=>', '->', '|>', '>>', '==', '!=', '<=', '>=',
            '=', '+', '-', '*', '/', '%', '<', '>', '!',
            '&&', '||',
        ],

        symbols: /[=><!~?:&|+\-*\/\^%]+/,

        tokenizer: {
            root: [
                // Comments
                [/\/\/.*$/, 'comment'],

                // Duration literals (before numbers)
                [/\b\d+(ms|s)\b/, 'number'],

                // Dimension literals
                [/\b\d+x\d+\b/, 'number'],

                // Numbers
                [/\b\d+(\.\d+)?\b/, 'number'],

                // Template literals
                [/@\w+/, 'tag'],

                // Strings with interpolation
                [/"/, 'string', '@string'],

                // Keywords and identifiers
                [/[a-zA-Z_]\w*/, {
                    cases: {
                        '@keywords': 'keyword',
                        '@typeKeywords': 'type',
                        '@constants': 'constant',
                        '@default': 'identifier',
                    },
                }],

                // Operators
                [/@symbols/, {
                    cases: {
                        '@operators': 'operator',
                        '@default': 'delimiter',
                    },
                }],

                // Delimiters
                [/[{}()\[\]]/, 'delimiter'],
                [/[;,.]/, 'delimiter'],
            ],

            string: [
                [/\{/, { token: 'delimiter', next: '@interpolation' }],
                [/[^"\\{]+/, 'string'],
                [/\\./, 'string'],
                [/"/, 'string', '@pop'],
            ],

            interpolation: [
                [/\}/, { token: 'delimiter', next: '@pop' }],
                [/[^}]+/, { token: '@rematch', next: '@root' }],
            ],
        },
    };

    function create(containerId, initialCode) {
        monaco.languages.register({ id: 'mac' });
        monaco.languages.setMonarchTokensProvider('mac', MAC_LANGUAGE);
        monaco.editor.defineTheme('mac-dark', MAC_THEME);

        _editor = monaco.editor.create(document.getElementById(containerId), {
            language: 'mac',
            theme: 'mac-dark',
            value: initialCode || '',
            fontSize: 13,
            fontFamily: "'IBM Plex Mono', monospace",
            lineHeight: 20,
            minimap: { enabled: false },
            scrollBeyondLastLine: false,
            renderLineHighlight: 'line',
            padding: { top: 10 },
            tabSize: 4,
            insertSpaces: true,
            automaticLayout: true,
            wordWrap: 'off',
            suggestOnTriggerCharacters: true,
            quickSuggestions: true,
            parameterHints: { enabled: true },
            folding: true,
            bracketPairColorization: { enabled: false },
        });

        _model = _editor.getModel();

        // Cmd/Ctrl+Enter to run
        _editor.addAction({
            id: 'mac-run',
            label: 'Run',
            keybindings: [monaco.KeyMod.CtrlCmd | monaco.KeyCode.Enter],
            run: () => document.getElementById('btn-run').click(),
        });

        // Cmd/Ctrl+Shift+Enter to run & render
        _editor.addAction({
            id: 'mac-render',
            label: 'Run & Render',
            keybindings: [monaco.KeyMod.CtrlCmd | monaco.KeyMod.Shift | monaco.KeyCode.Enter],
            run: () => document.getElementById('btn-render').click(),
        });

        return _editor;
    }

    function getCode() {
        return _editor ? _editor.getValue() : '';
    }

    function setCode(code) {
        if (_editor) _editor.setValue(code);
    }

    function getModel() {
        return _model;
    }

    function getInstance() {
        return _editor;
    }

    return { create, getCode, setCode, getModel, getInstance };
})();
```

- [ ] **Step 3: Create app.js — initialization**

```javascript
// app.js — Application initialization
'use strict';

(function boot() {
    // Load Monaco from CDN
    const loaderScript = document.createElement('script');
    loaderScript.src = 'https://cdn.jsdelivr.net/npm/monaco-editor@0.52.2/min/vs/loader.js';
    loaderScript.onload = () => {
        require.config({
            paths: { vs: 'https://cdn.jsdelivr.net/npm/monaco-editor@0.52.2/min/vs' },
        });
        require(['vs/editor/editor.main'], () => {
            initApp();
        });
    };
    document.head.appendChild(loaderScript);

    function initApp() {
        // Determine initial code: shared URL > default example
        const shared = MacSharing.decodeFromURL();
        const initialCode = shared || MacExamples.getDefault();

        // Create editor
        MacEditor.create('editor-container', initialCode);

        // Wire up examples dropdown
        MacExamples.populateDropdown(document.getElementById('examples-select'));

        // Wire up buttons
        document.getElementById('btn-run').addEventListener('click', () => MacRunner.run());
        document.getElementById('btn-render').addEventListener('click', () => MacRunner.render());
        document.getElementById('btn-share').addEventListener('click', () => MacSharing.share());
        document.getElementById('btn-upload').addEventListener('click', () => MacUploads.openPicker());

        // Start analyzer
        MacAnalyzer.start();

        // Fetch version from server
        fetch('/api/status')
            .then(r => r.json())
            .then(data => {
                if (data.version) {
                    document.getElementById('status-version').textContent = data.version;
                }
            })
            .catch(() => {});
    }
})();
```

- [ ] **Step 4: Create stub JS modules so the page loads without errors**

Create `static/js/examples.js`:

```javascript
// examples.js — Example definitions and dropdown loader
'use strict';

const MacExamples = (() => {
    const EXAMPLES = [
        {
            name: 'Hello World',
            code: [
                '// Hello, Mac!',
                'val greeting = "Hello, Mac!";',
                'print greeting;',
                '',
                'fun fib(n) {',
                '    if (n <= 1) return n;',
                '    fib(n - 1) + fib(n - 2)',
                '}',
                'print "fib(10) = {fib(10)}";',
            ].join('\n'),
        },
        {
            name: 'Meme Basics',
            code: [
                '// Create a two-panel meme',
                '@two_panel {',
                '    top: "Me explaining Mac to my team"',
                '    bottom: "My team: just use Photoshop"',
                '} => "first_meme.png";',
                '',
                '// One-liner with custom size',
                '@blank 400x400 "Compact meme" => "compact.png";',
            ].join('\n'),
        },
        {
            name: 'Styles & Effects',
            code: [
                '// Define a reusable text style',
                'style neon {',
                '    color: "#00FF41"',
                '    outline: 4',
                '    outlineColor: "#003300"',
                '    shadow: 3',
                '    shadowColor: "#00FF4180"',
                '}',
                '',
                '// Compose effects into a pipeline',
                'effect retro = sepia >> brightness(0.9);',
                '',
                '@dark neon {',
                '    top: "NEON"',
                '    bottom: "Styled text"',
                '} |> retro => "neon_retro.png";',
            ].join('\n'),
        },
        {
            name: 'GIF Animation',
            code: [
                '// Countdown GIF with frame timing',
                'gif loop {',
                '    @blank 480x480 "3" : 500ms',
                '    @blank 480x480 "2" : 500ms',
                '    @blank 480x480 "1" : 500ms',
                '    @blank 480x480 "GO!" : 1s',
                '} => "countdown.gif";',
            ].join('\n'),
        },
        {
            name: 'Functional Pipelines',
            code: [
                '// Pipe operator chains transformations',
                'val result = [1, 2, 3, 4, 5]',
                '    |> filter(x -> x > 2)',
                '    |> map(x -> x * 10)',
                '    |> reverse;',
                'print result;',
                '',
                '// String pipelines',
                'print "a,b,c" |> split(",") |> map(upper) |> join(" - ");',
                '',
                '// Reduce',
                'print [1, 2, 3, 4, 5] |> reduce((sum, n) -> sum + n, 0);',
            ].join('\n'),
        },
        {
            name: 'Pattern Matching',
            code: [
                '// Define a sum type',
                'enum Shape {',
                '    Circle(radius)',
                '    Rect(w, h)',
                '}',
                '',
                '// Match with destructuring',
                'val area = (s) -> match s {',
                '    Shape.Circle(r) -> 3.14159 * r * r',
                '    Shape.Rect(w, h) -> w * h',
                '};',
                '',
                'print area(Shape.Circle(5));',
                'print area(Shape.Rect(3, 4));',
            ].join('\n'),
        },
        {
            name: 'String Interpolation',
            code: [
                '// Expressions in strings with {braces}',
                'val name = "Mac";',
                'val version = 7;',
                'print "Welcome to {name} v0.{version}.0!";',
                '',
                '// Any expression works',
                'val nums = [1, 2, 3];',
                'print "Sum: {nums |> reduce((a, b) -> a + b, 0)}";',
                'print "Count: {len(nums)}";',
                '',
                '// Interpolation in meme text',
                'val lang = "Mac";',
                '@blank "{lang} is awesome" => "interp.png";',
            ].join('\n'),
        },
        {
            name: 'Data-Driven Memes',
            code: [
                '// Generate memes from data',
                'val quotes = [',
                '    ["Debugging", "print(\'here\')"],',
                '    ["Testing",   "works on my machine"],',
                '    ["Deploying", "YOLO"],',
                '];',
                '',
                '// Map data into meme frames',
                'val memes = quotes |> map(q -> @two_panel {',
                '    top: q[0]',
                '    bottom: q[1]',
                '});',
                '',
                '// Build a GIF from the frames',
                'val lifecycle = memes |> reduce(',
                '    (g, m) -> g.frame(m, Duration(1500)), Gif());',
                'lifecycle.save("dev_lifecycle.gif");',
            ].join('\n'),
        },
    ];

    function getDefault() {
        return EXAMPLES[0].code;
    }

    function populateDropdown(select) {
        EXAMPLES.forEach((ex, i) => {
            const opt = document.createElement('option');
            opt.value = i;
            opt.textContent = ex.name;
            select.appendChild(opt);
        });

        select.addEventListener('change', () => {
            const idx = parseInt(select.value, 10);
            if (!isNaN(idx) && EXAMPLES[idx]) {
                MacEditor.setCode(EXAMPLES[idx].code);
                select.value = '';
            }
        });
    }

    return { getDefault, populateDropdown, EXAMPLES };
})();
```

Create `static/js/sharing.js`:

```javascript
// sharing.js — URL encode/decode with lz-string, share button
'use strict';

const MacSharing = (() => {
    // lz-string loaded lazily from CDN
    let _lzReady = false;

    function _ensureLZ() {
        if (_lzReady) return Promise.resolve();
        return new Promise((resolve) => {
            const s = document.createElement('script');
            s.src = 'https://cdn.jsdelivr.net/npm/lz-string@1.5.0/libs/lz-string.min.js';
            s.onload = () => { _lzReady = true; resolve(); };
            document.head.appendChild(s);
        });
    }

    function decodeFromURL() {
        const hash = window.location.hash;
        if (!hash.includes('code=')) return null;
        try {
            const params = new URLSearchParams(hash.slice(1));
            const encoded = params.get('code');
            if (!encoded) return null;
            // Try lz-string decompression (sync — lib may not be loaded yet for initial load)
            if (typeof LZString !== 'undefined') {
                return LZString.decompressFromEncodedURIComponent(encoded);
            }
            // Fallback: base64 decode
            return atob(encoded);
        } catch {
            return null;
        }
    }

    async function share() {
        const code = MacEditor.getCode();
        if (!code.trim()) return;

        await _ensureLZ();
        const encoded = LZString.compressToEncodedURIComponent(code);
        const url = `${window.location.origin}/#code=${encoded}`;

        try {
            await navigator.clipboard.writeText(url);
            _toast('Link copied to clipboard');
        } catch {
            // Fallback: select a temp input
            const tmp = document.createElement('input');
            tmp.value = url;
            document.body.appendChild(tmp);
            tmp.select();
            document.execCommand('copy');
            tmp.remove();
            _toast('Link copied to clipboard');
        }

        // Update URL without reload
        history.replaceState(null, '', `/#code=${encoded}`);
    }

    function _toast(msg) {
        const el = document.getElementById('toast');
        el.textContent = msg;
        el.classList.add('show');
        setTimeout(() => el.classList.remove('show'), 2000);
    }

    return { decodeFromURL, share };
})();
```

Create `static/js/runner.js`:

```javascript
// runner.js — Run/Render API calls and output display
'use strict';

const MacRunner = (() => {
    let _running = false;

    function _setRunning(val) {
        _running = val;
        document.getElementById('btn-run').disabled = val;
        document.getElementById('btn-render').disabled = val;
    }

    function _showOutput(data) {
        const body = document.getElementById('output-body');
        const duration = document.getElementById('output-duration');
        body.innerHTML = '';

        if (data.stdout) {
            const pre = document.createElement('div');
            pre.className = 'output-stdout';
            pre.textContent = data.stdout;
            body.appendChild(pre);
        }

        if (data.stderr) {
            const pre = document.createElement('div');
            pre.className = 'output-stderr';
            pre.textContent = data.stderr;
            body.appendChild(pre);
        }

        if (!data.stdout && !data.stderr) {
            body.innerHTML = '<span class="output-empty">No output.</span>';
        }

        if (data.durationMs !== undefined) {
            duration.textContent = `${data.durationMs}ms`;
            duration.style.display = '';
        }
    }

    function _showRender(images) {
        const area = document.getElementById('render-area');
        // Clear old renders (keep header)
        while (area.children.length > 1) area.removeChild(area.lastChild);

        if (!images || images.length === 0) {
            area.classList.remove('visible');
            return;
        }

        images.forEach(img => {
            const el = document.createElement('img');
            el.src = img.url;
            area.appendChild(el);

            const dl = document.createElement('a');
            dl.className = 'render-download';
            dl.href = img.url;
            dl.download = img.url.split('/').pop();
            dl.textContent = 'Download';
            area.appendChild(dl);
        });

        area.classList.add('visible');
    }

    async function _execute(endpoint) {
        if (_running) return;
        _setRunning(true);

        const code = MacEditor.getCode();
        const body = document.getElementById('output-body');
        body.innerHTML = '<span class="output-empty">Running...</span>';
        document.getElementById('render-area').classList.remove('visible');

        try {
            const res = await fetch(endpoint, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ code }),
            });

            if (res.status === 429) {
                _showOutput({ stderr: 'Rate limited. Please wait a moment and try again.' });
                return;
            }

            const data = await res.json();

            if (data.error) {
                _showOutput({ stderr: data.error });
                return;
            }

            _showOutput(data);

            if (data.images) {
                _showRender(data.images);
            }
        } catch (err) {
            _showOutput({ stderr: `Network error: ${err.message}` });
        } finally {
            _setRunning(false);
        }
    }

    function run() { return _execute('/api/run'); }
    function render() { return _execute('/api/render'); }

    return { run, render };
})();
```

Create `static/js/uploads.js`:

```javascript
// uploads.js — Image upload, gallery management
'use strict';

const MacUploads = (() => {
    const _uploads = [];

    function openPicker() {
        document.getElementById('file-input').click();
    }

    function init() {
        document.getElementById('file-input').addEventListener('change', async (e) => {
            const file = e.target.files[0];
            if (!file) return;
            e.target.value = '';

            if (file.size > 5 * 1024 * 1024) {
                alert('Image must be under 5MB.');
                return;
            }

            const form = new FormData();
            form.append('file', file);

            try {
                const res = await fetch('/api/upload', { method: 'POST', body: form });
                if (res.status === 429) {
                    alert('Upload limit reached. Try again later.');
                    return;
                }
                const data = await res.json();
                if (data.error) {
                    alert(data.error);
                    return;
                }
                _uploads.push(data);
                _renderGallery();
            } catch (err) {
                alert(`Upload failed: ${err.message}`);
            }
        });
    }

    function _renderGallery() {
        const gallery = document.getElementById('upload-gallery');
        gallery.innerHTML = '';

        if (_uploads.length === 0) {
            gallery.classList.remove('visible');
            return;
        }

        _uploads.forEach(u => {
            const img = document.createElement('img');
            img.className = 'upload-thumb';
            img.src = u.previewUrl;
            img.title = `@"${u.id}" — click to insert`;
            img.addEventListener('click', () => {
                const editor = MacEditor.getInstance();
                const pos = editor.getPosition();
                editor.executeEdits('upload', [{
                    range: new monaco.Range(pos.lineNumber, pos.column, pos.lineNumber, pos.column),
                    text: `@"${u.id}"`,
                }]);
                editor.focus();
            });
            gallery.appendChild(img);
        });

        gallery.classList.add('visible');
    }

    function getUploadIds() {
        return _uploads.map(u => u.id);
    }

    return { openPicker, init, getUploadIds };
})();
```

Create `static/js/analyzer.js`:

```javascript
// analyzer.js — Debounced /api/analyze calls, Monaco provider wiring
'use strict';

const MacAnalyzer = (() => {
    let _timer = null;
    let _controller = null;
    let _lastHash = '';
    const DEBOUNCE_MS = 300;

    function _hash(str) {
        let h = 0;
        for (let i = 0; i < str.length; i++) {
            h = ((h << 5) - h + str.charCodeAt(i)) | 0;
        }
        return h;
    }

    function _setStatus(text, level) {
        const dot = document.getElementById('status-dot');
        const label = document.getElementById('status-text');
        dot.className = 'status-dot' + (level ? ` ${level}` : '');
        label.textContent = text;
    }

    async function _analyze() {
        const code = MacEditor.getCode();
        const hash = _hash(code);
        if (hash === _lastHash) return;
        _lastHash = hash;

        if (_controller) _controller.abort();
        _controller = new AbortController();

        _setStatus('Analyzing...', '');

        try {
            const res = await fetch('/api/analyze', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ code }),
                signal: _controller.signal,
            });

            if (!res.ok) {
                _setStatus('Ready', '');
                return;
            }

            const data = await res.json();
            _applyDiagnostics(data.diagnostics || []);

            const warnCount = (data.diagnostics || []).filter(d => d.severity === 'warning').length;
            const errCount = (data.diagnostics || []).filter(d => d.severity === 'error').length;

            if (errCount > 0) {
                _setStatus(`${errCount} error${errCount > 1 ? 's' : ''}`, 'error');
            } else if (warnCount > 0) {
                _setStatus(`${warnCount} warning${warnCount > 1 ? 's' : ''}`, 'warning');
            } else {
                _setStatus('Ready', '');
            }

            // Cache analysis for completions/hover
            MacAnalyzer._lastResult = data;
        } catch (err) {
            if (err.name !== 'AbortError') {
                _setStatus('Ready', '');
            }
        }
    }

    function _applyDiagnostics(diagnostics) {
        const model = MacEditor.getModel();
        if (!model) return;

        const markers = diagnostics
            .filter(d => d.source === 'user')
            .map(d => ({
                severity: d.severity === 'error'
                    ? monaco.MarkerSeverity.Error
                    : monaco.MarkerSeverity.Warning,
                message: d.message,
                startLineNumber: d.line,
                startColumn: d.col + 1,
                endLineNumber: d.line,
                endColumn: (d.endCol || d.col + 1) + 1,
            }));

        monaco.editor.setModelMarkers(model, 'mac-analyzer', markers);
    }

    function _registerProviders() {
        // Completion provider
        monaco.languages.registerCompletionItemProvider('mac', {
            triggerCharacters: ['.', '@'],
            provideCompletionItems: (model, position) => {
                const data = MacAnalyzer._lastResult;
                if (!data) return { suggestions: [] };

                const suggestions = [];

                // Symbols as completions
                (data.symbols || []).forEach(sym => {
                    let kind;
                    switch (sym.kind) {
                        case 'function': kind = monaco.languages.CompletionItemKind.Function; break;
                        case 'class':    kind = monaco.languages.CompletionItemKind.Class; break;
                        case 'variable': kind = monaco.languages.CompletionItemKind.Variable; break;
                        case 'field':    kind = monaco.languages.CompletionItemKind.Field; break;
                        default:         kind = monaco.languages.CompletionItemKind.Text;
                    }

                    suggestions.push({
                        label: sym.name,
                        kind,
                        detail: sym.type || '',
                        documentation: sym.description || '',
                        insertText: sym.name,
                    });
                });

                // Templates as completions (on @ trigger)
                const word = model.getWordUntilPosition(position);
                const lineContent = model.getLineContent(position.lineNumber);
                if (lineContent.charAt(word.startColumn - 2) === '@') {
                    (data.templates || []).forEach(t => {
                        suggestions.push({
                            label: t.name,
                            kind: monaco.languages.CompletionItemKind.Snippet,
                            detail: t.category || 'template',
                            documentation: t.description || '',
                            insertText: t.name,
                        });
                    });
                }

                return { suggestions };
            },
        });

        // Hover provider
        monaco.languages.registerHoverProvider('mac', {
            provideHover: (model, position) => {
                const data = MacAnalyzer._lastResult;
                if (!data) return null;

                const line = position.lineNumber;
                const col = position.column - 1;

                // Check symbols
                const sym = (data.symbols || []).find(s =>
                    s.line === line && col >= s.col && col <= (s.endCol || s.col + s.name.length)
                );

                if (sym) {
                    const parts = [`**${sym.name}**`];
                    if (sym.type) parts.push(`Type: \`${sym.type}\``);
                    if (sym.description) parts.push(sym.description);

                    return {
                        range: new monaco.Range(line, sym.col + 1, line, (sym.endCol || sym.col + sym.name.length) + 1),
                        contents: [{ value: parts.join('\n\n') }],
                    };
                }

                // Check references
                const ref = (data.references || []).find(r =>
                    r.line === line && col >= r.col && col <= (r.endCol || r.col)
                );

                if (ref) {
                    const parts = [`**${ref.defName}**`];
                    return {
                        range: new monaco.Range(line, ref.col + 1, line, (ref.endCol || ref.col) + 1),
                        contents: [{ value: parts.join('\n\n') }],
                    };
                }

                return null;
            },
        });

        // Signature help provider
        monaco.languages.registerSignatureHelpProvider('mac', {
            signatureHelpTriggerCharacters: ['(', ','],
            provideSignatureHelp: (model, position) => {
                const data = MacAnalyzer._lastResult;
                if (!data || !data.signatures) return null;

                const lineContent = model.getLineContent(position.lineNumber);
                const textBefore = lineContent.substring(0, position.column - 1);

                // Find the function name before the opening paren
                const match = textBefore.match(/(\w+)\s*\([^)]*$/);
                if (!match) return null;

                const funcName = match[1];
                const sig = data.signatures.find(s => s.name === funcName);
                if (!sig) return null;

                // Count commas to determine active parameter
                const afterParen = textBefore.substring(textBefore.lastIndexOf('(') + 1);
                const activeParam = (afterParen.match(/,/g) || []).length;

                return {
                    value: {
                        signatures: [{
                            label: `${sig.name}(${(sig.params || []).join(', ')})`,
                            parameters: (sig.params || []).map(p => ({ label: p })),
                            documentation: sig.description || '',
                        }],
                        activeSignature: 0,
                        activeParameter: activeParam,
                    },
                    dispose: () => {},
                };
            },
        });
    }

    function start() {
        _registerProviders();

        // Listen for editor changes
        const checkModel = setInterval(() => {
            const model = MacEditor.getModel();
            if (model) {
                clearInterval(checkModel);
                model.onDidChangeContent(() => {
                    clearTimeout(_timer);
                    _timer = setTimeout(_analyze, DEBOUNCE_MS);
                });
                // Initial analysis
                _analyze();
            }
        }, 100);
    }

    return { start, _lastResult: null };
})();
```

- [ ] **Step 5: Test Monaco loads in browser**

```bash
MAC_ROOT=~/workspace/mac-cpp python3 server.py --port 9002
# Open http://localhost:9002
# Monaco editor should load with the Hello World example
# Syntax highlighting should show keywords in orange, strings in green, etc.
```

- [ ] **Step 6: Commit**

```bash
git add static/app.js static/js/ static/mac.tmLanguage.json
git commit -m "feat: Monaco editor with mac language support and UI modules"
```

---

### Task 4: Run API Endpoint

**Files:**
- Modify: `server.py`

- [ ] **Step 1: Add rate limiter and subprocess semaphore to server.py**

Add after the constants block:

```python
EXEC_TIMEOUT = 5
MAX_CONCURRENT = 20
RATE_RUN_WINDOW = 60
RATE_RUN_MAX = 30
RATE_ANALYZE_WINDOW = 60
RATE_ANALYZE_MAX = 60
RATE_UPLOAD_WINDOW = 3600
RATE_UPLOAD_MAX = 100

class RateLimiter:
    """Thread-safe sliding-window rate limiter keyed by IP."""
    def __init__(self, window: int, max_reqs: int):
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

_rate_run = RateLimiter(RATE_RUN_WINDOW, RATE_RUN_MAX)
_rate_analyze = RateLimiter(RATE_ANALYZE_WINDOW, RATE_ANALYZE_MAX)
_rate_upload = RateLimiter(RATE_UPLOAD_WINDOW, RATE_UPLOAD_MAX)
_subprocess_sem = threading.Semaphore(MAX_CONCURRENT)
_ip_run_locks: dict[str, threading.Lock] = {}
_ip_run_locks_guard = threading.Lock()
```

- [ ] **Step 2: Add the /api/run endpoint**

Add helper functions and update `do_POST`:

```python
def _get_ip_run_lock(ip: str) -> threading.Lock:
    with _ip_run_locks_guard:
        if ip not in _ip_run_locks:
            _ip_run_locks[ip] = threading.Lock()
        return _ip_run_locks[ip]


def cleanup_mac_temp_files() -> None:
    import glob
    import shutil
    for d in glob.glob("/tmp/mac_effects_*"):
        try:
            pid = int(d.rsplit("_", 1)[-1])
            try:
                os.kill(pid, 0)
            except OSError:
                shutil.rmtree(d, ignore_errors=True)
        except (ValueError, Exception):
            pass


def cleanup_generated_dir() -> None:
    GENERATED_DIR.mkdir(parents=True, exist_ok=True)
    now = time.time()
    files = [(f.stat().st_mtime, f) for f in GENERATED_DIR.iterdir() if f.is_file()]
    files.sort()
    for mtime, f in files[:-50]:
        f.unlink(missing_ok=True)
    for mtime, f in files:
        if now - mtime > 3600:
            f.unlink(missing_ok=True)


def execute_code(code: str) -> dict:
    """Run Mac code and return stdout/stderr/exitCode/durationMs."""
    with tempfile.TemporaryDirectory(prefix="mac-play-") as tmp:
        script = Path(tmp) / "script.mac"
        script.write_text(code, encoding="utf-8")

        acquired = _subprocess_sem.acquire(timeout=5)
        if not acquired:
            return {"error": "Server busy. Try again shortly."}

        try:
            start = time.monotonic()
            result = subprocess.run(
                [str(MAC_BINARY), str(script)],
                cwd=str(MAC_ROOT),
                capture_output=True,
                text=True,
                timeout=EXEC_TIMEOUT,
                check=False,
            )
            elapsed = int((time.monotonic() - start) * 1000)
        except subprocess.TimeoutExpired:
            return {"error": "Execution timed out (5s limit).", "stdout": "", "stderr": "", "exitCode": 1, "durationMs": EXEC_TIMEOUT * 1000}
        finally:
            _subprocess_sem.release()

    cleanup_mac_temp_files()

    stdout = result.stdout or ""
    stderr = result.stderr or ""
    # Scrub temp file paths from error messages
    stderr = re.sub(r"/[^\s]+/script\.mac", "<script>", stderr)

    return {
        "stdout": stdout,
        "stderr": stderr,
        "exitCode": result.returncode,
        "durationMs": elapsed,
    }
```

Update `do_POST` in `PlaygroundHandler`:

```python
def do_POST(self) -> None:
    parsed = urlparse(self.path)
    path = unquote(parsed.path)
    client_ip = self.client_address[0]

    if path == "/api/run":
        if not _rate_run.allow(client_ip):
            json_response(self, {"error": "Rate limited. Try again in a minute."}, status=HTTPStatus.TOO_MANY_REQUESTS)
            return

        lock = _get_ip_run_lock(client_ip)
        if not lock.acquire(blocking=False):
            json_response(self, {"error": "A script is already running."}, status=HTTPStatus.TOO_MANY_REQUESTS)
            return

        try:
            body = read_json_body(self)
            code = str(body.get("code", ""))
            if len(code) > MAX_CODE_LENGTH:
                json_response(self, {"error": "Code too long (50KB limit)."}, status=HTTPStatus.BAD_REQUEST)
                return
            result = execute_code(code)
            status = HTTPStatus.OK if "error" not in result or result.get("exitCode") is not None else HTTPStatus.INTERNAL_SERVER_ERROR
            json_response(self, result, status=status)
        except Exception as e:
            json_response(self, {"error": str(e)}, status=HTTPStatus.BAD_REQUEST)
        finally:
            lock.release()
        return

    json_response(self, {"error": "Not found."}, status=HTTPStatus.NOT_FOUND)
```

- [ ] **Step 3: Test the run endpoint**

```bash
MAC_ROOT=~/workspace/mac-cpp python3 server.py --port 9002 &
curl -s -X POST http://localhost:9002/api/run \
  -H 'Content-Type: application/json' \
  -d '{"code": "print \"Hello from playground!\";\nprint 2 + 3;"}' | python3 -m json.tool
```

Expected:
```json
{
    "stdout": "Hello from playground!\n5\n",
    "stderr": "",
    "exitCode": 0,
    "durationMs": ...
}
```

- [ ] **Step 4: Test error handling**

```bash
curl -s -X POST http://localhost:9002/api/run \
  -H 'Content-Type: application/json' \
  -d '{"code": "print x;"}' | python3 -m json.tool
```

Expected: stderr contains an undefined variable error.

- [ ] **Step 5: Commit**

```bash
git add server.py
git commit -m "feat: /api/run endpoint with rate limiting and subprocess management"
```

---

### Task 5: Analyze API Endpoint

**Files:**
- Modify: `server.py`

- [ ] **Step 1: Add the /api/analyze endpoint to do_POST**

Add this block in `do_POST` before the "Not found" fallback:

```python
    if path == "/api/analyze":
        if not _rate_analyze.allow(client_ip):
            json_response(self, {"error": "Rate limited."}, status=HTTPStatus.TOO_MANY_REQUESTS)
            return

        try:
            body = read_json_body(self)
            code = str(body.get("code", ""))
            if len(code) > MAX_CODE_LENGTH:
                json_response(self, {"error": "Code too long."}, status=HTTPStatus.BAD_REQUEST)
                return

            acquired = _subprocess_sem.acquire(timeout=3)
            if not acquired:
                json_response(self, {"error": "Server busy."}, status=HTTPStatus.SERVICE_UNAVAILABLE)
                return

            try:
                with tempfile.TemporaryDirectory(prefix="mac-analyze-") as tmp:
                    script = Path(tmp) / "analyze.mac"
                    script.write_text(code, encoding="utf-8")

                    result = subprocess.run(
                        [str(MAC_BINARY), "--analyze", str(script)],
                        cwd=str(MAC_ROOT),
                        capture_output=True,
                        text=True,
                        timeout=5,
                        check=False,
                    )
            finally:
                _subprocess_sem.release()

            if result.returncode != 0 or not result.stdout.strip():
                json_response(self, {
                    "diagnostics": [],
                    "completions": [],
                    "signatures": [],
                    "semanticTokens": [],
                    "symbols": [],
                })
                return

            analysis = json.loads(result.stdout)
            json_response(self, analysis)

        except subprocess.TimeoutExpired:
            json_response(self, {"diagnostics": [], "symbols": []})
        except Exception as e:
            json_response(self, {"error": str(e)}, status=HTTPStatus.BAD_REQUEST)
        return
```

- [ ] **Step 2: Test the analyze endpoint**

```bash
curl -s -X POST http://localhost:9002/api/analyze \
  -H 'Content-Type: application/json' \
  -d '{"code": "val x = 42;\nprint x;\nprint y;"}' | python3 -m json.tool
```

Expected: JSON with `diagnostics` array containing a warning about undefined `y`, `symbols` with `x`, etc.

- [ ] **Step 3: Test in browser — type code and see squiggly underlines**

Open http://localhost:9002, type `print y;` and wait ~300ms. A red squiggly underline should appear under `y`.

- [ ] **Step 4: Commit**

```bash
git add server.py
git commit -m "feat: /api/analyze endpoint for real-time diagnostics"
```

---

### Task 6: Render API Endpoint

**Files:**
- Modify: `server.py`

- [ ] **Step 1: Add /api/render and /generated/* serving**

Add the render endpoint in `do_POST` (before the "Not found" fallback):

```python
    if path == "/api/render":
        if not _rate_run.allow(client_ip):
            json_response(self, {"error": "Rate limited."}, status=HTTPStatus.TOO_MANY_REQUESTS)
            return

        lock = _get_ip_run_lock(client_ip)
        if not lock.acquire(blocking=False):
            json_response(self, {"error": "A script is already running."}, status=HTTPStatus.TOO_MANY_REQUESTS)
            return

        try:
            body = read_json_body(self)
            code = str(body.get("code", ""))
            if len(code) > MAX_CODE_LENGTH:
                json_response(self, {"error": "Code too long."}, status=HTTPStatus.BAD_REQUEST)
                return

            # Rewrite save paths to our generated dir
            output_id = uuid.uuid4().hex[:12]
            patched_code = code
            # Replace => "filename.ext" with => "generated/output_id_filename.ext"
            def _rewrite_save(m):
                fname = m.group(1)
                ext = Path(fname).suffix or '.png'
                out_name = f"{output_id}_{fname}"
                out_path = GENERATED_DIR / out_name
                return f'=> "{out_path}"'

            patched_code = re.sub(r'=>\s*"([^"]+)"', _rewrite_save, patched_code)

            # Also handle .save("filename") calls
            def _rewrite_method_save(m):
                fname = m.group(1)
                out_name = f"{output_id}_{fname}"
                out_path = GENERATED_DIR / out_name
                return f'.save("{out_path}")'

            patched_code = re.sub(r'\.save\(\s*"([^"]+)"\s*\)', _rewrite_method_save, patched_code)

            result = execute_code(patched_code)

            # Find generated images
            images = []
            for f in GENERATED_DIR.iterdir():
                if f.is_file() and f.name.startswith(output_id):
                    fmt = f.suffix.lstrip('.') or 'png'
                    images.append({"url": f"/generated/{f.name}", "format": fmt})

            result["images"] = images
            cleanup_generated_dir()
            json_response(self, result)

        except Exception as e:
            json_response(self, {"error": str(e)}, status=HTTPStatus.BAD_REQUEST)
        finally:
            lock.release()
        return
```

Add generated file serving in `do_GET` (before the static file fallback):

```python
        if path.startswith("/generated/"):
            relative = path.removeprefix("/generated/").lstrip("/")
            candidate = safe_child_path(GENERATED_DIR, relative)
            if candidate is None:
                self.send_error(HTTPStatus.NOT_FOUND)
                return
            serve_file(self, candidate, cache_control="no-store")
            return

        if path.startswith("/assets/"):
            relative = path.removeprefix("/assets/").lstrip("/")
            candidate = safe_child_path(MAC_ROOT / "assets", relative)
            if candidate is None:
                self.send_error(HTTPStatus.NOT_FOUND)
                return
            serve_file(self, candidate)
            return
```

- [ ] **Step 2: Test render endpoint**

```bash
curl -s -X POST http://localhost:9002/api/render \
  -H 'Content-Type: application/json' \
  -d '{"code": "@blank 200x200 \"Test\" => \"test.png\";"}' | python3 -m json.tool
```

Expected: JSON with `images` array containing `{"url": "/generated/..._test.png", "format": "png"}`.

- [ ] **Step 3: Test in browser — click "Run & Render"**

Load the "Meme Basics" example and click "Run & Render". A meme image should appear in the render area.

- [ ] **Step 4: Commit**

```bash
git add server.py
git commit -m "feat: /api/render endpoint with image output and generated file serving"
```

---

### Task 7: Image Upload Endpoint

**Files:**
- Modify: `server.py`

- [ ] **Step 1: Add upload constants and TTL management**

Add constants:

```python
UPLOAD_MAX_BYTES = 5 * 1024 * 1024
UPLOAD_TTL = 60 * 60 * 48  # 48 hours of inactivity
UPLOAD_SWEEP_INTERVAL = 3600  # hourly cleanup
```

Add upload helpers:

```python
def _touch_upload(path: Path) -> None:
    """Reset the access time on an uploaded file to extend its TTL."""
    try:
        path.touch(exist_ok=True)
    except OSError:
        pass


def cleanup_uploads_dir() -> None:
    """Remove uploads that haven't been accessed in UPLOAD_TTL seconds."""
    if not UPLOADS_DIR.is_dir():
        return
    now = time.time()
    for f in UPLOADS_DIR.iterdir():
        if f.is_file() and f.name != '.gitkeep':
            if now - f.stat().st_mtime > UPLOAD_TTL:
                f.unlink(missing_ok=True)


def _start_cleanup_timer() -> None:
    """Run cleanup sweeps periodically in a daemon thread."""
    def _sweep():
        while True:
            time.sleep(UPLOAD_SWEEP_INTERVAL)
            cleanup_uploads_dir()
            cleanup_generated_dir()

    t = threading.Thread(target=_sweep, daemon=True)
    t.start()
```

Call `_start_cleanup_timer()` in `main()` before `server.serve_forever()`.

- [ ] **Step 2: Add upload and upload-serving endpoints**

Add to `do_POST` (before "Not found"):

```python
    if path == "/api/upload":
        if not _rate_upload.allow(client_ip):
            json_response(self, {"error": "Upload limit reached. Try again later."}, status=HTTPStatus.TOO_MANY_REQUESTS)
            return

        content_type = self.headers.get("Content-Type", "")
        if "multipart/form-data" not in content_type:
            json_response(self, {"error": "Expected multipart upload."}, status=HTTPStatus.BAD_REQUEST)
            return

        length = int(self.headers.get("Content-Length", 0))
        if length > UPLOAD_MAX_BYTES:
            json_response(self, {"error": "File too large (5MB limit)."}, status=HTTPStatus.BAD_REQUEST)
            return

        try:
            # Parse multipart boundary
            boundary = content_type.split("boundary=")[-1].strip()
            raw = self.rfile.read(length)

            # Extract file data between boundaries
            parts = raw.split(f"--{boundary}".encode())
            file_data = None
            file_ext = ".png"

            for part in parts:
                if b"filename=" in part:
                    # Extract filename for extension
                    header_end = part.find(b"\r\n\r\n")
                    if header_end < 0:
                        continue
                    header_text = part[:header_end].decode("utf-8", errors="replace")
                    fname_match = re.search(r'filename="([^"]+)"', header_text)
                    if fname_match:
                        ext = Path(fname_match.group(1)).suffix.lower()
                        if ext in {".png", ".jpg", ".jpeg", ".gif"}:
                            file_ext = ext

                    file_data = part[header_end + 4:]
                    # Strip trailing boundary markers
                    if file_data.endswith(b"\r\n"):
                        file_data = file_data[:-2]
                    break

            if file_data is None:
                json_response(self, {"error": "No file found in upload."}, status=HTTPStatus.BAD_REQUEST)
                return

            upload_id = uuid.uuid4().hex[:12]
            dest = UPLOADS_DIR / f"{upload_id}{file_ext}"
            dest.write_bytes(file_data)

            json_response(self, {
                "id": f"user.{upload_id}",
                "previewUrl": f"/api/upload/{upload_id}",
            })

        except Exception as e:
            json_response(self, {"error": f"Upload failed: {e}"}, status=HTTPStatus.INTERNAL_SERVER_ERROR)
        return
```

Add to `do_GET` (before static file fallback):

```python
        if path.startswith("/api/upload/"):
            upload_id = path.removeprefix("/api/upload/").strip("/")
            if not re.match(r'^[a-f0-9]{12}$', upload_id):
                self.send_error(HTTPStatus.NOT_FOUND)
                return
            # Find the file regardless of extension
            for f in UPLOADS_DIR.iterdir():
                if f.stem == upload_id:
                    _touch_upload(f)  # Reset TTL on access
                    serve_file(self, f, cache_control="public, max-age=600")
                    return
            self.send_error(HTTPStatus.NOT_FOUND)
            return
```

- [ ] **Step 3: Test upload endpoint**

```bash
# Create a tiny test image
convert -size 100x100 xc:red /tmp/test_upload.png 2>/dev/null || python3 -c "
from pathlib import Path
# 1x1 red PNG
Path('/tmp/test_upload.png').write_bytes(bytes.fromhex(
    '89504e470d0a1a0a0000000d49484452000000010000000108020000009001'
    '2e00000000c4944415478da6260f8cf0000000200016598c30000000049454e44ae426082'
))"

curl -s -X POST http://localhost:9002/api/upload \
  -F "file=@/tmp/test_upload.png" | python3 -m json.tool
```

Expected: `{"id": "user.abc123def456", "previewUrl": "/api/upload/abc123def456"}`

- [ ] **Step 4: Wire up uploads in browser and test**

Click "Upload" in the top bar, select an image. The gallery should appear below the editor with a thumbnail. Click the thumbnail to insert `@"user.xxx"` into the editor.

- [ ] **Step 5: Commit**

```bash
git add server.py
git commit -m "feat: image upload with access-based TTL and gallery"
```

---

### Task 8: Load Test

**Files:**
- Create: `loadtest.py`

- [ ] **Step 1: Create loadtest.py**

```python
#!/usr/bin/env python3
"""Load test for Mac Playground — 30 TPS sustained for 60s."""
from __future__ import annotations

import argparse
import json
import statistics
import sys
import threading
import time
import urllib.request
import urllib.error

SIMPLE_CODE = 'val x = 42;\nprint x;\nprint x * 2;'
ANALYZE_CODE = 'val name = "Mac";\nfun greet(x) { return "Hi " + x; }\nprint greet(name);'

def _post_json(url: str, payload: dict, timeout: float = 10) -> tuple[int, float]:
    """POST JSON, return (status_code, latency_ms)."""
    data = json.dumps(payload).encode()
    req = urllib.request.Request(
        url,
        data=data,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    start = time.monotonic()
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            resp.read()
            return resp.status, (time.monotonic() - start) * 1000
    except urllib.error.HTTPError as e:
        return e.code, (time.monotonic() - start) * 1000
    except Exception:
        return 0, (time.monotonic() - start) * 1000


def _get(url: str, timeout: float = 5) -> int:
    """GET a URL, return status code."""
    try:
        with urllib.request.urlopen(url, timeout=timeout) as resp:
            resp.read()
            return resp.status
    except Exception:
        return 0


def run_loadtest(base_url: str, tps: int, duration: int, studio_url: str, docs_url: str):
    print(f"Target: {base_url}")
    print(f"Rate: {tps} TPS for {duration}s")
    print(f"Monitoring: studio={studio_url}, docs={docs_url}")
    print()

    latencies: list[float] = []
    errors = 0
    total = 0
    lock = threading.Lock()

    def _fire(i: int):
        nonlocal errors, total
        if i % 3 == 0:
            # Every 3rd request is an analyze
            status, lat = _post_json(f"{base_url}/api/analyze", {"code": ANALYZE_CODE})
        else:
            status, lat = _post_json(f"{base_url}/api/run", {"code": SIMPLE_CODE})

        with lock:
            total += 1
            latencies.append(lat)
            if status >= 500 or status == 0:
                errors += 1

    start = time.monotonic()
    req_num = 0
    interval = 1.0 / tps

    while time.monotonic() - start < duration:
        t = threading.Thread(target=_fire, args=(req_num,))
        t.start()
        req_num += 1
        # Pace to target TPS
        elapsed = time.monotonic() - start
        expected = req_num * interval
        if expected > elapsed:
            time.sleep(expected - elapsed)

    # Wait for stragglers
    time.sleep(3)

    # Check sibling services
    studio_ok = _get(f"{studio_url}/health") == 200
    docs_ok = _get(docs_url) == 200

    # Report
    latencies.sort()
    p50 = latencies[len(latencies) // 2] if latencies else 0
    p95 = latencies[int(len(latencies) * 0.95)] if latencies else 0
    p99 = latencies[int(len(latencies) * 0.99)] if latencies else 0

    print(f"{'='*50}")
    print(f"Requests sent: {total}")
    print(f"5xx errors:    {errors}")
    print(f"Latency p50:   {p50:.0f}ms")
    print(f"Latency p95:   {p95:.0f}ms")
    print(f"Latency p99:   {p99:.0f}ms")
    print(f"Studio health: {'OK' if studio_ok else 'FAILED'}")
    print(f"Docs health:   {'OK' if docs_ok else 'FAILED'}")
    print(f"{'='*50}")

    passed = (
        errors == 0
        and p95 < 2000
        and studio_ok
        and docs_ok
    )

    if passed:
        print("RESULT: PASS")
    else:
        print("RESULT: FAIL")
        if errors > 0: print(f"  - {errors} server errors")
        if p95 >= 2000: print(f"  - p95 latency {p95:.0f}ms exceeds 2000ms")
        if not studio_ok: print("  - Studio not responding")
        if not docs_ok: print("  - Docs not responding")

    return 0 if passed else 1


def main():
    parser = argparse.ArgumentParser(description="Mac Playground Load Test")
    parser.add_argument("--url", default="http://localhost:9002", help="Playground base URL")
    parser.add_argument("--tps", type=int, default=30, help="Target requests per second")
    parser.add_argument("--duration", type=int, default=60, help="Test duration in seconds")
    parser.add_argument("--studio", default="http://localhost:9001", help="Studio URL for health check")
    parser.add_argument("--docs", default="http://localhost:3000", help="Docs URL for health check")
    args = parser.parse_args()
    sys.exit(run_loadtest(args.url, args.tps, args.duration, args.studio, args.docs))


if __name__ == "__main__":
    main()
```

- [ ] **Step 2: Test locally (reduced rate)**

```bash
python3 loadtest.py --url http://localhost:9002 --tps 5 --duration 10 --studio http://localhost:9001 --docs http://localhost:3000
```

Expected: PASS with low latencies (studio/docs checks will fail locally — that's fine, they're for the server).

- [ ] **Step 3: Commit**

```bash
git add loadtest.py
git commit -m "feat: load test script for 30 TPS sustained validation"
```

---

### Task 9: Deployment

**Files:**
- No new files — server setup and configuration

- [ ] **Step 1: Push repo to GitHub**

```bash
git push origin main
```

- [ ] **Step 2: Add RIGBOX_SSH_KEY secret to the repo**

```bash
gh secret set RIGBOX_SSH_KEY --repo mac-studio-meme/playground < ~/.ssh/id_ed25519
```

- [ ] **Step 3: Clone playground on Rigbox**

```bash
ssh base-uvvrblcc-1yvx9yo7@eu-west-1.rigbox.dev 'git clone git@github.com:mac-studio-meme/playground.git ~/playground'
```

- [ ] **Step 4: Create systemd service**

```bash
ssh base-uvvrblcc-1yvx9yo7@eu-west-1.rigbox.dev 'sudo tee /etc/systemd/system/rigbox-svc-mac-playground.service > /dev/null << EOF
[Unit]
Description=Rigbox Service: mac-playground
After=network.target

[Service]
Type=simple
User=developer
WorkingDirectory=/home/developer/playground
Environment=MAC_ROOT=/home/developer/mac-cpp-ui
ExecStart=/usr/bin/python3 /home/developer/playground/server.py --host 0.0.0.0 --port 9002
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
EOF
'
```

- [ ] **Step 5: Update deploy.sh to include playground**

```bash
ssh base-uvvrblcc-1yvx9yo7@eu-west-1.rigbox.dev 'cat > ~/deploy.sh << '\''SCRIPT'\''
#!/bin/bash
set -e

echo "=== Pulling mac-cpp ==="
cd ~/mac-cpp-ui
git fetch origin main
git reset --hard origin/main

echo "=== Pulling studio ==="
cd ~/studio
git fetch origin main
git reset --hard origin/main

echo "=== Pulling playground ==="
cd ~/playground
git fetch origin main
git reset --hard origin/main

echo "=== Building mac binary ==="
cd ~/mac-cpp-ui
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release 2>&1 | tail -1
cmake --build build 2>&1 | tail -3

echo "=== Building docs ==="
rm -rf docs/reference/book
cd docs/reference && mdbook build 2>&1 | tail -1
cd ~/mac-cpp-ui

echo "=== Reloading and restarting services ==="
sudo systemctl daemon-reload
sudo systemctl restart rigbox-svc-mac-ui.service
sudo systemctl restart rigbox-svc-mac-docs.service
sudo systemctl restart rigbox-svc-mac-playground.service
sleep 2

FAILED=0
if curl -sf http://localhost:9001/health > /dev/null; then
    echo "  studio: OK"
else
    echo "  studio: FAILED"
    sudo journalctl -u rigbox-svc-mac-ui --no-pager -n 5
    FAILED=1
fi

if curl -sf http://localhost:3000/ > /dev/null; then
    echo "  docs: OK"
else
    echo "  docs: FAILED"
    sudo journalctl -u rigbox-svc-mac-docs --no-pager -n 5
    FAILED=1
fi

if curl -sf http://localhost:9002/api/status > /dev/null; then
    echo "  playground: OK"
else
    echo "  playground: FAILED"
    sudo journalctl -u rigbox-svc-mac-playground --no-pager -n 10
    FAILED=1
fi

if [ $FAILED -eq 0 ]; then
    echo "=== Deploy complete ==="
else
    echo "=== Deploy FAILED ==="
    exit 1
fi
SCRIPT
'
```

- [ ] **Step 6: Enable and start the service**

```bash
ssh base-uvvrblcc-1yvx9yo7@eu-west-1.rigbox.dev 'sudo systemctl daemon-reload && sudo systemctl enable rigbox-svc-mac-playground.service'
```

- [ ] **Step 7: Run deploy.sh to verify everything starts**

```bash
ssh base-uvvrblcc-1yvx9yo7@eu-west-1.rigbox.dev 'bash ~/deploy.sh'
```

Expected output includes `playground: OK`.

- [ ] **Step 8: Configure play.macstudio.meme subdomain**

Coordinate with Rigbox to route `play.macstudio.meme` to port 9002 on the same instance. This follows the same pattern as `macstudio.meme` (port 9001) and `docs.macstudio.meme` (port 3000).

- [ ] **Step 9: Run load test on server**

```bash
ssh base-uvvrblcc-1yvx9yo7@eu-west-1.rigbox.dev 'cd ~/playground && python3 loadtest.py --url http://localhost:9002 --tps 30 --duration 60 --studio http://localhost:9001 --docs http://localhost:3000'
```

Expected: PASS — p95 < 2s, no 5xx errors, studio and docs remain responsive.

- [ ] **Step 10: Commit deploy workflow trigger verification**

Check that the GitHub Actions deploy ran successfully after the push:

```bash
gh run list --repo mac-studio-meme/playground --limit 1
```

Expected: `completed success`
