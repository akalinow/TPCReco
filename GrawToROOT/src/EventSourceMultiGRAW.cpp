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

  myDataFrame.Clear();

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
	       <<": ERROR: Can not open file: "<<RST<<fileName<<std::endl;
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

  #ifdef DEBUG
  std::cout<<__FUNCTION__<<KBLU<<": Start loading the file entry: "<<RST<<iEntry
	   <<KBLU<<" from the GRAW stream id: "<<RST<<streamIndex
	   <<KBLU<<" with option readFull="<<RST<<readFullEvent<<std::endl;
  #endif

  if(streamIndex>=myFilePathList.size() || streamIndex>=myNextFilePathList.size()) { // HOTFIX!!! // || streamIndex>=myDataFrameList.size()) {
    std::cerr<<KRED<<__FUNCTION__
	     <<": ERROR: wrong GRAW stream id: " <<RST<<streamIndex<<KRED<<" for file entry: "<<RST<<iEntry
	     <<std::endl;
    exit(1);
  }
  std::string tmpFilePath = myFilePathList[streamIndex];

  //#ifdef DEBUG
  //  std::cout<<__FUNCTION__<<": AFTER tmpFilePath ---> stream="<<streamIndex<<", frame_check="<<iEntry<<", readFull="<<readFullEvent<<std::endl<<std::flush;
  //#endif

  std::cout.setstate(std::ios_base::failbit);
#ifndef EVENTSOURCEGRAW_NEXT_FILE_DISABLE  

  if(iEntry>=nEntries){
    tmpFilePath = myNextFilePathList[streamIndex];
    iEntry -= nEntries;
  }

  //#ifdef DEBUG
  //  std::cout<<__FUNCTION__<<": BEFORE getGrawFrame(path="<<tmpFilePath
  //	   <<", frame="<<iEntry<<"+1"
  //	   <<", dataFrame="<<&myDataFrame // HOTFIX!!!!! // &myDataFrameList[streamIndex]
  //	   <<", readFullFrame="<<readFullEvent<<")"
  //	   <<std::endl<<std::flush;
  //#endif
  
  //  bool dataFrameRead = myFrameLoader.getGrawFrame(tmpFilePath, iEntry+1, myDataFrameList[streamIndex], readFullEvent);///FIXME getGrawFrame counts frames from 1 (WRRR!)
  bool dataFrameRead = myFrameLoader.getGrawFrame(tmpFilePath, iEntry+1, myDataFrame, readFullEvent); // HOTFIX!!!!! => fills myDataFrame 
  ///FIXME getGrawFrame counts frames from 1 (WRRR!)

  //#ifdef DEBUG
  //  std::cout<<__FUNCTION__<<": AFTER getGrawFrame(path="<<tmpFilePath
  //	   <<", frame="<<iEntry<<"+1"
  //	   <<", dataFrame="<<&myDataFrame // HOTFIX!!!!! // &myDataFrameList[streamIndex]
  //	   <<", readFullFrame="<<readFullEvent<<")"
  //	   <<std::endl<<std::flush;
  //#endif
  
#else
  bool dataFrameRead = false;

  if(iEntry<nEntries) {
   
    //#ifdef DEBUG
    //    std::cout<<__FUNCTION__<<": BEFORE calling getGrawFrame(path="<<tmpFilePath
    //	   <<", frame="<<iEntry<<"+1"
    //	   <<", dataFrame="<<&myDataFrame // HOTFIX!!!!! // &myDataFrameList[streamIndex]
    //	   <<", readFullFrame="<<readFullEvent<<")"
    //	   <<std::endl<<std::flush;
    //#endif

    //  dataFrameRead = myFrameLoader.getGrawFrame(tmpFilePath, iEntry+1, myDataFrameList[streamIndex], readFullEvent);///FIXME getGrawFrame counts frames from 1 (WRRR!)
    dataFrameRead = myFrameLoader.getGrawFrame(tmpFilePath, iEntry+1, myDataFrame, readFullEvent); // HOTFIX!!!!! => fills myDataFrame
    ///FIXME getGrawFrame counts frames from 1 (WRRR!)
  
    //#ifdef DEBUG
    //  std::cout<<__FUNCTION__<<": AFTER calling getGrawFrame(path="<<tmpFilePath
    //	   <<", frame="<<iEntry<<"+1"
    //	   <<", datAFrame="<<&myDataFrame // HOTFIX!!!!! // &myDataFrameList[streamIndex]
    //	   <<", readFullFrame="<<readFullEvent<<")"
    //	   <<std::endl;
    //  std::cout<<std::flush;
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
  
  #ifdef DEBUG
  std::cout<<__FUNCTION__<<KBLU<<": Finished loading the file entry: "<<RST<<iEntry
	   <<KBLU<<" from the GRAW stream id: "<<RST<<streamIndex
	   <<KBLU<<" with option readFull="<<RST<<readFullEvent<<std::endl;
  #endif

  return dataFrameRead;
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

  // find corresponding file entry for each GRAW file
  for( unsigned int streamIndex=0; streamIndex<myFramesMapList.size() ; streamIndex++) {

    // check map [eventId, frameIndex] for a given GRAW file
    auto it2 = myFramesMapList[streamIndex].find(eventId);
    if(it2!=myFramesMapList[streamIndex].end()) {
      #ifdef DEBUG
      unsigned int eventIndexFrame = it2->second;
      std::cout <<__FUNCTION__<<KBLU<<": Found file entry: "<<RST<<eventIndexFrame<<KBLU<<" for the GRAW stream id: "<<RST<<streamIndex
		<<KBLU<<", event id: "<<RST<<eventId<<std::endl;
       #endif
    } else {
      // count remaining frames of a given GRAW file
      for(auto iEntry=nEntries-1; iEntry>=0; --iEntry) {  // TODO: optimize for speed!!! (eg. check same frame index first and only then try to scan the whole file)

	//#ifdef DEBUG
	//	std::cout<<__FUNCTION__<<": before calling checkEntryForFragments(iEntry="<<iEntry<<", streamIndex="<<streamIndex<<")"
	//		 <<std::endl<<std::flush;
	//#endif
	checkEntryForFragments(iEntry, streamIndex);

	//#ifdef DEBUG
	//	std::cout<<__FUNCTION__<<": after calling checkEntryForFragments(iEntry="<<iEntry<<", streamIndex="<<streamIndex<<")"
	//		 <<std::endl<<std::flush;
	//#endif    

	if(myFramesMapList[streamIndex].find(eventId)!=myFramesMapList[streamIndex].end()) break;
      }
    }
  }

  // fill myCurrentEvent object using existing GRAW frame mapping
  //#ifdef DEBUG
  //  std::cout<<__FUNCTION__<<": before calling collectEventFragments(eventId="<<eventId<<")"
  //	   <<std::endl<<std::flush;
  //#endif    

  collectEventFragments(eventId);

  //#ifdef DEBUG
  //  std::cout<<__FUNCTION__<<": after calling collectEventFragments(eventId="<<eventId<<")"
  //	   <<std::endl<<std::flush;
  //#endif    
  #ifdef DEBUG
  std::cout<<__FUNCTION__<<KBLU
	   <<": Finished looking for the event id: "<<RST<<eventId
	   <<std::endl;
  #endif    
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

    ///////////////////////////////////////////////////////////////
    //    std::cout<<__FUNCTION__<<": stream="<<streamIndex
    //	     <<RST<<std::endl<<std::flush;
    ///////////////////////////////////////////////////////////////

    //#ifdef DEBUG
    //    std::cout<<__FUNCTION__<<": before calling checkEntryForFragments(iEntry="<<iEntry<<", streamIndex="<<streamIndex<<")"
    //	     <<std::endl<<std::flush;
    //#endif

    checkEntryForFragments(iEntry, streamIndex);

    //#ifdef DEBUG
    //    std::cout<<__FUNCTION__<<": after calling checkEntryForFragments(iEntry="<<iEntry<<", streamIndex="<<streamIndex<<")"
    //	     <<std::endl<<std::flush;
    //#endif

    // check map [eventId, frameIndex] for a given GRAW file
    for(auto const &it: myFramesMapList[streamIndex]) {

      ///////////////////////////////////////////////////////////////
      //      std::cout<<__FUNCTION__<<": stream="<<streamIndex<<", frame="<<it.second<<", frame_check="<<iEntry
      //	       <<RST<<std::endl<<std::flush;
      ///////////////////////////////////////////////////////////////

      if( it.second == iEntry ) {
	auto EventId= it.first;
	auto it2 = myAsadMapList[streamIndex].find(EventId);
	auto it3 = myCoboMapList[streamIndex].find(EventId);

	if(it2==myAsadMapList[streamIndex].end() || it3==myCoboMapList[streamIndex].end() ) continue;
	int ASAD_idx = it2->second;
	int COBO_idx = it3->second;

	///////////////////////////////////////////////////////////////
	//	std::cout<<__FUNCTION__<<": stream="<<streamIndex<<", frame="<<it.second<<", event="<<EventId<<", cobo="<<COBO_idx<<", asad="<<ASAD_idx
	//		 <<RST<<std::endl<<std::flush;
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
	      <<": WARNING: can not find any GRAW streams with {ASAD=0, COBO=0} for the file entry: "<<RST<<iEntry
	      <<std::endl;
    return;
  }
  /*
  std::cout<<__FUNCTION__<<KBLU<<": Found the GRAW stream id: "<<RST<<matchStreamIndex
	   <<KBLU<<" corresponding to {ASAD=0, COBO=0}, file entry: "<<RST<<iEntry
	   <<KBLU<<", event id: "<<RST<<matchEventId<<std::endl;    
  */
  // check frames corresponding to matched event ID
  for(unsigned int streamIndex=0; streamIndex<myReadEntriesSetList.size() ; streamIndex++) {
    if(streamIndex!=matchStreamIndex) {
      auto it = myFramesMapList[streamIndex].find(matchEventId);
      if(it!=myFramesMapList[streamIndex].end()) continue; 

      // strategy 1: check same frame index for matched event ID
      checkEntryForFragments(iEntry, streamIndex);
      if(myFramesMapList[streamIndex].find(matchEventId)!=myFramesMapList[streamIndex].end()) continue; // break;
      
      // strategy 2: scan all remaining frames for matched event ID
      for(unsigned long int iEntry2=0; iEntry2<=nEntries-1; ++iEntry2) {
	checkEntryForFragments(iEntry2, streamIndex);
	if(myFramesMapList[streamIndex].find(matchEventId)!=myFramesMapList[streamIndex].end()) continue; // break;
      }
    }
  }

  //#ifdef DEBUG
  //  std::cout<<__FUNCTION__<<": before calling collectEventFragments(eventId="<<matchEventId<<")"
  //	   <<std::endl<<std::flush;
  //#endif    

  // fill myCurrentEvent object using existing GRAW frame mapping
  collectEventFragments(matchEventId);

  //#ifdef DEBUG
  //  std::cout<<__FUNCTION__<<": after calling collectEventFragments(eventId="<<matchEventId<<")"
  //	   <<std::endl<<std::flush;
  //#endif
  #ifdef DEBUG
  std::cout<<__FUNCTION__<<KBLU
	   <<": Finished looking for the file entry: "<<RST<<iEntry
	   <<std::endl;
  #endif
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
  //  std::cout<<__FUNCTION__<<": END ---> stream="<<streamIndex<<", frame_check="<<iEntry<<std::endl<<std::flush;
  //#endif

}
/////////////////////////////////////////////////////////
// Fills myCurrentEvent object using existing GRAW frame mapping
/////////////////////////////////////////////////////////
void EventSourceMultiGRAW::collectEventFragments(unsigned int eventId){

  unsigned int nFragments=0;
  for(unsigned int streamIndex=0; streamIndex<myFramesMapList.size(); streamIndex++) {
    auto it = myFramesMapList[streamIndex].find(eventId);
    if(it==myFramesMapList[streamIndex].end()) continue;
    
    switch(fillEventType) {
      
    case EventType::raw:
      {	
	if(nFragments==0) {
	  myCurrentEventRaw->SetEventId(eventId);
	  std::cout<<KYEL<<"Creating a new EventRaw with event id: "<<eventId<<RST<<std::endl;
	}
	auto aFragment = it->second;
	loadGrawFrame(aFragment, true, streamIndex); // HOTFIX => fills myDataFrame
	
	myCurrentEventRaw->SetEventTimestamp(myDataFrame.fHeader.fEventTime); // HOTFIX !!!
	//int ASAD_idx = myDataFrame.fHeader.fAsadIdx; // HOTFIX !!!
	//int COBO_idx = myDataFrame.fHeader.fCoboIdx; // HOTFIX !!!
	unsigned long int eventId_fromFrame = myDataFrame.fHeader.fEventIdx; // HOTFIX !!!
	myCurrentEntry=aFragment; // update current event frame index for EventSourceBase::currententryNumber()

	//	std::cout<<KGRN<<__FUNCTION__
	//		 <<": myCurrentEntry="<<aFragment<<", BASE->getCurrentEntry()="<<this->currentEntryNumber()<<RST<<std::endl;

	//	myCurrentEventRaw->timestamp=myDataFrameList[streamIndex].fHeader.fEventTime;
	//	int ASAD_idx = myDataFrameList[streamIndex].fHeader.fAsadIdx;
	//	int COBO_idx = myDataFrameList[streamIndex].fHeader.fCoboIdx;
	//	unsigned long int eventId_fromFrame = myDataFrameList[streamIndex].fHeader.fEventIdx;
	if(eventId!=eventId_fromFrame){
	  std::cerr<<KRED<<__FUNCTION__<<KRED
		   <<": WARNING: event id mismatch! eventId="<<RST<<eventId
		   <<KRED<<", eventId_fromFrame="<<RST<<eventId_fromFrame<<RST<<std::endl;
	  return;
	} 
	fillEventRawFromFrame(myDataFrame); // HOTFIX!!!!!
	//	fillEventRawFromFrame(myDataFrameList[streamIndex]);
	nFragments++; // count good fragments
      }
      break;
      
    case EventType::tpc:  // fill EventTPC class (skip EventRaw)
    default:
      {
	if(nFragments==0) {
	  myCurrentPEvent->Clear();
	  
	  myCurrentEvent->Clear();
	  myCurrentEvent->SetGeoPtr(myGeometryPtr);
	
	  std::cout<<KYEL<<"Creating a new EventTPC with event id: "<<eventId<<RST<<std::endl;
	}
	auto aFragment = it->second;
	loadGrawFrame(aFragment, true, streamIndex); // HOTFIX => fills myDataFrame
	
	myCurrentEventInfo.SetEventId(eventId);      
	myCurrentEventInfo.SetEventTimestamp(myDataFrame.fHeader.fEventTime);
	int ASAD_idx = myDataFrame.fHeader.fAsadIdx; // HOTFIX !!!
	int COBO_idx = myDataFrame.fHeader.fCoboIdx; // HOTFIX !!!
	unsigned long int eventId_fromFrame = myDataFrame.fHeader.fEventIdx; // HOTFIX !!!
	myCurrentEntry=aFragment; // update current event frame index for EventSourceBase::currententryNumber()

	//	std::cout<<KGRN<<__FUNCTION__
	//		 <<": myCurrentEntry="<<aFragment<<", BASE->getCurrentEntry()="<<this->currentEntryNumber()<<RST<<std::endl;

	//	myCurrentEvent->SetEventTime(myDataFrameList[streamIndex].fHeader.fEventTime);
	//	int ASAD_idx = myDataFrameList[streamIndex].fHeader.fAsadIdx;
	//	int COBO_idx = myDataFrameList[streamIndex].fHeader.fCoboIdx;
	//	unsigned long int eventId_fromFrame = myDataFrameList[streamIndex].fHeader.fEventIdx;
	if(eventId!=eventId_fromFrame){
	  std::cerr<<KRED<<__FUNCTION__
		   <<": WARNING: event id mismatch!: eventId="<<RST<<eventId
		   <<KRED<<", eventId_fromFrame="<<RST<<eventId_fromFrame<<std::endl;
	  return;
	}     
	std::cout<<__FUNCTION__<<KBLU<<": Found a frame for event id: "<<RST<<eventId;
	if(aFragment<nEntries) {
	  std::cout<<KBLU<<" in file entry: "<<RST<<aFragment<<RST;
	} else {
	  std::cout<<KBLU<<" in next file entry: "<<RST<<aFragment-nEntries<<RST;
	}
	std::cout<<KBLU<<" for ASAD: "<<RST<<ASAD_idx
		 <<KBLU<<", COBO: "<<RST<<COBO_idx
		 <<KBLU<<", GRAW stream id: "<<RST<<streamIndex
		 <<std::endl;
	//myCurrentEvent->SetEventInfo(myCurrentEventInfo);
	myCurrentPEvent->SetEventInfo(myCurrentEventInfo);
	fillEventFromFrame(myDataFrame); // HOTFIX!!!!!
	nFragments++; // count good fragments
      }
    }; 
  }
  
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
