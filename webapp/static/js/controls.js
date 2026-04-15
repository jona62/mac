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
    const s = selectedScene(); if (!s) return;
    s.durationMs = clamp(v, state.metadata.limits.duration.min, state.metadata.limits.duration.max);
    commitChange();
  });
  bindRangePair("scenePaddingRange", "scenePaddingInput", (v) => {
    const s = selectedScene(); if (!s) return;
    s.layout.padding = clamp(v, state.metadata.limits.padding.min, state.metadata.limits.padding.max);
    commitChange();
  });
  bindRangePair("sceneBorderRange", "sceneBorderInput", (v) => {
    const s = selectedScene(); if (!s) return;
    s.layout.border = clamp(v, state.metadata.limits.border.min, state.metadata.limits.border.max);
    commitChange();
  });
  bindRangePair("slotOutlineRange", "slotOutlineInput", (v) => {
    const s = selectedSlot(); if (!s) return;
    s.style.outline = clamp(v, state.metadata.limits.outline.min, state.metadata.limits.outline.max);
    commitChange();
  });
  bindRangePair("slotShadowRange", "slotShadowInput", (v) => {
    const s = selectedSlot(); if (!s) return;
    s.style.shadow = clamp(v, state.metadata.limits.shadow.min, state.metadata.limits.shadow.max);
    commitChange();
  });

  $("sceneEffectSelect").addEventListener("change", (e) => {
    const s = selectedScene(); if (!s) return;
    s.layout.effect = e.target.value;
    commitChange();
  });

  $("slotTopText").addEventListener("input", (e) => updateSlotText("top", e.target.value));
  $("slotCenterText").addEventListener("input", (e) => updateSlotText("center", e.target.value));
  $("slotBottomText").addEventListener("input", (e) => updateSlotText("bottom", e.target.value));

  $("slotStylePreset").addEventListener("change", (e) => {
    const s = selectedSlot(); if (!s) return;
    s.style = createStyle(e.target.value || "");
    commitChange();
  });

  $("slotTextColor").addEventListener("input", (e) => updateHexField("color", e.target.value, false));
  $("slotTextColorHex").addEventListener("change", (e) => updateHexField("color", e.target.value, false));
  $("slotOutlineColor").addEventListener("input", (e) => updateHexField("outlineColor", e.target.value, false));
  $("slotOutlineColorHex").addEventListener("change", (e) => updateHexField("outlineColor", e.target.value, false));
  $("slotShadowColorHex").addEventListener("change", (e) => updateHexField("shadowColor", e.target.value, true));
  $("slotBgColor").addEventListener("input", (e) => updateHexField("background", e.target.value, false));
  $("slotBgColorHex").addEventListener("change", (e) => {
    const s = selectedSlot(); if (!s) return;
    const val = e.target.value.trim();
    if (val === "" || val.toLowerCase() === "none") { s.style.background = ""; }
    else { s.style.background = normalizeHex(val, s.style.background || "", true); }
    commitChange({ schedule: false }); renderInspector(); schedulePreview();
  });

  $("slotFontSizeMode").addEventListener("change", (e) => {
    const s = selectedSlot(); if (!s) return;
    s.style.fontSizeMode = e.target.value;
    if (s.style.fontSizeMode !== "custom") s.style.fontSizePx = clamp(s.style.fontSizePx, 8, 240);
    commitChange();
  });
  $("slotFontSizePx").addEventListener("change", (e) => {
    const s = selectedSlot(); if (!s) return;
    s.style.fontSizePx = clamp(e.target.value, state.metadata.limits.fontSizePx.min, state.metadata.limits.fontSizePx.max);
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
    const i = state.selectedSceneIndex + 1;
    state.scenes.splice(i, 0, createScene("single"));
    state.selectedSceneIndex = i;
    state.selectedSlotIndex = 0;
    commitChange();
  });

  $("addEffectBtn").addEventListener("click", () => {
    const scene = selectedScene(); if (!scene) return;
    const id = $("addEffectSelect").value; if (!id) return;
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
    const scene = selectedScene(); if (!scene) return;
    scene.layout.customEffects.splice(Number(del.dataset.fxDel), 1);
    commitChange();
  });
  $("effectChainBuilder").addEventListener("input", (e) => {
    const input = e.target.closest("[data-fx-index]");
    if (!input) return;
    const scene = selectedScene(); if (!scene) return;
    const i = Number(input.dataset.fxIndex);
    if (scene.layout.customEffects[i]) {
      scene.layout.customEffects[i].param = Number(input.value);
      commitChange();
    }
  });

  // Positioned text controls
  $("addPosTextBtn").addEventListener("click", () => {
    const slot = selectedSlot(); if (!slot) return;
    if (!slot.positionedTexts) slot.positionedTexts = [];
    slot.positionedTexts.push({ content: "Text", x: state.canvas.width / 2, y: state.canvas.height / 2 });
    commitChange();
  });
  $("posTextList").addEventListener("input", (e) => {
    const input = e.target.closest("[data-pos-i]"); if (!input) return;
    const slot = selectedSlot(); if (!slot) return;
    const i = Number(input.dataset.posI);
    const pt = (slot.positionedTexts || [])[i]; if (!pt) return;
    const field = input.dataset.posField;
    if (field === "content") pt.content = input.value;
    else if (field === "x") pt.x = Number(input.value);
    else if (field === "y") pt.y = Number(input.value);
    renderPosOverlay();
    schedulePreview();
  });
  $("posTextList").addEventListener("click", (e) => {
    const del = e.target.closest("[data-pos-del]"); if (!del) return;
    const slot = selectedSlot(); if (!slot) return;
    slot.positionedTexts.splice(Number(del.dataset.posDel), 1);
    commitChange();
  });

  // Drag positioned text labels on canvas overlay
  $("posOverlay").addEventListener("pointerdown", (e) => {
    const label = e.target.closest(".pos-label"); if (!label) return;
    const i = Number(label.dataset.posIndex);
    const slot = selectedSlot(); if (!slot) return;
    const pt = (slot.positionedTexts || [])[i]; if (!pt) return;

    label.classList.add("is-dragging");
    label.setPointerCapture(e.pointerId);

    const img = $("stageImage");
    const imgRect = img.getBoundingClientRect();
    const scaleX = state.canvas.width / imgRect.width;
    const scaleY = state.canvas.height / imgRect.height;

    const onMove = (ev) => {
      pt.x = clamp((ev.clientX - imgRect.left) * scaleX, 0, state.canvas.width);
      pt.y = clamp((ev.clientY - imgRect.top) * scaleY, 0, state.canvas.height);
      renderPosOverlay();
      renderPosTextList();
    };
    const onUp = () => {
      label.classList.remove("is-dragging");
      label.removeEventListener("pointermove", onMove);
      label.removeEventListener("pointerup", onUp);
      schedulePreview();
    };
    label.addEventListener("pointermove", onMove);
    label.addEventListener("pointerup", onUp);
  });

  $("refreshPreviewBtn").addEventListener("click", () => schedulePreview(20, true));
  $("exportBtn").addEventListener("click", exportDocument);

  $("canvasTemplateGrid").addEventListener("click", onTemplatePick);
  $("memeTemplateGrid").addEventListener("click", onTemplatePick);
  $("uploadTemplateGrid").addEventListener("click", (e) => {
    const delBtn = e.target.closest("[data-delete-upload]");
    if (delBtn) { onDeleteUpload(delBtn.dataset.deleteUpload); return; }
    onTemplatePick(e);
  });
  $("imageUpload").addEventListener("change", onImageUpload);
  $("layoutGrid").addEventListener("click", onLayoutPick);
  $("presetGrid").addEventListener("click", onPresetPick);
  $("sceneStrip").addEventListener("click", (e) => {
    const loopPip = e.target.closest("[data-transition-loop]");
    if (loopPip) { onTransitionPipClick(loopPip, state.scenes.length - 1); return; }
    const transPip = e.target.closest("[data-transition-index]");
    if (transPip) { onTransitionPipClick(transPip, Number(transPip.dataset.transitionIndex)); return; }
    onSceneStripAction(e);
  });
  $("slotTabs").addEventListener("click", onSlotTabAction);

  bindSidebarTabs();
  bindScriptEditor();
  bindMobileToggles();
}

function bindRangePair(rangeId, inputId, onChange) {
  $(rangeId).addEventListener("input", (e) => onChange(Number(e.target.value)));
  $(inputId).addEventListener("change", (e) => onChange(Number(e.target.value)));
}

function updateSlotText(field, value) {
  const s = selectedSlot(); if (!s) return;
  s.text[field] = value;
  commitChange();
}

function updateHexField(field, value, allowAlpha) {
  const s = selectedSlot(); if (!s) return;
  s.style[field] = normalizeHex(value, s.style[field], allowAlpha);
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
    // Add to metadata so it appears in the grid
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
  const s = selectedSlot(); if (!s) return;
  s.templateId = btn.dataset.templateId;
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
  const btn = e.target.closest("[data-layout-id]"); if (!btn) return;
  const scene = selectedScene(); if (!scene) return;
  scene.layout.kind = btn.dataset.layoutId;
  scene.slots = preserveSlots(scene.slots, scene.layout.kind);
  state.selectedSlotIndex = Math.min(state.selectedSlotIndex, scene.slots.length - 1);
  commitChange();
}

function onPresetPick(e) {
  const btn = e.target.closest("[data-preset-id]"); if (!btn) return;
  const preset = presetDocuments().find((p) => p.id === btn.dataset.presetId);
  if (preset) applyDocument(preset.build());
}

function onTransitionPipClick(pip, sceneIndex) {
  // Remove any existing popover
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
      <select class="tp-ease">${EASING_TYPES.map((e) =>
        `<option value="${e.id}"${e.id === trans.easing ? " selected" : ""}>${e.label}</option>`).join("")}
      </select></label>`;

  // Position popover above the pip using fixed positioning
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

  // Close on outside click — then do the full re-render + preview
  const close = (e) => {
    if (!pop.contains(e.target) && !pip.contains(e.target)) {
      pop.remove();
      document.removeEventListener("pointerdown", close);
      commitChange();
    }
  };
  setTimeout(() => document.addEventListener("pointerdown", close), 0);
}

function onSceneStripAction(e) {
  const btn = e.target.closest("[data-scene-action]"); if (!btn) return;
  const i = Number(btn.dataset.sceneIndex);
  if (!Number.isInteger(i) || !state.scenes[i]) return;
  const action = btn.dataset.sceneAction;
  if (action === "select") {
    state.selectedSceneIndex = i;
    state.selectedSlotIndex = Math.min(state.selectedSlotIndex, state.scenes[i].slots.length - 1);
  } else if (action === "dup") {
    state.scenes.splice(i + 1, 0, clone(state.scenes[i]));
    state.selectedSceneIndex = i + 1;
  } else if (action === "del") {
    if (state.scenes.length === 1) return;
    state.scenes.splice(i, 1);
    state.selectedSceneIndex = Math.min(state.selectedSceneIndex, state.scenes.length - 1);
  } else if (action === "up" && i > 0) {
    [state.scenes[i - 1], state.scenes[i]] = [state.scenes[i], state.scenes[i - 1]];
    state.selectedSceneIndex = i - 1;
  } else if (action === "down" && i < state.scenes.length - 1) {
    [state.scenes[i], state.scenes[i + 1]] = [state.scenes[i + 1], state.scenes[i]];
    state.selectedSceneIndex = i + 1;
  }
  state.selectedSlotIndex = Math.min(state.selectedSlotIndex, selectedScene().slots.length - 1);
  commitChange();
}

function onSlotTabAction(e) {
  const btn = e.target.closest("[data-slot-index]"); if (!btn) return;
  state.selectedSlotIndex = Number(btn.dataset.slotIndex);
  renderAll();
}

function applyDocument(doc, options = {}) {
  state.canvas = clone(doc.canvas);
  state.output = clone(doc.output);
  state.scenes = doc.scenes.map((s) => clone(s));
  state.selectedSceneIndex = 0;
  state.selectedSlotIndex = 0;
  ensureOutputCompatibility(false);
  setStatus("Studio ready.", "Scene preview is debounced and Mac-powered.");
  renderAll();
  if (!options.silent) schedulePreview(80);
}

function ensureOutputCompatibility(showMessage = true) {
  const pngBtn = document.querySelector('[data-format="png"]');
  if (state.scenes.length > 1 && state.output.format === "png") {
    state.output.format = "gif";
    if (showMessage) setStatus("Output switched to GIF.", "Multi-scene documents export as GIF loops.", "normal");
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
    .map((e) => `<option value="${escAttr(e.id)}">${esc(e.name)} — ${esc(e.description)}</option>`).join("");
  $("slotStylePreset").innerHTML = ['<option value="">Custom / Default</option>',
    ...state.metadata.stylePresets.map((p) => `<option value="${escAttr(p.id)}">${esc(p.name)}</option>`)].join("");
  $("slotFontSizeMode").innerHTML = FONT_SIZE_OPTIONS
    .map((o) => `<option value="${escAttr(o.id)}">${esc(o.label)}</option>`).join("");
}

// ── UI chrome ──

function bindSidebarTabs() {
  // Left sidebar: Assets / Imgflip / Presets
  const left = $("sidebarLeft");
  if (left) {
    left.querySelector(".sidebar__head").addEventListener("click", (e) => {
      const tab = e.target.closest(".sidebar-tab"); if (!tab) return;
      left.querySelectorAll(".sidebar-tab").forEach((t) => t.classList.remove("is-active"));
      tab.classList.add("is-active");
      left.querySelectorAll(".sidebar__pane").forEach((p) => { p.hidden = p.dataset.pane !== tab.dataset.tab; });
      // Lazy-load Imgflip memes on first tab click
      if (tab.dataset.tab === "imgflip") loadImgflipMemes();
    });
    // Imgflip grid click → import and use
    $("imgflipGrid").addEventListener("click", onImgflipPick);
    // Search filter
    $("imgflipSearch").addEventListener("input", (e) => renderImgflipGrid(e.target.value));
  }
  // Right sidebar: Slot / Scene / Document
  const right = $("sidebarRight");
  if (right) {
    right.querySelector(".sidebar__head").addEventListener("click", (e) => {
      const tab = e.target.closest(".sidebar-tab"); if (!tab || !tab.dataset.inspector) return;
      right.querySelectorAll(".sidebar-tab").forEach((t) => t.classList.remove("is-active"));
      tab.classList.add("is-active");
      right.querySelectorAll("[data-inspector-pane]").forEach((p) => { p.hidden = p.dataset.inspectorPane !== tab.dataset.inspector; });
    });
  }
}

function bindScriptEditor() {
  $("scriptPreview").addEventListener("input", () => {
    state.script = $("scriptPreview").value;
    $("scriptHighlight").innerHTML = highlightMac(state.script);
  });
  $("scriptApplyBtn").addEventListener("click", async () => {
    const script = $("scriptPreview").value; if (!script.trim()) return;
    $("scriptApplyBtn").disabled = true;
    $("scriptApplyBtn").textContent = "Applying…";
    setStatus("Applying script…", "Executing raw Mac code on the server.", "normal");
    renderStatus();
    try {
      const res = await fetch("/api/generate", {
        method: "POST", headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ rawScript: script }),
      });
      const data = await res.json();
      if (!res.ok) throw new Error(data.error || "Apply failed.");
      state.lastExport = data;
      state.stageAssetUrl = `${data.previewUrl}?v=${Date.now()}`;
      state.stageLabel = "Applied from script";
      setStatus("Script applied.", `${Math.max(1, Math.round((data.summary?.fileSizeBytes || 0) / 1024))} KB`, "normal");
    } catch (err) { setStatus(`Apply failed: ${err.message}`, "", "error"); }
    finally { $("scriptApplyBtn").disabled = false; $("scriptApplyBtn").textContent = "Apply Script"; renderStage(); renderStatus(); }
  });
  $("scriptFullscreenBtn").addEventListener("click", () => {
    const d = document.querySelector(".dock__script");
    d.classList.toggle("is-fullscreen");
    $("scriptFullscreenBtn").textContent = d.classList.contains("is-fullscreen") ? "Exit Fullscreen" : "Expand";
  });
  document.addEventListener("keydown", (e) => {
    if (e.key === "Escape") {
      const d = document.querySelector(".dock__script");
      if (d.classList.contains("is-fullscreen")) { d.classList.remove("is-fullscreen"); $("scriptFullscreenBtn").textContent = "Expand"; }
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
