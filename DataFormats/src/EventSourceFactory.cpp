#include "EventSourceFactory.h"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <thread>

#include "DirectoryWatch.h"

#include <TApplication.h>
#include "colorText.h"

#include <TSystem.h>
#include <TObjArray.h> 
#include <TObjString.h>
#include <TStyle.h>
#include <TString.h>
#include <TFrame.h>
#include <TVirtualX.h>
#include <TImage.h>

#include <TH2D.h>
#include <TH3D.h>
#include <TLatex.h>
#include <TProfile.h>

#include <boost/filesystem.hpp>

//#define WITH_GET 1
#ifdef WITH_GET
#include "EventSourceGRAW.h"
#include "EventSourceMultiGRAW.h"
#endif
#include "EventSourceROOT.h"
#include "EventSourceMC.h"

inline std::shared_ptr<EventSourceBase> EventSourceFactory::makeEventSourceObject(boost::property_tree::ptree myConfig, Modes& myWorkMode) {
	bool placeholder_bool_var;
	return makeEventSourceObject(myConfig, myWorkMode, placeholder_bool_var);
}

inline std::shared_ptr<EventSourceBase> EventSourceFactory::makeEventSourceObject(boost::property_tree::ptree myConfig, Modes& myWorkMode, bool& useFileWatch) {
	useFileWatch = false;
	std::string dataFileName = myConfig.get("dataFile", "");
	std::string geometryFileName = myConfig.get("geometryFile", "");

	std::shared_ptr<EventSourceBase> myEventSource;

	if (dataFileName.empty() || geometryFileName.empty()) {
		std::cerr << "No data or geometry file path provided." << _endl_;
		exit(1);
		return decltype(myEventSource)();
	}

	// parse dataFile string for comma separated files
	std::vector<boost::filesystem::path>
		dataFileVec,
		dataFilePaths;
	//  vector<std::string> basenameVec;
	//  vector<std::string> dirnameVec;
	size_t pos = 0;
	std::string token;
	while ((pos = dataFileName.find(",")) != std::string::npos) { //split paths from a string with "," delimiter
		token = dataFileName.substr(0, pos);
		dataFilePaths.push_back(token);
		dataFileName.erase(0, pos + 1);
	}

#ifdef WITH_GET
	bool all_graw = false;
#endif
	for (auto filePath : dataFilePaths) {
		if (boost::filesystem::exists(filePath)) {
			if (filePath.string().find("_MC_") != std::string::npos) {
				dataFileVec.push_back(filePath);
			}
		}
		else {
			std::cerr << KRED << "Invalid data path. No such file or directory: " << RST << filePath << _endl_;
			exit(1);
		}
#ifdef WITH_GET
		all_graw = boost::filesystem::is_regular_file(filePath) && filePath.string().find(".graw") != std::string::npos;
#endif
		dataFileVec.push_back(filePath);
	}

	std::cout << "dataFileVec.size(): " << dataFileVec.size()
		<< " (boost::filesystem::is_regular_file(dataFileVec[0])): " << boost::filesystem::is_regular_file(dataFileVec[0])
		<< "dataFileName.find(_MC_)!=std::string::npos: " << (dataFileName.find("_MC_") != std::string::npos)
		<< _endl_;


	if (dataFileVec.size() == 1 && boost::filesystem::is_regular_file(dataFileVec[0]) && dataFileName.find(".root") != std::string::npos) {
		myWorkMode = M_OFFLINE_ROOT_MODE;
		myEventSource = std::make_shared<EventSourceROOT>(geometryFileName);
	}
	else if (dataFileVec.size() == 1 && dataFileName.find("_MC_") != std::string::npos) {
		myWorkMode = M_OFFLINE_MC_MODE;
		myEventSource = std::make_shared<EventSourceMC>(geometryFileName);
	}

#ifdef WITH_GET
	else if (all_graw) { //ROOT_file_check && dataFileName.find(".graw")!=std::string::npos){

		myWorkMode = M_OFFLINE_GRAW_MODE;
		if (myConfig.find("singleAsadGrawFile") != myConfig.not_found()) {
			bool singleAsadGrawFile = myConfig.get<bool>("singleAsadGrawFile");
			if (singleAsadGrawFile) {
				myWorkMode = M_OFFLINE_NGRAW_MODE;
			}
		}
		switch (myWorkMode) {
		case M_OFFLINE_GRAW_MODE:
			myEventSource = std::make_shared<EventSourceGRAW>(geometryFileName);
			dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setFrameLoadRange(myConfig.get("frameLoadRange", 100));
			if (dataFileVec.size() > 1) {
				std::cerr << KRED << "Provided too many GRAW files. Expected 1. dataFile: " << RST << dataFileName << _endl_;
				return decltype(myEventSource)();
			}
			break;
		case M_OFFLINE_NGRAW_MODE:
			myEventSource = std::make_shared<EventSourceMultiGRAW>(geometryFileName);
			{
				unsigned int AsadNboards = dynamic_cast<EventSourceGRAW*>(myEventSource.get())->getGeometry()->GetAsadNboards();
				if (dataFileVec.size() > AsadNboards) {
					std::cerr << KRED << "Provided too many GRAW files. Expected up to " << AsadNboards << ".dataFile: " << RST << dataFileName << _endl_;
					return decltype(myEventSource)();
				}
			}
			break;
		default:;
		};

	}
	else if (dataFileVec.size() == 1 && boost::filesystem::is_directory(dataFileVec[0])) {
		useFileWatch = true;
		myWorkMode = M_ONLINE_GRAW_MODE;
		if (myConfig.find("singleAsadGrawFile") != myConfig.not_found()) {
			bool singleAsadGrawFile = myConfig.get<bool>("singleAsadGrawFile");
			if (singleAsadGrawFile) {
				myWorkMode = M_ONLINE_NGRAW_MODE;
			}
		}
		switch (myWorkMode) {
		case M_ONLINE_GRAW_MODE:
			myEventSource = std::make_shared<EventSourceGRAW>(geometryFileName);
			dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setFrameLoadRange(myConfig.get("frameLoadRange", 10));
			break;
		case M_ONLINE_NGRAW_MODE:
			myEventSource = std::make_shared<EventSourceMultiGRAW>(geometryFileName);
			break;
		default:;
		};
	}
	if (myConfig.find("removePedestal") != myConfig.not_found() && myEventSource.get()) {
		bool removePedestal = myConfig.get<bool>("removePedestal");
		EventSourceGRAW* aGrawEventSrc = dynamic_cast<EventSourceGRAW*>(myEventSource.get());
		if (aGrawEventSrc) aGrawEventSrc->setRemovePedestal(removePedestal);
	}
	if (myConfig.find("pedestal") != myConfig.not_found() && myEventSource.get()) {
		dynamic_cast<EventSourceGRAW*>(myEventSource.get())->configurePedestal(myConfig.find("pedestal")->second);
	}
#endif
	else if (!myEventSource) {
		std::cerr << KRED << "Input source not known. DataFile: " << RST << dataFileName << _endl_;
#ifndef WITH_GET
		std::cerr << KRED << "and GRAW libriaries not set." << RST << _endl_;
#endif
		exit(0);
		return decltype(myEventSource)();
	}
	if (myWorkMode != M_ONLINE_GRAW_MODE && myWorkMode != M_ONLINE_NGRAW_MODE) {
		myEventSource->loadDataFile(dataFileName);
		myEventSource->loadFileEntry(0);
	}
	return myEventSource;
}