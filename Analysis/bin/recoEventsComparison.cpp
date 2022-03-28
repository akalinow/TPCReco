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
#include "HIGGS_analysis.h"

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
//makeEventsList(){
//}
////////////////////////////
////////////////////////////
int compareRecoEvents(const  std::string & geometryFileName,
		      const  std::string & referenceDataFileName,
		      const  std::string & testDataFileName){


  TFile *aReferenceFile = new TFile(referenceDataFileName.c_str());
  if(!aReferenceFile || !aReferenceFile->IsOpen()){
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
  TTree *aReferenceTree = (TTree*)aReferenceFile->Get("TPCRecoData");
  if(!aReferenceTree) {
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
  		    
  return 0;
}
/////////////////////////////
////////////////////////////

