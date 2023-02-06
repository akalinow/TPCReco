#ifndef TPCSOFT_EVENTFILEEXPORTER_H
#define TPCSOFT_EVENTFILEEXPORTER_H

#include "VModule.h"
#include "TTree.h"
#include "TFile.h"
#include "ModuleExchangeSpace.h"

class EventFileExporter: public fwk::VModule{
    EventFileExporter();
    EResultFlag Init(boost::property_tree::ptree config) override;
    fwk::VModule::EResultFlag Process(ModuleExchangeSpace &event) override;
    fwk::VModule::EResultFlag Finish() override;
private:
    TFile* file;
    TTree* tree;
    TBranch* simEventBranch;
    SimEvent* currSimEvent;
    std::vector<std::string> offBranches;
    std::vector<std::string> onBranches;

    REGISTER_MODULE(EventFileExporter)
};

#endif //TPCSOFT_EVENTFILEEXPORTER_H
