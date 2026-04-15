// Mac Studio — Imgflip meme template integration (no API key needed)

const IMGFLIP_API = "https://api.imgflip.com/get_memes";
let imgflipMemes = [];
let imgflipLoaded = false;

async function loadImgflipMemes() {
  if (imgflipLoaded) return;
  try {
    const res = await fetch(IMGFLIP_API);
    const data = await res.json();
    if (data.success && data.data && data.data.memes) {
      imgflipMemes = data.data.memes;
      imgflipLoaded = true;
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

  grid.innerHTML = filtered.map((m) =>
    `<button class="template-card" type="button" data-imgflip-url="${escAttr(m.url)}" data-imgflip-name="${escAttr(m.name)}" title="${escAttr(m.name)}">
      <div class="template-card__thumb" style="background-image:url('${cssUrl(m.url)}')"></div>
      <span class="template-card__name">${esc(m.name)}</span>
    </button>`
  ).join("");
}

function onImgflipPick(e) {
  const btn = e.target.closest("[data-imgflip-url]");
  if (!btn) return;
  const url = btn.dataset.imgflipUrl;
  const name = btn.dataset.imgflipName;

  // Upload the image to our server so the Mac binary can access it
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
      state.metadata.templates.push({
        id: data.templateId, name: data.name, description: "Imgflip meme template",
        bestFor: "Imported meme template.", previewUrl: data.previewUrl, category: "uploads",
      });
      const slot = selectedSlot();
      if (slot) slot.templateId = data.templateId;
      setStatus(`Imported: ${name}`, "", "normal");
      renderAll();
      schedulePreview();
    })
    .catch((err) => {
      setStatus(`Import failed: ${err.message}`, "", "error");
      renderStatus();
    });
}
