// Mac Studio — Imgflip meme template integration (no API key needed)
// Caches API response in localStorage for 24h to avoid throttling.

const IMGFLIP_API = "https://api.imgflip.com/get_memes";
const IMGFLIP_CACHE_KEY = "mac_imgflip_memes";
const IMGFLIP_CACHE_TTL = 24 * 60 * 60 * 1000; // 24 hours
const imgflipImported = new Set(); // track already-imported URLs this session
let imgflipMemes = [];
let imgflipLoaded = false;

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
        renderImgflipGrid();
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
      // Cache in localStorage
      try {
        localStorage.setItem(IMGFLIP_CACHE_KEY, JSON.stringify({ ts: Date.now(), memes: imgflipMemes }));
      } catch (_) { /* quota exceeded — ignore */ }
      renderImgflipGrid();
    }
  } catch (err) {
    $("imgflipGrid").innerHTML = '<p class="empty-state">Could not load meme templates.</p>';
  }
}

function renderImgflipGrid(filter = "") {
  const grid = $("imgflipGrid");
  if (!grid) return;
  const query = filter.toLowerCase();
  const filtered = query
    ? imgflipMemes.filter((m) => m.name.toLowerCase().includes(query))
    : imgflipMemes;

  if (!filtered.length) {
    grid.innerHTML = '<p class="empty-state">No memes match your search.</p>';
    return;
  }

  grid.innerHTML = filtered.map((m) => {
    const imported = imgflipImported.has(m.url);
    return `<button class="template-card${imported ? " template-card--imported" : ""}" type="button" data-imgflip-url="${escAttr(m.url)}" data-imgflip-name="${escAttr(m.name)}" title="${escAttr(m.name)}">
      <div class="template-card__thumb" style="background-image:url('${cssUrl(m.url)}')"></div>
      <span class="template-card__name">${esc(m.name)}${imported ? " ✓" : ""}</span>
    </button>`;
  }).join("");
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
