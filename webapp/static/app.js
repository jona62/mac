const fallbackTemplates = [
  {
    id: "two_panel",
    name: "Two Panel",
    description: "Classic split-screen rhythm for before-and-after, expectation-vs-reality, or Monday-to-Friday arcs.",
    bestFor: "Pairing two ideas with a fast payoff.",
    previewUrl: "/assets/templates/two_panel.png",
  },
  {
    id: "three_panel",
    name: "Three Panel",
    description: "Three beats with escalating energy when you want the joke to land in stages.",
    bestFor: "Setups that need a beginning, middle, and spike.",
    previewUrl: "/assets/templates/three_panel.png",
  },
  {
    id: "bottom_text",
    name: "Bottom Text",
    description: "Poster-style composition with a giant image and a heavy caption block.",
    bestFor: "One-liners, announcements, and dramatic reveals.",
    previewUrl: "/assets/templates/bottom_text.png",
  },
  {
    id: "blank",
    name: "Blank",
    description: "Minimal empty canvas for countdowns, title cards, or stark graphic loops.",
    bestFor: "Simple text-driven GIFs and punchy hard cuts.",
    previewUrl: "/assets/templates/blank.png",
  },
];

const presets = [
  {
    id: "week",
    label: "Week Arc",
    template: "two_panel",
    width: 640,
    height: 640,
    frames: [
      { top: "MONDAY", bottom: "SHIPPING FEATURES", duration: 350 },
      { top: "WEDNESDAY", bottom: "FIXING THE EDGE CASE", duration: 350 },
      { top: "FRIDAY", bottom: "DEPLOYING ANYWAY", duration: 500 },
    ],
  },
  {
    id: "countdown",
    label: "Countdown",
    template: "blank",
    width: 520,
    height: 520,
    frames: [
      { top: "", bottom: "3", duration: 240 },
      { top: "", bottom: "2", duration: 240 },
      { top: "", bottom: "1", duration: 240 },
      { top: "", bottom: "GO", duration: 360 },
    ],
  },
  {
    id: "escalation",
    label: "Escalation",
    template: "three_panel",
    width: 720,
    height: 720,
    frames: [
      { top: "ME: JUST ONE TINY CHANGE", bottom: "FRAME ONE", duration: 320 },
      { top: "ME: STILL SAFE", bottom: "FRAME TWO", duration: 320 },
      { top: "PRODUCTION:", bottom: "FRAME THREE", duration: 480 },
    ],
  },
];

const state = {
  templates: [...fallbackTemplates],
  selectedTemplate: "two_panel",
  width: 640,
  height: 640,
  frames: [
    { top: "MONDAY", bottom: "CODING", duration: 400 },
    { top: "WEDNESDAY", bottom: "DEBUGGING", duration: 400 },
    { top: "FRIDAY", bottom: "DEPLOYING", duration: 400 },
  ],
  lastResult: null,
  isRendering: false,
};

const ui = {};

document.addEventListener("DOMContentLoaded", () => {
  cacheElements();
  bindControls();
  renderPresetButtons();
  loadTemplates();
  render();
});

function cacheElements() {
  ui.templateRail = document.getElementById("templateRail");
  ui.presetStrip = document.getElementById("presetStrip");
  ui.widthRange = document.getElementById("widthRange");
  ui.widthInput = document.getElementById("widthInput");
  ui.heightRange = document.getElementById("heightRange");
  ui.heightInput = document.getElementById("heightInput");
  ui.addFrameButton = document.getElementById("addFrameButton");
  ui.frameList = document.getElementById("frameList");
  ui.generateButton = document.getElementById("generateButton");
  ui.scriptPreview = document.getElementById("scriptPreview");
  ui.statusLine = document.getElementById("statusLine");
  ui.previewPlaceholder = document.getElementById("previewPlaceholder");
  ui.gifPreview = document.getElementById("gifPreview");
  ui.downloadLink = document.getElementById("downloadLink");
  ui.currentTemplateName = document.getElementById("currentTemplateName");
  ui.currentTemplateDescription = document.getElementById("currentTemplateDescription");
  ui.currentTemplateImage = document.getElementById("currentTemplateImage");
  ui.heroCanvas = document.getElementById("heroCanvas");
  ui.heroFrames = document.getElementById("heroFrames");
  ui.heroDuration = document.getElementById("heroDuration");
  ui.summaryTemplate = document.getElementById("summaryTemplate");
  ui.summaryFrames = document.getElementById("summaryFrames");
  ui.summaryDuration = document.getElementById("summaryDuration");
  ui.sizeBadge = document.getElementById("sizeBadge");
  ui.durationBadge = document.getElementById("durationBadge");
}

function bindControls() {
  syncDimensionControl(ui.widthRange, ui.widthInput, "width");
  syncDimensionControl(ui.heightRange, ui.heightInput, "height");

  ui.addFrameButton.addEventListener("click", () => {
    const lastFrame = state.frames[state.frames.length - 1];
    state.frames.push({
      top: lastFrame?.top || "",
      bottom: lastFrame?.bottom || "",
      duration: lastFrame?.duration || 400,
    });
    render();
  });

  ui.frameList.addEventListener("input", onFrameInput);
  ui.frameList.addEventListener("click", onFrameAction);
  ui.generateButton.addEventListener("click", renderGif);
}

async function loadTemplates() {
  try {
    const response = await fetch("/api/templates");
    if (!response.ok) {
      throw new Error("Template fetch failed");
    }
    const data = await response.json();
    if (Array.isArray(data.templates) && data.templates.length > 0) {
      state.templates = data.templates;
      if (!state.templates.some((template) => template.id === state.selectedTemplate)) {
        state.selectedTemplate = state.templates[0].id;
      }
      render();
    }
  } catch (error) {
    console.warn("Using fallback template list.", error);
  }
}

function renderPresetButtons() {
  ui.presetStrip.innerHTML = "";
  presets.forEach((preset) => {
    const button = document.createElement("button");
    button.type = "button";
    button.className = "preset-button";
    button.textContent = preset.label;
    button.addEventListener("click", () => {
      applyPreset(preset);
    });
    ui.presetStrip.appendChild(button);
  });
}

function applyPreset(preset) {
  state.selectedTemplate = preset.template;
  state.width = preset.width;
  state.height = preset.height;
  state.frames = preset.frames.map((frame) => ({ ...frame }));
  render();
}

function syncDimensionControl(rangeElement, numberElement, key) {
  const updateValue = (value) => {
    const parsed = clampNumber(value, 240, 1200, state[key]);
    state[key] = parsed;
    rangeElement.value = String(parsed);
    numberElement.value = String(parsed);
    render();
  };

  rangeElement.addEventListener("input", (event) => updateValue(event.target.value));
  numberElement.addEventListener("input", (event) => updateValue(event.target.value));
}

function onFrameInput(event) {
  const target = event.target;
  const index = Number(target.dataset.index);
  if (!Number.isInteger(index) || !state.frames[index]) {
    return;
  }

  if (target.dataset.field === "top" || target.dataset.field === "bottom") {
    state.frames[index][target.dataset.field] = target.value;
    renderScriptPreview();
    updateSummary();
    return;
  }

  if (target.dataset.field === "duration") {
    const duration = clampNumber(target.value, 80, 2500, state.frames[index].duration);
    state.frames[index].duration = duration;
    const card = target.closest(".frame-card");
    if (card) {
      const mirrors = card.querySelectorAll('[data-field="duration"]');
      mirrors.forEach((mirror) => {
        if (mirror !== target) {
          mirror.value = String(duration);
        }
      });
      const durationLabel = card.querySelector(".js-duration-label");
      if (durationLabel) {
        durationLabel.textContent = `${duration} ms`;
      }
    }
    updateSummary();
    renderScriptPreview();
  }
}

function onFrameAction(event) {
  const button = event.target.closest("[data-action]");
  if (!button) {
    return;
  }

  const index = Number(button.dataset.index);
  if (!Number.isInteger(index) || !state.frames[index]) {
    return;
  }

  const { action } = button.dataset;
  if (action === "remove" && state.frames.length > 1) {
    state.frames.splice(index, 1);
  } else if (action === "duplicate") {
    state.frames.splice(index + 1, 0, { ...state.frames[index] });
  } else if (action === "up" && index > 0) {
    [state.frames[index - 1], state.frames[index]] = [state.frames[index], state.frames[index - 1]];
  } else if (action === "down" && index < state.frames.length - 1) {
    [state.frames[index], state.frames[index + 1]] = [state.frames[index + 1], state.frames[index]];
  }

  render();
}

function render() {
  renderTemplateCards();
  renderFrames();
  updateSummary();
  renderScriptPreview();
  updateCurrentTemplate();
}

function renderTemplateCards() {
  ui.templateRail.innerHTML = "";
  state.templates.forEach((template) => {
    const button = document.createElement("button");
    button.type = "button";
    button.className = `template-card${template.id === state.selectedTemplate ? " is-selected" : ""}`;
    const thumb = document.createElement("div");
    thumb.className = "template-card__thumb";
    thumb.style.backgroundImage = `url('${CSS.escape(template.previewUrl)}')`;
    button.appendChild(thumb);

    const content = document.createElement("div");
    content.className = "template-card__content";
    content.innerHTML = `
        <h3>${escapeHtml(template.name)}</h3>
        <p>${escapeHtml(template.description)}</p>
        <span class="template-card__best">${escapeHtml(template.bestFor)}</span>
    `;
    button.appendChild(content);
    button.addEventListener("click", () => {
      state.selectedTemplate = template.id;
      render();
    });
    ui.templateRail.appendChild(button);
  });
}

function renderFrames() {
  ui.frameList.innerHTML = "";
  state.frames.forEach((frame, index) => {
    const card = document.createElement("article");
    card.className = "frame-card";
    card.innerHTML = `
      <div class="frame-card__head">
        <div class="frame-meta">
          <span class="pill">Frame ${index + 1}</span>
          <strong class="js-duration-label">${frame.duration} ms</strong>
        </div>
        <div class="frame-actions">
          <button type="button" class="frame-button" data-action="up" data-index="${index}">Up</button>
          <button type="button" class="frame-button" data-action="down" data-index="${index}">Down</button>
          <button type="button" class="frame-button" data-action="duplicate" data-index="${index}">Duplicate</button>
          <button type="button" class="frame-button" data-action="remove" data-index="${index}">Remove</button>
        </div>
      </div>
      <div class="frame-fields">
        <label class="field-label">
          <span>Top Text</span>
          <textarea data-index="${index}" data-field="top" placeholder="Set up the idea.">${escapeHtml(frame.top)}</textarea>
        </label>
        <label class="field-label">
          <span>Bottom Text</span>
          <textarea data-index="${index}" data-field="bottom" placeholder="Deliver the payoff.">${escapeHtml(frame.bottom)}</textarea>
        </label>
        <div class="dual-control compact-range">
          <div class="dual-control__head">
            <label for="durationRange${index}">Frame Duration</label>
            <span>80-2500 ms</span>
          </div>
          <div class="dual-control__row">
            <input id="durationRange${index}" data-index="${index}" data-field="duration" type="range" min="80" max="2500" step="10" value="${frame.duration}" />
            <input data-index="${index}" data-field="duration" type="number" min="80" max="2500" step="10" value="${frame.duration}" />
          </div>
        </div>
      </div>
    `;
    ui.frameList.appendChild(card);
  });
}

function updateSummary() {
  const template = getSelectedTemplate();
  const totalDuration = state.frames.reduce((sum, frame) => sum + frame.duration, 0);
  const durationLabel = formatDuration(totalDuration);
  const canvasLabel = `${state.width} × ${state.height}`;

  ui.widthRange.value = String(state.width);
  ui.widthInput.value = String(state.width);
  ui.heightRange.value = String(state.height);
  ui.heightInput.value = String(state.height);

  ui.sizeBadge.textContent = canvasLabel;
  ui.durationBadge.textContent = durationLabel;
  ui.heroCanvas.textContent = canvasLabel;
  ui.heroFrames.textContent = String(state.frames.length);
  ui.heroDuration.textContent = durationLabel;
  ui.summaryTemplate.textContent = template?.name || "Custom";
  ui.summaryFrames.textContent = String(state.frames.length);
  ui.summaryDuration.textContent = `${totalDuration} ms`;
}

function updateCurrentTemplate() {
  const template = getSelectedTemplate();
  if (!template) {
    return;
  }
  ui.currentTemplateName.textContent = template.name;
  ui.currentTemplateDescription.textContent = template.description;
  ui.currentTemplateImage.src = template.previewUrl;
  ui.currentTemplateImage.alt = `${template.name} template preview`;
}

function renderScriptPreview(scriptFromServer) {
  ui.scriptPreview.textContent = scriptFromServer || buildMacScriptPreview();
}

function buildMacScriptPreview() {
  const lines = [`var template = Template("${state.selectedTemplate}");`, "var gif = Gif();", ""];
  state.frames.forEach((frame) => {
    const top = normalizeCaption(frame.top);
    const bottom = normalizeCaption(frame.bottom);
    const duration = clampNumber(frame.duration, 80, 2500, 400);
    lines.push(
      `gif.frame(Meme(template).text(Top, "${top}").text(Bottom, "${bottom}").resize(Size(${state.width}, ${state.height})), Duration(${duration}));`
    );
  });
  lines.push("");
  lines.push('gif.save("generated.gif");');
  return lines.join("\n");
}

async function renderGif() {
  if (state.isRendering) {
    return;
  }

  state.isRendering = true;
  ui.generateButton.disabled = true;
  ui.generateButton.textContent = "Rendering...";
  ui.statusLine.textContent = "Rendering GIF through the Mac runtime...";

  try {
    const response = await fetch("/api/generate", {
      method: "POST",
      headers: {
        "Content-Type": "application/json",
      },
      body: JSON.stringify({
        template: state.selectedTemplate,
        width: state.width,
        height: state.height,
        frames: state.frames,
      }),
    });

    const data = await response.json();
    if (!response.ok) {
      throw new Error(data.error || "Render failed.");
    }

    state.lastResult = data;
    ui.gifPreview.src = `${data.previewUrl}?v=${Date.now()}`;
    ui.gifPreview.hidden = false;
    ui.previewPlaceholder.hidden = true;
    ui.downloadLink.href = data.downloadUrl;
    ui.downloadLink.download = "mac-meme.gif";
    ui.downloadLink.hidden = false;
    ui.statusLine.style.color = "";

    const fileSizeKb = Math.max(1, Math.round((data.summary.fileSizeBytes || 0) / 1024));
    ui.statusLine.textContent = `Rendered ${data.summary.frameCount} frames at ${data.summary.width} × ${data.summary.height}. Output size: ${fileSizeKb} KB.`;
    renderScriptPreview(data.script);
  } catch (error) {
    ui.statusLine.textContent = error.message;
    ui.statusLine.style.color = "var(--accent)";
  } finally {
    state.isRendering = false;
    ui.generateButton.disabled = false;
    ui.generateButton.textContent = "Render GIF";
  }
}

function getSelectedTemplate() {
  return state.templates.find((template) => template.id === state.selectedTemplate) || state.templates[0];
}

function normalizeCaption(text) {
  return String(text || "")
    .replaceAll("\\", "\\\\")
    .replaceAll('"', "'")
    .replace(/\s+/g, " ")
    .trim()
    .slice(0, 120);
}

function clampNumber(value, min, max, fallback) {
  const parsed = Number(value);
  if (!Number.isFinite(parsed)) {
    return fallback;
  }
  return Math.max(min, Math.min(max, Math.round(parsed)));
}

function formatDuration(totalMs) {
  if (totalMs >= 1000) {
    return `${(totalMs / 1000).toFixed(totalMs % 1000 === 0 ? 0 : 1)}s`;
  }
  return `${totalMs}ms`;
}

function escapeHtml(value) {
  return String(value)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;");
}
