#include <iostream>
#include <cstdlib>

#include "UtilsTPC.h"
#include "GeometryTPC.h"
#include "UVWprojector.h"
#include "EventTPC.h"

#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"

#include "utl/Logging.h"
#include "get/GDataSample.h"
#include "get/GDataChannel.h"
#include "get/GDataFrame.h"
#include "get/TGrawFile.h"
#include "mfm/FrameDictionary.h"

int main(int argc, char *argv[]) {
  
  ///Load TPC geometry
  std::string fileName = "geometry_mini_eTPC.dat";
  GeometryTPC myGeometry(fileName.c_str());

  ///Create event
  EventTPC myEvent(0, &myGeometry);


  ///Create ROOT Tree
  TFile aFile("EventTPC.root","RECREATE");
  TTree aTree("TPCData","");
  
  EventTPC *persistent_event = &myEvent;
  aTree.Branch("Event", &persistent_event);

  ///Load data
  fileName = "/data/edaq/CoBo_2018-05-11T14:23:14.736_0000.graw";
  GET::GDataFrame dataFrame;
  TGrawFile f(fileName.c_str());
  long lastevent=f.GetGrawFramesNumber();

  lastevent = 5; 
  for(long eventId = 0;eventId<lastevent;++eventId){
    myEvent.SetGeoPtr(&myGeometry);
    bool eventRead = f.GetGrawFrame(dataFrame, eventId);
    if(!eventRead){
      std::cerr << "ERROR: cannot read event " << eventId << std::endl;
      return -1;
    }

 // global channel indices
    //int ichannel_raw  = -1;  // 0-271 (with FPN channels)
    //int ichannel      = -1;  // 0-255 (w/o FPN channels)
    int COBO_idx = 0;
    int ASAD_idx = 0;
    
    // 2nd loop over AGET chips
    for (Int_t agetId = 0; agetId < myGeometry.GetAgetNchips(); ++agetId){
	// loop over normal channels and update channel mask for clustering
	for (Int_t chanId = 0; chanId < myGeometry.GetAgetNchannels(); ++chanId){
	  //ichannel_raw = myGeometry.Global_normal2raw(COBO_idx, ASAD_idx, agetId, chanId);   // 0-271 (with FPN)
	  //ichannel     = myGeometry.Global_normal2normal(COBO_idx, ASAD_idx, agetId, chanId);// 0-255 (without FPN)
	    
	    GET::GDataChannel* channel = dataFrame.SearchChannel(agetId, myGeometry.Aget_normal2raw(chanId));
	    if (!channel) continue;
	    
	    //Double_t FPN_pedestal = 0.0;//TEST pedestals.at(ichannel);
	    
	    for (Int_t i = 0; i < channel->fNsamples; ++i){
		GET::GDataSample* sample = (GET::GDataSample*) channel->fSamples.At(i);
		// skip cells outside signal time-window
		Int_t icell = sample->fBuckIdx;
		//TEST if(icell<2 || icell>509 || icell<minSignalCell || icell>maxSignalCell) continue;
		
		Double_t rawVal  = sample->fValue;		
		Double_t corrVal = rawVal;//TEST - FPN_ave[agetId][icell] - FPN_pedestal;
		// store corrected data in EventTPC object for later analysis
		myEvent.AddValByAgetChannel(COBO_idx, ASAD_idx, agetId, chanId, icell, corrVal);
		
	      } // end of loop over time buckets	    
	  } // end of AGET channels loop	
      } // end of AGET chips loop

    myEvent.SetGeoPtr(0);
    aTree.Fill();
  }

  aTree.Write();
  aFile.Close();
  
  return 0;
}
