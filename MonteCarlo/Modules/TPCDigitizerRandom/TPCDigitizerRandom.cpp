#include "TPCDigitizerRandom.h"
#include "TRandom.h"

fwk::VModule::EResultFlag TPCDigitizerRandom::Init(boost::property_tree::ptree config) {
    geometry=std::make_unique<GeometryTPC>(config.get<std::string>("GeometryConfig").c_str());
    aEventInfo=std::make_unique<eventraw::EventInfo>();
    aEventInfo->SetPedestalSubtracted(true);
    aEventInfo->SetRunId(100);
    MeVToChargeScale=100000;
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag TPCDigitizerRandom::Process(ModuleExchangeSpace &event) {
    aEventInfo->SetEventId(eventID++);
    auto& currentSimEvent = event.simEvt;
    auto& currentPEventTPC = event.tpcPEvt;
    currentPEventTPC.Clear();
    float sigma = 0.68;
    int nTries=100;
    bool err_flag=false;
    //looop over tracks
    for(const auto &t: currentSimEvent.GetTracks()){
        //loop over hits:
        for(const auto& h : t.GetHits()){
            auto pos = h.GetPosition();
            auto edep=h.GetEnergy();
            for(int i=0;i<nTries;i++){
                auto smearedPosition = TVector3(
                        gRandom->Gaus(pos.X(),sigma),
                        gRandom->Gaus(pos.Y(),sigma),
                        gRandom->Gaus(pos.Z(),sigma)
                        );
                auto iPolyBin = geometry->GetTH2Poly()->FindBin(smearedPosition.X(), smearedPosition.Y());
                auto iCell = geometry->Pos2timecell(smearedPosition.Z(), err_flag);
                auto strip =  geometry->GetTH2PolyStrip(iPolyBin);
                if(strip){
                    currentPEventTPC.AddValByStrip(strip,iCell,edep/nTries*MeVToChargeScale);
                }
            }
        }
    }
    currentPEventTPC.SetEventInfo(*aEventInfo);
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag TPCDigitizerRandom::Finish() {
    return fwk::VModule::eSuccess;
}
