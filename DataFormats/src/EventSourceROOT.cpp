#include<cstdlib>
#include <iostream>

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

  myFile = std::make_shared<TFile>(fileName.c_str(),"READ");
  if(!myFile){
    std::cerr<<"File: "<<fileName<<"not found!"<<std::endl;
    exit(0);
  }
  
  myTree.reset((TTree*)myFile->Get(treeName.c_str()));  
  myTree->SetBranchAddress("Event", &aPtr);
  nEvents = myTree->GetEntries();
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
