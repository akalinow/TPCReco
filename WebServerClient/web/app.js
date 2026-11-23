// tpcWebGUI browser client.
// Talks to the Crow server (WebGUIServer) via JSON over HTTP, gets "update"
// notifications over a WebSocket, and draws with Plotly.js.
'use strict';

// ROOT kBird palette (gStyle->SetPalette(57) in tpcGUI)
const KBIRD = [
  [0.000, 'rgb(53,42,135)'], [0.125, 'rgb(15,92,221)'], [0.250, 'rgb(20,129,214)'],
  [0.375, 'rgb(6,164,202)'], [0.500, 'rgb(46,183,164)'], [0.625, 'rgb(135,191,119)'],
  [0.750, 'rgb(209,187,89)'], [0.875, 'rgb(254,200,50)'], [1.000, 'rgb(249,251,14)'],
];

const state = {
  view: null,
  queue: Promise.resolve(),
  pending: 0,
  lastCommandAt: 0,
  panels: [],          // panel JSON currently shown, per plot div
  cursorFrame: null,
  stopped: false,
};

const $ = (id) => document.getElementById(id);

// ---------------------------------------------------------------- server API

async function getJSON(url) {
  const response = await fetch(url, { cache: 'no-store' });
  if (response.status === 401) throw new Error('Access denied: open the URL with the access token printed by tpcWebGUI.');
  if (!response.ok) throw new Error(`${url}: HTTP ${response.status}`);
  return response.json();
}

/** Commands are queued, so clicks are applied in order even if the server is slow. */
function sendCommand(cmd, args = {}) {
  state.queue = state.queue.then(() => runCommand(cmd, args));
  return state.queue;
}

async function runCommand(cmd, args) {
  if (state.stopped) return false;
  setBusy(+1);
  state.lastCommandAt = Date.now();
  try {
    const response = await fetch('/api/command', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ cmd, ...args }),
    });
    if (response.status === 401) throw new Error('access denied (access token missing or wrong)');
    const reply = await response.json();
    showMessage(reply.ok ? '' : reply.error);
    if (reply.view) render(reply.view);
    return reply.ok;
  } catch (error) {
    showMessage(`Command "${cmd}" failed: ${error.message}`);
    return false;
  } finally {
    state.lastCommandAt = Date.now();
    setBusy(-1);
  }
}

async function refreshView() {
  try {
    render(await getJSON('/api/view'));
  } catch (error) {
    showMessage(error.message);
  }
}

function connectWebSocket() {
  const protocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
  const socket = new WebSocket(`${protocol}//${location.host}/ws`);
  socket.onopen = () => $('connection').classList.remove('down');
  socket.onmessage = (event) => {
    // updates caused by this browser's own commands arrive with the command reply
    if (event.data === 'update' && state.pending === 0 && Date.now() - state.lastCommandAt > 1000) {
      refreshView();
    }
  };
  socket.onclose = () => {
    $('connection').classList.add('down');
    if (!state.stopped) setTimeout(connectWebSocket, 2000);
  };
}

function setBusy(delta) {
  state.pending += delta;
  document.body.classList.toggle('busy', state.pending > 0);
}

function showMessage(text) {
  $('message').textContent = text || '';
}

// ---------------------------------------------------------------- rendering

function render(view) {
  state.view = view;
  updateControls(view.status);
  renderPanels(view.panels || [], view.status);
  render3D(view.view3d);
}

/** ROOT TLatex subset used in titles -> HTML understood by Plotly */
function latex(text) {
  if (!text) return '';
  const greek = { alpha: 'α', beta: 'β', gamma: 'γ', delta: 'δ', mu: 'μ', Delta: 'Δ', theta: 'θ', phi: 'φ' };
  return text
    .replace(/#(alpha|beta|gamma|delta|mu|Delta|theta|phi)/g, (_, name) => greek[name])
    .replace(/\^\{([^}]*)\}/g, '<sup>$1</sup>')
    .replace(/_\{([^}]*)\}/g, '<sub>$1</sub>');
}

function ensurePanelDivs(count) {
  const container = $('plots');
  while (container.children.length < count) {
    const div = document.createElement('div');
    div.className = 'panel';
    div.dataset.index = container.children.length;
    container.appendChild(div);
    installPanelHandlers(div);
  }
  while (container.children.length > count) {
    Plotly.purge(container.lastChild);
    container.removeChild(container.lastChild);
  }
  return Array.from(container.children);
}

function renderPanels(panels, status) {
  state.panels = panels;
  const divs = ensurePanelDivs(panels.length);
  panels.forEach((panel, index) => renderPanel(divs[index], panel, index, status));
}

function heatmapTrace(panel) {
  let z = panel.z;
  const colorbar = { title: { text: latex(panel.zTitle), side: 'right' }, thickness: 12 };
  if (panel.logZ) {
    let max = -Infinity;
    z = z.map((row) => row.map((v) => {
      if (v === null || v <= 0) return null;
      const l = Math.log10(v);
      if (l > max) max = l;
      return l;
    }));
    if (Number.isFinite(max)) {
      const ticks = [];
      for (let k = 0; k <= Math.ceil(max); ++k) ticks.push(k);
      colorbar.tickvals = ticks;
      colorbar.ticktext = ticks.map((k) => (k < 4 ? String(10 ** k) : `10<sup>${k}</sup>`));
    }
  }
  return {
    type: 'heatmap', x: panel.x, y: panel.y, z,
    colorscale: KBIRD, colorbar, zsmooth: false, hoverongaps: false,
    hovertemplate: 'x: %{x:.1f}<br>y: %{y:.1f}<br>z: %{z:.4g}<extra></extra>',
  };
}

function lineTrace(item, showLegend) {
  return {
    type: 'scatter', mode: 'lines', x: item.x, y: item.y, name: item.name || '',
    showlegend: showLegend && !!item.name,
    line: { color: item.color, width: item.width, dash: item.dash || 'solid', shape: item.shape || 'linear' },
    hovertemplate: 'x: %{x:.4g}<br>y: %{y:.4g}<extra></extra>',
  };
}

function markerTrace(item) {
  return {
    type: 'scatter', mode: 'markers', x: item.x, y: item.y, showlegend: false,
    marker: { color: item.color, symbol: item.symbol, size: item.size, line: { color: item.lineColor, width: 1 } },
    hovertemplate: 'x: %{x:.4g}<br>y: %{y:.4g}<extra></extra>',
  };
}

function panelRevision(status, index) {
  // keeps the user's zoom while working on one event; reset for a new event or display mode
  return [status.file, status.entry, status.eventId, status.display,
    status.flags.autoZoom, status.flags.recoMode, index].join('|');
}

function renderPanel(div, panel, index, status) {
  const traces = [];
  if (panel.kind === 'heatmap') traces.push(heatmapTrace(panel));
  (panel.series || []).forEach((item) => traces.push(lineTrace(item, true)));
  (panel.lines || []).forEach((item) => traces.push(lineTrace(item, false)));
  (panel.markers || []).forEach((item) => traces.push(markerTrace(item)));

  const shapes = (panel.vlines || []).map((v) => ({
    type: 'line', xref: 'x', yref: 'paper', x0: v.x, x1: v.x, y0: 0, y1: 1,
    line: { color: v.color, width: v.width || 1 },
  }));
  const annotations = (panel.texts || []).map((t) => ({
    xref: 'paper', yref: 'paper', x: t.x, y: t.y, text: latex(t.text),
    showarrow: false, xanchor: 'left', font: { size: 13 },
  }));
  const hasLegend = (panel.series || []).some((s) => s.name);
  const axis = (title, range) => ({
    title: { text: latex(title) }, showgrid: false, zeroline: false, mirror: true,
    showline: true, linecolor: '#444', ticks: 'outside',
    ...(range ? { range, autorange: false } : { autorange: true }),
  });
  const layout = {
    title: { text: latex(panel.title), font: { size: 13 } },
    margin: { l: 62, r: 16, t: annotations.length ? 70 : 36, b: 46 },
    xaxis: axis(panel.xTitle, panel.xRange),
    yaxis: { ...axis(panel.yTitle, panel.yRange), ...(panel.equalAspect ? { scaleanchor: 'x', scaleratio: 1 } : {}) },
    plot_bgcolor: panel.frameColor || '#ffffff',
    paper_bgcolor: '#ffffff',
    shapes, annotations,
    showlegend: hasLegend,
    legend: { x: 1, xanchor: 'right', y: 1, bgcolor: 'rgba(255,255,255,0.85)', bordercolor: '#888', borderwidth: 1 },
    hovermode: 'closest',
    uirevision: panelRevision(status, index),
  };
  const accepting = panel.clickDir !== undefined && status.reco.acceptPoints;
  div.classList.toggle('accepting', accepting);
  Plotly.react(div, traces, layout, {
    responsive: true, displaylogo: false,
    // a double click would set two points
    doubleClick: accepting ? false : 'reset+autosize',
    modeBarButtonsToRemove: ['lasso2d', 'select2d'],
  });
}

// ---------------------------------------------------------------- manual reconstruction clicks

/** data coordinates of a mouse event inside the plot area of a Plotly div, or null */
function dataCoordinates(div, event) {
  const fullLayout = div._fullLayout;
  if (!fullLayout) return null;
  const xa = fullLayout.xaxis;
  const ya = fullLayout.yaxis;
  const box = div.getBoundingClientRect();
  const px = event.clientX - box.left - xa._offset;
  const py = event.clientY - box.top - ya._offset;
  if (px < 0 || py < 0 || px > xa._length || py > ya._length) return null;
  return { x: xa.p2c(px), y: ya.p2c(py) };
}

function panelOf(div) {
  return state.panels[Number(div.dataset.index)];
}

function installPanelHandlers(div) {
  // Plotly covers the page while the mouse button is down, so a click is detected
  // as mousedown + mouseup at (almost) the same position.
  div.addEventListener('mousedown', (event) => {
    if (event.button === 0) div._tpcDown = { x: event.clientX, y: event.clientY };
  }, true);
  div.addEventListener('mousemove', (event) => moveCursorLine(div, event), true);
}

document.addEventListener('mouseup', (event) => {
  for (const div of $('plots').children) {
    const down = div._tpcDown;
    div._tpcDown = null;
    if (!down || Math.abs(down.x - event.clientX) > 3 || Math.abs(down.y - event.clientY) > 3) continue;
    const panel = panelOf(div);
    if (!panel || panel.clickDir === undefined || !state.view.status.reco.acceptPoints) continue;
    const point = dataCoordinates(div, event);
    if (point) sendCommand('recoClick', { dir: panel.clickDir, x: point.x, y: point.y });
  }
});

/** vertical line on the charge vs time panel following the mouse (as in tpcGUI) */
function moveCursorLine(div, event) {
  const panel = panelOf(div);
  const timePanelIndex = state.panels.findIndex((p) => p.cursorLineColor);
  if (!panel || panel.clickDir === undefined || timePanelIndex < 0) return;
  const point = dataCoordinates(div, event);
  if (!point) return;
  if (state.cursorFrame) cancelAnimationFrame(state.cursorFrame);
  state.cursorFrame = requestAnimationFrame(() => {
    const timeDiv = $('plots').children[timePanelIndex];
    const timePanel = state.panels[timePanelIndex];
    const fixed = (timePanel.vlines || []).length;
    Plotly.relayout(timeDiv, {
      [`shapes[${fixed}]`]: {
        type: 'line', xref: 'x', yref: 'paper', x0: point.x, x1: point.x, y0: 0, y1: 1,
        line: { color: timePanel.cursorLineColor, width: 2 },
      },
    });
  });
}

// ---------------------------------------------------------------- 3D view

function render3D(view3d) {
  const box = $('view3dBox');
  if (!view3d) {
    if (!box.hidden) Plotly.purge($('view3d'));
    box.hidden = true;
    return;
  }
  box.hidden = false;
  const traces = [
    {
      type: 'scatter3d', mode: 'lines', name: 'drift cage', hoverinfo: 'skip', connectgaps: false,
      x: view3d.cage.x, y: view3d.cage.y, z: view3d.cage.z, line: { color: '#000000', width: 4 },
    },
    {
      type: 'scatter3d', mode: 'lines', name: 'beam', hoverinfo: 'skip',
      x: view3d.beam.x, y: view3d.beam.y, z: view3d.beam.z, line: { color: '#008800', width: 3, dash: 'dot' },
    },
  ];
  view3d.segments.forEach((segment, i) => traces.push({
    type: 'scatter3d', mode: 'lines+markers', name: `segment ${i + 1}`,
    x: segment.x, y: segment.y, z: segment.z,
    line: { color: segment.color, width: 8 }, marker: { color: segment.color, size: 3 },
    hovertemplate: 'x: %{x:.1f}<br>y: %{y:.1f}<br>z: %{z:.1f}<extra></extra>',
  }));
  Plotly.react($('view3d'), traces, {
    margin: { l: 0, r: 0, t: 0, b: 0 },
    showlegend: false,
    uirevision: '3d', // keep the camera between events
    scene: {
      aspectmode: 'data',
      xaxis: { title: { text: 'x [mm]' } },
      yaxis: { title: { text: 'y [mm]' } },
      zaxis: { title: { text: 'z [mm]' } },
    },
  }, { responsive: true, displaylogo: false });
}

// ---------------------------------------------------------------- controls

function setChecked(id, value, enabled) {
  const box = $(id);
  box.checked = !!value;
  box.disabled = !enabled;
}

function setValueUnlessEditing(id, value) {
  const input = $(id);
  if (document.activeElement !== input && value !== undefined) input.value = value;
}

function updateControls(status) {
  $('modeBadge').textContent = status.mode;
  $('modeLabel').textContent = status.mode;
  $('fileName').textContent = status.file || 'No input.';
  $('nEvents').textContent = status.nEvents;
  $('eventId').textContent = status.eventId;
  $('entry').textContent = status.entry;

  const enabled = status.enabled;
  $('btnNext').disabled = !enabled.navigate;
  $('btnPrevious').disabled = !enabled.previous;
  $('btnReset').disabled = !enabled.navigate;
  $('btnResetRate').hidden = !enabled.resetRate;
  $('btnOpen').disabled = !enabled.open;

  setChecked('chkLogScale', status.flags.logScale, true);
  setChecked('chkAutoZoom', status.flags.autoZoom, true);
  setChecked('chkRecoMode', status.flags.recoMode, enabled.recoMode || status.flags.recoMode);
  setChecked('chkRate', status.flags.rate, enabled.rate);

  $('gotoBox').hidden = !enabled.navigate;
  $('eventTypeBox').hidden = !enabled.eventType;
  $('markersBox').hidden = !enabled.markers;
  $('runConditionsBox').hidden = !enabled.runConditions;

  const eventTypes = $('eventTypes');
  if (!eventTypes.children.length) {
    status.eventTypeNames.forEach((name, bit) => {
      const label = document.createElement('label');
      label.innerHTML = `<input type="checkbox" data-bit="${bit}"> ${name}`;
      eventTypes.appendChild(label);
    });
  }

  $('btnAddSegment').disabled = !status.reco.addEnabled;
  $('btnFitSegments').disabled = !status.reco.fitEnabled;
  $('btnClearTrack').disabled = !status.reco.clearEnabled;
  $('recoHint').hidden = !status.reco.acceptPoints;

  if (status.runConditions) {
    setValueUnlessEditing('inpDriftVelocity', status.runConditions.driftVelocity);
    setValueUnlessEditing('inpSamplingRate', status.runConditions.samplingRate);
    setValueUnlessEditing('inpTriggerDelay', status.runConditions.triggerDelay);
  }
}

/** event type bits from the check boxes; the boxes are cleared afterwards (as in tpcGUI) */
function takeEventType() {
  let bits = 0;
  for (const box of $('eventTypes').querySelectorAll('input')) {
    if (box.checked) bits |= 1 << Number(box.dataset.bit);
    box.checked = false;
  }
  return bits;
}

// ---------------------------------------------------------------- unsaved clicked track warnings

const UNSAVED_TRACK = 'The clicked track of this event has not been saved yet: '
  + 'it is written to the output file only with "Next event".';
const PENDING_POINTS = 'Clicked points/segments have not been fitted yet ("Fit segments").';

/**
 * Asks before an action that loses the manual reconstruction of the current event.
 * kind: 'leave' (another event, file, exit, reco mode off), 'next', 'reset'.
 * Returns true when the action may proceed.
 */
function confirmClickedTrack(kind) {
  const reco = state.view?.status?.reco;
  if (!reco) return true;
  const lines = [];
  if (kind === 'leave') {
    if (reco.unsavedTrack) lines.push(UNSAVED_TRACK);
    if (reco.pendingPoints) lines.push(`${PENDING_POINTS} They will be lost.`);
    if (lines.length) lines.push('Leave this event anyway?');
  } else if (kind === 'next' && reco.pendingPoints) {
    lines.push(PENDING_POINTS,
      reco.unsavedTrack ? 'They are not saved: the previously fitted track is written instead.'
        : 'They are not saved: the automatic reconstruction is written instead.',
      'Go to the next event anyway?');
  } else if (kind === 'reset' && reco.pendingPoints) {
    lines.push(`${PENDING_POINTS} Reset clears them.`, 'Reset anyway?');
  }
  return lines.length === 0 || confirm(lines.join('\n\n'));
}

function installControls() {
  $('btnNext').onclick = () => {
    if (confirmClickedTrack('next')) sendCommand('next', { eventType: takeEventType() });
  };
  $('btnPrevious').onclick = () => {
    if (confirmClickedTrack('leave')) sendCommand('previous', { eventType: takeEventType() });
  };
  $('btnReset').onclick = () => { if (confirmClickedTrack('reset')) sendCommand('reset'); };
  $('btnResetRate').onclick = () => sendCommand('resetRate');
  $('btnExit').onclick = exitApplication;

  const option = (id, name) => { $(id).onchange = (e) => sendCommand('set', { option: name, value: e.target.checked }); };
  option('chkLogScale', 'logScale');
  option('chkAutoZoom', 'autoZoom');
  $('chkRecoMode').onchange = (e) => {
    // switching the manual reconstruction off: the clicked track would not be written on "Next"
    if (!e.target.checked && !confirmClickedTrack('leave')) { e.target.checked = true; return; }
    sendCommand('set', { option: 'recoMode', value: e.target.checked });
  };
  option('chkRate', 'rate');

  $('formGotoEvent').onsubmit = (e) => {
    e.preventDefault();
    if (confirmClickedTrack('leave')) sendCommand('gotoEvent', { eventId: Number($('inpEventId').value) });
  };
  $('formGotoEntry').onsubmit = (e) => {
    e.preventDefault();
    if (confirmClickedTrack('leave')) sendCommand('gotoEntry', { entry: Number($('inpEntry').value) });
  };
  $('formRunConditions').onsubmit = (e) => {
    e.preventDefault();
    sendCommand('runConditions', {
      driftVelocity: Number($('inpDriftVelocity').value),
      samplingRate: Number($('inpSamplingRate').value),
      triggerDelay: Number($('inpTriggerDelay').value),
    });
  };

  $('btnAddSegment').onclick = () => sendCommand('recoAddSegment');
  $('btnFitSegments').onclick = () => sendCommand('recoFit');
  $('btnClearTrack').onclick = () => sendCommand('recoClearTrack');

  installOpenDialog();
}

async function exitApplication() {
  if (!confirmClickedTrack('leave')) return;
  if (!confirm('Stop the tpcWebGUI server? The output file is closed.')) return;
  try {
    await fetch('/api/command', { method: 'POST', body: JSON.stringify({ cmd: 'exit' }) });
  } catch (error) { /* server already gone */ }
  state.stopped = true;
  $('stopped').hidden = false;
}

// ---------------------------------------------------------------- open file dialog

function installOpenDialog() {
  const dialog = $('openDialog');
  const selected = new Set();

  const showDirectory = async (dir) => {
    let listing;
    try {
      listing = await getJSON(`/api/ls?dir=${encodeURIComponent(dir)}`);
    } catch (error) {
      showMessage(error.message);
      return;
    }
    if (listing.error) { showMessage(listing.error); return; }
    $('inpDir').value = listing.dir;
    $('btnUp').onclick = () => showDirectory(listing.parent);
    selected.clear();
    const list = $('dirList');
    list.replaceChildren();
    for (const entry of listing.entries) {
      const item = document.createElement('li');
      item.textContent = entry.name;
      const path = `${listing.dir}/${entry.name}`;
      if (entry.isDir) {
        item.className = 'dir';
        item.onclick = () => showDirectory(path);
      } else {
        // several GRAW files (one per AsAd board) can be selected together
        item.onclick = (e) => {
          if (!e.ctrlKey && !e.metaKey && !e.shiftKey) {
            selected.clear();
            list.querySelectorAll('.selected').forEach((li) => li.classList.remove('selected'));
          }
          if (selected.has(path)) { selected.delete(path); item.classList.remove('selected'); }
          else { selected.add(path); item.classList.add('selected'); }
          $('inpPath').value = Array.from(selected).join(',');
        };
        item.ondblclick = () => { $('inpPath').value = path; dialog.close('open'); };
      }
      list.appendChild(item);
    }
  };

  $('btnOpen').onclick = () => {
    if (!confirmClickedTrack('leave')) return;
    $('inpPath').value = '';
    showDirectory('');
    dialog.showModal();
  };
  $('inpDir').onkeydown = (e) => {
    if (e.key === 'Enter') { e.preventDefault(); showDirectory($('inpDir').value); }
  };
  dialog.onclose = () => {
    const path = $('inpPath').value.trim();
    if (dialog.returnValue === 'open' && path) sendCommand('open', { path });
  };
}

// ---------------------------------------------------------------- start

// the access token is kept in a cookie: remove it from the address bar (screenshots, shared links)
if (new URLSearchParams(location.search).has('token')) history.replaceState(null, '', location.pathname);

installControls();
refreshView();
connectWebSocket();
