#include <CLHEP/Random/Random.h>
#include "GeantSim.h"
#include "G4RunManager.hh"
#include "GELIDetectorConstruction.hh"
#include "GELIPhysicsList.hh"
#include "GELIPrimaryGeneratorAction.hh"
#include "GELISteppingAction.hh"
#include "GELITrackingAction.hh"
#include "GELIEventAction.hh"
#include "G4PhysListFactory.hh"
#include "TRandom.h"

fwk::VModule::EResultFlag GeantSim::Init(boost::property_tree::ptree config) {
    auto cc = CentralConfig::GetInstance();
    cc->SetTopNode(config);
    CLHEP::HepRandom::setTheEngine(&fCLHEPRandomEngine);

    fRunManager = new G4RunManager;
    // set mandatory initialization classes
    fRunManager->SetUserInitialization(new GELIDetectorConstruction);
    fRunManager->SetUserInitialization(new GELIPhysicsList);

    fRunManager->SetUserAction(new GELIPrimaryGeneratorAction(buffer));
    fRunManager->SetUserAction(new GELISteppingAction(buffer));
    fRunManager->SetUserAction(new GELITrackingAction());
    fRunManager->SetUserAction(new GELIEventAction(buffer));

    fRunManager->Initialize();

    auto physListFactory = new G4PhysListFactory();

    for(auto & l: physListFactory->AvailablePhysLists())
        std::cout<<l<<std::endl;

    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag GeantSim::Process(ModuleExchangeSpace &event) {
    //Random seed 'magic' works here because there are no calls to gRandom in Geant code
    //pass by the seed to Geant
    CLHEP::HepRandom::setTheSeed(gRandom->GetSeed());
    buffer.simEv = &event.simEvt;
    fRunManager->BeamOn(1);
    //return the seed to gRandom
    gRandom->SetSeed(CLHEP::HepRandom::getTheSeed());
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag GeantSim::Finish() {
    delete fRunManager;
    return fwk::VModule::eSuccess;
}
