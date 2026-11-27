#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>

#include <boost/property_tree/json_parser.hpp>

#include <TF1.h>
#include <TGraph.h>
#include <TH1.h>
#include <TH1F.h>
#include <TH2D.h>
#include <TVector3.h>

#include "TPCReco/ChargeProfileBuilder.h"
#include "TPCReco/CommonDefinitions.h"
#include "TPCReco/EventSourceFactory.h"
#include "TPCReco/EventSourceGRAW.h"
#include "TPCReco/EventSourceMultiGRAW.h"
#include "TPCReco/EventList.h"
#include "TPCReco/GrawChunkPath.h"
#include "TPCReco/GrawFile.h"
#include "TPCReco/FileOutput.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/GUI_commons.h"
#include "TPCReco/ResourcePath.h"
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
  myStartConfig = aConfig;
  myFileOutput = std::make_shared<FileOutput>(myConfig);
  myEventSource = EventSourceFactory::makeEventSourceObject(myConfig);
  setWorkMode();

  myEventSource->loadFileEntry(0);
  auto aEvent = myEventSource->getCurrentEvent();
  if(aEvent && aEvent->GetEventInfo().GetPedestalSubtracted()){
    aEvent->setHitFilterConfig(filter_type::threshold, myConfig);
  }

  myRangeCalculator.setGasMixture(gas_mixture_type::CO2);
  myRangeCalculator.setGasPressure(myConfig.get<double>("conditions.pressure"));
  myRangeCalculator.setGasTemperature(myConfig.get<double>("conditions.temperature"));
  myRecoTool->setGeometry(myEventSource->getGeometry());

  isLogScaleOn = myConfig.get<bool>("display.zLogScale", false);
  isAutozoomOn = myConfig.get<bool>("display.autoZoom", false);
  isRecoModeOn = myConfig.get<bool>("display.recoMode", false) && canRecoMode();
  isRateDisplayOn = myConfig.get<bool>("display.rate", false) && canRate();

  const std::string eventListFile = myConfig.get<std::string>("input.eventList", "");
  if(eventListFile.size()){
    if(myWorkMode!=M_OFFLINE_GRAW_MODE && myWorkMode!=M_OFFLINE_NGRAW_MODE){
      throw std::runtime_error("input.eventList: only for GRAW input (OFFLINE)");
    }
    myEventList = readEventList(eventListFile);
    // default data directory: the one above the run directory of the input files
    myEventListDataDir = myConfig.get<std::string>("input.eventListDataPath", "");
    if(myEventListDataDir.empty()){
      std::string firstFile = myConfig.get<std::string>("input.dataFile");
      firstFile = firstFile.substr(0, firstFile.find(','));
      myEventListDataDir = fs::path(firstFile).parent_path().parent_path().string();
    }
    if(myEventList.size()){
      std::string error = loadListedEvent(0);
      if(error.size()) std::cerr<<KRED<<"WebGUIController: "<<RST<<error<<std::endl;
    }
  }

  eventLoaded();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void WebGUIController::setWorkMode(){

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
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<WebGUIController::ListedEvent> WebGUIController::readEventList(const std::string & fileName){

  std::vector<ListedEvent> events;

  // the parser of tpcReco --input.eventList (EventList.h); the GUI steps through the runs and chunks
  // of the listed events, so every line needs "runId , entry , eventId"; the order of the file is kept
  for(const auto & aEvent: ::readEventList(fileName)){
    if(!aEvent.hasRun() || !aEvent.hasEntry()){
      throw std::runtime_error("input.eventList: expected 'runId , entry , eventId' lines in "+fileName
                               +" (event "+std::to_string(aEvent.eventId)+")");
    }
    events.push_back({aEvent.runId, static_cast<unsigned long>(aEvent.entry), aEvent.eventId});
  }
  std::cout<<KBLU<<"WebGUIController: event list "<<RST<<fileName<<KBLU<<": "<<RST
           <<events.size()<<KBLU<<" events"<<RST<<std::endl;
  return events;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string WebGUIController::openRun(const std::string & files){

  if(!canNavigate()) return "Opening files is disabled in ONLINE mode.";
  // a run recorded with a file per AsAd board or with one file for all needs the matching reader:
  // a new event source (with the current geometry and conditions) when the run is of the other kind
  const bool isOneFilePerAsad = files.find(',')!=std::string::npos;
  const bool isMultiReader = myWorkMode==M_OFFLINE_NGRAW_MODE;
  if((myWorkMode==M_OFFLINE_GRAW_MODE || myWorkMode==M_OFFLINE_NGRAW_MODE) && isOneFilePerAsad!=isMultiReader){
    boost::property_tree::ptree aConfig = myConfig;
    aConfig.put("input.dataFile", files);
    aConfig.put("input.singleAsadGrawFile", isOneFilePerAsad);
    std::stringstream aStream(files);
    std::string item;
    while(std::getline(aStream, item, ',')) if(!fs::is_regular_file(item)) return "No such file: "+item;
    std::string error = replaceEventSource(aConfig);
    if(error.size()) return error;
    error = checkGrawFileCount(files);
    if(error.size()) return error;
    myEventSource->loadDataFile(files);
    myEventSource->loadFileEntry(0);
    eventLoaded();
  }
  else {
    std::string error = openFile(files);
    if(error.size()) return error;
  }
  myEventList.clear(); // the arrows step through the run (and its chunks)
  myListPosition = -1;
  myDataSetName.clear();
  return "";
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string WebGUIController::openDataSet(const std::string & jsonPath){

  if(!canNavigate()) return "Opening data sets is disabled in ONLINE mode.";
  if(myWorkMode!=M_OFFLINE_GRAW_MODE && myWorkMode!=M_OFFLINE_NGRAW_MODE){
    return "Data sets can be opened for GRAW input only.";
  }
  boost::property_tree::ptree aDataSet;
  try { boost::property_tree::read_json(jsonPath, aDataSet); }
  catch(const std::exception & e){ return "Cannot read the data set "+jsonPath+": "+e.what(); }

  // settings of the data set on top of the start-up configuration (not of the data set open
  // before: its settings must not leak into this one)
  const fs::path baseDir = fs::path(jsonPath).parent_path();
  auto relativeTo = [&baseDir](const std::string & path){
    return path.empty() || fs::path(path).is_absolute() ? path : (baseDir/path).string();
  };
  boost::property_tree::ptree aConfig = myStartConfig;
  aConfig.put("input.eventList", "");
  for(const std::string key: {"eventList", "eventListDataPath", "geometryFile", "dataFile"}){
    auto value = aDataSet.get_optional<std::string>("input."+key);
    if(!value || value->empty()) continue;
    std::string path = *value;
    if(key=="dataFile"){ // comma separated files
      std::stringstream aStream(*value);
      std::string item;
      path.clear();
      while(std::getline(aStream, item, ',')) path += (path.empty() ? "" : ",") + relativeTo(item);
    }
    else if(key=="geometryFile"){ // a resources file or a path relative to the data set
      path = ResourcePath::resolve(*value);
      if(!fs::exists(path)) path = relativeTo(*value);
    }
    else path = relativeTo(path);
    aConfig.put("input."+key, path);
  }
  for(const std::string group: {"conditions", "pedestal", "hitFilter", "beamParameters"}){
    auto aGroup = aDataSet.get_child_optional(group);
    if(!aGroup) continue;
    for(const auto & item: *aGroup) aConfig.put(group+"."+item.first, item.second.data());
  }
  const std::string name = aDataSet.get<std::string>("name", fs::path(jsonPath).stem().string());
  const std::string eventListFile = aDataSet.get<std::string>("input.eventList", "").empty() ? "" :
                                    aConfig.get<std::string>("input.eventList", "");
  const std::string dataDir = aConfig.get<std::string>("input.eventListDataPath", "");

  // a run: its files (input.dataFile) with the settings of the data set
  if(eventListFile.empty()){
    const std::string files = aDataSet.get<std::string>("input.dataFile", "").empty() ? "" :
                              aConfig.get<std::string>("input.dataFile", "");
    if(files.empty()) return "The data set "+name+" has neither an event list nor data files on disk.";
    if(aDataSet.get<std::string>("input.geometryFile", "").empty()) return "The data set "+name+" has no geometry file.";
    std::stringstream aStream(files);
    std::string item;
    while(std::getline(aStream, item, ',')) if(!fs::is_regular_file(item)) return "No such file: "+item;
    aConfig.put("input.singleAsadGrawFile", files.find(',')!=std::string::npos);
    std::string error = replaceEventSource(aConfig);
    if(error.size()) return error;
    error = checkGrawFileCount(files);
    if(error.size()) return error;
    myEventList.clear();
    myListPosition = -1;
    myDataSetName = name;
    myEventSource->loadDataFile(files);
    myEventSource->loadFileEntry(0);
    auto aEvent = myEventSource->getCurrentEvent();
    if(aEvent && aEvent->GetEventInfo().GetPedestalSubtracted()){
      aEvent->setHitFilterConfig(filter_type::threshold, myConfig);
    }
    std::cout<<KBLU<<"WebGUIController: data set "<<RST<<name<<KBLU<<" ("<<RST<<jsonPath<<KBLU<<")"<<RST<<std::endl;
    eventLoaded();
    return "";
  }
  if(dataDir.empty()) return "The data set "+name+" needs input.eventListDataPath.";
  std::vector<ListedEvent> events;
  try { events = readEventList(eventListFile); }
  catch(const std::exception & e){ return e.what(); }
  if(events.empty()) return "No events in the event list "+eventListFile;

  // a new event source (geometry, conditions) reading the run of the first listed event
  std::vector<std::string> files = findGrawRunFiles(dataDir, events.front().runId, 0);
  if(files.empty()) return "No GRAW files of run "+std::to_string(events.front().runId)+" in "+dataDir;
  std::string fileList;
  for(const auto & aFile: files) fileList += (fileList.empty() ? "" : ",") + aFile;
  aConfig.put("input.dataFile", fileList);
  aConfig.put("input.singleAsadGrawFile", files.size()>1);
  std::string error = replaceEventSource(aConfig);
  if(error.size()) return error;
  myEventList = events;
  myEventListDataDir = dataDir;
  myListPosition = -1;
  myDataSetName = name;

  error = loadListedEvent(0);
  auto aEvent = myEventSource->getCurrentEvent();
  if(aEvent && aEvent->GetEventInfo().GetPedestalSubtracted()){
    aEvent->setHitFilterConfig(filter_type::threshold, myConfig);
  }
  std::cout<<KBLU<<"WebGUIController: data set "<<RST<<myDataSetName<<KBLU<<" ("<<RST<<jsonPath
           <<KBLU<<"): "<<RST<<myEventList.size()<<KBLU<<" events"<<RST<<std::endl;
  eventLoaded();
  return error;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::size_t WebGUIController::framesPerChunk(long runId){

  auto it = myFramesPerChunk.find(runId);
  if(it!=myFramesPerChunk.end()) return it->second;
  std::vector<std::string> files = findGrawRunFiles(myEventListDataDir, runId, 0);
  std::size_t nFrames = 0;
  if(files.size()){
    graw::GrawFile aFile;
    if(aFile.open(files.front())) nFrames = aFile.nFrames();
  }
  if(nFrames) myFramesPerChunk[runId] = nFrames;
  return nFrames;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int WebGUIController::currentChunk() const {

  // ..._<chunk>.graw
  const std::string path = myEventSource->getCurrentPath();
  const std::size_t end = path.rfind(".graw");
  if(end==std::string::npos || end<4) return -1;
  try { return std::stoi(path.substr(end-4, 4)); }
  catch(const std::exception &){ return -1; }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string WebGUIController::loadListedEvent(long position){

  if(position<0 || position>=(long)myEventList.size()) return "No such event in the event list.";
  const ListedEvent & aEvent = myEventList[position];
  if(aEvent.eventId<aEvent.entry) return "Event list: event id smaller than the entry, line "+std::to_string(position+1);
  // chunk of the event: all chunks of a run have the same number of frames (but the last)
  const std::size_t nFrames = framesPerChunk(aEvent.runId);
  if(!nFrames){
    return "Event list: no GRAW files of run "+std::to_string(aEvent.runId)+" in "+myEventListDataDir;
  }
  const int chunk = (aEvent.eventId - aEvent.entry)/nFrames;
  const bool isLoaded = myEventSource->getCurrentEvent() &&
                        myEventSource->getCurrentEvent()->GetEventInfo().GetRunId()==aEvent.runId &&
                        currentChunk()==chunk;
  if(!isLoaded){
    std::vector<std::string> files = findGrawRunFiles(myEventListDataDir, aEvent.runId, chunk);
    if(files.empty()){
      return "Event list: no chunk "+std::to_string(chunk)+" of run "+std::to_string(aEvent.runId)+" in "+myEventListDataDir;
    }
    std::string fileList;
    for(const auto & aFile: files) fileList += (fileList.empty() ? "" : ",") + aFile;
    myEventSource->loadDataFile(fileList);
  }
  myEventSource->loadFileEntry(aEvent.entry);
  if(myEventSource->currentEventId()!=aEvent.eventId) myEventSource->loadEventId(aEvent.eventId);
  myListPosition = position;
  if(myEventSource->currentEventId()!=aEvent.eventId){
    return "Event list: event "+std::to_string(aEvent.eventId)+" not found in chunk "+std::to_string(chunk)
           +" of run "+std::to_string(aEvent.runId);
  }
  return "";
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
WebGUIController::~WebGUIController(){ close(); }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void WebGUIController::close(){

  // events saved more than once: only the last save is kept
  std::string outputFileName = myFileOutput ? myFileOutput->getFileName() : "";
  myFileOutput.reset();
  if(!outputFileName.empty()) FileOutput::keepLastEntryPerEvent(outputFileName);
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
  isTrackBuilderManual = false; // a newly loaded event is reconstructed automatically
  refresh();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void WebGUIController::refresh(){

  // equivalent of MainFrame::Update() without the drawing, which is done on demand by getView()
  myRecoTool->reset(isRecoModeOn);
  isManualTrackShown = false;
  isTrackLineOutdated = false;
  if(!myEventSource || !myEventSource->numberOfEvents()) return;
  if(isNewEvent){
    myClickedTrack.reset();
    if(isOnline()) updateEventRate();
    readSavedRecoEvent();
    // an event is written on "Next event" (writeCurrentEvent); ONLINE mode, without navigation:
    // automatic output once per loaded event
    if(myFileOutput && isOnline()) myFileOutput->update(myEventSource);
  }
  // manual RECO mode: the shown track (read back from the output file, fitted clicked or automatic)
  // is the starting point: its points as clicked points
  seedRecoTool();
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
void WebGUIController::seedRecoTool(){

  if(!isRecoModeOn || !myEventSource || !myEventSource->numberOfEvents()) return;
  const Track3D & aTrack = displayedTrack();
  const auto & segments = aTrack.getSegments();
  if(segments.empty() || aTrack.getLength()<1) return;
  // point at the fraction f of the segment (0: start, 1: end) in the U, V, W projections:
  // (time [mm], strip coordinate [mm])
  auto project = [](const TrackSegment3D & aSegment, double f){
    ManualRecoTool::Point3P aPoint;
    for(int strip_dir=projection_type::DIR_U;strip_dir<=projection_type::DIR_W;++strip_dir){
      const TrackSegment2D & aProjection = aSegment.get2DProjection(strip_dir, 0, aSegment.getLength());
      const TVector3 aPos = aProjection.getStart() + f*(aProjection.getEnd() - aProjection.getStart());
      aPoint.at(strip_dir) = {aPos.X(), aPos.Y()};
    }
    return aPoint;
  };
  std::vector<ManualRecoTool::Point3P> points;
  if(myRecoTool->getMode()==ManualRecoTool::Mode::TwoProng){
    // the common vertex (start of the segments), then the end of each segment
    points.push_back(project(segments.front(), 0.0));
    for(const auto & aSegment: segments) points.push_back(project(aSegment, 1.0));
  }
  else {
    // two points on each track, not the vertex (which would pin the track lines to the old vertex
    // and bias the new one): the middle and the end of the track
    for(const auto & aSegment: segments){
      points.push_back(project(aSegment, 0.5));
      points.push_back(project(aSegment, 1.0));
    }
  }
  myRecoTool->setPoints(points);
  isManualTrackShown = true;
  isTrackLineOutdated = false;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void WebGUIController::writeCurrentEvent(unsigned long eventType){

  // the state shown: RECO mode the saved / fitted clicked / automatic track, otherwise the automatic
  // reconstruction, with the event type bits; an earlier save of the event is replaced when the file
  // is closed (FileOutput::keepLastEntryPerEvent)
  if(!myFileOutput || isOnline() || !myEventSource || !myEventSource->numberOfEvents()) return;
  eventraw::EventInfo aEventInfo = myEventSource->getCurrentEvent()->GetEventInfo();
  aEventInfo.SetEventType(eventType);
  myFileOutput->update(myEventSource, aEventInfo, displayedTrack());
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void WebGUIController::readSavedRecoEvent(){

  mySavedTrack.reset();
  mySavedEventType = 0;
  if(!myFileOutput || isOnline()) return;
  eventraw::EventInfo aInfo;
  auto aTrack = std::make_shared<Track3D>();
  if(!myFileOutput->findRecoEvent(myEventSource->getCurrentEvent()->GetEventInfo(), aInfo, *aTrack)) return;
  mySavedTrack = aTrack;
  mySavedEventType = aInfo.GetEventType().to_ulong();
  std::cout<<KBLU<<"WebGUIController: event "<<RST<<aInfo.GetEventId()
           <<KBLU<<" read from the output file"<<RST<<std::endl;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const Track3D & WebGUIController::displayedTrack() const {

  // manual RECO mode: the track read from the output file or the fitted clicked track;
  // otherwise the automatic reconstruction
  if(isRecoModeOn && mySavedTrack) return *mySavedTrack;
  if(isRecoModeOn && myClickedTrack) return *myClickedTrack;
  return myEventSource->getTrackBuilder()->getTrack3D(0);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void WebGUIController::leaveRecoMode(){

  if(!myEventSource || !myEventSource->numberOfEvents()) return;
  // nothing is written here: an event is saved only on "Next event"
  myClickedTrack.reset();
  isManualTrackUnsaved = false;
  isManualTrackShown = false;
  // "Fit segments" replaced the automatic reconstruction in the TrackBuilder: made again
  if(isTrackBuilderManual){
    myEventSource->getTrackBuilder()->reconstruct();
    isTrackBuilderManual = false;
  }
  readSavedRecoEvent(); // "Fit segments" / "Clear track" forgot the track read from the output file
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Track3D WebGUIController::makeClickedTrack(const Track3D & aTrack, ManualRecoTool::Mode aMode){

  // the same convention as the analysis of the reconstructed events: segments in descending
  // order of length, 2-prong: alpha + carbon-12, 3-prong: three alphas
  TrackSegment3DCollection segments = aTrack.getSegments();
  std::stable_sort(segments.begin(), segments.end(),
                   [](const TrackSegment3D & a, const TrackSegment3D & b){ return a.getLength()>b.getLength(); });
  for(size_t iSegment=0;iSegment<segments.size();++iSegment){
    pid_type aPID = pid_type::UNKNOWN;
    if(aMode==ManualRecoTool::Mode::ThreeProng) aPID = pid_type::ALPHA;
    else if(segments.size()==1) aPID = pid_type::ALPHA;
    else if(segments.size()==2) aPID = iSegment==0 ? pid_type::ALPHA : pid_type::CARBON_12;
    segments[iSegment].setPID(aPID); // 2-prong mode with more than two segments: UNKNOWN
  }
  Track3D aClickedTrack;
  for(const auto & aSegment: segments) aClickedTrack.addSegment(aSegment);
  aClickedTrack.setHypothesisFitLoss(aTrack.getHypothesisFitLoss());
  aClickedTrack.setChargeProfile(aTrack.getChargeProfile()); // the stacked profile does not depend on the order
  return aClickedTrack;
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
  std::string error = checkGrawFileCount(fileName);
  if(error.size()) return error;
  myEventSource->loadDataFile(fileName);
  myEventSource->loadFileEntry(0);
  eventLoaded();
  return "";
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string WebGUIController::checkGrawFileCount(const std::string & files) const {

  if(myWorkMode!=M_OFFLINE_GRAW_MODE && myWorkMode!=M_OFFLINE_NGRAW_MODE) return "";
  const long nFiles = std::count(files.begin(), files.end(), ',') + 1;
  if(myWorkMode==M_OFFLINE_GRAW_MODE && nFiles!=1){
    return "This reader takes one GRAW file (all AsAd boards), not "+std::to_string(nFiles)+".";
  }
  const int nAsad = myEventSource->getGeometry() ? myEventSource->getGeometry()->GetAsadNboards() : 0;
  if(myWorkMode==M_OFFLINE_NGRAW_MODE && nAsad>0 && nFiles!=nAsad){
    return "This reader takes one GRAW file per AsAd board ("+std::to_string(nAsad)+"), not "
      +std::to_string(nFiles)+".";
  }
  return "";
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string WebGUIController::replaceEventSource(boost::property_tree::ptree & aConfig){

  std::shared_ptr<EventSourceBase> aEventSource;
  try { aEventSource = EventSourceFactory::makeEventSourceObject(aConfig); }
  catch(const std::exception & e){ return std::string("Cannot open the input: ")+e.what(); }
  if(!aEventSource) return "Cannot open the input.";
  myConfig = aConfig;
  myEventSource = aEventSource;
  setWorkMode();
  myFramesPerChunk.clear();
  myRangeCalculator.setGasPressure(myConfig.get<double>("conditions.pressure"));
  myRangeCalculator.setGasTemperature(myConfig.get<double>("conditions.temperature"));
  myRecoTool->setGeometry(myEventSource->getGeometry());
  myClickedTrack.reset();
  mySavedTrack.reset();
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
    writeCurrentEvent(aEventType);
    if(myEventList.size()){ // event list: the next listed event
      if(myListPosition+1>=(long)myEventList.size()) return "Last event of the event list.";
      std::string error = loadListedEvent(myListPosition+1);
      eventLoaded();
      return error;
    }
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
    if(myEventList.size()){ // event list: the previous listed event
      if(myListPosition<=0) return "First event of the event list.";
      std::string error = loadListedEvent(myListPosition-1);
      eventLoaded();
      return error;
    }
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
  else if(cmd=="gotoListPosition"){ // 1-based position in the event list
    if(!canNavigate()) return "Event navigation is disabled in ONLINE mode.";
    if(myEventList.empty()) return "No event list loaded (input.eventList).";
    long int position = command["position"].i();
    if(position<1 || position>(long)myEventList.size())
      return "List position must be between 1 and "+std::to_string(myEventList.size())+".";
    std::string error = loadListedEvent(position-1);
    eventLoaded();
    return error;
  }
  else if(cmd=="open") return openFile(command["path"].s());
  else if(cmd=="openRun") return openRun(command["files"].s());
  else if(cmd=="openDataSet") return openDataSet(command["path"].s());
  else if(cmd=="set"){
    const std::string option = command["option"].s();
    bool value = command["value"].b();
    // display options: only the drawing changes; the clicked points and the fitted track stay
    if(option=="logScale"){ isLogScaleOn = value; return ""; }
    else if(option=="autoZoom"){ isAutozoomOn = value; return ""; }
    else if(option=="recoMode"){
      if(value && !canRecoMode()) return "Manual reconstruction is not available in this mode.";
      if(!value && isRecoModeOn) leaveRecoMode();
      isRecoModeOn = value;
    }
    else if(option=="deconvolution"){
      // the current event is reconstructed again with / without the deconvolution
      auto aTrackBuilder = myEventSource->getTrackBuilder();
      if(!aTrackBuilder->setDeconvolution(value)){
        return "Deconvolution not available: no detector response (reconstruction.deconvolutionResponseFile).";
      }
      if(myEventSource->numberOfEvents()) aTrackBuilder->reconstruct();
      isTrackBuilderManual = false;
      myClickedTrack.reset();
      mySavedTrack.reset(); // the new reconstruction is shown, not the one read from the output file
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
  else if(cmd=="recoMode"){ // "2prong" / "3prong": a new tool, the clicked points are cleared
    auto aMode = ManualRecoTool::modeFromName(command["mode"].s());
    if(!aMode) return "Unknown manual reconstruction mode: "+std::string(command["mode"].s());
    if(*aMode!=myRecoTool->getMode()){
      myRecoTool = ManualRecoTool::create(*aMode);
      myRecoTool->setGeometry(myEventSource->getGeometry());
      myRecoTool->reset(isRecoModeOn);
      seedRecoTool(); // the points of the shown track, in the convention of the new mode
    }
  }
  else if(cmd=="recoAddSegment"){
    if(!isRecoModeOn || !myRecoTool->isAddEnabled()) return "Adding a segment is not possible now.";
    myRecoTool->addSegment();
  }
  else if(cmd=="recoClick"){
    if(!isRecoModeOn) return "Manual reconstruction mode is off.";
    // a new point: the shown track is out of date until the next fit
    if(myRecoTool->click(command["dir"].i(), command["x"].d(), command["y"].d())) isTrackLineOutdated = true;
  }
  else if(cmd=="recoMovePoint"){
    if(!isRecoModeOn) return "Manual reconstruction mode is off.";
    long index = command["index"].i();
    if(index<0 || !myRecoTool->movePoint(index, command["dir"].i(), command["x"].d(), command["y"].d())){
      return "This point cannot be moved now.";
    }
    isTrackLineOutdated = true; // the shown track is out of date until the next fit
  }
  else if(cmd=="recoFit"){
    if(!isRecoModeOn || !myRecoTool->isFitEnabled()) return "No complete segments to fit.";
    std::vector<double> segmentsXY = myRecoTool->takeSegmentsForFit();
    myEventSource->getTrackBuilder()->getSegment2DCollectionFromGUI(segmentsXY);
    isTrackBuilderManual = true; // the automatic reconstruction is replaced (made again when RECO mode is left)
    isClickedThreeProng = myRecoTool->getMode()==ManualRecoTool::Mode::ThreeProng;
    // the clicked track fitted to the hits (vertex, directions, lengths), both modes
    myEventSource->getTrackBuilder()->fitClickedTrackToHits();
    myClickedTrack = std::make_shared<Track3D>(makeClickedTrack(myEventSource->getTrackBuilder()->getTrack3D(0),
                                                                myRecoTool->getMode()));
    // 3-prong: dE/dx fit of the longest segment, as in the automatic reconstruction (for the plot)
    myProfileMatches.clear();
    // 2-prong: dE/dx fit as in the automatic reconstruction (12C + alpha allowed)
    if(!isClickedThreeProng) myEventSource->getTrackBuilder()->fitClickedTwoProngdEdx();
    if(isClickedThreeProng && myEventSource->getTrackBuilder()->fitLongestSegmentdEdx()!=pid_type::UNKNOWN){
      // the shorter alphas: shift along the long alpha's dE/dx fit -> lengths from dE/dx
      myProfileMatches.resize(myClickedTrack->getSegments().size());
      for(size_t iSegment=1;iSegment<myProfileMatches.size();++iSegment){
        myProfileMatches[iSegment] =
          myEventSource->getTrackBuilder()->matchToLongestAlpha(myClickedTrack->getSegments().at(iSegment));
        const auto & aMatch = myProfileMatches[iSegment];
        std::cout<<KBLU<<"Alpha "<<iSegment+1<<": "<<RST<<"length "<<myClickedTrack->getSegments().at(iSegment).getLength()
                 <<" mm, matched to the longest alpha with delta "<<aMatch.delta<<" mm -> length from dE/dx "
                 <<aMatch.length<<" mm (scale "<<aMatch.scale<<")"<<std::endl;
      }
    }
    std::cout<<KBLU<<"Particle types ("<<ManualRecoTool::modeName(myRecoTool->getMode())<<"): "<<RST;
    for(const auto & aSegment: myClickedTrack->getSegments()) std::cout<<aSegment.getPID()<<" ";
    std::cout<<std::endl;
    mySavedTrack.reset(); // the new fit replaces the track read from the output file
    isManualTrackShown = true;
    isTrackLineOutdated = false;
    isManualTrackUnsaved = true;
  }
  else if(cmd=="recoClearTrack"){
    // clicked points/segments (not fitted yet) and the shown track; the mode is kept
    myRecoTool->reset(isRecoModeOn);
    isManualTrackShown = false;
    mySavedTrack.reset();
  }
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
  status["dataSet"] = myDataSetName;
  status["nEvents"] = myEventSource ? myEventSource->numberOfEvents() : 0ul;
  status["eventId"] = myEventSource ? myEventSource->currentEventId() : 0ul;
  if(myEventList.size()){
    // position (1..N) of the event shown in the event list; 0 when another event is shown
    bool isListed = myListPosition>=0 && myEventSource &&
                    myEventSource->currentEventId()==myEventList[myListPosition].eventId;
    status["eventList"]["position"] = isListed ? myListPosition+1 : 0;
    status["eventList"]["size"] = static_cast<long>(myEventList.size());
  }
  status["entry"] = myEventSource ? myEventSource->currentEntryNumber() : 0ul;

  status["flags"]["logScale"] = isLogScaleOn;
  status["flags"]["autoZoom"] = isAutozoomOn;
  status["flags"]["recoMode"] = isRecoModeOn;
  status["flags"]["rate"] = isRateDisplayOn;
  status["flags"]["deconvolution"] = myEventSource && myEventSource->getTrackBuilder()->isDeconvolutionEnabled();

  auto aGeometry = myEventSource ? myEventSource->getGeometry() : nullptr;
  status["enabled"]["navigate"] = canNavigate();
  status["enabled"]["previous"] = canGoBack();
  status["enabled"]["open"] = canNavigate() && !isMC();
  status["enabled"]["recoMode"] = canRecoMode();
  status["enabled"]["rate"] = canRate();
  status["enabled"]["deconvolution"] = myEventSource && myEventSource->getTrackBuilder()->hasDeconvolutionResponseFile();
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

  status["reco"]["mode"] = ManualRecoTool::modeName(myRecoTool->getMode());
  status["reco"]["addLabel"] = myRecoTool->getAddLabel();
  status["reco"]["hint"] = isRecoModeOn ? myRecoTool->getHint() : std::string();
  status["reco"]["acceptPoints"] = myRecoTool->isAcceptingPoints();
  status["reco"]["addEnabled"] = isRecoModeOn && myRecoTool->isAddEnabled();
  status["reco"]["fitEnabled"] = isRecoModeOn && myRecoTool->isFitEnabled();
  status["reco"]["clearEnabled"] = isRecoModeOn && myRecoTool->isClearEnabled();
  // for the warnings before leaving an event: a fitted clicked track is written only by "Next event"
  status["reco"]["unsavedTrack"] = isRecoModeOn && isManualTrackUnsaved;
  // event type bits saved with the event in the output file (0 when not saved)
  status["savedEventType"] = static_cast<long long>(mySavedEventType);
  status["isSaved"] = static_cast<bool>(mySavedTrack);
  status["reco"]["pendingPoints"] = isRecoModeOn && myRecoTool->hasClickedPoints();
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
    if(!isRecoModeOn || isManualTrackShown) aTrack = &displayedTrack();
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

  // getSegments() returns a temporary: keep it alive for the loop
  const auto segments = myRecoTool->getSegments();
  for(const auto & aSegment: segments.at(strip_dir)){
    if(std::isnan(aSegment.x2) || std::isnan(aSegment.y2)) continue;
    ov.lines.push_back(line({aSegment.x1, aSegment.x2}, {aSegment.y1, aSegment.y2},
                            rootColor(aSegment.color), 3, "dash"));
  }
  // clicked points (they can be dragged to correct them)
  const auto & points = myRecoTool->getPoints();
  for(size_t iPoint=0;iPoint<points.size();++iPoint){
    const auto & aPoint = points.at(iPoint).p.at(strip_dir);
    ov.markers.push_back(markers({aPoint.x}, {aPoint.y}, rootColor(myRecoTool->getPointColor(iPoint)), "square", 8));
  }
  // 3-prong mode: the vertex calculated from the track lines
  if(auto aVertex = myRecoTool->getVertex(strip_dir)){
    // white dot, as the start of a fitted track
    ov.markers.push_back(markers({aVertex->x}, {aVertex->y}, "#ffffff", "circle", 7, "#000000"));
  }
  const auto & aMarker = myRecoTool->getMarkers().at(strip_dir);
  if(aMarker) ov.markers.push_back(markers({aMarker->x}, {aMarker->y}, rootColor(2), "circle", 8));
  if(auto fixedTime = myRecoTool->getFixedTime()){
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
  const Track3D & aTrack = displayedTrack();
  filter_type filterType = filter_type::threshold;
  if(!myConfig.get<bool>("hitFilter.recoClusterEnable")) filterType = filter_type::none;
  // with the deconvolution: its images (the deconvolved clean cluster the fits use) are shown
  auto aTrackBuilder = myEventSource->getTrackBuilder();
  const bool showDeconvolved = aTrackBuilder->isDeconvolutionEnabled();

  for(int strip_dir=projection_type::DIR_U;strip_dir<=projection_type::DIR_W;++strip_dir){
    auto aHisto = aEvent->get2DProjection(get2DProjectionType(strip_dir), filterType, scale_type::mm);
    wvalue aPanel = makeHeatmapPanel(showDeconvolved ? aTrackBuilder->getCluster2D(strip_dir) : *aHisto, true, true);
    aPanel["clickDir"] = strip_dir; // clicks define the segment end points
    PanelOverlay ov;
    addManualRecoOverlay(ov, strip_dir);
    if(isRecoModeOn && myRecoTool->canMovePoints()){ // points the user can drag: index, x, y
      std::vector<wvalue> dragPoints;
      const auto & points = myRecoTool->getPoints();
      for(size_t iPoint=0;iPoint<points.size();++iPoint){
        wvalue aPoint;
        aPoint["i"] = static_cast<long>(iPoint);
        aPoint["x"] = points.at(iPoint).p.at(strip_dir).x;
        aPoint["y"] = points.at(iPoint).p.at(strip_dir).y;
        dragPoints.push_back(std::move(aPoint));
      }
      aPanel["dragPoints"] = std::move(dragPoints);
    }
    // the track line is dropped once clicked points were moved / added (out of date until the next fit)
    if(isManualTrackShown && !isTrackLineOutdated) addTrackProjection(ov, strip_dir, aTrack);
    ov.moveTo(aPanel);
    panels.push_back(std::move(aPanel));
  }
  // charge along the fitted track (as in the devel mode), or the time projection while clicking
  if(isManualTrackShown) panels.push_back(makeChargeProfilePanel(aTrack));
  else {
    auto aHisto = aEvent->get1DProjection(projection_type::DIR_TIME, filterType, scale_type::mm);
    TH1D hDeconvolved(aTrackBuilder->getRecHitsTimeProjection());
    hDeconvolved.SetDirectory(nullptr);
    hDeconvolved.SetTitle("Time projection of the deconvolved clean cluster");
    wvalue aPanel = makeHist1DPanel(showDeconvolved ? static_cast<const TH1 &>(hDeconvolved) : *aHisto);
    PanelOverlay ov;
    for(const auto & item: myRecoTool->getTimeLines()){
      wvalue aLine;
      aLine["x"] = item.x;
      aLine["color"] = rootColor(item.color);
      aLine["width"] = 2;
      ov.vlines.push_back(std::move(aLine));
    }
    ov.moveTo(aPanel);
    // line following the mouse over the projections while the first point is being chosen
    if(myRecoTool->isAcceptingPoints() && !myRecoTool->getFixedTime()){
      aPanel["cursorLineColor"] = rootColor(myRecoTool->getCursorLineColor());
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
  const Track3D & aTrack = displayedTrack();
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

  // the automatically reconstructed track and a fitted clicked track (2-prong: the whole line,
  // 3-prong: the longest segment): the profile fitted by the dE/dx fit (with the model below);
  // otherwise (track read from the output file, failed fit): the first segment profile
  const TH1F & hdEdx = myEventSource->getTrackBuilder()->getdEdxHisto();
  const bool isClickedTrack = myClickedTrack && &aTrack==myClickedTrack.get();
  const bool isClicked3Prong = isClickedThreeProng && isClickedTrack;
  const bool isClickedWithFit = isClickedTrack && hdEdx.GetEntries()>0;
  bool isUnknownPID = !isClickedWithFit &&
                      (aTrack.getSegments().front().getPID()==pid_type::UNKNOWN ||
                       &aTrack!=&myEventSource->getTrackBuilder()->getTrack3D(0) || hdEdx.GetEntries()==0);
  TH1F hChargeProfile = isUnknownPID ? aTrack.getSegments().front().getChargeProfile() : hdEdx;
  // a track read from the output file has no geometry for the segment profile: its stored profile
  if(hChargeProfile.GetMaximum()<=0) hChargeProfile = aTrack.getChargeProfile();
  // clicked 3-prong track: the alphas from the vertex (ChargeProfileBuilder: short segments too,
  // normalised to 1), each in the colour of its segment; the longest one first (the fitted one)
  // (a clicked 2-prong track without a dE/dx fit: its longest segment, short ones too)
  ChargeProfileBuilder aBuilder;
  if(isClicked3Prong || (isClickedTrack && !isClickedWithFit)){
    hChargeProfile = aBuilder.getSegmentProfile(aTrack.getSegments().front());
    if(hChargeProfile.GetMaximum()>0) hChargeProfile.Scale(1.0/hChargeProfile.GetMaximum());
  }
  std::vector<double> x, y;
  for(int iBin=1;iBin<=hChargeProfile.GetNbinsX();++iBin){
    x.push_back(hChargeProfile.GetBinCenter(iBin));
    y.push_back(hChargeProfile.GetBinContent(iBin));
  }
  ov.markers.push_back(markers(x, y, rootColor(2), "circle", 6));
  // 3-prong: the alphas by length, alpha_1 the longest (fitted) one
  auto alphaName = [](size_t index){
    static const char * subscripts[] = {"₁", "₂", "₃", "₄", "₅", "₆", "₇", "₈", "₉"};
    return std::string("α") + (index<9 ? subscripts[index] : std::to_string(index+1).c_str());
  };
  if(isClicked3Prong) ov.markers.back()["name"] = alphaName(0);
  double maxContent = hChargeProfile.GetMaximum();
  if(isClicked3Prong){
    for(size_t iSegment=1;iSegment<aTrack.getSegments().size();++iSegment){
      TH1F hSegment = aBuilder.getSegmentProfile(aTrack.getSegments().at(iSegment));
      if(hSegment.GetMaximum()>0) hSegment.Scale(1.0/hSegment.GetMaximum());
      // shifted by its delta onto the fit of the longest alpha (when matched)
      const bool isMatched = isClickedWithFit && iSegment<myProfileMatches.size() && myProfileMatches[iSegment].valid;
      const double delta = isMatched ? myProfileMatches[iSegment].delta : 0.0;
      std::vector<double> sx, sy;
      for(int iBin=1;iBin<=hSegment.GetNbinsX();++iBin){
        if(isMatched && hSegment.GetBinCenter(iBin)<0) continue; // before its vertex: the other tracks
        sx.push_back(hSegment.GetBinCenter(iBin) + delta);
        sy.push_back(hSegment.GetBinContent(iBin));
      }
      ov.markers.push_back(markers(sx, sy, rootColor(2+iSegment), "circle", 6));
      ov.markers.back()["name"] = alphaName(iSegment) +
        (isMatched ? " (δ = "+format("%.1f", delta)+" mm)" : std::string());
      maxContent = std::max(maxContent, hSegment.GetMaximum());
    }
  }
  aPanel["xRange"] = range(-20, 20+aTrack.getLength());
  aPanel["yRange"] = range(0.0, 1.2*maxContent);

  if(isUnknownPID || (isClicked3Prong && !isClickedWithFit)){ // no model without a valid fit
    ov.moveTo(aPanel);
    return aPanel;
  }

  TF1 dEdx = myEventSource->getTrackBuilder()->getdEdx();
  if(dEdx.GetNpar() && !mySavedTrack){ // the fitted model belongs to the TrackBuilder track
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
    if(isClicked3Prong){ // alpha hypothesis of the longest alpha only
      ov.series.push_back(line(full.first, full.second, rootColor(1), 3, "solid", alphaName(0)+" fit"));
      ov.moveTo(aPanel);
      return aPanel;
    }
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
  if(aTrack==mySavedTrack.get()) out<<KYEL<<"Read from the output file."<<RST<<std::endl;
  out<<KBLU<<"Total length [mm]: "<<RST<<format("%3.0f", aTrack->getLength())<<std::endl;
  if(aTrack->getSegments().front().getPID()==pid_type::UNKNOWN){
    out<<KBLU<<"Particle PID: "<<RST<<"UNKNOWN"<<std::endl;
  }
  else{
    // energy of every segment from its range; ions without a range curve (e.g. a dot like
    // track) are counted as alphas, as before
    double totalEnergy = 0.0;
    std::string particles;
    for(const auto & aSegment: aTrack->getSegments()){
      pid_type aPID = aSegment.getPID();
      particles += (particles.empty() ? "" : " + ") + enumDict::GetPidName(aPID);
      bool hasRangeCurve = aPID==pid_type::ALPHA || aPID==pid_type::CARBON_12 ||
                           aPID==pid_type::CARBON_14 || aPID==pid_type::PROTON;
      totalEnergy += myRangeCalculator.getIonEnergyMeV(hasRangeCurve ? aPID : pid_type::ALPHA,
                                                       aSegment.getLength());
    }
    out<<KBLU<<"Particles: "<<RST<<particles<<std::endl;
    // 2-prong: ranges from the dE/dx fit
    const auto & aClickeddEdx = myEventSource->getTrackBuilder()->getClickeddEdx();
    if(!isClickedThreeProng && aTrack==myClickedTrack.get() && aClickeddEdx.eventType!=pid_type::UNKNOWN){
      out<<KBLU<<"From dE/dx ("<<RST<<aClickeddEdx.eventType<<KBLU<<"): alpha range [mm]: "<<RST
         <<format("%.1f", aClickeddEdx.alphaRange);
      if(aClickeddEdx.eventType==pid_type::C12_ALPHA) out<<KBLU<<", carbon range [mm]: "<<RST<<format("%.1f", aClickeddEdx.carbonRange);
      out<<std::endl;
    }
    // 3-prong: lengths of the shorter alphas from dE/dx (matched to the longest one)
    if(isClickedThreeProng && aTrack==myClickedTrack.get()){
      for(size_t iSegment=1;iSegment<myProfileMatches.size() && iSegment<aTrack->getSegments().size();++iSegment){
        const auto & aMatch = myProfileMatches[iSegment];
        if(!aMatch.valid) continue;
        out<<KBLU<<"Alpha "<<iSegment+1<<" length [mm]: "<<RST<<format("%.1f", aTrack->getSegments().at(iSegment).getLength())
           <<KBLU<<", from dE/dx: "<<RST<<format("%.1f", aMatch.length)
           <<KBLU<<" (delta "<<RST<<format("%.1f", aMatch.delta)<<KBLU<<" mm)"<<RST<<std::endl;
      }
    }
    out<<KBLU<<"Total E [MeV]: "<<RST<<format("%2.1f", totalEnergy)<<std::endl;
  }
  out<<std::endl<<KBLU<<"Reconstructed track: "<<RST<<std::endl;
  out<<myEventSource->getCurrentEvent()->GetEventInfo()<<std::endl;
  out<<*aTrack<<std::endl;
  return out.str();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
crow::json::wvalue WebGUIController::listDataSets() const {

  wvalue aResult;
  auto split = [](const std::string & text){
    std::vector<std::string> items;
    std::stringstream aStream(text);
    std::string item;
    while(std::getline(aStream, item, ',')) if(item.size()) items.push_back(item);
    return items;
  };
  // data-set JSON files (event lists, runs) in web.dataSetDirs
  std::vector<wvalue> lists, runSets;
  std::set<std::string> runSetFiles; // first files of the run data sets: not listed again below
  std::vector<std::string> jsonFiles;
  std::error_code ec;
  for(const auto & setDir: split(myConfig.get<std::string>("web.dataSetDirs", ""))){
    if(!fs::is_directory(setDir, ec)) continue;
    std::vector<std::string> dirFiles;
    for(const auto & entry: fs::directory_iterator(setDir, ec)){
      if(entry.path().extension()==".json") dirFiles.push_back(entry.path().string());
    }
    std::sort(dirFiles.begin(), dirFiles.end());
    jsonFiles.insert(jsonFiles.end(), dirFiles.begin(), dirFiles.end());
  }
  for(const auto & jsonFile: jsonFiles){
    wvalue item;
    item["path"] = jsonFile;
    item["name"] = fs::path(jsonFile).stem().string();
    const fs::path baseDir = fs::path(jsonFile).parent_path();
    auto relativeTo = [&baseDir](const std::string & path){
      return path.empty() || fs::path(path).is_absolute() ? path : (baseDir/path).string();
    };
    bool isRun = false;
    try {
      boost::property_tree::ptree aDataSet;
      boost::property_tree::read_json(jsonFile, aDataSet);
      item["name"] = aDataSet.get<std::string>("name", fs::path(jsonFile).stem().string());
      item["description"] = aDataSet.get<std::string>("description", "");
      item["geometry"] = fs::path(aDataSet.get<std::string>("input.geometryFile", "")).filename().string();
      item["pressure"] = aDataSet.get<std::string>("conditions.pressure", "");
      item["energy"] = aDataSet.get<std::string>("beamParameters.energy", "");
      const std::string listFile = relativeTo(aDataSet.get<std::string>("input.eventList", ""));
      isRun = listFile.empty();
      if(!isRun) item["events"] = static_cast<long>(readEventList(listFile).size());
      else {
        item["group"] = aDataSet.get<std::string>("run.campaign", baseDir.filename().string());
        item["runType"] = aDataSet.get<std::string>("run.run_type", "");
        item["quality"] = aDataSet.get<std::string>("run.quality", "");
        item["chunks"] = aDataSet.get<std::string>("run.n_chunks_found", "");
        const std::string files = aDataSet.get<std::string>("input.dataFile", "");
        const std::string firstFile = relativeTo(files.substr(0, files.find(',')));
        if(files.empty()) item["error"] = "no data on disk";
        else if(aDataSet.get<std::string>("input.geometryFile", "").empty()) item["error"] = "no geometry file for this working point";
        else if(!fs::is_regular_file(firstFile, ec)) item["error"] = "no such file: "+firstFile;
        else runSetFiles.insert(fs::weakly_canonical(firstFile, ec).string());
      }
    }
    catch(const std::exception & e){ item["error"] = e.what(); }
    (isRun ? runSets : lists).push_back(std::move(item));
  }
  aResult["dataSetDirs"] = myConfig.get<std::string>("web.dataSetDirs", "");
  aResult["eventLists"] = std::move(lists);
  aResult["runSets"] = std::move(runSets);

  // other GRAW runs in web.grawDataDirs (default: the directory of the event list or the one
  // above the input files)
  std::vector<std::string> dataDirs = split(myConfig.get<std::string>("web.grawDataDirs", ""));
  if(dataDirs.empty() && myEventListDataDir.size()) dataDirs.push_back(myEventListDataDir);
  if(dataDirs.empty() && myEventSource){
    fs::path current = myEventSource->getCurrentPath();
    current = current.string().substr(0, current.string().find(','));
    if(current.has_parent_path()) dataDirs.push_back(current.parent_path().parent_path().string());
  }
  std::vector<wvalue> runs;
  for(const auto & dataDir: dataDirs){
    for(const auto & aRun: findGrawRuns(dataDir)){
      if(aRun.firstChunk.size() && runSetFiles.count(fs::weakly_canonical(aRun.firstChunk.front(), ec).string())) continue;
      wvalue item;
      item["dataDir"] = dataDir;
      item["group"] = fs::path(aRun.directory).filename().string();
      item["runId"] = static_cast<long long>(aRun.runId);
      item["start"] = aRun.startTime;
      item["streams"] = aRun.nStreams;
      item["chunks"] = aRun.nChunks;
      std::string files;
      for(const auto & aFile: aRun.firstChunk) files += (files.empty() ? "" : ",") + aFile;
      item["files"] = files;
      runs.push_back(std::move(item));
    }
  }
  std::vector<wvalue> dirs;
  for(const auto & item: dataDirs) dirs.emplace_back(item);
  aResult["dataDirs"] = std::move(dirs);
  aResult["runs"] = std::move(runs);

  return aResult;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
