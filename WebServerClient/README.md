# WebServerClient – web based GUI (`tpcWebGUI`)

`tpcWebGUI` has the functionality of `tpcGUI`, displayed in a web browser.
It reads the same configuration files and command line options.

```bash
cd build_09_2026/bin
./tpcWebGUI --meta.configJson ../config/config_GUI__OFFLINE_190mbar_3332Vdrift_25MHz_NGRAW.json \
            --input.dataFile <comma separated GRAW or ROOT files> --display.develMode true
# then open http://localhost:8080/ in a browser; stop the server with Ctrl+C (or kill -TERM)
```

The server accepts remote connections. At start-up it prints the URL to open, with an access token:

```
tpcWebGUI: open http://hepzc62:8080/?token=8d799db55a8afff2b638bd2a7ab63d38 in a web browser.
```

* Open that URL from any computer that can reach the machine. The browser keeps the token in a
  cookie, and the token is removed from the address bar after the page loads.
* Requests without the token (pages, API, WebSocket) get HTTP 401.
* A new random token is generated at each start; set `--web.accessToken <token>` to keep a fixed one.
* Traffic is plain HTTP: on an untrusted network the token can be sniffed. There, run with
  `--web.bindAddress 127.0.0.1` (no token) and use an ssh tunnel instead:
  `ssh -L 8080:localhost:8080 user@analysis-machine`, then open http://localhost:8080/.
* The port must be open in the machine's firewall.

## Options (group `web`)

| option | default | meaning |
|---|---|---|
| `web.port` | 8080 | TCP port |
| `web.bindAddress` | 0.0.0.0 | listening address; `0.0.0.0`: all interfaces (remote access); `127.0.0.1`: this machine only, no token |
| `web.accessToken` | (empty) | token required from browsers; empty: random token generated when remote access is on |
| `web.documentRoot` | (empty) | directory with the client files; empty: `<install prefix>/web` |
| `web.urlPrefix` | (empty) | URL path of the GUI, e.g. `/tpcgui` behind a reverse proxy; the URL to open is then `http://host:port/tpcgui/` |

All other options (`input.*`, `display.*`, `online.*`, `output.*`, ...) are the same as for `tpcGUI`.

## Architecture

```
browser (web/index.html, app.js, Plotly.js)
   │  HTTP + JSON:  GET /api/view, POST /api/command, GET /api/ls
   │  WebSocket /ws: "update" pushed when the displayed event changes
   ▼
WebGUIServer      Crow web framework (header-only, Boost.Asio), routes and static files
   │  one mutex serialises all calls
   ▼
WebGUIController  tpcGUI logic without widgets (MainFrame + HistoManager equivalent):
                  event source, FileOutput, event log, run conditions, ONLINE rate,
                  conversion of the event histograms and tracks to JSON plot panels
   │
ManualRecoTool    state machine of the clicked track reconstruction (port of MarkersManager)
```

* Server side: [Crow](https://crowcpp.org) v1.3.4, fetched by CMake (`FetchContent`) at configure time,
  used with the Boost.Asio already required by TPCReco. JSON uses Crow's own JSON support.
* Client side: plain HTML/CSS/JavaScript with [Plotly.js](https://plotly.com/javascript/).
  `plotly.min.js` is copied from the python `plotly` package of the container (or downloaded),
  so the GUI works without internet access.
* ONLINE mode: `DirectoryWatch` (Utilities) calls the server through a C++ callback;
  connected browsers are notified over the WebSocket.

## Functionality (as in tpcGUI)

* Next (▶) / Previous (◀) / Reset event. For GRAW input, ▶ on the last event of a chunk opens the
  next chunk of the run (all AsAd files), ◀ on the first event the previous one; the chunk files are
  found on disk (their timestamps differ, e.g. `..44.531_0000.graw` is followed by `..44.540_0001.graw`).
  Other controls: go to event id, go to frame, open file (server side file browser,
  several GRAW files can be selected with Ctrl+click).
* Z log scale, auto zoom, manual reconstruction mode, event rate display (ONLINE).
* Display modes from the configuration: raw (default), `display.develMode` (rec hits, fitted track,
  charge profile with the dE/dx model, track projection on the readout plane, reconstruction
  printout in a terminal style cell), `display.technicalMode` (AGET channels).
* Manual reconstruction: "Add segment", then click a point on one U/V/W projection and the same point
  on a second projection (the time is taken from the first click, the third coordinate is calculated
  from the geometry); the first point is the common vertex. "Fit segments" fits and shows the track
  (projections, charge along the track, projection on the readout plane), "Clear track" hides it. The event is written with the clicked track on "Next event".
  Before leaving an event with a fitted but unsaved clicked track (Previous, go to event/frame,
  open file, switching manual reco off), or with clicked points that were not fitted
  (also on Next and Reset), the GUI asks for confirmation. tpcGUI asks the same questions.
* Event type bits, written to `<input file>.log` and to the output file (manual reco mode) on Next/Previous.
* Run conditions: drift velocity, sampling rate, trigger delay.
* Output file: the same `FileOutput` as tpcGUI, filled once per loaded event (manual reco mode:
  on "Next event"). An event already in the output file (e.g. when going back) is read from it:
  its saved track is shown (in manual reco mode as the saved clicked track; the printout starts
  with "Read from the output file.") together with its saved event type bits, and it is not
  written again unless a new track is fitted or the event type bits are changed.

Differences from tpcGUI:

* All connected browsers share one GUI state (one event source on the server).
* No Exit button: the server is started and stopped from the command line (Ctrl+C or SIGTERM
  closes the output file).
* The ONLINE rate graph gets one point per loaded event (tpcGUI added one per redraw);
  "Reset rate" is usable in ONLINE mode.
* Plots are interactive (zoom, pan, hover values, PNG export from the Plotly toolbar).
* Layout: mode, file name, number of events, event id. and frame are in the top bar. Click the file
  name to open another file; type an event id. or a frame there and press Enter to go to it
  (Escape restores the current one). The side blocks (Event type, Segment creation, Run conditions)
  are collapsed; the +/− button expands
  them and the browser remembers the choice. Segment creation is open only while manual reco is on.

## JSON command API

`POST /api/command` with one of:

```
{"cmd":"next", "eventType": <bits>}       {"cmd":"previous", "eventType": <bits>}
{"cmd":"reset"}                            {"cmd":"resetRate"}
{"cmd":"gotoEvent", "eventId": n}          {"cmd":"gotoEntry", "entry": n}
{"cmd":"open", "path": "file1,file2,..."}
{"cmd":"set", "option": "logScale|autoZoom|recoMode|rate", "value": true|false}
{"cmd":"runConditions", "driftVelocity": v, "samplingRate": r, "triggerDelay": d}
{"cmd":"recoAddSegment"}   {"cmd":"recoClick", "dir": 0|1|2, "x": t, "y": s}
{"cmd":"recoFit"}          {"cmd":"recoClearTrack"}
```

The reply is `{"ok": bool, "error": "...", "view": {...}}`, where `view` is the same document as
`GET /api/view`: `status` (file, event, flags, enabled controls) and `panels` (heatmaps/plots with overlays).
