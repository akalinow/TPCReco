#ifndef TPCSOFT_TRIGGERSIMULATOR_H
#define TPCSOFT_TRIGGERSIMULATOR_H

#include "VModule.h"
#include "GeometryTPC.h"

class TriggerSimulator : public fwk::VModule{
public:
    EResultFlag Init(boost::property_tree::ptree config) override;

    EResultFlag Process(ModuleExchangeSpace &event) override;

    EResultFlag Finish() override;
private:
    static double findMinZ(SimEvent& ev);

    double triggerArrival{};
    double getZmin{}, getZmax{};
    std::shared_ptr<GeometryTPC> geometry;
REGISTER_MODULE(TriggerSimulator)
};


#endif //TPCSOFT_TRIGGERSIMULATOR_H
