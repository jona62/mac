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

  const sel = $("addEffectSelect");
  if (sel) {
    const defs = state.metadata.effectDefinitions || [];
    if (defs.length && sel.options.length !== defs.length) {
      sel.innerHTML = defs.map((d) =>
        `<option value="${escAttr(d.id)}">${esc(d.name)}${d.param ? ` (${d.param})` : ""}</option>`
      ).join("");
    }
  }
}

function renderInspectorMeta() {
  const scene = selectedScene();
  const slot = selectedSlot();
  if (!scene || !slot) return;

  $("slotMeta").textContent = `Slot ${state.selectedSlotIndex + 1} of ${scene.slots.length}`;
  $("slotTemplateLabel").textContent = templateName(slot.templateId);
  $("slotHint").textContent = scene.slots.length > 1
    ? "Select a slot here, or click directly into a cell on the stage to make that slot active."
    : "Canvas text you place on the stage stays tied to this slot and inherits its style.";

  const selectedLayer = selectedTextLayer();
  const layerCount = (slot.textLayers || []).length;
  $("textLayerMeta").textContent = selectedLayer
    ? `${layerDisplayName(selectedLayer)} active`
    : `${layerCount} layer${layerCount === 1 ? "" : "s"}`;
}

function renderTextLayerList() {
  const list = $("textLayerList");
  if (!list) return;
  const slot = selectedSlot();
  const layers = slot ? (slot.textLayers || []) : [];
  if (!layers.length) {
    list.innerHTML = '<p class="empty-state empty-state--inline">No text yet. Click the stage or use one of the quick-add buttons.</p>';
    return;
  }

  list.innerHTML = layers.map((layer) => {
    const active = layer.id === state.selectedTextLayerId;
    const editing = layer.id === state.editingTextLayerId;
    const fontBadge = layer.fontSizeMode
      ? (layer.fontSizeMode === "custom" ? `${Math.round(layer.fontSizePx || 64)}px` : layer.fontSizeMode.toUpperCase())
      : "Slot";
    return `<div class="text-layer-row${active ? " is-active" : ""}${editing ? " is-editing" : ""}">
      <button class="text-layer-row__main" type="button" data-text-layer-select="${escAttr(layer.id)}">
        <span class="text-layer-row__eyebrow">${esc(layerDisplayName(layer))}</span>
        <span class="text-layer-row__summary">${esc(layerSummary(layer))}</span>
      </button>
      <div class="text-layer-row__meta">
        <span class="text-layer-row__badge">${esc(fontBadge)}</span>
        <button class="text-layer-row__icon" type="button" data-text-layer-edit="${escAttr(layer.id)}" title="Edit layer">Edit</button>
        <button class="text-layer-row__icon" type="button" data-text-layer-delete="${escAttr(layer.id)}" title="Delete layer">×</button>
      </div>
    </div>`;
  }).join("");
}

function textLayerFontSizeOptions(selectedMode) {
  const options = ['<option value="">Match slot style</option>'];
  FONT_SIZE_OPTIONS.forEach((option) => {
    options.push(
      `<option value="${escAttr(option.id)}"${option.id === selectedMode ? " selected" : ""}>${esc(option.label)}</option>`
    );
  });
  return options.join("");
}

function renderTextLayerEditor() {
  const editor = $("textLayerEditor");
  if (!editor) return;
  const layer = selectedTextLayer();
  if (!layer) {
    editor.innerHTML = '<p class="empty-state empty-state--inline">Choose a text layer to edit it here, or click the canvas to create a new one.</p>';
    return;
  }

  const bounds = selectedSlotCanvasRect();
  const local = textLayerLocalPoint(layer, bounds);
  const isFree = layer.kind === "free";
  const showCustomSize = layer.fontSizeMode === "custom";

  editor.innerHTML = `
    <div class="text-layer-editor__card">
      <div class="text-layer-editor__topline">
        <div>
          <p class="text-layer-editor__eyebrow">${esc(layerDisplayName(layer))}</p>
          <p class="text-layer-editor__sub">${isFree ? "Free-position text" : "Anchored text"}${isFree ? "" : " — drag it on the stage to convert it into a free layer."}</p>
        </div>
        <button class="btn btn--ghost btn--sm" id="deleteTextLayerBtn" type="button">Delete</button>
      </div>
      <label class="field">
        <span class="field__label">Content</span>
        <textarea id="textLayerContent" rows="3" placeholder="Write directly here or type on the stage.">${esc(layer.content)}</textarea>
      </label>
      <div class="text-layer-editor__grid">
        <label class="field${isFree ? "" : " is-disabled"}">
          <span class="field__label">X</span>
          <input type="number" id="textLayerX" min="0" max="${Math.max(1, bounds.width)}" step="1" value="${Math.round(local.x)}" ${isFree ? "" : "disabled"} />
        </label>
        <label class="field${isFree ? "" : " is-disabled"}">
          <span class="field__label">Y</span>
          <input type="number" id="textLayerY" min="0" max="${Math.max(1, bounds.height)}" step="1" value="${Math.round(local.y)}" ${isFree ? "" : "disabled"} />
        </label>
      </div>
      <label class="field">
        <span class="field__label">Font Size Override</span>
        <select id="textLayerFontSizeMode">${textLayerFontSizeOptions(layer.fontSizeMode)}</select>
      </label>
      <label class="field" id="textLayerFontSizePxField" ${showCustomSize ? "" : "hidden"}>
        <span class="field__label">Custom Size (px)</span>
        <input type="number" id="textLayerFontSizePx" min="${state.metadata.limits.fontSizePx.min}" max="${state.metadata.limits.fontSizePx.max}" step="1" value="${Math.round(layer.fontSizePx || 64)}" />
      </label>
    </div>
  `;
}

function renderPosOverlay() {
  const overlay = $("posOverlay");
  const img = $("stageImage");
  if (!overlay) return;

  // Hide overlay when a rendered image is showing or during render
  if (state.isPreviewing || state.stageAssetUrl) { overlay.innerHTML = ""; return; }

  const slot = selectedSlot();
  const layers = slot ? (slot.textLayers || []) : [];
  if (!layers.length) {
    overlay.innerHTML = "";
    return;
  }

  const slotRect = selectedSlotCanvasRect();
  const stageRect = overlay.parentElement.getBoundingClientRect();
  // Use image rect when visible, fall back to stage rect when editing without preview
  const hasImg = img && !img.hidden && img.naturalWidth > 0;
  const refRect = hasImg ? img.getBoundingClientRect() : stageRect;
  const offX = refRect.left - stageRect.left;
  const offY = refRect.top - stageRect.top;
  const scaleX = refRect.width / state.canvas.width;
  const scaleY = refRect.height / state.canvas.height;
  const style = slot.style || {};

  overlay.innerHTML = layers.map((layer) => {
    const local = textLayerLocalPoint(layer, slotRect);
    const left = offX + (slotRect.x + local.x) * scaleX;
    const top = offY + (slotRect.y + local.y) * scaleY;
    const isEditing = layer.id === state.editingTextLayerId;
    const selectedClass = layer.id === state.selectedTextLayerId ? " is-selected" : "";
    const draggingClass = layer.id === state.draggingTextLayerId ? " is-dragging" : "";
    const commonStyle = `left:${left}px;top:${top}px;--layer-color:${escAttr(style.color || "#FFFFFF")};--layer-outline:${escAttr(style.outlineColor || "#111111")};`;

    if (isEditing) {
      return `<textarea class="pos-editor${selectedClass}" data-layer-editor="${escAttr(layer.id)}" style="${commonStyle}" placeholder="Type here">${esc(layer.content)}</textarea>`;
    }

    return `<button class="pos-label${selectedClass}${draggingClass}" type="button" data-text-layer-id="${escAttr(layer.id)}" style="${commonStyle}">
      <span class="pos-label__tag">${esc(layerDisplayName(layer))}</span>
      <span class="pos-label__text">${esc(layer.content || "Text")}</span>
    </button>`;
  }).join("");

  const editor = overlay.querySelector(".pos-editor");
  if (editor && document.activeElement !== editor) {
    editor.focus({ preventScroll: true });
    const length = editor.value.length;
    editor.setSelectionRange(length, length);
  }
}

function renderAll() {
  ensureOutputCompatibility(false);
  ensureTextLayerSelection();
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
  requestAnimationFrame(renderPosOverlay);
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
    if (i > 0) {
      const trans = scene.transition || { type: "cut", durationMs: 150, easing: "linear" };
      const label = trans.type === "cut" ? "cut" : trans.type;
      parts.push(`<button class="transition-pip" type="button" data-transition-index="${i}" title="${label} ${trans.durationMs}ms ${trans.easing}">
        <span class="transition-pip__label">${esc(label)}</span>
      </button>`);
    }
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
  return slotTextPreview(first) || "Empty scene";
}

function renderSlotTabs() {
  const scene = selectedScene();
  if (!scene) {
    $("slotTabs").innerHTML = "";
    return;
  }
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

  renderInspectorMeta();
  renderTextLayerList();
  renderTextLayerEditor();

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

  let img = $("stageImage");
  if (hasAsset) {
    if (img.hidden || img.dataset.assetUrl !== state.stageAssetUrl) {
      const newImg = document.createElement("img");
      newImg.id = "stageImage";
      newImg.alt = "Studio preview";
      newImg.src = state.stageAssetUrl;
      newImg.dataset.assetUrl = state.stageAssetUrl;
      img.replaceWith(newImg);
      img = newImg;
    } else {
      img.hidden = false;
    }
  } else {
    img.hidden = true;
  }

  const compact = window.innerWidth < 800;
  $("stageLabel").textContent = hasAsset ? state.stageLabel : "Click Preview to render";
  $("refreshPreviewBtn").disabled = state.isPreviewing;
  $("refreshPreviewBtn").textContent = state.isPreviewing
    ? "Rendering…"
    : (compact ? "Preview" : "Preview");
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
