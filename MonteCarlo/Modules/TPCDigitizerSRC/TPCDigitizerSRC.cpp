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
        std::cout << "File " << filePath
                  << " does not exist, generating strip response with default parameters form StripResponseCalculator!"
                  << std::endl;
        calculator = std::make_unique<StripResponseCalculator>(geometry, nStrips, nCells, nPads, diffSigmaXY,
                                                               diffSigmaZ, peakingTime);
    }

    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag TPCDigitizerSRC::Process(ModuleExchangeSpace &event) {
    aEventInfo->SetEventId(eventID++);
    auto &currentSimEvent = event.simEvt;
    //hacky way to provide shared_ptr<PEventTPC> to calculator
    currentPEventTPC = std::shared_ptr<PEventTPC>(&event.tpcPEvt,boost::null_deleter());
    currentPEventTPC->Clear();
    //loop over tracks
    for (const auto &t: currentSimEvent.GetTracks()) {
        //loop over hits
        for (const auto &h: t.GetHits()) {
            auto pos = h.GetPosition();
            auto edep = h.GetEnergy();
            calculator->addCharge(pos, edep * MeVToChargeScale, currentPEventTPC);
        }
    }
    currentPEventTPC->SetEventInfo(*aEventInfo);
    event.eventInfo = *aEventInfo;
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag TPCDigitizerSRC::Finish() {
    return fwk::VModule::eSuccess;
}
