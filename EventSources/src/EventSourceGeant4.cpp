#include <iostream>
#include <memory>

#include "TPCReco/colorText.h"
#include "TPCReco/RunController.h"
#include "TPCReco/ModuleExchangeSpace.h"
#include "TPCReco/EventSourceGeant4.h"
#include "../../MonteCarlo/Modules/DummyModule/DummyModule.h"

EventSourceGeant4::EventSourceGeant4(const std::string & geometryFileName, std::shared_ptr<fwk::RunController> runController, unsigned long int nEvents):
      EventSourceBase(),
      myRunController(runController),
      nEvents(nEvents)
{
  loadGeometry(geometryFileName);
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
    // TODO: Implement the logic to return the number of events
    return nEvents;
}

void EventSourceGeant4::fillEvent(ModuleExchangeSpace &event){
  myRunController -> RunSingle();
  ModuleExchangeSpace currentEvent = myRunController -> GetCurrentEvent();
  event = currentEvent;
}
void EventSourceGeant4::fillEventsArray() {
  for (unsigned long int i = 0; i < nEvents; ++i) {
    ModuleExchangeSpace event;
    fillEvent(event);
    events[i] = event;
  }
}

