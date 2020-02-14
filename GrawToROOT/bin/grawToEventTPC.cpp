#include <iostream>
#include <cstdlib>
#include <string>
#include <memory>

#include "GeometryTPC.h"
#include "EventTPC.h"
#include "PedestalCalculator.h"
#include "EventSourceGRAW.h"

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

  bool skipEmptyEvents = false;
  
  ///Load TPC geometry
  std::string geomFileName = "geometry_mini_eTPC.dat";
  std::string dataFileName =  "/data/edaq/CoBo_2018-05-11T14:23:14.736_0000.graw";
  std::string timestamp = "";
  if(argc>3){
    geomFileName = std::string(argv[1]);
    std::cout<<"geomFileName: "<<geomFileName<<std::endl;

    dataFileName = std::string(argv[2]);
    std::cout<<"dataFileName: "<<dataFileName<<std::endl;

    timestamp = std::string(argv[3]);
    std::cout<<"timestamp: "<<timestamp<<std::endl;
  }
  EventSourceGRAW myEventSource(geomFileName);
  myEventSource.loadDataFile(dataFileName);
  std::shared_ptr<EventTPC> myEventPtr = myEventSource.getCurrentEvent();

  ///Create ROOT Tree
  std::string rootFileName = "EventTPC_"+timestamp+".root";  
  TFile aFile(rootFileName.c_str(),"RECREATE");
  TTree aTree("TPCData","");
  
  EventTPC *persistent_event = myEventPtr.get();
  aTree.Branch("Event", &persistent_event);

  long numberOfEvents = myEventSource.numberOfEvents();
  for(long iFileEntry = 0; iFileEntry<numberOfEvents; ++iFileEntry){
    myEventSource.loadFileEntry(iFileEntry);
    myEventPtr->SetGeoPtr(0);
    ///Skip empty events
    if(skipEmptyEvents && myEventPtr->GetMaxCharge()<100) continue;
    /////////////////////
    aTree.Fill();
  }
  aTree.Write();
  aFile.Close();
  return 0;
}
