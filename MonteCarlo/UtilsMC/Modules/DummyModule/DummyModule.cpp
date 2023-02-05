#include "DummyModule.h"

fwk::VModule::EResultFlag DummyModule::Init(boost::property_tree::ptree config) {
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag DummyModule::Process(ModuleExchangeSpace &event) {
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag DummyModule::Finish() {
    return fwk::VModule::eSuccess;
}
