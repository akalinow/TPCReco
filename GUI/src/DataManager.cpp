#include<cstdlib>
#include <iostream>

#include "TCanvas.h"

#include "GeometryTPC.h"
#include "EventTPC.h"

#include "DataManager.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void DataManager::loadDataFile(const std::string & fileName){

  std::string treeName = "TPCData";
  
  myFile.OpenFile(fileName.c_str(),"READ");
  if(myFile.IsOpen()){
    std::cerr<<"File: "<<fileName<<"not found!"<<std::endl;
    return;
  }
  myTree = (TTree*)myFile.Get(treeName.c_str());
  myTree->SetBranchAddress("Event", &currentEvent_internal); //MIGHT BE INCORRECT
  nEvents = myTree->GetEntries();
  loadTreeEntry(0);

  std::cout<<"File: "<<fileName<<" loaded."<<std::endl;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void DataManager::loadTreeEntry(unsigned int iEntry){

  if(myTree == nullptr){
    std::cerr<<"ROOT tree not available!"<<std::endl;
    return;
  }
  if(myTree->GetEntries()<=iEntry) return;
  
  myTree->GetEntry(iEntry);
  currentEvent_external_copy = std::make_shared<EventTPC>(*currentEvent_internal);
  myCurrentEntry = iEntry;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void DataManager::loadEventId(unsigned int iEvent){

  int iEntry = 0;
  while(currentEventNumber()!=iEvent){  
    loadTreeEntry(iEntry);
    ++iEntry;
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> DataManager::getCurrentEvent() const{

  return currentEvent_external_copy;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> DataManager::getNextEvent(){

  if(myCurrentEntry<nEvents){
    loadTreeEntry(++myCurrentEntry);
  }

  return currentEvent_external_copy;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> DataManager::getPreviousEvent(){

  if(myCurrentEntry>0){
    loadTreeEntry(--myCurrentEntry);
  }

  return currentEvent_external_copy;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
unsigned int DataManager::numberOfEvents() const{

  return nEvents;
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
unsigned int DataManager::currentEventNumber() const{

  if(currentEvent_external_copy != nullptr){
    return currentEvent_external_copy->GetEventId();
  }
  
  return 0;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
