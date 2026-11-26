#ifndef TPCRECO_WEBSERVERCLIENT_WEB_GUI_SERVER_H_
#define TPCRECO_WEBSERVERCLIENT_WEB_GUI_SERVER_H_

#include <memory>
#include <string>

class WebGUIController;

/// HTTP/WebSocket front end of the web GUI, built on the Crow web framework.
///
/// Endpoints:
///  - GET  /                 the web client (index.html)
///  - GET  /static/<file>    client files (app.js, style.css, plotly.min.js)
///  - GET  /logo/<file>      FUW and ELITPC logos from input.resourcesPath
///  - GET  /api/status       JSON: file, event and widget state
///  - GET  /api/view         JSON: status + plot panels of the current event
///  - POST /api/command      JSON command {"cmd": ...}; reply {"ok", "error", "view"}
///  - GET  /api/datasets      JSON: data sets (GRAW runs, event lists) for the "Open" dialog
///  - WS   /ws               server push: "update" when the event shown has changed
///                           (ONLINE mode, or a command sent from another browser)
///
/// Access control: when an access token is set, every request needs it, either as the
/// "token" URL parameter (the first page load: http://host:port/?token=...) or as the
/// cookie set by the server on that first load.
///
/// All controller calls are serialised with a mutex.
class WebGUIServer {

public:

  /// accessToken: empty for no access control
  /// urlPrefix: URL path under which the GUI is served, e.g. "/tpcgui" (empty: "/")
  WebGUIServer(WebGUIController & aController, const std::string & documentRoot,
               const std::string & resourcesPath, const std::string & accessToken="",
               const std::string & urlPrefix="");

  ~WebGUIServer();

  /// Runs the server; returns when a browser sends the "exit" command (Exit button) or on
  /// SIGINT/SIGTERM (Ctrl+C on the command line).
  void run(const std::string & address, int port);

  /// Loads new GRAW files in ONLINE mode (called from the DirectoryWatch thread)
  /// and notifies the connected clients.
  void loadOnlineFiles(const std::string & fileList);

private:

  struct Impl;
  std::unique_ptr<Impl> myImpl;
};

#endif // TPCRECO_WEBSERVERCLIENT_WEB_GUI_SERVER_H_
