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
int convertGRAWFile(boost::property_tree::ptree & aConfig){
  
  std::shared_ptr<EventSourceBase> myEventSource = EventSourceFactory::makeEventSourceObject(aConfig);

  std::string grawFileName = aConfig.get("input.dataFile","");

  std::string rootFileName = createROOTFileName(grawFileName);
  TFile aFile(rootFileName.c_str(),"RECREATE");

  int nEntries = myEventSource->numberOfEntries();
  int readNEvents = aConfig.get<int>("input.readNEvents",-1);
  if(readNEvents < 0 || readNEvents > nEntries)
    readNEvents = nEntries;

  std::cout << "File with " << nEntries << " frames opened." << std::endl;
  std::cout << "Reading " << readNEvents << " frames." << std::endl;
  
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
