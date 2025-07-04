#include "Generator.h"
#include "TPCReco/ConfigManager.h"

fwk::VModule::EResultFlag Generator::Init(boost::property_tree::ptree config) {
    evGen=std::make_unique<EventGenerator>(config.get_child("EventGenerator"));
    verbosity = ConfigManager::getScalar<unsigned int>(config, "Verbosity");
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag Generator::Process(ModuleExchangeSpace &event) {

    //Generate new event:
    event.simEvt=evGen->GenerateEvent();

    //skip empty events, the rest of the ModuleSequence will be skipped:
    if(event.simEvt.GetTracks().empty()) return fwk::VModule::eContinueLoop;
    nEventsGenerated++;
    if(nEventsGenerated%1000 == 0){
        std::cout<<"EventGenerator generated: "<<nEventsGenerated<<" non empty events."<<std::endl;
    }
    event.trackPEvt.resize(0); // reset transient vector of PEventTPC with true hits per track
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag Generator::Finish() {
    std::cout<<"Generator module generated "<<nEventsGenerated<<" non empty events."<<std::endl;
    return fwk::VModule::eSuccess;
}
