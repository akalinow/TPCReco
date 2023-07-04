#include "TPCReco/CommonDefinitions.h"
#ifdef WITH_GET_DISABLED_BY_AK
#include <iostream>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>
#include "TPCReco/colorText.h"
#include <TFile.h>
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/EventSourceGRAW.h"
#include "TPCReco/EventSourceMultiGRAW.h"
#include "TPCReco/RawSignal_tree_analysis.h"
#include "TPCReco/RunIdParser.h"
#include "TPCReco/ConfigManager.h"

void analyzeRawEvents(const boost::property_tree::ptree& aConfig);
////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
	ConfigManager cm;
	boost::property_tree::ptree tree = cm.getConfig(argc,argv);
	if (argc < 2) {
		std::cout << _endl_
			<< "rawSignalAnalysis config.json [options]" << _endl_ << _endl_;
		return 0;
	}
	else {
		std::cout << "Using configFileName: " << argv[1] << _endl_;
		boost::property_tree::read_json(argv[1], tree);
	}

	// start analysis job
	analyzeRawEvents(tree);
	return 0;
}

void analyzeRawEvents(const boost::property_tree::ptree& aConfig) {

  auto geometryFileName = aConfig.get<std::string>("geometryFile");
  auto dataFileName = aConfig.get<std::string>("dataFile");
  auto outputFileName = aConfig.get<std::string>("outputFile");
  auto clusterEnable = aConfig.get<bool>("hitFilter.recoClusterEnable");
  auto clusterThreshold = ( clusterEnable ? aConfig.get<float>("hitFilter.recoClusterThreshold") : 0 );
  auto clusterDeltaStrips = ( clusterEnable ? aConfig.get<unsigned int>("hitFilter.recoClusterDeltaStrips") : 0 );
  auto clusterDeltaTimeCells = ( clusterEnable ? aConfig.get<unsigned int>("hitFilter.recoClusterDeltaTimeCells") : 0 );
  auto singleAsadGrawFile = aConfig.get<bool>("singleAsadGrawFile"); // true = multi-GRAW mode
  auto frameLoadRange = aConfig.get<unsigned int>("frameLoadRange"); // used in single-GRAW mode only
  auto removePedestal = aConfig.get<bool>("removePedestal");
  auto maxNevents = aConfig.get<unsigned int>("maxNevents");

  std::cout << std::endl << "analyzeRawEvents: Parameter settings: " << std::endl << std::endl
	    << "Data file(s)             = " << dataFileName << std::endl
	    << "TPC geometry file        = " << geometryFileName << std::endl
	    << "Output file              = " << outputFileName << std::endl
	    << "recoCluster enable           = " << clusterEnable << std::endl
	    << "recoCluster threshold        = " << clusterThreshold << std::endl
	    << "recoCluster delta strips     = " << clusterDeltaStrips << std::endl
	    << "recoCluster delta time cells = " << clusterDeltaTimeCells << std::endl
	    << "Frame load range         = " << frameLoadRange << std::endl
	    << "Multi-GRAW mode          = " << singleAsadGrawFile << std::endl
	    << "Pedestal removal enable  = " << removePedestal << std::endl
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

	if (dataFileName.find(".graw") == std::string::npos ||
		geometryFileName.find(".dat") == std::string::npos ||
		outputFileName.find(".root") == std::string::npos) {
		std::cerr << __FUNCTION__ << KRED << ": Wrong input argument(s)." << _endl_
			<< "Check that GRAW file(s) and geometry file are correct." << _endl_
			<< "The output ROOT file must not be present."
			<< RST << _endl_;
		exit(1);
	}

	const char del = ','; // delimiter character
	std::set<std::string> fileNameList; // list of unique strings
	std::stringstream sstream(dataFileName);
	std::string fileName;
	while (std::getline(sstream, fileName, del)) {
		if (fileName.size() > 0) fileNameList.insert(fileName);
	};

	// initialize EventSource  
	std::shared_ptr<EventSourceBase> myEventSource;
	if (singleAsadGrawFile) {
		myEventSource = std::make_shared<EventSourceMultiGRAW>(geometryFileName);
		unsigned int AsadNboards = dynamic_cast<EventSourceGRAW*>(myEventSource.get())->getGeometry()->GetAsadNboards();
		if (fileNameList.size() > AsadNboards) {
			std::cerr << __FUNCTION__ << KRED << ": Provided too many GRAW files. Expected up to " << AsadNboards << RST << _endl_;
			exit(1);
		}
	}
	else {
		myEventSource = std::make_shared<EventSourceGRAW>(geometryFileName);
		dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setFrameLoadRange(frameLoadRange);
		if (fileNameList.size() > 1) {
			std::cerr << __FUNCTION__ << KRED << ": Provided too many GRAW files. Expected 1." << RST << _endl_;
			exit(1);
		}
	}

	myEventSource->loadDataFile(dataFileName);
	std::cout << "File with " << myEventSource->numberOfEntries() << " frames loaded."
		<< _endl_;

  // initialize RawSignalAnalysis
  ClusterConfig myClusterConfig;
  myClusterConfig.clusterEnable = clusterEnable;
  myClusterConfig.clusterThreshold = clusterThreshold;
  myClusterConfig.clusterDeltaStrips = clusterDeltaStrips;
  myClusterConfig.clusterDeltaTimeCells = clusterDeltaTimeCells;
  RawSignal_tree_analysis myAnalysis(myEventSource->getGeometry(), myClusterConfig, outputFileName); //dynamic_cast<EventSourceGRAW*>(myEventSource.get())->getGeometry());

	// initialize pedestal removal parameters for EventSource
	dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setRemovePedestal(removePedestal);
	if (removePedestal) {
		if (aConfig.find("pedestal") != aConfig.not_found()) {
			dynamic_cast<EventSourceGRAW*>(myEventSource.get())->configurePedestal(aConfig.find("pedestal")->second);
		}
		else {
			std::cerr << _endl_
				<< __FUNCTION__ << KRED << ": Some pedestal configuration options are missing!" << RST << _endl_ << _endl_;
		}

  ////// DEBUG
  Long64_t counter=0;
  ////// DEBUG

	// loop over ALL events
	Long64_t currentEventIdx = -1;
	bool isFirst = true; // flag to indicate first event for time period / rate calculations
	
  while(currentEventIdx!=(Long64_t)myEventSource->currentEventNumber());

		std::cout << "EventInfo: " << myEventSource->getCurrentEvent()->GetEventInfo() << _endl_;

		// fill statistical histograms per run (before & after user-defined cuts)
		myAnalysis.fillTree(myEventSource->getCurrentEvent(), isFirst);
		
		if(maxNevents && maxNevents==++counter ) break;

		// load next event (if any)
		currentEventIdx = myEventSource->currentEventNumber();
		myEventSource->getNextEvent();

		////// DEBUG
		//    if(++counter==100) break;
		////// DEBUG
	} while (currentEventIdx != (Long64_t)myEventSource->currentEventNumber());

	// write histograms to ROOTFILE
	//  myAnalysis.finalize();
}

#else

#include "TPCReco/colorText.h"
#include <iostream>

int main() {
	std::cout << KRED << "TPCReco was compiled without GET libraries." << RST
		<< " This application requires GET libraries." << _endl_;
	return -1;
}
#endif
