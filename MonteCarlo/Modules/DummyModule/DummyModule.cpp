#include "DummyModule.h"

//unused attribute to silence compiler warnings
fwk::VModule::EResultFlag DummyModule::Init(__attribute__((unused)) boost::property_tree::ptree config) {
    return fwk::VModule::eSuccess;
}

//unused attribute to silence compiler warnings
fwk::VModule::EResultFlag DummyModule::Process(__attribute__((unused)) ModuleExchangeSpace &event) {
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag DummyModule::Finish() {
    return fwk::VModule::eSuccess;
}
