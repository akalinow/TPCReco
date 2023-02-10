#include <CLHEP/Random/Random.h>
#include "GeantSim.h"
#include "G4RunManager.hh"
#include "GELIDetectorConstruction.hh"
#include "GELIPhysicsList.hh"
#include "GELIPrimaryGeneratorAction.hh"
#include "GELISteppingAction.hh"
#include "GELITrackingAction.hh"
#include "GELIEventAction.hh"

#include <ctime>

fwk::VModule::EResultFlag GeantSim::Init(boost::property_tree::ptree config) {
    auto cc = CentralConfig::GetInstance();
    cc->SetTopNode(config);
    CLHEP::HepRandom::setTheEngine(&fCLHEPRandomEngine);
    //TODO: set better seed!
    CLHEP::HepRandom::setTheSeed(time(nullptr));

    fRunManager = new G4RunManager;
    // set mandatory initialization classes
    fRunManager->SetUserInitialization(new GELIDetectorConstruction);
    fRunManager->SetUserInitialization(new GELIPhysicsList);

    fRunManager->SetUserAction(new GELIPrimaryGeneratorAction(buffer));
    fRunManager->SetUserAction(new GELISteppingAction(buffer));
    fRunManager->SetUserAction(new GELITrackingAction());
    fRunManager->SetUserAction(new GELIEventAction(buffer));

    fRunManager->Initialize();

    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag GeantSim::Process(ModuleExchangeSpace &event) {
    buffer.simEv = &event.simEvt;
    fRunManager->BeamOn(1);
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag GeantSim::Finish() {
    delete fRunManager;
    return fwk::VModule::eSuccess;
}
