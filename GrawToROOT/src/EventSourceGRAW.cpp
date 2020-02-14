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

  myCurrentEvent->Clear();
  if(iEntry>=nEvents) iEntry = nEvents;
  bool dataFrameRead = myFile->GetGrawFrame(myDataFrame, iEntry);
  if(!dataFrameRead){
    std::cerr << "ERROR: cannot read event " << iEntry << std::endl;
    exit(0);
  }
  myCurrentEvent->SetGeoPtr(myGeometryPtr);
  myCurrentEvent->SetEventId(iEntry);
  fillEventFromFrame(myDataFrame);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceGRAW::fillEventFromFrame(GET::GDataFrame & aGrawFrame){

  myPedestalCalculator.CalculateEventPedestals(aGrawFrame);
    
  for(int COBO_idx = 0; COBO_idx < myGeometryPtr->GetCoboNboards(); ++COBO_idx){
    for(int ASAD_idx = 0; ASAD_idx < myGeometryPtr->GetAsadNboards(COBO_idx); ++ASAD_idx){   
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
	    Double_t corrVal = rawVal - myPedestalCalculator.GetPedestalCorrection(iChannelGlobal, agetId, icell);
	    myCurrentEvent->AddValByAgetChannel(COBO_idx, ASAD_idx, agetId, chanId, icell, corrVal);
	  }
	} 
      } 
    }
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
