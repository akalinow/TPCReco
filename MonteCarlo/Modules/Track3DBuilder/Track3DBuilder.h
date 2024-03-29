#ifndef TPCSOFT_TRACK3DBUILDER_H
#define TPCSOFT_TRACK3DBUILDER_H

#include "TPCReco/VModule.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/EventInfo.h"


class Track3DBuilder : public fwk::VModule {
public:
    EResultFlag Init(boost::property_tree::ptree config) override;

    EResultFlag Process(ModuleExchangeSpace &event) override;

    EResultFlag Finish() override;

private:
    uint32_t eventID{};

    REGISTER_MODULE(Track3DBuilder)
};


#endif //TPCSOFT_TRACK3DBUILDER_H
