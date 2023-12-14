#include <cstdlib>
#include <iostream>

#include <TFile.h>
#include <TTree.h>

#include "TPCReco/colorText.h"
#include "TPCReco/EventSourceROOT.h"
#include "TPCReco/PedestalCalculator.h"

#include "TPCReco/ConfigManager.h"

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceROOT::EventSourceROOT(const std::string & geometryFileName) {

  loadGeometry(geometryFileName);
  myPedestalCalculator.SetGeometryAndInitialize(myGeometryPtr);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceROOT::setTreePointers() {

  treeName = ConfigManager::getConfig().get<std::string>("input.treeName");
  aPtr = myCurrentPEvent.get();
  aPtrEventInfo=NULL;
  aPtrEventData=NULL;
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceROOT::~EventSourceROOT() { }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceROOT::setRemovePedestal(bool aFlag){
  removePedestal = aFlag;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceROOT::configurePedestal(const boost::property_tree::ptree &config){
  auto parser=[this, &config](std::string &&parameter, void (PedestalCalculator::*setter)(int)){
    if(config.find(parameter)!=config.not_found()){
      (this->myPedestalCalculator.*setter)(config.get<int>(parameter));
    }
  };
  parser("pedestal.minPedestalCell", &PedestalCalculator::SetMinPedestalCell);
  parser("pedestal.maxPedestalCell", &PedestalCalculator::SetMaxPedestalCell);
  parser("pedestal.minSignalCell", &PedestalCalculator::SetMinSignalCell);
  parser("pedestal.maxSignalCell", &PedestalCalculator::SetMaxSignalCell);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceROOT::loadDataFile(const std::string & fileName){

  EventSourceBase::loadDataFile(fileName);

  myFile = std::make_shared<TFile>(fileName.c_str(),"READ");
  if(!myFile){
    std::cerr<<KRED<<"Can not open file: "<<RST<<fileName<<KRED<<"!"<<RST<<std::endl;
    exit(1);
  }

  setTreePointers();
  myTree.reset((TTree*)myFile->Get(treeName.c_str()));
  if(!myTree){
    std::cout<<KRED<<"ERROR "<<RST<<"TTree with name: "<<treeName
	     <<" not found in TFile. "<<std::endl;
    std::cout<<"TFile content is: "<<std::endl;
    myFile->ls();
    exit(0);
  }

    myTree->SetBranchAddress("Event", &aPtr);
    myTree->BuildIndex("myEventInfo.runId", "myEventInfo.eventId");

  nEntries = myTree->GetEntries();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceROOT::loadFileEntry(unsigned long int iEntry){

  if(!myTree){
    std::cerr<<"ROOT tree not available!"<<std::endl;
    return;
  }
  if((long int)iEntry>=myTree->GetEntries()) iEntry = myTree->GetEntries() - 1;

  myTree->GetEntry(iEntry);
  fillEventTPC();
			      
  myCurrentEntry = iEntry;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
unsigned long int EventSourceROOT::numberOfEvents() const{ return nEntries; }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> EventSourceROOT::getNextEvent(){

  if(nEntries>0 && myCurrentEntry<nEntries-1){
    loadFileEntry(++myCurrentEntry);
  }
  return myCurrentEvent;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> EventSourceROOT::getPreviousEvent(){

  if(myCurrentEntry>0 && nEntries>0){
    loadFileEntry(--myCurrentEntry);
  }
  return myCurrentEvent;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceROOT::loadEventId(unsigned long int iEvent){

  if(!myTree){
    std::cerr<<"ROOT tree not available!"<<std::endl;
    return;
  }
  // primary method: assumes that TTree::BuildIndex() was performed while storing/reading myTree
  loadFileEntry(myTree->GetEntryNumberWithIndex(getCurrentEvent()->GetEventInfo().GetRunId(), iEvent));

  // secondary (failover) method: when TTree::BuildIndex() did not work properly
  unsigned long int iEntry = 0;
  while(currentEventNumber()!=iEvent && iEntry<nEntries){  
    loadFileEntry(iEntry);
    ++iEntry;
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceROOT::loadGeometry(const std::string & fileName){

  EventSourceBase::loadGeometry(fileName);
  myPedestalCalculator.SetGeometryAndInitialize(myGeometryPtr);
}