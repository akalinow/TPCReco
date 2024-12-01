#include "TPCDigitizerSRC.h"
#include "boost/core/null_deleter.hpp"

namespace fs = boost::filesystem;

fwk::VModule::EResultFlag TPCDigitizerSRC::Init(boost::property_tree::ptree config) {
    aEventInfo = std::make_unique<eventraw::EventInfo>();
    aEventInfo->SetPedestalSubtracted(true);
    aEventInfo->SetRunId(100);

    MeVToChargeScale = config.get<double>("MeVToChargeScale");
    diffSigmaXY = config.get<double>("sigmaXY");
    diffSigmaZ = config.get<double>("sigmaZ");
    th2PolyPartitionX = config.get<int>("th2PolyPartitionX");
    th2PolyPartitionY = config.get<int>("th2PolyPartitionY");
    peakingTime = config.get<int>("peakingTime");
    nStrips = config.get<int>("nStrips");
    nCells = config.get<int>("nCells");
    nPads = config.get<int>("nPads");
    pathToResponses = config.get<fs::path>("StripResponsePath");

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

    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag TPCDigitizerSRC::Process(ModuleExchangeSpace &event) {
    aEventInfo->SetEventId(eventID++);
    auto &currentSimEvent = event.simEvt;
    currentPEventTPC = std::shared_ptr<PEventTPC>(&event.tpcPEvt,boost::null_deleter());
    currentPEventTPC->Clear();
    // Loop over tracks
    for (auto &t: currentSimEvent.GetTracks()) {
        // Loop over hits
        for (auto &h: t.GetHits()) {
            auto pos = h.GetPosition();
            auto edep = h.GetEnergy();
            auto isIn = geometry->IsInsideActiveVolume(pos);
            h.SetInside(isIn);
            if (isIn)
                calculator->addCharge(pos, edep * MeVToChargeScale, currentPEventTPC);
        }
    }
    currentPEventTPC->SetEventInfo(*aEventInfo);
    event.eventInfo = *aEventInfo;
    event.tpcPEvt = *currentPEventTPC;
    std::cout << "Event ID: " << aEventInfo->GetEventId() << std::endl;
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag TPCDigitizerSRC::Finish() {
    return fwk::VModule::eSuccess;
}
