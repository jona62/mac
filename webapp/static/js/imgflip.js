// Mac Studio — Imgflip meme template integration (no API key needed)
// Caches API response in localStorage for 24h to avoid throttling.

const IMGFLIP_API = "https://api.imgflip.com/get_memes";
const IMGFLIP_CACHE_KEY = "mac_imgflip_memes";
const IMGFLIP_CACHE_TTL = 24 * 60 * 60 * 1000; // 24 hours
const IMGFLIP_BATCH_SIZE = 20;
const imgflipImported = new Set();
let imgflipMemes = [];
let imgflipLoaded = false;
let imgflipRenderedCount = 0;
let imgflipCurrentFilter = "";

async function loadImgflipMemes() {
  if (imgflipLoaded) return;

  // Try localStorage cache first
  try {
    const cached = localStorage.getItem(IMGFLIP_CACHE_KEY);
    if (cached) {
      const { ts, memes } = JSON.parse(cached);
      if (Date.now() - ts < IMGFLIP_CACHE_TTL && Array.isArray(memes) && memes.length) {
        imgflipMemes = memes;
        imgflipLoaded = true;
        renderImgflipGrid("");
        initImgflipScroll();
        return;
      }
    }
  } catch (_) { /* ignore corrupt cache */ }

  // Fetch from API
  try {
    const res = await fetch(IMGFLIP_API);
    const data = await res.json();
    if (data.success && data.data && data.data.memes) {
      imgflipMemes = data.data.memes;
      imgflipLoaded = true;
      try {
        localStorage.setItem(IMGFLIP_CACHE_KEY, JSON.stringify({ ts: Date.now(), memes: imgflipMemes }));
      } catch (_) { /* quota exceeded */ }
      renderImgflipGrid("");
      initImgflipScroll();
    }
  } catch (err) {
    $("imgflipGrid").innerHTML = '<p class="empty-state">Could not load meme templates.</p>';
  }
}

function getFilteredImgflip() {
  const query = imgflipCurrentFilter.toLowerCase();
  return query
    ? imgflipMemes.filter((m) => m.name.toLowerCase().includes(query))
    : imgflipMemes;
}

function renderImgflipGrid(filter) {
  if (filter !== undefined) {
    imgflipCurrentFilter = filter;
    imgflipRenderedCount = 0;
    $("imgflipGrid").innerHTML = "";
  }
  const grid = $("imgflipGrid");
  if (!grid) return;

  const filtered = getFilteredImgflip();
  if (!filtered.length) {
    grid.innerHTML = '<p class="empty-state">No memes match your search.</p>';
    return;
  }

  const batch = filtered.slice(imgflipRenderedCount, imgflipRenderedCount + IMGFLIP_BATCH_SIZE);
  batch.forEach((m) => {
    const imported = imgflipImported.has(m.url);
    const btn = document.createElement("button");
    btn.className = `template-card${imported ? " template-card--imported" : ""}`;
    btn.type = "button";
    btn.dataset.imgflipUrl = m.url;
    btn.dataset.imgflipName = m.name;
    btn.title = m.name;
    btn.innerHTML = `<div class="template-card__thumb" style="background-image:url('${cssUrl(m.url)}')"></div>
      <span class="template-card__name">${esc(m.name)}${imported ? " ✓" : ""}</span>`;
    grid.appendChild(btn);
  });
  imgflipRenderedCount += batch.length;
}

function initImgflipScroll() {
  const pane = document.querySelector('[data-pane="imgflip"]');
  if (!pane) return;
  const scrollParent = pane.closest(".sidebar__scroll");
  if (!scrollParent) return;
  scrollParent.addEventListener("scroll", () => {
    const { scrollTop, scrollHeight, clientHeight } = scrollParent;
    if (scrollTop + clientHeight >= scrollHeight - 100) {
      const filtered = getFilteredImgflip();
      if (imgflipRenderedCount < filtered.length) {
        renderImgflipGrid();
      }
    }
  });
}

function onImgflipPick(e) {
  const btn = e.target.closest("[data-imgflip-url]");
  if (!btn) return;
  const url = btn.dataset.imgflipUrl;
  const name = btn.dataset.imgflipName;

  // If already imported this session, just reuse the existing upload
  if (imgflipImported.has(url)) {
    const existing = state.metadata.templates.find((t) => t.description === "Imgflip: " + name);
    if (existing) {
      const slot = selectedSlot();
      if (slot) slot.templateId = existing.id;
      commitChange();
      return;
    }
  }

  setStatus("Importing meme template...", "", "normal");
  renderStatus();

  fetch(url)
    .then((res) => res.blob())
    .then((blob) => {
      const form = new FormData();
      const ext = url.split(".").pop().split("?")[0] || "jpg";
      form.append("image", blob, `${name.replace(/[^a-zA-Z0-9]/g, "_")}.${ext}`);
      return fetch("/api/upload", { method: "POST", body: form });
    })
    .then((res) => res.json())
    .then((data) => {
      if (!data.ok) throw new Error(data.error || "Import failed.");
      imgflipImported.add(url);
      state.metadata.templates.push({
        id: data.templateId, name: data.name, description: "Imgflip: " + name,
        bestFor: "Imported meme template.", previewUrl: data.previewUrl, category: "uploads",
      });
      const slot = selectedSlot();
      if (slot) slot.templateId = data.templateId;
      setStatus(`Imported: ${name}`, "", "normal");
      renderAll();
      renderImgflipGrid($("imgflipSearch").value);
      schedulePreview();
    })
    .catch((err) => {
      setStatus(`Import failed: ${err.message}`, "", "error");
      renderStatus();
    });
}
