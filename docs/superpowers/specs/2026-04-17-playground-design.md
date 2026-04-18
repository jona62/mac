# Mac Playground Design Spec

## Overview

A browser-based coding playground for the Mac language, hosted at `play.macstudio.meme`. Code-first editor with real-time analysis (diagnostics, autocomplete, hover), stdout/stderr display, and opt-in meme/GIF rendering. Shares the macstudio.meme design language (warm dark tones, editorial typography).

## Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Audience | Learners + power users | Approachable examples, no dumbed-down features |
| Repo | `mac-studio-meme/playground` (private) | Same pattern as studio; deploy.sh pulls latest mac-cpp for binary |
| Editor | Monaco Editor | TextMate grammar reuse, built-in LSP-like APIs, familiar UX |
| Layout | Vertical split | Editor left (60%), output + render stacked right (40%) |
| Rendering | Server-side, opt-in | "Run" for stdout only, "Run & Render" for images |
| Sharing | URL fragment encoding | LZ-compressed code in `#code=...`, no server storage |
| Examples | Dropdown with 8 curated snippets | Loaded on selection, first example is default |
| Image uploads | Global pool with access-based TTL | 48h inactivity expiry, any access resets TTL |

## Architecture

### Stack

- **Frontend:** Single HTML page + CSS + JS. Monaco Editor loaded from CDN. No build step, no framework.
- **Backend:** Python stdlib HTTP server (`http.server`). Shells out to `mac` binary for execution and `mac --analyze` for analysis.
- **Hosting:** Rigbox (`eu-west-1.rigbox.dev`), alongside studio (port 9001) and docs (port 3000). Playground runs on port 9002.

### Data Flow

```
Browser (Monaco) → POST /api/run     → mac binary      → { stdout, stderr, exitCode, durationMs }
                 → POST /api/render  → mac binary      → { stdout, stderr, exitCode, durationMs, images }
                 → POST /api/analyze → mac --analyze   → { diagnostics, completions, signatures, semanticTokens, symbols }
                 → POST /api/upload  → disk write      → { id, previewUrl }
```

### Repo Structure

```
mac-studio-meme/playground/
├── server.py              # Python HTTP server
├── run.sh                 # Startup script (builds mac binary, starts server)
├── loadtest.py            # Load test script (30 TPS sustained)
├── .gitignore
├── static/
│   ├── index.html         # SPA entry point
│   ├── styles.css         # Design tokens + layout
│   ├── app.js             # App initialization, Monaco setup, API glue
│   ├── js/
│   │   ├── editor.js      # Monaco configuration, theme, grammar loading
│   │   ├── analyzer.js    # Debounced analysis, marker/completion/hover wiring
│   │   ├── runner.js      # Run/Render API calls, output display
│   │   ├── sharing.js     # URL encode/decode, clipboard
│   │   ├── uploads.js     # Image upload, gallery management
│   │   └── examples.js    # Example definitions and loader
│   └── mac.tmLanguage.json # Copied from mac-lang/syntaxes/
├── generated/             # Rendered output images (gitignored)
├── uploads/               # User-uploaded images (gitignored)
└── .github/
    └── workflows/
        └── deploy.yml     # SSH deploy to Rigbox
```

## UI Components

### Top Bar

- **Brand:** "Mac Playground" — Cormorant Garamond for "Mac", IBM Plex Mono for "Playground"
- **Examples dropdown:** 8 curated snippets, selecting one loads it into the editor
- **Upload button:** Opens file picker for image uploads (PNG, JPG, GIF, max 5MB)
- **Run button:** Rust accent (`#c44b2b`). Executes code, shows stdout/stderr.
- **Run & Render button:** Teal (`#3aaa8a`). Executes code and displays generated images.
- **Share button:** Compresses code into URL fragment, copies to clipboard.
- **Docs link:** Links to `docs.macstudio.meme`

### Left Panel — Editor (60% width)

- Monaco Editor instance
- Dark theme matching studio palette:
  - Background: `#1a1918`
  - Keyword: `#e06040`
  - String: `#98c379`
  - Number: `#d19a66`
  - Comment: `#5c6370` italic
  - Operator: `#c678dd`
  - Function: `#61afef`
  - Class: `#e5c07b`
  - Constant: `#56b6c2`
  - Template: `#e06040` bold
  - Punctuation: `#8a8070`
- Diagnostics shown as squiggly underlines (from `/api/analyze`)
- Autocomplete on `.` and `@` triggers
- Signature help on `(` and `,` triggers
- Hover info showing type and description
- Semantic token coloring (variables, functions, classes)

### Right Panel — Output (40% width)

**Top section — Console:**
- **Stdout area:** Monospace, green-tinted (`#98c379`) text. Shows `print` output.
- **Stderr area:** Below stdout, red-tinted (`#c44b2b`) text. Shows errors and warnings.
- Execution duration badge (e.g., "42ms")

**Bottom section — Render (shown when "Run & Render" produces images):**
- Generated PNG/GIF displayed inline
- Download button for the rendered image
- Hidden when no render output exists

### Status Bar (bottom edge)

- Analyzer status: "Ready" / "Analyzing..." / "2 warnings"
- Language version badge: `v0.7.0`

## Design Language

Inherits from macstudio.meme:

**Fonts:**
- Display: Bricolage Grotesque (UI elements, buttons)
- Serif: Cormorant Garamond (brand/headings)
- Mono: IBM Plex Mono (editor, output, badges)

**Colors (CSS variables):**
```css
--bg: #1a1918;
--surface: #211f1d;
--surface-hover: #292724;
--surface-active: #332f2c;
--ink: #e8e0d4;
--ink-muted: rgba(232, 224, 212, 0.5);
--accent: #c44b2b;
--accent-hover: #d4593a;
--teal: #3aaa8a;
--gold: #c8992a;
--divider: rgba(232, 224, 212, 0.1);
--radius: 10px;
--radius-sm: 6px;
```

## API

### Endpoints

| Method | Path | Request | Response |
|--------|------|---------|----------|
| `GET` | `/api/status` | — | `{ ok, version, binaryReady }` |
| `POST` | `/api/run` | `{ code }` | `{ stdout, stderr, exitCode, durationMs }` |
| `POST` | `/api/render` | `{ code }` | `{ stdout, stderr, exitCode, durationMs, images: [{ url, format }] }` |
| `POST` | `/api/analyze` | `{ code }` | `{ diagnostics, completions, signatures, semanticTokens, symbols }` |
| `POST` | `/api/upload` | multipart file | `{ id, previewUrl }` |
| `GET` | `/api/upload/:id` | — | image binary (resets TTL) |
| `GET` | `/generated/*` | — | rendered image binary |

### Rate Limits

| Endpoint | Limit |
|----------|-------|
| `/api/run`, `/api/render` | 30 requests/IP/minute |
| `/api/analyze` | 60 requests/IP/minute |
| `/api/upload` | 100 uploads/IP/hour |

### Concurrency

- **Max 20 concurrent `mac` subprocesses** across all endpoints (run + render + analyze)
- **One concurrent execution per IP** for `/api/run` and `/api/render` — second request gets 429
- Additional requests beyond subprocess limit get 429

## Image Uploads

- Max 5MB per image, PNG/JPG/GIF accepted
- Uploaded images stored in `uploads/` with UUID filenames
- Referenced in code via template ID (e.g., `@"user.abc123"`)
- **Access-based TTL: 48 hours of inactivity**
  - Any access (uploader revisiting, shared link opened, image served) resets the TTL
  - Images are global — not segmented by user/session
  - Hourly cleanup sweep removes expired images
- Shared links encode referenced image IDs alongside the code
- If an image has expired when a shared link is opened, a placeholder is shown: "Image expired — re-upload to restore"

## Sharing

- Code is LZ-compressed (using `lz-string` library) and base64-encoded into the URL fragment: `play.macstudio.meme/#code=<encoded>&images=id1,id2`
- On page load, if `#code=` is present, decode and populate the editor
- If `&images=` is present, verify each image ID is still available via `/api/upload/:id`
- "Share" button copies the encoded URL to clipboard with a brief toast confirmation
- All decoding happens client-side — no server round-trip to load shared code

## Monaco + Analyzer Integration

### Syntax Highlighting (static, immediate)

- `mac.tmLanguage.json` loaded via `monaco-textmate` + `vscode-oniguruma` WASM
- Provides immediate keyword/string/comment coloring without server calls
- Falls back to a simpler regex tokenizer while WASM loads

### Analyzer Integration (dynamic, debounced)

On each keystroke, debounce 300ms, then `POST /api/analyze`:

| Analyzer output | Monaco API | Result |
|----------------|------------|--------|
| `diagnostics` | `monaco.editor.setModelMarkers()` | Squiggly underlines with messages |
| `completions` | `registerCompletionItemProvider` (triggers: `.`, `@`) | Autocomplete dropdown |
| `signatures` | `registerSignatureHelpProvider` (triggers: `(`, `,`) | Parameter hints |
| `semanticTokens` | `registerDocumentSemanticTokensProvider` | Rich coloring beyond grammar |
| `symbols` | `registerHoverProvider` | Type + description on hover |

### Client-Side Optimizations

- **AbortController:** Cancel in-flight analysis requests when new keystrokes arrive
- **Content hashing:** Skip requests if code hasn't changed since last analysis
- **Debounce: 300ms** after last keystroke before sending analysis request

## Examples

8 curated snippets in the dropdown, inlined in the HTML:

1. **Hello World** — `print`, `val`, basic expressions
2. **Meme Basics** — `@two_panel` with top/bottom text, save operator
3. **Styles & Effects** — `style` block + effect pipeline (`|> sepia |> border`)
4. **GIF Animation** — `gif loop { }` with frames and timing
5. **Functional Pipelines** — `map`, `reduce`, pipe operator on arrays
6. **Pattern Matching** — `enum` definition + `match` with destructuring
7. **String Interpolation** — `"Hello, {name}!"` with expressions in strings
8. **Data-Driven Memes** — Array of data piped through `map` to generate memes

First example loads by default. Each is 5-15 lines, self-contained, and runs successfully.

## Performance & Shared Machine

The Rigbox instance runs three services concurrently:
- **Studio** — port 9001 (meme editor)
- **Docs** — port 3000 (mdBook static site)
- **Playground** — separate port (code playground)

### Safeguards

- **Execution timeout: 5 seconds** — playground scripts are small; runaway loops get killed fast
- **Max 20 concurrent subprocesses** — prevents CPU starvation of studio/docs
- **One run/render per IP** — spamming "Run" doesn't multiply load
- **Analyzer is lightweight** — parses AST without executing code, typically <100ms
- **Generated image cleanup** — hourly sweep, max 50 files, 1-hour TTL
- **Upload cleanup** — hourly sweep, 48-hour access-based TTL

### Load Test

`loadtest.py` script validates the shared machine handles sustained load:

- **Target: 30 TPS** against `/api/run` and `/api/analyze` (mixed)
- **Duration: 60 seconds**
- **Monitors:** CPU usage, disk I/O, response latency (p50, p95, p99)
- **Pass criteria:**
  - p95 latency < 2 seconds
  - No 5xx errors
  - Studio (`macstudio.meme/health`) and docs (`docs.macstudio.meme`) remain responsive throughout
  - CPU doesn't sustain >80% for more than 10 seconds

## Deployment

### New Repo Setup

1. Create `mac-studio-meme/playground` (private)
2. Add `RIGBOX_SSH_KEY` secret

### Server Setup (Rigbox)

1. Clone `~/playground` on Rigbox
2. Create systemd service `rigbox-svc-mac-playground`:
   ```ini
   [Service]
   Environment=MAC_ROOT=/home/developer/mac-cpp-ui
   ExecStart=/usr/bin/python3 /home/developer/playground/server.py --host 0.0.0.0 --port 9002
   ```
3. Configure subdomain `play.macstudio.meme` → playground port
4. Update `deploy.sh` to pull playground repo and restart its service

### deploy.sh Changes

Add alongside existing mac-cpp and studio pulls:
```bash
echo "=== Pulling playground ==="
cd ~/playground
git fetch origin main
git reset --hard origin/main
```

Add service restart:
```bash
sudo systemctl restart rigbox-svc-mac-playground.service
```

Add health check:
```bash
if curl -sf http://localhost:9002/api/status > /dev/null; then
    echo "  playground: OK"
else
    echo "  playground: FAILED"
fi
```
