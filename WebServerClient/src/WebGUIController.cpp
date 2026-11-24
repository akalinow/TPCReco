#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <tuple>

#include <TF1.h>
#include <TGraph.h>
#include <TH1.h>
#include <TH1F.h>
#include <TH2D.h>
#include <TVector3.h>

#include "TPCReco/CommonDefinitions.h"
#include "TPCReco/EventSourceFactory.h"
#include "TPCReco/EventSourceGRAW.h"
#include "TPCReco/EventSourceMultiGRAW.h"
#include "TPCReco/GrawChunkPath.h"
#include "TPCReco/FileOutput.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/GUI_commons.h"
#include "TPCReco/Track3D.h"
#include "TPCReco/TrackBuilder.h"
#include "TPCReco/colorText.h"

#include "TPCReco/WebGUIController.h"

using crow::json::wvalue;
using definitions::projection_type;
namespace fs = std::filesystem;

namespace {

  /// JSON list from a vector of values (crow::json::wvalue has no list move constructor)
  wvalue toValue(std::vector<wvalue> && aList){
    wvalue aValue;
    aValue = std::move(aList);
    return aValue;
  }

  // bulk numbers are sent as float: crow writes them with "%f" (trailing zeros trimmed)
  // instead of 17 significant digits used for double
  wvalue num(double value) { return wvalue(static_cast<float>(value)); }

  wvalue numList(const std::vector<double> & values){
    std::vector<wvalue> aList;
    aList.reserve(values.size());
    for(auto value: values) aList.push_back(std::isfinite(value) ? num(value) : wvalue());
    return toValue(std::move(aList));
  }

  wvalue range(double min, double max){
    std::vector<wvalue> aList;
    aList.push_back(num(min));
    aList.push_back(num(max));
    return toValue(std::move(aList));
  }

  /// CSS colour of a ROOT colour index (only the indices used by the GUI)
  std::string rootColor(int index){
    static const std::vector<std::string> colors = {
      "#ffffff", "#000000", "#ff0000", "#00cc00", "#0000ff", "#ffff00", "#ff00ff", "#00ffff",
      "#59d454", "#5954d8"};
    if(index>=0 && index<int(colors.size())) return colors[index];
    return colors[2+(index%8)];
  }

  // ROOT kAzure-6: frame colour of the charge projections in tpcGUI
  const std::string frameColor = "#00339a";

  wvalue line(const std::vector<double> & x, const std::vector<double> & y,
              const std::string & color, double width, const std::string & dash="solid",
              const std::string & name=""){
    wvalue aLine;
    aLine["x"] = numList(x);
    aLine["y"] = numList(y);
    aLine["color"] = color;
    aLine["width"] = width;
    aLine["dash"] = dash;
    if(name.size()) aLine["name"] = name;
    return aLine;
  }

  wvalue markers(const std::vector<double> & x, const std::vector<double> & y,
                 const std::string & color, const std::string & symbol="circle",
                 double size=8, const std::string & lineColor=""){
    wvalue aMarkers;
    aMarkers["x"] = numList(x);
    aMarkers["y"] = numList(y);
    aMarkers["color"] = color;
    aMarkers["symbol"] = symbol;
    aMarkers["size"] = size;
    aMarkers["lineColor"] = lineColor.size() ? lineColor : color;
    return aMarkers;
  }

  std::string format(const char *fmt, double value){
    char buffer[128];
    snprintf(buffer, sizeof(buffer), fmt, value);
    return buffer;
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/// Items drawn on top of a panel; collected in vectors and moved to the panel JSON once.
struct WebGUIController::PanelOverlay {
  std::vector<wvalue> lines, markers, vlines, series, texts;
  void moveTo(wvalue & aPanel){
    if(lines.size()) aPanel["lines"] = std::move(lines);
    if(markers.size()) aPanel["markers"] = std::move(markers);
    if(vlines.size()) aPanel["vlines"] = std::move(vlines);
    if(series.size()) aPanel["series"] = std::move(series);
    if(texts.size()) aPanel["texts"] = std::move(texts);
  }
};
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const std::vector<std::string> WebGUIController::eventTypeNames = {
  "Noise", "Multi-vertex", "Fractured track", "Pretty event", "Weird event",
  "Spare cat. 1", "Spare cat. 2", "Spare cat. 3"};
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
WebGUIController::WebGUIController(const boost::property_tree::ptree & aConfig){

  myConfig = aConfig;
  myFileOutput = std::make_shared<FileOutput>(myConfig);
  myEventSource = EventSourceFactory::makeEventSourceObject(myConfig);

  event_type eventSourceType = myConfig.get<event_type>("transient.eventType");
  bool onlineFlag = myConfig.get<bool>("transient.onlineFlag");
  if (eventSourceType == event_type::EventSourceROOT) myWorkMode = M_OFFLINE_ROOT_MODE;
  else if (eventSourceType == event_type::EventSourceMC) myWorkMode = M_OFFLINE_MC_MODE;
  else if (eventSourceType == event_type::EventSourceGRAW) {
    myWorkMode = (onlineFlag ? M_ONLINE_GRAW_MODE : M_OFFLINE_GRAW_MODE);
  }
  else if (eventSourceType == event_type::EventSourceMultiGRAW) {
    myWorkMode = (onlineFlag ? M_ONLINE_NGRAW_MODE : M_OFFLINE_NGRAW_MODE);
  }
  else throw std::runtime_error("Unknown event source type");

  myEventSource->loadFileEntry(0);
  auto aEvent = myEventSource->getCurrentEvent();
  if(aEvent && aEvent->GetEventInfo().GetPedestalSubtracted()){
    aEvent->setHitFilterConfig(filter_type::threshold, myConfig);
  }

  myRangeCalculator.setGasMixture(gas_mixture_type::CO2);
  myRangeCalculator.setGasPressure(myConfig.get<double>("conditions.pressure"));
  myRangeCalculator.setGasTemperature(myConfig.get<double>("conditions.temperature"));
  myRecoTool.setGeometry(myEventSource->getGeometry());

  isLogScaleOn = myConfig.get<bool>("display.zLogScale", false);
  isAutozoomOn = myConfig.get<bool>("display.autoZoom", false);
  isRecoModeOn = myConfig.get<bool>("display.recoMode", false) && canRecoMode();
  isRateDisplayOn = myConfig.get<bool>("display.rate", false) && canRate();

  eventLoaded();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
WebGUIController::~WebGUIController(){ close(); }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void WebGUIController::close(){

  myFileOutput.reset();
  myEventSource.reset();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool WebGUIController::isOnline() const {
  return myWorkMode==M_ONLINE_GRAW_MODE || myWorkMode==M_ONLINE_NGRAW_MODE;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool WebGUIController::isMC() const { return myWorkMode==M_OFFLINE_MC_MODE; }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool WebGUIController::isOfflineGraw() const {
  return myWorkMode==M_OFFLINE_GRAW_MODE || myWorkMode==M_OFFLINE_NGRAW_MODE;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool WebGUIController::canRecoMode() const {
  // as in tpcGUI: disabled ONLINE, and OFFLINE from GRAW without pedestal subtraction
  return !isOnline() && !(isOfflineGraw() && !myConfig.get<bool>("pedestal.remove"));
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string WebGUIController::getOnlineDirectory() const {
  return myConfig.get<std::string>("input.dataFile");
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int WebGUIController::getOnlineUpdateInterval() const {
  return myConfig.get<int>("online.updateInterval");
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string WebGUIController::getModeLabel() const {
  if(isOnline()) return "ONLINE";
  if(myWorkMode==M_OFFLINE_ROOT_MODE) return "OFFLINE from ROOT";
  if(isMC()) return "OFFLINE from Monte Carlo";
  if(isOfflineGraw()) return "OFFLINE from GRAW";
  return "NONE";
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string WebGUIController::getDisplayMode() const {
  if(isRecoModeOn) return "reco";
  if(myConfig.get<bool>("display.develMode", false)) return "devel";
  if(myConfig.get<bool>("display.technicalMode", false)) return "technical";
  return "raw";
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void WebGUIController::eventLoaded(){

  isNewEvent = true;
  isManualTrackUnsaved = false;
  refresh();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void WebGUIController::refresh(){

  // equivalent of MainFrame::Update() without the drawing, which is done on demand by getView()
  myRecoTool.reset(isRecoModeOn);
  isManualTrackShown = false;
  if(!myEventSource || !myEventSource->numberOfEvents()) return;
  if(isNewEvent){
    if(isOnline()) updateEventRate();
    // automatic output once per loaded event; in manual RECO mode the event
    // is written on "Next event" with the clicked track instead
    if(myFileOutput && !isRecoModeOn) myFileOutput->update(myEventSource);
  }
  isNewEvent = false;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void WebGUIController::updateEventRate(){

  const auto & aInfo = myEventSource->getCurrentEvent()->GetEventInfo();
  long long currentEventTime = aInfo.GetEventTimestamp()*1E-8; // [s]
  long long currentEventNumber = aInfo.GetEventId();
  if(previousEventTime<0 || previousEventNumber>currentEventNumber){
    previousEventTime = currentEventTime;
    previousEventNumber = currentEventNumber;
  }
  long long deltaTime = currentEventTime - previousEventTime;
  long long deltaEventCount = currentEventNumber - previousEventNumber;
  double rate = deltaTime ? double(deltaEventCount)/deltaTime : 0.0; // [Hz]
  previousEventTime = currentEventTime;
  previousEventNumber = currentEventNumber;
  myEventRate.emplace_back(currentEventTime, rate);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
unsigned long WebGUIController::updateEventLog(unsigned long eventType){

  // same file and format as tpcGUI: <input file name>.log in the working directory
  std::string path = myEventSource->getCurrentPath();
  std::string logFileName = path.substr(path.find_last_of('/')+1) + ".log";
  bool isNewFile = !fs::exists(logFileName);
  std::ofstream out(logFileName, std::ofstream::app);
  if(isNewFile){
    out << "Event type bits:" << std::endl;
    for(size_t iBit=0;iBit<eventTypeNames.size();++iBit){
      out << iBit << " - " << eventTypeNames[iBit] << std::endl;
    }
    out << "\n";
    out << "Event Id \t entry number \t Event type" << std::endl;
  }
  out << myEventSource->currentEventId() << " \t\t "
      << myEventSource->currentEntryNumber() << " \t\t "
      << eventType << std::endl;
  return eventType;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void WebGUIController::writeManualReco(unsigned long eventType){

  if(!myFileOutput) return;
  eventraw::EventInfo aEventInfo = myEventSource->getCurrentEvent()->GetEventInfo();
  aEventInfo.SetEventType(eventType);
  myFileOutput->update(myEventSource, aEventInfo, myEventSource->getTrackBuilder()->getTrack3D(0));
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool WebGUIController::isLastEntry() const {

  const auto nEntries = myEventSource->numberOfEvents();
  return nEntries>0 && myEventSource->currentEntryNumber()+1>=nEntries;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string WebGUIController::neighbourChunkFiles(int chunkOffset) const {

  std::vector<std::string> currentFiles;
  if(auto aSource = std::dynamic_pointer_cast<EventSourceMultiGRAW>(myEventSource)){
    currentFiles = aSource->getFilePathList();
  }
  else if(auto aSource = std::dynamic_pointer_cast<EventSourceGRAW>(myEventSource)){
    currentFiles.push_back(aSource->getFilePath());
  }
  std::string fileList;
  for(const auto & aFile: currentFiles){
    std::string aChunk = findGrawChunkPath(aFile, chunkOffset);
    if(aChunk.empty()) return ""; // all streams must have the chunk
    fileList += (fileList.empty() ? "" : ",") + aChunk;
  }
  return fileList;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string WebGUIController::openFile(const std::string & fileName){

  if(!canNavigate()) return "Opening files is disabled in ONLINE mode.";
  if(isMC()) return "Opening files is not possible in Monte Carlo mode.";
  std::stringstream aStream(fileName);
  std::string item;
  while(std::getline(aStream, item, ',')){
    if(!fs::is_regular_file(item)) return "No such file: "+item;
  }
  myEventSource->loadDataFile(fileName);
  myEventSource->loadFileEntry(0);
  eventLoaded();
  return "";
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string WebGUIController::setRunConditions(double driftVelocity, double samplingRate,
                                               double triggerDelay){

  auto aGeometry = myEventSource->getGeometry();
  if(!aGeometry) return "Geometry not available.";
  if(driftVelocity<=0 || samplingRate<=0) return "Drift velocity and sampling rate must be positive.";
  bool isGeometryChanged = aGeometry->GetDriftVelocity()!=driftVelocity ||
                           aGeometry->GetSamplingRate()!=samplingRate ||
                           aGeometry->GetTriggerDelay()!=triggerDelay;
  aGeometry->setDriftVelocity(driftVelocity);
  aGeometry->setSamplingRate(samplingRate);
  aGeometry->setTriggerDelay(triggerDelay);
  std::cout << aGeometry->getRunConditions() << std::endl;
  if(isGeometryChanged) refresh();
  return "";
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void WebGUIController::loadOnlineFiles(const std::string & fileList){

  myEventSource->loadDataFile(fileList);
  myEventSource->getLastEvent();
  eventLoaded();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string WebGUIController::execute(const crow::json::rvalue & command){

  if(!command.has("cmd")) return "Missing command name.";
  const std::string cmd = command["cmd"].s();
  auto eventType = [&command](){
    return command.has("eventType") ? static_cast<unsigned long>(command["eventType"].i()) : 0ul;
  };

  if(cmd=="next"){
    if(!canNavigate()) return "Event navigation is disabled in ONLINE mode.";
    unsigned long aEventType = updateEventLog(eventType());
    if(isRecoModeOn) writeManualReco(aEventType);
    // last event of a GRAW chunk: continue with the next chunk of the run
    std::string nextChunk;
    if(isLastEntry()) nextChunk = neighbourChunkFiles(+1);
    if(!nextChunk.empty()){
      myEventSource->loadDataFile(nextChunk);
      myEventSource->loadFileEntry(0);
    }
    else myEventSource->getNextEventLoop();
    eventLoaded();
  }
  else if(cmd=="previous"){
    if(!canGoBack()) return "Previous event is not available in this mode.";
    updateEventLog(eventType());
    // first event of a GRAW chunk: continue with the last event of the previous chunk
    std::string previousChunk;
    if(myEventSource->currentEntryNumber()==0) previousChunk = neighbourChunkFiles(-1);
    if(!previousChunk.empty()){
      myEventSource->loadDataFile(previousChunk);
      myEventSource->loadFileEntry(myEventSource->numberOfEvents()-1);
    }
    else myEventSource->getPreviousEventLoop();
    eventLoaded();
  }
  else if(cmd=="reset") refresh();
  else if(cmd=="gotoEvent"){
    if(!canNavigate()) return "Event navigation is disabled in ONLINE mode.";
    long int eventId = command["eventId"].i();
    if(eventId<0) return "Event id must be non-negative.";
    myEventSource->loadEventId(eventId);
    eventLoaded();
  }
  else if(cmd=="gotoEntry"){
    if(!canNavigate()) return "Event navigation is disabled in ONLINE mode.";
    long int entry = command["entry"].i();
    if(entry<0) return "Frame number must be non-negative.";
    myEventSource->loadFileEntry(entry);
    eventLoaded();
  }
  else if(cmd=="open") return openFile(command["path"].s());
  else if(cmd=="set"){
    const std::string option = command["option"].s();
    bool value = command["value"].b();
    if(option=="logScale") isLogScaleOn = value;
    else if(option=="autoZoom") isAutozoomOn = value;
    else if(option=="recoMode"){
      if(value && !canRecoMode()) return "Manual reconstruction is not available in this mode.";
      isRecoModeOn = value;
    }
    else if(option=="rate"){
      if(value && !canRate()) return "The rate display is available in ONLINE mode only.";
      isRateDisplayOn = value;
    }
    else return "Unknown option: "+option;
    refresh();
  }
  else if(cmd=="resetRate"){
    myEventRate.clear();
    refresh();
  }
  else if(cmd=="runConditions"){
    if(isOnline()) return "Run conditions are not used in ONLINE mode.";
    return setRunConditions(command["driftVelocity"].d(),
                            command["samplingRate"].d(),
                            command["triggerDelay"].d());
  }
  else if(cmd=="recoAddSegment"){
    if(!isRecoModeOn || !myRecoTool.isAddEnabled()) return "Adding a segment is not possible now.";
    myRecoTool.addSegment();
  }
  else if(cmd=="recoClick"){
    if(!isRecoModeOn) return "Manual reconstruction mode is off.";
    myRecoTool.click(command["dir"].i(), command["x"].d(), command["y"].d());
  }
  else if(cmd=="recoFit"){
    if(!isRecoModeOn || !myRecoTool.isFitEnabled()) return "No complete segments to fit.";
    std::vector<double> segmentsXY = myRecoTool.takeSegmentsForFit();
    myEventSource->getTrackBuilder()->getSegment2DCollectionFromGUI(segmentsXY);
    isManualTrackShown = true;
    isManualTrackUnsaved = true;
  }
  else if(cmd=="recoClearTrack") isManualTrackShown = false;
  else return "Unknown command: "+cmd;
  return "";
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
crow::json::wvalue WebGUIController::getStatus() const {

  wvalue status;
  status["mode"] = getModeLabel();
  status["online"] = isOnline();
  status["mc"] = isMC();
  status["display"] = getDisplayMode();
  status["file"] = myEventSource ? myEventSource->getCurrentPath() : std::string("");
  status["nEvents"] = myEventSource ? myEventSource->numberOfEvents() : 0ul;
  status["eventId"] = myEventSource ? myEventSource->currentEventId() : 0ul;
  status["entry"] = myEventSource ? myEventSource->currentEntryNumber() : 0ul;

  status["flags"]["logScale"] = isLogScaleOn;
  status["flags"]["autoZoom"] = isAutozoomOn;
  status["flags"]["recoMode"] = isRecoModeOn;
  status["flags"]["rate"] = isRateDisplayOn;

  auto aGeometry = myEventSource ? myEventSource->getGeometry() : nullptr;
  status["enabled"]["navigate"] = canNavigate();
  status["enabled"]["previous"] = canGoBack();
  status["enabled"]["open"] = canNavigate() && !isMC();
  status["enabled"]["recoMode"] = canRecoMode();
  status["enabled"]["rate"] = canRate();
  status["enabled"]["resetRate"] = isOnline();
  status["enabled"]["eventType"] = !isOnline();
  status["enabled"]["markers"] = !isOnline();
  status["enabled"]["runConditions"] = !isOnline() && aGeometry!=nullptr;

  std::vector<wvalue> names;
  for(const auto & item: eventTypeNames) names.emplace_back(item);
  status["eventTypeNames"] = std::move(names);

  if(aGeometry){
    status["runConditions"]["driftVelocity"] = aGeometry->GetDriftVelocity();
    status["runConditions"]["samplingRate"] = aGeometry->GetSamplingRate();
    status["runConditions"]["triggerDelay"] = aGeometry->GetTriggerDelay();
  }

  status["reco"]["acceptPoints"] = myRecoTool.isAcceptingPoints();
  status["reco"]["addEnabled"] = isRecoModeOn && myRecoTool.isAddEnabled();
  status["reco"]["fitEnabled"] = isRecoModeOn && myRecoTool.isFitEnabled();
  status["reco"]["clearEnabled"] = isRecoModeOn && myRecoTool.isClearEnabled();
  // for the warnings before leaving an event: a fitted clicked track is written only by "Next event"
  status["reco"]["unsavedTrack"] = isRecoModeOn && isManualTrackUnsaved;
  status["reco"]["pendingPoints"] = isRecoModeOn && myRecoTool.hasClickedPoints();
  return status;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
crow::json::wvalue WebGUIController::getView() const {

  wvalue view;
  view["status"] = getStatus();
  if(!myEventSource || !myEventSource->numberOfEvents()){
    view["panels"] = std::vector<wvalue>{};
    return view;
  }
  std::string displayMode = getDisplayMode();
  if(displayMode=="reco") view["panels"] = makeRecoPanels();
  else if(displayMode=="devel") view["panels"] = makeDevelPanels();
  else if(displayMode=="technical") view["panels"] = makeTechnicalPanels();
  else view["panels"] = makeRawPanels();

  // reconstruction printout: devel mode; track of the automatic reconstruction,
  // or the manually fitted track in RECO mode
  if(isDevelView()){
    const Track3D * aTrack = nullptr;
    if(!isRecoModeOn || isManualTrackShown) aTrack = &myEventSource->getTrackBuilder()->getTrack3D(0);
    view["recoText"] = makeRecoText(aTrack);
  }
  return view;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::pair<int,int> WebGUIController::autozoomBins(const TH1 & aHisto, int iAxis) const {

  // same algorithm as HistoManager::makeAutozoom(), without modifying the histogram
  const TAxis * aAxis = iAxis==1 ? aHisto.GetXaxis() : aHisto.GetYaxis();
  int nBins = aAxis->GetNbins();
  double threshold = 0.1*aHisto.GetMaximum();
  int lowBin = aHisto.FindFirstBinAbove(threshold, iAxis);
  int highBin = aHisto.FindLastBinAbove(threshold, iAxis);
  if(lowBin<1 || highBin<1) return {1, nBins};
  int margin = 10 + (highBin - lowBin)*0.1;
  return {std::max(lowBin-margin, 1), std::min(highBin+margin, nBins)};
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
crow::json::wvalue WebGUIController::makeHeatmapPanel(const TH1 & aHisto, bool zeroIsEmpty,
                                                      bool withFrameColor, bool allowAutozoom) const {
  wvalue aPanel;
  aPanel["kind"] = "heatmap";
  aPanel["title"] = aHisto.GetTitle();
  aPanel["xTitle"] = aHisto.GetXaxis()->GetTitle();
  aPanel["yTitle"] = aHisto.GetYaxis()->GetTitle();
  aPanel["zTitle"] = aHisto.GetZaxis()->GetTitle();
  aPanel["logZ"] = isLogScaleOn;
  if(withFrameColor) aPanel["frameColor"] = frameColor;

  const TAxis * xAxis = aHisto.GetXaxis();
  const TAxis * yAxis = aHisto.GetYaxis();
  std::vector<double> x, y;
  for(int ix=1;ix<=xAxis->GetNbins();++ix) x.push_back(xAxis->GetBinCenter(ix));
  for(int iy=1;iy<=yAxis->GetNbins();++iy) y.push_back(yAxis->GetBinCenter(iy));
  aPanel["x"] = numList(x);
  aPanel["y"] = numList(y);

  std::vector<wvalue> z;
  z.reserve(y.size());
  for(int iy=1;iy<=yAxis->GetNbins();++iy){
    std::vector<wvalue> row;
    row.reserve(x.size());
    for(int ix=1;ix<=xAxis->GetNbins();++ix){
      double value = aHisto.GetBinContent(ix, iy);
      if(zeroIsEmpty && value==0) row.emplace_back(); // null: frame colour shows through
      else row.push_back(num(value));
    }
    z.push_back(toValue(std::move(row)));
  }
  aPanel["z"] = std::move(z);

  int x1 = 1, x2 = xAxis->GetNbins(), y1 = 1, y2 = yAxis->GetNbins();
  if(isAutozoomOn && allowAutozoom){
    std::tie(x1, x2) = autozoomBins(aHisto, 1);
    std::tie(y1, y2) = autozoomBins(aHisto, 2);
  }
  aPanel["xRange"] = range(xAxis->GetBinLowEdge(x1), xAxis->GetBinUpEdge(x2));
  aPanel["yRange"] = range(yAxis->GetBinLowEdge(y1), yAxis->GetBinUpEdge(y2));
  return aPanel;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
crow::json::wvalue WebGUIController::makeHist1DPanel(const TH1 & aHisto) const {

  wvalue aPanel;
  aPanel["kind"] = "plot";
  aPanel["title"] = aHisto.GetTitle();
  aPanel["xTitle"] = aHisto.GetXaxis()->GetTitle();
  aPanel["yTitle"] = aHisto.GetYaxis()->GetTitle();
  const TAxis * xAxis = aHisto.GetXaxis();
  std::vector<double> x, y;
  for(int ix=1;ix<=xAxis->GetNbins();++ix){
    x.push_back(xAxis->GetBinCenter(ix));
    y.push_back(aHisto.GetBinContent(ix));
  }
  PanelOverlay ov;
  wvalue aSeries = line(x, y, rootColor(4), 1);
  aSeries["shape"] = "hvh"; // histogram steps
  ov.series.push_back(std::move(aSeries));
  ov.moveTo(aPanel);
  int x1 = 1, x2 = xAxis->GetNbins();
  if(isAutozoomOn) std::tie(x1, x2) = autozoomBins(aHisto, 1);
  aPanel["xRange"] = range(xAxis->GetBinLowEdge(x1), xAxis->GetBinUpEdge(x2));
  return aPanel;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
crow::json::wvalue WebGUIController::makeRatePanel() const {

  wvalue aPanel;
  aPanel["kind"] = "plot";
  aPanel["title"] = "Event rate";
  aPanel["xTitle"] = "Time since start of run [s]";
  aPanel["yTitle"] = "Event rate [Hz]";
  std::vector<double> x, y;
  for(const auto & item: myEventRate){
    x.push_back(item.first);
    y.push_back(item.second);
  }
  PanelOverlay ov;
  ov.markers.push_back(markers(x, y, rootColor(4), "circle", 7));
  ov.moveTo(aPanel);
  return aPanel;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void WebGUIController::addTrackProjection(PanelOverlay & ov, int strip_dir,
                                          const Track3D & aTrack) const {
  int iSegment = 0;
  for(const auto & aSegment: aTrack.getSegments()){
    const TrackSegment2D & aProjection = aSegment.get2DProjection(strip_dir, 0, aSegment.getLength());
    const TVector3 & start = aProjection.getStart();
    const TVector3 & end = aProjection.getEnd();
    ov.lines.push_back(line({start.X(), end.X()}, {start.Y(), end.Y()},
                                 rootColor(2+iSegment), 4, "dashdot"));
    ov.markers.push_back(markers({start.X()}, {start.Y()}, "#ffffff", "circle", 7, "#000000"));
    ov.markers.push_back(markers({end.X()}, {end.Y()}, "#000000", "circle", 7));
    ++iSegment;
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void WebGUIController::addManualRecoOverlay(PanelOverlay & ov, int strip_dir) const {

  for(const auto & aSegment: myRecoTool.getSegments().at(strip_dir)){
    std::string color = rootColor(aSegment.color);
    ov.markers.push_back(markers({aSegment.x1}, {aSegment.y1}, color, "square", 8));
    if(std::isnan(aSegment.x2) || std::isnan(aSegment.y2)) continue;
    ov.markers.push_back(markers({aSegment.x2}, {aSegment.y2}, color, "square", 8));
    ov.lines.push_back(line({aSegment.x1, aSegment.x2}, {aSegment.y1, aSegment.y2}, color, 3, "dash"));
  }
  const auto & aMarker = myRecoTool.getMarkers().at(strip_dir);
  if(aMarker) ov.markers.push_back(markers({aMarker->x}, {aMarker->y}, rootColor(2), "circle", 8));
  if(auto fixedTime = myRecoTool.getFixedTime()){
    wvalue aLine;
    aLine["x"] = *fixedTime;
    aLine["color"] = rootColor(1);
    aLine["width"] = 2;
    ov.vlines.push_back(std::move(aLine));
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<crow::json::wvalue> WebGUIController::makeRawPanels() const {

  std::vector<wvalue> panels;
  auto aEvent = myEventSource->getCurrentEvent();
  for(int strip_dir=projection_type::DIR_U;strip_dir<=projection_type::DIR_W;++strip_dir){
    auto aHisto = aEvent->get2DProjection(get2DProjectionType(strip_dir), filter_type::none, scale_type::raw);
    panels.push_back(makeHeatmapPanel(*aHisto, true, true));
  }
  if(isRateDisplayOn) panels.push_back(makeRatePanel());
  else {
    auto aHisto = aEvent->get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::raw);
    panels.push_back(makeHist1DPanel(*aHisto));
  }
  return panels;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<crow::json::wvalue> WebGUIController::makeTechnicalPanels() const {

  std::vector<wvalue> panels;
  auto aEvent = myEventSource->getCurrentEvent();
  int nAgetChips = myEventSource->getGeometry()->GetAgetNchips();
  for(int aget_id=0;aget_id<nAgetChips;++aget_id){
    auto aHisto = aEvent->GetChannels(0, aget_id);
    if(!aHisto) continue;
    panels.push_back(makeHeatmapPanel(*aHisto, true, false, false)); // technical plots are not zoomed
  }
  return panels;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<crow::json::wvalue> WebGUIController::makeRecoPanels() const {

  std::vector<wvalue> panels;
  auto aEvent = myEventSource->getCurrentEvent();
  const Track3D & aTrack = myEventSource->getTrackBuilder()->getTrack3D(0);
  filter_type filterType = filter_type::threshold;
  if(!myConfig.get<bool>("hitFilter.recoClusterEnable")) filterType = filter_type::none;

  for(int strip_dir=projection_type::DIR_U;strip_dir<=projection_type::DIR_W;++strip_dir){
    auto aHisto = aEvent->get2DProjection(get2DProjectionType(strip_dir), filterType, scale_type::mm);
    wvalue aPanel = makeHeatmapPanel(*aHisto, true, true);
    aPanel["clickDir"] = strip_dir; // clicks define the segment end points
    PanelOverlay ov;
    addManualRecoOverlay(ov, strip_dir);
    if(isManualTrackShown) addTrackProjection(ov, strip_dir, aTrack);
    ov.moveTo(aPanel);
    panels.push_back(std::move(aPanel));
  }
  // charge along the fitted track (as in the devel mode), or the time projection while clicking
  if(isManualTrackShown) panels.push_back(makeChargeProfilePanel(aTrack));
  else {
    auto aHisto = aEvent->get1DProjection(projection_type::DIR_TIME, filterType, scale_type::mm);
    wvalue aPanel = makeHist1DPanel(*aHisto);
    PanelOverlay ov;
    for(const auto & item: myRecoTool.getTimeLines()){
      wvalue aLine;
      aLine["x"] = item.x;
      aLine["color"] = rootColor(item.color);
      aLine["width"] = 2;
      ov.vlines.push_back(std::move(aLine));
    }
    ov.moveTo(aPanel);
    // line following the mouse over the projections while the first point is being chosen
    if(myRecoTool.isAcceptingPoints() && !myRecoTool.getFixedTime()){
      aPanel["cursorLineColor"] = rootColor(myRecoTool.getCursorLineColor());
    }
    panels.push_back(std::move(aPanel));
  }
  // track projection on the readout plane: fitted track, or the detector outline in the devel mode
  if(isManualTrackShown) panels.push_back(makeXYPanel(aTrack));
  else if(isDevelView()) panels.push_back(makeXYPanel(Track3D()));
  return panels;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<crow::json::wvalue> WebGUIController::makeDevelPanels() const {

  std::vector<wvalue> panels;
  auto aTrackBuilder = myEventSource->getTrackBuilder();
  const Track3D & aTrack = aTrackBuilder->getTrack3D(0);
  for(int strip_dir=projection_type::DIR_U;strip_dir<=projection_type::DIR_W;++strip_dir){
    wvalue aPanel = makeHeatmapPanel(aTrackBuilder->getRecHits2D(strip_dir), true, true);
    PanelOverlay ov;
    addTrackProjection(ov, strip_dir, aTrack);
    ov.moveTo(aPanel);
    panels.push_back(std::move(aPanel));
  }
  panels.push_back(makeChargeProfilePanel(aTrack));
  panels.push_back(makeXYPanel(aTrack));
  return panels;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
crow::json::wvalue WebGUIController::makeXYPanel(const Track3D & aTrack) const {

  wvalue aPanel;
  aPanel["kind"] = "plot";
  aPanel["title"] = "Track projection on the readout plane";
  aPanel["xTitle"] = "x [mm]";
  aPanel["yTitle"] = "y [mm]";
  aPanel["equalAspect"] = true;
  PanelOverlay ov;

  auto aGeometry = myEventSource->getGeometry();
  auto hullLine = [](const TGraph & aGraph, const std::string & dash, double width){
    std::vector<double> x(aGraph.GetX(), aGraph.GetX()+aGraph.GetN());
    std::vector<double> y(aGraph.GetY(), aGraph.GetY()+aGraph.GetN());
    return line(x, y, rootColor(1), width, dash);
  };
  ov.series.push_back(hullLine(aGeometry->GetActiveAreaConvexHull(), "solid", 2));
  ov.series.push_back(hullLine(aGeometry->GetActiveAreaConvexHull(10.0), "dash", 1));

  // 1:1 aspect ratio, +/-5% margin around the longest dimension (as HistoManager::setDetLayout)
  float xmin, xmax, ymin, ymax;
  std::tie(xmin, xmax, ymin, ymax) = aGeometry->rangeXY();
  double width = 1.1*std::max(xmax-xmin, ymax-ymin);
  aPanel["xRange"] = range(0.5*(xmin+xmax)-0.5*width, 0.5*(xmin+xmax)+0.5*width);
  aPanel["yRange"] = range(0.5*(ymin+ymax)-0.5*width, 0.5*(ymin+ymax)+0.5*width);

  int iSegment = 0;
  for(const auto & aSegment: aTrack.getSegments()){
    const TVector3 & start = aSegment.getStart();
    const TVector3 & end = aSegment.getEnd();
    ov.series.push_back(line({start.X(), end.X()}, {start.Y(), end.Y()}, rootColor(2+iSegment), 3));
    ov.markers.push_back(markers({start.X()}, {start.Y()}, "#ffffff", "circle", 7, "#000000"));
    ov.markers.push_back(markers({end.X()}, {end.Y()}, "#000000", "circle", 7));
    ++iSegment;
  }
  ov.moveTo(aPanel);
  return aPanel;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
crow::json::wvalue WebGUIController::makeChargeProfilePanel(const Track3D & aTrack) const {

  wvalue aPanel;
  aPanel["kind"] = "plot";
  aPanel["title"] = "Charge along the track";
  aPanel["xTitle"] = "d [mm]";
  aPanel["yTitle"] = "charge/mm [arb. units]";
  if(aTrack.getLength()<1 || aTrack.getSegments().empty()) return aPanel;
  PanelOverlay ov;

  bool isUnknownPID = aTrack.getSegments().front().getPID()==pid_type::UNKNOWN;
  TH1F hChargeProfile = isUnknownPID ? aTrack.getSegments().front().getChargeProfile()
                                     : aTrack.getChargeProfile();
  std::vector<double> x, y;
  for(int iBin=1;iBin<=hChargeProfile.GetNbinsX();++iBin){
    x.push_back(hChargeProfile.GetBinCenter(iBin));
    y.push_back(hChargeProfile.GetBinContent(iBin));
  }
  ov.markers.push_back(markers(x, y, rootColor(2), "circle", 6));
  aPanel["xRange"] = range(-20, 20+aTrack.getLength());
  aPanel["yRange"] = range(0.0, 1.2*hChargeProfile.GetMaximum());

  if(isUnknownPID){
    ov.moveTo(aPanel);
    return aPanel;
  }

  TF1 dEdx = myEventSource->getTrackBuilder()->getdEdx();
  if(dEdx.GetNpar()){
    double carbonScale = dEdx.GetParameter("carbonScale");
    auto curve = [&dEdx](){
      std::vector<double> cx, cy;
      double xMin = dEdx.GetXmin(), xMax = dEdx.GetXmax();
      int nPoints = std::max(100, std::min(4000, int((xMax-xMin)*10)));
      for(int i=0;i<=nPoints;++i){
        double xx = xMin + (xMax-xMin)*i/nPoints;
        cx.push_back(xx);
        cy.push_back(dEdx.Eval(xx));
      }
      return std::make_pair(cx, cy);
    };
    auto full = curve();
    ov.series.push_back(line(full.first, full.second, rootColor(1), 3, "solid", "¹²C + α"));
    dEdx.SetParameter("carbonScale", 0.0);
    auto alpha = curve();
    ov.series.push_back(line(alpha.first, alpha.second, rootColor(2), 2, "dash", "α"));
    dEdx.SetParameter("alphaScale", 0.0);
    dEdx.SetParameter("carbonScale", carbonScale);
    auto carbon = curve();
    ov.series.push_back(line(carbon.first, carbon.second, "#9999ff", 2, "dash", "¹²C"));
  }

  ov.moveTo(aPanel);
  return aPanel;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string WebGUIController::makeRecoText(const Track3D * aTrack) const {

  // the same printout as on the terminal (TrackBuilder), with its ANSI colour codes
  std::ostringstream out;
  if(!aTrack || aTrack->getSegments().empty() || aTrack->getLength()<1){
    out<<KRED<<"No reconstructed track."<<RST<<std::endl;
    return out.str();
  }
  out<<KBLU<<"Total length [mm]: "<<RST<<format("%3.0f", aTrack->getLength())<<std::endl;
  if(aTrack->getSegments().front().getPID()==pid_type::UNKNOWN){
    out<<KBLU<<"Particle PID: "<<RST<<"UNKNOWN"<<std::endl;
  }
  else{
    double alphaRange = aTrack->getSegments().front().getLength();
    double carbonRange = aTrack->getSegments().size()==2 ? aTrack->getSegments().back().getLength() : 0.0;
    double totalEnergy = myRangeCalculator.getIonEnergyMeV(pid_type::ALPHA, alphaRange) +
                         myRangeCalculator.getIonEnergyMeV(pid_type::CARBON_12, carbonRange);
    out<<KBLU<<"Total E [MeV]: "<<RST<<format("%2.1f", totalEnergy)<<std::endl;
  }
  out<<std::endl<<KBLU<<"Reconstructed track: "<<RST<<std::endl;
  out<<myEventSource->getCurrentEvent()->GetEventInfo()<<std::endl;
  out<<*aTrack<<std::endl;
  return out.str();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
crow::json::wvalue WebGUIController::listDirectory(const std::string & dirName) const {

  wvalue aResult;
  fs::path aDir = dirName;
  if(dirName.empty()){
    fs::path current = myEventSource ? myEventSource->getCurrentPath() : std::string("");
    current = current.string().substr(0, current.string().find(','));
    aDir = current.has_parent_path() ? current.parent_path() : fs::current_path();
  }
  std::error_code ec;
  aDir = fs::weakly_canonical(fs::absolute(aDir, ec), ec);
  if(ec || !fs::is_directory(aDir, ec)){
    aResult["error"] = "Not a directory: "+aDir.string();
    return aResult;
  }
  aResult["dir"] = aDir.string();
  aResult["parent"] = aDir.has_parent_path() ? aDir.parent_path().string() : aDir.string();

  std::vector<std::pair<std::string,bool>> entries; // name, is directory
  for(const auto & item: fs::directory_iterator(aDir, fs::directory_options::skip_permission_denied, ec)){
    std::string name = item.path().filename().string();
    if(name.empty() || name[0]=='.') continue;
    std::error_code ec2;
    bool isDir = item.is_directory(ec2);
    std::string ext = item.path().extension().string();
    if(!isDir && ext!=".root" && ext!=".graw") continue;
    entries.emplace_back(name, isDir);
  }
  std::sort(entries.begin(), entries.end(), [](const auto & a, const auto & b){
    if(a.second!=b.second) return a.second;
    return a.first<b.first;
  });
  std::vector<wvalue> aList;
  for(const auto & item: entries){
    wvalue aEntry;
    aEntry["name"] = item.first;
    aEntry["isDir"] = item.second;
    aList.push_back(std::move(aEntry));
  }
  aResult["entries"] = std::move(aList);
  return aResult;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
