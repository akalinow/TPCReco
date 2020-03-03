#include<cstdlib>
#include <iostream>

#include "EventSourceGRAW.h"

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceGRAW::EventSourceGRAW(const std::string & geometryFileName) {

  loadGeometry(geometryFileName); 
  myPedestalCalculator.SetGeometryAndInitialize(myGeometryPtr);
 
  minSignalCell = 51;//FIXME read from config
  maxSignalCell = 500;//FIXME read from config
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceGRAW::~EventSourceGRAW() { }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceGRAW::loadDataFile(const std::string & fileName){

  EventSourceBase::loadDataFile(fileName);

  myFile =  std::make_shared<TGrawFile>(fileName.c_str());
  if(!myFile){
    std::cerr<<"File: "<<fileName<<"not found!"<<std::endl;
    exit(0);
  }
  nEvents = myFile->GetGrawFramesNumber();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceGRAW::loadFileEntry(unsigned long int iEntry){

  if(iEntry>=nEvents) iEntry = nEvents;
  bool dataFrameRead = myFile->GetGrawFrame(myDataFrame, iEntry);
  if(!dataFrameRead){
    std::cerr << "ERROR: cannot read event " << iEntry << std::endl;
    exit(0);
  }

  int eventIdx = myDataFrame.fHeader.fEventIdx;
  if(myCurrentEvent->GetEventId()!=eventIdx){
    myCurrentEvent->Clear();
    myCurrentEvent->SetEventId(eventIdx);
    myCurrentEvent->SetGeoPtr(myGeometryPtr);
     std::cout<<"\033[34m";
     std::cout<<"Creating a new event: "<<eventIdx<<std::endl;
    std::cout<<"\033[39m";
  }
  else{
    std::cout<<"\033[34m";
    std::cout<<"Addng a frame from ASAD: "<< myDataFrame.fHeader.fAsadIdx<< " to existing event: "<<eventIdx<<std::endl;
    std::cout<<"\033[39m";
  }
  myCurrentEvent->SetGeoPtr(myGeometryPtr);
  fillEventFromFrame(myDataFrame);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceGRAW::fillEventFromFrame(GET::GDataFrame & aGrawFrame){

  myPedestalCalculator.CalculateEventPedestals(aGrawFrame);

  int  COBO_idx = myDataFrame.fHeader.fCoboIdx;
  int  ASAD_idx = myDataFrame.fHeader.fAsadIdx;
  if(ASAD_idx >= myGeometryPtr->GetAsadNboards()){
    std::cout<<"\033[31m";
    std::cout<<"Data format mismatch!. ASAD: "<<ASAD_idx
	     <<" number of ASAD boards in geometry: "<<myGeometryPtr->GetAsadNboards()
	     <<" Skipping the frame."
	     <<std::endl;
    std::cout<<"\033[39m";
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
