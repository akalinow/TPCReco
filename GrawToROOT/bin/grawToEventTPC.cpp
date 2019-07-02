#include <iostream>
#include <cstdlib>
#include <string>
#include <memory>

#include "GeometryTPC.h"
#include "EventTPC.h"
#include "PedestalCalculator.h"

#include "TFile.h"
#include "TTree.h"

#include "utl/Logging.h"
#include "get/GDataSample.h"
#include "get/GDataChannel.h"
#include "get/GDataFrame.h"
#include "get/TGrawFile.h"
#include "mfm/FrameDictionary.h"

int main(int argc, char *argv[]) {

  if(argc<3) return -1;

  int minSignalCell = 51;
  int maxSignalCell = 500;
  bool skipEmptyEvents = true;
  
  ///Load TPC geometry
  std::string geomFileName = "geometry_mini_eTPC.dat";
  std::string dataFileName =  "/data/edaq/CoBo_2018-05-11T14:23:14.736_0000.graw";
  long runId = 0;
  if(argc>3){
    geomFileName = std::string(argv[1]);
    std::cout<<"geomFileName: "<<geomFileName<<std::endl;

    dataFileName = std::string(argv[2]);
    std::cout<<"dataFileName: "<<dataFileName<<std::endl;

    runId = std::stoi(argv[3]);
    std::cout<<"runNumber: "<<runId<<std::endl;
  }
  std::shared_ptr<GeometryTPC> myGeometryPtr = std::make_shared<GeometryTPC>(geomFileName.c_str());

  ///Create event
  EventTPC myEvent;

  PedestalCalculator myPedestalCalculator;
  myPedestalCalculator.SetGeometryAndInitialize(myGeometryPtr);

  ///Create ROOT Tree
  std::string rootFileName = "EventTPC_"+std::to_string(runId)+".root";
  
  TFile aFile(rootFileName.c_str(),"RECREATE");
  TTree aTree("TPCData","");
  
  EventTPC *persistent_event = &myEvent;
  aTree.Branch("Event", &persistent_event);

  ///Load data
  GET::GDataFrame dataFrame;
  TGrawFile f(dataFileName.c_str());
  long lastevent=f.GetGrawFramesNumber();

  myEvent.SetRunId(runId);
      
  for(long eventId = 0;eventId<lastevent;++eventId){
    myEvent.Clear();
    myEvent.SetGeoPtr(myGeometryPtr);
    myEvent.SetEventId(eventId);
    bool eventRead = f.GetGrawFrame(dataFrame, eventId);
    if(!eventRead){
      std::cerr << "ERROR: cannot read event " << eventId << std::endl;
      return -1;
    }

    myPedestalCalculator.CalculateEventPedestals(dataFrame);
    
    int COBO_idx = 0;
    int ASAD_idx = 0;
    
    for (Int_t agetId = 0; agetId < myGeometryPtr->GetAgetNchips(); ++agetId){
	// loop over normal channels and update channel mask for clustering
	for (Int_t chanId = 0; chanId < myGeometryPtr->GetAgetNchannels(); ++chanId){
	  int iChannelGlobal     = myGeometryPtr->Global_normal2normal(COBO_idx, ASAD_idx, agetId, chanId);// 0-255 (without FPN)
	    
	    GET::GDataChannel* channel = dataFrame.SearchChannel(agetId, myGeometryPtr->Aget_normal2raw(chanId));
	    if (!channel) continue;
	    
	    for (Int_t i = 0; i < channel->fNsamples; ++i){
		GET::GDataSample* sample = (GET::GDataSample*) channel->fSamples.At(i);
		// skip cells outside signal time-window
		Int_t icell = sample->fBuckIdx;
		if(icell<2 || icell>509 || icell<minSignalCell || icell>maxSignalCell) continue;
		
		Double_t rawVal  = sample->fValue;		
		Double_t corrVal = rawVal - myPedestalCalculator.GetPedestalCorrection(iChannelGlobal, agetId, icell);
		myEvent.AddValByAgetChannel(COBO_idx, ASAD_idx, agetId, chanId, icell, corrVal);
		
	      } // end of loop over time buckets	    
	  } // end of AGET channels loop	
      } // end of AGET chips loop
    myEvent.SetGeoPtr(0);

    ///Skip empty events
    if(skipEmptyEvents && myEvent->GetMaxCharge()<100) continue;
    /////////////////////
    aTree.Fill();
  }

  aTree.Write();
  aFile.Close();
  return 0;
}
