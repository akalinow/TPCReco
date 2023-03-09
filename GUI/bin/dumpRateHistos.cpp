#ifdef WITH_GET
#include <iostream>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>
#include "TPCReco/colorText.h"
#include <TFile.h>
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/EventSourceGRAW.h"
#include "TPCReco/EventSourceMultiGRAW.h"
#include "TPCReco/DotFinder.h"

void analyzeRawEvents(const boost::property_tree::ptree &aConfig);

boost::program_options::variables_map parseCmdLineArgs(int argc, char **argv){

  boost::program_options::options_description cmdLineOptDesc("Allowed command line options");

  cmdLineOptDesc.add_options()
    ("help", "produce help message")
    ("geometryFile",  boost::program_options::value<std::string>(), "string - path to TPC geometry file")
    ("dataFile",  boost::program_options::value<std::string>(), "string - path to raw data file in single-GRAW mode (or list of comma-separated raw data files in multi-GRAW mode)")
    ("frameLoadRange", boost::program_options::value<unsigned int>(), "int - maximal number of frames to be read by event builder in single-GRAW mode")
    ("singleAsadGrawFile", boost::program_options::bool_switch()->default_value(false), "flag indicating multi-GRAW mode")
    ("hitThr", boost::program_options::value<unsigned int>(), "int - minimal hit charge after pedestal subtraction [ADC units]")
    ("totalChargeThr", boost::program_options::value<unsigned int>(), "int - minimal event total charge after pedestal subtraction [ADC units]")
    ("matchRadiusInMM", boost::program_options::value<float>(), "float - matching radius for strips and time cells from different U/V/W directions [mm]")
    ("outputFile", boost::program_options::value<std::string>(), "string - path to the output ROOT file");

  boost::program_options::variables_map varMap;

  try {
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cmdLineOptDesc), varMap);
    if (varMap.count("help")) {
      std::cout << std::endl
		<< "dumpRateHistos config.json [options]" << std::endl << std::endl;
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

  boost::program_options::variables_map varMap = parseCmdLineArgs(argc, argv);
  boost::property_tree::ptree tree;
  if(argc<2){
    std::cout << std::endl
	      << "dumpRateHistos config.json [options]" << std::endl << std::endl;
    return 0;
  }
  else {
    std::cout<<"Using configFileName: "<<argv[1]<<std::endl;
    boost::property_tree::read_json(argv[1], tree);
  }

  // optional overrides of the JSON config file
  if (varMap.count("outputFile")) {
    tree.put("outputFile",varMap["outputFile"].as<std::string>());
  }
  if (varMap.count("geometryFile")) {
    tree.put("geometryFile",varMap["geometryFile"].as<std::string>());
  }
  if (varMap.count("dataFile")) {
    tree.put("dataFile",varMap["dataFile"].as<std::string>());
  }
  if(varMap.count("hitThr")){
    tree.put("hitThr", varMap["hitThr"].as<unsigned int>());
  }
  if(varMap.count("totalChargeThr")){
    tree.put("totalChargeThr", varMap["totalChargeThr"].as<unsigned int>());
  }
  if(varMap.count("matchRadiusInMM")){
    tree.put("matchRadiusInMM", varMap["matchRadiusInMM"].as<float>());
  }
  if( (tree.find("singleAsadGrawFile")==tree.not_found() || // if not present in config JSON
       tree.get<bool>("singleAsadGrawFile")==false) && // or single-GRAW mode is FALSE
      varMap.count("singleAsadGrawFile")){ // then allow to override JSON settings
    tree.put("singleAsadGrawFile", varMap["singleAsadGrawFile"].as<bool>());
  }
  if( tree.get<bool>("singleAsadGrawFile")==false && // if in single-GRAW mode
      varMap.count("frameLoadRange")) { // then allow to override JSON settings
    tree.put("frameLoadRange", varMap["frameLoadRange"].as<unsigned int>());
  }

  //sanity checks
  if(tree.find("dataFile")==tree.not_found() ||
     tree.find("geometryFile")==tree.not_found() ||
     tree.find("outputFile")==tree.not_found() ||
     tree.find("hitThr")==tree.not_found() ||
     tree.find("totalChargeThr")==tree.not_found() ||
     tree.find("matchRadiusInMM")==tree.not_found()  ||
     tree.find("singleAsadGrawFile")==tree.not_found() ||
     (tree.find("singleAsadGrawFile")!=tree.not_found() &&
     tree.find("frameLoadRange")==tree.not_found())
) {
    std::cerr << std::endl
	      << __FUNCTION__ << KRED << ": Some configuration options are missing!" << RST << std::endl << std::endl;
    std::cout << "dataFile: " << tree.count("dataFile") << std::endl;
    std::cout << "geometryFile: " << tree.count("geometryFile") << std::endl;
    std::cout << "outputFile: " << tree.count("outputFile") << std::endl;
    std::cout << "hitThr: " << tree.count("hitThr") << std::endl;
    std::cout << "totalChargeThr: " << tree.count("totalChargeThr") << std::endl;
    std::cout << "matchRadiusInMM: " << tree.count("matchRadiusInMM") << std::endl;
    std::cout << "singleAsadGrawFile: " << tree.count("singleAsadGrawFile") << std::endl;
    std::cout << "frameLoadRange: " << tree.count("frameLoadRange") << std::endl;
    exit(1);
  }

  // start analysis job
  analyzeRawEvents(tree);
  return 0;
}

void analyzeRawEvents(const boost::property_tree::ptree &aConfig){
  auto geometryFileName = aConfig.get<std::string>("geometryFile");
  auto dataFileName = aConfig.get<std::string>("dataFile");
  auto outputFileName = aConfig.get<std::string>("outputFile");
  auto hitThr = aConfig.get<unsigned int>("hitThr");
  auto totalChargeThr = aConfig.get<unsigned int>("totalChargeThr");
  auto matchRadiusInMM = aConfig.get<float>("matchRadiusInMM");
  auto singleAsadGrawFile = aConfig.get<bool>("singleAsadGrawFile"); // true = multi-GRAW mode
  auto frameLoadRange = aConfig.get<unsigned int>("frameLoadRange"); // used in single-GRAW mode only

  std::cout << std::endl << "DotFinder: Parameter settings: " << std::endl << std::endl
	    << "Data file(s)           = " << dataFileName << std::endl
	    << "TPC geometry file      = " << geometryFileName << std::endl
	    << "Output file            = " << outputFileName << std::endl
	    << "Hit threshold          = " << hitThr << std::endl
	    << "Total charge threshold = " << totalChargeThr << std::endl
	    << "Matching radius        = " << matchRadiusInMM << " mm" << std::endl
	    << "Frame load range       = " << frameLoadRange << std::endl
	    << "Multi-GRAW mode        = " << singleAsadGrawFile << std::endl;
    
  if (dataFileName.find(".graw") == std::string::npos ||
      geometryFileName.find(".dat") == std::string::npos ||
      outputFileName.find(".root") == std::string::npos) {
    std::cerr << __FUNCTION__ << KRED << ": Wronge input argument(s)." << std::endl
	      << "Check that GRAW file(s) and geometry file are correct." << std::endl
	      << "The output ROOT file must not be present."
              << RST << std::endl;
    exit(1);
  }

  const char del = ','; // delimiter character
  std::set<std::string> fileNameList; // list of unique strings
  std::stringstream sstream(dataFileName);
  std::string fileName;
  while (std::getline(sstream, fileName, del)) {
    if(fileName.size()>0) fileNameList.insert(fileName);
  };

  std::shared_ptr<EventSourceBase> myEventSource;
  // std::shared_ptr<EventSourceGRAW> myEventSource;

  if(singleAsadGrawFile) {
    myEventSource = std::make_shared<EventSourceMultiGRAW>(geometryFileName);
    unsigned int AsadNboards=dynamic_cast<EventSourceGRAW*>(myEventSource.get())->getGeometry()->GetAsadNboards();
    if (fileNameList.size()>AsadNboards) {
      std::cerr << __FUNCTION__ << KRED << ": Provided too many GRAW files. Expected up to " << AsadNboards << RST << std::endl;
      exit(1);
    }
  } else {
    myEventSource = std::make_shared<EventSourceGRAW>(geometryFileName);
    dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setFrameLoadRange(frameLoadRange);
    if (fileNameList.size()>1) {
      std::cerr << __FUNCTION__ << KRED << ": Provided too many GRAW files. Expected 1." << RST << std::endl;
      exit(1);
    }
  }

  myEventSource->loadDataFile(dataFileName);
  std::cout << "File with " << myEventSource->numberOfEntries() << " frames loaded."
	    << std::endl;

  DotFinder myDotFinder;
  myDotFinder.initializeDotFinder(hitThr, totalChargeThr, matchRadiusInMM, outputFileName);

  // loop over ALL events and fill various histograms per RUN
  Long64_t currentEventIdx=-1;

  ////// DEBUG
  //  Long64_t counter=0;
  ////// DEBUG

  do {
    // load first event
    if(currentEventIdx==-1) {
      myEventSource->loadFileEntry(0);
    }

    // fill statistical histograms per run (before & after user-defined cuts)
    std::cout << "EventID: " << myEventSource->currentEventNumber() << std::endl;
    myDotFinder.runDotFinder(myEventSource->getCurrentEvent());
    
    // load next event (if any)
    currentEventIdx=myEventSource->currentEventNumber();
    myEventSource->getNextEvent();

    ////// DEBUG
    //    if(++counter==10) break;
    ////// DEBUG
  }
  while(currentEventIdx!=(Long64_t)myEventSource->currentEventNumber());

  // write histograms to ROOTFILE
  myDotFinder.finalizeDotFinder();
}

#else

#include "TPCReco/colorText.h"
#include <iostream>

int main(){
  std::cout<<KRED<<"TPCReco was compiled without GET libraries."<<RST
	    <<" This application requires GET libraries."<<std::endl;
  return -1;
}
#endif
