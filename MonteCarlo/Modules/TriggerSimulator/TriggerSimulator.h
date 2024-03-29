#ifndef TPCSOFT_TRIGGERSIMULATOR_H
#define TPCSOFT_TRIGGERSIMULATOR_H

#include "TPCReco/VModule.h"
#include "TPCReco/GeometryTPC.h"

class TriggerSimulator : public fwk::VModule{
public:
    EResultFlag Init(boost::property_tree::ptree config) override;

    EResultFlag Process(ModuleExchangeSpace &event) override;

    EResultFlag Finish() override;
private:
    double findMinZ(SimEvent& ev);

    double triggerArrival{};
    double getZmin{}, getZmax{};

REGISTER_MODULE(TriggerSimulator)
};


#endif //TPCSOFT_TRIGGERSIMULATOR_H
