// Mac GIF Studio — v2

const state = {
  templates: [],
  effects: [],
  selectedTemplate: "two_panel",
  selectedEffect: "none",
  width: 640,
  height: 640,
  textColor: "#FFFFFF",
  outlineWidth: 3,
  frames: [
    { top: "MONDAY", bottom: "CODING", duration: 400 },
    { top: "WEDNESDAY", bottom: "DEBUGGING", duration: 400 },
    { top: "FRIDAY", bottom: "DEPLOYING", duration: 400 },
  ],
  isRendering: false,
};

const presets = [
  {
    label: "Week Arc", template: "two_panel", width: 640, height: 640,
    frames: [
      { top: "MONDAY", bottom: "SHIPPING FEATURES", duration: 350 },
      { top: "WEDNESDAY", bottom: "FIXING THE EDGE CASE", duration: 350 },
      { top: "FRIDAY", bottom: "DEPLOYING ANYWAY", duration: 500 },
    ],
  },
  {
    label: "Countdown", template: "blank", width: 480, height: 480,
    frames: [
      { top: "", bottom: "3", duration: 240 },
      { top: "", bottom: "2", duration: 240 },
      { top: "", bottom: "1", duration: 240 },
      { top: "", bottom: "GO", duration: 360 },
    ],
  },
  {
    label: "Escalation", template: "three_panel", width: 720, height: 720,
    frames: [
      { top: "JUST ONE TINY CHANGE", bottom: "FRAME ONE", duration: 320 },
      { top: "STILL SAFE", bottom: "FRAME TWO", duration: 320 },
      { top: "PRODUCTION:", bottom: "FRAME THREE", duration: 480 },
    ],
  },
];

const $ = (id) => document.getElementById(id);

document.addEventListener("DOMContentLoaded", () => {
  loadTemplates();
  bindControls();
  renderPresets();
  render();
});

async function loadTemplates() {
  try {
    const res = await fetch("/api/templates");
    const data = await res.json();
    if (data.templates) state.templates = data.templates;
    if (data.effects) state.effects = data.effects;
    renderTemplates();
    renderEffects();
  } catch {
    console.warn("Using defaults");
  }
}

function bindControls() {
  syncRange($("widthRange"), $("widthNum"), "width");
  syncRange($("heightRange"), $("heightNum"), "height");

  $("outlineRange").addEventListener("input", (e) => {
    state.outlineWidth = +e.target.value;
    $("outlineVal").textContent = state.outlineWidth + "px";
    renderScript();
  });

  $("textColor").addEventListener("input", (e) => {
    state.textColor = e.target.value;
    $("textColorHex").value = e.target.value.toUpperCase();
    renderScript();
  });

  $("textColorHex").addEventListener("change", (e) => {
    const v = e.target.value.trim();
    if (/^#[0-9a-fA-F]{6}$/.test(v)) {
      state.textColor = v;
      $("textColor").value = v;
      renderScript();
    }
  });

  $("effectSelect").addEventListener("change", (e) => {
    state.selectedEffect = e.target.value;
    renderScript();
    updateStats();
  });

  $("addFrameBtn").addEventListener("click", () => {
    const last = state.frames[state.frames.length - 1] || {};
    state.frames.push({ top: last.top || "", bottom: last.bottom || "", duration: last.duration || 400 });
    render();
  });

  $("frameList").addEventListener("input", onFrameInput);
  $("frameList").addEventListener("click", onFrameAction);
  $("renderBtn").addEventListener("click", renderGif);
}

function syncRange(range, num, key) {
  const update = (v) => {
    const val = Math.max(240, Math.min(1200, Math.round(+v)));
    state[key] = val;
    range.value = val;
    num.value = val;
    render();
  };
  range.addEventListener("input", (e) => update(e.target.value));
  num.addEventListener("change", (e) => update(e.target.value));
}

function renderTemplates() {
  const grid = $("templateGrid");
  grid.innerHTML = "";
  state.templates.forEach((t) => {
    const card = document.createElement("button");
    card.type = "button";
    card.className = `tmpl-card${t.id === state.selectedTemplate ? " is-active" : ""}`;
    card.title = t.description;
    card.innerHTML = `
      <div class="tmpl-card__thumb" style="background-image:url('${CSS.escape(t.previewUrl)}')"></div>
      <div class="tmpl-card__name">${esc(t.name)}</div>`;
    card.addEventListener("click", () => { state.selectedTemplate = t.id; render(); });
    grid.appendChild(card);
  });
}

function renderEffects() {
  const sel = $("effectSelect");
  sel.innerHTML = "";
  state.effects.forEach((e) => {
    const opt = document.createElement("option");
    opt.value = e.id;
    opt.textContent = `${e.name} — ${e.description}`;
    sel.appendChild(opt);
  });
}

function renderPresets() {
  const strip = $("presetStrip");
  strip.innerHTML = "";
  presets.forEach((p) => {
    const btn = document.createElement("button");
    btn.type = "button";
    btn.className = "btn btn--sm";
    btn.textContent = p.label;
    btn.addEventListener("click", () => {
      state.selectedTemplate = p.template;
      state.width = p.width;
      state.height = p.height;
      state.frames = p.frames.map((f) => ({ ...f }));
      render();
    });
    strip.appendChild(btn);
  });
}

function renderFrames() {
  const list = $("frameList");
  list.innerHTML = "";
  state.frames.forEach((f, i) => {
    const card = document.createElement("div");
    card.className = "frame-card";
    card.innerHTML = `
      <div class="frame-card__head">
        <span class="frame-card__num">FRAME ${i + 1}</span>
        <div class="frame-card__actions">
          <button class="btn btn--sm" data-action="up" data-i="${i}">&uarr;</button>
          <button class="btn btn--sm" data-action="down" data-i="${i}">&darr;</button>
          <button class="btn btn--sm" data-action="dup" data-i="${i}">Dup</button>
          <button class="btn btn--sm" data-action="del" data-i="${i}">&times;</button>
        </div>
      </div>
      <div class="frame-card__fields">
        <div class="field">
          <label>Top</label>
          <textarea data-i="${i}" data-f="top" placeholder="Top caption">${esc(f.top)}</textarea>
        </div>
        <div class="field">
          <label>Bottom</label>
          <textarea data-i="${i}" data-f="bottom" placeholder="Bottom caption">${esc(f.bottom)}</textarea>
        </div>
        <div class="range-row">
          <label>${f.duration}ms</label>
          <input type="range" data-i="${i}" data-f="duration" min="80" max="2500" step="10" value="${f.duration}" />
        </div>
      </div>`;
    list.appendChild(card);
  });
}

function onFrameInput(e) {
  const t = e.target;
  const i = +t.dataset.i;
  if (!state.frames[i]) return;
  if (t.dataset.f === "top" || t.dataset.f === "bottom") {
    state.frames[i][t.dataset.f] = t.value;
  } else if (t.dataset.f === "duration") {
    state.frames[i].duration = +t.value;
    const label = t.closest(".range-row")?.querySelector("label");
    if (label) label.textContent = t.value + "ms";
  }
  renderScript();
  updateStats();
}

function onFrameAction(e) {
  const btn = e.target.closest("[data-action]");
  if (!btn) return;
  const i = +btn.dataset.i;
  const a = btn.dataset.action;
  if (a === "del" && state.frames.length > 1) state.frames.splice(i, 1);
  else if (a === "dup") state.frames.splice(i + 1, 0, { ...state.frames[i] });
  else if (a === "up" && i > 0) [state.frames[i - 1], state.frames[i]] = [state.frames[i], state.frames[i - 1]];
  else if (a === "down" && i < state.frames.length - 1) [state.frames[i], state.frames[i + 1]] = [state.frames[i + 1], state.frames[i]];
  render();
}

function render() {
  renderTemplates();
  renderFrames();
  updateStats();
  renderScript();
}

function updateStats() {
  $("statCanvas").innerHTML = `${state.width}&times;${state.height}`;
  $("sizeBadge").innerHTML = `${state.width}&times;${state.height}`;
  $("statFrames").textContent = state.frames.length;
  const totalMs = state.frames.reduce((s, f) => s + f.duration, 0);
  $("statDuration").textContent = totalMs >= 1000 ? (totalMs / 1000).toFixed(1) + "s" : totalMs + "ms";
  $("widthRange").value = state.width;
  $("widthNum").value = state.width;
  $("heightRange").value = state.height;
  $("heightNum").value = state.height;
}

function renderScript(serverScript) {
  $("scriptPreview").textContent = serverScript || buildScript();
}

function buildScript() {
  const lines = [];
  const hasEffect = state.selectedEffect && state.selectedEffect !== "none";
  const effectMap = {
    sepia: "sepia", invert: "invert", sharpen: "sharpen", vignette: "vignette",
    grayscale: "grayscale",
    glitch: "pixelate(4) >> contrast(1.8) >> noise(0.2)",
    vintage: "sepia >> brightness(0.9)",
    deepfry: "saturate(3.0) >> contrast(2.0) >> jpeg(10) >> noise(0.1)",
    cyberpunk: "hueShift(180) >> contrast(1.5) >> chromatic(3) >> glow(4)",
    comic: "posterize(5) >> contrast(1.4) >> sharpen",
    retro: "grayscale >> contrast(1.3) >> noise(0.1)",
  };

  if (hasEffect) lines.push(`effect fx = ${effectMap[state.selectedEffect] || state.selectedEffect};`, "");

  const hasStyle = state.textColor !== "#FFFFFF" && state.textColor !== "#ffffff";
  const hasOutline = state.outlineWidth !== 3;
  if (hasStyle || hasOutline) {
    lines.push("style custom {");
    if (hasStyle) lines.push(`    color: "${state.textColor.toUpperCase()}"`);
    lines.push(`    outline: ${state.outlineWidth}`);
    lines.push("}", "");
  }

  lines.push("gif loop {");
  const stylePart = (hasStyle || hasOutline) ? " custom" : "";
  const fxPart = hasEffect ? " |> fx" : "";
  state.frames.forEach((f) => {
    const top = norm(f.top);
    const bot = norm(f.bottom);
    lines.push(`    @${state.selectedTemplate} ${state.width}x${state.height}${stylePart} { top: "${top}" bottom: "${bot}" }${fxPart} : ${f.duration}ms`);
  });
  lines.push('} => "output.gif"');
  return lines.join("\n");
}

async function renderGif() {
  if (state.isRendering) return;
  state.isRendering = true;
  $("renderBtn").disabled = true;
  $("renderBtn").textContent = "Rendering...";
  $("statusLine").textContent = "Rendering GIF...";
  $("statusLine").className = "status";

  try {
    const res = await fetch("/api/generate", {
      method: "POST",
      headers: { "Content-Type": "application/json" },
      body: JSON.stringify({
        template: state.selectedTemplate,
        width: state.width,
        height: state.height,
        frames: state.frames,
        effect: state.selectedEffect,
        textColor: state.textColor,
        outlineWidth: state.outlineWidth,
      }),
    });
    const data = await res.json();
    if (!res.ok) throw new Error(data.error || "Render failed");

    $("gifPreview").src = `${data.previewUrl}?v=${Date.now()}`;
    $("gifPreview").hidden = false;
    $("placeholder").hidden = true;
    $("downloadLink").href = data.downloadUrl;
    $("downloadLink").hidden = false;

    const kb = Math.max(1, Math.round((data.summary?.fileSizeBytes || 0) / 1024));
    $("statusLine").textContent = `${data.summary?.frameCount || "?"} frames, ${kb} KB`;
    if (data.script) renderScript(data.script);
  } catch (err) {
    $("statusLine").textContent = err.message;
    $("statusLine").className = "status status--error";
  } finally {
    state.isRendering = false;
    $("renderBtn").disabled = false;
    $("renderBtn").textContent = "Render GIF";
  }
}

function norm(s) {
  return String(s || "").replace(/"/g, "'").replace(/\s+/g, " ").trim().slice(0, 120);
}

function esc(s) {
  return String(s).replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;");
}
