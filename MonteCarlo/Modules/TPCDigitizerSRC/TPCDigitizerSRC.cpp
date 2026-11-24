#include "TPCDigitizerSRC.h"
#include "boost/core/null_deleter.hpp"
#include "TPCReco/ConfigManager.h"
#include "TRandom.h"
#include <cmath>

namespace fs = boost::filesystem;

fwk::VModule::EResultFlag TPCDigitizerSRC::Init(boost::property_tree::ptree config) {
    // NOTE: Arithmetic BOOST ptree members are accessed via static method: ConfigManager::getScalar<double>(tree, "some.branch")
    //       instead of: tree.get<double>("some.branch")
    //       in order to enable MATH expressions in MC JSON config files (e.g. "M_PI/2")

    aEventInfo = std::make_unique<eventraw::EventInfo>();
    aEventInfo->SetPedestalSubtracted(true);
    aEventInfo->SetRunId(100);

    // MeVToChargeScale = config.get<double>("MeVToChargeScale"); // without MATH expressions
    // diffSigmaXY = config.get<double>("sigmaXY"); // without MATH expressions
    // diffSigmaZ = config.get<double>("sigmaZ"); // without MATH expressions
    // th2PolyPartitionX = config.get<int>("th2PolyPartitionX"); // without MATH expressions
    // th2PolyPartitionY = config.get<int>("th2PolyPartitionY"); // without MATH expressions
    // peakingTime = config.get<int>("peakingTime"); // without MATH expressions
    // nStrips = config.get<int>("nStrips"); // without MATH expressions
    // nCells = config.get<int>("nCells"); // without MATH expressions
    // nPads = config.get<int>("nPads"); // without MATH expressions
    pathToResponses = config.get<fs::path>("StripResponsePath"); // without MATH expressions
    MeVToChargeScale = ConfigManager::getScalar<double>(config, "MeVToChargeScale"); // [ADC counts/MeV]
    diffSigmaXY = ConfigManager::getScalar<double>(config, "sigmaXY"); // [mm]
    diffSigmaZ = ConfigManager::getScalar<double>(config, "sigmaZ"); // [mm]
    th2PolyPartitionX = ConfigManager::getScalar<int>(config, "th2PolyPartitionX");
    th2PolyPartitionY = ConfigManager::getScalar<int>(config, "th2PolyPartitionY");
    peakingTime = ConfigManager::getScalar<int>(config, "peakingTime"); // [ns]
    nStrips = ConfigManager::getScalar<int>(config, "nStrips"); // strip response model: +/- # of neighbour strips
    nCells = ConfigManager::getScalar<int>(config, "nCells"); // strip response model: +/- # of neighbour time cells
    nPads = ConfigManager::getScalar<double>(config, "nPads"); // strip response model: +/- # of neighbour diamond-shaped pads along the strip

    geometry->SetTH2PolyPartition(th2PolyPartitionX,th2PolyPartitionY);

    auto fname = StripResponseCalculator::generateRootFileName(nStrips, nCells, nPads, diffSigmaXY, diffSigmaZ,
                                                               peakingTime, geometry->GetSamplingRate(),
                                                               geometry->GetDriftVelocity());
    auto filePath = pathToResponses / fname;
    if (fs::exists(filePath)) {
        calculator = std::make_unique<StripResponseCalculator>(geometry, nStrips, nCells, nPads, diffSigmaXY,
                                                               diffSigmaZ, peakingTime, filePath.c_str());
    } else {
        std::stringstream msg;
        msg << "File " << filePath
                  << " does not exist! Please make sure a ROOT file with strip responses is present in given directory!"
                  << std::endl;
        throw std::runtime_error(msg.str());
    }
    enableSimHitsPerTrack = config.get<bool>("transient.enableSimHitsPerTrack"); // enabled when Track3DBuilder MC module is present
    driftDiffusionT = config.get_optional<std::string>("driftDiffusionT") ? ConfigManager::getScalar<double>(config, "driftDiffusionT") : 0.0;
    driftDiffusionL = config.get_optional<std::string>("driftDiffusionL") ? ConfigManager::getScalar<double>(config, "driftDiffusionL") : 0.0;
    driftGEMZ = config.get_optional<std::string>("driftGEMZ") ? ConfigManager::getScalar<double>(config, "driftGEMZ") : geometry->GetDriftCageZmin();
    if(config.get_optional<std::string>("driftZRef")) throw std::runtime_error("TPCDigitizerSRC: driftZRef is obsolete, the drift distance is taken from the GEM plane (driftGEMZ)");
    driftSubCharges = config.get_optional<std::string>("driftSubCharges") ? ConfigManager::getScalar<int>(config, "driftSubCharges") : 10;
    if(driftDiffusionT<0 || driftDiffusionL<0 || driftSubCharges<1) throw std::runtime_error("TPCDigitizerSRC: invalid drift diffusion settings");
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag TPCDigitizerSRC::Process(ModuleExchangeSpace &event) {
    aEventInfo->SetEventId(eventID++);
    auto &currentSimEvent = event.simEvt;
    auto null_deleter = [](PEventTPC*) {};
    currentPEventTPC = std::shared_ptr<PEventTPC>(&event.tpcPEvt, null_deleter);
    currentPEventTPC->Clear();

    // used by Track3DBuilder to store true RecHits per individual generator level TrackSegment3D
    auto iTrack=0U; // track iterator for event.trackPEvt[] vector
    if(enableSimHitsPerTrack) {
      event.trackPEvt.resize(currentSimEvent.GetTracks().size());
    }

    // z shift applied by TriggerSimulator (0 without it): z_true = z - triggerShiftZ
    const double triggerShiftZ = currentSimEvent.GetTrigShiftedVertexPosition().Z() - currentSimEvent.GetTrueVertexPosition().Z();

    // Loop over tracks
    for (auto &t: currentSimEvent.GetTracks()) {
        iTrack++;
        // used by Track3DBuilder to store true RecHits per individual generator level TrackSegment3D
        auto trackPEventTPC = std::shared_ptr<PEventTPC>((enableSimHitsPerTrack ? &event.trackPEvt.at(iTrack-1) : nullptr), null_deleter);
        if(enableSimHitsPerTrack) {
	  trackPEventTPC->Clear();
	}

        // Loop over hits
        for (auto &h: t.GetHits()) {
            auto pos = h.GetPosition();
            auto edep = h.GetEnergy();
            auto isIn = geometry->IsInsideActiveVolume(pos);
            h.SetInside(isIn);
            if (isIn) {
	      const double drift = std::max(0.0, pos.Z() - triggerShiftZ - driftGEMZ) / 10.0; // [cm] physical drift distance
	      const double sT = driftDiffusionT * std::sqrt(drift), sL = driftDiffusionL * std::sqrt(drift);
	      if (sT > 0 || sL > 0) {
		const double q = edep * MeVToChargeScale / driftSubCharges;
		for (int i = 0; i < driftSubCharges; i++) {
		  TVector3 p(gRandom->Gaus(pos.X(), sT), gRandom->Gaus(pos.Y(), sT), gRandom->Gaus(pos.Z(), sL));
		  calculator->addCharge(p, q, currentPEventTPC);
		  if(enableSimHitsPerTrack) calculator->addCharge(p, q, trackPEventTPC);
		}
	      } else {
		calculator->addCharge(pos, edep * MeVToChargeScale, currentPEventTPC);
		// used by Track3DBuilder to store true RecHits per individual generator level TrackSegment3D
		if(enableSimHitsPerTrack) calculator->addCharge(pos, edep * MeVToChargeScale, trackPEventTPC);
	      }
	    }
        }
    }
    currentPEventTPC->SetEventInfo(*aEventInfo);
    event.eventInfo = *aEventInfo;
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag TPCDigitizerSRC::Finish() {
    return fwk::VModule::eSuccess;
}
