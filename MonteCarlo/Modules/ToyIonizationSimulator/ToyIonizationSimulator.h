#ifndef TPCSOFT_TOYIONIZATIONSIMULATOR_H
#define TPCSOFT_TOYIONIZATIONSIMULATOR_H

#include "TPCReco/VModule.h"
#include "TPCReco/IonRangeCalculator.h"

class ToyIonizationSimulator : public fwk::VModule{
public:
    EResultFlag Init(boost::property_tree::ptree config) override;

    EResultFlag Process(ModuleExchangeSpace &event) override;

    EResultFlag Finish() override;
private:
    std::unique_ptr<IonRangeCalculator> rangeCalc;
    double pointsPerMm{1};
    REGISTER_MODULE(ToyIonizationSimulator)
};


#endif //TPCSOFT_TOYIONIZATIONSIMULATOR_H
