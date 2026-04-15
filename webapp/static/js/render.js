// Mac Studio — All render* functions (DOM updates)

function renderEffectChain() {
  const scene = selectedScene();
  if (!scene) return;
  const chain = scene.layout.customEffects || [];
  const builder = $("effectChainBuilder");
  builder.innerHTML = chain.map((item, i) => {
    const defn = state.metadata.effectDefinitions.find((d) => d.id === item.id);
    const hasParam = defn && defn.param;
    return `<div class="fx-item">
      <span class="fx-item__name">${esc(defn ? defn.name : item.id)}</span>
      ${hasParam ? `<input class="fx-item__param" type="number" min="${defn.min}" max="${defn.max}" step="${defn.step}" value="${item.param != null ? item.param : defn.default}" data-fx-index="${i}" />` : ""}
      <button class="fx-item__del" data-fx-del="${i}" type="button">&times;</button>
    </div>`;
  }).join("");

  // Populate the add dropdown
  const sel = $("addEffectSelect");
  if (sel && !sel.options.length) {
    sel.innerHTML = state.metadata.effectDefinitions.map((d) =>
      `<option value="${escAttr(d.id)}">${esc(d.name)}${d.param ? ` (${d.param})` : ""}</option>`
    ).join("");
  }
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
  renderEffectChain();
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
    const isUpload = t.category === "uploads";
    return `<div class="template-card${active ? " is-active" : ""}${isUpload ? " template-card--upload" : ""}">
      <button class="template-card__pick" type="button" data-template-id="${escAttr(t.id)}" title="${escAttr(t.description)}">
        <div class="template-card__thumb" style="background-image:url('${cssUrl(t.previewUrl)}')"></div>
        <span class="template-card__name">${esc(t.name)}</span>
      </button>${isUpload ? `<button class="template-card__del" type="button" data-delete-upload="${escAttr(t.id)}" title="Remove">×</button>` : ""}</div>`;
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
  const parts = [];
  state.scenes.forEach((scene, i) => {
    // Transition connector between scenes
    if (i > 0) {
      const trans = scene.transition || { type: "cut", durationMs: 150, easing: "linear" };
      const label = trans.type === "cut" ? "cut" : trans.type;
      parts.push(`<button class="transition-pip" type="button" data-transition-index="${i}" title="${label} ${trans.durationMs}ms ${trans.easing}">
        <span class="transition-pip__label">${esc(label)}</span>
      </button>`);
    }
    // Scene pill
    const active = i === state.selectedSceneIndex;
    parts.push(`<button class="scene-card${active ? " is-active" : ""}" type="button" data-scene-action="select" data-scene-index="${i}">
      <span class="scene-card__label">Scene ${i + 1}</span>
      <span class="scene-card__meta">${formatDuration(scene.durationMs)}</span>
      <span class="scene-card__actions">
        <span class="icon-btn" data-scene-action="dup" data-scene-index="${i}">+</span>
        <span class="icon-btn" data-scene-action="del" data-scene-index="${i}" ${state.scenes.length === 1 ? "style='display:none'" : ""}>×</span>
      </span>
    </button>`);
  });
  // Loop-back transition pip after last scene (only for multi-scene)
  if (state.scenes.length > 1) {
    const last = state.scenes[state.scenes.length - 1];
    const loopTrans = last.transition || { type: "cut", durationMs: 150, easing: "linear" };
    const loopLabel = loopTrans.type === "cut" ? "cut" : loopTrans.type;
    parts.push(`<button class="transition-pip transition-pip--loop" type="button" data-transition-loop title="Loop back: ${loopLabel} ${loopTrans.durationMs}ms">
      <span class="transition-pip__label">${esc(loopLabel)}</span>
      <span class="transition-pip__loop-icon">↩</span>
    </button>`);
  }
  $("sceneStrip").innerHTML = parts.join("");
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
  const bg = slot.style.background || "";
  $("slotBgColor").value = bg && bg.startsWith("#") ? bg.slice(0, 7) : "#000000";
  $("slotBgColorHex").value = bg || "";
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
  $("stagePlaceholder").hidden = hasAsset;
  const oldImg = $("stageImage");
  if (hasAsset) {
    // Replace <img> element to force GIF animation restart
    const newImg = document.createElement("img");
    newImg.id = "stageImage";
    newImg.alt = "Studio preview";
    newImg.src = state.stageAssetUrl;
    oldImg.replaceWith(newImg);
  } else {
    oldImg.hidden = true;
  }
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
