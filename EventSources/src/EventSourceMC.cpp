#include <iostream>
#include <memory>

#include "TPCReco/colorText.h"
#include "TPCReco/RunController.h"
#include "TPCReco/ModuleExchangeSpace.h"
#include "TPCReco/EventSourceMC.h"
#include "TPCReco/GeometryTPC.h"
// Workaround for runControler | Without this module Factory does not see any of MC modules
#include "../../MonteCarlo/Modules/DummyModule/DummyModule.h"

EventSourceMC::EventSourceMC(const std::string & geometryFileName, std::shared_ptr<fwk::RunController> runController, unsigned long int nEvents):
      EventSourceBase(),
      myRunController(runController),
      nEvents(nEvents)
{
  loadGeometry(geometryFileName);
}


EventSourceMC::~EventSourceMC(){
  if(myRunController) myRunController -> Finish(); // properly closes output ROOT file created by EventFileExporter MC module
}


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

reaction_type EventSourceMC::GetGeneratedReactionType(){
    return  myCurrentSimEvent -> GetReactionType();
}

const Track3D & EventSourceMC::getGeneratedTrack(){

  myTrack = Track3D();

  //loop over tracks
  for (const auto &t: myCurrentSimEvent -> GetTracks()) {
    
      //do not add segments when track is fully out of active volume
      if(t.IsOutOfActiveVolume()) continue;

      // set basic TrackSegment3D info
      mySegment3D.setGeometry(myGeometryPtr);
      mySegment3D.setStartEnd(t.GetTruncatedStart(), t.GetTruncatedStop());
      mySegment3D.setPID(t.GetPrimaryParticle().GetID());
      myTrack.addSegment(mySegment3D);
  } 
  return myTrack;
}

void EventSourceMC::generateNextEvent(){
    while(myRunController -> RunSingle()!=fwk::VModule::eSuccess) {}
    myCurrentPEvent = std::make_shared<PEventTPC>(myRunController -> getCurrentPEventTPC());
    myCurrentSimEvent = std::make_shared<SimEvent>(myRunController -> getCurrentSimEvent());
    fillEventTPC();
}

