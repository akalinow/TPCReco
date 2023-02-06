#include "GeantSim.h"

fwk::VModule::EResultFlag GeantSim::Init(boost::property_tree::ptree config) {
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag GeantSim::Process(ModuleExchangeSpace &event) {
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag GeantSim::Finish() {
    return fwk::VModule::eSuccess;
}
