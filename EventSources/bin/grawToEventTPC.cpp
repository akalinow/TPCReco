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

  std::string grawFileName;

  grawFileName = aConfig.get("input.dataFile","");

  std::string rootFileName = createROOTFileName(grawFileName);
  TFile aFile(rootFileName.c_str(),"RECREATE");

  std::cout << "File with " << myEventSource->numberOfEntries() << " frames opened." << std::endl;
  
  auto myEventPtr = myEventSource->getCurrentPEvent();

  #ifdef DEBUG
  std::cout << "==== GrawToEvenTPC INITIALIZATION: myPtr_EventTPC="
	    << myEventPtr << " ====" << std::endl;
  #endif

  TTree aTree("TPCData","");
  auto persistent_event = myEventPtr.get();
  Int_t bufsize=128000;
  int splitlevel=2;
  aTree.Branch("Event", &persistent_event, bufsize, splitlevel); 
  Long64_t currentEventId=-1;
  std::map<unsigned int, bool> eventIdMap;


  do {

#ifdef DEBUG
    std::cout << "==== GrawToEventTPC X-CHECK: EventSourceGRAW EventID= "
    	      << myEventSource->currentEventNumber()
    	      << ", EventTPC EventID="
    	      << myEventPtr->GetEventInfo().GetEventId()
     	      << "====" << std::endl;
#endif
    
    unsigned int eventId = myEventPtr->GetEventInfo().GetEventId();    
    if(eventIdMap.find(eventId)==eventIdMap.end()){
      eventIdMap[eventId] = true;

#ifdef DEBUG
      std::cout << "==== GrawToEventTPC LOOP: persistentPtr_EventTPC="
		<< persistent_event << " ====" << std::endl;
      std::cout << "---- EventTPC content start ----" << std::endl;
      std::cout << *persistent_event << std::endl;
      std::cout << "---- EventTPC content end ----" << std::endl;
#endif

      std::cout<< myEventPtr->GetEventInfo()<<std::endl;
      aTree.Fill();
      if(eventIdMap.size()%100==0) aTree.FlushBaskets();
    }

#ifdef DEBUG
    if( eventIdMap.size()==10) break;
#endif

    currentEventId=myEventSource->currentEventNumber();
    myEventSource->getNextEvent();
  }
  while(currentEventId!=(Long64_t)myEventSource->currentEventNumber());  
  aTree.Print();
  // build index based on: majorname=EventId, minorname=NONE
  aTree.BuildIndex("Event.myEventInfo.eventId");
  aTree.Write("", TObject::kOverwrite); // save only the new version of the tree
  //aFile.Close();

  return 0;
      
  
 #ifdef DEBUG
  std::shared_ptr<EventSourceROOT> myEventSourceRoot;
  myEventSourceRoot = std::make_shared<EventSourceROOT>(geometryFileName);
  myEventSourceRoot->loadDataFile(rootFileName);
  std::cout<<"myEventSourceRoot.loadEventId(3)"<<std::endl;
  myEventSourceRoot->loadEventId(3);
  std::cout<<*(myEventSourceRoot->getCurrentEvent())<<std::endl;
  double chargeThreshold = 35;
  int delta_timecells = 25;
  int delta_strips = 5;
  myEventSourceRoot->getCurrentEvent()->MakeOneCluster(chargeThreshold, delta_strips, delta_timecells);
  #endif

  return 0;
}
/////////////////////////////////////
/////////////////////////////////////
