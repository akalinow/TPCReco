#ifndef TPCSOFT_TPCDIGITIZERSRC_H
#define TPCSOFT_TPCDIGITIZERSRC_H

#include "VModule.h"
#include "GeometryTPC.h"
#include "EventInfo.h"
#include "StripResponseCalculator.h"
#include "boost/filesystem.hpp"


class TPCDigitizerSRC : public fwk::VModule {
public:
    EResultFlag Init(boost::property_tree::ptree config) override;

    EResultFlag Process(ModuleExchangeSpace &event) override;

    EResultFlag Finish() override;

private:
    std::unique_ptr<eventraw::EventInfo> aEventInfo;
    std::unique_ptr<StripResponseCalculator> calculator;
    std::shared_ptr<PEventTPC> currentPEventTPC;

    uint32_t eventID{};
    double MeVToChargeScale{1};
    double diffSigmaXY{};
    double diffSigmaZ{};
    int th2PolyPartitionX{};
    int th2PolyPartitionY{};
    double peakingTime{};
    int nStrips{};
    int nCells{};
    int nPads{};
    boost::filesystem::path pathToResponses;
    REGISTER_MODULE(TPCDigitizerSRC)
};


#endif //TPCSOFT_TPCDIGITIZERSRC_H
