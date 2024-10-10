#include <cstdlib>
#include <iostream>
#include <fstream>

#include "TPCReco/colorText.h"
#include "TPCReco/EventSourceBase.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceBase::EventSourceBase() {

  myCurrentEvent =  std::make_shared<EventTPC>();
  myCurrentPEvent =  std::make_shared<PEventTPC>();
  myCurrentEntry = 0;
  nEntries = 0;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceBase::~EventSourceBase() { }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceBase::loadDataFile(const std::string & fileName){

  if(!std::ifstream(fileName)){
    std::cout<<KRED<<"Input data file: "<<RST<<fileName<<KRED<<" not found!"<<RST<<std::endl;
    exit(1);
  }

  currentFilePath = fileName;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceBase::loadGeometry(const std::string & fileName){
  
  myGeometryPtr = std::make_shared<GeometryTPC>(fileName.c_str(), false);
  if(!myGeometryPtr){
    std::cerr<<"Geometry not loaded! Refuse to work anymore."<<std::endl;
    exit(0);
  }
  myCurrentEvent->SetGeoPtr(myGeometryPtr);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<PEventTPC> EventSourceBase::getCurrentPEvent() const{

  return myCurrentPEvent;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> EventSourceBase::getCurrentEvent() const{

  return myCurrentEvent;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> EventSourceBase::getLastEvent(){

  if(nEntries>0){
    loadFileEntry(nEntries-1);
    myCurrentEntry = nEntries-1;
  }

  return getCurrentEvent();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
unsigned long int EventSourceBase::currentEntryNumber() const{

  return myCurrentEntry;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<GeometryTPC> EventSourceBase::getGeometry() const{ return myGeometryPtr; }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
unsigned long int EventSourceBase::numberOfEntries() const{ return nEntries; }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
unsigned long int EventSourceBase::currentEventNumber() const{

  if(getCurrentEvent()){
    return getCurrentEvent()->GetEventInfo().GetEventId();
  }
  return -1;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string EventSourceBase::getCurrentPath() const{

  return currentFilePath;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> EventSourceBase::getNextEventLoop(){
  unsigned int currentEventIdx;
  do{
    currentEventIdx=getCurrentEvent()->GetEventInfo().GetEventId();
    getNextEvent();
  }
  while(!eventFilter.pass(*getCurrentEvent()) &&
	currentEventIdx!=getCurrentEvent()->GetEventInfo().GetEventId());
  return getCurrentEvent();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> EventSourceBase::getPreviousEventLoop(){
  unsigned int currentEventIdx;
  do{
    currentEventIdx=getCurrentEvent()->GetEventInfo().GetEventId();
    getPreviousEvent();
  }
  while(!eventFilter.pass(*getCurrentEvent()) &&
	currentEventIdx!=getCurrentEvent()->GetEventInfo().GetEventId());
  return getCurrentEvent();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceBase::fillEventTPC(){

  myCurrentEvent->Clear();
  myCurrentEvent->SetGeoPtr(myGeometryPtr);
  myCurrentEvent->SetChargeMap(myCurrentPEvent->GetChargeMap());
  myCurrentEvent->SetEventInfo(myCurrentPEvent->GetEventInfo());
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
