// Mac Studio — Network: preview, export, metadata loading

function sessionHeaders(extra = {}) {
  return { "X-Session-Id": SESSION_ID, ...extra };
}

async function safeJson(res) {
  try { return await res.json(); }
  catch (_) { throw new Error(`Server error (${res.status})`); }
}

async function loadMetadata() {
  try {
    const res = await fetch("/api/templates", { headers: sessionHeaders() });
    const data = await safeJson(res);
    if (!res.ok) throw new Error(data.error || "Could not load studio metadata.");
    state.metadata.templates = data.templates || [];
    state.metadata.effects = data.effects || [];
    state.metadata.effectDefinitions = data.effectDefinitions || [];
    state.metadata.layouts = Array.isArray(data.layouts) && data.layouts.length
      ? data.layouts : clone(FALLBACK_LAYOUTS);
    state.metadata.stylePresets = Array.isArray(data.stylePresets) && data.stylePresets.length
      ? data.stylePresets : clone(FALLBACK_STYLE_PRESETS);
    state.metadata.limits = { ...DEFAULT_LIMITS, ...(data.limits || {}) };

    const looksLegacy = !Array.isArray(data.layouts) || !data.layouts.length || !Array.isArray(data.stylePresets);
    state.backendCompatibility = looksLegacy ? "legacy" : "studio";
    if (looksLegacy) applyLegacyBackendWarning();
  } catch (error) {
    setStatus(error.message, "Metadata did not load, so the studio cannot initialize.", "error");
    renderStatus();
    throw error;
  }
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

function schedulePreview(delay = 320, immediateMessage = false) {
  clearTimeout(state.previewTimer);
  // Only auto-render if preview is currently showing (user hasn't stopped it)
  if (!immediateMessage && !state.stageAssetUrl) return;
  if (immediateMessage) {
    const label = state.scenes.length > 1 ? "Rendering GIF…" : "Rendering preview…";
    setStatus(label, "", "normal");
    renderStatus();
  }
  state.previewTimer = window.setTimeout(() => requestPreview(), delay);
}

async function requestPreview() {
  if (state.backendCompatibility !== "studio") {
    applyLegacyBackendWarning();
    renderAll();
    return;
  }

  if (state.previewAbortController) state.previewAbortController.abort();
  const controller = new AbortController();
  state.previewAbortController = controller;
  // Auto-timeout after 30s
  const timeout = setTimeout(() => controller.abort(), 30000);

  // Single scene: render still. Multi-scene: render full animated GIF with transitions.
  const isMulti = state.scenes.length > 1;
  const payload = buildPayload(isMulti ? {} : { previewSceneIndex: state.selectedSceneIndex });
  const requestId = ++state.previewSeq;
  state.isPreviewing = true;
  setStatus("Rendering selected scene…", "The stage preview is always a still PNG, even for GIF documents.", "normal");
  renderStatus();

  try {
    const res = await fetch("/api/generate", {
      method: "POST",
      headers: sessionHeaders({ "Content-Type": "application/json" }),
      body: JSON.stringify(payload),
      signal: controller.signal,
    });
    const data = await safeJson(res);
    if (requestId !== state.previewSeq) return;
    if (!res.ok) throw new Error(data.error || "Preview render failed.");

    state.script = data.script || state.script;
    state.stageMode = "preview";
    state.stageAssetUrl = `${data.previewUrl}?v=${Date.now()}`;
    state.stageLabel = isMulti
      ? `${state.scenes.length}-scene GIF`
      : `Scene ${state.selectedSceneIndex + 1}`;
    const summary = data.summary || {};
    setStatus(
      "Preview ready.",
      `${layoutName(selectedScene().layout.kind) || "Scene"} · ${formatBytes(summaryValue(summary, "fileSizeBytes", 0))}`,
      "normal"
    );
  } catch (error) {
    if (requestId !== state.previewSeq) return;
    if (error && error.name === "AbortError") {
      if (state.previewMode === "paused") {
        setStatus("Preview paused.", "The last rendered still stays on stage until you start preview again.", "normal");
      }
      return;
    }
    if (!markLegacyBackendIfNeeded(error.message)) {
      setStatus(error.message, "Preview failed. Fix the document or try starting preview again.", "error");
    }
  } finally {
    clearTimeout(timeout);
    if (requestId === state.previewSeq) {
      state.isPreviewing = false;
      if (state.previewAbortController === controller) state.previewAbortController = null;
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
      headers: sessionHeaders({ "Content-Type": "application/json" }),
      body: JSON.stringify(buildPayload()),
    });
    const data = await safeJson(res);
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

    if (data.downloadUrl) {
      const a = document.createElement("a");
      a.href = data.downloadUrl;
      a.download = `mac-studio-export.${state.output.format || "png"}`;
      document.body.appendChild(a);
      a.click();
      a.remove();
    }
  } catch (error) {
    if (!markLegacyBackendIfNeeded(error.message)) {
      setStatus(error.message, "The document export did not complete.", "error");
    }
  } finally {
    state.isExporting = false;
    renderAll();
  }
}
