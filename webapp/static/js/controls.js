// Mac Studio — Event bindings and control handlers

function bindStaticControls() {
  bindRangePair("canvasWidthRange", "canvasWidthInput", (v) => {
    state.canvas.width = clamp(v, state.metadata.limits.width.min, state.metadata.limits.width.max);
    commitChange();
  });
  bindRangePair("canvasHeightRange", "canvasHeightInput", (v) => {
    state.canvas.height = clamp(v, state.metadata.limits.height.min, state.metadata.limits.height.max);
    commitChange();
  });
  bindRangePair("sceneDurationRange", "sceneDurationInput", (v) => {
    const scene = selectedScene();
    if (!scene) return;
    scene.durationMs = clamp(v, state.metadata.limits.duration.min, state.metadata.limits.duration.max);
    commitChange();
  });
  bindRangePair("scenePaddingRange", "scenePaddingInput", (v) => {
    const scene = selectedScene();
    if (!scene) return;
    scene.layout.padding = clamp(v, state.metadata.limits.padding.min, state.metadata.limits.padding.max);
    commitChange();
  });
  bindRangePair("sceneBorderRange", "sceneBorderInput", (v) => {
    const scene = selectedScene();
    if (!scene) return;
    scene.layout.border = clamp(v, state.metadata.limits.border.min, state.metadata.limits.border.max);
    commitChange();
  });
  bindRangePair("slotOutlineRange", "slotOutlineInput", (v) => {
    const slot = selectedSlot();
    if (!slot) return;
    slot.style.outline = clamp(v, state.metadata.limits.outline.min, state.metadata.limits.outline.max);
    commitChange();
  });
  bindRangePair("slotShadowRange", "slotShadowInput", (v) => {
    const slot = selectedSlot();
    if (!slot) return;
    slot.style.shadow = clamp(v, state.metadata.limits.shadow.min, state.metadata.limits.shadow.max);
    commitChange();
  });

  $("sceneEffectSelect").addEventListener("change", (e) => {
    const scene = selectedScene();
    if (!scene) return;
    scene.layout.effect = e.target.value;
    commitChange();
  });

  $("slotStylePreset").addEventListener("change", (e) => {
    const slot = selectedSlot();
    if (!slot) return;
    slot.style = createStyle(e.target.value || "");
    commitChange();
  });

  $("slotTextColor").addEventListener("input", (e) => updateHexField("color", e.target.value, false));
  $("slotTextColorHex").addEventListener("change", (e) => updateHexField("color", e.target.value, false));
  $("slotOutlineColor").addEventListener("input", (e) => updateHexField("outlineColor", e.target.value, false));
  $("slotOutlineColorHex").addEventListener("change", (e) => updateHexField("outlineColor", e.target.value, false));
  $("slotShadowColorHex").addEventListener("change", (e) => updateHexField("shadowColor", e.target.value, true));
  $("slotBgColor").addEventListener("input", (e) => updateHexField("background", e.target.value, false));
  $("slotBgColorHex").addEventListener("change", (e) => {
    const slot = selectedSlot();
    if (!slot) return;
    const value = e.target.value.trim();
    if (value === "" || value.toLowerCase() === "none") slot.style.background = "";
    else slot.style.background = normalizeHex(value, slot.style.background || "", true);
    commitChange({ schedule: false });
    renderInspector();
    schedulePreview();
  });

  $("slotTextTransform").addEventListener("change", (e) => {
    const s = selectedSlot(); if (!s) return;
    s.style.textTransform = e.target.value;
    commitChange();
  });
  $("slotFontWeight").addEventListener("change", (e) => {
    const s = selectedSlot(); if (!s) return;
    s.style.fontWeight = e.target.value;
    commitChange();
  });

  $("slotFontSizeMode").addEventListener("change", (e) => {
    const slot = selectedSlot();
    if (!slot) return;
    slot.style.fontSizeMode = e.target.value;
    if (slot.style.fontSizeMode !== "custom") {
      slot.style.fontSizePx = clamp(slot.style.fontSizePx, 8, 240);
    }
    commitChange();
  });
  $("slotFontSizePx").addEventListener("change", (e) => {
    const slot = selectedSlot();
    if (!slot) return;
    slot.style.fontSizePx = clamp(e.target.value, state.metadata.limits.fontSizePx.min, state.metadata.limits.fontSizePx.max);
    commitChange();
  });

  $("formatToggle").addEventListener("click", (e) => {
    const btn = e.target.closest("[data-format]");
    if (!btn || btn.disabled) return;
    state.output.format = btn.dataset.format;
    commitChange({ schedule: false });
    schedulePreview(40);
  });

  $("addSceneBtn").addEventListener("click", () => {
    const index = state.selectedSceneIndex + 1;
    state.scenes.splice(index, 0, createScene("single"));
    state.selectedSceneIndex = index;
    state.selectedSlotIndex = 0;
    state.selectedTextLayerId = null;
    state.editingTextLayerId = null;
    commitChange();
  });

  $("addEffectBtn").addEventListener("click", () => {
    const scene = selectedScene();
    if (!scene) return;
    const id = $("addEffectSelect").value;
    if (!id) return;
    const defn = state.metadata.effectDefinitions.find((d) => d.id === id);
    const entry = { id };
    if (defn && defn.param) entry.param = defn.default;
    if (!scene.layout.customEffects) scene.layout.customEffects = [];
    scene.layout.customEffects.push(entry);
    commitChange();
  });
  $("effectChainBuilder").addEventListener("click", (e) => {
    const del = e.target.closest("[data-fx-del]");
    if (!del) return;
    const scene = selectedScene();
    if (!scene) return;
    scene.layout.customEffects.splice(Number(del.dataset.fxDel), 1);
    commitChange();
  });
  $("effectChainBuilder").addEventListener("input", (e) => {
    const input = e.target.closest("[data-fx-index]");
    if (!input) return;
    const scene = selectedScene();
    if (!scene) return;
    const index = Number(input.dataset.fxIndex);
    if (scene.layout.customEffects[index]) {
      scene.layout.customEffects[index].param = Number(input.value);
      commitChange();
    }
  });

  $("textLayerQuickAdd").addEventListener("click", (e) => {
    const btn = e.target.closest("[data-text-layer-add]");
    if (!btn) return;
    if (btn.dataset.textLayerAdd === "canvas") {
      const bounds = selectedSlotCanvasRect();
      createCanvasTextLayer(Math.round(bounds.width / 2), Math.round(bounds.height / 2), { edit: true });
      return;
    }
    createAnchoredTextLayer(btn.dataset.textLayerAdd);
  });

  $("textLayerList").addEventListener("click", (e) => {
    const del = e.target.closest("[data-text-layer-delete]");
    if (del) {
      deleteTextLayer(del.dataset.textLayerDelete);
      return;
    }
    const edit = e.target.closest("[data-text-layer-edit]");
    if (edit) {
      startTextLayerEditing(edit.dataset.textLayerEdit);
      return;
    }
    const select = e.target.closest("[data-text-layer-select]");
    if (select) {
      selectTextLayer(select.dataset.textLayerSelect);
      renderAll();
    }
  });

  $("textLayerEditor").addEventListener("input", (e) => {
    const layer = selectedTextLayer();
    if (!layer) return;
    if (e.target.id === "textLayerContent") {
      layer.content = e.target.value;
      const inlineEditor = document.querySelector(".pos-editor");
      if (inlineEditor && inlineEditor.value !== layer.content) inlineEditor.value = layer.content;
      renderTextLayerList();
      renderInspectorMeta();
      if (!state.editingTextLayerId) renderPosOverlay();
      schedulePreview();
      return;
    }
    if (e.target.id === "textLayerX" && layer.kind === "free") {
      const bounds = selectedSlotCanvasRect();
      layer.x = clamp(e.target.value, 0, bounds.width);
      renderPosOverlay();
      renderTextLayerList();
      schedulePreview();
      return;
    }
    if (e.target.id === "textLayerY" && layer.kind === "free") {
      const bounds = selectedSlotCanvasRect();
      layer.y = clamp(e.target.value, 0, bounds.height);
      renderPosOverlay();
      renderTextLayerList();
      schedulePreview();
    }
  });

  $("textLayerEditor").addEventListener("change", (e) => {
    const layer = selectedTextLayer();
    if (!layer) return;
    if (e.target.id === "textLayerFontSizeMode") {
      layer.fontSizeMode = e.target.value;
      renderTextLayerEditor();
      renderTextLayerList();
      renderInspectorMeta();
      schedulePreview();
      return;
    }
    if (e.target.id === "textLayerFontSizePx") {
      layer.fontSizePx = clamp(e.target.value, state.metadata.limits.fontSizePx.min, state.metadata.limits.fontSizePx.max);
      renderTextLayerEditor();
      renderTextLayerList();
      schedulePreview();
      return;
    }
    if (e.target.id === "textLayerX" && layer.kind === "free") {
      const bounds = selectedSlotCanvasRect();
      layer.x = clamp(e.target.value, 0, bounds.width);
      renderTextLayerEditor();
      renderPosOverlay();
      schedulePreview();
      return;
    }
    if (e.target.id === "textLayerY" && layer.kind === "free") {
      const bounds = selectedSlotCanvasRect();
      layer.y = clamp(e.target.value, 0, bounds.height);
      renderTextLayerEditor();
      renderPosOverlay();
      schedulePreview();
    }
  });

  $("textLayerEditor").addEventListener("click", (e) => {
    if (e.target.id === "deleteTextLayerBtn") deleteTextLayer();
  });

  $("stageFrame").addEventListener("click", (e) => {
    if (e.target.closest(".pos-label, .pos-editor")) return;
    if (state.editingTextLayerId) {
      stopTextLayerEditing();
      return;
    }
    const point = canvasPointFromEvent(e);
    if (!point) return;
    state.selectedSlotIndex = point.slotIndex;
    createCanvasTextLayer(point.localX, point.localY, { edit: true });
  });

  $("posOverlay").addEventListener("click", (e) => {
    const label = e.target.closest("[data-text-layer-id]");
    if (!label) return;
    selectTextLayer(label.dataset.textLayerId);
    renderAll();
  });

  $("posOverlay").addEventListener("dblclick", (e) => {
    const label = e.target.closest("[data-text-layer-id]");
    if (!label) return;
    startTextLayerEditing(label.dataset.textLayerId);
  });

  $("posOverlay").addEventListener("input", (e) => {
    const editor = e.target.closest("[data-layer-editor]");
    if (!editor) return;
    const slot = selectedSlot();
    const layer = textLayerById(slot, editor.dataset.layerEditor);
    if (!layer) return;
    layer.content = editor.value;
    const content = $("textLayerContent");
    if (content && content.value !== layer.content) content.value = layer.content;
    renderTextLayerList();
    renderInspectorMeta();
    schedulePreview();
  });

  $("posOverlay").addEventListener("keydown", (e) => {
    const editor = e.target.closest("[data-layer-editor]");
    if (!editor) return;
    if (e.key === "Escape") {
      e.preventDefault();
      stopTextLayerEditing();
      return;
    }
    if ((e.metaKey || e.ctrlKey) && e.key === "Enter") {
      e.preventDefault();
      stopTextLayerEditing();
    }
  });

  $("posOverlay").addEventListener("blur", (e) => {
    if (!e.target.closest("[data-layer-editor]")) return;
    stopTextLayerEditing();
  }, true);

  $("posOverlay").addEventListener("pointerdown", (e) => {
    const label = e.target.closest("[data-text-layer-id]");
    if (!label) return;
    beginTextLayerDrag(e, label.dataset.textLayerId);
  });

  $("refreshPreviewBtn").addEventListener("click", () => {
    if (state.stageAssetUrl) {
      // Stop: clear rendered output, go back to editing mode
      state.stageAssetUrl = "";
      state.stageLabel = "";
      renderAll();
    } else {
      // Preview: render the current scene
      schedulePreview(20, true);
    }
  });
  $("exportBtn").addEventListener("click", exportDocument);

  $("canvasTemplateGrid").addEventListener("click", onTemplatePick);
  $("memeTemplateGrid").addEventListener("click", onTemplatePick);
  $("uploadTemplateGrid").addEventListener("click", (e) => {
    const delBtn = e.target.closest("[data-delete-upload]");
    if (delBtn) {
      onDeleteUpload(delBtn.dataset.deleteUpload);
      return;
    }
    onTemplatePick(e);
  });
  $("imageUpload").addEventListener("change", onImageUpload);
  $("layoutGrid").addEventListener("click", onLayoutPick);
  $("presetGrid").addEventListener("click", onPresetPick);
  $("sceneStrip").addEventListener("click", (e) => {
    const loopPip = e.target.closest("[data-transition-loop]");
    if (loopPip) {
      onTransitionPipClick(loopPip, state.scenes.length - 1);
      return;
    }
    const transPip = e.target.closest("[data-transition-index]");
    if (transPip) {
      onTransitionPipClick(transPip, Number(transPip.dataset.transitionIndex));
      return;
    }
    onSceneStripAction(e);
  });
  $("slotTabs").addEventListener("click", onSlotTabAction);

  document.addEventListener("keydown", (e) => {
    if (isTypingTarget(e.target)) return;
    if (e.key === "Enter" && state.selectedTextLayerId && !state.editingTextLayerId) {
      e.preventDefault();
      startTextLayerEditing(state.selectedTextLayerId);
      return;
    }
    if (e.key === "Escape" && state.editingTextLayerId) {
      e.preventDefault();
      stopTextLayerEditing();
    }
  });

  bindSidebarTabs();
  bindScriptEditor();
  bindMobileToggles();
}

function bindRangePair(rangeId, inputId, onChange) {
  $(rangeId).addEventListener("input", (e) => onChange(Number(e.target.value)));
  $(inputId).addEventListener("change", (e) => onChange(Number(e.target.value)));
}

function ensureTextLayerSelection() {
  const slot = selectedSlot();
  if (!slot || !slot.textLayers || !slot.textLayers.length) {
    state.selectedTextLayerId = null;
    state.editingTextLayerId = null;
    return;
  }
  if (!textLayerById(slot, state.selectedTextLayerId)) {
    state.selectedTextLayerId = slot.textLayers[0].id;
  }
  if (!textLayerById(slot, state.editingTextLayerId)) {
    state.editingTextLayerId = null;
  }
}

function selectTextLayer(layerId, options = {}) {
  state.selectedTextLayerId = layerId;
  state.editingTextLayerId = options.edit ? layerId : null;
}

function startTextLayerEditing(layerId) {
  state.selectedTextLayerId = layerId;
  state.editingTextLayerId = layerId;
  renderAll();
}

function stopTextLayerEditing() {
  if (!state.editingTextLayerId) return;
  state.editingTextLayerId = null;
  renderAll();
}

function createAnchoredTextLayer(anchor) {
  const slot = selectedSlot();
  if (!slot) return null;
  const existing = anchoredLayerForSlot(slot, anchor);
  if (existing) {
    selectTextLayer(existing.id, { edit: true });
    renderAll();
    return existing;
  }
  const layer = normalizeTextLayer({
    kind: "anchored",
    anchor,
    content: "",
  });
  slot.textLayers.push(layer);
  selectTextLayer(layer.id, { edit: true });
  commitChange();
  return layer;
}

function createCanvasTextLayer(localX, localY, options = {}) {
  const slot = selectedSlot();
  if (!slot) return null;
  const bounds = selectedSlotCanvasRect();
  const layer = createFreeTextLayer(
    clamp(localX, 0, bounds.width),
    clamp(localY, 0, bounds.height),
    options
  );
  slot.textLayers.push(layer);
  selectTextLayer(layer.id, { edit: options.edit !== false });
  commitChange();
  return layer;
}

function deleteTextLayer(layerId = state.selectedTextLayerId) {
  const slot = selectedSlot();
  if (!slot) return;
  const index = (slot.textLayers || []).findIndex((layer) => layer.id === layerId);
  if (index === -1) return;
  slot.textLayers.splice(index, 1);
  state.selectedTextLayerId = slot.textLayers[index] ? slot.textLayers[index].id : (slot.textLayers[index - 1] ? slot.textLayers[index - 1].id : null);
  if (state.editingTextLayerId === layerId) state.editingTextLayerId = null;
  commitChange();
}

function setPreviewMode(mode, options = {}) {
  if (mode === state.previewMode && !options.force) return;
  state.previewMode = mode;
  if (mode === "paused") {
    clearTimeout(state.previewTimer);
    if (state.previewAbortController) {
      state.previewAbortController.abort();
      state.previewAbortController = null;
    }
    state.isPreviewing = false;
    state.stageAssetUrl = "";
    state.stageLabel = "Editing — start preview to render";
    if (options.message !== false) {
      setStatus("Preview stopped.", "Edit text on the canvas. Start preview to render.", "normal");
    }
  } else if (options.message !== false) {
    setStatus("Live preview on.", "Edits will re-render the selected scene as a still.", "normal");
  }
  renderAll();
}

function canvasPointFromEvent(event) {
  const img = $("stageImage");
  if (!img || img.hidden) return null;
  const rect = img.getBoundingClientRect();
  if (!rect.width || !rect.height) return null;
  if (event.clientX < rect.left || event.clientX > rect.right || event.clientY < rect.top || event.clientY > rect.bottom) {
    return null;
  }

  const sceneX = clamp((event.clientX - rect.left) * (state.canvas.width / rect.width), 0, state.canvas.width);
  const sceneY = clamp((event.clientY - rect.top) * (state.canvas.height / rect.height), 0, state.canvas.height);
  const scene = selectedScene();
  const slotIndex = slotIndexAtCanvasPoint(scene ? scene.layout.kind : "single", sceneX, sceneY, state.canvas);
  const slotRect = slotCanvasRect(scene ? scene.layout.kind : "single", slotIndex, state.canvas);
  return {
    sceneX,
    sceneY,
    slotIndex,
    localX: clamp(sceneX - slotRect.x, 0, slotRect.width),
    localY: clamp(sceneY - slotRect.y, 0, slotRect.height),
  };
}

function isTypingTarget(target) {
  return Boolean(target && (
    target.closest("textarea") ||
    target.closest("input") ||
    target.closest("select") ||
    target.isContentEditable
  ));
}

function beginTextLayerDrag(event, layerId) {
  if (event.button !== 0) return;
  const slot = selectedSlot();
  let layer = textLayerById(slot, layerId);
  if (!slot || !layer) return;

  const point = canvasPointFromEvent(event);
  if (!point) return;

  event.preventDefault();
  selectTextLayer(layerId);
  renderTextLayerList();
  renderTextLayerEditor();
  renderInspectorMeta();

  const bounds = selectedSlotCanvasRect();
  const startLocal = textLayerLocalPoint(layer, bounds);
  const startSceneX = point.sceneX;
  const startSceneY = point.sceneY;
  let dragging = false;

  const onMove = (moveEvent) => {
    if (moveEvent.pointerId !== event.pointerId) return;
    const next = canvasPointFromEvent(moveEvent);
    if (!next) return;
    const dx = next.sceneX - startSceneX;
    const dy = next.sceneY - startSceneY;
    if (!dragging && Math.abs(dx) < 2 && Math.abs(dy) < 2) return;

    if (!dragging) {
      dragging = true;
      if (layer.kind === "anchored") {
        const anchorPoint = anchorLocalPoint(layer.anchor, bounds);
        layer.kind = "free";
        layer.anchor = null;
        layer.x = anchorPoint.x;
        layer.y = anchorPoint.y;
      }
      state.draggingTextLayerId = layer.id;
    }

    layer.x = clamp(startLocal.x + dx, 0, bounds.width);
    layer.y = clamp(startLocal.y + dy, 0, bounds.height);
    renderTextLayerList();
    renderTextLayerEditor();
    renderInspectorMeta();
    renderPosOverlay();
  };

  const onUp = (upEvent) => {
    if (upEvent.pointerId !== event.pointerId) return;
    window.removeEventListener("pointermove", onMove);
    window.removeEventListener("pointerup", onUp);
    window.removeEventListener("pointercancel", onUp);

    if (!dragging) {
      renderAll();
      return;
    }

    state.draggingTextLayerId = null;
    commitChange();
  };

  window.addEventListener("pointermove", onMove);
  window.addEventListener("pointerup", onUp);
  window.addEventListener("pointercancel", onUp);
}

function updateHexField(field, value, allowAlpha) {
  const slot = selectedSlot();
  if (!slot) return;
  slot.style[field] = normalizeHex(value, slot.style[field], allowAlpha);
  commitChange({ schedule: false });
  renderInspector();
  schedulePreview();
}

async function onImageUpload(e) {
  const file = e.target.files[0];
  if (!file) return;
  if (file.size > 5 * 1024 * 1024) {
    setStatus("Upload failed: file exceeds 5 MB limit.", "", "error");
    renderStatus();
    return;
  }
  const form = new FormData();
  form.append("image", file);
  setStatus("Uploading image…", "", "normal");
  renderStatus();
  try {
    const res = await fetch("/api/upload", { method: "POST", body: form });
    const data = await res.json();
    if (!res.ok) throw new Error(data.error || "Upload failed.");
    state.metadata.templates.push({
      id: data.templateId, name: data.name, description: "User upload",
      bestFor: "Custom templates.", previewUrl: data.previewUrl, category: "uploads",
    });
    const slot = selectedSlot();
    if (slot) slot.templateId = data.templateId;
    setStatus("Image uploaded.", "", "normal");
    renderAll();
    schedulePreview();
  } catch (err) {
    setStatus(`Upload failed: ${err.message}`, "", "error");
    renderStatus();
  }
  e.target.value = "";
}

function onTemplatePick(e) {
  const btn = e.target.closest("[data-template-id]");
  if (!btn) return;
  const slot = selectedSlot();
  if (!slot) return;
  slot.templateId = btn.dataset.templateId;
  commitChange();
}

async function onDeleteUpload(templateId) {
  try {
    const res = await fetch("/api/upload/delete", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({ templateId }),
    });
    const data = await res.json();
    if (!res.ok) throw new Error(data.error || "Delete failed.");
    state.metadata.templates = state.metadata.templates.filter((t) => t.id !== templateId);
    renderAll();
  } catch (err) {
    setStatus(`Delete failed: ${err.message}`, "", "error");
    renderStatus();
  }
}

function onLayoutPick(e) {
  const btn = e.target.closest("[data-layout-id]");
  if (!btn) return;
  const scene = selectedScene();
  if (!scene) return;
  scene.layout.kind = btn.dataset.layoutId;
  scene.slots = preserveSlots(scene.slots, scene.layout.kind);
  state.selectedSlotIndex = Math.min(state.selectedSlotIndex, scene.slots.length - 1);
  state.selectedTextLayerId = null;
  state.editingTextLayerId = null;
  commitChange();
}

function onPresetPick(e) {
  const btn = e.target.closest("[data-preset-id]");
  if (!btn) return;
  const preset = presetDocuments().find((p) => p.id === btn.dataset.presetId);
  if (preset) applyDocument(preset.build());
}

function onTransitionPipClick(pip, sceneIndex) {
  document.querySelectorAll(".transition-popover").forEach((el) => el.remove());

  const scene = state.scenes[sceneIndex];
  if (!scene) return;
  const trans = scene.transition || { type: "cut", durationMs: 150, easing: "linear" };

  const pop = document.createElement("div");
  pop.className = "transition-popover";
  pop.innerHTML = `
    <label class="field"><span class="field__label">Type</span>
      <select class="tp-type">${TRANSITION_TYPES.map((t) =>
        `<option value="${t.id}"${t.id === trans.type ? " selected" : ""}>${t.label}</option>`).join("")}
      </select></label>
    <label class="field"><span class="field__label">Duration</span>
      <input class="tp-dur" type="number" min="50" max="1000" step="10" value="${trans.durationMs}" /></label>
    <label class="field"><span class="field__label">Easing</span>
      <select class="tp-ease">${EASING_TYPES.map((item) =>
        `<option value="${item.id}"${item.id === trans.easing ? " selected" : ""}>${item.label}</option>`).join("")}
      </select></label>`;

  const rect = pip.getBoundingClientRect();
  pop.style.position = "fixed";
  pop.style.bottom = `${window.innerHeight - rect.top + 8}px`;
  pop.style.left = `${rect.left + rect.width / 2 - 90}px`;
  document.body.appendChild(pop);

  const durField = pop.querySelector(".tp-dur").closest(".field");
  const easeField = pop.querySelector(".tp-ease").closest(".field");
  const toggleFields = () => {
    const isCut = pop.querySelector(".tp-type").value === "cut";
    durField.hidden = isCut;
    easeField.hidden = isCut;
  };
  toggleFields();

  const updateState = () => {
    const type = pop.querySelector(".tp-type").value;
    scene.transition = {
      type,
      durationMs: clamp(pop.querySelector(".tp-dur").value, 50, 1000),
      easing: pop.querySelector(".tp-ease").value,
    };
    const label = pip.querySelector(".transition-pip__label");
    if (label) label.textContent = type === "cut" ? "cut" : type;
    pip.classList.toggle("transition-pip--cut", type === "cut");
    toggleFields();
  };
  pop.addEventListener("change", updateState);
  pop.addEventListener("input", updateState);

  const close = (evt) => {
    if (!pop.contains(evt.target) && !pip.contains(evt.target)) {
      pop.remove();
      document.removeEventListener("pointerdown", close);
      commitChange();
    }
  };
  setTimeout(() => document.addEventListener("pointerdown", close), 0);
}

function onSceneStripAction(e) {
  const btn = e.target.closest("[data-scene-action]");
  if (!btn) return;
  const index = Number(btn.dataset.sceneIndex);
  if (!Number.isInteger(index) || !state.scenes[index]) return;
  const action = btn.dataset.sceneAction;

  if (action === "select") {
    state.selectedSceneIndex = index;
    state.selectedSlotIndex = Math.min(state.selectedSlotIndex, state.scenes[index].slots.length - 1);
  } else if (action === "dup") {
    state.scenes.splice(index + 1, 0, cloneSceneWithFreshTextLayerIds(state.scenes[index]));
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
  state.selectedTextLayerId = null;
  state.editingTextLayerId = null;
  commitChange({ schedule: false });
}

function onSlotTabAction(e) {
  const btn = e.target.closest("[data-slot-index]");
  if (!btn) return;
  state.selectedSlotIndex = Number(btn.dataset.slotIndex);
  state.selectedTextLayerId = null;
  state.editingTextLayerId = null;
  renderAll();
}

function applyDocument(doc, options = {}) {
  const hydrated = hydrateDocument(doc);
  state.canvas = clone(hydrated.canvas);
  state.output = clone(hydrated.output);
  state.scenes = hydrated.scenes.map((scene) => hydrateScene(scene));
  state.selectedSceneIndex = 0;
  state.selectedSlotIndex = 0;
  state.selectedTextLayerId = null;
  state.editingTextLayerId = null;
  state.draggingTextLayerId = null;
  state.lastPreviewSceneIndex = null;
  setPreviewMode("paused", { force: true, message: false });
  ensureOutputCompatibility(false);
  setStatus("Studio ready.", "The stage is paused by default. Start preview when you want a fresh still render.", "normal");
  renderAll();
  if (!options.silent) schedulePreview(80, false, true);
}

function ensureOutputCompatibility(showMessage = true) {
  const pngBtn = document.querySelector('[data-format="png"]');
  if (state.scenes.length > 1 && state.output.format === "png") {
    state.output.format = "gif";
    if (showMessage) {
      setStatus("Output switched to GIF.", "Multi-scene documents export as GIF loops.", "normal");
    }
  }
  if (pngBtn) pngBtn.disabled = state.scenes.length > 1;
}

function commitChange(options = {}) {
  ensureOutputCompatibility(options.showMessage !== false);
  renderAll();
  if (options.schedule !== false) schedulePreview();
}

function populateSelectOptions() {
  $("sceneEffectSelect").innerHTML = state.metadata.effects
    .map((effect) => `<option value="${escAttr(effect.id)}">${esc(effect.name)} — ${esc(effect.description)}</option>`).join("");
  $("slotStylePreset").innerHTML = ['<option value="">Custom / Default</option>',
    ...state.metadata.stylePresets.map((preset) => `<option value="${escAttr(preset.id)}">${esc(preset.name)}</option>`)].join("");
  $("slotFontSizeMode").innerHTML = FONT_SIZE_OPTIONS
    .map((option) => `<option value="${escAttr(option.id)}">${esc(option.label)}</option>`).join("");
}

// ── UI chrome ──

function bindSidebarTabs() {
  const left = $("sidebarLeft");
  if (left) {
    left.querySelector(".sidebar__head").addEventListener("click", (e) => {
      const tab = e.target.closest(".sidebar-tab");
      if (!tab) return;
      left.querySelectorAll(".sidebar-tab").forEach((item) => item.classList.remove("is-active"));
      tab.classList.add("is-active");
      left.querySelectorAll(".sidebar__pane").forEach((pane) => { pane.hidden = pane.dataset.pane !== tab.dataset.tab; });
      if (tab.dataset.tab === "imgflip") loadImgflipMemes();
    });
    $("imgflipGrid").addEventListener("click", onImgflipPick);
    $("imgflipSearch").addEventListener("input", (e) => renderImgflipGrid(e.target.value));
  }

  const right = $("sidebarRight");
  if (right) {
    right.querySelector(".sidebar__head").addEventListener("click", (e) => {
      const tab = e.target.closest(".sidebar-tab");
      if (!tab || !tab.dataset.inspector) return;
      right.querySelectorAll(".sidebar-tab").forEach((item) => item.classList.remove("is-active"));
      tab.classList.add("is-active");
      right.querySelectorAll("[data-inspector-pane]").forEach((pane) => { pane.hidden = pane.dataset.inspectorPane !== tab.dataset.inspector; });
    });
  }
}

function bindScriptEditor() {
  $("scriptPreview").addEventListener("input", () => {
    state.script = $("scriptPreview").value;
    $("scriptHighlight").innerHTML = highlightMac(state.script);
  });
  $("scriptApplyBtn").addEventListener("click", async () => {
    const script = $("scriptPreview").value;
    if (!script.trim()) return;
    $("scriptApplyBtn").disabled = true;
    $("scriptApplyBtn").textContent = "Applying…";
    setStatus("Applying script…", "Executing raw Mac code on the server.", "normal");
    renderStatus();
    try {
      const res = await fetch("/api/generate", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ rawScript: script }),
      });
      const data = await res.json();
      if (!res.ok) throw new Error(data.error || "Apply failed.");
      state.lastExport = data;
      state.stageAssetUrl = `${data.previewUrl}?v=${Date.now()}`;
      state.stageLabel = "Applied from script";
      setStatus("Script applied.", `${Math.max(1, Math.round((data.summary?.fileSizeBytes || 0) / 1024))} KB`, "normal");
    } catch (err) {
      setStatus(`Apply failed: ${err.message}`, "", "error");
    } finally {
      $("scriptApplyBtn").disabled = false;
      $("scriptApplyBtn").textContent = "Apply Script";
      renderStage();
      renderStatus();
    }
  });
  $("scriptFullscreenBtn").addEventListener("click", () => {
    const dock = document.querySelector(".dock__script");
    dock.classList.toggle("is-fullscreen");
    $("scriptFullscreenBtn").textContent = dock.classList.contains("is-fullscreen") ? "Exit Fullscreen" : "Expand";
  });
  document.addEventListener("keydown", (e) => {
    if (e.key !== "Escape") return;
    const dock = document.querySelector(".dock__script");
    if (dock.classList.contains("is-fullscreen")) {
      dock.classList.remove("is-fullscreen");
      $("scriptFullscreenBtn").textContent = "Expand";
    }
  });
}

function bindMobileToggles() {
  function close() {
    $("sidebarLeft").classList.remove("is-open");
    document.querySelector(".sidebar--right").classList.remove("is-open");
    $("mobileBackdrop").classList.remove("is-open");
  }
  $("mobileAssetsBtn").addEventListener("click", () => {
    const open = $("sidebarLeft").classList.toggle("is-open");
    document.querySelector(".sidebar--right").classList.remove("is-open");
    $("mobileBackdrop").classList.toggle("is-open", open);
  });
  $("mobileInspectorBtn").addEventListener("click", () => {
    const open = document.querySelector(".sidebar--right").classList.toggle("is-open");
    $("sidebarLeft").classList.remove("is-open");
    $("mobileBackdrop").classList.toggle("is-open", open);
  });
  $("mobileBackdrop").addEventListener("click", close);
}
