// Mac Studio — All render* functions (DOM updates)

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

function renderStatus() {
  $("statusLine").textContent = state.status.line;
  $("statusLine").className = `status${state.status.kind === "error" ? " status--error" : ""}`;
  $("statusMeta").textContent = state.status.meta;
}

function renderStats() {
  $("statCanvas").textContent = `${state.canvas.width}×${state.canvas.height}`;
  $("statScenes").textContent = String(state.scenes.length);
  const totalMs = state.scenes.reduce((sum, s) => sum + s.durationMs, 0);
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
  renderTemplateGrid("uploadTemplateGrid", "uploads");
}

function renderTemplateGrid(containerId, category) {
  const container = $(containerId);
  const slot = selectedSlot();
  const templates = state.metadata.templates.filter((t) => t.category === category);
  if (!templates.length) {
    container.innerHTML = '<p class="empty-state">No templates in this shelf yet.</p>';
    return;
  }
  container.innerHTML = templates.map((t) => {
    const active = slot && slot.templateId === t.id;
    return `<button class="template-card${active ? " is-active" : ""}" type="button" data-template-id="${escAttr(t.id)}" title="${escAttr(t.description)}">
      <div class="template-card__thumb" style="background-image:url('${cssUrl(t.previewUrl)}')"></div>
      <span class="template-card__name">${esc(t.name)}</span></button>`;
  }).join("");
}

function renderLayoutGrid() {
  const scene = selectedScene();
  $("layoutGrid").innerHTML = state.metadata.layouts.map((l) => `
    <button class="layout-card${scene && scene.layout.kind === l.id ? " is-active" : ""}" type="button" data-layout-id="${escAttr(l.id)}">
      <span class="layout-card__name">${esc(l.name)}</span>
      <span class="layout-card__meta">${l.slotCount} slot${l.slotCount > 1 ? "s" : ""}</span>
      <span class="layout-card__desc">${esc(l.description)}</span>
    </button>`).join("");
}

function renderPresetGrid() {
  $("presetGrid").innerHTML = presetDocuments().map((p) => `
    <button class="preset-card" type="button" data-preset-id="${escAttr(p.id)}">
      <span class="preset-card__name">${esc(p.label)}</span>
      <span class="preset-card__desc">${esc(p.description)}</span>
    </button>`).join("");
}

function renderSceneStrip() {
  $("sceneStrip").innerHTML = state.scenes.map((scene, i) => {
    const active = i === state.selectedSceneIndex;
    const lead = sceneLeadText(scene);
    return `<article class="scene-card${active ? " is-active" : ""}">
      <button class="scene-card__body" type="button" data-scene-action="select" data-scene-index="${i}">
        <span class="scene-card__kicker">Scene ${i + 1}</span>
        <strong class="scene-card__title">${esc(layoutName(scene.layout.kind))}</strong>
        <span class="scene-card__summary">${esc(lead)}</span>
        <span class="scene-card__meta">${scene.slots.length} slot${scene.slots.length > 1 ? "s" : ""} · ${formatDuration(scene.durationMs)}</span>
      </button>
      <div class="scene-card__actions">
        <button class="icon-btn" type="button" data-scene-action="up" data-scene-index="${i}" ${i === 0 ? "disabled" : ""}>↑</button>
        <button class="icon-btn" type="button" data-scene-action="down" data-scene-index="${i}" ${i === state.scenes.length - 1 ? "disabled" : ""}>↓</button>
        <button class="icon-btn" type="button" data-scene-action="dup" data-scene-index="${i}">Dup</button>
        <button class="icon-btn" type="button" data-scene-action="del" data-scene-index="${i}" ${state.scenes.length === 1 ? "disabled" : ""}>×</button>
      </div></article>`;
  }).join("");
}

function sceneLeadText(scene) {
  const first = scene.slots[0];
  const snippets = [first.text.top, first.text.center, first.text.bottom].filter(Boolean);
  return snippets[0] || "Empty scene";
}

function renderSlotTabs() {
  const scene = selectedScene();
  if (!scene) { $("slotTabs").innerHTML = ""; return; }
  $("slotTabs").innerHTML = scene.slots.map((slot, i) => `
    <button class="slot-tab${i === state.selectedSlotIndex ? " is-active" : ""}" type="button" data-slot-index="${i}">
      <span class="slot-tab__label">Slot ${i + 1}</span>
      <span class="slot-tab__meta">${esc(templateName(slot.templateId))}</span>
    </button>`).join("");
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
  document.querySelectorAll("[data-format]").forEach((btn) => {
    btn.classList.toggle("is-active", btn.dataset.format === state.output.format);
    if (btn.dataset.format === "png") btn.disabled = pngDisabled;
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
  if (hasAsset) $("stageImage").src = state.stageAssetUrl;
  $("stageLabel").textContent = state.stageLabel;
  const compact = window.innerWidth < 800;
  $("refreshPreviewBtn").disabled = state.isPreviewing;
  $("refreshPreviewBtn").textContent = state.isPreviewing ? "Refreshing…" : (compact ? "Refresh" : "Refresh Preview");
  $("exportBtn").disabled = state.isExporting;
  $("exportBtn").textContent = state.isExporting ? "Exporting…" : (compact ? "Download" : "Export & Download");
  if (state.lastExport && state.lastExport.downloadUrl) {
    $("downloadLink").hidden = false;
    $("downloadLink").href = state.lastExport.downloadUrl;
    $("downloadLink").download = `mac-studio-export.${state.output.format || "png"}`;
  } else {
    $("downloadLink").hidden = true;
  }
}

function renderScript() {
  const code = state.script || "// Studio script will appear here.\n";
  $("scriptPreview").value = code;
  $("scriptHighlight").innerHTML = highlightMac(code);
}
