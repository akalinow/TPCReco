#ifndef TPCRECO_WEBSERVERCLIENT_WEB_GUI_CONTROLLER_H_
#define TPCRECO_WEBSERVERCLIENT_WEB_GUI_CONTROLLER_H_

#include <memory>
#include <string>
#include <vector>
#include <map>
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

  /// Data sets for the "Open" dialog: data-set JSON files in web.dataSetDirs (event lists, runs:
  /// data with their settings) and the other GRAW runs in web.grawDataDirs.
  crow::json::wvalue listDataSets() const;

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
  /// devel mode display: readout plane projection and reconstruction printout
  bool isDevelView() const { return myConfig.get<bool>("display.develMode", false) && !isOnline(); }
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
  /// true for the last entry of the current file
  bool isLastEntry() const;
  /// comma separated files of the previous (-1) / next (+1) chunk of the current GRAW run,
  /// empty when not reading GRAW files or the chunk does not exist for every stream
  std::string neighbourChunkFiles(int chunkOffset) const;

  /// event list (input.eventList): one event per line "runId , entry , eventId"
  struct ListedEvent { long runId; unsigned long entry; unsigned long eventId; };
  static std::vector<ListedEvent> readEventList(const std::string & fileName);
  /// opens the files of a run (comma separated) and leaves the event list
  std::string openRun(const std::string & files);
  /// loads a data-set JSON: an event list (input.eventList, input.eventListDataPath) or a run
  /// (input.dataFile), with its settings (input.geometryFile, conditions, pedestal, hitFilter,
  /// beamParameters); the event source is created again with them. The current state is kept
  /// when the data set cannot be loaded.
  std::string openDataSet(const std::string & jsonPath);
  /// work mode from the transient settings of the event source factory
  void setWorkMode();
  /// error message when the number of GRAW files does not fit the reader (one file per AsAd or
  /// one file for all), empty otherwise
  std::string checkGrawFileCount(const std::string & files) const;
  /// new event source for aConfig (input.dataFile set), replacing the current one on success
  std::string replaceEventSource(boost::property_tree::ptree & aConfig);
  /// loads event myEventList[position], opening the GRAW chunk of its run when needed;
  /// returns an error message (empty on success)
  std::string loadListedEvent(long position);
  /// number of frames of the chunks of a run (from its first chunk, cached)
  std::size_t framesPerChunk(long runId);
  /// chunk number of the files read now (-1 if unknown)
  int currentChunk() const;

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
  /// track to display: the one read from the output file, the fitted clicked track (manual reco),
  /// or the one of the TrackBuilder
  const Track3D & displayedTrack() const;
  /// clicked track with the particle types of the manual reconstruction mode: segments sorted
  /// by length; 2-prong: alpha (longer) + carbon-12, 3-prong: all alphas
  static Track3D makeClickedTrack(const Track3D & aTrack, ManualRecoTool::Mode aMode);
  /// looks the current event up in the output file (mySavedTrack, mySavedEventType)
  void readSavedRecoEvent();
  /// track reconstruction printout (terminal text with ANSI colour codes)
  std::string makeRecoText(const Track3D * aTrack) const;
  void addTrackProjection(PanelOverlay & aOverlay, int strip_dir, const Track3D & aTrack) const;
  void addManualRecoOverlay(PanelOverlay & aOverlay, int strip_dir) const;
  std::pair<int,int> autozoomBins(const TH1 & aHisto, int iAxis) const;

  boost::property_tree::ptree myConfig;
  int myWorkMode{0};
  std::shared_ptr<EventSourceBase> myEventSource;
  std::shared_ptr<FileOutput> myFileOutput;
  mutable IonRangeCalculator myRangeCalculator;
  std::unique_ptr<ManualRecoTool> myRecoTool{ManualRecoTool::create(ManualRecoTool::Mode::TwoProng)};

  bool isLogScaleOn{false};
  bool isAutozoomOn{false};
  bool isRecoModeOn{false};
  bool isRateDisplayOn{false};
  bool isNewEvent{true};
  bool isManualTrackShown{false};
  bool isManualTrackUnsaved{false}; // fitted clicked track not yet written ("Next event")

  // reconstruction of the current event read back from the output file (event already
  // written, e.g. when going back): shown instead of a new reconstruction, not written again
  std::shared_ptr<Track3D> mySavedTrack;
  // clicked track of the current event after "Fit segments", with the particle types of the
  // reconstruction mode (makeClickedTrack); shown and written instead of the TrackBuilder track
  std::shared_ptr<Track3D> myClickedTrack;

  std::vector<ListedEvent> myEventList;
  long myListPosition{-1};          // index in myEventList of the event shown, -1: none
  std::string myEventListDataDir;   // directory with the run directories (GRAW files)
  std::map<long, std::size_t> myFramesPerChunk;
  std::string myDataSetName;        // name of the data set opened from the "Open" dialog
  boost::property_tree::ptree myStartConfig; // configuration at start-up: data sets are applied on it
  unsigned long mySavedEventType{0};

  std::vector<std::pair<double,double>> myEventRate; // (time [s], rate [Hz])
  long long previousEventTime{-1}, previousEventNumber{-1};

  static const std::vector<std::string> eventTypeNames;
};

#endif // TPCRECO_WEBSERVERCLIENT_WEB_GUI_CONTROLLER_H_
