const panelTitle = document.getElementById('panelTitle');
const panelBody = document.getElementById('panelBody');
const closePanel = document.getElementById('closePanel');
const infoPanel = document.getElementById('infoPanel');

function showInfo(title, body) {
  panelTitle.textContent = title;
  panelBody.textContent = body;
  infoPanel.style.display = 'block';
}

document.querySelectorAll('.module-hit, .pin-hit').forEach((el) => {
  el.addEventListener('click', () => {
    showInfo(el.dataset.title || '说明', el.dataset.body || '暂无说明');
  });
});

closePanel.addEventListener('click', () => {
  infoPanel.style.display = 'none';
});

window.addEventListener('keydown', (event) => {
  if (event.key === 'Escape') {
    infoPanel.style.display = 'none';
  }
});
