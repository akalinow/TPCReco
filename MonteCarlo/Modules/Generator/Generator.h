#ifndef TPCSOFT_GENERATOR_H
#define TPCSOFT_GENERATOR_H

#include "TPCReco/VModule.h"
#include "TPCReco/EventGenerator.h"

class Generator : public fwk::VModule{
public:
    EResultFlag Init(boost::property_tree::ptree config) override;
    fwk::VModule::EResultFlag Process(ModuleExchangeSpace &event) override;
    fwk::VModule::EResultFlag Finish() override;

private:

    std::unique_ptr<EventGenerator> evGen;
    unsigned int nEventsToGenerate{0};
    unsigned int nEventsGenerated{0};

    REGISTER_MODULE(Generator)
};

#endif //TPCSOFT_GENERATOR_H
