#include "CommonDefinitions.h"
#ifdef WITH_GET
#include <iostream>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>
#include "colorText.h"
#include "ConfigManager.h"
#include "TFile.h"
#include "GeometryTPC.h"
#include "EventSourceGRAW.h"
#include "EventSourceMultiGRAW.h"
#include "RawSignal_tree_analysis.h"
#include "RunIdParser.h"

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
	auto clusterEnable = aConfig.get<bool>("clusterEnable");
	auto clusterThreshold = (clusterEnable ? aConfig.get<float>("clusterThreshold") : 0);
	auto clusterDeltaStrips = (clusterEnable ? aConfig.get<unsigned int>("clusterDeltaStrips") : 0);
	auto clusterDeltaTimeCells = (clusterEnable ? aConfig.get<unsigned int>("clusterDeltaTimeCells") : 0);
	auto singleAsadGrawFile = aConfig.get<bool>("singleAsadGrawFile"); // true = multi-GRAW mode
	auto frameLoadRange = aConfig.get<unsigned int>("frameLoadRange"); // used in single-GRAW mode only
	auto removePedestal = aConfig.get<bool>("removePedestal");

	std::cout << _endl_ << "analyzeRawEvents: Parameter settings: " << _endl_ << _endl_
		<< "Data file(s)             = " << dataFileName << _endl_
		<< "TPC geometry file        = " << geometryFileName << _endl_
		<< "Output file              = " << outputFileName << _endl_
		<< "Cluster enable           = " << clusterEnable << _endl_
		<< "Cluster threshold        = " << clusterThreshold << _endl_
		<< "Cluster delta strips     = " << clusterDeltaStrips << _endl_
		<< "Cluster delta time cells = " << clusterDeltaTimeCells << _endl_
		<< "Frame load range         = " << frameLoadRange << _endl_
		<< "Multi-GRAW mode          = " << singleAsadGrawFile << _endl_
		<< "Pedestal removal enable  = " << removePedestal << _endl_;

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

	// DEBUG - parsing RunId from file name
	//  auto id = RunIdParser(*(fileNameList.begin()));
	//  std::cout << "Parsing 1st file name from the list: " << *(fileNameList.begin())
	//	    << ": run=" << id.runId() << ", chunk=" << id.fileId() << ", cobo=" << id.CoBoId() << ", asad=" << id.AsAdId() << _endl_;
	auto id = RunIdParser(dataFileName);
	std::cout << "Parsing whole file name list: " << dataFileName
		<< ": run=" << id.runId() << ", chunk=" << id.fileId() << ", cobo=" << id.CoBoId() << ", asad=" << id.AsAdId() << _endl_;
	// DEBUG

	// initialize pedestal removal parameters for EventSource
	dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setRemovePedestal(removePedestal);
	if (removePedestal) {
		if (aConfig.find("pedestal") != aConfig.not_found()) {
			dynamic_cast<EventSourceGRAW*>(myEventSource.get())->configurePedestal(aConfig.find("pedestal")->second);
		}
		else {
			std::cerr << _endl_
				<< __FUNCTION__ << KRED << ": Some pedestal configuration options are missing!" << RST << _endl_ << _endl_;

			exit(1);
		}
	}

	// initialize RawSignalAnalysis
	ClusterConfig myClusterConfig;
	myClusterConfig.clusterEnable = clusterEnable;
	myClusterConfig.clusterThreshold = clusterThreshold;
	myClusterConfig.clusterDeltaStrips = clusterDeltaStrips;
	myClusterConfig.clusterDeltaTimeCells = clusterDeltaTimeCells;
	RawSignal_tree_analysis myAnalysis(myEventSource->getGeometry(), myClusterConfig); //dynamic_cast<EventSourceGRAW*>(myEventSource.get())->getGeometry());

	// loop over ALL events
	Long64_t currentEventIdx = -1;
	bool isFirst = true; // flag to indicate first event for time period / rate calculations

	////// DEBUG
	//  Long64_t counter=0;
	////// DEBUG

	do {
		// load first event
		if (currentEventIdx == -1) {
			myEventSource->loadFileEntry(0);
		}

		std::cout << "EventInfo: " << myEventSource->getCurrentEvent()->GetEventInfo() << _endl_;

		// fill statistical histograms per run (before & after user-defined cuts)
		myAnalysis.fillTree(myEventSource->getCurrentEvent(), isFirst);

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

#include "colorText.h"
#include <iostream>

int main() {
	std::cout << KRED << "TPCReco was compiled without GET libraries." << RST
		<< " This application requires GET libraries." << _endl_;
	return -1;
}
#endif
