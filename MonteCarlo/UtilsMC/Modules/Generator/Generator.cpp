#include "Generator.h"

fwk::VModule::EResultFlag Generator::Init(boost::property_tree::ptree config) {
    evGen=std::make_unique<EventGenerator>(config.get_child("EventGenerator"));

    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag Generator::Process(evt::Event &event) {
    evGen->GenerateEvent();
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag Generator::Finish() {
    return fwk::VModule::eSuccess;
}
