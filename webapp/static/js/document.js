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
    shadow: 0, shadowColor: "#00000080", fontSizeMode: "sm", fontSizePx: 64, background: "",
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

function createAnchoredTextLayers(text = {}) {
  return TEXT_LAYER_ANCHORS
    .filter((anchor) => text[anchor])
    .map((anchor) => normalizeTextLayer({
      kind: "anchored",
      anchor,
      content: text[anchor],
    }));
}

function createFreeTextLayer(x, y, overrides = {}) {
  let fontSizeMode = overrides.fontSizeMode || "";
  let fontSizePx = overrides.fontSizePx;
  if (!fontSizeMode && overrides.fontSize) {
    if (typeof overrides.fontSize === "number") {
      fontSizeMode = "custom";
      fontSizePx = overrides.fontSize;
    } else if (typeof overrides.fontSize === "string") {
      fontSizeMode = overrides.fontSize;
    }
  }
  return normalizeTextLayer({
    kind: "free",
    x,
    y,
    content: overrides.content || "",
    fontSizeMode,
    fontSizePx,
  });
}

function createSlot(templateId = "blank", text = {}, presetId = "cinematic") {
  return {
    templateId,
    textLayers: createAnchoredTextLayers(text),
    style: createStyle(presetId),
  };
}

function hydrateSlot(slot = {}) {
  const textLayers = Array.isArray(slot.textLayers) && slot.textLayers.length
    ? slot.textLayers.map((layer) => normalizeTextLayer(layer))
    : [
        ...createAnchoredTextLayers(slot.text || {}),
        ...((slot.positionedTexts || []).map((pt) => createFreeTextLayer(pt.x, pt.y, {
          content: pt.content,
          fontSizeMode: pt.fontSizeMode || "",
          fontSizePx: pt.fontSizePx,
          fontSize: pt.fontSize,
        }))),
      ];

  return {
    templateId: slot.templateId || "blank",
    textLayers,
    style: createStyle(slot.style && slot.style.preset || "", slot.style || {}),
  };
}

function hydrateScene(scene = {}) {
  return {
    durationMs: Number(scene.durationMs) || 420,
    layout: {
      kind: scene.layout && scene.layout.kind || "single",
      padding: scene.layout && Number(scene.layout.padding) || 0,
      border: scene.layout && Number(scene.layout.border) || 0,
      effect: scene.layout && scene.layout.effect || "none",
      customEffects: clone(scene.layout && scene.layout.customEffects || []),
    },
    slots: (scene.slots || []).map((slot) => hydrateSlot(slot)),
    transition: scene.transition ? clone(scene.transition) : null,
  };
}

function hydrateDocument(doc = {}) {
  const hydrated = {
    canvas: clone(doc.canvas || { width: 720, height: 720 }),
    output: clone(doc.output || { format: "png" }),
    scenes: (doc.scenes || []).map((scene) => hydrateScene(scene)),
  };

  let maxNumericId = 0;
  hydrated.scenes.forEach((scene) => {
    scene.slots.forEach((slot) => {
      slot.textLayers.forEach((layer) => {
        const match = /^text-layer-(\d+)$/.exec(layer.id);
        if (match) maxNumericId = Math.max(maxNumericId, Number(match[1]));
      });
    });
  });
  state.textLayerSeq = Math.max(state.textLayerSeq, maxNumericId + 1);
  return hydrated;
}

function serializeSlotTextLayers(slot) {
  const text = { top: "", center: "", bottom: "" };
  const positionedTexts = [];

  (slot.textLayers || []).forEach((layer) => {
    if (!layer.content) return;
    if (layer.kind === "anchored" && TEXT_LAYER_ANCHORS.includes(layer.anchor)) {
      text[layer.anchor] = layer.content;
      return;
    }
    const entry = {
      content: layer.content,
      x: Math.round(layer.x),
      y: Math.round(layer.y),
    };
    if (layer.fontSizeMode === "custom") {
      entry.fontSizeMode = "custom";
      entry.fontSizePx = Math.round(layer.fontSizePx);
    } else if (layer.fontSizeMode) {
      entry.fontSizeMode = layer.fontSizeMode;
    }
    positionedTexts.push(entry);
  });

  return { text, positionedTexts };
}

function cloneSceneWithFreshTextLayerIds(scene) {
  const cloned = hydrateScene(clone(scene));
  cloned.slots.forEach((slot) => {
    slot.textLayers = slot.textLayers.map((layer) => normalizeTextLayer({
      ...layer,
      id: nextTextLayerId(),
    }));
  });
  return cloned;
}

function slotCountForLayout(kind) {
  const layout = state.metadata.layouts.find((item) => item.id === kind);
  return layout ? layout.slotCount : 1;
}

function createScene(kind = "single", seedSlots = []) {
  const slotCount = slotCountForLayout(kind);
  const slots = seedSlots.slice(0, slotCount).map((s) => hydrateSlot(clone(s)));
  while (slots.length < slotCount) slots.push(createSlot());
  return {
    durationMs: 420,
    layout: { kind, padding: 0, border: 0, effect: "none", customEffects: [] },
    slots,
    transition: { type: "crossfade", durationMs: 150, easing: "ease" },
  };
}

function preserveSlots(existingSlots, layoutKind) {
  const targetCount = slotCountForLayout(layoutKind);
  const next = existingSlots.slice(0, targetCount).map((s) => hydrateSlot(clone(s)));
  while (next.length < targetCount) next.push(createSlot());
  return next;
}

function buildDefaultDocument() {
  return {
    canvas: { width: 720, height: 720 },
    output: { format: "gif" },
    scenes: [
      { durationMs: 2000,
        layout: { kind: "single", padding: 0, border: 0, effect: "none" },
        slots: [createSlot("meme.shrek_smirk", { bottom: "Ask AI to fix the bug" }, "chill")],
        transition: null },
      { durationMs: 1500,
        layout: { kind: "single", padding: 0, border: 0, effect: "none" },
        slots: [createSlot("meme.king_bach_stare", { bottom: "AI refactors entire codebase" }, "cinematic")],
        transition: { type: "slideLeft", durationMs: 1000, easing: "ease" } },
      { durationMs: 2000,
        layout: { kind: "single", padding: 0, border: 0, effect: "none" },
        slots: [createSlot("meme.kid_crying", { bottom: "Now nothing compiles" }, "panic")],
        transition: { type: "fadeBlack", durationMs: 1200, easing: "easeOut" } },
      { durationMs: 2500,
        layout: { kind: "single", padding: 0, border: 0, effect: "none" },
        slots: [createSlot("meme.jordan_crying", { bottom: "git reset --hard" }, "shout")],
        transition: { type: "zoom", durationMs: 1000, easing: "easeIn" } },
    ],
  };
}

function presetDocuments() {
  return [
    // ── Stills ──
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
    { id: "quad-board", label: "Quad Board", description: "Four-slot grid for comparison and multi-beat punchlines.",
      build: () => ({ canvas: { width: 900, height: 900 }, output: { format: "png" },
        scenes: [{ durationMs: 900, layout: { kind: "grid2x2", padding: 12, border: 2, effect: "retro" },
          slots: [
            createSlot("blank", { center: "HOOK" }, "shout"),
            createSlot("blank", { center: "CONFLICT" }, "cinematic"),
            createSlot("blank", { center: "TWIST" }, "whisper"),
            createSlot("blank", { center: "PAYOFF" }, "chill"),
          ] }] }) },

    // ── Animated with transitions ──
    { id: "week-arc", label: "Week Arc", description: "Three-scene crossfade arc — Monday to Friday.",
      build: () => ({ canvas: { width: 720, height: 720 }, output: { format: "gif" },
        scenes: [
          { durationMs: 2000, layout: { kind: "single", padding: 0, border: 0, effect: "none" },
            slots: [createSlot("two_panel", { top: "MONDAY", bottom: "SHIPPING FEATURES" }, "cinematic")],
            transition: null },
          { durationMs: 2000, layout: { kind: "single", padding: 0, border: 0, effect: "comic" },
            slots: [createSlot("two_panel", { top: "WEDNESDAY", bottom: "FINDING THE EDGE CASE" }, "chill")],
            transition: { type: "crossfade", durationMs: 1000, easing: "ease" } },
          { durationMs: 2500, layout: { kind: "single", padding: 0, border: 0, effect: "glitch" },
            slots: [createSlot("two_panel", { top: "FRIDAY", bottom: "DEPLOYING ANYWAY" }, "panic")],
            transition: { type: "crossfade", durationMs: 1200, easing: "easeOut" } },
        ] }) },
    { id: "slide-story", label: "Slide Story", description: "Three scenes sliding up — scrolling story feel.",
      build: () => ({ canvas: { width: 720, height: 720 }, output: { format: "gif" },
        scenes: [
          { durationMs: 2500, layout: { kind: "single", padding: 0, border: 0, effect: "none" },
            slots: [createSlot("blank", { top: "ONCE UPON A TIME", bottom: "A developer had an idea" }, "cinematic")],
            transition: null },
          { durationMs: 2000, layout: { kind: "single", padding: 0, border: 0, effect: "none" },
            slots: [createSlot("dark", { top: "THEY CODED ALL NIGHT", bottom: "Coffee was involved" }, "chill")],
            transition: { type: "slideUp", durationMs: 1200, easing: "ease" } },
          { durationMs: 3000, layout: { kind: "single", padding: 0, border: 0, effect: "none" },
            slots: [createSlot("blank", { top: "IT ACTUALLY WORKED", bottom: "Somehow" }, "shout")],
            transition: { type: "slideUp", durationMs: 1500, easing: "easeOut" } },
        ] }) },
    { id: "dramatic-reveal", label: "Dramatic Reveal", description: "Fade to black between scenes — cinematic pacing.",
      build: () => ({ canvas: { width: 720, height: 960 }, output: { format: "gif" },
        scenes: [
          { durationMs: 2500, layout: { kind: "single", padding: 0, border: 0, effect: "none" },
            slots: [createSlot("dark", { center: "ACT I" }, "whisper")],
            transition: null },
          { durationMs: 2500, layout: { kind: "single", padding: 0, border: 0, effect: "none" },
            slots: [createSlot("dark", { center: "ACT II" }, "cinematic")],
            transition: { type: "fadeBlack", durationMs: 1500, easing: "easeOut" } },
          { durationMs: 3000, layout: { kind: "single", padding: 0, border: 0, effect: "none" },
            slots: [createSlot("dark", { center: "FIN" }, "shout")],
            transition: { type: "fadeBlack", durationMs: 2000, easing: "easeInOut" } },
        ] }) },
    { id: "quick-cuts", label: "Quick Cuts", description: "Four fast wipe cuts — high energy montage.",
      build: () => ({ canvas: { width: 720, height: 720 }, output: { format: "gif" },
        scenes: [
          { durationMs: 1200, layout: { kind: "single", padding: 0, border: 0, effect: "none" },
            slots: [createSlot("blank", { center: "READY" }, "shout")],
            transition: null },
          { durationMs: 1000, layout: { kind: "single", padding: 0, border: 0, effect: "none" },
            slots: [createSlot("dark", { center: "SET" }, "chill")],
            transition: { type: "wipe", durationMs: 1000, easing: "easeInOut" } },
          { durationMs: 1000, layout: { kind: "single", padding: 0, border: 0, effect: "none" },
            slots: [createSlot("blank", { center: "GO" }, "panic")],
            transition: { type: "slideRight", durationMs: 1000, easing: "easeIn" } },
          { durationMs: 1500, layout: { kind: "single", padding: 0, border: 0, effect: "none" },
            slots: [createSlot("dark", { center: "!!!" }, "shout")],
            transition: { type: "slideLeft", durationMs: 1000, easing: "easeOut" } },
        ] }) },
    { id: "zoom-loop", label: "Zoom Loop", description: "Three scenes with zoom transitions — escalating intensity.",
      build: () => ({ canvas: { width: 720, height: 720 }, output: { format: "gif" },
        scenes: [
          { durationMs: 2000, layout: { kind: "single", padding: 0, border: 0, effect: "none" },
            slots: [createSlot("meme.shrek_smirk", { bottom: "This is fine" }, "chill")],
            transition: null },
          { durationMs: 1500, layout: { kind: "single", padding: 0, border: 0, effect: "none" },
            slots: [createSlot("meme.king_bach_stare", { bottom: "Wait what" }, "cinematic")],
            transition: { type: "zoom", durationMs: 1200, easing: "easeIn" } },
          { durationMs: 2500, layout: { kind: "single", padding: 0, border: 0, effect: "none" },
            slots: [createSlot("meme.kid_crying", { bottom: "NOT FINE" }, "panic")],
            transition: { type: "zoom", durationMs: 1500, easing: "easeInOut" } },
        ] }) },
  ];
}

function buildPayload(extra = {}) {
  return {
    canvas: clone(state.canvas),
    output: clone(state.output),
    scenes: state.scenes.map((scene, i) => ({
      durationMs: scene.durationMs,
      layout: { ...clone(scene.layout), customEffects: scene.layout.customEffects || [] },
      transition: (i > 0 && scene.transition && scene.transition.type !== "cut")
        ? clone(scene.transition) : null,
      slots: scene.slots.map((slot) => {
        const serialized = serializeSlotTextLayers(slot);
        return {
          templateId: slot.templateId,
          text: serialized.text,
          positionedTexts: serialized.positionedTexts,
          style: {
            preset: slot.style.preset || "", color: slot.style.color,
            outline: slot.style.outline, outlineColor: slot.style.outlineColor,
            shadow: slot.style.shadow, shadowColor: slot.style.shadowColor,
            fontSizeMode: slot.style.fontSizeMode, fontSizePx: slot.style.fontSizePx,
            background: slot.style.background || "",
          },
        };
      }),
    })),
    ...extra,
  };
}
