#include "EventFileExporter.h"
#include "SaveCurrentTDirectory.h"

EventFileExporter::EventFileExporter()
        : file{nullptr}, tree{nullptr}, simEventBranch{nullptr}, currSimEvent{nullptr} {}

fwk::VModule::EResultFlag EventFileExporter::Init(boost::property_tree::ptree config) {
    //create file and ttree
    auto fname = config.get<std::string>("FileName");
    utl::SaveCurrentTDirectory s;
    file=new TFile(fname.c_str(),"RECREATE");
    tree = new TTree("eventTree","");

    //setup branches:
    for(const auto& br: config.get_child("EnabledBranches")){
        auto branchName = std::string(br.second.data());
        if(branchName=="SimEvent")
            tree->Branch("SimEvent","SimEvent",&currSimEvent);

    }
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag EventFileExporter::Process(ModuleExchangeSpace &event) {
    currSimEvent=&(event.simEvt);
    tree->Fill();
    return fwk::VModule::eSuccess;
}

fwk::VModule::EResultFlag EventFileExporter::Finish() {
    utl::SaveCurrentTDirectory s;
    file->cd();
    tree->Write("",TObject::kWriteDelete);
    file->Close();
    delete file;
    return fwk::VModule::eSuccess;
}