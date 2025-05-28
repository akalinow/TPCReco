#include "Generator.h"
#include "TPCReco/ConfigManager.h"

fwk::VModule::EResultFlag Generator::Init(boost::property_tree::ptree config) {
    // NOTE: Arithmetic BOOST ptree members are accessed via static method: ConfigManager::getScalar<double>(tree, "some.branch")
    //       instead of: tree.get<double>("some.branch")
    //       in order to enable MATH expressions in MC JSON config files (e.g. "M_PI/2")

    evGen=std::make_unique<EventGenerator>(config.get_child("EventGenerator"));
    // nEventsToGenerate = config.get<unsigned int>("NumberOfEvents"); // without MATH expressions
    nEventsToGenerate = ConfigManager::getScalar<unsigned int>(config, "NumberOfEvents");
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag Generator::Process(ModuleExchangeSpace &event) {
    //break ModuleSequence loop when we reach desired number of events
    if(nEventsGenerated==nEventsToGenerate)
        return fwk::VModule::eBreakLoop;
    //Generate new event:
    event.simEvt=evGen->GenerateEvent();
    //skip empty events, the rest of the ModuleSequence will be skipped:
    if(event.simEvt.GetTracks().empty())
        return fwk::VModule::eContinueLoop;
    nEventsGenerated++;
    if(nEventsGenerated%1000 == 0){
        std::cout<<"EventGenerator generated: "<<nEventsGenerated<<" events."<<std::endl;
    }
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag Generator::Finish() {
    std::cout<<"Generator module generated "<<nEventsGenerated<<" events."<<std::endl;
    return fwk::VModule::eSuccess;
}
