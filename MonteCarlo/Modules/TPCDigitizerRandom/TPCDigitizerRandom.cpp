#include "TPCDigitizerRandom.h"
#include "TRandom.h"

fwk::VModule::EResultFlag TPCDigitizerRandom::Init(boost::property_tree::ptree config) {
    geometry = std::make_unique<GeometryTPC>(config.get<std::string>("GeometryConfig").c_str());
    aEventInfo = std::make_unique<eventraw::EventInfo>();
    aEventInfo->SetPedestalSubtracted(true);
    aEventInfo->SetRunId(100);
    MeVToChargeScale = config.get<double>("MeVToChargeScale");
    diffSigmaXY = config.get<double>("sigmaXY");
    diffSigmaZ = config.get<double>("sigmaZ");
    nSamplesPerHit = config.get<unsigned int>("NSamplesPerHit");
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag TPCDigitizerRandom::Process(ModuleExchangeSpace &event) {
    aEventInfo->SetEventId(eventID++);
    auto &currentSimEvent = event.simEvt;
    auto &currentPEventTPC = event.tpcPEvt;
    currentPEventTPC.Clear();
    bool err_flag = false;
    //loop over tracks
    for (const auto &t: currentSimEvent.GetTracks()) {
        //loop over hits
        for (const auto &h: t.GetHits()) {
            auto pos = h.GetPosition();
            auto edep = h.GetEnergy();
            for (unsigned int i = 0; i < nSamplesPerHit; i++) {
                auto smearedPosition = TVector3(
                        gRandom->Gaus(pos.X(), diffSigmaXY),
                        gRandom->Gaus(pos.Y(), diffSigmaXY),
                        gRandom->Gaus(pos.Z(), diffSigmaZ)
                );
                auto iPolyBin = geometry->GetTH2Poly()->FindBin(smearedPosition.X(), smearedPosition.Y());
                auto iCell = static_cast<int>(geometry->Pos2timecell(smearedPosition.Z(), err_flag));
                auto strip = geometry->GetTH2PolyStrip(iPolyBin);
                if (strip) {
                    currentPEventTPC.AddValByStrip(strip, iCell, edep / nSamplesPerHit * MeVToChargeScale);
                }
            }
        }
    }
    currentPEventTPC.SetEventInfo(*aEventInfo);
    event.eventInfo = *aEventInfo;
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag TPCDigitizerRandom::Finish() {
    return fwk::VModule::eSuccess;
}
