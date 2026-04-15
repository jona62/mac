// Mac Studio — Entry point
// Components: js/state.js, js/document.js, js/network.js, js/controls.js, js/render.js, js/highlight.js

document.addEventListener("DOMContentLoaded", async () => {
  bindStaticControls();
  await loadMetadata();
  applyDocument(buildDefaultDocument(), { silent: true });
  populateSelectOptions();
  renderAll();
  if (state.backendCompatibility === "studio") {
    schedulePreview(80);
  }
});
