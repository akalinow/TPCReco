#include <iostream>
#include <memory>

#include "TPCReco/colorText.h"
#include "TPCReco/RunController.h"
#include "TPCReco/ModuleExchangeSpace.h"
#include "TPCReco/EventSourceMC.h"
// Workaround for runControler | Without this module Factory does not see any of MC modules
#include "../../MonteCarlo/Modules/DummyModule/DummyModule.h"

EventSourceMC::EventSourceMC(const std::string & geometryFileName, std::shared_ptr<fwk::RunController> runController, unsigned long int nEvents):
      EventSourceBase(),
      myRunController(runController),
      nEvents(nEvents)
{
  loadGeometry(geometryFileName);
}


EventSourceMC::~EventSourceMC(){ }


void EventSourceMC::loadDataFile(const std::string & fileName){ }


void EventSourceMC::loadFileEntry(unsigned long int iEntry){
  generateNextEvent();
  myCurrentEntry = iEntry;
}


std::shared_ptr<EventTPC> EventSourceMC::getNextEvent(){
  generateNextEvent();
  return myCurrentEvent;
}


std::shared_ptr<EventTPC> EventSourceMC::getPreviousEvent(){
  generateNextEvent();
  return myCurrentEvent;
}


void EventSourceMC::loadEventId(unsigned long int iEvent){
  myCurrentEntry = iEvent;
  generateNextEvent();
}


void EventSourceMC::loadGeometry(const std::string & fileName){
  EventSourceBase::loadGeometry(fileName);
  // myProjectorPtr.reset(new UVWprojector(myGeometryPtr));
}

unsigned long int EventSourceMC::numberOfEvents() const {
    return nEvents;
}

reaction_type EventSourceMC::GetGeneratedReactiontType(){
    return  myCurrentSimEvent -> GetReactionType();
}

Track3D EventSourceMC::getGeneratedTrack(){
    return myRunController -> getCurrentTrack3D();
}



void EventSourceMC::generateNextEvent(){
    myRunController -> RunSingle();
    myCurrentPEvent = std::make_shared<PEventTPC>(myRunController -> getCurrentPEventTPC());
    myCurrentSimEvent = std::make_shared<SimEvent>(myRunController -> getCurrentSimEvent());
    fillEventTPC();
}

