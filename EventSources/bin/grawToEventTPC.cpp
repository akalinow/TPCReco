#include <iostream>
#include <cstdlib>
#include <string>
#include <memory>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>


#include "TPCReco/GeometryTPC.h"
#include "TPCReco/EventTPC.h"
#include "TPCReco/PEventTPC.h"
#include "TPCReco/PedestalCalculator.h"
#include "TPCReco/EventSourceGRAW.h"
#include "TPCReco/EventSourceMultiGRAW.h"
#include "TPCReco/EventSourceFactory.h"
#include "TPCReco/ConfigManager.h"

#include <TFile.h>
#include <TTree.h>

#include "TPCReco/colorText.h"

#include <utl/Logging.h>
#include <get/GDataSample.h>
#include <get/GDataChannel.h>
#include <get/GDataFrame.h>
#include <get/TGrawFile.h>
#include <mfm/FrameDictionary.h>

#ifdef DEBUG
#include "TPCReco/EventSourceROOT.h" 
#endif

/////////////////////////////////////
/////////////////////////////////////
std::string createROOTFileName(const  std::string & grawFileName){

  std::string rootFileName;
  unsigned int index = grawFileName.find(",");
  if(index!=std::string::npos){
    rootFileName = grawFileName.substr(0,index);
  }
  index = rootFileName.rfind("/");
  rootFileName = rootFileName.substr(index,-1);

  
  if(rootFileName.find("CoBo_ALL_AsAd_ALL")!=std::string::npos){
    rootFileName = rootFileName.replace(0,std::string("CoBo_ALL_AsAd_ALL").size()+1,"EventTPC");
  }
  else if(rootFileName.find("CoBo0_AsAd")!=std::string::npos){
    rootFileName = rootFileName.replace(0,std::string("CoBo0_AsAd").size()+2,"EventTPC");
  }
  else{
    std::cout<<KRED<<"File format unknown: "<<RST<<rootFileName<<std::endl;
    exit(1);
  }
  index = rootFileName.rfind("graw");
  rootFileName = rootFileName.replace(index,-1,"root");
  
  return rootFileName;
}
/////////////////////////////////////
/////////////////////////////////////
int convertGRAWFile(boost::property_tree::ptree & aConfig);
/////////////////////////////////////
/////////////////////////////////////
int main(int argc, char *argv[]) {

  ConfigManager cm;
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);

  convertGRAWFile(myConfig);
  return 0;
}
/////////////////////////////////////
/////////////////////////////////////
int convertGRAWFile(boost::property_tree::ptree & aConfig){
  
  std::shared_ptr<EventSourceBase> myEventSource = EventSourceFactory::makeEventSourceObject(aConfig);

  std::string grawFileName = aConfig.get("input.dataFile","");

  std::string rootFileName = createROOTFileName(grawFileName);
  TFile aFile(rootFileName.c_str(),"RECREATE");

  std::cout << "File with " << myEventSource->numberOfEntries() << " frames opened." << std::endl;
  
  auto myEventPtr = myEventSource->getCurrentPEvent();

  TTree aTree("TPCData","");
  auto persistent_event = myEventPtr.get();
  Int_t bufsize=128000;
  int splitlevel=2;
  aTree.Branch("Event", &persistent_event, bufsize, splitlevel); 
  Long64_t currentEventId=-1;
  std::map<unsigned int, bool> eventIdMap;
  myEventSource->loadFileEntry(0);

  do {
    
    unsigned int eventId = myEventPtr->GetEventInfo().GetEventId();    
    if(eventIdMap.find(eventId)==eventIdMap.end()){
      eventIdMap[eventId] = true;

      std::cout<< myEventPtr->GetEventInfo()<<std::endl;
      aTree.Fill();
      if(eventIdMap.size()%100==0) aTree.FlushBaskets();
    }
    currentEventId=myEventSource->currentEventNumber();
    myEventSource->getNextEvent();
  }
  while(currentEventId!=(Long64_t)myEventSource->currentEventNumber());  
  aTree.Print();
  // build index based on: majorname=EventId, minorname=NONE
  aTree.BuildIndex("Event.myEventInfo.eventId");
  aTree.Write("", TObject::kOverwrite); // save only the new version of the tree
  aFile.Close();

  return 0;
      
}
/////////////////////////////////////
/////////////////////////////////////
