#include <cstdlib>
#include <iostream>
#include <ostream>
#include <iomanip>
#include <iterator>
#include <map>
#include <cstdint>
#include <string>
#include <vector>
#include <sstream>


#include "TPCReco/RunIdParser.h"
#include "TPCReco/EventSourceMultiGRAW.h"
#include "TPCReco/colorText.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceMultiGRAW::EventSourceMultiGRAW(const std::string & geometryFileName) : EventSourceGRAW(geometryFileName) {}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceMultiGRAW::~EventSourceMultiGRAW(){}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> EventSourceMultiGRAW::getNextEvent(){

  myDataFrame.clear();

  auto currentEventId = myCurrentEvent->GetEventInfo().GetEventId();
  for( unsigned int streamIndex=0; streamIndex<myFramesMapList.size(); streamIndex++ ) {
    auto it = myFramesMapList[streamIndex].find(currentEventId);
    auto it2 = myAsadMapList[streamIndex].find(currentEventId);
    auto it3 = myCoboMapList[streamIndex].find(currentEventId);
    if( it==myFramesMapList[streamIndex].end() || it2==myAsadMapList[streamIndex].end() || it3==myCoboMapList[streamIndex].end() ) continue;
    unsigned int lastEventFrame = it->second;
    int ASAD_idx = it2->second;
    int COBO_idx = it3->second;

    // use only {ASAD=0, COBO=0} for frame counting purpose
    if( ASAD_idx==0 && COBO_idx==0) {
      if(lastEventFrame<nEvents-1) ++lastEventFrame;
      loadFileEntry(lastEventFrame);
      break;
    }
  }
  return myCurrentEvent;  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> EventSourceMultiGRAW::getPreviousEvent(){

  auto currentEventId = myCurrentEvent->GetEventInfo().GetEventId();  
  for( unsigned int streamIndex=0; streamIndex<myFramesMapList.size() ; streamIndex++ ) {
    auto it = myFramesMapList[streamIndex].find(currentEventId);
    auto it2 = myAsadMapList[streamIndex].find(currentEventId);
    auto it3 = myCoboMapList[streamIndex].find(currentEventId);
    if( it==myFramesMapList[streamIndex].end() || it2==myAsadMapList[streamIndex].end() || it3==myCoboMapList[streamIndex].end() ) continue;
    unsigned int startingEventIndexFrame = it->second;
    int ASAD_idx = it2->second;
    int COBO_idx = it3->second;

    // use only {ASAD=0, COBO=0} for frame counting purpose
    if( ASAD_idx==0 && COBO_idx==0) {
      if(startingEventIndexFrame>0) --startingEventIndexFrame; 
      loadFileEntry(startingEventIndexFrame);
      break;
    }
  }
  return myCurrentEvent;  
}
/////////////////////////////////////////////////////////
// Modified to accept a list of files separated by "," delimeter.
/////////////////////////////////////////////////////////
void EventSourceMultiGRAW::loadDataFile(const std::string & commaSeparatedFileNames){

  const char del = ','; // delimiter character
  std::set<std::string> fileNameList; // list of unique strings
  std::stringstream sstream(commaSeparatedFileNames);
  std::string fileName;
  while (std::getline(sstream, fileName, del)) {
    if(fileName.size()) fileNameList.insert(fileName);
  };

  // display warning for incomplete lists, but allow to proceed (this is important for ONLINE mode!)
  if (fileNameList.size() != GRAW_EVENT_FRAGMENTS) {
    std::cerr << KRED << __FUNCTION__
	      << ": WARNING: Provided wrong number of GRAW files. Expected  " << RST
	      << GRAW_EVENT_FRAGMENTS
	      << KRED << ". The file list is:" << RST << std::endl
	      << commaSeparatedFileNames << std::endl;
  }

  if(fileNameList.size()) {
    loadDataFileList(fileNameList);
  }
}  
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceMultiGRAW::loadDataFileList(const std::set<std::string> & fileNameList){

  myFileList.clear();
  myNextFileList.clear();
  myFilePathList.clear();
  myNextFilePathList.clear();
  myAsadMapList.clear();
  myCoboMapList.clear();
  myFramesMapList.clear();
  myReadEntriesSetList.clear();

  unsigned int streamIndex=0;
  for(auto fileName: fileNameList) {

    myFramesMapList.push_back(std::map<unsigned int, unsigned int>{});
    myAsadMapList.push_back(std::map<unsigned int, unsigned int>{});
    myCoboMapList.push_back(std::map<unsigned int, unsigned int>{});
    myReadEntriesSetList.push_back(std::set<unsigned int>{});

    EventSourceBase::loadDataFile(fileName);

    auto myFile = std::make_shared<graw::GrawFile>();
    if(!myFile->open(fileName)){
      std::cerr<<KRED<<__FUNCTION__
	       <<": ERROR: Can not open file: "<<RST<<fileName<<std::endl;
      exit(1);
    }
    myFileList.push_back(myFile);
    myNextFileList.push_back(std::make_shared<graw::GrawFile>());
    
    unsigned int nFrames = myFile->nFrames();
    if(streamIndex==0 || nFrames<nEvents) nEvents=nFrames; // take the lowest number of frames
    if(nEventsToRead<0) nEventsToRead = nEvents;
		     
    myFilePathList.push_back(fileName);
#ifdef EVENTSOURCEGRAW_NEXT_FILE_DISABLE  
    myNextFilePathList.push_back(fileName);
#else
    myNextFilePathList.push_back(getNextFilePath(streamIndex));
#endif

    streamIndex++;
  }
#ifdef DEBUG
  std::cout<<__FUNCTION__<<": Number of GRAW streams: "<<myFramesMapList.size()
	   <<". Expected: "<<GRAW_EVENT_FRAGMENTS
	   <<std::endl;
  std::cout<<__FUNCTION__<<": Number of events: "<<nEvents
	   <<std::endl;
#endif
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool EventSourceMultiGRAW::loadGrawFrame(unsigned int iEntry, bool readFullEvent, unsigned int streamIndex){

  #ifdef DEBUG
  std::cout<<__FUNCTION__<<KBLU<<": Start loading the file entry: "<<RST<<iEntry
	   <<KBLU<<" from the GRAW stream id: "<<RST<<streamIndex
	   <<KBLU<<" with option readFull="<<RST<<readFullEvent<<std::endl;
  #endif

  if(streamIndex>=myFilePathList.size() || streamIndex>=myNextFilePathList.size()) { 
    std::cerr<<KRED<<__FUNCTION__
	     <<": ERROR: wrong GRAW stream id: " <<RST<<streamIndex<<KRED<<" for file entry: "<<RST<<iEntry
	     <<std::endl;
    exit(1); // internal error!
  }
  std::string tmpFilePath = myFilePathList[streamIndex];

  //#ifdef DEBUG
  //  std::cout<<__FUNCTION__<<": AFTER tmpFilePath ---> stream="<<streamIndex<<", frame_check="<<iEntry<<", readFull="<<readFullEvent<<std::endl<<std::flush;
  //#endif

#ifndef EVENTSOURCEGRAW_NEXT_FILE_DISABLE  

  if(iEntry>=nEvents){
    tmpFilePath = myNextFilePathList[streamIndex];
    iEntry -= nEvents;
  }

  
  bool dataFrameRead = readStreamFrame(tmpFilePath, iEntry, readFullEvent, streamIndex); // HOTFIX!!!!! => fills myDataFrame 
  ///FIXME getGrawFrame counts frames from 1 (WRRR!)

  
#else
  bool dataFrameRead = false;

  if(iEntry<nEvents) {
    dataFrameRead = readStreamFrame(tmpFilePath, iEntry, readFullEvent, streamIndex); // HOTFIX!!!!! => fills myDataFrame
    ///FIXME getGrawFrame counts frames from 1 (WRRR!)
  } else {
    std::cerr <<KRED<<__FUNCTION__
	      <<": ERROR: File entry: " <<RST<<iEntry
	      <<KRED<<" is out of allowed range for file: "<<std::endl
	      <<RST<<tmpFilePath
	      << std::endl;
    return false;
  }
#endif

  if(!dataFrameRead){
    std::cerr <<KRED<<__FUNCTION__
	      <<": ERROR: cannot read file entry: " <<RST<<iEntry
	      <<KRED<<"from file: "<<std::endl
	      <<RST<<tmpFilePath
	      << std::endl;
    std::cerr <<KRED
	      <<"Please check if the data file is missing or damaged."
	      <<RST
	      <<std::endl;
    exit(1);
  }
  
  #ifdef DEBUG
  std::cout<<__FUNCTION__<<KBLU<<": Finished loading the file entry: "<<RST<<iEntry
	   <<KBLU<<" from the GRAW stream id: "<<RST<<streamIndex
	   <<KBLU<<" with option readFull="<<RST<<readFullEvent<<std::endl;
  #endif

  return dataFrameRead;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool EventSourceMultiGRAW::readStreamFrame(const std::string & filePath, unsigned int iEntry,
					   bool readFullEvent, unsigned int streamIndex){

  graw::GrawFile *aFile = myFileList.at(streamIndex).get();
  if(filePath!=aFile->path()){
    aFile = myNextFileList.at(streamIndex).get();
    if(filePath!=aFile->path() && !aFile->open(filePath)) return false;
  }
  if(readFullEvent) return aFile->readFrame(iEntry, myDataFrame);
  return aFile->readHeader(iEntry, myDataFrame.header);
}
/////////////////////////////////////////////////////////
// Checks all GRAW files for frames with eventId.
// On success uptades myCurrentEvent object.
/////////////////////////////////////////////////////////
void EventSourceMultiGRAW::loadEventId(unsigned long int eventId){

  #ifdef DEBUG
  std::cout<<__FUNCTION__<<KBLU
	   <<": Start looking for the event id: "<<RST<<eventId
	   <<std::endl;
  #endif

  /////////////// MC DEBUG - 12 May 2022
  //
  std::set<unsigned int> frameList; // ordered list of frames
  for( unsigned int streamIndex=0; streamIndex<myFramesMapList.size() ; streamIndex++) {
    auto it2 = myFramesMapList[streamIndex].find(eventId);
    if(it2!=myFramesMapList[streamIndex].end()) {
      auto eventIndexFrame=it2->second;
      frameList.insert(eventIndexFrame);
      std::cout <<__FUNCTION__<<KBLU<<": Found file entry: "<<RST<<eventIndexFrame<<KBLU<<" for the GRAW stream id: "<<RST<<streamIndex
		<<KBLU<<", event id: "<<RST<<eventId<<std::endl;
    }
  }
  // at least one frame matched eventId
  if(frameList.size()) {
    loadFileEntry(*frameList.begin());
    std::cout<<__FUNCTION__<<KBLU
	     <<": Finished looking for the event id: "<<RST<<eventId<<std::endl;
    return;
  }
  // find corresponding file entry for each GRAW file
  for( unsigned int streamIndex=0; streamIndex<myFramesMapList.size() ; streamIndex++) {

    // strategy 1: check frame index of the current event and calculate the offset

    // check map [eventId, frameIndex] for a given GRAW file
    auto it2=myFramesMapList[streamIndex].find(currentEventId());
    if(it2!=myFramesMapList[streamIndex].end()) {
      auto iEntry=it2->second+(eventId-currentEventId()); // this works in NGRAW mode

      if(iEntry<0 || iEntry>=nEvents) {
	std::cout <<__FUNCTION__<<KRED<<": Event id: "<<RST<<eventId<<KBLU<<" seems to be out of range for the GRAW stream id: "
		  <<RST<<streamIndex<<std::endl;
      } else {
	checkEntryForFragments(iEntry, streamIndex);
	// check map [eventId, frameIndex] for a given GRAW file
        auto it2 = myFramesMapList[streamIndex].find(eventId);
        if(it2!=myFramesMapList[streamIndex].end()) {
          unsigned int eventIndexFrame = it2->second;
          std::cout <<__FUNCTION__<<KBLU<<": Found file entry: "<<RST<<eventIndexFrame<<KBLU<<" for the GRAW stream id: "<<RST<<streamIndex
		    <<KBLU<<", event id: "<<RST<<eventId<<std::endl;
	  loadFileEntry(eventIndexFrame);
	  std::cout<<__FUNCTION__<<KBLU
		   <<": Finished looking for the event id: "<<RST<<eventId<<std::endl;
	  return;
	}
      }
    }
  }
  std::cout<<__FUNCTION__<<KRED
	   <<": Finished looking for the event id: "<<RST<<eventId
	   <<KRED<<", no fragments found!"<<RST<<std::endl;
}
/////////////////////////////////////////////////////////
// Finds eventId corresponding to a given frame index of the GRAW file for {COBO=0, ASAD=0}.
// Checks remaining GRAW files for frames with this eventId.
// On success uptades myCurrentEvent object.
/////////////////////////////////////////////////////////
void EventSourceMultiGRAW::loadFileEntry(unsigned long int iEntry){

  #ifdef DEBUG
  std::cout<<__FUNCTION__<<KBLU
	   <<": Start looking for the file entry: "<<RST<<iEntry
	   <<std::endl;
  #endif

  bool result = false;
  unsigned long int matchEventId = 0;
  unsigned int matchStreamIndex = 0;
  
  ///////////////////////////////////////////////////////////////
  //  std::cout<<__FUNCTION__<<": myReadEntriesSetList.size="<<myReadEntriesSetList.size()
  //	   <<RST<<std::endl<<std::flush;
  ///////////////////////////////////////////////////////////////
  
  // find eventId for {ASAD=0, COBO=0}
  for( unsigned int streamIndex=0; streamIndex<myReadEntriesSetList.size() ; streamIndex++) {
    checkEntryForFragments(iEntry, streamIndex);
    // check map [eventId, frameIndex] for a given GRAW file
    for(auto const &it: myFramesMapList[streamIndex]) {
      if( it.second == iEntry ) {
	auto EventId= it.first;
	auto it2 = myAsadMapList[streamIndex].find(EventId);
	auto it3 = myCoboMapList[streamIndex].find(EventId);

	if(it2==myAsadMapList[streamIndex].end() || it3==myCoboMapList[streamIndex].end() ) continue;
	int ASAD_idx = it2->second;
	int COBO_idx = it3->second;
	// use only {ASAD=0, COBO=0} for frame counting purpose
	if( ASAD_idx==0 && COBO_idx==0) {
	  matchEventId=EventId;
	  matchStreamIndex=streamIndex;
	  result = true;
	  break;
	}
      }
    }
    if(result) break;    
  }
  if(!result) {
    std::cerr <<KRED<<__FUNCTION__
	      <<": WARNING: cannot find any GRAW streams with {ASAD=0, COBO=0} for the file entry: "<<RST<<iEntry
	      <<std::endl;
    return;
  } 
  // check frames corresponding to matched event ID
  for(unsigned int streamIndex=0; streamIndex<myReadEntriesSetList.size() ; streamIndex++) {
    if(streamIndex!=matchStreamIndex) {
      auto it = myFramesMapList[streamIndex].find(matchEventId);
      if(it!=myFramesMapList[streamIndex].end()) continue; 

      // strategy 1: check same frame index for matched event ID
      checkEntryForFragments(iEntry, streamIndex);
      if(myFramesMapList[streamIndex].find(matchEventId)!=myFramesMapList[streamIndex].end()) continue; // break;
      
      // strategy 2: scan all remaining frames for matched event ID
      //      for(unsigned long int iEntry2=0; iEntry2<=nEvents-1; ++iEntry2) {
      //	checkEntryForFragments(iEntry2, streamIndex);
      //	if(myFramesMapList[streamIndex].find(matchEventId)!=myFramesMapList[streamIndex].end()) continue; // break;
      //      }
      // strategy 2A: scan all remaining frames in the following order: n+1, n-1, n+2, n-2, etc
      long int deltaEntry=1;
      unsigned long int maxTries=2*nEvents, iTries=0;
      bool minFlag=false, maxFlag=false;
      while((++iTries)<maxTries && (!minFlag || !maxFlag)) {
	auto iEntry2=(long int)(iEntry+deltaEntry);
	deltaEntry=(deltaEntry < 0 ? abs(deltaEntry)+1 : -deltaEntry);
	if(iEntry2<0) { minFlag=true; continue; }
	if(iEntry2>=(long int)nEvents) { maxFlag=true; continue; }
	checkEntryForFragments((unsigned long int)iEntry2, streamIndex);
	if(myFramesMapList[streamIndex].find(matchEventId)!=myFramesMapList[streamIndex].end()) break; // go to next stream
      }
    }
  }
  // fill myCurrentEvent object using existing GRAW frame mapping
  collectEventFragments(matchEventId); 
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceMultiGRAW::checkEntryForFragments(unsigned int iEntry, unsigned int streamIndex){

  //#ifdef DEBUG
  //  std::cout<<__FUNCTION__<<": START ---> stream="<<streamIndex<<", frame_check="<<iEntry<<std::endl<<std::flush;
  //#endif

  if(streamIndex>=myReadEntriesSetList.size()) { // HOTFIX!!!!! // || streamIndex>=myDataFrameList.size()) {
    std::cerr<<KRED<<__FUNCTION__
	       <<": ERROR: wrong GRAW stream id: "<<RST<<streamIndex<<KRED<<" for file entry: "<<RST<<iEntry<<std::endl;
      exit(1);
  }
  if(myReadEntriesSetList[streamIndex].count(iEntry)) return; // nothing to do

  //#ifdef DEBUG
  //  std::cout<<__FUNCTION__<<": before calling loadGrawFrame(iEntry="<<iEntry
  //	   <<", readFullFrame="<<false
  //	   <<", streamIndex="<<streamIndex<<")"
  //	   <<std::endl<<std::flush;
  //#endif

  // check new frame header
  loadGrawFrame(iEntry, false, streamIndex); // HOTFIX!!!!! => fills myDataFrame

  //#ifdef DEBUG
  //  std::cout<<__FUNCTION__<<": after calling loadGrawFrame(iEntry="<<iEntry
  //	   <<", readFullFrame="<<false
  //	   <<", streamIndex="<<streamIndex<<")"
  //	   <<std::endl<<std::flush;
  //#endif    

  unsigned long int currentEventId = myDataFrame.header.eventIdx; // HOTFIX!!!!!
  unsigned int ASAD_idx = myDataFrame.header.asadIdx; // HOTFIX!!!!!
  unsigned int COBO_idx = myDataFrame.header.coboIdx; // HOTFIX!!!!!
 
  myAsadMapList[streamIndex][currentEventId] = ASAD_idx;
  myCoboMapList[streamIndex][currentEventId] = COBO_idx;
  myFramesMapList[streamIndex][currentEventId] = iEntry;
  myReadEntriesSetList[streamIndex].insert(iEntry);
}
/////////////////////////////////////////////////////////
// Fills myCurrentEvent object using existing GRAW frame mapping
/////////////////////////////////////////////////////////
void EventSourceMultiGRAW::collectEventFragments(unsigned int eventId){

  unsigned int nFragments=0;
  std::set<int> asadCounter;
  for(unsigned int streamIndex=0; streamIndex<myFramesMapList.size(); streamIndex++) {
    auto it = myFramesMapList[streamIndex].find(eventId);
    if(it==myFramesMapList[streamIndex].end()) continue;

    if(nFragments==0) {
      myCurrentPEvent->Clear();
	  	
      std::cout<<KYEL<<"Creating a new PEventTPC/Raw with event id: "<<eventId<<RST<<std::endl;
    }
    auto aFragment = it->second;
    loadGrawFrame(aFragment, true, streamIndex);
	    
    int ASAD_idx = myDataFrame.header.asadIdx;
    int COBO_idx = myDataFrame.header.coboIdx;
    unsigned long int eventId_fromFrame = myDataFrame.header.eventIdx; // HOTFIX !!!
    myCurrentEntry=aFragment; 
    asadCounter.insert(ASAD_idx);

    myCurrentEventInfo.SetEventId(eventId);      
    myCurrentEventInfo.SetEventTimestamp(myDataFrame.header.eventTime);
    RunIdParser runParser(myFilePathList.front());
    myCurrentEventInfo.SetRunId(runParser.runId());

    if(eventId!=eventId_fromFrame){
      std::cerr<<KRED<<__FUNCTION__
	       <<": WARNING: event id mismatch!: eventId="<<RST<<eventId
	       <<KRED<<", eventId_fromFrame="<<RST<<eventId_fromFrame<<std::endl;
      return;
    }     
    std::cout<<__FUNCTION__<<KBLU<<": Found a frame for event id: "<<RST<<eventId;
    if(aFragment<nEvents) {
      std::cout<<KBLU<<" in file entry: "<<RST<<aFragment<<RST;
    } else {
      std::cout<<KBLU<<" in next file entry: "<<RST<<aFragment-nEvents<<RST;
    }
    std::cout<<KBLU<<" for ASAD: "<<RST<<ASAD_idx
	     <<KBLU<<", COBO: "<<RST<<COBO_idx
	     <<KBLU<<", GRAW stream id: "<<RST<<streamIndex
	     <<std::endl;
    
    if(fillEventType==EventType::tpc) fillEventFromFrame(myDataFrame);
    else if(fillEventType==EventType::raw) fillEventRawFromFrame(myDataFrame);
    nFragments++; 
  }
  fillEventTPC();
  
  if(nFragments!=GRAW_EVENT_FRAGMENTS) {
    std::cerr<<KRED<<__FUNCTION__
	     <<": WARNING: Fragment counts for event id: "<<RST<<eventId
	     <<KRED<<" mismatch. Expected: "<<RST<<GRAW_EVENT_FRAGMENTS
	     <<KRED<<" found: "<<RST<<nFragments<<std::endl;
  }
  //long int eventNumberInFile = std::distance(myFramesMap.begin(), it);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string EventSourceMultiGRAW::getNextFilePath(unsigned int streamIndex){

  if(streamIndex>=myFilePathList.size()) {
    std::cerr <<KRED<<__FUNCTION__
	      <<": ERROR: wrong GRAW stream id: " <<RST<<streamIndex<<std::endl;
    exit(1);    
  }
  auto myFilePath = myFilePathList[streamIndex];
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
