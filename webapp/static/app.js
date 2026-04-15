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
  {
    id: "cinematic",
    name: "Cinematic",
    description: "Soft white type with a restrained shadow.",
    style: {
      color: "#FFFFFF",
      outline: 2,
      outlineColor: "#111111",
      shadow: 4,
      shadowColor: "#00000088",
      fontSize: "md",
    },
  },
  {
    id: "panic",
    name: "Panic",
    description: "Red alert styling with heavier edges.",
    style: {
      color: "#FF0000",
      outline: 5,
      outlineColor: "#440000",
      shadow: 3,
      shadowColor: "#00000099",
      fontSize: "lg",
    },
  },
  {
    id: "chill",
    name: "Chill",
    description: "Terminal green with crisp outline.",
    style: {
      color: "#00FF41",
      outline: 3,
      outlineColor: "#003300",
      fontSize: "md",
    },
  },
  {
    id: "shout",
    name: "Shout",
    description: "Loud white all-caps energy.",
    style: {
      color: "#FFFFFF",
      outline: 4,
      outlineColor: "#000000",
      shadow: 3,
      shadowColor: "#00000099",
      fontSize: "xlg",
    },
  },
  {
    id: "whisper",
    name: "Whisper",
    description: "Muted grey with a fine outline.",
    style: {
      color: "#CCCCCC",
      outline: 1,
      outlineColor: "#333333",
      fontSize: "sm",
    },
  },
];

const $ = (id) => document.getElementById(id);

const state = {
  metadata: {
    templates: [],
    effects: [],
    layouts: [],
    stylePresets: [],
    limits: { ...DEFAULT_LIMITS },
  },
  canvas: { width: 720, height: 720 },
  output: { format: "png" },
  scenes: [],
  selectedSceneIndex: 0,
  selectedSlotIndex: 0,
  previewTimer: 0,
  previewSeq: 0,
  stageMode: "preview",
  stageAssetUrl: "",
  stageLabel: "Selected scene preview",
  script: "// Studio script will appear here.\n",
  backendCompatibility: "unknown",
  status: {
    line: "Loading studio…",
    meta: "Scene preview is debounced and Mac-powered.",
    kind: "normal",
  },
  isPreviewing: false,
  isExporting: false,
  lastExport: null,
};

document.addEventListener("DOMContentLoaded", async () => {
  bindStaticControls();
  await loadMetadata();
  bootstrapStudio();
});

async function loadMetadata() {
  try {
    const res = await fetch("/api/templates");
    const data = await res.json();
    if (!res.ok) throw new Error(data.error || "Could not load studio metadata.");
    state.metadata.templates = data.templates || [];
    state.metadata.effects = data.effects || [];
    state.metadata.layouts = Array.isArray(data.layouts) && data.layouts.length
      ? data.layouts
      : clone(FALLBACK_LAYOUTS);
    state.metadata.stylePresets = Array.isArray(data.stylePresets) && data.stylePresets.length
      ? data.stylePresets
      : clone(FALLBACK_STYLE_PRESETS);
    state.metadata.limits = { ...DEFAULT_LIMITS, ...(data.limits || {}) };

    const looksLegacyBackend = !Array.isArray(data.layouts) || !data.layouts.length || !Array.isArray(data.stylePresets);
    state.backendCompatibility = looksLegacyBackend ? "legacy" : "studio";
    if (looksLegacyBackend) applyLegacyBackendWarning();
  } catch (error) {
    setStatus(error.message, "Metadata did not load, so the studio cannot initialize.", "error");
    renderStatus();
    throw error;
  }
}

function bootstrapStudio() {
  applyDocument(buildDefaultDocument(), { silent: true });
  populateSelectOptions();
  renderAll();
  if (state.backendCompatibility === "studio") {
    schedulePreview(80);
  }
}

function buildDefaultDocument() {
  return {
    canvas: { width: 720, height: 720 },
    output: { format: "png" },
    scenes: [
      {
        durationMs: 760,
        layout: { kind: "single", padding: 0, border: 0, effect: "none" },
        slots: [
          createSlot("blank", {
            top: "MAC STUDIO",
            center: "ALL-INCLUSIVE",
            bottom: "Layouts • Styles • Scenes",
          }, "cinematic"),
        ],
      },
    ],
  };
}

function presetDocuments() {
  return [
    {
      id: "poster",
      label: "Poster Still",
      description: "One dramatic still with stacked editorial text.",
      build: () => ({
        canvas: { width: 720, height: 960 },
        output: { format: "png" },
        scenes: [
          {
            durationMs: 900,
            layout: { kind: "single", padding: 0, border: 0, effect: "none" },
            slots: [
              createSlot("dark", {
                top: "MAC",
                center: "STUDIO",
                bottom: "Design scenes with code underneath.",
              }, "shout"),
            ],
          },
        ],
      }),
    },
    {
      id: "split-verdict",
      label: "Split Verdict",
      description: "Two-slot layout with contrasting moods.",
      build: () => ({
        canvas: { width: 960, height: 640 },
        output: { format: "png" },
        scenes: [
          {
            durationMs: 820,
            layout: { kind: "beside", padding: 10, border: 2, effect: "vintage" },
            slots: [
              createSlot("blank", {
                top: "PLAN",
                center: "CALM",
                bottom: "Everything feels deliberate.",
              }, "cinematic"),
              createSlot("dark", {
                top: "REALITY",
                center: "CHAOS",
                bottom: "There are twelve moving parts now.",
              }, "panic"),
            ],
          },
        ],
      }),
    },
    {
      id: "week-arc",
      label: "Week Arc GIF",
      description: "A three-scene arc for fast animated exports.",
      build: () => ({
        canvas: { width: 720, height: 720 },
        output: { format: "gif" },
        scenes: [
          {
            durationMs: 320,
            layout: { kind: "single", padding: 0, border: 0, effect: "none" },
            slots: [
              createSlot("two_panel", {
                top: "MONDAY",
                bottom: "SHIPPING FEATURES",
              }, "cinematic"),
            ],
          },
          {
            durationMs: 320,
            layout: { kind: "single", padding: 0, border: 0, effect: "comic" },
            slots: [
              createSlot("two_panel", {
                top: "WEDNESDAY",
                bottom: "FINDING THE EDGE CASE",
              }, "chill"),
            ],
          },
          {
            durationMs: 520,
            layout: { kind: "single", padding: 0, border: 0, effect: "glitch" },
            slots: [
              createSlot("two_panel", {
                top: "FRIDAY",
                bottom: "EXPORTING ANYWAY",
              }, "panic"),
            ],
          },
        ],
      }),
    },
    {
      id: "quad-board",
      label: "Quad Board",
      description: "Four-slot board for comparison, summaries, or beats.",
      build: () => ({
        canvas: { width: 900, height: 900 },
        output: { format: "png" },
        scenes: [
          {
            durationMs: 900,
            layout: { kind: "grid2x2", padding: 12, border: 2, effect: "retro" },
            slots: [
              createSlot("blank", { center: "HOOK" }, "shout"),
              createSlot("blank", { center: "CONFLICT" }, "cinematic"),
              createSlot("blank", { center: "TWIST" }, "whisper"),
              createSlot("blank", { center: "PAYOFF" }, "chill"),
            ],
          },
        ],
      }),
    },
  ];
}

function clone(value) {
  return JSON.parse(JSON.stringify(value));
}

function defaultStyleFields() {
  return {
    preset: "",
    color: "#FFFFFF",
    outline: 3,
    outlineColor: "#000000",
    shadow: 0,
    shadowColor: "#00000080",
    fontSizeMode: "auto",
    fontSizePx: 64,
  };
}

function getPresetStyle(presetId) {
  const preset = state.metadata.stylePresets.find((item) => item.id === presetId);
  return preset ? preset.style : null;
}

function createStyle(presetId = "", overrides = {}) {
  const base = defaultStyleFields();
  const presetStyle = getPresetStyle(presetId);
  if (presetStyle) {
    base.preset = presetId;
    base.color = presetStyle.color || base.color;
    base.outline = Number(presetStyle.outline != null ? presetStyle.outline : base.outline);
    base.outlineColor = presetStyle.outlineColor || base.outlineColor;
    base.shadow = Number(presetStyle.shadow != null ? presetStyle.shadow : base.shadow);
    base.shadowColor = presetStyle.shadowColor || base.shadowColor;
    if (typeof presetStyle.fontSize === "number") {
      base.fontSizeMode = "custom";
      base.fontSizePx = presetStyle.fontSize;
    } else if (presetStyle.fontSize) {
      base.fontSizeMode = presetStyle.fontSize;
    }
  }

  const merged = { ...base, ...overrides };
  if (!FONT_SIZE_OPTIONS.some((option) => option.id === merged.fontSizeMode)) {
    merged.fontSizeMode = "auto";
  }
  if (merged.fontSizeMode !== "custom" && typeof merged.fontSizePx !== "number") {
    merged.fontSizePx = 64;
  }
  return merged;
}

function createSlot(templateId = "blank", text = {}, presetId = "cinematic") {
  return {
    templateId,
    text: {
      top: text.top || "",
      center: text.center || "",
      bottom: text.bottom || "",
    },
    style: createStyle(presetId),
  };
}

function slotCountForLayout(kind) {
  const layout = state.metadata.layouts.find((item) => item.id === kind);
  return layout ? layout.slotCount : 1;
}

function createScene(kind = "single", seedSlots = []) {
  const slotCount = slotCountForLayout(kind);
  const slots = seedSlots.slice(0, slotCount).map((slot) => clone(slot));
  while (slots.length < slotCount) slots.push(createSlot());
  return {
    durationMs: 420,
    layout: {
      kind,
      padding: 0,
      border: 0,
      effect: "none",
    },
    slots,
  };
}

function selectedScene() {
  return state.scenes[state.selectedSceneIndex];
}

function selectedSlot() {
  const scene = selectedScene();
  return scene ? scene.slots[state.selectedSlotIndex] || null : null;
}

function templateById(templateId) {
  return state.metadata.templates.find((template) => template.id === templateId);
}

function layoutById(layoutId) {
  return state.metadata.layouts.find((layout) => layout.id === layoutId);
}

function layoutName(layoutId) {
  const layout = layoutById(layoutId);
  return layout ? layout.name : layoutId;
}

function templateName(templateId) {
  const template = templateById(templateId);
  return template ? template.name : templateId;
}

function summaryValue(summary, key, fallback) {
  if (!summary || summary[key] == null) return fallback;
  return summary[key];
}

function cssUrl(url) {
  return String(url || "").replace(/["'()\\\n\r]/g, (char) => {
    if (char === "\n" || char === "\r") return "";
    return `\\${char}`;
  });
}

function effectById(effectId) {
  return state.metadata.effects.find((effect) => effect.id === effectId);
}

function setStatus(line, meta, kind = "normal") {
  state.status = { line, meta, kind };
}

function legacyBackendMeta() {
  return {
    line: "Backend restart required.",
    meta: "This tab loaded the new studio UI, but the server on this port is still the old GIF backend. Restart `./webapp/run.sh` and hard refresh.",
  };
}

function applyLegacyBackendWarning() {
  const message = legacyBackendMeta();
  state.stageLabel = "Backend restart required";
  state.script = "// Restart ./webapp/run.sh and hard refresh the browser.\n";
  setStatus(message.line, message.meta, "error");
}

function markLegacyBackendIfNeeded(errorMessage) {
  const text = String(errorMessage || "");
  if (text.includes("At least one frame is required.") || text.includes("At least one frame")) {
    state.backendCompatibility = "legacy";
    applyLegacyBackendWarning();
    return true;
  }
  return false;
}

function renderStatus() {
  $("statusLine").textContent = state.status.line;
  $("statusLine").className = `status${state.status.kind === "error" ? " status--error" : ""}`;
  $("statusMeta").textContent = state.status.meta;
}

function bindStaticControls() {
  bindRangePair("canvasWidthRange", "canvasWidthInput", (value) => {
    state.canvas.width = clamp(value, state.metadata.limits.width.min, state.metadata.limits.width.max);
    commitChange();
  });
  bindRangePair("canvasHeightRange", "canvasHeightInput", (value) => {
    state.canvas.height = clamp(value, state.metadata.limits.height.min, state.metadata.limits.height.max);
    commitChange();
  });
  bindRangePair("sceneDurationRange", "sceneDurationInput", (value) => {
    const scene = selectedScene();
    if (!scene) return;
    scene.durationMs = clamp(value, state.metadata.limits.duration.min, state.metadata.limits.duration.max);
    commitChange();
  });
  bindRangePair("scenePaddingRange", "scenePaddingInput", (value) => {
    const scene = selectedScene();
    if (!scene) return;
    scene.layout.padding = clamp(value, state.metadata.limits.padding.min, state.metadata.limits.padding.max);
    commitChange();
  });
  bindRangePair("sceneBorderRange", "sceneBorderInput", (value) => {
    const scene = selectedScene();
    if (!scene) return;
    scene.layout.border = clamp(value, state.metadata.limits.border.min, state.metadata.limits.border.max);
    commitChange();
  });
  bindRangePair("slotOutlineRange", "slotOutlineInput", (value) => {
    const slot = selectedSlot();
    if (!slot) return;
    slot.style.outline = clamp(value, state.metadata.limits.outline.min, state.metadata.limits.outline.max);
    commitChange();
  });
  bindRangePair("slotShadowRange", "slotShadowInput", (value) => {
    const slot = selectedSlot();
    if (!slot) return;
    slot.style.shadow = clamp(value, state.metadata.limits.shadow.min, state.metadata.limits.shadow.max);
    commitChange();
  });

  $("sceneEffectSelect").addEventListener("change", (event) => {
    const scene = selectedScene();
    if (!scene) return;
    scene.layout.effect = event.target.value;
    commitChange();
  });

  $("slotTopText").addEventListener("input", (event) => updateSlotText("top", event.target.value));
  $("slotCenterText").addEventListener("input", (event) => updateSlotText("center", event.target.value));
  $("slotBottomText").addEventListener("input", (event) => updateSlotText("bottom", event.target.value));

  $("slotStylePreset").addEventListener("change", (event) => {
    const slot = selectedSlot();
    if (!slot) return;
    slot.style = createStyle(event.target.value || "");
    commitChange();
  });

  $("slotTextColor").addEventListener("input", (event) => updateHexField("color", event.target.value, false));
  $("slotTextColorHex").addEventListener("change", (event) => updateHexField("color", event.target.value, false));
  $("slotOutlineColor").addEventListener("input", (event) => updateHexField("outlineColor", event.target.value, false));
  $("slotOutlineColorHex").addEventListener("change", (event) => updateHexField("outlineColor", event.target.value, false));
  $("slotShadowColorHex").addEventListener("change", (event) => updateHexField("shadowColor", event.target.value, true));

  $("slotFontSizeMode").addEventListener("change", (event) => {
    const slot = selectedSlot();
    if (!slot) return;
    slot.style.fontSizeMode = event.target.value;
    if (slot.style.fontSizeMode !== "custom") slot.style.fontSizePx = clamp(slot.style.fontSizePx, 8, 240);
    commitChange();
  });

  $("slotFontSizePx").addEventListener("change", (event) => {
    const slot = selectedSlot();
    if (!slot) return;
    slot.style.fontSizePx = clamp(event.target.value, state.metadata.limits.fontSizePx.min, state.metadata.limits.fontSizePx.max);
    commitChange();
  });

  $("formatToggle").addEventListener("click", (event) => {
    const button = event.target.closest("[data-format]");
    if (!button || button.disabled) return;
    state.output.format = button.dataset.format;
    commitChange({ schedule: false });
    schedulePreview(40);
  });

  $("addSceneBtn").addEventListener("click", () => {
    const insertIndex = state.selectedSceneIndex + 1;
    state.scenes.splice(insertIndex, 0, createScene("single"));
    state.selectedSceneIndex = insertIndex;
    state.selectedSlotIndex = 0;
    commitChange();
  });

  $("refreshPreviewBtn").addEventListener("click", () => {
    schedulePreview(20, true);
  });

  $("exportBtn").addEventListener("click", exportDocument);

  $("canvasTemplateGrid").addEventListener("click", onTemplatePick);
  $("memeTemplateGrid").addEventListener("click", onTemplatePick);
  $("layoutGrid").addEventListener("click", onLayoutPick);
  $("presetGrid").addEventListener("click", onPresetPick);
  $("sceneStrip").addEventListener("click", onSceneStripAction);
  $("slotTabs").addEventListener("click", onSlotTabAction);

  // Sidebar tab switching
  const sidebarLeft = $("sidebarLeft");
  if (sidebarLeft) {
    sidebarLeft.querySelector(".sidebar__head").addEventListener("click", (e) => {
      const tab = e.target.closest(".sidebar-tab");
      if (!tab) return;
      const target = tab.dataset.tab;
      sidebarLeft.querySelectorAll(".sidebar-tab").forEach((t) => t.classList.remove("is-active"));
      tab.classList.add("is-active");
      sidebarLeft.querySelectorAll(".sidebar__pane").forEach((p) => {
        p.hidden = p.dataset.pane !== target;
      });
    });
  }
}

function bindRangePair(rangeId, inputId, onChange) {
  const range = $(rangeId);
  const input = $(inputId);
  range.addEventListener("input", (event) => onChange(Number(event.target.value)));
  input.addEventListener("change", (event) => onChange(Number(event.target.value)));
}

function clamp(value, min, max) {
  const number = Number(value);
  if (!Number.isFinite(number)) return min;
  return Math.max(min, Math.min(max, Math.round(number)));
}

function updateSlotText(field, value) {
  const slot = selectedSlot();
  if (!slot) return;
  slot.text[field] = value;
  commitChange();
}

function normalizeHex(value, fallback, allowAlpha = false) {
  const text = String(value || "").trim().toUpperCase();
  const pattern = allowAlpha ? /^#[0-9A-F]{6}([0-9A-F]{2})?$/ : /^#[0-9A-F]{6}$/;
  return pattern.test(text) ? text : fallback;
}

function updateHexField(field, value, allowAlpha) {
  const slot = selectedSlot();
  if (!slot) return;
  const fallback = slot.style[field];
  const normalized = normalizeHex(value, fallback, allowAlpha);
  slot.style[field] = normalized;
  commitChange({ schedule: false });
  renderInspector();
  schedulePreview();
}

function onTemplatePick(event) {
  const button = event.target.closest("[data-template-id]");
  if (!button) return;
  const slot = selectedSlot();
  if (!slot) return;
  slot.templateId = button.dataset.templateId;
  commitChange();
}

function onLayoutPick(event) {
  const button = event.target.closest("[data-layout-id]");
  if (!button) return;
  const scene = selectedScene();
  if (!scene) return;
  const nextLayout = button.dataset.layoutId;
  scene.layout.kind = nextLayout;
  scene.slots = preserveSlots(scene.slots, nextLayout);
  state.selectedSlotIndex = Math.min(state.selectedSlotIndex, scene.slots.length - 1);
  commitChange();
}

function onPresetPick(event) {
  const button = event.target.closest("[data-preset-id]");
  if (!button) return;
  const preset = presetDocuments().find((item) => item.id === button.dataset.presetId);
  if (!preset) return;
  applyDocument(preset.build());
}

function onSceneStripAction(event) {
  const actionButton = event.target.closest("[data-scene-action]");
  if (!actionButton) return;

  const index = Number(actionButton.dataset.sceneIndex);
  if (!Number.isInteger(index) || !state.scenes[index]) return;

  const action = actionButton.dataset.sceneAction;
  if (action === "select") {
    state.selectedSceneIndex = index;
    state.selectedSlotIndex = Math.min(state.selectedSlotIndex, state.scenes[index].slots.length - 1);
  } else if (action === "dup") {
    state.scenes.splice(index + 1, 0, clone(state.scenes[index]));
    state.selectedSceneIndex = index + 1;
  } else if (action === "del") {
    if (state.scenes.length === 1) return;
    state.scenes.splice(index, 1);
    state.selectedSceneIndex = Math.min(state.selectedSceneIndex, state.scenes.length - 1);
  } else if (action === "up" && index > 0) {
    [state.scenes[index - 1], state.scenes[index]] = [state.scenes[index], state.scenes[index - 1]];
    state.selectedSceneIndex = index - 1;
  } else if (action === "down" && index < state.scenes.length - 1) {
    [state.scenes[index], state.scenes[index + 1]] = [state.scenes[index + 1], state.scenes[index]];
    state.selectedSceneIndex = index + 1;
  }

  state.selectedSlotIndex = Math.min(state.selectedSlotIndex, selectedScene().slots.length - 1);
  commitChange();
}

function onSlotTabAction(event) {
  const button = event.target.closest("[data-slot-index]");
  if (!button) return;
  state.selectedSlotIndex = Number(button.dataset.slotIndex);
  renderAll();
}

function preserveSlots(existingSlots, layoutKind) {
  const targetCount = slotCountForLayout(layoutKind);
  const next = existingSlots.slice(0, targetCount).map((slot) => clone(slot));
  while (next.length < targetCount) next.push(createSlot());
  return next;
}

function applyDocument(document, options = {}) {
  state.canvas = clone(document.canvas);
  state.output = clone(document.output);
  state.scenes = document.scenes.map((scene) => clone(scene));
  state.selectedSceneIndex = 0;
  state.selectedSlotIndex = 0;
  ensureOutputCompatibility(false);
  setStatus("Studio ready.", "Scene preview is debounced and Mac-powered.");
  renderAll();
  if (!options.silent) schedulePreview(80);
}

function ensureOutputCompatibility(showMessage = true) {
  const pngButton = document.querySelector('[data-format="png"]');
  const shouldForceGif = state.scenes.length > 1 && state.output.format === "png";
  if (shouldForceGif) {
    state.output.format = "gif";
    if (showMessage) {
      setStatus("Output switched to GIF.", "Multi-scene documents export as GIF loops.", "normal");
    }
  }
  if (pngButton) pngButton.disabled = state.scenes.length > 1;
}

function commitChange(options = {}) {
  ensureOutputCompatibility(options.showMessage !== false);
  renderAll();
  if (options.schedule !== false) schedulePreview();
}

function schedulePreview(delay = 320, immediateMessage = false) {
  clearTimeout(state.previewTimer);
  if (immediateMessage) {
    setStatus("Refreshing preview…", "Rendering the selected scene through Mac.", "normal");
    renderStatus();
  }
  state.previewTimer = window.setTimeout(() => {
    requestPreview();
  }, delay);
}

async function requestPreview() {
  if (state.backendCompatibility !== "studio") {
    applyLegacyBackendWarning();
    renderAll();
    return;
  }

  const payload = buildPayload({ previewSceneIndex: state.selectedSceneIndex });
  const requestId = ++state.previewSeq;
  state.isPreviewing = true;
  setStatus("Refreshing preview…", "Rendering the selected scene through Mac.", "normal");
  renderStatus();

  try {
    const res = await fetch("/api/generate", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(payload),
    });
    const data = await res.json();
    if (requestId !== state.previewSeq) return;
    if (!res.ok) throw new Error(data.error || "Preview render failed.");

    state.script = data.script || state.script;
    state.stageMode = "preview";
    state.stageAssetUrl = `${data.previewUrl}?v=${Date.now()}`;
    state.stageLabel = `Selected scene preview · Scene ${state.selectedSceneIndex + 1}`;
    const summary = data.summary || {};
    setStatus(
      "Scene preview ready.",
      `${layoutName(selectedScene().layout.kind) || "Scene"} · ${formatBytes(summaryValue(summary, "fileSizeBytes", 0))} preview`,
      "normal"
    );
  } catch (error) {
    if (requestId !== state.previewSeq) return;
    if (!markLegacyBackendIfNeeded(error.message)) {
      setStatus(error.message, "Preview failed. Fix the document or try exporting after the next edit.", "error");
    }
  } finally {
    if (requestId === state.previewSeq) {
      state.isPreviewing = false;
      renderAll();
    }
  }
}

async function exportDocument() {
  if (state.backendCompatibility !== "studio") {
    applyLegacyBackendWarning();
    renderAll();
    return;
  }

  state.isExporting = true;
  setStatus("Exporting document…", "Rendering the full studio document.", "normal");
  renderAll();

  try {
    const res = await fetch("/api/generate", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify(buildPayload()),
    });
    const data = await res.json();
    if (!res.ok) throw new Error(data.error || "Export failed.");

    state.lastExport = data;
    state.script = data.script || state.script;
    state.stageMode = "export";
    state.stageAssetUrl = `${data.previewUrl}?v=${Date.now()}`;
    const summary = data.summary || {};
    state.stageLabel = `Exported ${String(summaryValue(summary, "format", "")).toUpperCase()} document`;
    setStatus(
      "Export complete.",
      `${summaryValue(summary, "frameCount", 1)} frame(s) · ${formatBytes(summaryValue(summary, "fileSizeBytes", 0))} · ${String(summaryValue(summary, "format", "")).toUpperCase()}`,
      "normal"
    );
  } catch (error) {
    if (!markLegacyBackendIfNeeded(error.message)) {
      setStatus(error.message, "The document export did not complete.", "error");
    }
  } finally {
    state.isExporting = false;
    renderAll();
  }
}

function buildPayload(extra = {}) {
  return {
    canvas: clone(state.canvas),
    output: clone(state.output),
    scenes: state.scenes.map((scene) => ({
      durationMs: scene.durationMs,
      layout: clone(scene.layout),
      slots: scene.slots.map((slot) => ({
        templateId: slot.templateId,
        text: clone(slot.text),
        style: {
          preset: slot.style.preset || "",
          color: slot.style.color,
          outline: slot.style.outline,
          outlineColor: slot.style.outlineColor,
          shadow: slot.style.shadow,
          shadowColor: slot.style.shadowColor,
          fontSizeMode: slot.style.fontSizeMode,
          fontSizePx: slot.style.fontSizePx,
        },
      })),
    })),
    ...extra,
  };
}

function populateSelectOptions() {
  $("sceneEffectSelect").innerHTML = state.metadata.effects
    .map((effect) => `<option value="${escAttr(effect.id)}">${esc(effect.name)} — ${esc(effect.description)}</option>`)
    .join("");

  $("slotStylePreset").innerHTML = [
    '<option value="">Custom / Default</option>',
    ...state.metadata.stylePresets.map((preset) => `<option value="${escAttr(preset.id)}">${esc(preset.name)}</option>`),
  ].join("");

  $("slotFontSizeMode").innerHTML = FONT_SIZE_OPTIONS
    .map((option) => `<option value="${escAttr(option.id)}">${esc(option.label)}</option>`)
    .join("");
}

function renderAll() {
  ensureOutputCompatibility(false);
  renderStats();
  renderTemplateGrids();
  renderLayoutGrid();
  renderPresetGrid();
  renderSceneStrip();
  renderSlotTabs();
  renderInspector();
  renderStage();
  renderScript();
  renderStatus();
}

function renderStats() {
  $("statCanvas").textContent = `${state.canvas.width}×${state.canvas.height}`;
  $("statScenes").textContent = String(state.scenes.length);
  const totalMs = state.scenes.reduce((sum, scene) => sum + scene.durationMs, 0);
  $("statRuntime").textContent = formatDuration(totalMs);
  $("canvasBadge").textContent = `${state.canvas.width}×${state.canvas.height}`;
  $("canvasWidthRange").value = state.canvas.width;
  $("canvasWidthInput").value = state.canvas.width;
  $("canvasHeightRange").value = state.canvas.height;
  $("canvasHeightInput").value = state.canvas.height;
}

function renderTemplateGrids() {
  renderTemplateGrid("canvasTemplateGrid", "canvas");
  renderTemplateGrid("memeTemplateGrid", "meme");
}

function renderTemplateGrid(containerId, category) {
  const container = $(containerId);
  const slot = selectedSlot();
  const templates = state.metadata.templates.filter((template) => template.category === category);
  if (!templates.length) {
    container.innerHTML = '<p class="empty-state">No templates in this shelf yet.</p>';
    return;
  }

  container.innerHTML = templates.map((template) => {
    const active = slot && slot.templateId === template.id;
    return `
      <button class="template-card${active ? " is-active" : ""}" type="button" data-template-id="${escAttr(template.id)}" title="${escAttr(template.description)}">
        <div class="template-card__thumb" style="background-image:url('${cssUrl(template.previewUrl)}')"></div>
        <span class="template-card__name">${esc(template.name)}</span>
      </button>
    `;
  }).join("");
}

function renderLayoutGrid() {
  const scene = selectedScene();
  $("layoutGrid").innerHTML = state.metadata.layouts.map((layout) => `
    <button class="layout-card${scene && scene.layout.kind === layout.id ? " is-active" : ""}" type="button" data-layout-id="${escAttr(layout.id)}">
      <span class="layout-card__name">${esc(layout.name)}</span>
      <span class="layout-card__meta">${layout.slotCount} slot${layout.slotCount > 1 ? "s" : ""}</span>
      <span class="layout-card__desc">${esc(layout.description)}</span>
    </button>
  `).join("");
}

function renderPresetGrid() {
  $("presetGrid").innerHTML = presetDocuments().map((preset) => `
    <button class="preset-card" type="button" data-preset-id="${escAttr(preset.id)}">
      <span class="preset-card__name">${esc(preset.label)}</span>
      <span class="preset-card__desc">${esc(preset.description)}</span>
    </button>
  `).join("");
}

function renderSceneStrip() {
  $("sceneStrip").innerHTML = state.scenes.map((scene, index) => {
    const active = index === state.selectedSceneIndex;
    const lead = sceneLeadText(scene);
    return `
      <article class="scene-card${active ? " is-active" : ""}">
        <button class="scene-card__body" type="button" data-scene-action="select" data-scene-index="${index}">
          <span class="scene-card__kicker">Scene ${index + 1}</span>
          <strong class="scene-card__title">${esc(layoutName(scene.layout.kind))}</strong>
          <span class="scene-card__summary">${esc(lead)}</span>
          <span class="scene-card__meta">${scene.slots.length} slot${scene.slots.length > 1 ? "s" : ""} · ${formatDuration(scene.durationMs)}</span>
        </button>
        <div class="scene-card__actions">
          <button class="icon-btn" type="button" data-scene-action="up" data-scene-index="${index}" ${index === 0 ? "disabled" : ""}>↑</button>
          <button class="icon-btn" type="button" data-scene-action="down" data-scene-index="${index}" ${index === state.scenes.length - 1 ? "disabled" : ""}>↓</button>
          <button class="icon-btn" type="button" data-scene-action="dup" data-scene-index="${index}">Dup</button>
          <button class="icon-btn" type="button" data-scene-action="del" data-scene-index="${index}" ${state.scenes.length === 1 ? "disabled" : ""}>×</button>
        </div>
      </article>
    `;
  }).join("");
}

function sceneLeadText(scene) {
  const firstSlot = scene.slots[0];
  const snippets = [firstSlot.text.top, firstSlot.text.center, firstSlot.text.bottom].filter(Boolean);
  return snippets[0] || "Empty scene";
}

function renderSlotTabs() {
  const scene = selectedScene();
  if (!scene) {
    $("slotTabs").innerHTML = "";
    return;
  }
  $("slotTabs").innerHTML = scene.slots.map((slot, index) => `
    <button class="slot-tab${index === state.selectedSlotIndex ? " is-active" : ""}" type="button" data-slot-index="${index}">
      <span class="slot-tab__label">Slot ${index + 1}</span>
      <span class="slot-tab__meta">${esc(templateName(slot.templateId))}</span>
    </button>
  `).join("");
}

function renderInspector() {
  const scene = selectedScene();
  const slot = selectedSlot();
  if (!scene || !slot) return;

  $("sceneMeta").textContent = `Scene ${state.selectedSceneIndex + 1}`;
  $("selectedLayoutLabel").textContent = layoutName(scene.layout.kind);
  $("sceneDurationRange").value = scene.durationMs;
  $("sceneDurationInput").value = scene.durationMs;
  $("scenePaddingRange").value = scene.layout.padding;
  $("scenePaddingInput").value = scene.layout.padding;
  $("sceneBorderRange").value = scene.layout.border;
  $("sceneBorderInput").value = scene.layout.border;
  $("sceneEffectSelect").value = scene.layout.effect;

  $("slotMeta").textContent = `Slot ${state.selectedSlotIndex + 1} of ${scene.slots.length}`;
  $("slotTemplateLabel").textContent = templateName(slot.templateId);
  $("slotHint").textContent = scene.slots.length > 1
    ? "Choose Slot 1-4 here, then click a template or meme image on the left to place it into that grid cell."
    : "Templates and meme images you click on the left apply to the currently selected slot.";
  $("slotTopText").value = slot.text.top;
  $("slotCenterText").value = slot.text.center;
  $("slotBottomText").value = slot.text.bottom;

  $("slotStylePreset").value = slot.style.preset || "";
  $("slotTextColor").value = normalizeHex(slot.style.color, "#FFFFFF", false);
  $("slotTextColorHex").value = normalizeHex(slot.style.color, "#FFFFFF", false);
  $("slotOutlineRange").value = slot.style.outline;
  $("slotOutlineInput").value = slot.style.outline;
  $("slotOutlineColor").value = normalizeHex(slot.style.outlineColor, "#000000", false);
  $("slotOutlineColorHex").value = normalizeHex(slot.style.outlineColor, "#000000", false);
  $("slotShadowRange").value = slot.style.shadow;
  $("slotShadowInput").value = slot.style.shadow;
  $("slotShadowColorHex").value = normalizeHex(slot.style.shadowColor, "#00000080", true);
  $("slotFontSizeMode").value = slot.style.fontSizeMode;
  $("slotFontSizePx").value = slot.style.fontSizePx;
  $("slotFontSizePxField").hidden = slot.style.fontSizeMode !== "custom";

  const pngDisabled = state.scenes.length > 1;
  document.querySelectorAll("[data-format]").forEach((button) => {
    const active = button.dataset.format === state.output.format;
    button.classList.toggle("is-active", active);
    if (button.dataset.format === "png") button.disabled = pngDisabled;
  });
  $("formatHelp").textContent = pngDisabled
    ? "Multi-scene documents export as GIF loops. Reduce to one scene to enable PNG."
    : "Single-scene documents can export as PNG or GIF.";
  $("exportMeta").textContent = state.output.format === "gif" ? "Loop export" : "Still export";
}

function renderStage() {
  const hasAsset = Boolean(state.stageAssetUrl);
  $("stageImage").hidden = !hasAsset;
  $("stagePlaceholder").hidden = hasAsset;
  if (hasAsset) {
    $("stageImage").src = state.stageAssetUrl;
  }
  $("stageLabel").textContent = state.stageLabel;
  $("refreshPreviewBtn").disabled = state.isPreviewing;
  $("refreshPreviewBtn").textContent = state.isPreviewing ? "Refreshing…" : "Refresh Preview";
  $("exportBtn").disabled = state.isExporting;
  $("exportBtn").textContent = state.isExporting ? "Exporting…" : "Export Document";

  if (state.lastExport && state.lastExport.downloadUrl) {
    $("downloadLink").hidden = false;
    $("downloadLink").href = state.lastExport.downloadUrl;
  } else {
    $("downloadLink").hidden = true;
  }
}

function renderScript() {
  $("scriptPreview").textContent = state.script || "// Studio script will appear here.\n";
}

function formatDuration(ms) {
  return ms >= 1000 ? `${(ms / 1000).toFixed(1)}s` : `${ms}ms`;
}

function formatBytes(bytes) {
  const value = Math.max(0, Number(bytes) || 0);
  if (value < 1024) return `${value} B`;
  return `${Math.max(1, Math.round(value / 1024))} KB`;
}

function esc(text) {
  return String(text)
    .replace(/&/g, "&amp;")
    .replace(/</g, "&lt;")
    .replace(/>/g, "&gt;")
    .replace(/"/g, "&quot;");
}

function escAttr(text) {
  return esc(text).replace(/'/g, "&#39;");
}
