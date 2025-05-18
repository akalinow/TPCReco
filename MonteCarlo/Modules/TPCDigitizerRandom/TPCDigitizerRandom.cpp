#include "TPCDigitizerRandom.h"
#include "TRandom.h"

fwk::VModule::EResultFlag TPCDigitizerRandom::Init(boost::property_tree::ptree config) {
    aEventInfo = std::make_unique<eventraw::EventInfo>();
    aEventInfo->SetPedestalSubtracted(true);
    aEventInfo->SetRunId(100);
    MeVToChargeScale = config.get<double>("MeVToChargeScale");
    diffSigmaXYmin = config.get<double>("sigmaXYmin");
    diffSigmaXYmax = config.get<double>("sigmaXYmax");
    diffSigmaZmin = config.get<double>("sigmaZmin");
    diffSigmaZmax = config.get<double>("sigmaZmax");
    nSamplesPerHit = config.get<unsigned int>("NSamplesPerHit");
    enableSimHitsPerTrack = config.get<bool>("transient.enableSimHitsPerTrack"); // enabled when Track3DBuilder MC module is present
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag TPCDigitizerRandom::Process(ModuleExchangeSpace &event) {
    aEventInfo->SetEventId(eventID++);
    auto &currentSimEvent = event.simEvt;
    auto &currentPEventTPC = event.tpcPEvt;
    currentPEventTPC.Clear();

    // used by Track3DBuilder to store true RecHits per individual generator level TrackSegment3D
    auto iTrack=0U; // track iterator for event.trackPEvt[] vector 
    if(enableSimHitsPerTrack) {
      event.trackPEvt.resize(currentSimEvent.GetTracks().size());
      for(auto &pEvt : event.trackPEvt) {
	pEvt.Clear();
      }
    }

    bool err_flag = false;
    //loop over tracks
    // diffsigmaXY = rand->Gaus(0, diffSigmaXY);
    diffSigmaXY = gRandom->Uniform(diffSigmaXYmin, diffSigmaXYmax); // TODO: replace by diffsigmaXY(Z_DET)
    diffSigmaZ = gRandom->Uniform(diffSigmaZmin, diffSigmaZmax); // TODO: replace by diffsigmaZ(Z_DET)
    for (auto &t: currentSimEvent.GetTracks()) {
        iTrack++;
        //loop over hits
        for (auto &h: t.GetHits()) {
            auto pos = h.GetPosition();
            auto edep = h.GetEnergy();
            auto isIn = geometry->IsInsideActiveVolume(pos);
            h.SetInside(isIn);
            if(isIn) {
                for (unsigned int i = 0; i < nSamplesPerHit; i++) {
                    auto smearedPosition = TVector3(
                            gRandom->Gaus(pos.X(), diffSigmaXY),
                            gRandom->Gaus(pos.Y(), diffSigmaXY),
                            gRandom->Gaus(pos.Z(), diffSigmaZ)
                    );
                    auto iPolyBin = geometry->GetTH2Poly()->FindBin(smearedPosition.X(), smearedPosition.Y());
                    auto iCell = static_cast<int>(geometry->Pos2timecell(smearedPosition.Z(), err_flag));
                    auto strip = geometry->GetTH2PolyStrip(iPolyBin);
                    if (strip && !err_flag) {
                        currentPEventTPC.AddValByStrip(strip, iCell, edep / nSamplesPerHit * MeVToChargeScale);
			// used by Track3DBuilder to store true RecHits per individual generator level TrackSegment3D
			if(enableSimHitsPerTrack) event.trackPEvt.at(iTrack-1).AddValByStrip(strip, iCell, edep / nSamplesPerHit * MeVToChargeScale);
                    }
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
