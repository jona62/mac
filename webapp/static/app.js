// Mac Studio — Entry point
// Components: js/state.js, js/document.js, js/network.js, js/controls.js, js/render.js, js/highlight.js

document.addEventListener("DOMContentLoaded", async () => {
  bindStaticControls();
  try {
    await loadMetadata();
  } catch (e) {
    setStatus("Failed to connect to server.", "Check that the server is running.", "error");
    renderStatus();
    return;
  }
  applyDocument(buildDefaultDocument(), { silent: true });
  populateSelectOptions();
  renderAll();
  if (state.backendCompatibility === "studio") {
    schedulePreview(80, true);
  }
});
