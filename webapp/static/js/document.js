// Mac Studio — Document, scene, slot, and style builders + presets

const TRANSITION_TYPES = [
  { id: "cut", label: "Cut (none)" },
  { id: "crossfade", label: "Crossfade" },
  { id: "slideLeft", label: "Slide Left" },
  { id: "slideRight", label: "Slide Right" },
  { id: "slideUp", label: "Slide Up" },
  { id: "slideDown", label: "Slide Down" },
  { id: "wipe", label: "Wipe" },
  { id: "fadeBlack", label: "Fade to Black" },
  { id: "zoom", label: "Zoom" },
];

const EASING_TYPES = [
  { id: "linear", label: "Linear" },
  { id: "ease", label: "Ease" },
  { id: "easeIn", label: "Ease In" },
  { id: "easeOut", label: "Ease Out" },
  { id: "easeInOut", label: "Ease In-Out" },
];

function defaultStyleFields() {
  return {
    preset: "", color: "#FFFFFF", outline: 3, outlineColor: "#000000",
    shadow: 0, shadowColor: "#00000080", fontSizeMode: "sm", fontSizePx: 64,
  };
}

function getPresetStyle(presetId) {
  const preset = state.metadata.stylePresets.find((item) => item.id === presetId);
  return preset ? preset.style : null;
}

function createStyle(presetId = "", overrides = {}) {
  const base = defaultStyleFields();
  const ps = getPresetStyle(presetId);
  if (ps) {
    base.preset = presetId;
    base.color = ps.color || base.color;
    base.outline = Number(ps.outline != null ? ps.outline : base.outline);
    base.outlineColor = ps.outlineColor || base.outlineColor;
    base.shadow = Number(ps.shadow != null ? ps.shadow : base.shadow);
    base.shadowColor = ps.shadowColor || base.shadowColor;
    if (typeof ps.fontSize === "number") { base.fontSizeMode = "custom"; base.fontSizePx = ps.fontSize; }
    else if (ps.fontSize) { base.fontSizeMode = ps.fontSize; }
  }
  const merged = { ...base, ...overrides };
  if (!FONT_SIZE_OPTIONS.some((o) => o.id === merged.fontSizeMode)) merged.fontSizeMode = "auto";
  if (merged.fontSizeMode !== "custom" && typeof merged.fontSizePx !== "number") merged.fontSizePx = 64;
  return merged;
}

function createSlot(templateId = "blank", text = {}, presetId = "cinematic") {
  return {
    templateId,
    text: { top: text.top || "", center: text.center || "", bottom: text.bottom || "" },
    style: createStyle(presetId),
  };
}

function slotCountForLayout(kind) {
  const layout = state.metadata.layouts.find((item) => item.id === kind);
  return layout ? layout.slotCount : 1;
}

function createScene(kind = "single", seedSlots = []) {
  const slotCount = slotCountForLayout(kind);
  const slots = seedSlots.slice(0, slotCount).map((s) => clone(s));
  while (slots.length < slotCount) slots.push(createSlot());
  return {
    durationMs: 420,
    layout: { kind, padding: 0, border: 0, effect: "none" },
    slots,
    transition: { type: "crossfade", durationMs: 150, easing: "ease" },
  };
}

function preserveSlots(existingSlots, layoutKind) {
  const targetCount = slotCountForLayout(layoutKind);
  const next = existingSlots.slice(0, targetCount).map((s) => clone(s));
  while (next.length < targetCount) next.push(createSlot());
  return next;
}

function buildDefaultDocument() {
  return {
    canvas: { width: 720, height: 720 },
    output: { format: "png" },
    scenes: [{
      durationMs: 760,
      layout: { kind: "grid2x2", padding: 5, border: 2, effect: "none" },
      slots: [
        createSlot("meme.shrek_smirk", { bottom: "Ask AI to fix the bug" }, "chill"),
        createSlot("meme.king_bach_stare", { bottom: "AI refactors entire codebase" }, "cinematic"),
        createSlot("meme.kid_crying", { bottom: "Now nothing compiles" }, "panic"),
        createSlot("meme.jordan_crying", { bottom: "git reset --hard" }, "panic"),
      ],
    }],
  };
}

function presetDocuments() {
  return [
    { id: "poster", label: "Poster Still", description: "One dramatic still with stacked editorial text.",
      build: () => ({ canvas: { width: 720, height: 960 }, output: { format: "png" },
        scenes: [{ durationMs: 900, layout: { kind: "single", padding: 0, border: 0, effect: "none" },
          slots: [createSlot("dark", { top: "MAC", center: "STUDIO", bottom: "Design scenes with code underneath." }, "shout")] }] }) },
    { id: "split-verdict", label: "Split Verdict", description: "Two-slot layout with contrasting moods.",
      build: () => ({ canvas: { width: 960, height: 640 }, output: { format: "png" },
        scenes: [{ durationMs: 820, layout: { kind: "beside", padding: 10, border: 2, effect: "vintage" },
          slots: [
            createSlot("blank", { top: "PLAN", center: "CALM", bottom: "Everything feels deliberate." }, "cinematic"),
            createSlot("dark", { top: "REALITY", center: "CHAOS", bottom: "There are twelve moving parts now." }, "panic"),
          ] }] }) },
    { id: "week-arc", label: "Week Arc GIF", description: "A three-scene arc for fast animated exports.",
      build: () => ({ canvas: { width: 720, height: 720 }, output: { format: "gif" },
        scenes: [
          { durationMs: 320, layout: { kind: "single", padding: 0, border: 0, effect: "none" },
            slots: [createSlot("two_panel", { top: "MONDAY", bottom: "SHIPPING FEATURES" }, "cinematic")] },
          { durationMs: 320, layout: { kind: "single", padding: 0, border: 0, effect: "comic" },
            slots: [createSlot("two_panel", { top: "WEDNESDAY", bottom: "FINDING THE EDGE CASE" }, "chill")] },
          { durationMs: 520, layout: { kind: "single", padding: 0, border: 0, effect: "glitch" },
            slots: [createSlot("two_panel", { top: "FRIDAY", bottom: "EXPORTING ANYWAY" }, "panic")] },
        ] }) },
    { id: "quad-board", label: "Quad Board", description: "Four-slot board for comparison, summaries, or beats.",
      build: () => ({ canvas: { width: 900, height: 900 }, output: { format: "png" },
        scenes: [{ durationMs: 900, layout: { kind: "grid2x2", padding: 12, border: 2, effect: "retro" },
          slots: [
            createSlot("blank", { center: "HOOK" }, "shout"),
            createSlot("blank", { center: "CONFLICT" }, "cinematic"),
            createSlot("blank", { center: "TWIST" }, "whisper"),
            createSlot("blank", { center: "PAYOFF" }, "chill"),
          ] }] }) },
  ];
}

function buildPayload(extra = {}) {
  return {
    canvas: clone(state.canvas),
    output: clone(state.output),
    scenes: state.scenes.map((scene, i) => ({
      durationMs: scene.durationMs,
      layout: clone(scene.layout),
      transition: (i > 0 && scene.transition && scene.transition.type !== "cut")
        ? clone(scene.transition) : null,
      slots: scene.slots.map((slot) => ({
        templateId: slot.templateId,
        text: clone(slot.text),
        style: {
          preset: slot.style.preset || "", color: slot.style.color,
          outline: slot.style.outline, outlineColor: slot.style.outlineColor,
          shadow: slot.style.shadow, shadowColor: slot.style.shadowColor,
          fontSizeMode: slot.style.fontSizeMode, fontSizePx: slot.style.fontSizePx,
        },
      })),
    })),
    ...extra,
  };
}
