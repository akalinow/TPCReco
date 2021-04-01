#include<cstdlib>
#include <iostream>

#include "colorText.h"
#include "EventSourceROOT.h"
#include "TFile.h"
#include "TTree.h"

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceROOT::EventSourceROOT() {

  treeName = "TPCData";
  aPtr = myCurrentEvent.get();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceROOT::~EventSourceROOT() { }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceROOT::loadDataFile(const std::string & fileName){

  EventSourceBase::loadDataFile(fileName);

  myFile = std::make_shared<TFile>(fileName.c_str(),"READ");
  if(!myFile){
    std::cerr<<KRED<<"Can not open file: "<<RST<<fileName<<KRED<<"!"<<RST<<std::endl;
    exit(1);
  }
  
  myTree.reset((TTree*)myFile->Get(treeName.c_str()));  
  myTree->SetBranchAddress("Event", &aPtr);
  nEntries = myTree->GetEntries();
  std::cout<<"File: "<<fileName<<" loaded."<<std::endl;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceROOT::loadFileEntry(unsigned long int iEntry){

  if(!myTree){
    std::cerr<<"ROOT tree not available!"<<std::endl;
    return;
  }
  if((long int)iEntry>=myTree->GetEntries()) iEntry = myTree->GetEntries() - 1;
  
  myCurrentEvent->SetGeoPtr(0);
  myTree->GetEntry(iEntry);
  myCurrentEvent->SetGeoPtr(myGeometryPtr);
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

  unsigned long int iEntry = 0;
  while(currentEventNumber()!=iEvent && iEntry<nEntries){  
    loadFileEntry(iEntry);
    ++iEntry;
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
