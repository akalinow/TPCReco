#include <cstdlib>
#include <iostream>
#include <vector>
#include <algorithm>

#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TLatex.h"
#include "TString.h"
#include "TTreeIndex.h"


#include <boost/program_options.hpp>

#include "GeometryTPC.h"
#include "Track3D.h"
#include "EventInfo.h"
#include "Comp_analysis.h"
#include "ConfigManager.h"

#include "colorText.h"

int compareRecoEvents(const  std::string & geometryFileName, 
		      const  std::string & referenceDataFileName,
		      const  std::string & testDataFileName);
/////////////////////////////////////
/////////////////////////////////////
int main(int argc, char **argv){

  ConfigManager cm;
  boost::property_tree::ptree tree = cm.getConfig(argc,argv);
  auto geometryFileName = tree.get("geometryFile","");
  auto referenceDataFileName = tree.get("referenceDataFile","");
  auto testDataFileName = tree.get("testDataFile","");
  compareRecoEvents(geometryFileName, referenceDataFileName, testDataFileName);
  return 0;
}
/////////////////////////////
////////////////////////////
std::shared_ptr<GeometryTPC> loadGeometry(const std::string fileName){
  return std::make_shared<GeometryTPC>(fileName.c_str(), false);
}
////////////////////////////
////////////////////////////
std::vector<Long64_t> setBranchAdressesAndIndex(TTree *&aTree, eventraw::EventInfo *&aEventInfo, Track3D *&aTrack){
  
 TBranch *aBranch  = aTree->GetBranch("RecoEvent");
 if(!aBranch) {
   std::cerr<<KRED<<"ERROR: "<<RST
	    <<"Cannot find 'RecoEvent' branch!"<<std::endl;
   return std::vector<Long64_t>();
 }
 aBranch->SetAddress(&aTrack);
 
 aBranch  = aTree->GetBranch("EventInfo");
 if(!aBranch) {
   std::cerr<<KRED<<"ERROR: "<<RST
	    <<"Cannot find 'EventInfo' branch!"<<std::endl;
   return std::vector<Long64_t>();
 }
 aBranch->SetAddress(&aEventInfo);

 aTree->BuildIndex("eventId");
 TTreeIndex *aIndex = (TTreeIndex*)aTree->GetTreeIndex();
 Long64_t *indexValues = aIndex->GetIndexValues();   
 std::vector<Long64_t> indexValuesVec(indexValues, indexValues+aTree->GetEntries());
 return indexValuesVec;
}
////////////////////////////
////////////////////////////
int compareRecoEvents(const  std::string & geometryFileName,
		      const  std::string & referenceDataFileName,
		      const  std::string & testDataFileName){

  TFile *aRefFile = new TFile(referenceDataFileName.c_str());
  if(!aRefFile || !aRefFile->IsOpen()){
    std::cout<<KRED<<"Input file: "<<RST<<referenceDataFileName
	     <<KRED<<" not found!"<<RST
	     <<std::endl;
    return -1;
  }
  TFile *aTestFile = new TFile(testDataFileName.c_str());
  if(!aTestFile || !aTestFile->IsOpen()){
    std::cout<<KRED<<"Input file: "<<RST<<testDataFileName
	     <<KRED<<" not found!"<<RST
	     <<std::endl;
    return -1;
  }
  std::shared_ptr<GeometryTPC> aGeometry = loadGeometry(geometryFileName);
  if(!aGeometry){
    std::cout<<KRED<<"Geometry file: "<<RST<<geometryFileName
	     <<KRED<<" not found!"<<RST
	     <<std::endl;
    return -1;
  }
  
  TTree *aRefTree = (TTree*)aRefFile->Get("TPCRecoData");
  if(!aRefTree) {
    std::cerr<<KRED<<"ERROR: "<<RST
	     <<" Cannot find 'TPCRecoData' tree from reference file!"<<std::endl;
    return -1;
  }

  TTree *aTestTree = (TTree*)aTestFile->Get("TPCRecoData");
  if(!aTestTree) {
    std::cerr<<KRED<<"ERROR: "<<RST
	     <<" Cannot find 'TPCRecoData' tree from test file!"<<std::endl;
    return -1;
  }

  Track3D *aRefTrack = new Track3D();
  Track3D *aTestTrack = new Track3D();

  eventraw::EventInfo *aRefEventInfo = new eventraw::EventInfo();
  eventraw::EventInfo *aTestEventInfo = new eventraw::EventInfo();

  std::vector<Long64_t> refTreeIndex = setBranchAdressesAndIndex(aRefTree, aRefEventInfo, aRefTrack);
  if(!refTreeIndex.size()) return 1;

  std::vector<Long64_t> testTreeIndex = setBranchAdressesAndIndex(aTestTree, aTestEventInfo, aTestTrack);
  if(!testTreeIndex.size()) return 1;

  aRefTree->GetEntry(0);
  aTestTree->GetEntry(0);

  if(aRefEventInfo->GetRunId()!=aTestEventInfo->GetRunId()){
    std::cout<<KRED<<"Data files have uncompatible run ids: \n"<<RST
	     <<"  reference file: "<<aRefEventInfo->GetRunId()<<std::endl
	     <<"  test file:      "<<aTestEventInfo->GetRunId()
	     <<std::endl;
    return -1;
  }

  std::sort(refTreeIndex.begin(), refTreeIndex.end());
  std::sort(testTreeIndex.begin(), testTreeIndex.end());
  std::vector<Long64_t> mergedTreeIndex = refTreeIndex;
  mergedTreeIndex.insert(mergedTreeIndex.end(), testTreeIndex.begin(), testTreeIndex.end());
  std::sort(mergedTreeIndex.begin(), mergedTreeIndex.end());
  mergedTreeIndex.erase(std::unique(mergedTreeIndex.begin(), mergedTreeIndex.end()), mergedTreeIndex.end());

  TTree *theTree = aRefTree;
  if(aTestTree->GetEntries()>aRefTree->GetEntries()){
    theTree = aTestTree;
    theTree->AddFriend(aRefTree,"TestEvents");
  }
  else{    
    theTree->AddFriend(aTestTree,"TestEvents");
  }

  Comp_analysis myAnalysis(aGeometry); 
  for(auto entryIndex: mergedTreeIndex){

    aRefEventInfo->reset();
    aTestEventInfo->reset();
    
    theTree->GetEntryWithIndex(entryIndex);    
    for (auto & aSegment: aRefTrack->getSegments())  aSegment.setGeometry(aGeometry);
    for (auto & aSegment: aTestTrack->getSegments())  aSegment.setGeometry(aGeometry);

    myAnalysis.fillHistos(aRefTrack, aRefEventInfo,
			  aTestTrack, aTestEventInfo);
  }
  
  return 0;
}
/////////////////////////////
////////////////////////////

