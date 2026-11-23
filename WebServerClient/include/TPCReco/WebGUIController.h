#ifndef TPCRECO_WEBSERVERCLIENT_WEB_GUI_CONTROLLER_H_
#define TPCRECO_WEBSERVERCLIENT_WEB_GUI_CONTROLLER_H_

#include <memory>
#include <string>
#include <vector>
#include <utility>

#include <boost/property_tree/ptree.hpp>
#include <crow/json.h>

#include "TPCReco/IonRangeCalculator.h"
#include "TPCReco/ManualRecoTool.h"

class EventSourceBase;
class FileOutput;
class GeometryTPC;
class TH1;
class Track3D;

/// Application logic of the web GUI: the equivalent of tpcGUI's MainFrame + HistoManager
/// without any widgets. The state is turned into JSON documents consumed by the
/// browser client (web/app.js), commands from the client are JSON documents too.
///
/// The class is NOT thread safe: WebGUIServer serialises all calls.
class WebGUIController {

public:

  explicit WebGUIController(const boost::property_tree::ptree & aConfig);

  ~WebGUIController();

  /// File, event, mode and widget state.
  crow::json::wvalue getStatus() const;

  /// Plot panels of the current event (and the 3D view in devel mode).
  crow::json::wvalue getView() const;

  /// Executes a command {"cmd": name, ...}. Returns an empty string on success,
  /// an error message otherwise.
  std::string execute(const crow::json::rvalue & command);

  /// Entries of a server side directory, for the "Open file" dialog.
  crow::json::wvalue listDirectory(const std::string & dirName) const;

  bool isOnline() const;
  std::string getOnlineDirectory() const;
  int getOnlineUpdateInterval() const;

  /// New GRAW files reported by DirectoryWatch in ONLINE mode (comma separated list).
  void loadOnlineFiles(const std::string & fileList);

  /// Closes the output file and the event source (e.g. MC event exporter file).
  void close();

private:

  // modes and capabilities
  bool isMC() const;
  bool isOfflineGraw() const;
  bool canNavigate() const { return !isOnline(); }
  bool canGoBack() const { return !isOnline() && !isMC(); }
  bool canRecoMode() const;
  bool canRate() const { return isOnline(); }
  std::string getModeLabel() const;
  std::string getDisplayMode() const;

  // actions
  void eventLoaded();
  void refresh();
  unsigned long updateEventLog(unsigned long eventType);
  void writeManualReco(unsigned long eventType);
  void updateEventRate();
  std::string setRunConditions(double driftVelocity, double samplingRate, double triggerDelay);
  std::string openFile(const std::string & fileName);

  // panels
  std::vector<crow::json::wvalue> makeRawPanels() const;
  std::vector<crow::json::wvalue> makeRecoPanels() const;
  std::vector<crow::json::wvalue> makeDevelPanels() const;
  std::vector<crow::json::wvalue> makeTechnicalPanels() const;
  struct PanelOverlay;
  crow::json::wvalue makeHeatmapPanel(const TH1 & aHisto, bool zeroIsEmpty, bool withFrameColor,
                                      bool allowAutozoom=true) const;
  crow::json::wvalue makeHist1DPanel(const TH1 & aHisto) const;
  crow::json::wvalue makeRatePanel() const;
  crow::json::wvalue makeXYPanel(const Track3D & aTrack) const;
  crow::json::wvalue makeChargeProfilePanel(const Track3D & aTrack) const;
  crow::json::wvalue makeView3D(const Track3D * aTrack) const;
  void addTrackProjection(PanelOverlay & aOverlay, int strip_dir, const Track3D & aTrack) const;
  void addManualRecoOverlay(PanelOverlay & aOverlay, int strip_dir) const;
  std::pair<int,int> autozoomBins(const TH1 & aHisto, int iAxis) const;

  boost::property_tree::ptree myConfig;
  int myWorkMode{0};
  std::shared_ptr<EventSourceBase> myEventSource;
  std::shared_ptr<FileOutput> myFileOutput;
  mutable IonRangeCalculator myRangeCalculator;
  ManualRecoTool myRecoTool;

  bool isLogScaleOn{false};
  bool isAutozoomOn{false};
  bool isRecoModeOn{false};
  bool isRateDisplayOn{false};
  bool isNewEvent{true};
  bool isManualTrackShown{false};
  bool isManualTrackUnsaved{false}; // fitted clicked track not yet written ("Next event")

  std::vector<std::pair<double,double>> myEventRate; // (time [s], rate [Hz])
  long long previousEventTime{-1}, previousEventNumber{-1};

  static const std::vector<std::string> eventTypeNames;
};

#endif // TPCRECO_WEBSERVERCLIENT_WEB_GUI_CONTROLLER_H_
