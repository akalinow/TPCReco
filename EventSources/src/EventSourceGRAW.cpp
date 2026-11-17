#include <cstdlib>
#include <iostream>
#include <ostream>
#include <iomanip>
#include <iterator>
#include <map>
#include <cstdint>



#include "TPCReco/EventSourceGRAW.h"
#include "TPCReco/RunIdParser.h"
#include "TPCReco/colorText.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceGRAW::EventSourceGRAW(const std::string & geometryFileName):EventSourceBase() {

  loadGeometry(geometryFileName);
  GRAW_EVENT_FRAGMENTS = myGeometryPtr->GetAsadNboards();
  myPedestalCalculator.SetGeometryAndInitialize(myGeometryPtr);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceGRAW::~EventSourceGRAW() {}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> EventSourceGRAW::getNextEvent(){

  // nothing loaded yet (as in EventSourceROOT::getNextEvent): start from the first entry
  if(myCurrentEntry==std::numeric_limits<unsigned long int>::max()){
    if(nEvents>0) loadFileEntry(0);
    return myCurrentEvent;
  }

  auto currentEventId = myCurrentEvent->GetEventInfo().GetEventId();
  auto it = myFramesMap.find(currentEventId);
  if(it==myFramesMap.end() || it->second.empty()) return myCurrentEvent;
  unsigned int lastEventFrame = *it->second.rbegin();
  if(lastEventFrame<nEvents-1) ++lastEventFrame;
  loadFileEntry(lastEventFrame);
  return myCurrentEvent;  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> EventSourceGRAW::getPreviousEvent(){

  auto currentEventId = myCurrentEvent->GetEventInfo().GetEventId();
  auto it = myFramesMap.find(currentEventId);
  unsigned int startingEventIndexFrame = *it->second.begin();
  if(startingEventIndexFrame>0) --startingEventIndexFrame; 
  loadFileEntry(startingEventIndexFrame);
  return myCurrentEvent;  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceGRAW::loadDataFile(const std::string & fileName){

  EventSourceBase::loadDataFile(fileName);

  if(!myFile.open(fileName)){
    std::cerr<<KRED<<"Can not open file: "<<fileName<<"!"<<RST<<std::endl;
    exit(1);
  }
  nEvents = myFile.nFrames();
  if(nEventsToRead<0) nEventsToRead = nEvents;
  
  const int firstEventSize=10;
  if(fileName!=myFilePath || nEvents<firstEventSize){
    findStartingIndex(firstEventSize);
  }

  myFilePath = fileName;
  myNextFilePath = getNextFilePath();
  myFramesMap.clear();
  myASADMap.clear();
  myReadEntriesSet.clear();
  isFullFileScanned = false;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool EventSourceGRAW::loadGrawFrame(unsigned int iEntry, bool readFullEvent){

  std::string tmpFilePath = myFilePath;
#ifndef EVENTSOURCEGRAW_NEXT_FILE_DISABLE  
  if(iEntry>=nEvents){
    tmpFilePath = myNextFilePath;
    iEntry -= nEvents;
  }
  bool dataFrameRead = readFrame(tmpFilePath, iEntry, readFullEvent);

  if(!dataFrameRead){
    std::cerr <<KRED<<std::endl<<"ERROR: cannot read file entry: " <<RST<<iEntry<<std::endl
	      <<KRED<<"from file: "<<std::endl
	      <<RST<<tmpFilePath
	      << std::endl;
    std::cerr <<KRED
	      <<"Please check if the data file is missing or damaged."
	      <<RST
	      <<std::endl;
    exit(1);
  }
#else
  bool dataFrameRead = false;
  if(iEntry<nEvents) {
    dataFrameRead = readFrame(tmpFilePath, iEntry, readFullEvent);
  }
    
  if(!dataFrameRead){
    std::cerr <<KRED<<std::endl<<"ERROR: cannot read file entry: " <<RST<<iEntry<<std::endl
	      <<KRED<<"from file: "<<std::endl
	      <<RST<<tmpFilePath
	      << std::endl;
  }
#endif
  
  return dataFrameRead;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceGRAW::loadEventId(unsigned long int eventId){

  std::cout<<KBLU
	   <<"Start looking for the event id: "<<eventId
	   <<RST<<std::endl;
  
  auto it = myFramesMap.find(eventId);

  if(!isFullFileScanned && it!=myFramesMap.end() &&
     it->second.size()<GRAW_EVENT_FRAGMENTS){
    unsigned int iEntry =  *it->second.rbegin();
    findEventFragments(eventId, iEntry);
  }
  else if(!isFullFileScanned && it==myFramesMap.end() ){
    findEventFragments(eventId, GRAW_EVENT_FRAGMENTS *( eventId- startingEventIndex));
  }
  collectEventFragments(eventId);

  std::cout<<KBLU
	   <<"Finished looking for the event id: "<<eventId
	   <<RST<<std::endl;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceGRAW::loadFileEntry(unsigned long int iEntry){

  myCurrentEntry = iEntry;
  loadGrawFrame(iEntry, false);
  
  unsigned long int eventId = myDataFrame.header.eventIdx;

  std::cout<<KBLU
	   <<"Looking for event fragments from file entry id: "<<RST<<iEntry
	   <<KBLU<<" event id: "<<RST<<eventId
	   <<std::endl;
  
  if(!isFullFileScanned &&
     (myFramesMap.find(eventId)==myFramesMap.end() ||
      myFramesMap.find(eventId)->second.size()<GRAW_EVENT_FRAGMENTS)){
    findEventFragments(eventId, iEntry);
  }
  collectEventFragments(eventId);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceGRAW::checkEntryForFragments(unsigned int iEntry){

  if(myReadEntriesSet.count(iEntry)) return;
  loadGrawFrame(iEntry, false);
  myReadEntriesSet.insert(iEntry);
  unsigned long int currentEventId = myDataFrame.header.eventIdx;
  int ASAD_idx = myDataFrame.header.asadIdx;  
  if(!myASADMap[currentEventId].count(ASAD_idx)) {
    myFramesMap[currentEventId].insert(iEntry);
    myASADMap[currentEventId].insert(ASAD_idx);
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceGRAW::findEventFragments(unsigned long int eventId, unsigned int iInitialEntry){

  auto it=myFramesMap.find(eventId);
  unsigned int nFragments = 0;
  if(it!=myFramesMap.end()){
    nFragments = it->second.size();
  }

  //bool reachStartOfFile = false;
  //bool reachEndOfFile = false;
  
  unsigned int lowEndScanRange = 0;
  if(iInitialEntry>frameLoadRange){
    lowEndScanRange = iInitialEntry-frameLoadRange;
  }  
  unsigned int highEndScanRange = std::min((unsigned int)nEvents, iInitialEntry+frameLoadRange);
  
  for(unsigned int iEntry=iInitialEntry;
      iEntry>=lowEndScanRange && iEntry<nEvents && nFragments<GRAW_EVENT_FRAGMENTS;
      --iEntry){
    checkEntryForFragments(iEntry);
    nFragments =  myFramesMap[eventId].size();
    //reachStartOfFile = (iEntry==0);
    std::cout<<"\r going backward and reading file entry: "<<iEntry
	     <<" fragments found so far: "
	     <<nFragments
	     <<" expected: "<<GRAW_EVENT_FRAGMENTS << "     ";
  }
  for(unsigned int iEntry=iInitialEntry;iEntry<highEndScanRange && nFragments<GRAW_EVENT_FRAGMENTS;++iEntry){
    checkEntryForFragments(iEntry);
    nFragments =  myFramesMap[eventId].size();
    //reachEndOfFile = (iEntry==nEvents);
    std::cout<<"\r going forward and reading file entry: "<<iEntry
	     <<" fragments found so far: "
	     <<nFragments
	     <<" expected: "<<GRAW_EVENT_FRAGMENTS << "     ";
  }
  for(unsigned int iEntry=0;iEntry<frameLoadRange && nFragments<GRAW_EVENT_FRAGMENTS;++iEntry){
    checkEntryForFragments(iEntry+nEvents);
    nFragments =  myFramesMap[eventId].size();
    std::cout<<"\r going to next run file and reading file entry: "<<iEntry
	     <<" fragments found so far: "
	     <<nFragments
	     <<" expected: "<<GRAW_EVENT_FRAGMENTS << "     ";
  }
  
  std::cout<<std::endl;
  if(myFramesMap.size()>=nEvents) isFullFileScanned = true;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceGRAW::collectEventFragments(unsigned int eventId){

  auto it = myFramesMap.find(eventId);
  if(it==myFramesMap.end()) return;
  if(it->second.size()!=GRAW_EVENT_FRAGMENTS){
      std::cerr<<KRED<<"Fragment counts for eventId = "<<RST<<eventId
	       <<KRED<<" mismatch. Expected: "<<RST<<GRAW_EVENT_FRAGMENTS
	       <<KRED<<" found: "<<RST<<it->second.size()
	       <<RST<<std::endl;
  }
  //long int eventNumberInFile = std::distance(myFramesMap.begin(), it);  
  myCurrentPEvent->Clear();
  std::cout<<KYEL<<"Creating a new PEventTPC/Raw with eventId: "<<eventId<<RST<<std::endl;
  std::set<int> asadCounter;

  RunIdParser runParser(myFilePath);
  myCurrentEventInfo.SetRunId(runParser.runId());
  
  for(auto aFragment: it->second){
    loadGrawFrame(aFragment, true);
    int  ASAD_idx = myDataFrame.header.asadIdx;
    unsigned long int eventId_fromFrame = myDataFrame.header.eventIdx;
    asadCounter.insert(ASAD_idx);

    myCurrentEventInfo.SetEventId(eventId);
    myCurrentEventInfo.SetRunId(eventId);
    myCurrentEventInfo.SetEventTimestamp(myDataFrame.header.eventTime);
    std::cout<<myCurrentEventInfo<<std::endl;
    
    if(eventId!=eventId_fromFrame){
      std::cerr<<KRED<<__FUNCTION__
	       <<": Event id mismatch!: eventId="<<eventId
	       <<", eventId_fromFrame="<<eventId_fromFrame
	       <<RST<<std::endl;
      return;
    }     
    std::cout<<KBLU<<"Found a frame for eventId: "<<RST<<eventId;
    if(aFragment<nEvents) std::cout<<KBLU<<" in file entry: "<<RST<<aFragment<<RST;
    else std::cout<<KBLU<<" in next file entry: "<<RST<<aFragment-nEvents<<RST;
    std::cout<<KBLU<<" for  ASAD: "<<RST<<ASAD_idx<<RST<<std::endl;
    if(fillEventType==EventType::tpc) fillEventFromFrame(myDataFrame);
    else if(fillEventType==EventType::raw) fillEventRawFromFrame(myDataFrame);
  }
  fillEventTPC();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceGRAW::fillEventFromFrame(graw::Frame & aGrawFrame){

  if(removePedestal) myPedestalCalculator.CalculateEventPedestals(aGrawFrame);
  myCurrentEventInfo.SetPedestalSubtracted(removePedestal);

  int  COBO_idx = aGrawFrame.header.coboIdx;
  int  ASAD_idx = aGrawFrame.header.asadIdx;

  if(ASAD_idx >= myGeometryPtr->GetAsadNboards()){
    std::cout<<KRED<<__FUNCTION__
	     <<": Data format mismatch! ASAD="<<ASAD_idx
	     <<", number of ASAD boards in geometry="<<myGeometryPtr->GetAsadNboards()
	     <<". Frame skipped."
	     <<RST<<std::endl;
    return;
  }
  
  for (Int_t agetId = 0; agetId < myGeometryPtr->GetAgetNchips(); ++agetId){
    for (Int_t chanId = 0; chanId < myGeometryPtr->GetAgetNchannels(); ++chanId){
      unsigned int rawChanId = myGeometryPtr->Aget_normal2raw(chanId);
      if(!aGrawFrame.hasChannel(agetId, rawChanId)) continue;
      std::shared_ptr<StripTPC> aStrip(myGeometryPtr->GetStripByAget(COBO_idx, ASAD_idx, agetId, chanId));
      if(!aStrip) continue;
      aGrawFrame.forEachSample(agetId, rawChanId, [&](unsigned int icell, uint16_t rawVal){
	// skip cells outside signal time-window
	if(icell<2 || icell>509 ||
	   (int)icell<myPedestalCalculator.GetMinSignalCell() ||
	   (int)icell>myPedestalCalculator.GetMaxSignalCell()) return;
	Double_t corrVal = rawVal;
	if(removePedestal){
	  corrVal -= myPedestalCalculator.GetPedestalCorrection(COBO_idx, ASAD_idx, agetId, chanId, icell);
	}
	myCurrentPEvent->AddValByStrip(aStrip, icell, corrVal);
      });
    }
  }
  myCurrentPEvent->SetEventInfo(myCurrentEventInfo);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceGRAW::fillEventRawFromFrame(graw::Frame & aGrawFrame){

  // fills raw data container class per: COBO, ASAD, AGET, CHANNEL_RAW, TIME_CELL 
  // optimized for data size
  // no pedestal subtracion (can be done later using raw data stored in the container class

  myCurrentEventRaw->SetEventId((uint64_t)aGrawFrame.header.eventIdx);
  myCurrentEventRaw->SetEventTimestamp(aGrawFrame.header.eventTime);
  
  uint8_t COBO_idx = (uint8_t)aGrawFrame.header.coboIdx;
  uint8_t ASAD_idx = (uint8_t)aGrawFrame.header.asadIdx;
  if(ASAD_idx >= myGeometryPtr->GetAsadNboards()){
    std::cout<<KRED<<__FUNCTION__
	     <<": Data format mismatch! ASAD="<<ASAD_idx
	     <<", number of ASAD boards in geometry="<<myGeometryPtr->GetAsadNboards()
	     <<". Frame skipped."
	     <<RST<<std::endl;
    return;
  }

  // reset EventRaw.channelData for given {COBO, ASAD} pair
  eventraw::AgetRawMap_t::iterator a_it;
  for(a_it=(myCurrentEventRaw->data).begin(); a_it!=(myCurrentEventRaw->data).end(); a_it++) {
    MultiKey2 aKey(COBO_idx, ASAD_idx);
    if(std::get<0>(a_it->first)==COBO_idx &&
       std::get<1>(a_it->first)==ASAD_idx) (a_it->second).channelData.resize(0);
  }
  
  // temporary map of AGET channels
  std::map< MultiKey2, eventraw::ChannelRaw> map2; // index={agetIdx[0-3], channelIdx[0-67]}, val=ChannelRaw
  std::map< MultiKey2, eventraw::ChannelRaw>::iterator map2_it;

  for(uint8_t AGET_idx=0; AGET_idx<graw::kAgetsPerAsad; ++AGET_idx) {
    for(uint8_t CHAN_idx=0; CHAN_idx<graw::kChannelsPerAget; ++CHAN_idx) {
      if(!aGrawFrame.hasChannel(AGET_idx, CHAN_idx)) continue;

      // filling ChannelRaw with samples ordered by cell index [0-511]
      eventraw::ChannelRaw c;
      aGrawFrame.forEachSample(AGET_idx, CHAN_idx, [&c](unsigned int icell, uint16_t rawVal){
	c.cellMask[ icell/8 ] |= (1 << (icell % 8)); // update bit mask
	c.cellData.push_back(rawVal);
      });

      // adding ChannelRaw to temporary AGET map
      MultiKey2 mkey(AGET_idx, CHAN_idx);
      map2.insert( std::pair< MultiKey2, eventraw::ChannelRaw >( mkey, c ) );
    }
  }
  
  // filling AgetRaw from temporary AGET map
  // NOTE: map2 is sorted by KEY={aget[0-3], chan[0-67]}
  for(map2_it=map2.begin(); map2_it!=map2.end(); map2_it++) {

    uint8_t AGET_idx = std::get<0>(map2_it->first);
    uint8_t CHAN_idx = std::get<1>(map2_it->first);
    MultiKey3_uint8 mkey(COBO_idx, ASAD_idx, AGET_idx);

    // add new AGET to map if necessary
    if( (a_it=(myCurrentEventRaw->data).find(mkey))==(myCurrentEventRaw->data).end()) {
      eventraw::AgetRaw a;
      a_it=std::get<0>((myCurrentEventRaw->data).insert( std::pair< MultiKey3_uint8, eventraw::AgetRaw >(mkey, a)));
    }
    // fill data for already existing AGET
    (a_it->second).channelMask[ CHAN_idx/8 ] |= (1 << (CHAN_idx % 8)); // update bit mask
    (a_it->second).channelData.push_back(map2_it->second); // correct order should be preserved for map2 sorted by KEY

#ifdef DEBUG
    //// DEBUG
    std::cout << __FUNCTION__
	      << ": Adding channel: coboIdx=" << (int)COBO_idx << ", asadIdx=" << (int)ASAD_idx << ", agetIdx=" << (int)AGET_idx << ", chanIdx=" << (int)CHAN_idx << std::endl;
    //// DEBUG
#endif
  }
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool EventSourceGRAW::readFrame(const std::string & filePath, unsigned int iEntry, bool readFullEvent){

  graw::GrawFile *aFile = &myFile;
  if(filePath!=myFile.path()){
    if(filePath!=myNextFile.path() && !myNextFile.open(filePath)) return false;
    aFile = &myNextFile;
  }
  if(readFullEvent) return aFile->readFrame(iEntry, myDataFrame);
  return aFile->readHeader(iEntry, myDataFrame.header);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceGRAW::findStartingIndex(unsigned long int size){
  if(nEvents==0){
    startingEventIndex=0;
  } else {
    auto preloadSize=std::min(nEvents,size);
    startingEventIndex=std::numeric_limits<UInt_t>::max();
    for(unsigned long int i=0; i<preloadSize; ++i){
      myFile.readHeader(i, myDataFrame.header);
      startingEventIndex=std::min(startingEventIndex, static_cast<unsigned long int>(myDataFrame.header.eventIdx));
    }
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceGRAW::configurePedestal(const boost::property_tree::ptree &config){
  auto parser=[this, &config](std::string &&parameter, void (PedestalCalculator::*setter)(int)){
    if(config.find(parameter)!=config.not_found()){
      (this->myPedestalCalculator.*setter)(config.get<int>(parameter));
    }
  };
  
  removePedestal = config.get<bool>("remove");
  
  parser("minPedestalCell", &PedestalCalculator::SetMinPedestalCell);
  parser("maxPedestalCell", &PedestalCalculator::SetMaxPedestalCell);
  parser("minSignalCell", &PedestalCalculator::SetMinSignalCell);
  parser("maxSignalCell", &PedestalCalculator::SetMaxSignalCell);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string EventSourceGRAW::getNextFilePath(){

#ifdef EVENTSOURCEGRAW_NEXT_FILE_DISABLE  
  return myFilePath;
#endif

  std::string token = "_";
  int index = myFilePath.rfind(token);
  int fieldLength = 4;
  std::string fileIndex = myFilePath.substr(index+token.size(), fieldLength);
  int nextFileIndex = std::stoi(fileIndex) + 1;  
  ///
  token = "_";
  index = myFilePath.rfind(token);
  std::string fileNamePrefix = myFilePath.substr(0,index+token.size());
  ///
  std::ostringstream ostr;
  ostr<<fileNamePrefix<<std::setfill('0') <<std::setw(4)<<nextFileIndex<<".graw";
  std::string nextFilePath = ostr.str();
  return nextFilePath;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
