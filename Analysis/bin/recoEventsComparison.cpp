#include <cstdlib>
#include <iostream>
#include <vector>

#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TLatex.h"
#include "TString.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include "GeometryTPC.h"
#include "Track3D.h"
#include "EventInfo.h"

#include "colorText.h"

int compareRecoEvents(const  std::string & geometryFileName, 
		      const  std::string & referenceDataFileName,
		      const  std::string & testDataFileName);
/////////////////////////////////////
/////////////////////////////////////
boost::program_options::variables_map parseCmdLineArgs(int argc, char **argv){

  boost::program_options::options_description cmdLineOptDesc("Allowed options");
  cmdLineOptDesc.add_options()
    ("help", "produce help message")
    ("geometryFile",  boost::program_options::value<std::string>(), "string - path to the geometry file")
    ("referenceDataFile",  boost::program_options::value<std::string>(), "string - path to reference data file")
    ("testDataFile",  boost::program_options::value<std::string>(), "string - path to test data file");
  
  boost::program_options::variables_map varMap;        
  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cmdLineOptDesc), varMap);
  boost::program_options::notify(varMap); 

  if (varMap.count("help")) {
    std::cout<<cmdLineOptDesc<<std::endl;
    exit(1);
  }
  return varMap;
}
/////////////////////////////////////
/////////////////////////////////////
int main(int argc, char **argv){

  std::string geometryFileName, referenceDataFileName, testDataFileName;
  boost::program_options::variables_map varMap = parseCmdLineArgs(argc, argv);
  boost::property_tree::ptree tree;
  if(argc<4){
    char text[] = "--help";
    char *argvTmp[] = {text, text};
    parseCmdLineArgs(2,argvTmp);
    return 1;
  }
  if (varMap.count("geometryFile")) {
    geometryFileName = varMap["geometryFile"].as<std::string>();
  }
  if (varMap.count("referenceDataFile")) {
    referenceDataFileName = varMap["referenceDataFile"].as<std::string>();
  }
   if (varMap.count("testDataFile")) {
    testDataFileName = varMap["testDataFile"].as<std::string>();
  }
  if(referenceDataFileName.size() &&
     testDataFileName.size() && geometryFileName.size()) {
    compareRecoEvents(geometryFileName, referenceDataFileName, testDataFileName);
  }
  else{
    std::cout<<KRED<<"Configuration not complete: "<<RST
	     <<" geometryFile: "<<geometryFileName<<"\n"
	     <<" referenceDataFile: "<<referenceDataFileName<<"\n"
	     <<" testDataFile: "<<testDataFileName<<"\n"
	     <<std::endl;
  }
  return 0;
}
/////////////////////////////
////////////////////////////
std::shared_ptr<GeometryTPC> loadGeometry(const std::string fileName){
  return std::make_shared<GeometryTPC>(fileName.c_str(), false);
}
////////////////////////////
////////////////////////////
int setBranchAdresses(TTree *&aTree, eventraw::EventInfo *&aEventInfo, Track3D *&aTrack){
  
 TBranch *aBranch  = aTree->GetBranch("RecoEvent");
 if(!aBranch) {
   std::cerr<<KRED<<"ERROR: "<<RST
	    <<"Cannot find 'RecoEvent' branch!"<<std::endl;
   return -1;
 }
 aBranch->SetAddress(&aTrack);

 std::cout<<"&aRefTrack: "<<&aTrack<<std::endl;
  
 aBranch  = aTree->GetBranch("EventInfo");
 if(!aBranch) {
   std::cerr<<KRED<<"ERROR: "<<RST
	    <<"Cannot find 'EventInfo' branch!"<<std::endl;
   return -1;
 }
 aBranch->SetAddress(&aEventInfo);

 aTree->BuildIndex("eventId","20210622120156%(10000000000)");

 return 0;
}
////////////////////////////
////////////////////////////
/*
std::vector<int> makeEventsList(TTree *&aTree){

  std::map<int> eventsList;
  
  unsigned int nEntries = aTree->GetEntries();
  for(unsigned int iEntry=0;iEntry<nEntries;++iEntry){
    aTree->GetEntry(iEntry);
  }
}
*/
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
  
  //Comp_analysis myAnalysis(aGeometry, beamEnergy, beamDir); // need TPC geometry for proper histogram ranges
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

  std::cout<<"&aRefTrack: "<<&aRefTrack<<std::endl;
  int status = setBranchAdresses(aRefTree, aRefEventInfo, aRefTrack);
  if(status<0) return 1;

  std::cout<<"&aRefTrack: "<<&aTestTrack<<std::endl;
  status = setBranchAdresses(aTestTree, aTestEventInfo, aTestTrack);
  if(status<0) return 1;

  aRefTree->AddFriend(aTestTree,"TestEvents");
  /*
  int iEntry = 0;
  aRefTree->GetEntry(iEntry);
  std::cout<<*aRefEventInfo<<std::endl;
  */
  int bytesRead = aRefTree->GetEntryWithIndex(8, 20210622120156%(10000000000));
  std::cout<<"bytesRead: "<<bytesRead<<std::endl;

  std::cout<<*aRefEventInfo<<std::endl;
  std::cout<<*aTestEventInfo<<std::endl;

  return 0;
}
/////////////////////////////
////////////////////////////

