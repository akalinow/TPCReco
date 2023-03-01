#include "TriggerSimulator.h"

fwk::VModule::EResultFlag TriggerSimulator::Init(boost::property_tree::ptree config) {
    triggerArrival = config.get<double>("TriggerArrival");
    auto err=false;
    getZmin = geometry->Timecell2pos(0,err);
    getZmax = geometry->Timecell2pos(geometry->GetAgetNtimecells()-1,err);
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

//take into account only points in active area
double TriggerSimulator::findMinZ(SimEvent &ev) {
    std::vector<double> zPoints;
    for(const auto& t: ev.GetTracks() ){
        for(const auto& h: t.GetHits()){
            auto pos = h.GetPosition();
            if(geometry->IsInsideActiveVolume(pos))
                zPoints.push_back(pos.Z());
        }
    }

    return *std::min_element(zPoints.begin(), zPoints.end());
}
