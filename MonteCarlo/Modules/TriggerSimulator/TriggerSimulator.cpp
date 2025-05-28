#include "TriggerSimulator.h"
#include "TPCReco/ConfigManager.h"

fwk::VModule::EResultFlag TriggerSimulator::Init(boost::property_tree::ptree config) {
    // NOTE: Arithmetic BOOST ptree members are accessed via static method: ConfigManager::getScalar<double>(tree, "some.branch")
    //       instead of: tree.get<double>("some.branch")
    //       in order to enable MATH expressions in MC JSON config files (e.g. "M_PI/2")

    // triggerArrival = ConfigManager::getScalar<double>(config, "TriggerArrival"); // without MATH expressionss
    triggerArrival = ConfigManager::getScalar<double>(config, "TriggerArrival"); // range [0,1]
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
    for(auto& t: ev.GetTracks() ){
        bool trackInside = false;
        for(const auto& h: t.GetHits()){
            auto pos = h.GetPosition();
            if(geometry->IsInsideActiveVolume(pos)) {
                zPoints.push_back(pos.Z());
                trackInside =true;
            }
        }
        t.SetOutOfActiveVolume(!trackInside);
    }
    //if there are no energy deposits inside the active volume we return zero here
    if(zPoints.empty())
        return 0;
    return *std::min_element(zPoints.begin(), zPoints.end());
}
