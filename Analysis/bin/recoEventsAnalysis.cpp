#include <TCanvas.h>
#include <TFile.h>
#include <TLatex.h>
#include <TString.h>
#include <TTree.h>
#include <TTreeIndex.h>
#include <boost/program_options.hpp>
#include <cstdlib>
#include <iostream>
#include <vector>

#include <TFile.h>
#include <TTree.h>
#include <TCanvas.h>
#include <TLatex.h>
#include <TString.h>
#include <TTreeIndex.h>

#include <boost/program_options.hpp>

#include "TPCReco/Cuts.h"
#include "TPCReco/CoordinateConverter.h"
#include "TPCReco/RequirementsCollection.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/Track3D.h"
#include "TPCReco/HIGGS_analysis.h"
#include "TPCReco/HIGS_trees_analysis.h"
#include "TPCReco/colorText.h"

int analyzeRecoEvents(const  std::string & geometryFileName, 
		      const  std::string & dataFileName,
		      const  float & beamEnergy, // [MeV]
		      const  TVector3 & beamDir, // unit vector in LAB detector frame
		      const  double & beam_offset, // offset Y_DET [mm] and
		      const  double & beam_slope,  // slope (dy/dx)_DET of the actual beam axis: Y_DET = slope * X_DET + offset
		      const  double & beam_diameter, // beam spot diameter [mm] used by event quality cuts
		      const  double & pressure, // [mbar]
		      const  double & temperature, // [K]
		      const  bool & makeTreeFlag);

enum class BeamDirection{
  X,
  MINUS_X
};

std::istream& operator>>(std::istream& in, BeamDirection& direction){
  std::string token;
  in >> token;
  if (token == "x" || token == "X"){
     direction = BeamDirection::X;
  }
  else if (token == "-x" || token == "-X"){
    direction = BeamDirection::MINUS_X;
  } 
  else
    in.setstate(std::ios_base::failbit);
  return in;
  }

boost::program_options::variables_map parseCmdLineArgs(int argc, char **argv){

  //  bool optionTree=true;
  boost::program_options::options_description cmdLineOptDesc("Allowed options");
  cmdLineOptDesc.add_options()
    ("help", "produce help message")
    ("geometryFile",  boost::program_options::value<std::string>()->required(), "string - path to the geometry file")
    ("dataFile",  boost::program_options::value<std::string>()->required(), "string - path to data file")
    ("beamEnergy", boost::program_options::value<float>()->required(), "float - LAB gamma beam energy [MeV]")
    ("beamDir", boost::program_options::value<BeamDirection>()->required(), "string - LAB gamma beam direction [\"x\" xor \"-x\"]")
    ("beamOffset", boost::program_options::value<float>()->default_value(0.0), "float - offset in Y_DET [mm] of actual beam line: Y_DET=slope*X_DET+offset")
    ("beamSlope", boost::program_options::value<float>()->default_value(0.0), "float - slope (dY/dX)_DET of actual beam line: Y_DET=slope*X_DET+offset")
    ("beamDiameter", boost::program_options::value<float>()->default_value(12.0), "float - beam spot diameter [mm] used by event quality cuts")
    ("pressure", boost::program_options::value<float>()->required(), "float - CO2 pressure [mbar]")
    ("temperature", boost::program_options::value<float>()->default_value(273.15+20), "(option) float - CO2 temperature [K], default=293.15K (20 C)")
    ("noTree", boost::program_options::bool_switch()->default_value(false), "(option) bool - flag to skip creating additional TTree for 1,2,3-prongs, default=false ");
  
  boost::program_options::variables_map varMap;  

  try {     
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cmdLineOptDesc), varMap);
    if (varMap.count("help")) {
      std::cout << "recoEventAnalysis" << "\n\n";
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
  auto dataFileName = varMap["dataFile"].as<std::string>();
  auto beamEnergy = varMap["beamEnergy"].as<float>();
  auto pressure = varMap["pressure"].as<float>();
  auto temperature = varMap["temperature"].as<float>();
  auto makeTreeFlag = !varMap["noTree"].as<bool>();
  auto beamOffset = varMap["beamOffset"].as<float>();
  auto beamSlope = varMap["beamSlope"].as<float>();
  auto beamDiameter = varMap["beamDiameter"].as<float>();
  TVector3 beamDir;
  switch(varMap["beamDir"].as<BeamDirection>()){
    case BeamDirection::X : 
      beamDir = TVector3(1,0,0);
      break;
    case BeamDirection::MINUS_X : 
      beamDir = TVector3(-1,0,0);
      break;
    default:
      return 1;
  }
  analyzeRecoEvents(geometryFileName, dataFileName, beamEnergy, beamDir, beamOffset, beamSlope, beamDiameter, pressure, temperature, makeTreeFlag);
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
		      const  float & beamEnergy, // [MeV]
		      const  TVector3 & beamDir, // unit vector in LAB detector frame
		      const  double & beam_offset, // offset Y_DET [mm] and
		      const  double & beam_slope,  // slope (dy/dx)_DET of the actual beam axis: Y_DET = slope * X_DET + offset
		      const  double & beam_diameter, // beam spot diameter [mm] used by event quality cuts
		      const  double & pressure, // [mbar]
		      const  double & temperature, // [K]
		      const  bool & makeTreeFlag){

  std::cout << __FUNCTION__ << ": Input parameters:" << std::endl
	    << "* geometry file: " << geometryFileName << std::endl
	    << "* input ROOT file: " << dataFileName << std::endl
	    << "* LAB gamma beam energy: " << beamEnergy << " MeV" << std::endl
	    << "* LAB gamma beam direction in DET coordinates: [x=" << beamDir.X() << ", y=" << beamDir.Y() << ", z=" << beamDir.Z() << "]" << std::endl
	    << "* CO2 pressure (for ion ranges):  " << pressure <<" mbar" <<std::endl
	    << "* CO2 temperature (for ion ranges): " << temperature << " K, "<< (temperature-273.15) << " C" << std::endl;

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
    std::cout<<KRED<<"Wrong beam energy: "<<RST<<beamEnergy<<" MeV"
	     <<std::endl;
    return -1;
  }
  if(beamDir.Mag2()==0) {
    std::cout<<KRED<<"Wrong beam direction vector: "<<RST<<"["<<beamDir.X()<<", "<<beamDir.Y()<<", "<<beamDir.Z()<<"]"
	     <<std::endl;
    return -1;
  }
  if(pressure<50.0 || pressure>1100.0) {
    std::cout<<KRED<<"Wrong CO2 pressure: "<<RST<<pressure<<" mbar"
	     <<std::endl;
    return -1;
  }
  if(temperature<273.15 || temperature>273.15+50) {
    std::cout<<KRED<<"Wrong CO2 temperature: "<<RST<<temperature<<" K"
	     <<std::endl;
    return -1;
  }

  // double beam_offset = -1.3;  // [mm]    Beam central line on horizontal UVW / XY_DET plane is parameterized as:
  // double beam_slope = 3.0E-3; // [rad]   Y_DET(X_DET) = beam_slope * X_DET + beam_offset
  // double beam_diameter = 12.; // [mm]
  auto cuts = RequirementsCollection<std::function<bool(Track3D *)>>{};
  cuts.push_back(tpcreco::cuts::Cut1{});
  cuts.push_back(tpcreco::cuts::Cut2{beam_offset, beam_slope, beam_diameter});
  cuts.push_back(tpcreco::cuts::Cut3{aGeometry.get(), 5});
  cuts.push_back(tpcreco::cuts::Cut4{aGeometry.get(), 25, 5});
  cuts.push_back(tpcreco::cuts::Cut5{aGeometry.get(), beam_diameter});

  // 1. Nominal transformation between DET-->BEAM coordinates for the HIGS experiment
  //    provided that beam direction is anti-paralell to X_DET (HIGS case):
  //    X_DET -> -Z_BEAM
  //    Y_DET ->  X_BEAM
  //    Z_DET -> -Y_BEAM
  //    This corresponds to the "nominal" rotation matrix DET-->BEAM with Euler angles: PHI=-Pi/2, THETA=Pi/2, PSI=0.
  // 2. Correction for tilt in horizontal XY_DET (or XZ_BEAM) plane of the central beam axis parameterized as:
  //    Y_DET = beam_slope * X_DET + beam_offset
  //    is equivalent to:
  //    X_BEAM = beam_slope * (-Z_BEAM) + beam_offset.
  //    This corresponds to the rotation about Y_BEAM axis by ATAN(beam_slope) angle,
  //    which is equivalent to the "correction" rotation matrix with Euler angles: PHI=Pi/2, THETA=ATAN(-beam_slope), PSI=-Pi/2.
  // 3. Correction for offset in horizontal XY_DET (or XZ_BEAM) plane of the central beam axis is done in such way
  //    that vertex positioned at X_DET=0 (detector center) will be positioned at Z_BEAM=0.
  //    This corresponds to translation of the origin of the BEAM coordinate system by vector [DX, DY, DZ]_DET, where:
  //    DX_DET = - beam_offset * sin(atan(beam_slope))
  //    DY_DET =   beam_offset * cos(atan(beam_slope))
  //    DZ_DET = 0.
  //
  auto coordinateConverter = CoordinateConverter({-M_PI/2, M_PI/2, 0},
						 {M_PI/2, atan(-beam_slope), -M_PI/2},
						 {-beam_offset*sin(atan(beam_slope)), beam_offset*cos(atan(beam_slope)), 0});
  auto ionRangeCalculator = IonRangeCalculator(gas_mixture_type::CO2, pressure, temperature);

  auto myAnalysis = HIGGS_analysis(aGeometry, beamEnergy, beamDir, ionRangeCalculator, coordinateConverter);
  auto myTreesAnalysis= std::unique_ptr<HIGS_trees_analysis>(nullptr);
  if(makeTreeFlag) myTreesAnalysis=std::make_unique<HIGS_trees_analysis>(aGeometry, beamEnergy, beamDir, pressure, temperature);

  TTree *aTree = (TTree*)aFile->Get("TPCRecoData");
  if(!aTree) {
    std::cerr<<KRED<<"ERROR: Cannot find 'TPCRecoData' tree!"<<RST<<std::endl;
    return -1;
  }
  
  TBranch *aBranch  = aTree->GetBranch("RecoEvent");
  if(!aBranch) {
    std::cerr<<KRED<<"ERROR: Cannot find 'RecoEvent' branch!"<<RST<<std::endl;
    return -1;
  }
  auto *aTrack = new Track3D();
  aBranch->SetAddress(&aTrack);
  
  TBranch *aBranchInfo = aTree->GetBranch("EventInfo");
  if(!aBranchInfo) {
   std::cerr<<KRED<<"WARNING: "
	    <<"Cannot find 'EventInfo' branch!"<<RST<<std::endl;
    return -1;
  }
  auto *aEventInfo = new eventraw::EventInfo();
  aBranchInfo->SetAddress(&aEventInfo);
  
  std::cout << __FUNCTION__ << ": Starting to loop " << aTree->GetEntries() << " events with sorting by {runId, eventId}" << std::endl;
  aTree->BuildIndex("runId", "eventId");
  auto index =static_cast<TTreeIndex*>(aTree->GetTreeIndex())->GetIndex();

  for(unsigned int iEntry=0;iEntry<aTree->GetEntries();++iEntry){
    aTree->GetEntry(index[iEntry]);

    for (auto & aSegment: aTrack->getSegments())  aSegment.setGeometry(aGeometry); // need TPC geometry for track projections
    
    if(!cuts(aTrack)){
      continue;
    }

    myAnalysis.fillHistos(aTrack);
    if(makeTreeFlag) myTreesAnalysis->fillTrees(aTrack, aEventInfo);
  }

  return 0;
}
/////////////////////////////
////////////////////////////

