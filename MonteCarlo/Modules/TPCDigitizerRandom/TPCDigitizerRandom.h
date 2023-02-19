#ifndef TPCSOFT_TPCDIGITIZERRANDOM_H
#define TPCSOFT_TPCDIGITIZERRANDOM_H

#include "VModule.h"
#include "GeometryTPC.h"
#include "EventInfo.h"


class TPCDigitizerRandom : public fwk::VModule {
public:
    EResultFlag Init(boost::property_tree::ptree config) override;

    EResultFlag Process(ModuleExchangeSpace &event) override;

    EResultFlag Finish() override;

private:
    std::unique_ptr<GeometryTPC> geometry;
    std::unique_ptr<eventraw::EventInfo> aEventInfo;
    uint32_t eventID{};
    double MeVToChargeScale{1};
    double diffSigmaXY{};
    double diffSigmaZ{};
    unsigned int nSamplesPerHit{};

    REGISTER_MODULE(TPCDigitizerRandom)
};


#endif //TPCSOFT_TPCDIGITIZERRANDOM_H
