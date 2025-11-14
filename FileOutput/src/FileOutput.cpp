#include <cstdlib>
#include <iostream>

#include <TFile.h>
#include <TTree.h>


#include "TPCReco/EventSourceBase.h"
#include "TPCReco/EventSourceMC.h"
#include "TPCReco/SimEvent.h"
#include "TPCReco/PEventTPC.h"
#include "TPCReco/EventTPC.h"
#include "TPCReco/Track3D.h"

#include "TPCReco/ConfigManager.h"
#include "TPCReco/InputFileHelper.h"
#include "TPCReco/FileOutput.h"
#include "TPCReco/colorText.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
FileOutput::FileOutput(boost::property_tree::ptree & aConfig) {

  init(aConfig);
}
////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
FileOutput::FileOutput() {}
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
void FileOutput::init(boost::property_tree::ptree aConfig){

  auto fileName = ConfigManager::getScalar<std::string>(aConfig, "output.fileName");
  if(fileName.find(".root")==std::string::npos){ 
    std::string inputFileName = aConfig.get<std::string>("input.dataFile","");
    fileName = InputFileHelper::makeOutputFileName(inputFileName, fileName);
  }

  myOutputFilePtr = std::make_shared<TFile>(fileName.c_str(), "RECREATE");
  std::cout<<KBLU<<"Opening output stream to file: "<<RST<<fileName<<std::endl;

  myEventInfoPtr = std::make_shared<eventraw::EventInfo>();
  myPEventPtr = std::make_shared<PEventTPC>();
  myRecoEventPtr = std::make_shared<Track3D>();
  mySimEventPtr = std::make_shared<Track3D>();

  std::vector<std::string> treeNames = {"TPCData"};
  auto autoSaveFreq = ConfigManager::getScalar<int>(aConfig, "output.autoSaveFrequency");
  for (const auto &treeName : treeNames) {
      myOutputTrees[treeName] = std::make_shared<TTree>(treeName.c_str(), "");
      myOutputTrees[treeName]->SetDirectory(myOutputFilePtr.get());
      if(treeName=="TPCData") {
        myOutputTrees[treeName]->SetAutoSave(autoSaveFreq); 
        myOutputTrees[treeName]->Branch("EventInfo", myEventInfoPtr.get());
        myOutputTrees[treeName]->Branch("Event", myPEventPtr.get());
        myOutputTrees[treeName]->Branch("SimEvent", mySimEventPtr.get());
        myOutputTrees[treeName]->Branch("RecoEvent", myRecoEventPtr.get());
  }

  //setup OFF branches if there are any:
  auto disabledBranches = aConfig.get_child_optional("output.disabledBranches");
  if(disabledBranches) {
    std::cout<<KBLU<<"FileOutput::init disabling branches: "<<RST<<std::endl;
      for (const auto &br: *disabledBranches) {
          auto tree_branchName = std::string(br.second.data());
          auto treeName = tree_branchName.substr(0, tree_branchName.find('.'));
          auto branchName = tree_branchName.substr(tree_branchName.find('.')+1);
          std::cout<<"\t disabling branch: "<<tree_branchName<<std::endl;
          myOutputTrees[treeName]->SetBranchStatus(branchName.c_str(), false);
      }
    }
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
FileOutput::~FileOutput() {

  std::cout<<KBLU<<"FileOutput::~FileOutput"<<RST
           <<" closing output file."<<std::endl;
  close();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void FileOutput::setRecoEvent(const Track3D & aRecTrack){

  *myRecoEventPtr = aRecTrack;
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void FileOutput::close(){

  if(!myOutputFilePtr){
     std::cout<<KRED<<"FileOutput::close"<<RST
	     <<" pointer to output file not set!"
	     <<std::endl;
     return;
  }
  myOutputFilePtr->cd();
  for(auto aTree: myOutputTrees) {
    if(aTree.second) {
      aTree.second->Write("", TObject::kOverwrite);
    }
  }
  myOutputFilePtr->Close();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void FileOutput::update(std::shared_ptr<EventSourceBase> aEventSource) {

  *myPEventPtr = *aEventSource->getCurrentPEvent();
  *myEventInfoPtr = aEventSource->getCurrentEvent()->GetEventInfo();
  *myRecoEventPtr = aEventSource->getRecoEvent();
  auto mcEventSource = std::dynamic_pointer_cast<EventSourceMC>(aEventSource);
  if(mcEventSource) *mySimEventPtr = mcEventSource->getGeneratedTrack();
  else *mySimEventPtr = aEventSource->getRecoEvent(); //Hack by AK to avoid troubles in ROOT->Python step

  for(auto aTree: myOutputTrees) {
    if (aTree.second==nullptr) {
      std::cout << KRED << "FileOutput::update" << RST
                << " pointer to output tree not set for tree: "
                << aTree.first << std::endl;
      continue;
    }
    aTree.second->Fill();
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void FileOutput::update(const eventraw::EventInfo & aEventInfoPtr,
                        const Track3D & aRecoEventPtr){

   *myEventInfoPtr = aEventInfoPtr;
   *myRecoEventPtr = aRecoEventPtr;

    for(auto aTree: myOutputTrees) {
    if (aTree.second==nullptr) {
      std::cout << KRED << "FileOutput::update" << RST
                << " pointer to output tree not set for tree: "
                << aTree.first << std::endl;
      continue;
    }
    aTree.second->Fill();
  } 
}
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
