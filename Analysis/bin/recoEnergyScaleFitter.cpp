#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <vector>
#include <set>
#include <map>
#include <tuple>
#include <algorithm>

#include <TFile.h>
#include <TChain.h>
#include <TBranch.h>
#include <TTreeIndex.h>
#include <TVector3.h>
#include <Math/Functor.h>
#include <Fit/Fitter.h>

#include "TPCReco/colorText.h"
#include "TPCReco/CommonDefinitions.h"
#include "TPCReco/EventInfo.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/CoordinateConverter.h"
#include "TPCReco/IonRangeCalculator.h"
#include "TPCReco/Track3D.h"
#include "TPCReco/Cuts.h"
#include "TPCReco/RequirementsCollection.h"
#include "TPCReco/EnergyScale_analysis.h"

////// DEBUG
#define USE_REAL_DATA__9_56_MEV  false
#define TOY_MC_TEST              false
#define TOY_MC_TEST_ALPHA_ONLY   true
////// DEBUG

enum class BeamDirection{
  PLUS_X,
  MINUS_X
};

CoordinateConverter getCoordinateConverter(const BeamDirection beamDir, const double beam_offset, const double beam_slope);
TVector3 getBeamDir_DET(const BeamDirection beamDir, const double beam_offset, const double beam_slope);
std::shared_ptr<GeometryTPC> loadGeometry(const std::string fileName);
bool assignMissingPIDs(const reaction_type aReaction,
		       EnergyScale_analysis::TrackCollection &list); // track collection to be edited
size_t selectRecoEvents(const std::vector<std::string> &recoFileList,
			const std::shared_ptr<GeometryTPC> &geometryPtr,
			const CoordinateConverter &coordinateConverter,
			RequirementsCollection<std::function<bool(Track3D *)>> &cuts,
			std::vector<std::vector<EnergyScale_analysis::TrackCollection>> &collection); // event collection to be filled
double fitPeaks(const EnergyScale_analysis::FitOptionType &aOption,
		std::vector<EnergyScale_analysis::EventCollection> &aSelection);

/////////////////////////////////////
/////////////////////////////////////
std::istream& operator>>(std::istream& in, BeamDirection& direction){
  std::string token;
  in >> token;
  if (token == "x" || token == "X" || token == "+x" || token == "+X"){
     direction = BeamDirection::PLUS_X;
  }
  else if (token == "-x" || token == "-X"){
    direction = BeamDirection::MINUS_X;
  } 
  else
    in.setstate(std::ios_base::failbit);
  return in;
}

/////////////////////////////////////
/////////////////////////////////////
std::ostream& operator<<(std::ostream& out, BeamDirection const &direction){
  switch(direction) {
  case BeamDirection::PLUS_X:
    out << "parallel to X_DET axis, unit vector=[+1, 0, 0]";
    break;
  case BeamDirection::MINUS_X:
    out << "anti-parallel to X_DET axis, unit vector=[-1, 0, 0]";
    break;
  default:
    out << "value not supported in HIGS analysis";
  };
  return out;
}

/////////////////////////////////////
/////////////////////////////////////
boost::program_options::variables_map parseCmdLineArgs(int argc, char **argv){

  boost::program_options::options_description cmdLineOptDesc("Allowed options");
  cmdLineOptDesc.add_options()
    ("help", "produce help message")
    ("correctPIDs", boost::program_options::bool_switch(), "(override flag) - force to use best guess for missing PIDs according to assumed reaction hypothesis")
    ("nominalBoost", boost::program_options::bool_switch(), "(override flag) - force to use nominal beam energy for LAB<->CMS boost")
    ("leadingTrack", boost::program_options::bool_switch(), "(override flag) - force to use only leading particle information for calculating properties of 2-body decay in CMS")
    ("scaleOnly", boost::program_options::bool_switch(), "(override flag) - force to use only multiplicative corrections instead of linear scale and offset")
    ("type", boost::program_options::value<std::string>()->required(), "string - type of correction [\"length\" xor \"energy_cms\" xor \"energy_lab\" xor \"zenek\"]");
  boost::program_options::variables_map varMap;  

  try {     
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cmdLineOptDesc), varMap);
    if (varMap.count("help")) {
      std::cout << "recoEventScaleFitter" << "\n\n";
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
  boost::property_tree::ptree tree;

  // configuration from JSON is not supported yet
  // if(argc<2){
  //   std::cout << std::endl
  // 	      << "recoEnergyScaleFitter config.json [options]" << std::endl << std::endl;
  //   return 0;
  // }
  // else {
  //   std::cout<<"Using configFileName: " << argv[1] <<std::endl;
  //   boost::property_tree::read_json(argv[1], tree);
  // }

  // optional overrides of the JSON config file
  if (varMap.count("correctPIDs")) {
    tree.put("correctPIDs", varMap["correctPIDs"].as<bool>());
  }
  if (varMap.count("scaleOnly")) {
    tree.put("scaleOnly", varMap["scaleOnly"].as<bool>());
  }
  if (varMap.count("leadingTrack")) {
    tree.put("leadingTrack", varMap["leadingTrack"].as<bool>());
  }
  if(varMap.count("nominalBoost")) {
    tree.put("nominalBoost", varMap["nominalBoost"].as<bool>());
  }
  if(varMap.count("type")) {
    tree.put("type", varMap["type"].as<std::string>());
  }

  ///////////////////////
  //
  // hardcoded fit parameters - TO BE PRAMATERIZED FROM JSON
  //
  EnergyScale_analysis::FitOptionType myOptions{
    10,           // ROOT fitter TOLERANCE parameter - TO BE PARAMETERIZED FROM JSON
                  // NOTE: MIGRAD ends when EDM < 0.001 * TOLERANCE
    5e-9,         // ROOT fitter PRECISION parameter - TO BE PARAMETERIZED FROM JSON
                  // NOTE: For best 2-parameter fits based on the leading ALPHA track information only use these settings:
                  //       LENGTH      --> TOL=10, PREC=1e-6
                  //       ENERGY_CMS  --> TOL=10, PREC=5e-9
                  //       ENERGY_LAB  --> TOL=10, PREC=5e-9
    // DETECTOR RESOLUTION:
#if(TOY_MC_TEST)  // Monte Carlo data
    1e-3,         // perfect detector
#else             // Real data
    0.05,         // estimate of detector Qvalue_CMS resolution in [MeV] - TO BE PARAMETERIZED FROM JSON
#endif
    // LIST OF PARTICLES TO BE TUNED:
#if(TOY_MC_TEST)  // Monte Carlo data
#if(TOY_MC_TEST_ALPHA_ONLY)
    { { "ALPHA", {pid_type::ALPHA} } // tune ALPHA particles only
    },
#else
    { { "ALPHA", {pid_type::ALPHA} }, // tune both: ALPHA particles and all CARBON isotopes
      { "CARBON", {pid_type::CARBON_12, pid_type::CARBON_13, pid_type::CARBON_14} }
    },
#endif
#else             // Real data
      { { "ALPHA",  {pid_type::ALPHA} } // TO BE PARAMETERIZED FROM JSON
    },
#endif
    tree.get<bool>("scaleOnly"),            // use multiplicative corrections only?
    tree.get<bool>("leadingTrack"),         // use leading track information only?
    tree.get<bool>("nominalBoost"),         // use nominal gamma energy for LAB-CMS boost?
    tree.get<bool>("correctPIDs"),          // use best guess for missing PIDs according to assumed reaction hypothesis?
    enumDict::GetEnergyScaleType(boost::algorithm::to_upper_copy(tree.get<std::string>("type"))), // type of energy scale correction
    true // debug flag
  };
  
  ///////////////////////
  //
  // hardcoded list of RECO files for a given reaction channel - TO BE PARAMETERIZED
  //
  struct InputSourceData {
    const std::string description; // unique identifier of reaction channel / selection cuts
    const reaction_type reaction; // reaction type ID
    const int ntracks; // required number of tracks
    const double gammaEnergy_LAB; // [MeV] - nominal energy of the gamma beam in LAB reference frame
    const double peakEnergy_LAB; // [MeV] - peak position from theoretical prediction (e.g. resonance)
    const double peakSigma_LAB; // [MeV] - peak RMS from theoretical prediction (excluding detector resolution)
    const double excitedMassDiff; // [MeV/c^2] - excitation energy of the 2nd component in 2-body decay, e.g. C-12 -> ALPHA + Be-8(*)
    const double pressure; // [mbar] - pressure
    const double temperature; // [K] - temperature
    const std::string geometryFile; // TPC geometry/conditions config file
    const BeamDirection beamDir; // +X or -X
    const double beamOffset; // [mm]
    const double beamSlope;
    const std::vector<std::string> recoFiles; // list of RECO files
  };
  ///////////////////////
  //
  // hardcoded list of reaction channels with expected energy peaks, run conditions and input RECO files
  //
  // [1] TUNL compilation data for O-16 levels: https://nucldata.tunl.duke.edu/nucldata/HTML/A=16/16_13_1993.pdf
  // [2] W.R.Zimmermann et al., PRL 110 (2013) 152502
  //
  std::vector<InputSourceData> myRecoFiles={
#if(TOY_MC_TEST)
    ///////////////////////
    //
    // E_GAMMA = 8.66 MeV -- SIMULATION
    //
    // 2-prong / Oxygen-16 decay / no resonance: peak=8.66 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=8.66 MeV
    { "Eg=8.66 MeV O-16 (none)",
      reaction_type::C12_ALPHA, 2, 8.66, 8.657, 0.1498, 0.0,
      130, 273.15+20, "geometry_ELITPC_130mbar_1372Vdrift_25MHz.dat", BeamDirection::MINUS_X, -1.3, 3.0e-3, // exact tilt from MC GEN
      { "./Generated_Track3D__O16_8.66MeV.root" } },
    // 3-prong / Carbon-12 decay democratic / no resonance: peak=8.66 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=8.66 MeV
    { "Eg=8.66 MeV C-12 democratic (none)",
      reaction_type::THREE_ALPHA_DEMOCRATIC, 3, 8.66, 8.657, 0.1491, 0.0,
      130, 273.15+20, "geometry_ELITPC_130mbar_1372Vdrift_25MHz.dat", BeamDirection::MINUS_X, -1.3, 3.0e-3, // exact tilt from MC GEN
      { "./Generated_Track3D__C12demo_8.66MeV.root" } },
    ///////////////////////
    //
    // E_GAMMA = 9.56 MeV -- SIMULATION
    //
    // 2-prong / Oxygen-16 decay / no resonance: peak=9.56 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=9.56 MeV
    { "Eg=9.56 MeV O-16 (none)",
      reaction_type::C12_ALPHA, 2, 9.56, 9.555, 0.1498, 0.0,
      130, 273.15+20, "geometry_ELITPC_130mbar_1764Vdrift_25MHz.dat", BeamDirection::MINUS_X, -1.3, 3.0e-3, // exact tilt from MC GEN
      { "./Generated_Track3D__O16_9.56MeV.root" } },
    ///////////////////////
    //
    // E_GAMMA = 9.845 MeV -- SIMULATION
    //
    // 2-prong / Oxygen-16 decay / no resonance: peak=9.845 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=9.845 MeV
    { "Eg=9.845 MeV O-16 (none)",
      reaction_type::C12_ALPHA, 2, 9.845, 9.839, 0.1505, 0.0,
      130, 273.15+20, "geometry_ELITPC_130mbar_1764Vdrift_25MHz.dat", BeamDirection::MINUS_X, -1.3, 3.0e-3, // exact tilt from MC GEN
      { "./Generated_Track3D__O16_9.845MeV.root" } },
    ///////////////////////
    //
    // E_GAMMA = 12.3 MeV -- SIMULATION
    //
    // 2-prong / Oxygen-16 decay / no resonance: peak=12.3 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=12.3 MeV
    { "Eg=12.3 MeV O-16 (none)",
      reaction_type::C12_ALPHA, 2, 12.3, 12.29, 0.1493, 0.0,
      190, 273.15+20, "geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat", BeamDirection::MINUS_X, -1.3, 3.0e-3, // exact tilt from MC GEN
      { "./Generated_Track3D__O16_12.3MeV.root" } },
    ///////////////////////
    //
    // E_GAMMA = 13.1 MeV -- SIMULATION
    //
    // 2-prong / Oxygen-16 decay / no resonance: peak=13.1 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=13.1 MeV
    { "Eg=13.1 MeV O-16 (none)",
      reaction_type::C12_ALPHA, 2, 13.1, 13.09, 0.1484, 0.0,
      250, 273.15+20, "geometry_ELITPC_250mbar_2744Vdrift_12.5MHz.dat", BeamDirection::MINUS_X, -1.3, 3.0e-3, // exact tilt from MC GEN
      { "./Generated_Track3D__O16_13.1MeV.root" } }
#else
    // ///////////////////////
    // //
    // // E_GAMMA = 8.66 MeV
    // //
    // // 2-prong / Oxygen-18 decay / no resonance: peak=8.66 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=8.66 MeV
    // { "Eg=8.66 MeV O-18 (none)",
    //   reaction_type::C14_ALPHA, 2, 8.66, 8.66, 0.150, 0.0,
    //   130, 273.15+20, "geometry_ELITPC_130mbar_1372Vdrift_25MHz.dat", BeamDirection::MINUS_X, -1.36, 2.34e-3, // run=20220901163827
    //   { "./Reco_8.66MeV_merged.root" } },
    // // // 3-prong / Carbon-12 decay / no resonance: peak=8.66 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=8.66 MeV
    // // { "Eg=8.66 MeV C-12 (none)",
    // //   reaction_type::THREE_ALPHA_BE, 3, 8.66, 8.66, 0.150, 0.0,
    // //   130, 273.15+20, "geometry_ELITPC_130mbar_1372Vdrift_25MHz.dat", BeamDirection::MINUS_X, -1.36, 2.34e-3, // run=20220901163827 
    // //   { "Reco_8.66MeV_merged.root" } },
#if(USE_REAL_DATA__9_56_MEV)
    ///////////////////////
    //
    // E_GAMMA = 9.56 MeV
    //
    // 2-prong / Oxygen-16 decay / 1- resonance [1]: peak=9.585(11) MeV, gamma_width=0.420(20) MeV, multipolarity=E1 / Egamma=9.56 MeV
    { "Eg=9.56 MeV O-16 (1- 9.585 MeV)",
      reaction_type::C12_ALPHA, 2, 9.56, 9.585, 0.5*0.420, 0.0,
      130, 273.15+20, "geometry_ELITPC_130mbar_1764Vdrift_25MHz.dat", BeamDirection::MINUS_X, -0.92, 1.15e-3, // run=20220823091420
      { "/mnt/NAS_STORAGE_ANALYSIS/higs_2022/9_56MeV/Reco/clicked/Reco_20220823T091420_0000_ZJ.root" } },
    // // 2-prong / Oxygen-16 decay / 2+ resonance [1]: peak=9.84450(5) MeV, gamma_width=0.000625(100) MeV, multipolarity=E2 / Egamma=9.56 MeV
    // { "Eg=9.56 MeV O-16 (2+ 9.8445 MeV)",
    //   reaction_type::C12_ALPHA, 2, 9.56, 9.8445, 0.00625, 0.0,
    //   130, 273.15+20, "geometry_ELITPC_130mbar_1764Vdrift_25MHz.dat", BeamDirection::MINUS_X, -0.92, 1.15e-3, // run=20220823091420
    //   { "/mnt/NAS_STORAGE_ANALYSIS/higs_2022/9_56MeV/Reco/clicked/Reco_20220823T091420_0000_ZJ.root" } },
    // // 2-prong / Oxygen-18 decay / no resonance: peak=9.56 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=9.56 MeV
    // { "Eg=9.56 MeV O-18 (none)",
    //   reaction_type::C14_ALPHA, 2, 9.56, 9.56, 0.150, 0.0,
    //   130, 273.15+20, "geometry_ELITPC_130mbar_1764Vdrift_25MHz.dat", BeamDirection::MINUS_X, -0.92, 1.15e-3, // run=20220823091420
    //   { "/mnt/NAS_STORAGE_ANALYSIS/higs_2022/9_56MeV/Reco/clicked/Reco_20220823T091420_0000_ZJ.root" } },
#endif
    ///////////////////////
    //
    // E_GAMMA = 9.845 MeV
    //
    // 2-prong / Oxygen-16 decay / 2+ resonance [1]: peak=9.84450(5) MeV, gamma_width=0.000625(100) MeV, multipolarity=E2 / Egamma=9.845 MeV
    { "Eg=9.845 MeV O-16 (2+ 9.8445 MeV)",
      reaction_type::C12_ALPHA, 2, 9.845, 9.8445, 0.00625, 0.0,
      130, 273.15+20, "geometry_ELITPC_130mbar_1764Vdrift_25MHz.dat", BeamDirection::MINUS_X, -0.83, 5.51e-4, // run=20220822220311
      { "./Reco_9.845MeV_merged.root" } },
      // { "/mnt/NAS_STORAGE_ANALYSIS/higs_2022/9_85MeV/Reco/clicked/Reco_20220822T220311_0000_0001_0002_0003_0004_0005_0007_0008_0009.root" } },
    // // 2-prong / Oxygen-18 decay / no resonance: peak=9.845 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=9.845 MeV
    // { "Eg=9.845 MeV O-18 (none)",
    //   reaction_type::C14_ALPHA, 2, 9.845, 9.845, 0.150, 0.0,
    //   130, 273.15+20, "geometry_ELITPC_130mbar_1764Vdrift_25MHz.dat", BeamDirection::MINUS_X, -0.83, 5.51e-4, // run=20220822220311
    //   { "./Reco_9.845MeV_merged.root" } },
    // //      { "/mnt/NAS_STORAGE_ANALYSIS/higs_2022/9_85MeV/Reco/clicked/Reco_20220822T220311_0000_0001_0002_0003_0004_0005_0007_0008_0009.root" } },
    // // 3-prong / Carbon-12 decay / 2+ resonance [2]: peak=10.03(11) MeV, gamma_width=0.800(0.130)MeV, multipolarity=E2 / Egamma=9.845 MeV
    // { "Eg=9.845 MeV C-12 (2+ 10.03 MeV)",
    //   reaction_type::THREE_ALPHA_BE, 3, 9.845, 10.03, 0.5*0.800, 0.0,
    //   130, 273.15+20, "geometry_ELITPC_130mbar_1764Vdrift_25MHz.dat", BeamDirection::MINUS_X, -0.83, 5.51e-4, // run=20220822220311
    //   { "./Reco_9.845MeV_merged.root" } },
    //   //      { "/mnt/NAS_STORAGE_ANALYSIS/higs_2022/9_85MeV/Reco/clicked/Reco_20220822T220311_0000_0001_0002_0003_0004_0005_0007_0008_0009.root" } },
    ///////////////////////
    //
    // E_GAMMA = 11.5 MeV
    //
    // 2-prong / Oxygen-16 decay / 2+ resonance [1]: peak=11.520(4) MeV, gamma_width=0.071(3) MeV, multipolarity=E2 / Egamma=11.5 MeV
    { "Eg=11.5 MeV O-16 (2+ 11.520 MeV)",
      reaction_type::C12_ALPHA, 2, 11.5, 11.520, 0.5*0.071, 0.0,
      190, 273.15+20, "geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat", BeamDirection::MINUS_X, -1.20, 3.79e-3, // run=20220412064752
      { "/mnt/NAS_STORAGE_ANALYSIS/higs_2022/11_5MeV/Reco/clicked/Reco_20220412064752.root" } },
    // ///////////////////////
    // //
    // // E_GAMMA = 12.3 MeV
    // //
    // // 2-prong / Oxygen-16 decay / no resonance: peak=12.3 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=12.3 MeV
    // { "Eg=12.3 MeV O-16 (none)",
    //   reaction_type::C12_ALPHA, 2, 12.3, 12.3, 0.150, 0.0,
    //   190, 273.15+20, "geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat", BeamDirection::MINUS_X, -1.52, 3.25e-3, // run=20220412152817
    //   { "/mnt/NAS_STORAGE_ANALYSIS/higs_2022/12_3MeV/Reco/clicked/Reco_20220412152817.root" } },
    ///////////////////////
    //
    // E_GAMMA = 13.1 MeV
    //
    // 2-prong / Oxygen-16 decay / 1- resonance [1]: peak=13.090(8) MeV, gamma_width=0.130(5) MeV, multipolarity=E1 / Egamma=13.1 MeV
    { "Eg=13.1 MeV O-16 (1- 13.09 MeV)",
      reaction_type::C12_ALPHA, 2, 13.1, 13.09, 0.5*0.130, 0.0,
      250, 273.15+20, "geometry_ELITPC_250mbar_2744Vdrift_12.5MHz.dat", BeamDirection::MINUS_X, -0.73, 1.36e-3, // run=20220413095040
      { "/mnt/NAS_STORAGE_ANALYSIS/higs_2022/13_1MeV/Reco/clicked/Reco_20220413095040.root" } }// ,
    // ///////////////////////
    // //
    // // E_GAMMA = 13.5 MeV
    // //
    // // 2-prong / Oxygen-16 decay / no resonance: peak=13.5 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=13.5 MeV
    // { "Eg=13.5 MeV O-16 (none)",
    //   reaction_type::C12_ALPHA, 2, 13.5, 13.5, 0.150, 0.0,
    //   250, 273.15+20, "geometry_ELITPC_250mbar_2744Vdrift_12.5MHz.dat", BeamDirection::MINUS_X, 0.99, 3.45e-3, // run=20220413142950
    //   { "/mnt/NAS_STORAGE_ANALYSIS/higs_2022/13_5MeV/Reco/clicked/Reco_20220413142950.root" } }
#endif
  };
  // initialize necessary GeometryTPC pointers
  std::map<std::string, std::shared_ptr<GeometryTPC>> myGeometries;
  for(auto &it: myRecoFiles) {
    auto name = it.geometryFile;
    if(myGeometries.find(name)==myGeometries.end()) {
      myGeometries[name]=loadGeometry(name);
    }
  }
  // initialize necessary IonRangeCalculator pointers
  std::map<std::string, std::shared_ptr<IonRangeCalculator>> myRangeCalculators;
  for(auto &it: myRecoFiles) {
    auto name = it.geometryFile;
    if(myRangeCalculators.find(name)==myRangeCalculators.end()) {
      myRangeCalculators[name]=std::make_shared<IonRangeCalculator>(gas_mixture_type::CO2, it.pressure, it.temperature);
    }
  }
  
  std::cout << "=================" << std::endl;
  std::cout << "Input parameters:" << std::endl
	    << "* expected detector energy resolution: " << myOptions.detectorQvalueResolutionInMeV_CMS << " MeV" << std::endl
	    << "* list of PID categories to be tuned:";
  for(auto &category: myOptions.tuned_pid_map) {
    std::cout << std::endl
	      << "  \"" << category.first << "\" :";
    for(auto &pid: category.second) {
      std::cout << " " << enumDict::GetPidName(pid);
    }
  }
  std::cout << std::endl
  << "* type of energy scale correction: "<<enumDict::GetEnergyScaleName(myOptions.correction_type)<<std::endl
  << "* use only multiplicative corrections: "<<myOptions.use_scale_only<<std::endl
  << "* use nominal LAB gamma beam energy for LAB<->CMS boost: "<<myOptions.use_nominal_beam_energy<<std::endl
  << "* use only leading track information: "<<myOptions.use_nominal_beam_energy<<std::endl
  << "* correct missing PID information: "<<myOptions.assignMissingPIDs<<std::endl
  << "* ROOT minimizer TOLERANCE: " <<myOptions.tolerance<<std::endl
  << "* ROOT minimizer PRECISION: " <<myOptions.precision<<std::endl;

  ///////////////////////
  //
  // hardcoded list of cuts for different reaction channels and expected (resonance) peaks
  //
  size_t ireq=0;
  std::vector<RequirementsCollection<std::function<bool(Track3D *)>>> myRequirements{
#if(TOY_MC_TEST)
    ///////////////////////
    //
    // E_GAMMA = 8.66 MeV -- SIMULATION
    //
    // 2-prong / Oxygen-16 decay / no resonance: peak=8.66 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=8.66 MeV
    {tpcreco::cuts::Cut1{},
     tpcreco::cuts::Cut2{myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope, 12.5 /*beam_diameter*/},
     tpcreco::cuts::Cut3{myGeometries[myRecoFiles[ireq].geometryFile].get(), 5},
     tpcreco::cuts::Cut4{myGeometries[myRecoFiles[ireq].geometryFile].get(), 25, 5},
     tpcreco::cuts::Cut5{myGeometries[myRecoFiles[ireq].geometryFile].get(), 12.5 /*beam_diameter*/},
     tpcreco::cuts::Cut7{false,
    	 pid_type::ALPHA,     0, 100,  // pure O16 sample => accept ALL
    	 pid_type::CARBON_12, 0, 100}, // pure O16 sample => accept ALL
     tpcreco::cuts::Cut8{false,
    	 pid_type::ALPHA, 0, 300, 0, // very loose cuts - TO BE REFINED
	 getBeamDir_DET(myRecoFiles[ireq].beamDir, myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope)},
     tpcreco::cuts::Cut9{myRecoFiles[ireq++].ntracks}
    },
    // 3-prong / Carbon-12 decay democratic / no resonance: peak=8.66 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=8.66 MeV
    {tpcreco::cuts::Cut1{},
     tpcreco::cuts::Cut2{myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope, 12.5 /*beam_diameter*/},
     tpcreco::cuts::Cut3{myGeometries[myRecoFiles[ireq].geometryFile].get(), 5},
     tpcreco::cuts::Cut4{myGeometries[myRecoFiles[ireq].geometryFile].get(), 25, 5},
     tpcreco::cuts::Cut5{myGeometries[myRecoFiles[ireq].geometryFile].get(), 12.5 /*beam_diameter*/},
     tpcreco::cuts::Cut8{false,
     	 pid_type::ALPHA, 0, 300, 0.0, // pure C12 sample ==> accept ALL
     	 getBeamDir_DET(myRecoFiles[ireq].beamDir, myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope)},
     tpcreco::cuts::Cut9{myRecoFiles[ireq++].ntracks}
    },
    ///////////////////////
    //
    // E_GAMMA = 9.56 MeV -- SIMULATION
    //
    // 2-prong / Oxygen-16 decay / no resonance: peak=9.56 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=9.56 MeV
    {tpcreco::cuts::Cut1{},
     tpcreco::cuts::Cut2{myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope, 12.5 /* beam_diameter*/},
     tpcreco::cuts::Cut3{myGeometries[myRecoFiles[ireq].geometryFile].get(), 5},
     tpcreco::cuts::Cut4{myGeometries[myRecoFiles[ireq].geometryFile].get(), 25, 5},
     tpcreco::cuts::Cut5{myGeometries[myRecoFiles[ireq].geometryFile].get(), 12.5 /*beam_diameter*/},
     tpcreco::cuts::Cut7{false,
    	 pid_type::ALPHA,     0, 100,  // pure O16 sample => accept ALL
    	 pid_type::CARBON_12, 0, 100}, // pure O16 sample => accept ALL
     tpcreco::cuts::Cut8{false,
     	 pid_type::ALPHA, 0 /*lengthOffsetMin*/, 100 /*lengthOffsetMax*/, 0.0 /*lengthSlope*/,  // pure O16 sample => accept ALL
    	 getBeamDir_DET(myRecoFiles[ireq].beamDir, myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope)},
     tpcreco::cuts::Cut9{myRecoFiles[ireq++].ntracks}
    },
    ///////////////////////
    //
    // E_GAMMA = 9.845 MeV -- SIMULATION
    //
    // 2-prong / Oxygen-16 decay / no resonance: peak=9.845 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=9.845 MeV
    {tpcreco::cuts::Cut1{},
     tpcreco::cuts::Cut2{myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope, 12.5 /*beam_diameter*/},
     tpcreco::cuts::Cut3{myGeometries[myRecoFiles[ireq].geometryFile].get(), 5},
     tpcreco::cuts::Cut4{myGeometries[myRecoFiles[ireq].geometryFile].get(), 25, 5},
     tpcreco::cuts::Cut5{myGeometries[myRecoFiles[ireq].geometryFile].get(), 12.5 /*beam_diameter*/},
     tpcreco::cuts::Cut7{false,
	 pid_type::ALPHA,     0, 100,  // pure O16 sample => accept ALL
	 pid_type::CARBON_12, 0, 100}, // pure O16 sample => accept ALL
     tpcreco::cuts::Cut8{false,
         pid_type::ALPHA, 0 /*lengthOffsetMin*/, 100 /*lengthOffsetMax*/, 0.0 /*lengthSlope*/, // pure O16 sample => accept ALL
	 getBeamDir_DET(myRecoFiles[ireq].beamDir, myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope)},
     tpcreco::cuts::Cut9{myRecoFiles[ireq++].ntracks}
    },
    ///////////////////////
    //
    // E_GAMMA = 12.3 MeV -- SIMULATION
    //
    // 2-prong / Oxygen-16 decay / no resonance: peak=12.3 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=12.3 MeV
    {tpcreco::cuts::Cut1{},
     tpcreco::cuts::Cut2{myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope, 12.5 /*beam_diameter*/},
     tpcreco::cuts::Cut3{myGeometries[myRecoFiles[ireq].geometryFile].get(), 5},
     tpcreco::cuts::Cut4{myGeometries[myRecoFiles[ireq].geometryFile].get(), 25, 5},
     tpcreco::cuts::Cut5{myGeometries[myRecoFiles[ireq].geometryFile].get(), 12.5 /*beam_diameter*/},
     tpcreco::cuts::Cut7{false,
    	 pid_type::ALPHA,     0, 100,  // pure O16 sample => accept ALL
    	 pid_type::CARBON_12, 0, 100}, // pure O16 sample => accept ALL
     tpcreco::cuts::Cut8{false,
     	 pid_type::ALPHA, 0 /*lengthOffsetMin*/, 100 /*lengthOffsetMax*/, 0.0 /*lengthSlope*/, // pure O16 sample => accept ALL
    	 getBeamDir_DET(myRecoFiles[ireq].beamDir, myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope)},
     tpcreco::cuts::Cut9{myRecoFiles[ireq++].ntracks}
    },
    ///////////////////////
    //
    // E_GAMMA = 13.1 MeV -- SIMULATION
    //
    // 2-prong / Oxygen-16 decay / no resonance: peak=13.1 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=13.1 MeV
    {tpcreco::cuts::Cut1{},
     tpcreco::cuts::Cut2{myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope, 12.5 /*beam_diameter*/},
     tpcreco::cuts::Cut3{myGeometries[myRecoFiles[ireq].geometryFile].get(), 5},
     tpcreco::cuts::Cut4{myGeometries[myRecoFiles[ireq].geometryFile].get(), 25, 5},
     tpcreco::cuts::Cut5{myGeometries[myRecoFiles[ireq].geometryFile].get(), 12.5 /*beam_diameter*/},
     tpcreco::cuts::Cut7{false,
    	 pid_type::ALPHA,     0, 100,  // pure O16 sample => accept ALL
    	 pid_type::CARBON_12, 0, 100}, // pure O16 sample => accept ALL
     tpcreco::cuts::Cut8{false,
     	 pid_type::ALPHA, 0 /*lengthOffsetMin*/, 100 /*lengthOffsetMax*/, 0.0 /*lengthSlope*/, // pure O16 sample => accept ALL
    	 getBeamDir_DET(myRecoFiles[ireq].beamDir, myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope)},
     tpcreco::cuts::Cut9{myRecoFiles[ireq++].ntracks}
    }
#else
    // ///////////////////////
    // //
    // // E_GAMMA = 8.66 MeV
    // //
    // // 2-prong / Oxygen-18 decay / no resonance: peak=8.66 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=8.66 MeV
    // {tpcreco::cuts::Cut1{},
    //  tpcreco::cuts::Cut2{myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope, 12.5 /*beam_diameter*/},
    //  tpcreco::cuts::Cut3{myGeometries[myRecoFiles[ireq].geometryFile].get(), 5},
    //  tpcreco::cuts::Cut4{myGeometries[myRecoFiles[ireq].geometryFile].get(), 25, 5},
    //  tpcreco::cuts::Cut5{myGeometries[myRecoFiles[ireq].geometryFile].get(), 12.5 /*beam_diameter*/},
    //  tpcreco::cuts::Cut7{false,
    // 	 pid_type::ALPHA,     44, 61,
    // 	 pid_type::CARBON_14,  9, 16},
    //  tpcreco::cuts::Cut8{false,
    // 	 pid_type::ALPHA, 44 /*lengthOffsetMin*/, 61 /*lengthOffsetMax*/, 0.0 /*lengthSlope*/, // LOOSE CUT !!!!!
    // 	 getBeamDir_DET(myRecoFiles[ireq].beamDir, myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope)},
    //  tpcreco::cuts::Cut9{myRecoFiles[ireq++].ntracks}
    // },
    // // 3-prong / Carbon-12 decay / no resonance: peak=8.66 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=8.66 MeV
    // {tpcreco::cuts::Cut1{},
    //  tpcreco::cuts::Cut2{myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope, 12.5 /*beam_diameter*/},
    //  tpcreco::cuts::Cut3{myGeometries[myRecoFiles[ireq].geometryFile].get(), 5},
    //  tpcreco::cuts::Cut4{myGeometries[myRecoFiles[ireq].geometryFile].get(), 25, 5},
    //  tpcreco::cuts::Cut5{myGeometries[myRecoFiles[ireq].geometryFile].get(), 12.5 /*beam_diameter*/},
    //  tpcreco::cuts::Cut8{false,
    //  	 pid_type::ALPHA, 0, 300, 0.0, // very loose cuts - TO BE REFINED
    //  	 getBeamDir_DET(myRecoFiles[ireq].beamDir, myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope)},
    //  tpcreco::cuts::Cut9{myRecoFiles[ireq++].ntracks}
    // },
#if(USE_REAL_DATA__9_56_MEV)
    ///////////////////////
    //
    // E_GAMMA = 9.56 MeV
    //
    // 2-prong / Oxygen-16 decay / 1- resonance [1]: peak=9.585(11) MeV, gamma_width=0.420(20) MeV, multipolarity=E1 / Egamma=9.56 MeV
    {tpcreco::cuts::Cut1{},
     tpcreco::cuts::Cut2{myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope, 12.5 /* beam_diameter*/},
     tpcreco::cuts::Cut3{myGeometries[myRecoFiles[ireq].geometryFile].get(), 5},
     tpcreco::cuts::Cut4{myGeometries[myRecoFiles[ireq].geometryFile].get(), 25, 5},
     tpcreco::cuts::Cut5{myGeometries[myRecoFiles[ireq].geometryFile].get(), 12.5 /*beam_diameter*/},
     tpcreco::cuts::Cut7{false,
    	 pid_type::ALPHA,     42-2.6, 50+2.6,
    	 pid_type::CARBON_12, 10, 15},
     tpcreco::cuts::Cut8{false,
    	 pid_type::ALPHA, 42 /*lengthOffsetMin*/, 50 /*lengthOffsetMax*/, 2.6 /*lengthSlope*/, // slope from MC fit
    	 getBeamDir_DET(myRecoFiles[ireq].beamDir, myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope)},
     tpcreco::cuts::Cut9{myRecoFiles[ireq++].ntracks}
    },
    // // 2-prong / Oxygen-16 decay / 2+ resonance [1]: peak=9.84450(5) MeV, gamma_width=0.000625(100) MeV, multipolarity=E2 / Egamma=9.56 MeV
    // {tpcreco::cuts::Cut1{},
    //  tpcreco::cuts::Cut2{myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope, 12.5 /* beam_diameter*/},
    //  tpcreco::cuts::Cut3{myGeometries[myRecoFiles[ireq].geometryFile].get(), 5},
    //  tpcreco::cuts::Cut4{myGeometries[myRecoFiles[ireq].geometryFile].get(), 25, 5},
    //  tpcreco::cuts::Cut5{myGeometries[myRecoFiles[ireq].geometryFile].get(), 12.5 /*beam_diameter*/},
    //  tpcreco::cuts::Cut7{false,
    // 	 pid_type::ALPHA,     47, 60,
    // 	 pid_type::CARBON_12, 9,  16},
    //  tpcreco::cuts::Cut8{false,
    // 	 pid_type::ALPHA, 0.5*(47+60)-6 /*lengthOffsetMin*/, 0.5*(47+60)+6 /*lengthOffsetMax*/, 2.6 /*lengthSlope*/, // slope from MC fit
    // 	 getBeamDir_DET(myRecoFiles[ireq].beamDir, myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope)},
    //  tpcreco::cuts::Cut9{myRecoFiles[ireq++].ntracks}
    // },
    // // 2-prong / Oxygen-18 decay / no resonance: peak=9.56 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=9.56 MeV
    // {tpcreco::cuts::Cut1{},
    //  tpcreco::cuts::Cut2{myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope, 12.5 /* beam_diameter*/},
    //  tpcreco::cuts::Cut3{myGeometries[myRecoFiles[ireq].geometryFile].get(), 5},
    //  tpcreco::cuts::Cut4{myGeometries[myRecoFiles[ireq].geometryFile].get(), 25, 5},
    //  tpcreco::cuts::Cut5{myGeometries[myRecoFiles[ireq].geometryFile].get(), 12.5 /*beam_diameter*/},
    //  tpcreco::cuts::Cut7{false,
    // 	 pid_type::ALPHA,     64, 83,
    // 	 pid_type::CARBON_14, 12, 17},
    //  tpcreco::cuts::Cut8{false,
    // 	 pid_type::ALPHA, 64 /*lengthOffsetMin*/, 83 /*lengthOffsetMax*/, 0.0 /*lengthSlope*/, // LOOSE CUT !!!!!
    // 	 getBeamDir_DET(myRecoFiles[ireq].beamDir, myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope)},
    //  tpcreco::cuts::Cut9{myRecoFiles[ireq++].ntracks}
    // },
#endif
    ///////////////////////
    //
    // E_GAMMA = 9.845 MeV
    //
    // 2-prong / Oxygen-16 decay / 2+ resonance [1]: peak=9.84450(5) MeV, gamma_width=0.000625(100) MeV, multipolarity=E2 / Egamma=9.845 MeV
    {tpcreco::cuts::Cut1{},
     tpcreco::cuts::Cut2{myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope, 12.5 /*beam_diameter*/},
     tpcreco::cuts::Cut3{myGeometries[myRecoFiles[ireq].geometryFile].get(), 5},
     tpcreco::cuts::Cut4{myGeometries[myRecoFiles[ireq].geometryFile].get(), 25, 5},
     tpcreco::cuts::Cut5{myGeometries[myRecoFiles[ireq].geometryFile].get(), 12.5 /*beam_diameter*/},
     tpcreco::cuts::Cut7{false,
	 pid_type::ALPHA,     47, 60,
	 pid_type::CARBON_12,  9, 16},
     tpcreco::cuts::Cut8{false,
         pid_type::ALPHA, 0.5*(47+60)-6 /*lengthOffsetMin*/, 0.5*(47+60)+6 /*lengthOffsetMax*/, 2.6 /*lengthSlope*/, // slope from MC fit
	 getBeamDir_DET(myRecoFiles[ireq].beamDir, myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope)},
     tpcreco::cuts::Cut9{myRecoFiles[ireq++].ntracks}
    },
    // // 2-prong / Oxygen-18 decay / no resonance: peak=9.845 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=9.845 MeV
    // {tpcreco::cuts::Cut1{},
    //  tpcreco::cuts::Cut2{myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope, 12.5 /*beam_diameter*/},
    //  tpcreco::cuts::Cut3{myGeometries[myRecoFiles[ireq].geometryFile].get(), 5},
    //  tpcreco::cuts::Cut4{myGeometries[myRecoFiles[ireq].geometryFile].get(), 25, 5},
    //  tpcreco::cuts::Cut5{myGeometries[myRecoFiles[ireq].geometryFile].get(), 12.5 /*beam_diameter*/},
    //  tpcreco::cuts::Cut7{false,
    // 	 pid_type::ALPHA,     69, 86,
    // 	 pid_type::CARBON_14, 11, 19},
    //  tpcreco::cuts::Cut8{false,
    // 	 pid_type::ALPHA, 69 /*lengthOffsetMin*/, 86 /*lengthOffsetMax*/, 0.0 /*lengthSlope*/, // LOOSE CUT !!!!!
    // 	 getBeamDir_DET(myRecoFiles[ireq].beamDir, myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope)},
    //  tpcreco::cuts::Cut9{myRecoFiles[ireq++].ntracks}
    // },
    // // 3-prong / Carbon-12 decay / 2+ resonance [2]: peak=10.03(11) MeV, gamma_width=0.800(0.130)MeV, multipolarity=E2 / Egamma=9.845 MeV
    // {tpcreco::cuts::Cut1{},
    //  tpcreco::cuts::Cut2{myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope, 12.5 /*beam_diameter*/},
    //  tpcreco::cuts::Cut3{myGeometries[myRecoFiles[ireq].geometryFile].get(), 5},
    //  tpcreco::cuts::Cut4{myGeometries[myRecoFiles[ireq].geometryFile].get(), 25, 5},
    //  tpcreco::cuts::Cut5{myGeometries[myRecoFiles[ireq].geometryFile].get(), 12.5 /*beam_diameter*/},
    //  tpcreco::cuts::Cut8{false,
    // 	 pid_type::ALPHA, 0, 300, 0.0, // very loose cuts - TO BE REFINED
    // 	 getBeamDir_DET(myRecoFiles[ireq].beamDir, myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope)},
    //  tpcreco::cuts::Cut9{myRecoFiles[ireq++].ntracks}
    // },
    ///////////////////////
    //
    // E_GAMMA = 11.5 MeV
    //
    // 2-prong / Oxygen-16 decay / 2+ resonance [1]: peak=11.520(4) MeV, gamma_width=0.071(3) MeV, multipolarity=E2 / Egamma=11.5 MeV
    {tpcreco::cuts::Cut1{},
     tpcreco::cuts::Cut2{myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope, 12.5 /*beam_diameter*/},
     tpcreco::cuts::Cut3{myGeometries[myRecoFiles[ireq].geometryFile].get(), 5},
     tpcreco::cuts::Cut4{myGeometries[myRecoFiles[ireq].geometryFile].get(), 25, 5},
     tpcreco::cuts::Cut5{myGeometries[myRecoFiles[ireq].geometryFile].get(), 12.5 /*beam_diameter*/},
     tpcreco::cuts::Cut7{false,
	 pid_type::ALPHA,     59, 78,
	 pid_type::CARBON_12, 6,  14},
     tpcreco::cuts::Cut8{false,
         pid_type::ALPHA, 59 /*lengthOffsetMin*/, 78 /*lengthOffsetMax*/, 0.0 /*lengthSlope*/, // LOOSE CUT !!!!!
	 getBeamDir_DET(myRecoFiles[ireq].beamDir, myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope)},
     tpcreco::cuts::Cut9{myRecoFiles[ireq++].ntracks}
    },
    // ///////////////////////
    // //
    // // E_GAMMA = 12.3 MeV
    // //
    // // 2-prong / Oxygen-16 decay / no resonance: peak=12.3 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=12.3 MeV
    // {tpcreco::cuts::Cut1{},
    //  tpcreco::cuts::Cut2{myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope, 12.5 /*beam_diameter*/},
    //  tpcreco::cuts::Cut3{myGeometries[myRecoFiles[ireq].geometryFile].get(), 5},
    //  tpcreco::cuts::Cut4{myGeometries[myRecoFiles[ireq].geometryFile].get(), 25, 5},
    //  tpcreco::cuts::Cut5{myGeometries[myRecoFiles[ireq].geometryFile].get(), 12.5 /*beam_diameter*/},
    //  tpcreco::cuts::Cut7{false,
    // 	 pid_type::ALPHA,     80, 94,
    // 	 pid_type::CARBON_12, 11, 18},
    //  tpcreco::cuts::Cut8{false,
    //      pid_type::ALPHA, 80 /*lengthOffsetMin*/, 94 /*lengthOffsetMax*/, 0.0 /*lengthSlope*/, // LOOSE CUT !!!!!
    // 	 getBeamDir_DET(myRecoFiles[ireq].beamDir, myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope)},
    //  tpcreco::cuts::Cut9{myRecoFiles[ireq++].ntracks}
    // },
    ///////////////////////
    //
    // E_GAMMA = 13.1 MeV
    //
    // 2-prong / Oxygen-16 decay / 1- resonance [1]: peak=13.090(8) MeV, gamma_width=0.130(5) MeV, multipolarity=E1 / Egamma=13.1 MeV
    {tpcreco::cuts::Cut1{},
     tpcreco::cuts::Cut2{myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope, 12.5 /*beam_diameter*/},
     tpcreco::cuts::Cut3{myGeometries[myRecoFiles[ireq].geometryFile].get(), 5},
     tpcreco::cuts::Cut4{myGeometries[myRecoFiles[ireq].geometryFile].get(), 25, 5},
     tpcreco::cuts::Cut5{myGeometries[myRecoFiles[ireq].geometryFile].get(), 12.5 /*beam_diameter*/},
     tpcreco::cuts::Cut7{false,
	 pid_type::ALPHA,     70, 90,
	 pid_type::CARBON_12, 7,  14},
     tpcreco::cuts::Cut8{false,
         pid_type::ALPHA, 70 /*lengthOffsetMin*/, 90 /*lengthOffsetMax*/, 0.0 /*lengthSlope*/, // LOOSE CUT !!!!!
	 getBeamDir_DET(myRecoFiles[ireq].beamDir, myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope)},
     tpcreco::cuts::Cut9{myRecoFiles[ireq++].ntracks}
    }// ,
    // ///////////////////////
    // //
    // // E_GAMMA = 13.5 MeV
    // //
    // // 2-prong / Oxygen-16 decay / no resonance: peak=13.5 MeV, sigma=0.150 MeV, multipolarity=???? / Egamma=13.5 MeV
    // {tpcreco::cuts::Cut1{},
    //  tpcreco::cuts::Cut2{myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope, 12.5 /*beam_diameter*/},
    //  tpcreco::cuts::Cut3{myGeometries[myRecoFiles[ireq].geometryFile].get(), 5},
    //  tpcreco::cuts::Cut4{myGeometries[myRecoFiles[ireq].geometryFile].get(), 25, 5},
    //  tpcreco::cuts::Cut5{myGeometries[myRecoFiles[ireq].geometryFile].get(), 12.5 /*beam_diameter*/},
    //  tpcreco::cuts::Cut7{false,
    // 	 pid_type::ALPHA,     73, 89,
    // 	 pid_type::CARBON_12, 8,  14},
    //  tpcreco::cuts::Cut8{false,
    //      pid_type::ALPHA, 73 /*lengthOffsetMin*/, 89 /*lengthOffsetMax*/, 0.0 /*lengthSlope*/, // LOOSE CUT !!!!!
    // 	 getBeamDir_DET(myRecoFiles[ireq].beamDir, myRecoFiles[ireq].beamOffset, myRecoFiles[ireq].beamSlope)},
    //  tpcreco::cuts::Cut9{myRecoFiles[ireq++].ntracks}
    //  }
#endif
  };

  ///////////////////////
  //
  // process RECO files and store selected events for a given reaction channel
  //
  std::vector<std::vector<EnergyScale_analysis::TrackCollection>> myEvents;
  assert(myRequirements.size()==myRecoFiles.size());
  std::cout << "=================" << std::endl;
  std::cout << "Event selections:" << std::endl;

  for(auto isel=0; isel<myRecoFiles.size(); isel++) {
    auto nevents = selectRecoEvents(myRecoFiles[isel].recoFiles,
				    myGeometries[myRecoFiles[isel].geometryFile],
				    getCoordinateConverter(myRecoFiles[isel].beamDir,
							   myRecoFiles[isel].beamOffset,
							   myRecoFiles[isel].beamSlope),
				    myRequirements[isel], myEvents);
    // on demand, correct for missing PIDs according to assumed reaction hypothesis
    // NOTE: this step is not needed for generator level MC, but is crucial
    //       for manually reconstructed data in which PIDs are not assigned at all.
    auto ncorrected = 0U;
    if(myOptions.assignMissingPIDs) {
      for(auto &event: myEvents.back()) {
	ncorrected += ( assignMissingPIDs(myRecoFiles[isel].reaction, event) ? 1 : 0 );
      }
    }

    std::cout << "-----------------" << std::endl;
    std::cout << "Data sample:     " << myRecoFiles[isel].description << std::endl; 
    std::cout << "Accepted events: " << nevents << " (including " << ncorrected << " with PID corrections)" << std::endl;

  }
  std::cout << "=================" << std::endl;
  assert(myRecoFiles.size()==myEvents.size());
   
  ///////////////////////
  //
  // list of reaction channels and events after selection
  //
  std::vector<EnergyScale_analysis::EventCollection> mySelections;
  size_t isel=0U;
  for(auto &it: myRecoFiles) {
    mySelections.push_back({ it.description, it.reaction, it.gammaEnergy_LAB,
	                     it.peakEnergy_LAB, it.peakSigma_LAB, it.excitedMassDiff,
	                     myRangeCalculators[myRecoFiles[isel].geometryFile],
	                     myEvents[isel] });
    isel++;
  }
  assert(myRecoFiles.size()==mySelections.size());

  ///////////////////////
  //
  // perform global fit on selected events
  //
  fitPeaks(myOptions, mySelections);

  return 0;
}

/////////////////////////////
////////////////////////////
// helper function to initialize BEAM<->DET coordinate converter
CoordinateConverter getCoordinateConverter(const BeamDirection beamDir, const double beam_offset, const double beam_slope) {
  switch(beamDir) {
  case BeamDirection::MINUS_X:
    { auto aConv = CoordinateConverter({-M_PI/2, M_PI/2, 0}, // NOTE: see caveats for TRotation::SetXEulerAngles in ROOT documentation
				       {M_PI/2, atan(-beam_slope), -M_PI/2},
				       {-beam_offset*sin(atan(beam_slope)), beam_offset*cos(atan(beam_slope)), 0});
      return aConv;
    }
    break;
  case BeamDirection::PLUS_X:
    { auto aConv = CoordinateConverter({M_PI/2, M_PI/2, 0}, // NOTE: see caveats for TRotation::SetXEulerAngles in ROOT documentation
				       {M_PI/2, atan(-beam_slope), -M_PI/2},
				       {beam_offset*sin(atan(beam_slope)), -beam_offset*cos(atan(beam_slope)), 0});
      return aConv;
    }
    break;
  default:
    throw std::runtime_error("Wrong beam direction");
  };
  return CoordinateConverter({},{},{});
}

/////////////////////////////
////////////////////////////
// helper function to calculate unit vector of tilted beam direction in DET coordinate system
TVector3 getBeamDir_DET(const BeamDirection beamDir, const double beam_offset, const double beam_slope){
  return getCoordinateConverter(beamDir, beam_offset, beam_slope).beamToDet({0,0,1}); // actual beam unit vector in DET coordinates (i.e. direction of photons)
}

////////////////////////////
////////////////////////////
// helper function to initialize TPC geometry / worrking conditions
std::shared_ptr<GeometryTPC> loadGeometry(const std::string fileName){
  return std::make_shared<GeometryTPC>(fileName.c_str(), false);
}

////////////////////////////
////////////////////////////
// helper function to correct single event for missing PIDs according to a given reaction channel hypothesis,
// returns TRUE if PID correction was actually made.
// NOTE: input tracks HAVE TO BE pre-sorted in descending order of track lengths (longest first).
bool assignMissingPIDs(const reaction_type aReaction, EnergyScale_analysis::TrackCollection &list){ // track collectiom to be edited

  auto result = false;
  
  const int ntracks = list.size(); // number of observed final tracks
  switch(ntracks) {
  case 2:
    switch(aReaction) {
    case reaction_type::C12_ALPHA: // O-16 breakup
      if(list.front().pid==pid_type::UNKNOWN) list.front().pid = pid_type::ALPHA;
      if(list.back().pid==pid_type::UNKNOWN) list.back().pid = pid_type::CARBON_12;
      result = true;
      break;
    case reaction_type::C13_ALPHA: // O-17 breakup
      if(list.front().pid==pid_type::UNKNOWN) list.front().pid = pid_type::ALPHA;
      if(list.back().pid==pid_type::UNKNOWN) list.back().pid = pid_type::CARBON_13;
      result = true;
      break;
    case reaction_type::C14_ALPHA: // O-18 breakup
      if(list.front().pid==pid_type::UNKNOWN) list.front().pid = pid_type::ALPHA;
      if(list.back().pid==pid_type::UNKNOWN) list.back().pid = pid_type::CARBON_14;
      result = true;
      break;
    default:;
    };
    break;
  case 3:
    switch(aReaction) {
    case reaction_type::THREE_ALPHA_BE:
    case reaction_type::THREE_ALPHA_DEMOCRATIC:
      for(auto &track: list) {
	if(track.pid==pid_type::UNKNOWN) track.pid = pid_type::ALPHA;
      }
      result = true;
      break;
    default:;
    };
  default:;
  };
  
  return result;
}

////////////////////////////
////////////////////////////
// helper function to select events passing the cuts for a given reaction channel
size_t selectRecoEvents(const std::vector<std::string> &recoFileList, // list of input RECO files
			const std::shared_ptr<GeometryTPC> &geometryPtr, // TPC geometry input config file
			const CoordinateConverter &coordinateConverter, // DET<-->BEAM coordinate converter
			RequirementsCollection<std::function<bool(Track3D *)>> &cuts, // list of cuts
			std::vector<std::vector<EnergyScale_analysis::TrackCollection>> &collection){ // event collection to be filled
  collection.push_back({}); // add new empty element for a given reaction channel / event selection
  // TChain *aChain = new TChain("TPCRecoData");
  // for(auto &name: recoFileList) {
  //   aChain->Add(name.c_str());
  // }
  // if(!aChain->GetEntries()){
  //   throw std::runtime_error("No entries found in the provided list of RECO files");
  // }
  
  // TBranch *aBranch  = aChain->GetBranch("RecoEvent");
  // if(!aBranch) {
  //   throw std::runtime_error("Cannot find 'RecoEvent' branch");
  // }
  // auto *aTrack = new Track3D();
  // aBranch->SetAddress(&aTrack);
  
  // TBranch *aBranchInfo = aChain->GetBranch("EventInfo");
  // if(!aBranchInfo) {
  //   throw std::runtime_error("Cannot find 'EventInfo' branch");
  // }
  // auto *aEventInfo = new eventraw::EventInfo();
  // aBranchInfo->SetAddress(&aEventInfo);
  
  // std::cout << __FUNCTION__ << ": Starting to loop " << aChain->GetEntries() << " events" << std::endl;
  //  std::cout << __FUNCTION__ << ": Starting to loop " << aChain->GetEntries() << " events with sorting by {runId, eventId}" << std::endl;
  //  aChain->BuildIndex("runId", "eventId");
  //  auto index =static_cast<TTreeIndex*>(aChain->GetTreeIndex())->GetIndex();

  ///////////////
  TFile *aFile = new TFile(recoFileList.front().c_str());
  if(!aFile || !aFile->IsOpen()){
    std::cout<<KRED<<"Input file: "<<RST<<recoFileList.front()
  	     <<KRED<<" not found!"<<RST
  	     <<std::endl;
    return -1;
  }
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
  
  //  std::cout << __FUNCTION__ << ": Starting to loop " << aTree->GetEntries() << " events with sorting by {runId, eventId}" << std::endl;
  std::cout << __FUNCTION__ << ": Starting to loop " << aTree->GetEntries() << " events" << std::endl;
  // aTree->BuildIndex("runId", "eventId");
  // auto index =static_cast<TTreeIndex*>(aTree->GetTreeIndex())->GetIndex();
  for(unsigned int iEntry=0;iEntry<aTree->GetEntries();++iEntry){
    aTree->GetEntry(iEntry);
  ///////////////////
  
  // for(unsigned int iEntry=0;iEntry<aChain->GetEntries();++iEntry){
  //   //    aChain->GetEntry(index[iEntry]);
  //   aChain->GetEntry(iEntry);
    
    auto list = aTrack->getSegments();
    for (auto & aSegment: list)  aSegment.setGeometry(geometryPtr); // need TPC geometry for track projections

    ////////// DEBUG
    //    std::cout << __FUNCTION__ << ": list.size=" << list.size() << std::endl;
    // for (auto & aSegment: list) {
    //   std::cout << __FUNCTION__ << ": pid=" << enumDict::GetPidName(aSegment.getPID())
    // 		<< " segment_BEAM=";
    //   auto tangent_BEAM = coordinateConverter.detToBeam(aSegment.getTangent());
    //   tangent_BEAM.Print();
    // }
    ////////// DEBUG
    
    if(!cuts(aTrack)){
      continue; // skip this event
    }
    
    // get sorted list of tracks (descending order by track length)
    std::sort(list.begin(), list.end(),
	    [](const TrackSegment3D& a, const TrackSegment3D& b) {
	      return a.getLength() > b.getLength();
	    });

    // store accepted event data
    EnergyScale_analysis::TrackCollection aColl;
    for (auto & aSegment: list) {
      // convert DET to BEAM coordinates
      auto tangent_BEAM = coordinateConverter.detToBeam(aSegment.getTangent());
      aColl.push_back({aSegment.getPID(), tangent_BEAM.Phi(), tangent_BEAM.Theta(),
	    aSegment.getLength()});
    }   
    collection.back().push_back(aColl);
  }
  return collection.back().size();
}

////////////////////////////
////////////////////////////
double fitPeaks(const EnergyScale_analysis::FitOptionType &aOption,
		std::vector<EnergyScale_analysis::EventCollection> &aSelection){ // input event collection
  
  EnergyScale_analysis myFit(aOption, aSelection); // provides chi^2
  const int npar=myFit.getNparams();
  ROOT::Fit::Fitter fitter;
  ROOT::Math::Functor fcn(myFit, npar); // this line must be called after fixing all initialization parameters of chi^2 function

  double pStart[npar];
  for(auto ipar=0U; ipar<npar; ipar++) {
    pStart[ipar] = myFit.getParameter(ipar);
  }
  fitter.SetFCN(fcn, pStart, aSelection.size(), true);

  // set limits for all parameters
  for(auto ipar=0U; ipar<npar; ipar++) {
    if(ipar%2 == 0) { // offset [mm] or [MeV]
      switch(aOption.correction_type) {
      case escale_type::LENGTH:
      case escale_type::ZENEK:
	fitter.Config().ParSettings(ipar).SetLimits(-10.0, 10.0); // [mm]
	break;
      case escale_type::ENERGY_LAB:
      case escale_type::ENERGY_CMS:
	fitter.Config().ParSettings(ipar).SetLimits(-2.0, 2.0); // [MeV]
	break;
	fitter.Config().ParSettings(ipar).SetLimits(-10.0, 10.0); // [mm]
	break;
      default:
	throw std::runtime_error("Unsupported energy scale correction type");
      };
    } else { // scale
      fitter.Config().ParSettings(ipar).SetLimits(0.1, 5.0);
    }
    std::cout << "Setting par[" << ipar << "]  limits=["
	      << fitter.Config().ParSettings(ipar).LowerLimit()<<", "
	      << fitter.Config().ParSettings(ipar).UpperLimit()<<"]" << std::endl;
  }
  // fix some parameters
  for(auto ipar=0U; ipar<npar; ipar++) {
    if(myFit.isParameterFixed(ipar)) {
      fitter.Config().ParSettings(ipar).Fix();
      std::cout << "Fixing par[" << ipar << "]  to value=" 
		<< fitter.Config().ParSettings(ipar).Value() << std::endl;
    }
  }
  // set step sizes
  for (auto ipar=0U; ipar<npar; ipar++) {
    fitter.Config().ParSettings(ipar).SetStepSize(0.1);
  }

  ////// DEBUG
  //  exit(-1);
  ////// DEBUG

  bool fitStatus=false;
  ////////////////////////
  // Fit ITER=1
  ////////////////////////
  //
  std::cout << __FUNCTION__ << std::endl
	    << __FUNCTION__ << ": >>>>>>>> Fitter 1st pass <<<<<<<<" << std::endl
	    << __FUNCTION__ << std::endl;

  // print initial values
  auto initialChi2 = myFit(pStart);
  std::cout << "==============================" << std::endl;
  std::cout << "Intial values and constraints:" << std::endl;
  std::cout << "------------------------------" << std::endl;
  for (auto ipar=0U; ipar<npar; ++ipar) {
    std::cout <<"par[" << ipar << "]=" << fitter.Config().ParSettings(ipar).Value() <<"  limits=["
	      << fitter.Config().ParSettings(ipar).LowerLimit()<<", "
	      << fitter.Config().ParSettings(ipar).UpperLimit()<<"]  isFixed="
	      << fitter.Config().ParSettings(ipar).IsFixed() << std::endl;
  }
  std::cout << "------------------------------" << std::endl;
  std::cout << "CHI2 evaluated at starting point =" << initialChi2 << std::endl;
  std::cout << "==============================" << std::endl;
  ///////// DEBUG
  myFit.plotQvalueFits("beforeFit");
  ///////// DEBUG
  
  fitter.Config().MinimizerOptions().SetMinimizerType("Minuit2");
  fitter.Config().MinimizerOptions().SetMinimizerAlgorithm("Fumili2");
  myFit.setMinimizerAlgorithm(fitter.Config().MinimizerOptions().MinimizerAlgorithm()); // sets proper CHI2 scaling factor for FUMILI2
  fitter.Config().MinimizerOptions().SetMaxFunctionCalls(10000);
  fitter.Config().MinimizerOptions().SetMaxIterations(1000);
  fitter.Config().MinimizerOptions().SetStrategy(1);
  fitter.Config().MinimizerOptions().SetTolerance(aOption.tolerance);
  fitter.Config().MinimizerOptions().SetPrecision(aOption.precision);
  fitter.Config().MinimizerOptions().SetPrintLevel(2);

  // check fit results
  fitStatus = fitter.FitFCN();
  std::cout << __FUNCTION__ << ": "
	    << fitter.Config().MinimizerOptions().MinimizerType() << "/" << fitter.Config().MinimizerOptions().MinimizerAlgorithm()
	    << " --> Fit " << ( !fitStatus ? "FAILED" : "OK") << std::endl;

  ////////////////////////
  // Fit ITER=2
  ////////////////////////
  //
  std::cout << __FUNCTION__ << std::endl
	    << __FUNCTION__ << ": >>>>>>>> Fitter 2nd pass <<<<<<<<" << std::endl
	    << __FUNCTION__ << std::endl;

  // set parameters from previous iteration
  const ROOT::Fit::FitResult & result_0 = fitter.Result();
  const double * parFit_0 = result_0.GetParams();

  // print initial values
  auto initialChi2_0 = myFit(parFit_0);
  std::cout << "==============================" << std::endl;
  std::cout << "Intial values and constraints:" << std::endl;
  std::cout << "------------------------------" << std::endl;
  for (auto ipar=0U; ipar<npar; ++ipar) {
    fitter.Config().ParSettings(ipar).SetValue(parFit_0[ipar]);
    std::cout <<"par[" << ipar << "]=" << fitter.Config().ParSettings(ipar).Value() <<"  limits=["
	      << fitter.Config().ParSettings(ipar).LowerLimit()<<", "
	      << fitter.Config().ParSettings(ipar).UpperLimit()<<"]  isFixed="
	      << fitter.Config().ParSettings(ipar).IsFixed() << std::endl;
  }
  std::cout << "------------------------------" << std::endl;
  std::cout << "CHI2 evaluated at starting point =" << initialChi2+0 << std::endl;
  std::cout << "==============================" << std::endl;

  fitter.Config().MinimizerOptions().SetMinimizerType("Minuit2");
  fitter.Config().MinimizerOptions().SetMinimizerAlgorithm("Migrad2");
  myFit.setMinimizerAlgorithm(fitter.Config().MinimizerOptions().MinimizerAlgorithm()); // sets proper CHI2 scaling factor for MIGRAD2
  fitter.Config().MinimizerOptions().SetMaxFunctionCalls(10000);
  fitter.Config().MinimizerOptions().SetMaxIterations(1000);
  fitter.Config().MinimizerOptions().SetStrategy(1);
  fitter.Config().MinimizerOptions().SetTolerance(aOption.tolerance);
  fitter.Config().MinimizerOptions().SetPrecision(aOption.precision);
  fitter.Config().MinimizerOptions().SetPrintLevel(2);

  // check fit results
  fitStatus = fitter.FitFCN();
  std::cout << __FUNCTION__ << ": "
	    << fitter.Config().MinimizerOptions().MinimizerType() << "/" << fitter.Config().MinimizerOptions().MinimizerAlgorithm()
	    << " --> Fit " << ( !fitStatus ? "FAILED" : "OK") << std::endl;

  // get fit parameters
  const ROOT::Fit::FitResult & result = fitter.Result();
  const double * parFit = result.GetParams();
  const double * parErr = result.GetErrors();

  // print final values
  auto finalChi2=myFit(parFit);
  std::cout << "==============" << std::endl;
  std::cout << "Final results:" << std::endl;  
  std::cout << "--------------" << std::endl;
  result.Print(std::cout);
  std::cout << "--------------" << std::endl;
  result.PrintCovMatrix(std::cout);
  std::cout << "--------------" << std::endl;
  std::cout << "CHI2 evaluated at FINAL point = " << finalChi2
	    << ",  fitter.Result().Chi2() = " << result.Chi2()
	    << ",  fitter.Result.MinFcnValue() = " << result.MinFcnValue()
	    << std::endl;
  std::cout << "==============" << std::endl;

  ///////// DEBUG
  myFit.plotQvalueFits("afterFit");
  ///////// DEBUG

  return result.MinFcnValue();
}
