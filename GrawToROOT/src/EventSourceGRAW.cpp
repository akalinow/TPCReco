#include<cstdlib>
#include <iostream>

#include "get/graw2dataframe.h"
#include "EventSourceGRAW.h"
#include "colorText.h"

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceGRAW::EventSourceGRAW(const std::string & geometryFileName) {

  loadGeometry(geometryFileName);
  GRAW_EVENT_FRAGMENTS = myGeometryPtr->GetAsadNboards();
  myPedestalCalculator.SetGeometryAndInitialize(myGeometryPtr);
 
  minSignalCell = 51;//FIXME read from config
  maxSignalCell = 500;//FIXME read from config
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceGRAW::~EventSourceGRAW() { }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> EventSourceGRAW::getNextEvent(){

  unsigned int currentEventIdx = myCurrentEvent->GetEventId();
  auto it = myFramesMap.find(currentEventIdx);
  unsigned int lastEventFrame = *it->second.rbegin();
  if(lastEventFrame<nEntries-1) ++lastEventFrame;
  loadFileEntry(lastEventFrame);
  return myCurrentEvent;  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> EventSourceGRAW::getPreviousEvent(){

  unsigned int currentEventIdx = myCurrentEvent->GetEventId();
  auto it = myFramesMap.find(currentEventIdx);
  unsigned int firstEventFrame = *it->second.begin();
  if(firstEventFrame>0) --firstEventFrame; 
  loadFileEntry(firstEventFrame);
  return myCurrentEvent;  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceGRAW::loadDataFile(const std::string & fileName){

  EventSourceBase::loadDataFile(fileName);

  myFilePath = fileName;
  myFile =  std::make_shared<TGrawFile>(fileName.c_str());
  if(!myFile){
    std::cerr<<KRED<<"Can not open file: "<<fileName<<"!"<<RST<<std::endl;
    exit(1);
  }
  nEntries = myFile->GetGrawFramesNumber();

  /*
  for(unsigned int iEntry=0;iEntry<nEntries;++iEntry){
    loadGrawFrame(iEntry);
    int currentEventIdx = myDataFrame.fHeader.fEventIdx;
    std::cout<<"iEntry: "<<iEntry
	     <<" currentEventIdx: "<<currentEventIdx
	     <<std::endl;
      
  }*/
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool EventSourceGRAW::loadGrawFrame(unsigned int iEntry){

  if(iEntry>=nEntries) iEntry = nEntries;
  ///getGrawFrame counts frames from 1 (WRRR!)
  std::cout.setstate(std::ios_base::failbit);
  bool dataFrameRead = getGrawFrame(myFilePath, iEntry+1, myDataFrame);
  std::cout.clear();
  
  if(!dataFrameRead){
    std::cerr <<KRED<< "ERROR: cannot read event " << RST<<iEntry << std::endl;
    std::cerr <<KRED
	      <<"Please check if you are running the application from the resources directory."
	      <<RST
	      <<std::endl;
    exit(1);
  }
  return dataFrameRead;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceGRAW::loadEventId(unsigned long int eventIdx){

  std::cout<<KBLU
	   <<"Start looking for the event id: "<<eventIdx
	   <<RST<<std::endl;
  
  auto it = myFramesMap.find(eventIdx);

  if(it!=myFramesMap.end() &&
     it->second.size()<GRAW_EVENT_FRAGMENTS){
    unsigned int iEntry =  *it->second.rbegin();
    findEventFragments(eventIdx, iEntry);
  }
  else if(it==myFramesMap.end()){
    findEventFragments(eventIdx,0);
  }
  collectEventFragments(eventIdx);

  std::cout<<KBLU
	   <<"Finished looking for the event id: "<<eventIdx
	   <<RST<<std::endl;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceGRAW::loadFileEntry(unsigned long int iEntry){

  myCurrentEntry = iEntry;
  loadGrawFrame(iEntry);
  unsigned long int eventIdx = myDataFrame.fHeader.fEventIdx;
  if(myFramesMap.find(eventIdx)==myFramesMap.end() ||
     myFramesMap.find(eventIdx)->second.size()<GRAW_EVENT_FRAGMENTS){
    findEventFragments(eventIdx, iEntry);
  }
  collectEventFragments(eventIdx);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceGRAW::findEventFragments(unsigned long int eventIdx, unsigned int iInitialEntry){

  auto it=myFramesMap.find(eventIdx);
  unsigned int nFragments = 0;
  if(it!=myFramesMap.end()){
    nFragments = it->second.size();
  }
  unsigned int currentEventIdx = 0;

  if(iInitialEntry>5) iInitialEntry-=5;
  else iInitialEntry = 1;
  for(unsigned int iEntry=iInitialEntry;iEntry<nEntries && nFragments<GRAW_EVENT_FRAGMENTS;++iEntry){
    loadGrawFrame(iEntry);
    currentEventIdx = myDataFrame.fHeader.fEventIdx;
    myFramesMap[currentEventIdx].insert(iEntry);
    nFragments =  myFramesMap[eventIdx].size();
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceGRAW::collectEventFragments(unsigned int eventIdx){

  auto it = myFramesMap.find(eventIdx);
  if(it==myFramesMap.end()) return;
  if(it->second.size()!=GRAW_EVENT_FRAGMENTS){
      std::cerr<<__FUNCTION__
	       <<KRED<<" Fragment counts for eventIdx = "<<eventIdx
	       <<" mismatch. Expected: "<<GRAW_EVENT_FRAGMENTS
	       <<" found: "<<it->second.size()
	       <<RST<<std::endl;
  }

  myCurrentEvent->Clear();
  myCurrentEvent->SetEventId(eventIdx);
  myCurrentEvent->SetGeoPtr(myGeometryPtr);

  std::cout<<KYEL<<"Creating a new event with eventIdx: "<<eventIdx<<RST<<std::endl;

  for(auto aFramgent: it->second){
    loadGrawFrame(aFramgent);
    int  ASAD_idx = myDataFrame.fHeader.fAsadIdx;
    unsigned int eventIdx_fromFrame = myDataFrame.fHeader.fEventIdx;
    if(eventIdx!=eventIdx_fromFrame){
      std::cerr<<KRED<<__FUNCTION__
	       <<" Event id mismath!: eventIdx = "<<eventIdx
	       <<" eventIdx_fromFrame: "<<eventIdx_fromFrame
	       <<RST<<std::endl;
      return;
    }
    std::cout<<KBLU<<"Found a frame for eventIdx: "<<eventIdx
	     <<" for  ASAD: "<<ASAD_idx<<RST<<std::endl;
    fillEventFromFrame(myDataFrame);
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceGRAW::fillEventFromFrame(GET::GDataFrame & aGrawFrame){

  myPedestalCalculator.CalculateEventPedestals(aGrawFrame);

  int  COBO_idx = aGrawFrame.fHeader.fCoboIdx;
  int  ASAD_idx = aGrawFrame.fHeader.fAsadIdx;
  if(ASAD_idx >= myGeometryPtr->GetAsadNboards()){
    std::cout<<KRED<<__FUNCTION__
	     <<" Data format mismatch!. ASAD: "<<ASAD_idx
	     <<" number of ASAD boards in geometry: "<<myGeometryPtr->GetAsadNboards()
	     <<" Skipping the frame."
	     <<RST<<std::endl;
    return;
  }
  
  for (Int_t agetId = 0; agetId < myGeometryPtr->GetAgetNchips(); ++agetId){
    // loop over normal channels and update channel mask for clustering
    for (Int_t chanId = 0; chanId < myGeometryPtr->GetAgetNchannels(); ++chanId){
      int iChannelGlobal     = myGeometryPtr->Global_normal2normal(COBO_idx, ASAD_idx, agetId, chanId);// 0-255 (without FPN)
      GET::GDataChannel* channel = aGrawFrame.SearchChannel(agetId, myGeometryPtr->Aget_normal2raw(chanId));
      if (!channel) continue;
	  
      for (Int_t i = 0; i < channel->fNsamples; ++i){
	GET::GDataSample* sample = (GET::GDataSample*) channel->fSamples.At(i);
	// skip cells outside signal time-window
	Int_t icell = sample->fBuckIdx;
	if(icell<2 || icell>509 || icell<minSignalCell || icell>maxSignalCell) continue;
	    
	Double_t rawVal  = sample->fValue;
	Double_t corrVal = rawVal;
//	if(myGeometryPtr->GetAsadNboards()==1){
    // Beware HACK!!!
  //TProfile with pedestals is only 256 (max chans in frame) long, pedestals are calculated for each frame and reset
  //to fit into TProfile the global number of first chan in COBO/ASAD has to be substracted from global chanel
  int minChannelGlobal     = myGeometryPtr->Global_normal2normal(COBO_idx, ASAD_idx, 0, 0);
  corrVal -= myPedestalCalculator.GetPedestalCorrection(iChannelGlobal-minChannelGlobal, agetId, icell);
//	} 
	myCurrentEvent->AddValByAgetChannel(COBO_idx, ASAD_idx, agetId, chanId, icell, corrVal);
      }
    }
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
