#ifndef TPCSOFT_GENERATOR_H
#define TPCSOFT_GENERATOR_H

#include "VModule.h"
#include "EventGenerator.h"

class Generator : public fwk::VModule{
public:
    EResultFlag Init(boost::property_tree::ptree config) override;
    fwk::VModule::EResultFlag Process(evt::Event& event) override;
    fwk::VModule::EResultFlag Finish() override;

private:

    std::unique_ptr<EventGenerator> evGen;

    REGISTER_MODULE(Generator);
};

#endif //TPCSOFT_GENERATOR_H
