#pragma once
#ifndef EventSourceFactory_h
#define EventSourceFactory_h

//**********************************************************************************************************
//
//
//
// THIS FUNCTION WILL NOT COMPILE IF IT'S DEFINITION IS SEPARATED INTO A .cpp FILE!!!
//
//
//
//**********************************************************************************************************

#include "EventSourceBase.h"
#include <boost/property_tree/json_parser.hpp>

#include "TPCReco/GUI_commons.h"
#include "TPCReco/colorText.h"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>

#include <boost/filesystem.hpp>

#ifdef WITH_GET
#include "TPCReco/EventSourceGRAW.h"
#include "TPCReco/EventSourceMultiGRAW.h"
#endif
#include "TPCReco/EventSourceROOT.h"
#include "TPCReco/EventSourceMC.h"

namespace EventSourceFactory {
	inline std::shared_ptr<EventSourceBase> makeEventSourceObject(boost::property_tree::ptree& myConfig) {
		std::string dataFileName = myConfig.get<std::string>("input.dataFile");
		std::string geometryFileName = myConfig.get<std::string>("input.geometryFile");
		
		std::shared_ptr<EventSourceBase> myEventSource;

		if (dataFileName.empty() || geometryFileName.empty()) {
			std::cerr << "No data or geometry file path provided." << _endl_;
			exit(1);
		}

		// parse dataFile string for comma separated files
		std::vector<boost::filesystem::path> dataFileVec, dataFilePaths;

		size_t pos = 0;
		std::string token;
		std::string tmp_dataFileName = dataFileName;
		while ((pos = tmp_dataFileName.find(",")) != std::string::npos) {
			token = tmp_dataFileName.substr(0, pos);
			dataFilePaths.push_back(token);
			tmp_dataFileName.erase(0, pos + 1);
		}
		if(tmp_dataFileName.size()) dataFilePaths.push_back(tmp_dataFileName);
		
		#ifdef WITH_GET
        bool all_graw=false;
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

		if (dataFileVec.size() == 1 && boost::filesystem::is_regular_file(dataFileVec[0]) && dataFileName.find(".root") != std::string::npos) {
			myConfig.put("transient.onlineFlag", false);
			myEventSource = std::make_shared<EventSourceROOT>(geometryFileName);
			myConfig.put("transient.eventType", event_type::EventSourceROOT);
			EventSourceROOT* aRootEventSrc = dynamic_cast<EventSourceROOT*>(myEventSource.get());
			aRootEventSrc->configurePedestal(myConfig.find("pedestal")->second);
		}
		else if (dataFileVec.size() == 1 && dataFileName.find("_MC_") != std::string::npos) {
			myEventSource = std::make_shared<EventSourceMC>(geometryFileName);
			myConfig.put("transient.eventType", event_type::EventSourceMC);
		}		
#ifdef WITH_GET
		else if (all_graw) {
			myConfig.put("transient.onlineFlag", false);
			if (myConfig.get<bool>("input.singleAsadGrawFile")) {
				myEventSource = std::make_shared<EventSourceMultiGRAW>(geometryFileName);
				myConfig.put("transient.eventType", event_type::EventSourceMultiGRAW);				
			}
			else {
				myEventSource = std::make_shared<EventSourceGRAW>(geometryFileName);
				myConfig.put("transient.eventType", event_type::EventSourceGRAW);
				dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setFrameLoadRange(myConfig.get<int>("input.frameLoadRange"));
				if (dataFileVec.size() > 1) {
					std::cerr << KRED << "Provided too many GRAW files. Expected 1. dataFile: " << RST << dataFileName << _endl_;
					exit(0);
				}
			}
			EventSourceGRAW* aGrawEventSrc = dynamic_cast<EventSourceGRAW*>(myEventSource.get());
			aGrawEventSrc->configurePedestal(myConfig.find("pedestal")->second);
		}
		else if (dataFileVec.size() == 1 && boost::filesystem::is_directory(dataFileVec[0])) {
			myConfig.put("transient.onlineFlag", true);
			if (myConfig.get<bool>("input.singleAsadGrawFile")) {
				myEventSource = std::make_shared<EventSourceMultiGRAW>(geometryFileName);
				myConfig.put("transient.eventType", event_type::EventSourceMultiGRAW);
			}
			else {
				myEventSource = std::make_shared<EventSourceGRAW>(geometryFileName);
				myConfig.put("transient.eventType", event_type::EventSourceGRAW);
				dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setFrameLoadRange(myConfig.get<int>("input.frameLoadRange"));
			}
			EventSourceGRAW* aGrawEventSrc = dynamic_cast<EventSourceGRAW*>(myEventSource.get());
			aGrawEventSrc->configurePedestal(myConfig.find("pedestal")->second);
		}

#endif
	 if(!myEventSource) {
			std::cerr << KRED << "Input source not known. DataFile: " << RST << dataFileName << _endl_;
#ifndef WITH_GET
			std::cerr << KRED << "and GRAW libraries not set." << RST << _endl_;
#endif
			exit(0);
		}

		if (!myConfig.get<bool>("transient.onlineFlag")) {
			myEventSource->loadDataFile(dataFileName);
			myEventSource->loadFileEntry(0);
		}
		return myEventSource;
	}
};
#endif // !EventSourceFactory_h