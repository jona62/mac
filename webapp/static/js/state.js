// Mac Studio — State, constants, and utility helpers

const DEFAULT_LIMITS = {
  width: { min: 240, max: 1200 },
  height: { min: 240, max: 1200 },
  duration: { min: 80, max: 2500 },
  padding: { min: 0, max: 60 },
  border: { min: 0, max: 24 },
  outline: { min: 0, max: 10 },
  shadow: { min: 0, max: 10 },
  fontSizePx: { min: 8, max: 240 },
};

const FONT_SIZE_OPTIONS = [
  { id: "auto", label: "Auto" },
  { id: "sm", label: "Small" },
  { id: "md", label: "Medium" },
  { id: "lg", label: "Large" },
  { id: "xlg", label: "Extra Large" },
  { id: "custom", label: "Custom" },
];

const FALLBACK_LAYOUTS = [
  { id: "single", name: "Single", slotCount: 1, description: "One full-canvas scene." },
  { id: "beside", name: "Beside", slotCount: 2, description: "Two slots side by side." },
  { id: "stack", name: "Stack", slotCount: 2, description: "Two slots stacked vertically." },
  { id: "grid2x2", name: "Grid 2x2", slotCount: 4, description: "Four slots in a 2x2 comparison grid." },
];

const FALLBACK_STYLE_PRESETS = [
  { id: "cinematic", name: "Cinematic", description: "Soft white type with a restrained shadow.",
    style: { color: "#FFFFFF", outline: 2, outlineColor: "#111111", shadow: 4, shadowColor: "#00000088", fontSize: "md" } },
  { id: "panic", name: "Panic", description: "Red alert styling with heavier edges.",
    style: { color: "#FF0000", outline: 5, outlineColor: "#440000", shadow: 3, shadowColor: "#00000099", fontSize: "lg" } },
  { id: "chill", name: "Chill", description: "Terminal green with crisp outline.",
    style: { color: "#00FF41", outline: 3, outlineColor: "#003300", fontSize: "md" } },
  { id: "shout", name: "Shout", description: "Loud white all-caps energy.",
    style: { color: "#FFFFFF", outline: 4, outlineColor: "#000000", shadow: 3, shadowColor: "#00000099", fontSize: "xlg" } },
  { id: "whisper", name: "Whisper", description: "Muted grey with a fine outline.",
    style: { color: "#CCCCCC", outline: 1, outlineColor: "#333333", fontSize: "sm" } },
];

const $ = (id) => document.getElementById(id);
const TEXT_LAYER_ANCHORS = ["top", "center", "bottom"];

const state = {
  metadata: { templates: [], effects: [], effectDefinitions: [], layouts: [], stylePresets: [], limits: { ...DEFAULT_LIMITS } },
  canvas: { width: 720, height: 720 },
  output: { format: "png" },
  scenes: [],
  selectedSceneIndex: 0,
  selectedSlotIndex: 0,
  selectedTextLayerId: null,
  editingTextLayerId: null,
  textLayerSeq: 1,
  previewTimer: 0,
  previewSeq: 0,
  previewMode: "live",
  previewAbortController: null,
  stageMode: "preview",
  stageAssetUrl: "",
  stageLabel: "Selected scene preview",
  lastPreviewSceneIndex: null,
  script: "// Studio script will appear here.\n",
  backendCompatibility: "unknown",
  status: { line: "Loading studio…", meta: "Scene preview is debounced and Mac-powered.", kind: "normal" },
  isPreviewing: false,
  isExporting: false,
  lastExport: null,
  draggingTextLayerId: null,
};

// ── Utility helpers ──

function clone(value) { return JSON.parse(JSON.stringify(value)); }

function clamp(value, min, max) {
  const number = Number(value);
  if (!Number.isFinite(number)) return min;
  return Math.max(min, Math.min(max, Math.round(number)));
}

function esc(text) {
  return String(text).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;");
}

function escAttr(text) { return esc(text).replace(/'/g, "&#39;"); }

function cssUrl(url) {
  return String(url || "").replace(/["'()\\\n\r]/g, (c) => (c === "\n" || c === "\r") ? "" : `\\${c}`);
}

function formatDuration(ms) { return ms >= 1000 ? `${(ms / 1000).toFixed(1)}s` : `${ms}ms`; }

function formatBytes(bytes) {
  const v = Math.max(0, Number(bytes) || 0);
  return v < 1024 ? `${v} B` : `${Math.max(1, Math.round(v / 1024))} KB`;
}

function normalizeHex(value, fallback, allowAlpha = false) {
  const text = String(value || "").trim().toUpperCase();
  const pattern = allowAlpha ? /^#[0-9A-F]{6}([0-9A-F]{2})?$/ : /^#[0-9A-F]{6}$/;
  return pattern.test(text) ? text : fallback;
}

function setStatus(line, meta, kind = "normal") { state.status = { line, meta, kind }; }

function selectedScene() { return state.scenes[state.selectedSceneIndex]; }
function selectedSlot() { const s = selectedScene(); return s ? s.slots[state.selectedSlotIndex] || null : null; }
function nextTextLayerId() {
  const id = `text-layer-${state.textLayerSeq}`;
  state.textLayerSeq += 1;
  return id;
}
function normalizeTextLayer(layer = {}) {
  return {
    id: layer.id || nextTextLayerId(),
    kind: layer.kind === "anchored" ? "anchored" : "free",
    anchor: TEXT_LAYER_ANCHORS.includes(layer.anchor) ? layer.anchor : null,
    content: String(layer.content || ""),
    x: Number.isFinite(Number(layer.x)) ? Number(layer.x) : 0,
    y: Number.isFinite(Number(layer.y)) ? Number(layer.y) : 0,
    fontSizeMode: FONT_SIZE_OPTIONS.some((o) => o.id === layer.fontSizeMode) ? layer.fontSizeMode : "",
    fontSizePx: Number.isFinite(Number(layer.fontSizePx)) ? Number(layer.fontSizePx) : 64,
  };
}
function textLayerById(slot, id = state.selectedTextLayerId) {
  return (slot && slot.textLayers || []).find((layer) => layer.id === id) || null;
}
function selectedTextLayer() { return textLayerById(selectedSlot(), state.selectedTextLayerId); }
function anchoredLayerForSlot(slot, anchor) {
  return (slot && slot.textLayers || []).find((layer) => layer.kind === "anchored" && layer.anchor === anchor) || null;
}
function slotCanvasRect(layoutKind, slotIndex, canvas = state.canvas) {
  const width = Number(canvas.width) || 0;
  const height = Number(canvas.height) || 0;
  if (layoutKind === "beside") {
    const slotWidth = Math.max(1, Math.floor(width / 2));
    return { x: slotIndex === 1 ? slotWidth : 0, y: 0, width: slotWidth, height };
  }
  if (layoutKind === "stack") {
    const slotHeight = Math.max(1, Math.floor(height / 2));
    return { x: 0, y: slotIndex === 1 ? slotHeight : 0, width, height: slotHeight };
  }
  if (layoutKind === "grid2x2") {
    const slotWidth = Math.max(1, Math.floor(width / 2));
    const slotHeight = Math.max(1, Math.floor(height / 2));
    return {
      x: (slotIndex % 2) * slotWidth,
      y: Math.floor(slotIndex / 2) * slotHeight,
      width: slotWidth,
      height: slotHeight,
    };
  }
  return { x: 0, y: 0, width, height };
}
function selectedSlotCanvasRect() {
  const scene = selectedScene();
  return slotCanvasRect(scene ? scene.layout.kind : "single", state.selectedSlotIndex, state.canvas);
}
function anchorLocalPoint(anchor, bounds) {
  const width = Math.max(1, Number(bounds && bounds.width) || 0);
  const height = Math.max(1, Number(bounds && bounds.height) || 0);
  if (anchor === "top") return { x: Math.round(width / 2), y: Math.round(height * 0.14) };
  if (anchor === "bottom") return { x: Math.round(width / 2), y: Math.round(height * 0.86) };
  return { x: Math.round(width / 2), y: Math.round(height / 2) };
}
function textLayerLocalPoint(layer, bounds) {
  if (layer && layer.kind === "anchored" && layer.anchor) {
    return anchorLocalPoint(layer.anchor, bounds);
  }
  return {
    x: clamp(layer && layer.x, 0, Math.max(1, Number(bounds && bounds.width) || 0)),
    y: clamp(layer && layer.y, 0, Math.max(1, Number(bounds && bounds.height) || 0)),
  };
}
function slotIndexAtCanvasPoint(layoutKind, x, y, canvas = state.canvas) {
  const width = Number(canvas.width) || 0;
  const height = Number(canvas.height) || 0;
  if (layoutKind === "beside") return x >= width / 2 ? 1 : 0;
  if (layoutKind === "stack") return y >= height / 2 ? 1 : 0;
  if (layoutKind === "grid2x2") {
    const col = x >= width / 2 ? 1 : 0;
    const row = y >= height / 2 ? 1 : 0;
    return row * 2 + col;
  }
  return 0;
}
function slotTextPreview(slot) {
  const anchored = TEXT_LAYER_ANCHORS
    .map((anchor) => anchoredLayerForSlot(slot, anchor))
    .find((layer) => layer && layer.content);
  if (anchored) return anchored.content;
  const free = (slot && slot.textLayers || []).find((layer) => layer.kind === "free" && layer.content);
  return free ? free.content : "";
}
function layerDisplayName(layer) {
  if (!layer) return "Text";
  if (layer.kind === "anchored" && layer.anchor) {
    return layer.anchor.charAt(0).toUpperCase() + layer.anchor.slice(1);
  }
  return "Canvas";
}
function layerSummary(layer) {
  if (!layer || !layer.content) return "Empty";
  const text = layer.content.replace(/\s+/g, " ").trim();
  return text.length > 36 ? `${text.slice(0, 36)}…` : text;
}
function templateById(id) { return state.metadata.templates.find((t) => t.id === id); }
function layoutById(id) { return state.metadata.layouts.find((l) => l.id === id); }
function layoutName(id) { const l = layoutById(id); return l ? l.name : id; }
function templateName(id) { const t = templateById(id); return t ? t.name : id; }
function effectById(id) { return state.metadata.effects.find((e) => e.id === id); }
function summaryValue(s, k, fb) { return (!s || s[k] == null) ? fb : s[k]; }
