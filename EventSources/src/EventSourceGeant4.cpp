#include <iostream>
#include <memory>

#include "TPCReco/colorText.h"
#include "TPCReco/RunController.h"
#include "TPCReco/ModuleExchangeSpace.h"
#include "TPCReco/EventSourceGeant4.h"
// Workaround for runControler | Without this module Factory does not see any of MC modules
#include "../../MonteCarlo/Modules/DummyModule/DummyModule.h"

EventSourceGeant4::EventSourceGeant4(const std::string & geometryFileName, std::shared_ptr<fwk::RunController> runController, unsigned long int nEvents):
      EventSourceBase(),
      myRunController(runController),
      nEvents(nEvents)
{
  loadGeometry(geometryFileName);
}


EventSourceGeant4::~EventSourceGeant4(){ }


void EventSourceGeant4::loadDataFile(const std::string & fileName){ }


void EventSourceGeant4::loadFileEntry(unsigned long int iEntry){
  generateNextEvent();
}


std::shared_ptr<EventTPC> EventSourceGeant4::getNextEvent(){
  generateNextEvent();
  return myCurrentEvent;
}


std::shared_ptr<EventTPC> EventSourceGeant4::getPreviousEvent(){
  generateNextEvent();
  return myCurrentEvent;
}


void EventSourceGeant4::loadEventId(unsigned long int iEvent){
  myCurrentEntry = iEvent;
  generateNextEvent();
}


void EventSourceGeant4::loadGeometry(const std::string & fileName){
  EventSourceBase::loadGeometry(fileName);
  myProjectorPtr.reset(new UVWprojector(myGeometryPtr));
}

unsigned long int EventSourceGeant4::numberOfEvents() const {
    return nEvents;
}


void EventSourceGeant4::generateNextEvent(){
    myRunController -> RunSingle();
    myCurrentPEvent = std::make_shared<PEventTPC>(myRunController -> getCurrentPEventTPC());
    fillEventTPC();
}

