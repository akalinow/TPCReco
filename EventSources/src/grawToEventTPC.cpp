#include "TPCReco/grawToEventTPC.h"

#include <iostream>
#include <cstdlib>
#include <memory>

#include <TFile.h>
#include <TTree.h>

#include "TPCReco/EventTPC.h"
#include "TPCReco/PEventTPC.h"
#include "TPCReco/EventSourceGRAW.h"
#include "TPCReco/EventSourceMultiGRAW.h"
#include "TPCReco/EventSourceFactory.h"
#include "TPCReco/InputFileHelper.h"
#include "TPCReco/colorText.h"
/////////////////////////////////////
/////////////////////////////////////
int convertGRAWFile(boost::property_tree::ptree & aConfig){
  
  std::shared_ptr<EventSourceBase> myEventSource = EventSourceFactory::makeEventSourceObject(aConfig);

  std::string grawFileName = aConfig.get("input.dataFile","");
  std::string rootFileName = InputFileHelper::makeOutputFileName(grawFileName,"EventTPC");
  TFile aFile(rootFileName.c_str(),"RECREATE");

  int nEntries = myEventSource->numberOfEntries();
  int readNEvents = aConfig.get<int>("input.readNEvents",-1);
  if(readNEvents < 0 || readNEvents > nEntries) readNEvents = nEntries;

  std::cout <<KGRN<< "File with " <<RST<< nEntries <<KGRN<< " frames opened." <<RST<< std::endl;
  std::cout <<KGRN<< "Reading " <<RST<< readNEvents <<KGRN<< " frames." <<RST<< std::endl;
  std::cout <<KGRN<< "Output file: " <<RST<< rootFileName << std::endl;
  
  auto myEventPtr = myEventSource->getCurrentPEvent();

  TTree aTree(aConfig.get<std::string>("input.treeName").c_str(),"");
  auto persistent_event = myEventPtr.get();
  Int_t bufsize=128000;
  int splitlevel=2;
  aTree.Branch("Event", &persistent_event, bufsize, splitlevel); 
  std::map<unsigned int, bool> eventIdMap;

  for(int iEntry=0; iEntry<readNEvents; iEntry++) 
  {
    myEventSource->loadFileEntry(iEntry);

    unsigned int eventId = myEventPtr->GetEventInfo().GetEventId();    
    if(eventIdMap.find(eventId)==eventIdMap.end()){
      eventIdMap[eventId] = true;

      std::cout<< myEventPtr->GetEventInfo()<<std::endl;
      aTree.Fill();
      if(eventIdMap.size()%100==0) aTree.FlushBaskets();
    }
  }
  
  aTree.Print();
  // build index based on: majorname=EventId, minorname=NONE
  //aTree.BuildIndex("Event.myEventInfo.eventId");
  aTree.Write("", TObject::kOverwrite); // save only the new version of the tree
  aFile.Close();

  return 0;
      
}
/////////////////////////////////////
/////////////////////////////////////
