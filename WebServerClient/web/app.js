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
  drag: null,          // clicked point being dragged
  stopped: false,      // the server was stopped with the Exit button
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
  setBusy(+1);
  state.lastCommandAt = Date.now();
  try {
    const response = await fetch('api/command', {
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
    render(await getJSON('api/view'));
  } catch (error) {
    showMessage(error.message);
  }
}

function connectWebSocket() {
  const protocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
  // relative to the page, so it works under a path prefix too
  const socket = new WebSocket(`${protocol}//${location.host}${location.pathname.replace(/[^/]*$/, '')}ws`);
  socket.onopen = () => $('connection').classList.remove('down');
  socket.onmessage = (event) => {
    // updates caused by this browser's own commands arrive with the command reply
    if (event.data === 'update' && !state.stopped && state.pending === 0 && Date.now() - state.lastCommandAt > 1000) {
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
  renderRecoText(view.recoText);
}

// ---------------------------------------------------------------- reconstruction printout

// terminal (ANSI) colours on a dark background
const ANSI_COLORS = {
  30: '#7f7f7f', 31: '#ff6b6b', 32: '#6ee36e', 33: '#ffd75f',
  34: '#6cb6ff', 35: '#e58cff', 36: '#5fe0e0', 37: '#e6e6e6',
};

/** text with ANSI colour codes -> HTML (escaped) with coloured spans */
function ansiToHtml(text) {
  const escape = (t) => t.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
  let html = '';
  let open = false;
  for (const part of text.split(/(\x1b\[[0-9;]*m)/)) {
    const code = part.match(/^\x1b\[([0-9;]*)m$/);
    if (!code) { html += escape(part); continue; }
    if (open) { html += '</span>'; open = false; }
    const color = code[1].split(';').map(Number).map((n) => ANSI_COLORS[n]).find(Boolean);
    if (color) { html += `<span style="color:${color}">`; open = true; }
  }
  return open ? `${html}</span>` : html;
}

function renderRecoText(text) {
  const box = $('recoInfoBox');
  box.hidden = !text;
  document.querySelector('main').classList.toggle('no-terminal', !text);
  if (!text) return;
  const pre = $('recoInfo');
  const html = ansiToHtml(text);
  if (pre.innerHTML !== html) pre.innerHTML = html;
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
    type: 'scatter', mode: 'markers', x: item.x, y: item.y, name: item.name || '', showlegend: !!item.name,
    marker: { color: item.color, symbol: item.symbol, size: item.size, line: { color: item.lineColor, width: item.lineWidth || 1 } },
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

  const shapes = vlineShapes(panel);
  // extra information (e.g. track length, energy) in the top left corner of the frame
  const info = (panel.texts || []).map((t) => latex(t.text)).join('<br>');
  const annotations = info ? [{
    xref: 'paper', yref: 'paper', x: 0.02, y: 0.98, xanchor: 'left', yanchor: 'top', align: 'left',
    text: info, showarrow: false, font: { size: 15 },
    bgcolor: 'rgba(255,255,255,0.8)', borderpad: 3,
  }] : [];
  // the U/V/W strip projections have no title (the axis titles say it all)
  const isStripProjection = panel.kind === 'heatmap' && /^[UVW] \[/.test(panel.yTitle || '');
  const title = isStripProjection ? '' : latex(panel.title);
  const hasLegend = (panel.series || []).some((s) => s.name) || (panel.markers || []).some((m) => m.name);
  const axis = (title, range) => ({
    title: { text: latex(title) }, showgrid: false, zeroline: false, mirror: true,
    showline: true, linecolor: '#444', ticks: 'outside',
    ...(range ? { range, autorange: false } : { autorange: true }),
  });
  const layout = {
    // title just above the frame, below the Plotly toolbar
    title: { text: title, font: { size: 12 }, yref: 'paper', y: 1, yanchor: 'bottom', pad: { b: 6 } },
    margin: { l: 62, r: 16, t: title ? 50 : (isStripProjection ? 12 : 32), b: 46 },
    xaxis: axis(panel.xTitle, panel.xRange),
    yaxis: { ...axis(panel.yTitle, panel.yRange), ...(panel.equalAspect ? { scaleanchor: 'x', scaleratio: 1 } : {}) },
    plot_bgcolor: panel.frameColor || '#ffffff',
    paper_bgcolor: '#ffffff',
    shapes, annotations,
    showlegend: hasLegend,
    legend: { x: 1, xanchor: 'right', y: 1, bgcolor: 'rgba(255,255,255,0.85)', bordercolor: '#888', borderwidth: 1 },
    // no hover tooltips on the U/V/W projections (they cover the track while clicking points)
    hovermode: isStripProjection ? false : 'closest',
    uirevision: panelRevision(status, index),
  };
  const accepting = panel.clickDir !== undefined && status.reco.acceptPoints;
  div.classList.toggle('accepting', accepting);
  Plotly.react(div, traces, layout, {
    responsive: true, displaylogo: false,
    // no Plotly toolbar on the U/V/W projections (drag still zooms, double click resets)
    displayModeBar: !isStripProjection,
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

/** data coordinates of a mouse event, clamped to the plot area of a Plotly div */
function clampedDataCoordinates(div, event) {
  const xa = div._fullLayout.xaxis;
  const ya = div._fullLayout.yaxis;
  const box = div.getBoundingClientRect();
  const px = Math.min(Math.max(event.clientX - box.left - xa._offset, 0), xa._length);
  const py = Math.min(Math.max(event.clientY - box.top - ya._offset, 0), ya._length);
  return { x: xa.p2c(px), y: ya.p2c(py) };
}

const DRAG_RADIUS = 8; // [px] a mouse press this close to a clicked point drags it

/** clicked point (panel.dragPoints) under the mouse, or null */
function dragPointAt(div, event) {
  const panel = panelOf(div);
  if (!panel || !panel.dragPoints || !panel.dragPoints.length || !div._fullLayout) return null;
  const xa = div._fullLayout.xaxis;
  const ya = div._fullLayout.yaxis;
  const box = div.getBoundingClientRect();
  let best = null;
  let bestDistance = DRAG_RADIUS;
  for (const point of panel.dragPoints) {
    const px = box.left + xa._offset + xa.c2p(point.x);
    const py = box.top + ya._offset + ya.c2p(point.y);
    const distance = Math.hypot(px - event.clientX, py - event.clientY);
    if (distance <= bestDistance) { best = point; bestDistance = distance; }
  }
  return best;
}

function installPanelHandlers(div) {
  // Plotly covers the page while the mouse button is down, so a click is detected
  // as mousedown + mouseup at (almost) the same position.
  div.addEventListener('mousedown', (event) => {
    if (event.button !== 0) return;
    const point = dragPointAt(div, event);
    if (point) { // drag a clicked point; Plotly does not get the event (no zoom box)
      event.stopPropagation();
      event.preventDefault();
      startPointDrag(div, point, event);
      return;
    }
    div._tpcDown = { x: event.clientX, y: event.clientY };
  }, true);
  div.addEventListener('mousemove', (event) => {
    moveCursorLine(div, event);
    div.classList.toggle('over-point', !state.drag && dragPointAt(div, event) !== null);
  }, true);
}

// ---------------------------------------------------------------- dragging clicked points

function startPointDrag(div, point, event) {
  const ghost = document.createElement('div');
  ghost.className = 'drag-ghost';
  document.body.appendChild(ghost);
  document.body.classList.add('dragging-point');
  state.drag = { div, index: point.i, dir: panelOf(div).clickDir, x0: event.clientX, y0: event.clientY, moved: false, ghost };
  moveGhost(event);
}

function moveGhost(event) {
  const drag = state.drag;
  const xa = drag.div._fullLayout.xaxis;
  const ya = drag.div._fullLayout.yaxis;
  const box = drag.div.getBoundingClientRect();
  const left = box.left + xa._offset;
  const top = box.top + ya._offset;
  drag.ghost.style.left = `${Math.min(Math.max(event.clientX, left), left + xa._length)}px`;
  drag.ghost.style.top = `${Math.min(Math.max(event.clientY, top), top + ya._length)}px`;
  if (Math.abs(event.clientX - drag.x0) > 3 || Math.abs(event.clientY - drag.y0) > 3) drag.moved = true;
}

document.addEventListener('mousemove', (event) => { if (state.drag) moveGhost(event); });

document.addEventListener('mouseup', (event) => {
  const drag = state.drag;
  if (!drag) return;
  state.drag = null;
  drag.ghost.remove();
  document.body.classList.remove('dragging-point');
  if (drag.moved) {
    const point = clampedDataCoordinates(drag.div, event);
    sendCommand('recoMovePoint', { index: drag.index, dir: drag.dir, x: point.x, y: point.y });
  } else if (state.view.status.reco.acceptPoints) {
    // no movement: a click at that position, as anywhere else on the projection
    const point = dataCoordinates(drag.div, event);
    if (point) sendCommand('recoClick', { dir: drag.dir, x: point.x, y: point.y });
  }
}, true);

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

/** fixed vertical lines of a panel (e.g. the clicked times on the charge vs time panel) */
function vlineShapes(panel) {
  return (panel.vlines || []).map((v) => ({
    type: 'line', xref: 'x', yref: 'paper', x0: v.x, x1: v.x, y0: 0, y1: 1,
    line: { color: v.color, width: v.width || 1 },
  }));
}

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
    // the whole list is replaced: the fixed lines and one cursor line
    // (setting 'shapes[i]' to an object would add a new shape on every mouse move)
    Plotly.relayout(timeDiv, {
      shapes: [...vlineShapes(timePanel), {
        type: 'line', xref: 'x', yref: 'paper', x0: point.x, x1: point.x, y0: 0, y1: 1,
        line: { color: timePanel.cursorLineColor, width: 2 },
      }],
    });
  });
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

let lastEventKey = null;
let wasRecoModeOn = null; // unknown before the first status: set the segment block from the mode

function updateControls(status) {
  $('modeLabel').textContent = status.mode;
  // file names only (several GRAW files are comma separated); the full paths are in the tooltip
  const fileNames = status.file ? status.file.split(',').map((f) => f.split('/').pop()).join(', ') : 'No input.';
  $('fileName').textContent = status.dataSet ? `${status.dataSet} — ${fileNames}` : fileNames;
  $('fileName').title = [status.dataSet && `Data set: ${status.dataSet}`, status.file].filter(Boolean).join('\n');
  $('nEvents').textContent = status.nEvents;
  setValueUnlessEditing('inpEventId', status.eventId);
  $('listBox').hidden = !status.eventList;
  if (status.eventList) {
    // position 0: the displayed event is not on the list (e.g. after "go to event id.")
    setValueUnlessEditing('inpListPosition', status.eventList.position || '');
    $('listSize').textContent = `/ ${status.eventList.size}`;
  }
  setValueUnlessEditing('inpEntry', status.entry);

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

  $('inpEventId').disabled = !enabled.navigate;
  $('inpEntry').disabled = !enabled.navigate;
  $('inpListPosition').disabled = !enabled.navigate;
  $('eventTypeBox').hidden = !enabled.eventType;
  // the segment buttons are shown only while the manual reco mode is on
  if (status.flags.recoMode !== wasRecoModeOn) $('markersBox').open = status.flags.recoMode;
  wasRecoModeOn = status.flags.recoMode;
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
  // a newly loaded event already in the output file: show its saved event type bits
  const eventKey = `${status.file}#${status.entry}#${status.eventId}`;
  if (eventKey !== lastEventKey) {
    lastEventKey = eventKey;
    for (const box of eventTypes.querySelectorAll('input')) {
      box.checked = Boolean(Math.floor((status.savedEventType || 0) / 2 ** Number(box.dataset.bit)) % 2);
    }
  }

  $('btnAddSegment').disabled = !status.reco.addEnabled;
  $('btnFitSegments').disabled = !status.reco.fitEnabled;
  $('btnClearTrack').disabled = !status.reco.clearEnabled;
  $('recoHint').textContent = status.reco.hint || '';
  $('recoHint').hidden = !status.reco.hint;
  $('btnAddSegment').textContent = status.reco.addLabel || 'Add segment';
  $('btnAddSegment').title = status.reco.mode === '3prong'
    ? 'Click to add a track: two points on the track, each set by coordinates on two projections. The vertex is calculated from the track lines'
    : 'Click to add segments end point. All segments share a common vertex - the starting point of the first segment. Each point is set by coordinates on two projections';
  for (const radio of document.querySelectorAll('input[name="recoMode"]')) {
    radio.checked = radio.value === status.reco.mode;
    radio.disabled = !status.flags.recoMode;
  }

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

  // event id. and frame in the top bar: type a value and press Enter; Escape or leaving the box
  // without Enter restores the displayed event
  const gotoBox = (formId, inputId, cmd, key, current) => {
    const input = $(inputId);
    const shown = () => (typeof current === 'function' ? current(state.view?.status) : state.view?.status?.[current]);
    const restore = () => { const v = shown(); if (v !== undefined) input.value = v; };
    $(formId).onsubmit = (e) => {
      e.preventDefault();
      const value = input.value.trim();
      input.blur();
      if (value === '' || Number(value) === shown()) { restore(); return; }
      // a refused value (e.g. out of range) is replaced by the displayed event again
      if (confirmClickedTrack('leave')) sendCommand(cmd, { [key]: Number(value) }).then(restore);
      else restore();
    };
    input.onkeydown = (e) => { if (e.key === 'Escape') { restore(); input.blur(); } };
    input.onblur = () => setTimeout(() => { if (document.activeElement !== input && state.pending === 0) restore(); }, 0);
    input.onfocus = () => input.select();
  };
  gotoBox('formGotoEvent', 'inpEventId', 'gotoEvent', 'eventId', 'eventId');
  gotoBox('formGotoEntry', 'inpEntry', 'gotoEntry', 'entry', 'entry');
  gotoBox('listBox', 'inpListPosition', 'gotoListPosition', 'position',
          (status) => (status?.eventList ? status.eventList.position || '' : undefined));
  $('formRunConditions').onsubmit = (e) => {
    e.preventDefault();
    sendCommand('runConditions', {
      driftVelocity: Number($('inpDriftVelocity').value),
      samplingRate: Number($('inpSamplingRate').value),
      triggerDelay: Number($('inpTriggerDelay').value),
    });
  };

  $('btnAddSegment').onclick = () => sendCommand('recoAddSegment');
  for (const radio of document.querySelectorAll('input[name="recoMode"]')) {
    radio.onchange = () => {
      const reco = state.view?.status?.reco;
      if (reco?.pendingPoints && !confirm(`${PENDING_POINTS} Changing the mode clears them. Change anyway?`)) {
        radio.checked = false;
        document.querySelector(`input[name="recoMode"][value="${reco.mode}"]`).checked = true;
        return;
      }
      sendCommand('recoMode', { mode: radio.value });
    };
  }
  $('btnFitSegments').onclick = () => sendCommand('recoFit');
  $('btnClearTrack').onclick = () => sendCommand('recoClearTrack');

  installOpenDialog();
}

// ---------------------------------------------------------------- open file dialog

function installOpenDialog() {
  const dialog = $('openDialog');
  let selected = null; // { kind: 'run' | 'list', value, item }

  const select = (item, kind, value) => {
    dialog.querySelectorAll('.ds-list li.selected').forEach((li) => li.classList.remove('selected'));
    item.classList.add('selected');
    selected = { kind, value };
    $('btnOpenConfirm').disabled = false;
  };
  const entry = (kind, value, title, detail, tooltip) => {
    const item = document.createElement('li');
    const name = document.createElement('span');
    name.className = 'ds-name';
    name.textContent = title;
    const info = document.createElement('span');
    info.className = 'ds-info';
    info.textContent = detail;
    item.append(name, info);
    item.title = tooltip || '';
    if (value) {
      item.onclick = () => select(item, kind, value);
      item.ondblclick = () => { select(item, kind, value); dialog.close('open'); };
    } else item.classList.add('ds-error');
    return item;
  };

  const showDataSets = async () => {
    let sets;
    try {
      sets = await getJSON('api/datasets');
    } catch (error) {
      showMessage(error.message);
      return false;
    }
    selected = null;
    $('btnOpenConfirm').disabled = true;
    const status = state.view?.status || {};

    // runs grouped (by campaign or directory) in collapsible blocks; the block of the open run is open
    const grouped = (container, items, makeItem, emptyText) => {
      container.replaceChildren();
      const blocks = new Map();
      for (const set of items) {
        let block = blocks.get(set.groupKey);
        if (!block) {
          block = document.createElement('details');
          const summary = document.createElement('summary');
          summary.textContent = set.groupLabel;
          const list = document.createElement('ul');
          list.className = 'ds-list';
          block.append(summary, list);
          container.appendChild(block);
          blocks.set(set.groupKey, block);
        }
        const item = makeItem(set);
        if (item.classList.contains('current')) block.open = true;
        block.querySelector('ul').appendChild(item);
        block.querySelector('summary').dataset.count = `${block.querySelectorAll('li').length}`;
      }
      if (!blocks.size) container.textContent = emptyText;
    };
    const numbers = (value, unit) => (value !== '' && value !== undefined ? `${Number(value)} ${unit}` : '');

    // event lists (data-set JSON files)
    $('dataSetDirs').textContent = sets.dataSetDirs || '(web.dataSetDirs not set)';
    const lists = $('eventListSets');
    lists.replaceChildren();
    for (const set of sets.eventLists || []) {
      const detail = set.error ? `error: ${set.error}`
        : [`${set.events} events`, numbers(set.pressure, 'mbar'), numbers(set.energy, 'MeV'), set.geometry]
          .filter(Boolean).join(' · ');
      const item = entry('list', set.error ? null : set.path, set.name, detail, [set.description, set.path].filter(Boolean).join('\n'));
      if (set.name === status.dataSet) item.classList.add('current');
      lists.appendChild(item);
    }
    if (!lists.children.length) {
      const item = document.createElement('li');
      item.className = 'ds-empty';
      item.textContent = 'No event lists.';
      lists.appendChild(item);
    }

    // run data sets (JSON: data files with their settings), grouped by campaign
    grouped($('runDataSets'), (sets.runSets || []).map((set) => ({ ...set, groupKey: set.group, groupLabel: set.group })),
      (set) => {
        const detail = set.error ? set.error
          : [set.quality !== 'good' && set.quality, set.chunks && `${set.chunks} chunks`, numbers(set.pressure, 'mbar'), set.geometry]
            .filter(Boolean).join(' · ');
        const item = entry('list', set.error ? null : set.path, set.name, detail, [set.description, set.path].filter(Boolean).join('\n'));
        if (set.quality && set.quality !== 'good') item.classList.add('ds-bad');
        if (set.name === status.dataSet) item.classList.add('current');
        return item;
      }, 'No run data sets.');

    // other GRAW runs (no data set): the current settings are kept
    $('runDirs').textContent = (sets.dataDirs || []).join(', ');
    const currentFiles = new Set((status.file || '').split(','));
    grouped($('runSets'), (sets.runs || []).map((run) => ({ ...run, groupKey: `${run.dataDir}/${run.group}`, groupLabel: run.group })),
      (run) => {
        const item = entry('run', run.files, run.start,
          `${run.chunks} chunk${run.chunks === 1 ? '' : 's'} · ${run.streams === 1 ? 'one file (all AsAd)' : `${run.streams} AsAd files`}`,
          run.files.split(',').join('\n'));
        if (!status.dataSet && run.files.split(',').some((f) => currentFiles.has(f))) item.classList.add('current');
        return item;
      }, 'No other GRAW runs.');
    return true;
  };

  $('btnOpen').onclick = async () => {
    if (!confirmClickedTrack('leave')) return;
    if (await showDataSets()) {
      dialog.showModal();
      dialog.querySelector('li.current')?.scrollIntoView({ block: 'center' });
    }
  };
  dialog.onclose = () => {
    if (dialog.returnValue !== 'open' || !selected) return;
    if (selected.kind === 'run') sendCommand('openRun', { files: selected.value });
    else sendCommand('openDataSet', { path: selected.value });
  };
}

// ---------------------------------------------------------------- collapsible side blocks

// the open/closed state of each block is remembered by the browser
function installCollapsibles() {
  const key = (box) => `tpcwebgui.open.${box.id}`;
  for (const box of document.querySelectorAll('details.collapsible')) {
    try {
      const saved = localStorage.getItem(key(box));
      if (saved !== null) box.open = saved === '1';
    } catch (e) { /* storage unavailable: keep the default */ }
    box.addEventListener('toggle', () => {
      try { localStorage.setItem(key(box), box.open ? '1' : '0'); } catch (e) { /* ignore */ }
    });
  }
}

// ---------------------------------------------------------------- start

// the access token is kept in a cookie: remove it from the address bar (screenshots, shared links)
if (new URLSearchParams(location.search).has('token')) history.replaceState(null, '', location.pathname);

installCollapsibles();
installControls();
refreshView();
connectWebSocket();

// ---------------------------------------------------------------- exit

async function exitApplication() {
  if (!confirmClickedTrack('leave')) return;
  if (!confirm('Stop the tpcWebGUI server? The output file is closed, for all connected browsers.')) return;
  await state.queue; // let queued commands finish first
  state.stopped = true;
  try {
    await fetch('api/command', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ cmd: 'exit' }),
    });
  } catch (error) { /* server already gone */ }
  $('stopped').hidden = false;
}
