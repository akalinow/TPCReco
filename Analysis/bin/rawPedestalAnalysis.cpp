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
#include "TPCReco/Pedestal_analysis.h"
#include "TPCReco/RunIdParser.h"

void analyzeRawEvents(const boost::property_tree::ptree &aConfig);

boost::program_options::variables_map parseCmdLineArgs(int argc, char **argv){

  boost::program_options::options_description cmdLineOptDesc("Allowed command line options");

  cmdLineOptDesc.add_options()
    ("help", "produce help message")
    ("geometryFile",  boost::program_options::value<std::string>(), "string - path to TPC geometry file")
    ("dataFile",  boost::program_options::value<std::string>(), "string - path to raw data file in single-GRAW mode (or list of comma-separated raw data files in multi-GRAW mode)")
    ("frameLoadRange", boost::program_options::value<unsigned int>(), "int - maximal number of frames to be read by event builder in single-GRAW mode")
    ("singleAsadGrawFile", boost::program_options::bool_switch()->default_value(false), "flag indicating multi-GRAW mode (default=FALSE)")
    ("outputFile", boost::program_options::value<std::string>(), "string - path to the output ROOT file")
    ("maxNevents", boost::program_options::value<unsigned int>()->default_value(0), "int - number of events to process");
  
  boost::program_options::variables_map varMap;

  try {
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cmdLineOptDesc), varMap);
    if (varMap.count("help")) {
      std::cout << std::endl
		<< "rawPedestalAnalysis config.json [options]" << std::endl << std::endl;
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
	      << "rawPedestalAnalysis config.json [options]" << std::endl << std::endl;
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
     tree.find("singleAsadGrawFile")==tree.not_found() ||
     (tree.find("singleAsadGrawFile")!=tree.not_found() &&
      tree.find("frameLoadRange")==tree.not_found()) ||
     tree.find("maxNevents")==tree.not_found()
     ) {
    std::cerr << std::endl
	      << __FUNCTION__ << KRED << ": Some configuration options are missing!" << RST << std::endl << std::endl;
    std::cout << "dataFile: " << tree.count("dataFile") << std::endl;
    std::cout << "geometryFile: " << tree.count("geometryFile") << std::endl;
    std::cout << "outputFile: " << tree.count("outputFile") << std::endl;
    std::cout << "singleAsadGrawFile: " << tree.count("singleAsadGrawFile") << std::endl;
    std::cout << "frameLoadRange: " << tree.count("frameLoadRange") << std::endl;
    std::cout << "maxNevents:" << tree.count("maxNevents") << std::endl;
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
  auto singleAsadGrawFile = aConfig.get<bool>("singleAsadGrawFile"); // true = multi-GRAW mode
  auto frameLoadRange = aConfig.get<unsigned int>("frameLoadRange"); // used in single-GRAW mode only
  auto maxNevents = aConfig.get<unsigned int>("maxNevents");

  std::cout << std::endl << "analyzeRawEvents: Parameter settings: " << std::endl << std::endl
	    << "Data file(s)             = " << dataFileName << std::endl
	    << "TPC geometry file        = " << geometryFileName << std::endl
	    << "Output file              = " << outputFileName << std::endl
	    << "Frame load range         = " << frameLoadRange << std::endl
	    << "Multi-GRAW mode          = " << singleAsadGrawFile << std::endl
	    << "Max. events to precess   = " << maxNevents << " (0=all)" << std::endl;
    
  if (dataFileName.find(".graw") == std::string::npos ||
      geometryFileName.find(".dat") == std::string::npos ||
      outputFileName.find(".root") == std::string::npos) {
    std::cerr << __FUNCTION__ << KRED << ": Wrong input argument(s)." << std::endl
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

  // initialize EventSource  
  std::shared_ptr<EventSourceBase> myEventSource;
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

  // DEBUG - parsing RunId from file name
  auto id = RunIdParser(dataFileName);
  std::cout << "Parsing whole file name list: " << dataFileName
	    << ": run=" << id.runId() << ", chunk=" << id.fileId() << ", cobo=" << id.CoBoId() << ", asad=" << id.AsAdId() << std::endl;
  // DEBUG
  
  // initialize pedestal removal parameters for EventSource
  if(aConfig.find("pedestal")!=aConfig.not_found()) {
    dynamic_cast<EventSourceGRAW*>(myEventSource.get())->configurePedestal(aConfig.find("pedestal")->second);
  }
  else {
    std::cerr << std::endl
	      << __FUNCTION__ << KRED << ": Some pedestal configuration options are missing!" << RST << std::endl << std::endl;
    
    exit(1);
  }

  // initialize Pedestal_analysis
  Pedestal_analysis myAnalysis(dynamic_cast<EventSourceGRAW*>(myEventSource.get()), outputFileName);

  // loop over ALL events
  Long64_t currentEventIdx=-1;
  Long64_t counter=0;

  do {
    // load first event
    if(currentEventIdx==-1) {
      myEventSource->loadFileEntry(0);
    }

    std::cout << "EventInfo: " << myEventSource->getCurrentEvent()->GetEventInfo()<<std::endl;

    // fill statistics per event and per run
    myAnalysis.fillHistos();
    
    std::cout << maxNevents << " " << counter << std::endl;
    if(maxNevents && maxNevents==++counter) break;

    // load next event (if any)
    currentEventIdx=myEventSource->currentEventNumber();
    myEventSource->getNextEvent();
  }
  while(currentEventIdx!=(Long64_t)myEventSource->currentEventNumber());

  // write histograms to ROOTFILE
  // myAnalysis.finalize();
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
