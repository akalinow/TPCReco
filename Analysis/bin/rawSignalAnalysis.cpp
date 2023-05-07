#include <iostream>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>
#include "TPCReco/colorText.h"
#include <TFile.h>
#include "TPCReco/GeometryTPC.h"
#ifdef WITH_GET
#include "TPCReco/EventSourceGRAW.h"
#include "TPCReco/EventSourceMultiGRAW.h"
#endif
#include "TPCReco/EventSourceROOT.h"
#include "TPCReco/RawSignal_tree_analysis.h"
#include "TPCReco/RunIdParser.h"

int analyzeRawEvents(const boost::property_tree::ptree aConfig);

boost::program_options::variables_map parseCmdLineArgs(int argc, char **argv){

  boost::program_options::options_description cmdLineOptDesc("Allowed command line options");

  cmdLineOptDesc.add_options()
    ("help", "produce help message")
    ("geometryFile",  boost::program_options::value<std::string>(), "string - path to TPC geometry file")
#ifdef WITH_GET
    ("dataFile",  boost::program_options::value<std::string>(), "string - path to a raw data file in ROOT/GRAW format (or list of comma-separated files in multi-GRAW mode) ")
    ("frameLoadRange", boost::program_options::value<unsigned int>(), "int - maximal number of frames to be read by event builder in single-GRAW mode")
    ("singleAsadGrawFile", boost::program_options::bool_switch()->default_value(false), "bool - flag indicating multi-GRAW mode (default=FALSE)")
    ("removePedestal",  boost::program_options::value<bool>(), "bool - flag to control pedestal removal. Overrides the value from config file.")
#else
    ("dataFile",  boost::program_options::value<std::string>(), "string - path to a raw data file in ROOT format")
#endif
    ("recoClusterEnable",  boost::program_options::value<bool>(), "bool - flag to enable clustering")
    ("recoClusterThreshold",  boost::program_options::value<float>(), "float - ADC threshold above pedestal used for clustering")
    ("recoClusterDeltaStrips",  boost::program_options::value<int>(), "int - envelope in strip units around seed hits for clustering")
    ("recoClusterDeltaTimeCells",  boost::program_options::value<int>(), "int - envelope in time cell units around seed hits for clustering")
    ("outputFile", boost::program_options::value<std::string>(), "string - path to the output ROOT file")
    ("maxNevents", boost::program_options::value<unsigned int>()->default_value(0), "int - number of events to process");

  boost::program_options::variables_map varMap;

  try {
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cmdLineOptDesc), varMap);
    if (varMap.count("help")) {
      std::cout << std::endl
		<< "rawSignalAnalysis config.json [options]" << std::endl << std::endl;
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
	      << "rawSignalAnalysis config.json [options]" << std::endl << std::endl;
    return 0;
  }
  else {
    std::cout<<"Using configFileName: "<<argv[1]<<std::endl;
    boost::property_tree::read_json(argv[1], tree);
  }

  // optional overrides of the JSON config file
  if (varMap.count("outputFile")) {
    tree.put("outputFile", varMap["outputFile"].as<std::string>());
  }
  if (varMap.count("geometryFile")) {
    tree.put("geometryFile", varMap["geometryFile"].as<std::string>());
  }
  if (varMap.count("dataFile")) {
    tree.put("dataFile", varMap["dataFile"].as<std::string>());
  }
  if (varMap.count("maxNevents")) {
    tree.put("maxNevents", varMap["maxNevents"].as<unsigned int>());
  }
  if(varMap.count("recoClusterEnable")) {
    tree.put("hitFilter.recoClusterEnable", varMap["recoClusterEnable"].as<bool>());
    if(tree.get<bool>("hitFilter.recoClusterEnable")==false) { // skip threshold, delta-strip, delta-timecells when clustering is disabled
      tree.put("hitFilter.recoClusterThreshold", 0.0);
      tree.put("hitFilter.recoClusterDeltaStrips", 0);
      tree.put("hitFilter.recoClusterDeltaTimeCells", 0);
    } else { // look for threshold, delta-strip, delta-timecells only when clustering is enabled
      if(varMap.count("recoClusterThreshold")) {
	      tree.put("hitFilter.recoClusterThreshold", varMap["recoClusterThreshold"].as<float>());
      }
      if(varMap.count("recoClusterDeltaStrips")) {
	      tree.put("hitFilter.recoClusterDeltaStrips", varMap["recoClusterDeltaStrips"].as<int>());
      }
      if(varMap.count("recoClusterDeltaTimeCells")) {
	      tree.put("hitFilter.recoClusterDeltaTimeCells", varMap["recoClusterDeltaTimeCells"].as<int>());
      }
    }
  }
#ifdef WITH_GET
  if (varMap.count("removePedestal")) {
    tree.put("removePedestal", varMap["removePedestal"].as<bool>());
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
#endif

  //sanity checks
  if(tree.find("dataFile")==tree.not_found() ||
     tree.find("geometryFile")==tree.not_found() ||
     tree.find("outputFile")==tree.not_found() ||
#ifdef WITH_GET
     tree.find("singleAsadGrawFile")==tree.not_found() ||
     (tree.find("singleAsadGrawFile")!=tree.not_found() &&
      tree.find("frameLoadRange")==tree.not_found()) ||
     tree.find("removePedestal")==tree.not_found() ||
#endif
     tree.get_child("hitFilter").find("recoClusterEnable")==tree.not_found() ||
     tree.get_child("hitFilter").find("recoClusterThreshold")==tree.not_found() ||
     tree.get_child("hitFilter").find("recoClusterDeltaStrips")==tree.not_found() ||
     tree.get_child("hitFilter").find("recoClusterDeltaTimeCells")==tree.not_found() ||
     tree.find("maxNevents")==tree.not_found()
     ) {
    std::cerr << std::endl
	      << __FUNCTION__ << KRED << ": Some configuration options are missing!" << RST << std::endl << std::endl;
    std::cout << "dataFile: " << tree.count("dataFile") << std::endl;
    std::cout << "geometryFile: " << tree.count("geometryFile") << std::endl;
    std::cout << "outputFile: " << tree.count("outputFile") << std::endl;
    std::cout << "recoClusterEnable: " << tree.get_child("hitFilter").count("recoClusterEnable") << std::endl;
    std::cout << "recoClusterThreshold: " << tree.get_child("hitFilter").count("recoClusterThreshold") << std::endl;
    std::cout << "recoClusterDeltaStrips: " << tree.get_child("hitFilter").count("recoClusterDeltaStrips") << std::endl;
    std::cout << "recoClusterDeltaTimeCells: " << tree.get_child("hitFilter").count("recoClusterDeltaTimeCells") << std::endl;
#ifdef WITH_GET
    std::cout << "singleAsadGrawFile: " << tree.count("singleAsadGrawFile") << std::endl;
    std::cout << "frameLoadRange: " << tree.count("frameLoadRange") << std::endl;
    std::cout << "removePedestal: " << tree.count("removePedestal") << std::endl;
#endif
    std::cout << "maxNevents:" << tree.count("maxNevents") << std::endl;
    exit(1);
  }

  // start analysis job
  int nEventsProcessed = analyzeRawEvents(tree);
  return 0;
}

int analyzeRawEvents(const boost::property_tree::ptree aConfig){

  auto geometryFileName = aConfig.get<std::string>("geometryFile");
  auto dataFileName = aConfig.get<std::string>("dataFile");
  auto outputFileName = aConfig.get<std::string>("outputFile");
  auto clusterEnable = aConfig.get<bool>("hitFilter.recoClusterEnable");
  auto clusterThreshold = ( clusterEnable ? aConfig.get<float>("hitFilter.recoClusterThreshold") : 0 );
  auto clusterDeltaStrips = ( clusterEnable ? aConfig.get<unsigned int>("hitFilter.recoClusterDeltaStrips") : 0 );
  auto clusterDeltaTimeCells = ( clusterEnable ? aConfig.get<unsigned int>("hitFilter.recoClusterDeltaTimeCells") : 0 );
#ifdef WITH_GET
  auto singleAsadGrawFile = aConfig.get<bool>("singleAsadGrawFile"); // true = multi-GRAW mode
  auto frameLoadRange = aConfig.get<unsigned int>("frameLoadRange"); // used in single-GRAW mode only
  auto removePedestal = aConfig.get<bool>("removePedestal");
#endif
  auto maxNevents = aConfig.get<unsigned int>("maxNevents");

  std::cout << std::endl << "analyzeRawEvents: Parameter settings: " << std::endl << std::endl
	    << "Data file(s)             = " << dataFileName << std::endl
	    << "TPC geometry file        = " << geometryFileName << std::endl
	    << "Output file              = " << outputFileName << std::endl
	    << "recoCluster enable           = " << clusterEnable << std::endl
	    << "recoCluster threshold        = " << clusterThreshold << std::endl
	    << "recoCluster delta strips     = " << clusterDeltaStrips << std::endl
	    << "recoCluster delta time cells = " << clusterDeltaTimeCells << std::endl
#ifdef WITH_GET
	    << "Frame load range         = " << frameLoadRange << std::endl
	    << "Multi-GRAW mode          = " << singleAsadGrawFile << std::endl
	    << "Pedestal removal enable  = " << removePedestal << std::endl
#endif
	    << "Max. events to precess   = " << maxNevents << " (0=all)" << std::endl;

#ifdef WITH_GET
  if ((dataFileName.find(".graw") == std::string::npos && dataFileName.find(".root") == std::string::npos) ||
#else
  if (dataFileName.find(".root") == std::string::npos ||
#endif
      geometryFileName.find(".dat") == std::string::npos ||
      outputFileName.find(".root") == std::string::npos) {
    std::cerr << __FUNCTION__ << KRED << ": Wrong input argument(s)." << std::endl
#ifdef WITH_GET
	      << "Check that input ROOT/GRAW file(s) and geometry file are correct." << std::endl
#else
	      << "Check that input ROOT file and geometry file are correct." << std::endl
#endif
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

  // initialize EventSource  
  std::shared_ptr<EventSourceBase> myEventSource;
#ifdef WITH_GET
  if(dataFileName.find(".graw")!=std::string::npos && dataFileName.find(".root")==std::string::npos) {
    if(singleAsadGrawFile) {
      myEventSource = std::make_shared<EventSourceMultiGRAW>(geometryFileName);
      unsigned int AsadNboards=dynamic_cast<EventSourceGRAW*>(myEventSource.get())->getGeometry()->GetAsadNboards();
      if (fileNameList.size()>AsadNboards) {
	std::cerr << __FUNCTION__ << KRED << ": Provided too many input GRAW files. Expected up to " << AsadNboards << RST << std::endl;
	exit(1);
      }
    } else {
      myEventSource = std::make_shared<EventSourceGRAW>(geometryFileName);
      dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setFrameLoadRange(frameLoadRange);
      if (fileNameList.size()>1) {
	std::cerr << __FUNCTION__ << KRED << ": Provided too many input GRAW files. Expected 1." << RST << std::endl;
	exit(1);
      }
    }
  } else {
#endif
    if((dataFileName.find(".root")!=std::string::npos && dataFileName.find(".graw")!=std::string::npos) ||
       fileNameList.size()>1) {
      std::cerr << __FUNCTION__ << KRED << ": Provided too many input ROOT files. Expected 1." << RST << std::endl;
      exit(1);
    }

    myEventSource = std::make_shared<EventSourceROOT>(geometryFileName);
#ifdef WITH_GET
  }
#endif

  myEventSource->loadDataFile(dataFileName);
  std::cout << "File with " << myEventSource->numberOfEntries() << " frames loaded."
	    << std::endl;

#ifdef WITH_GET
  if(dataFileName.find(".graw")!=std::string::npos) {
    auto id = RunIdParser(dataFileName);
    std::cout << "Parsing list of file names: " << dataFileName
	      << ": run=" << id.runId() << ", chunk=" << id.fileId() << ", cobo=" << id.CoBoId() << ", asad=" << id.AsAdId() << std::endl;
    
    // initialize pedestal removal parameters for EventSource
    dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setRemovePedestal(removePedestal);
    if(removePedestal) {
      if(aConfig.find("pedestal")!=aConfig.not_found()) {
	dynamic_cast<EventSourceGRAW*>(myEventSource.get())->configurePedestal(aConfig.find("pedestal")->second);
      }
      else {
	std::cerr << std::endl
		  << __FUNCTION__ << KRED << ": Some pedestal configuration options are missing!" << RST << std::endl << std::endl;
	exit(1);
      }
    }
  }
#endif

  // initialize RawSignalAnalysis
  ClusterConfig myClusterConfig;
  myClusterConfig.clusterEnable = clusterEnable;
  myClusterConfig.clusterThreshold = clusterThreshold;
  myClusterConfig.clusterDeltaStrips = clusterDeltaStrips;
  myClusterConfig.clusterDeltaTimeCells = clusterDeltaTimeCells;
  RawSignal_tree_analysis myAnalysis(myEventSource->getGeometry(), myClusterConfig, outputFileName);

  // loop over ALL events
  Long64_t currentEventIdx=-1;
  bool isFirst=true; // flag to indicate first event for time period / rate calculations
  Long64_t counter=0;

  do {
    // load first event
    if(currentEventIdx==-1) {
      myEventSource->loadFileEntry(0);
    }

    std::cout << __FUNCTION__ << ": " << myEventSource->getCurrentEvent()->GetEventInfo() << std::endl;

    // fill diagnostic tree with raw signal properties
    myAnalysis.fillTree(myEventSource->getCurrentEvent(), isFirst);

    if(maxNevents && maxNevents==++counter ) break;

    // load next event (if any)
    currentEventIdx=myEventSource->currentEventNumber();
    myEventSource->getNextEvent();
  }
  while(currentEventIdx!=(Long64_t)myEventSource->currentEventNumber());

  // write histograms to ROOTFILE
  //  myAnalysis.finalize();

  return counter;
}
