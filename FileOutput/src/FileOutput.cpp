#include <cstdlib>
#include <iostream>

#include <TFile.h>
#include <TTree.h>
#include <TBranch.h>


#include "TPCReco/EventSourceBase.h"
#include "TPCReco/EventSourceMC.h"
#include "TPCReco/SimEvent.h"
#include "TPCReco/PEventTPC.h"
#include "TPCReco/EventTPC.h"
#include "TPCReco/Track3D.h"

#include "TPCReco/ConfigManager.h"
#include "TPCReco/InputFileHelper.h"
#include "TPCReco/FileOutput.h"
#include "TPCReco/MLTree.h"
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
  if(aConfig.get<bool>("output.mlTree", false)){
    myMLTree = std::make_shared<MLTree>(aConfig);
    treeNames.push_back("MLData");
  }
  auto autoSaveFreq = ConfigManager::getScalar<int>(aConfig, "output.autoSaveFrequency");
  for (const auto &treeName : treeNames) {
      myOutputTrees[treeName] = std::make_shared<TTree>(treeName.c_str(), "");
      myOutputTrees[treeName]->SetDirectory(myOutputFilePtr.get());
      if(treeName=="MLData") {
        myOutputTrees[treeName]->SetAutoSave(autoSaveFreq);
        myMLTree->book(myOutputTrees[treeName].get());
      }
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

  //HACKS. Move to configuration
  double margin = 5; //[mm] margin from the drift cage limits to define fiducial volume
  double maxAbsZ = aEventSource->getGeometry()->GetDriftCageZmax() - margin;

  // MC: two prong (e.g. alpha + carbon) or three prong (e.g. three alpha) events;
  // data: all reconstructed tracks (one segment: e.g. alpha only, short tracks, dots)
  const auto nSegments = mySimEventPtr->getSegments().size();
  if(mcEventSource && nSegments!=2 && nSegments!=3) return;
  if(!mcEventSource && nSegments<1) return;
  //Fiducial volume cut: ends of all segments inside the fiducial volume
  if(mcEventSource){
      for(const auto &aSegment: mySimEventPtr->getSegments()){
        bool isFiducial = std::abs(aSegment.getEnd().X())< 140 &&
                          std::abs(aSegment.getEnd().Y())< 90 &&
                          std::abs(aSegment.getEnd().Z())< maxAbsZ;
        if(!isFiducial) return;
      }
  }
  /////////////////////////

  if(myMLTree){
    myMLTree->set(*aEventSource->getCurrentEvent(), *myRecoEventPtr,
                  mcEventSource ? mySimEventPtr.get() : nullptr);
  }


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
bool FileOutput::findRecoEvent(const eventraw::EventInfo & aKey,
                               eventraw::EventInfo & aInfo, Track3D & aRecoEvent){

  auto it = myOutputTrees.find("TPCData");
  if(it==myOutputTrees.end() || !it->second) return false;
  TTree *aTree = it->second.get();
  TBranch *infoBranch = aTree->GetBranch("EventInfo");
  TBranch *recoBranch = aTree->GetBranch("RecoEvent");
  if(!infoBranch || !recoBranch) return false;

  // the branches read into the objects used for writing: every update() sets
  // EventInfo and RecoEvent again before the next Fill()
  bool found = false;
  for(Long64_t iEntry=aTree->GetEntries()-1;iEntry>=0 && !found;--iEntry){
    if(infoBranch->GetEntry(iEntry)<=0) continue;
    if(myEventInfoPtr->GetRunId()!=aKey.GetRunId() ||
       myEventInfoPtr->GetEventId()!=aKey.GetEventId()) continue;
    if(recoBranch->GetEntry(iEntry)<=0) continue;
    aInfo = *myEventInfoPtr;
    aRecoEvent = *myRecoEventPtr;
    found = true;
  }
  return found;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void FileOutput::update(const eventraw::EventInfo & aEventInfoPtr,
                        const Track3D & aRecoEventPtr){

   *myEventInfoPtr = aEventInfoPtr;
   *myRecoEventPtr = aRecoEventPtr;

    for(auto aTree: myOutputTrees) {
    if(aTree.first=="MLData") continue; // needs the charge map: filled only by update(aEventSource)
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
void FileOutput::update(std::shared_ptr<EventSourceBase> aEventSource,
                        const eventraw::EventInfo & aEventInfo,
                        const Track3D & aRecoEvent){

  *myPEventPtr = *aEventSource->getCurrentPEvent();
  *myEventInfoPtr = aEventInfo;
  *myRecoEventPtr = aRecoEvent;
  auto mcEventSource = std::dynamic_pointer_cast<EventSourceMC>(aEventSource);
  *mySimEventPtr = mcEventSource ? mcEventSource->getGeneratedTrack() : Track3D();

  for(auto aTree: myOutputTrees) {
    if(aTree.first=="MLData") continue; // ML tree is filled only by update(aEventSource)
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
