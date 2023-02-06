#ifndef TPCSOFT_GEANTSIM_H
#define TPCSOFT_GEANTSIM_H

#include "VModule.h"

class GeantSim : public fwk::VModule{
public:
    EResultFlag Init(boost::property_tree::ptree config) override;
    fwk::VModule::EResultFlag Process(ModuleExchangeSpace &event) override;
    fwk::VModule::EResultFlag Finish() override;

    REGISTER_MODULE(GeantSim)
};

#endif //TPCSOFT_GEANTSIM_H
