#include "TriggerSimulator.h"

fwk::VModule::EResultFlag TriggerSimulator::Init(boost::property_tree::ptree config) {
    triggerArrival = config.get<double>("TriggerArrival");
    auto err=false;
    getZmin = geometry->Timecell2pos(config.get<double>("GetCellMin"),err);
    getZmax = geometry->Timecell2pos(config.get<double>("GetCellMax"),err);
    return eSuccess;
}

fwk::VModule::EResultFlag TriggerSimulator::Process(ModuleExchangeSpace &event) {
    auto& currentSimEv = event.simEvt;
    auto minZFromTrack = findMinZ(currentSimEv);
    auto zOffset = -minZFromTrack + getZmin + triggerArrival * (getZmax - getZmin);
    TVector3 offset{0,0,zOffset};
    currentSimEv.Shift(offset);
    return eSuccess;
}

fwk::VModule::EResultFlag TriggerSimulator::Finish() {
    return eSuccess;
}

double TriggerSimulator::findMinZ(SimEvent &ev) {
    std::vector<double> zEnds;
    for(const auto& t: ev.GetTracks() ){
        zEnds.push_back(t.GetStart().Z());
        zEnds.push_back(t.GetStop().Z());
    }

    return *std::min_element(zEnds.begin(),zEnds.end());
}
