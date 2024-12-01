#include <iostream>
#include <memory>

#include "TPCReco/colorText.h"
#include "TPCReco/RunController.h"
#include "TPCReco/ModuleExchangeSpace.h"
#include "TPCReco/EventSourceGeant4.h"
// Workaround for runControler | Without module Factory does not see any module
#include "../../MonteCarlo/Modules/DummyModule/DummyModule.h"

EventSourceGeant4::EventSourceGeant4(const std::string & geometryFileName, std::shared_ptr<fwk::RunController> runController, unsigned long int nEvents):
      EventSourceBase(),
      myRunController(runController),
      nEvents(nEvents)
{
  //loadGeometry(geometryFileName);
  events.reserve(nEvents);
  fillEventsArray();
}


EventSourceGeant4::~EventSourceGeant4(){ }


void EventSourceGeant4::loadDataFile(const std::string & fileName){ }


void EventSourceGeant4::loadFileEntry(unsigned long int iEntry){

  myCurrentPEvent = events[iEntry].tpcPEvt;
  currentEntry = iEntry;
  fillEventTPC();
}


std::shared_ptr<EventTPC> EventSourceGeant4::getNextEvent(){
  currentEntry = (currentEntry + 1) % nEvents;
  loadFileEntry(currentEntry);
  return myCurrentEvent;
}


std::shared_ptr<EventTPC> EventSourceGeant4::getPreviousEvent(){
  currentEntry = (currentEntry - 1 + nEvents) % nEvents;
  loadFileEntry(currentEntry);
  return myCurrentEvent;
}


void EventSourceGeant4::loadEventId(unsigned long int iEvent){
  currentEntry = iEvent;
  loadFileEntry(iEvent);
}


void EventSourceGeant4::loadGeometry(const std::string & fileName){
  EventSourceBase::loadGeometry(fileName);
}

unsigned long int EventSourceGeant4::numberOfEvents() const {
    return nEvents;
}

//void EventSourceGeant4::fillEvent(ModuleExchangeSpace &event){
//  myRunController -> GetCurrentEvent();
//}
void EventSourceGeant4::fillEventsArray() {
  for (unsigned long int i = 0; i < nEvents; ++i) {
    myRunController -> RunSingle();
    ModuleExchangeSpace *currentEvent = myRunController -> GetCurrentEvent();
    
    auto simEvt = currentEvent -> simEvt;
    auto eventInfo = currentEvent -> eventInfo;
    auto track3D = currentEvent -> track3D;
    //auto tpcPEvt = currentEvent -> tpcPEvt;
    std::cout << "EventSourceGeant4::fillEventsArray()" << std::endl;
    std::exit(0);
  }
}

