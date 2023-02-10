#ifndef TPCSOFT_GEANTSIM_H
#define TPCSOFT_GEANTSIM_H

#include "VModule.h"
#include "DataBuffer.h"

#include <CLHEP/Random/MTwistEngine.h>

class G4RunManager;

class G4VUserPhysicsList;

class G4UIExecutive;

class G4VisManager;


class GeantSim : public fwk::VModule {
public:
    EResultFlag Init(boost::property_tree::ptree config) override;

    fwk::VModule::EResultFlag Process(ModuleExchangeSpace &event) override;

    fwk::VModule::EResultFlag Finish() override;

private:
    /** Given ROOT's TRandom3, the random engine used natively in
        Shine, not only uses the same algorithm as CLHEP's
        MTwistEngine but is actually based on it, it is quite obvious
        which HepRandomEngine we should use. */
    CLHEP::MTwistEngine fCLHEPRandomEngine;
    G4RunManager *fRunManager{};
    G4UIExecutive *fUserInterface{};
    G4VisManager *fVisManager{};
    DataBuffer buffer{};


REGISTER_MODULE(GeantSim)
};

#endif //TPCSOFT_GEANTSIM_H
