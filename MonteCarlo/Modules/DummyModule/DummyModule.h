#ifndef TPCSOFT_DUMMYMODULE_H
#define TPCSOFT_DUMMYMODULE_H

#include "TPCReco/VModule.h"

class DummyModule : public fwk::VModule{
public:
    EResultFlag Init(boost::property_tree::ptree config) override;
    fwk::VModule::EResultFlag Process(ModuleExchangeSpace &event) override;
    fwk::VModule::EResultFlag Finish() override;

    REGISTER_MODULE(DummyModule)
};

#endif //TPCSOFT_DUMMYMODULE_H
