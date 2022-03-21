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

int analyzeRecoEvents(const  std::string & geometryFileName,
		      const  std::string & dataFileName,
		      const  float & beamEnergy,
		      const  TVector3 & beamDir);
/////////////////////////////////////
/////////////////////////////////////
boost::program_options::variables_map parseCmdLineArgs(int argc, char **argv){

  boost::program_options::options_description cmdLineOptDesc("Allowed options");
  cmdLineOptDesc.add_options()
    ("help", "produce help message")
    ("geometryFile",  boost::program_options::value<std::string>(), "string - path to the geometry file")
    ("dataFile",  boost::program_options::value<std::string>(), "string - path to data file")
    ("beamEnergy", boost::program_options::value<float>(), "float - LAB gamma beam energy [keV]")
    ("beamDir", boost::program_options::value<std::string>(), "string - LAB gamma beam direction [\"x\" xor \"-x\"]");
  
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

  float beamEnergy=0.0; // keV
  TVector3 beamDir(0,0,0); // dimensionless, in detector LAB coordinates
  std::string geometryFileName, dataFileName;
  boost::program_options::variables_map varMap = parseCmdLineArgs(argc, argv);
  boost::property_tree::ptree tree;
  if(argc<5){
    char text[] = "--help";
    char *argvTmp[] = {text, text};
    parseCmdLineArgs(2,argvTmp);
    return 1;
  }
  if (varMap.count("geometryFile")) {
    geometryFileName = varMap["geometryFile"].as<std::string>();
  }
  if (varMap.count("dataFile")) {
    dataFileName = varMap["dataFile"].as<std::string>();
  }
  if (varMap.count("beamEnergy")) {
    beamEnergy = varMap["beamEnergy"].as<float>();
  }
  if (varMap.count("beamDir")) {
    std::string str = varMap["beamDir"].as<std::string>();
    boost::to_upper(str);
    if(str=="X")       beamDir=TVector3(1,0,0);
    else if(str=="-X") beamDir=TVector3(-1,0,0);
  }

  if(dataFileName.size() && geometryFileName.size() && beamEnergy>0 && beamDir.Mag2()>0) {
    analyzeRecoEvents(geometryFileName, dataFileName, beamEnergy, beamDir);
  }
  else{
    std::cout<<KRED<<"Configuration not complete: "<<RST
	     <<" geometryFile: "<<geometryFileName<<"\n"
	     <<" dataFile: "<<dataFileName<<"\n"
	     <<" beamEnergy: "<<beamEnergy<<" keV\n"
	     <<" beamDir: ["<<beamDir.X()<<", "<<beamDir.Y()<<", "<<beamDir.Z()<<"]"
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
int analyzeRecoEvents(const  std::string & geometryFileName,
		      const  std::string & dataFileName,
		      const  float & beamEnergy,
		      const  TVector3 & beamDir) {

  TFile *aFile = new TFile(dataFileName.c_str());
  if(!aFile || !aFile->IsOpen()){
    std::cout<<KRED<<"Input file: "<<RST<<dataFileName
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
  if(beamEnergy<=0) {
    std::cout<<KRED<<"Wrong beam energy: "<<RST<<beamEnergy<<" keV"
	     <<std::endl;
    return -1;
  }
  if(beamDir.Mag2()==0) {
    std::cout<<KRED<<"Wrong beam direction vector: "<<RST<<"["<<beamDir.X()<<", "<<beamDir.Y()<<", "<<beamDir.Z()<<"]"
	     <<std::endl;
    return -1;
  }
  
  HIGGS_analysis myAnalysis(aGeometry, beamEnergy, beamDir); // need TPC geometry for proper histogram ranges
  TTree *aTree = (TTree*)aFile->Get("TPCRecoData");
  if(!aTree) {
    std::cerr<<"ERROR: Cannot find 'TPCRecoData' tree!"<<std::endl;
    return -1;
  }
  Track3D *aTrack = new Track3D();
  TBranch *aBranch  = aTree->GetBranch("RecoEvent");
  if(!aBranch) {
    std::cerr<<"ERROR: Cannot find 'RecoEvent' branch!"<<std::endl;
    return -1;
  }
  aBranch->SetAddress(&aTrack);
  
  unsigned int nEntries = aTree->GetEntries();
  for(unsigned int iEntry=0;iEntry<nEntries;++iEntry){

    aBranch->GetEntry(iEntry);
    for (auto & aSegment: aTrack->getSegments())  aSegment.setGeometry(aGeometry); // need TPC geometry for track projections
    myAnalysis.fillHistos(aTrack);
  }			      

  return 0;
}
/////////////////////////////
////////////////////////////

