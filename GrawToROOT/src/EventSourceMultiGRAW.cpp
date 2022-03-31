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


#include "TCollection.h"
#include "TClonesArray.h"

#include "EventSourceMultiGRAW.h"
//#include "EventRaw.h"
#include "colorText.h"

#include "get/graw2dataframe.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceMultiGRAW::EventSourceMultiGRAW(const std::string & geometryFileName) : EventSourceGRAW(geometryFileName) {
  /*
  loadGeometry(geometryFileName);
  GRAW_EVENT_FRAGMENTS = myGeometryPtr->GetAsadNboards();
  myPedestalCalculator.SetGeometryAndInitialize(myGeometryPtr);

  std::string formatsFilePath = "./CoboFormats.xcfg";
  myFrameLoader.initialize(formatsFilePath);
  // minSignalCell = 2;//FIXME read from config
  // maxSignalCell = 500;//FIXME read from config
  */
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceMultiGRAW::~EventSourceMultiGRAW(){}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/*
void EventSourceGRAW::setRemovePedestal(bool aFlag){
  removePedestal = aFlag;
}
*/
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> EventSourceMultiGRAW::getNextEvent(){

  unsigned long int currentEventId = myCurrentEvent->GetEventId();
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
      if(lastEventFrame<nEntries-1) ++lastEventFrame;
      loadFileEntry(lastEventFrame);
      break;
    }
  }
  return myCurrentEvent;  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> EventSourceMultiGRAW::getPreviousEvent(){

  unsigned int currentEventId = myCurrentEvent->GetEventId();
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
    if(fileName.size()>0 && fileNameList.size()<GRAW_EVENT_FRAGMENTS) fileNameList.insert(fileName);
  };
  if(fileNameList.size()) {
    loadDataFileList(fileNameList);
  }
}  
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceMultiGRAW::loadDataFileList(const std::set<std::string> & fileNameList){

  //  myDataFrameList.clear(); // HOTFIX!!!!!
  myFileList.clear();
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
    //    myDataFrameList.emplace_back(); // MF magic trick - doesn't work 100% 
    //    myDataFrameList.emplace_back(myDataFrame); // HOTFIX!!!!!
    
    //#ifdef DEBUG
    std::cout<<__FUNCTION__<<": calling EventSourceBase::loadDataFile with fileName: "<<fileName<<std::endl<<std::flush;
    //#endif
    EventSourceBase::loadDataFile(fileName);

    auto myFile = std::make_shared<TGrawFile>(fileName.c_str());
    if(!myFile){
      std::cerr<<KRED<<__FUNCTION__
	       <<": ERROR: Can not open file: "<<fileName<<"!"<<RST<<std::endl;
      exit(1);
    }
    myFileList.push_back(myFile);
    
    unsigned int nFrames = myFile->GetGrawFramesNumber();
    if(streamIndex==0 || nFrames<nEntries) nEntries=nFrames; // take the lowest number of frames

    /*
    const int firstEventSize=10;
    if(fileName!=myFilePath || nEntries<firstEventSize){
      findStartingIndex(firstEventSize);
    }
    */
		     
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
  std::cout<<__FUNCTION__<<": Number of entries: "<<nEntries
	   <<std::endl;
#endif
  /*
  for(unsigned int iEntry=0;iEntry<nEntries;++iEntry){
    loadGrawFrame(iEntry, false);
    int currentEventId = myDataFrame.fHeader.fEventIdx;
    int ASAD_idx = myDataFrame.fHeader.fAsadIdx;
    std::cout<<"iEntry: "<<iEntry
	     <<" currentEventId: "<<currentEventId
	     <<" ASAD: "<<ASAD_idx
	     <<std::endl;
  }
  std::cout<<KBLU<<"End of file"<<RST<<std::endl;  
  */ 
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool EventSourceMultiGRAW::loadGrawFrame(unsigned int iEntry, bool readFullEvent, unsigned int streamIndex){

  //#ifdef DEBUG
  std::cout<<__FUNCTION__<<": START ---> stream="<<streamIndex<<", frame_check="<<iEntry<<", readFull="<<readFullEvent<<std::endl<<std::flush;
  //#endif

  if(streamIndex>=myFilePathList.size() || streamIndex>=myNextFilePathList.size()) { // HOTFIX!!! // || streamIndex>=myDataFrameList.size()) {
    std::cerr<<KRED<<__FUNCTION__
	     <<": ERROR: wrong GRAW stream id: " <<RST<<streamIndex<<KRED<<" for file entry: "<<RST<<iEntry
	     <<std::endl;
    exit(1);
  }
  std::string tmpFilePath = myFilePathList[streamIndex];

  //#ifdef DEBUG
  std::cout<<__FUNCTION__<<": AFTER tmpFilePath ---> stream="<<streamIndex<<", frame_check="<<iEntry<<", readFull="<<readFullEvent<<std::endl<<std::flush;
  //#endif

  std::cout.setstate(std::ios_base::failbit);
#ifndef EVENTSOURCEGRAW_NEXT_FILE_DISABLE  

  if(iEntry>=nEntries){
    tmpFilePath = myNextFilePathList[streamIndex];
    iEntry -= nEntries;
  }

  //#ifdef DEBUG
  std::cout<<__FUNCTION__<<": BEFORE getGrawFrame(path="<<tmpFilePath
	   <<", frame="<<iEntry<<"+1"
	   <<", dataFrame="<<&myDataFrame // HOTFIX!!!!! // &myDataFrameList[streamIndex]
	   <<", readFullFrame="<<readFullEvent<<")"
	   <<std::endl<<std::flush;
  //#endif
  
  //  bool dataFrameRead = myFrameLoader.getGrawFrame(tmpFilePath, iEntry+1, myDataFrameList[streamIndex], readFullEvent);///FIXME getGrawFrame counts frames from 1 (WRRR!)
  bool dataFrameRead = myFrameLoader.getGrawFrame(tmpFilePath, iEntry+1, myDataFrame, readFullEvent); // HOTFIX!!!!! => fills myDataFrame
  ///FIXME getGrawFrame counts frames from 1 (WRRR!)

  //#ifdef DEBUG
  std::cout<<__FUNCTION__<<": AFTER getGrawFrame(path="<<tmpFilePath
	   <<", frame="<<iEntry<<"+1"
	   <<", dataFrame="<<&myDataFrame // HOTFIX!!!!! // &myDataFrameList[streamIndex]
	   <<", readFullFrame="<<readFullEvent<<")"
	   <<std::endl<<std::flush;
  //#endif
  
#else
  bool dataFrameRead = false;

  if(iEntry<nEntries) {
   
    //#ifdef DEBUG
    std::cout<<__FUNCTION__<<": BEFORE calling getGrawFrame(path="<<tmpFilePath
	   <<", frame="<<iEntry<<"+1"
	   <<", dataFrame="<<&myDataFrame // HOTFIX!!!!! // &myDataFrameList[streamIndex]
	   <<", readFullFrame="<<readFullEvent<<")"
	   <<std::endl<<std::flush;
    //#endif

    //  dataFrameRead = myFrameLoader.getGrawFrame(tmpFilePath, iEntry+1, myDataFrameList[streamIndex], readFullEvent);///FIXME getGrawFrame counts frames from 1 (WRRR!)
    dataFrameRead = myFrameLoader.getGrawFrame(tmpFilePath, iEntry+1, myDataFrame, readFullEvent); // HOTFIX!!!!! => fills myDataFrame
    ///FIXME getGrawFrame counts frames from 1 (WRRR!)
  
  //#ifdef DEBUG
  std::cout<<__FUNCTION__<<": AFTER calling getGrawFrame(path="<<tmpFilePath
	   <<", frame="<<iEntry<<"+1"
	   <<", datAFrame="<<&myDataFrame // HOTFIX!!!!! // &myDataFrameList[streamIndex]
	   <<", readFullFrame="<<readFullEvent<<")"
	   <<std::endl;
  std::cout<<std::flush;
  //#endif

  }
#endif
  std::cout.clear();

  if(!dataFrameRead){
    std::cerr <<KRED<<__FUNCTION__
	      <<": ERROR: cannot read file entry: " <<RST<<iEntry<<std::endl
	      <<KRED<<"from file: "<<std::endl
	      <<RST<<tmpFilePath
	      << std::endl;
    std::cerr <<KRED
	      <<"Please check if you are running the application from the resources directory."
	      <<std::endl<<"or if the data file is missing."
	      <<RST
	      <<std::endl;
    exit(1);
  }
  
  //#ifdef DEBUG
  std::cout<<__FUNCTION__<<": END ---> stream="<<streamIndex<<", frame_check="<<iEntry<<", readFull="<<readFullEvent
	   <<", dataFrameRead="<<dataFrameRead<<std::endl<<std::flush;
  //#endif

  return dataFrameRead;
}
/////////////////////////////////////////////////////////
// Checks all GRAW files for frames with eventId.
// On success uptades myCurrentEvent object.
/////////////////////////////////////////////////////////
void EventSourceMultiGRAW::loadEventId(unsigned long int eventId){

  std::cout<<KBLU
	   <<"Start looking for the event id: "<<eventId
	   <<RST<<std::endl;

  // find corresponding file entry for each GRAW file
  for( unsigned int streamIndex=0; streamIndex<myFramesMapList.size() ; streamIndex++) {

    // check map [eventId, frameIndex] for a given GRAW file
    auto it2 = myFramesMapList[streamIndex].find(eventId);
    if(it2!=myFramesMapList[streamIndex].end()) {
      unsigned int eventIndexFrame = it2->second;
      std::cout <<KBLU<<std::endl<<"Found file entry: "<<RST<<eventIndexFrame<<KBLU<<" for GRAW stream id: "<<RST<<streamIndex
		<<KBLU<<", eventId: "<<RST<<eventId
		<< std::endl;
    } else {
      // count remaining frames of a given GRAW file
      for(auto iEntry=nEntries-1; iEntry>0; iEntry--) {

	//#ifdef DEBUG
	std::cout<<__FUNCTION__<<": before calling checkEntryForFragments(iEntry="<<iEntry<<", streamIndex="<<streamIndex<<")"
		 <<std::endl<<std::flush;
	//#endif
	checkEntryForFragments(iEntry, streamIndex);

	//#ifdef DEBUG
	std::cout<<__FUNCTION__<<": after calling checkEntryForFragments(iEntry="<<iEntry<<", streamIndex="<<streamIndex<<")"
		 <<std::endl<<std::flush;
	//#endif    

	if(myFramesMapList[streamIndex].find(eventId)!=myFramesMapList[streamIndex].end()) break;
      }
    }
  }

  // fill myCurrentEvent object using existing GRAW frame mapping
  //#ifdef DEBUG
  std::cout<<__FUNCTION__<<": before calling collectEventFragments(eventId="<<eventId<<")"
	   <<std::endl<<std::flush;
  //#endif    

  collectEventFragments(eventId);

  //#ifdef DEBUG
  std::cout<<__FUNCTION__<<": after calling collectEventFragments(eventId="<<eventId<<")"
	   <<std::endl<<std::flush;
  //#endif    

  std::cout<<KBLU
	   <<"Finished looking for the event id: "<<eventId
	   <<RST<<std::endl;
}
/////////////////////////////////////////////////////////
// Finds eventId corresponding to a given frame index of the GRAW file for {COBO=0, ASAD=0}.
// Checks remaining GRAW files for frames with this eventId.
// On success uptades myCurrentEvent object.
/////////////////////////////////////////////////////////
void EventSourceMultiGRAW::loadFileEntry(unsigned long int iEntry){

  std::cout<<KBLU
	   <<"Start looking for the file entry: "<<iEntry
	   <<RST<<std::endl;

  bool result = false;
  unsigned long int matchEventId = 0;
  unsigned int matchStreamIndex = 0;

  ///////////////////////////////////////////////////////////////
  std::cout<<__FUNCTION__<<": myReadEntriesSetList.size="<<myReadEntriesSetList.size()
	   <<RST<<std::endl<<std::flush;
  ///////////////////////////////////////////////////////////////
  
  // find eventId for {ASAD=0, COBO=0}
  for( unsigned int streamIndex=0; streamIndex<myReadEntriesSetList.size() ; streamIndex++) {

    ///////////////////////////////////////////////////////////////
    std::cout<<__FUNCTION__<<": stream="<<streamIndex
	     <<RST<<std::endl<<std::flush;
    ///////////////////////////////////////////////////////////////

      //#ifdef DEBUG
    std::cout<<__FUNCTION__<<": before calling checkEntryForFragments(iEntry="<<iEntry<<", streamIndex="<<streamIndex<<")"
	     <<std::endl<<std::flush;
    //#endif

    checkEntryForFragments(iEntry, streamIndex);

    //#ifdef DEBUG
    std::cout<<__FUNCTION__<<": after calling checkEntryForFragments(iEntry="<<iEntry<<", streamIndex="<<streamIndex<<")"
	     <<std::endl<<std::flush;
    //#endif

    // check map [eventId, frameIndex] for a given GRAW file
    for(auto const &it: myFramesMapList[streamIndex]) {

      ///////////////////////////////////////////////////////////////
      std::cout<<__FUNCTION__<<": stream="<<streamIndex<<", frame="<<it.second<<", frame_check="<<iEntry
	       <<RST<<std::endl<<std::flush;
      ///////////////////////////////////////////////////////////////

      if( it.second == iEntry ) {
	auto EventId= it.first;
	auto it2 = myAsadMapList[streamIndex].find(EventId);
	auto it3 = myCoboMapList[streamIndex].find(EventId);

	if(it2==myAsadMapList[streamIndex].end() || it3==myCoboMapList[streamIndex].end() ) continue;
	int ASAD_idx = it2->second;
	int COBO_idx = it3->second;

	///////////////////////////////////////////////////////////////
	std::cout<<__FUNCTION__<<": stream="<<streamIndex<<", frame="<<it.second<<", event="<<EventId<<", cobo="<<COBO_idx<<", asad="<<ASAD_idx
		 <<RST<<std::endl<<std::flush;
	///////////////////////////////////////////////////////////////

	
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
	      <<": WARNING: can not find GRAW stream with {ASAD=0, COBO=0} for file entry: " <<RST<<iEntry
	      << std::endl;
    return;
  }
  std::cout <<KBLU<<std::endl<<"Found GRAW stream id: "<<RST<<matchStreamIndex
	    <<KBLU<<" corresponding to {ASAD=0, COBO=0}, file entry: " <<RST<<iEntry
	    <<KBLU<<", eventId: "<<RST<<matchEventId
	    << std::endl;    

  // check frames corresponding to matched event ID
  for( unsigned int streamIndex=0; streamIndex<myReadEntriesSetList.size() ; streamIndex++) {
    if(streamIndex!=matchStreamIndex) {
      auto it = myFramesMapList[streamIndex].find(matchEventId);
      if(it!=myFramesMapList[streamIndex].end()) continue; 
      
      // count remaining frames corresponding to matched event ID
      for(auto iEntry=nEntries-1; iEntry>0; iEntry--) {
	checkEntryForFragments(iEntry, streamIndex);
	if(myFramesMapList[streamIndex].find(matchEventId)!=myFramesMapList[streamIndex].end()) break;
      }
    }
  }

  //#ifdef DEBUG
  std::cout<<__FUNCTION__<<": before calling collectEventFragments(eventId="<<matchEventId<<")"
	   <<std::endl<<std::flush;
  //#endif    

  // fill myCurrentEvent object using existing GRAW frame mapping
  collectEventFragments(matchEventId);

  //#ifdef DEBUG
  std::cout<<__FUNCTION__<<": after calling collectEventFragments(eventId="<<matchEventId<<")"
	   <<std::endl<<std::flush;
  //#endif

  std::cout<<KBLU
	   <<"Finished looking for the file entry: "<<iEntry
	   <<RST<<std::endl;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceMultiGRAW::checkEntryForFragments(unsigned int iEntry, unsigned int streamIndex){

  //#ifdef DEBUG
  std::cout<<__FUNCTION__<<": START ---> stream="<<streamIndex<<", frame_check="<<iEntry<<std::endl<<std::flush;
  //#endif

  if(streamIndex>=myReadEntriesSetList.size()) { // HOTFIX!!!!! // || streamIndex>=myDataFrameList.size()) {
    std::cerr<<KRED<<__FUNCTION__
	       <<": ERROR: wrong GRAW stream id: "<<RST<<streamIndex<<KRED<<" for file entry: "<<RST<<iEntry
	       <<std::endl;
      exit(1);
  }
  if(myReadEntriesSetList[streamIndex].count(iEntry)) return; // nothing to do

  //#ifdef DEBUG
  std::cout<<__FUNCTION__<<": before calling loadGrawFrame(iEntry="<<iEntry
	   <<", readFullFrame="<<false
	   <<", streamIndex="<<streamIndex<<")"
	   <<std::endl<<std::flush;
  //#endif

  // check new frame header
  loadGrawFrame(iEntry, false, streamIndex); // HOTFIX!!!!! => fills myDataFrame

  //#ifdef DEBUG
  std::cout<<__FUNCTION__<<": after calling loadGrawFrame(iEntry="<<iEntry
	   <<", readFullFrame="<<false
	   <<", streamIndex="<<streamIndex<<")"
	   <<std::endl<<std::flush;
  //#endif    

  unsigned long int currentEventId = myDataFrame.fHeader.fEventIdx; // HOTFIX!!!!!
  unsigned int ASAD_idx = myDataFrame.fHeader.fAsadIdx; // HOTFIX!!!!!
  unsigned int COBO_idx = myDataFrame.fHeader.fCoboIdx; // HOTFIX!!!!!
  //  unsigned long int currentEventId = myDataFrameList[streamIndex].fHeader.fEventIdx;
  //  unsigned int ASAD_idx = myDataFrameList[streamIndex].fHeader.fAsadIdx;
  //  unsigned int COBO_idx = myDataFrameList[streamIndex].fHeader.fCoboIdx;

  myAsadMapList[streamIndex][currentEventId] = ASAD_idx;
  myCoboMapList[streamIndex][currentEventId] = COBO_idx;
  myFramesMapList[streamIndex][currentEventId] = iEntry;
  myReadEntriesSetList[streamIndex].insert(iEntry);
  
  //#ifdef DEBUG
  std::cout<<__FUNCTION__<<": END ---> stream="<<streamIndex<<", frame_check="<<iEntry<<std::endl<<std::flush;
  //#endif

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/*
void EventSourceMultiGRAW::findEventFragments(unsigned long int eventId, unsigned int iInitialEntry){

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
  unsigned int highEndScanRange = std::min((unsigned int)nEntries, iInitialEntry+frameLoadRange);

  for(unsigned int iEntry=iInitialEntry;
      iEntry>=lowEndScanRange && iEntry<nEntries && nFragments<GRAW_EVENT_FRAGMENTS;
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
    //reachEndOfFile = (iEntry==nEntries);
    std::cout<<"\r going forward and reading file entry: "<<iEntry
	     <<" fragments found so far: "
	     <<nFragments
	     <<" expected: "<<GRAW_EVENT_FRAGMENTS << "     ";
  }
  for(unsigned int iEntry=0;iEntry<frameLoadRange && nFragments<GRAW_EVENT_FRAGMENTS;++iEntry){
    checkEntryForFragments(iEntry+nEntries);
    nFragments =  myFramesMap[eventId].size();
    std::cout<<"\r going to next run file and reading file entry: "<<iEntry
	     <<" fragments found so far: "
	     <<nFragments
	     <<" expected: "<<GRAW_EVENT_FRAGMENTS << "     ";
  }
  
  std::cout<<std::endl;
  if(myFramesMap.size()>=nEntries) isFullFileScanned = true;
}
*/
/////////////////////////////////////////////////////////
// Fills myCurrentEvent object using existing GRAW frame mapping
/////////////////////////////////////////////////////////
void EventSourceMultiGRAW::collectEventFragments(unsigned int eventId){

  unsigned int nFragments=0;
  for(unsigned int streamIndex=0; streamIndex<myFramesMapList.size(); streamIndex++) {
    auto it = myFramesMapList[streamIndex].find(eventId);
    if(it==myFramesMapList[streamIndex].end()) continue;
    
    switch(fillEventType) {
      
    case raw:
      {	
	if(nFragments==0) {
	  myCurrentEventRaw->SetEventId(eventId);
	  myCurrentEvent->SetEventId(eventId); // for compatibility with EventSourceBase::currentEventNumber()
	  std::cout<<KYEL<<"Creating a new EventRaw with eventId: "<<eventId<<RST<<std::endl;
	}
	auto aFragment = it->second;
	loadGrawFrame(aFragment, true, streamIndex); // HOTFIX => fills myDataFrame
	
	myCurrentEventRaw->SetEventTimestamp(myDataFrame.fHeader.fEventTime); // HOTFIX !!!
	int ASAD_idx = myDataFrame.fHeader.fAsadIdx; // HOTFIX !!!
	int COBO_idx = myDataFrame.fHeader.fCoboIdx; // HOTFIX !!!
	unsigned long int eventId_fromFrame = myDataFrame.fHeader.fEventIdx; // HOTFIX !!!
	//	myCurrentEventRaw->timestamp=myDataFrameList[streamIndex].fHeader.fEventTime;
	//	int ASAD_idx = myDataFrameList[streamIndex].fHeader.fAsadIdx;
	//	int COBO_idx = myDataFrameList[streamIndex].fHeader.fCoboIdx;
	//	unsigned long int eventId_fromFrame = myDataFrameList[streamIndex].fHeader.fEventIdx;
	if(eventId!=eventId_fromFrame){
	  std::cerr<<KRED<<__FUNCTION__
		   <<": WARNING: event id mismatch! eventId="<<eventId
		   <<", eventId_fromFrame="<<eventId_fromFrame
		   <<RST<<std::endl;
	  return;
	}
	std::cout<<KBLU<<"Found a frame for eventId: "<<RST<<eventId;
	if(aFragment<nEntries) {
	  std::cout<<KBLU<<" in file entry: "<<RST<<aFragment<<RST;
	} else {
	  std::cout<<KBLU<<" in next file entry: "<<RST<<aFragment-nEntries<<RST;
	}
	std::cout<<KBLU<<" for ASAD: "<<RST<<ASAD_idx
		 <<KBLU<<", COBO: "<<RST<<COBO_idx
		 <<KBLU<<", GRAW stream id: "<<RST<<streamIndex
		 <<std::endl;      
	fillEventRawFromFrame(myDataFrame); // HOTFIX!!!!!
	//	fillEventRawFromFrame(myDataFrameList[streamIndex]);
	nFragments++; // count good fragments
      }
      break;
      
    case tpc:  // fill EventTPC class (skip EventRaw)
    default:
      {
	if(nFragments==0) {
	  myCurrentEvent->Clear();
	  myCurrentEvent->SetEventId(eventId);
	  myCurrentEvent->SetGeoPtr(myGeometryPtr);
	
	  std::cout<<KYEL<<"Creating a new EventTPC with eventId: "<<eventId<<RST<<std::endl;
	}
	auto aFragment = it->second;
	loadGrawFrame(aFragment, true, streamIndex); // HOTFIX => fills myDataFrame
	
	myCurrentEvent->SetEventTime(myDataFrame.fHeader.fEventTime); // HOTFIX !!!
	int ASAD_idx = myDataFrame.fHeader.fAsadIdx; // HOTFIX !!!
	int COBO_idx = myDataFrame.fHeader.fCoboIdx; // HOTFIX !!!
	unsigned long int eventId_fromFrame = myDataFrame.fHeader.fEventIdx; // HOTFIX !!!
	//	myCurrentEvent->SetEventTime(myDataFrameList[streamIndex].fHeader.fEventTime);
	//	int ASAD_idx = myDataFrameList[streamIndex].fHeader.fAsadIdx;
	//	int COBO_idx = myDataFrameList[streamIndex].fHeader.fCoboIdx;
	//	unsigned long int eventId_fromFrame = myDataFrameList[streamIndex].fHeader.fEventIdx;
	if(eventId!=eventId_fromFrame){
	  std::cerr<<KRED<<__FUNCTION__
		   <<": WARNING: event id mismatch!: eventId="<<eventId
		   <<", eventId_fromFrame="<<eventId_fromFrame
		   <<RST<<std::endl;
	  return;
	}     
	std::cout<<KBLU<<"Found a frame for eventId: "<<RST<<eventId;
	if(aFragment<nEntries) {
	  std::cout<<KBLU<<" in file entry: "<<RST<<aFragment<<RST;
	} else {
	  std::cout<<KBLU<<" in next file entry: "<<RST<<aFragment-nEntries<<RST;
	}
	std::cout<<KBLU<<" for ASAD: "<<RST<<ASAD_idx
		 <<KBLU<<", COBO: "<<RST<<COBO_idx
		 <<KBLU<<", GRAW stream id: "<<RST<<streamIndex
		 <<std::endl;      
	fillEventFromFrame(myDataFrame); // HOTFIX!!!!!
	//	fillEventFromFrame(myDataFrameList[streamIndex]); 
	nFragments++; // count good fragments
      }
    }; 
  }
  
  if(nFragments!=GRAW_EVENT_FRAGMENTS) {
    std::cerr<<KRED<<__FUNCTION__
	     <<": WARNING: Fragment counts for eventId: "<<RST<<eventId
	     <<KRED<<" mismatch. Expected: "<<RST<<GRAW_EVENT_FRAGMENTS
	     <<KRED<<" found: "<<RST<<nFragments
	     <<RST<<std::endl;
  }
  //long int eventNumberInFile = std::distance(myFramesMap.begin(), it);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/*
void EventSourceGRAW::fillEventFromFrame(GET::GDataFrame & aGrawFrame){

  if(removePedestal) myPedestalCalculator.CalculateEventPedestals(aGrawFrame);

  int  COBO_idx = aGrawFrame.fHeader.fCoboIdx;
  int  ASAD_idx = aGrawFrame.fHeader.fAsadIdx;

  if(ASAD_idx >= myGeometryPtr->GetAsadNboards()){
    std::cout<<KRED<<__FUNCTION__
	     <<": Data format mismatch! ASAD="<<ASAD_idx
	     <<", number of ASAD boards in geometry="<<myGeometryPtr->GetAsadNboards()
	     <<". Frame skipped."
	     <<RST<<std::endl;
    return;
  }
  
  for (Int_t agetId = 0; agetId < myGeometryPtr->GetAgetNchips(); ++agetId){
    // loop over normal channels and update channel mask for clustering
    for (Int_t chanId = 0; chanId < myGeometryPtr->GetAgetNchannels(); ++chanId){
      //      int iChannelGlobal     = myGeometryPtr->Global_normal2normal(COBO_idx, ASAD_idx, agetId, chanId);// 0-255 (without FPN)
      GET::GDataChannel* channel = aGrawFrame.SearchChannel(agetId, myGeometryPtr->Aget_normal2raw(chanId));
      if (!channel) continue;
	  
      for (Int_t i = 0; i < channel->fNsamples; ++i){
	GET::GDataSample* sample = (GET::GDataSample*) channel->fSamples.At(i);
	// skip cells outside signal time-window
	Int_t icell = sample->fBuckIdx;
	if(icell<2 || icell>509 || icell<myPedestalCalculator.GetMinSignalCell() || icell>myPedestalCalculator.GetMaxSignalCell()) continue;
	    
	Double_t rawVal  = sample->fValue;
	Double_t corrVal = rawVal;
//	if(myGeometryPtr->GetAsadNboards()==1){
    // Beware HACK!!!
  //TProfile with pedestals is only 256 (max chans in frame) long, pedestals are calculated for each frame and reset
  //to fit into TProfile the global number of first chan in COBO/ASAD has to be substracted from global chanel
  if(removePedestal){
    //int minChannelGlobal = myGeometryPtr->Global_normal2normal(COBO_idx, ASAD_idx, 0, 0);
    //    corrVal -= myPedestalCalculator.GetPedestalCorrection(iChannelGlobal-minChannelGlobal, agetId, icell);
    corrVal -= myPedestalCalculator.GetPedestalCorrection(COBO_idx, ASAD_idx, agetId, chanId, icell);
  }
//	} 
	myCurrentEvent->AddValByAgetChannel(COBO_idx, ASAD_idx, agetId, chanId, icell, corrVal);
      }
    }
  }
  ////// DEBUG
  //  std::shared_ptr<TProfile> tp=myPedestalCalculator.GetPedestalProfilePerAsad(COBO_idx, ASAD_idx);
  //  std::cout << __FUNCTION__ << ": TProfile[Cobo=" << COBO_idx
  //  	    << ", Asad=" << ASAD_idx
  //  	    << "]=" << tp->GetName()
  //  	    << std::endl << std::flush;
  ////// DEBUG
}
*/
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/*void EventSourceGRAW::fillEventRawFromFrame(GET::GDataFrame & aGrawFrame){

  // fills raw data container class per: COBO, ASAD, AGET, CHANNEL_RAW, TIME_CELL 
  // optimized for data size
  // no pedestal subtracion (can be done later using raw data stored in the container class

  myCurrentEventRaw->eventId = (uint64_t)aGrawFrame.fHeader.fEventIdx;
  myCurrentEventRaw->timestamp = (uint32_t)aGrawFrame.fHeader.fEventTime;
  
  uint8_t COBO_idx = (uint8_t)aGrawFrame.fHeader.fCoboIdx;
  uint8_t ASAD_idx = (uint8_t)aGrawFrame.fHeader.fAsadIdx;
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
    if( (a_it->first).key1==COBO_idx && (a_it->first).key2==ASAD_idx) (a_it->second).channelData.resize(0);
  }
  
  // temporary map of AGET channels
  std::map< MultiKey2, eventraw::ChannelRaw, multikey2_less> map2; // index={agetIdx[0-3], channelIdx[0-67]}, val=ChannelRaw
  std::map< MultiKey2, eventraw::ChannelRaw, multikey2_less>::iterator map2_it;

  TClonesArray* channels = aGrawFrame.GetChannels();
  GET::GDataChannel* channel = 0;
  TIter iter(channels->begin());
  while ((channel = (GET::GDataChannel*) iter.Next())) {
    
    if (!channel) continue;	  
    uint8_t AGET_idx = (uint8_t)channel->fAgetIdx;
    uint8_t CHAN_idx = (uint8_t)channel->fChanIdx;

    // temporary map of samples per channel
    static std::map< uint16_t, uint16_t> map1; // index=cell[0-511], val=value[0-4096] 
    static std::map< uint16_t, uint16_t>::iterator map1_it;
    map1.clear();

    for (int i = 0; i < channel->fNsamples; ++i){
      GET::GDataSample* sample = (GET::GDataSample*) channel->fSamples.At(i);
      uint16_t icell = (uint16_t) sample->fBuckIdx;
      uint16_t rawVal = (uint16_t) sample->fValue;
      map1[icell] = rawVal;      
    }

    // filling ChannelRaw from temporary CHANNEL map
    // NOTE: map1 is sorted by KEY=cell[0-511]
    eventraw::ChannelRaw c;
    for(map1_it=map1.begin(); map1_it!=map1.end(); map1_it++) {
      c.cellMask[ map1_it->first/8 ] |= (1 << (map1_it->first % 8)); // update bit mask
      c.cellData.push_back(map1_it->second); // correct order should be preserved for map1 sorted by KEY

      //#ifdef DEBUG
      //// DEBUG
      //      std::cout << __FUNCTION__
      //      		<< ": Adding sample point: coboIdx=" << (int)COBO_idx << ", asadIdx=" << (int)ASAD_idx << ", agetIdx=" << (int)AGET_idx << ", chanIdx=" << (int)CHAN_idx << ", cellIdx=" << map1_it->first << std::endl;
      //// DEBUG
      //#endif
    }

    // adding ChannelRaw to temporary AGET map
    MultiKey2 mkey(AGET_idx, CHAN_idx);
    map2.insert( std::pair< MultiKey2, eventraw::ChannelRaw >( mkey, c ) );
    //[mkey]=c;
  };
  
  // filling AgetRaw from temporary AGET map
  // NOTE: map2 is sorted by KEY={aget[0-3], chan[0-67]}
  for(map2_it=map2.begin(); map2_it!=map2.end(); map2_it++) {

    uint8_t AGET_idx = (uint8_t)map2_it->first.key1;
    uint8_t CHAN_idx = (uint8_t)map2_it->first.key2;
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
*/
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/*
void EventSourceGRAW::findStartingIndex(unsigned long int size){
  if(nEntries==0){
    startingEventIndex=0;
  } else {
    auto preloadSize=std::min(nEntries,size);
    startingEventIndex=std::numeric_limits<UInt_t>::max();
    for(unsigned long int i=0; i<preloadSize; ++i){
      for(auto fileIndex=0; fileIndex<GRAW_EVENT_FRAGMENTS; fileIndex++) {
	auto myFile=*(std::next(myFileList.begin(), fileIndex));
	auto myDataFrame=*(std::next(myDataFrameList.begin(), fileIndex));
	myFile->GetGrawFrame(myDataFrame, i);
	startingEventIndex=std::min(startingEventIndex, static_cast<unsigned long int>(myDataFrame.fHeader.fEventIdx));
      }
    }
  }
}
*/
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
/*
void EventSourceGRAW::configurePedestal(const boost::property_tree::ptree &config){
  auto parser=[this, &config](std::string &&parameter, void (PedestalCalculator::*setter)(int)){
    if(config.find(parameter)!=config.not_found()){
      (this->myPedestalCalculator.*setter)(config.get<int>(parameter));
    }
  };
  parser("minPedestalCell", &PedestalCalculator::SetMinPedestalCell);
  parser("maxPedestalCell", &PedestalCalculator::SetMaxPedestalCell);
  parser("minSignalCell", &PedestalCalculator::SetMinSignalCell);
  parser("maxSignalCell", &PedestalCalculator::SetMaxSignalCell);
}
*/
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string EventSourceMultiGRAW::getNextFilePath(unsigned int streamIndex){

  if(streamIndex>=myFilePathList.size()) {
    std::cerr <<KRED<<__FUNCTION__
	      <<": ERROR: wrong GRAW stream index: " <<RST<<streamIndex<<std::endl
	      <<std::endl;
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
