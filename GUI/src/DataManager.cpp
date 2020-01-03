#include<cstdlib>
#include <iostream>

#include "TCanvas.h"

#include "GeometryTPC.h"
#include "EventTPC.h"

#include "DataManager.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool DataManager::loadGeometry(const std::string & fileName){
  
  myGeometryPtr = std::make_shared<GeometryTPC>(fileName);
  return myGeometryPtr != nullptr && myGeometryPtr->IsOK();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void DataManager::loadDataFile(const std::string & fileName){

  std::string treeName = "TPCData";
  
  myFile = std::make_shared<TFile>(fileName.c_str(),"READ");
  if(!myFile){
    std::cerr<<"File: "<<fileName<<"not found!"<<std::endl;
    return;
  }
  
  myTree = (TTree*)myFile->Get(treeName.c_str());
  myTree->SetBranchAddress("Event", &*currentEvent); //CHECK IF CORRECT
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
  currentEvent->SetGeoPtr(myGeometryPtr);
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

  return currentEvent;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> DataManager::getNextEvent(){

  if(myCurrentEntry<nEvents){
    loadTreeEntry(++myCurrentEntry);
  }

  return currentEvent;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> DataManager::getPreviousEvent(){

  if(myCurrentEntry>0){
    loadTreeEntry(--myCurrentEntry);
  }

  return currentEvent;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<GeometryTPC> DataManager::getGeometry() const{

  return myGeometryPtr;
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
unsigned int DataManager::numberOfEvents() const{

  return nEvents;
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
unsigned int DataManager::currentEventNumber() const{

  if(currentEvent != nullptr){
    return currentEvent->GetEventId();
  }
  
  return 0;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
