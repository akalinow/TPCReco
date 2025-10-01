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
void EventSourceBase::setTrackBuilder(std::shared_ptr<TrackBuilder> aTkBuilderPtr){

  myTkBuilderPtr = aTkBuilderPtr;
  myTkBuilderPtr->setGeometry(myGeometryPtr);
  myTkBuilderPtr->setEvent(myCurrentEvent);
} 
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TrackBuilder> EventSourceBase::getTrackBuilder() const {
  return myTkBuilderPtr;
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

  if(nEvents>0){
    loadFileEntry(nEvents-1);
    myCurrentEntry = nEvents-1;
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
unsigned long int EventSourceBase::numberOfEvents() const{ return nEvents;}
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

  if(nEventsToRead--<=0 || nEventsToRead>(long int)nEvents) return std::shared_ptr<EventTPC>(0);

  do{
    currentEventIdx=getCurrentEvent()->GetEventInfo().GetEventId();
    getNextEvent();
  }
  while(!eventFilter.pass(*this) && 
        currentEventIdx!=getCurrentEvent()->GetEventInfo().GetEventId());

  std::cout<<KBLU<<"EventSourceBase: processed: "<<RST<<100 - int(100.0*nEventsToRead/numberOfEvents())<<KBLU<<" % events"<<RST<<std::endl;

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
  while(!eventFilter.pass(*this) && currentEventIdx!=getCurrentEvent()->GetEventInfo().GetEventId());
  return getCurrentEvent();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceBase::fillEventTPC(){

  myCurrentEvent->Clear();
  myCurrentEvent->SetGeoPtr(myGeometryPtr);
  myCurrentEvent->SetChargeMap(myCurrentPEvent->GetChargeMap());
  myCurrentEvent->SetEventInfo(myCurrentPEvent->GetEventInfo());
  myTkBuilderPtr->reconstruct();
}
////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const Track3D & EventSourceBase::getRecoEvent() const {

  return myTkBuilderPtr->getTrack3D(0);

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////