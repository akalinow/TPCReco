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

#include "colorText.h"

int compareRecoEvents(const  std::string & geometryFileName, 
		      const  std::string & referenceDataFileName,
		      const  std::string & testDataFileName,
		      const  double & pressure, // [mbar]
		      const  double & temperature // [K]
);
/////////////////////////////////////
/////////////////////////////////////
boost::program_options::variables_map parseCmdLineArgs(int argc, char **argv){

  boost::program_options::options_description cmdLineOptDesc("Allowed options");
  cmdLineOptDesc.add_options()
    ("help", "produce help message")
    ("geometryFile",  boost::program_options::value<std::string>()->required(), "string - path to the geometry file")
    ("referenceDataFile",  boost::program_options::value<std::string>()->required(), "string - path to REFERENCE Reco data file")
    ("testDataFile",  boost::program_options::value<std::string>()->required(), "string - path to TEST Reco data file")
    ("pressure", boost::program_options::value<float>()->required(), "float - CO2 pressure [mbar]")
    ("temperature", boost::program_options::value<float>()->default_value(273.15+20), "(option) float - CO2 temperature [K], default=293.15K (20 C)");
  
  boost::program_options::variables_map varMap;        
  try {     
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cmdLineOptDesc), varMap);
    if (varMap.count("help")) {
      std::cout << "recoEventsComparison" << "\n\n";
      std::cout << cmdLineOptDesc << std::endl;
      exit(1);
    }
    boost::program_options::notify(varMap);
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    std::cout << cmdLineOptDesc << std::endl;
    exit(1);
  }

  return varMap;
}
/////////////////////////////////////
/////////////////////////////////////
int main(int argc, char **argv){

  auto varMap = parseCmdLineArgs(argc, argv);
  auto geometryFileName = varMap["geometryFile"].as<std::string>();
  auto referenceDataFileName = varMap["referenceDataFile"].as<std::string>();
  auto testDataFileName = varMap["testDataFile"].as<std::string>();
  auto pressure = varMap["pressure"].as<float>();
  auto temperature = varMap["temperature"].as<float>();
  compareRecoEvents(geometryFileName, referenceDataFileName, testDataFileName, pressure, temperature);
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
		      const  std::string & testDataFileName,
		      const  double & pressure, // [mbar]
		      const  double & temperature){ // [K]

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

  Comp_analysis myAnalysis(aGeometry, pressure, temperature);
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

