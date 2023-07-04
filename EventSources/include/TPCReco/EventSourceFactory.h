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
		std::string dataFileName = myConfig.get("dataFile", "");
		std::string geometryFileName = myConfig.get("geometryFile", "");

		std::shared_ptr<EventSourceBase> myEventSource;

		if (dataFileName.empty() || geometryFileName.empty()) {
			std::cerr << "No data or geometry file path provided." << _endl_;
			exit(1);
		}

		// parse dataFile string for comma separated files
		std::vector<boost::filesystem::path>
			dataFileVec,
			dataFilePaths;

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
			myEventSource = std::make_shared<EventSourceROOT>(geometryFileName);
			myConfig.put("eventType", event_type::EventSourceROOT);
		}
		else if (dataFileVec.size() == 1 && dataFileName.find("_MC_") != std::string::npos) {
			myEventSource = std::make_shared<EventSourceMC>(geometryFileName);
			myConfig.put("eventType", event_type::EventSourceMC);
		}
		

#ifdef WITH_GET
		else if (all_graw) { //ROOT_file_check && dataFileName.find(".graw")!=std::string::npos){

			myConfig.put("onlineFlag", false);
			if (myConfig.find("singleAsadGrawFile") != myConfig.not_found() && myConfig.get<bool>("singleAsadGrawFile")) {
				myEventSource = std::make_shared<EventSourceMultiGRAW>(geometryFileName);
				myConfig.put("eventType", event_type::EventSourceMultiGRAW);
				{
					unsigned int AsadNboards = dynamic_cast<EventSourceGRAW*>(myEventSource.get())->getGeometry()->GetAsadNboards();
					if (dataFileVec.size() > AsadNboards) {
						std::cerr << KRED << "Provided too many GRAW files. Expected up to " << AsadNboards << ".dataFile: " << RST << dataFileName << _endl_;
						return decltype(myEventSource)();
					}
				}
			}
			else {
				myEventSource = std::make_shared<EventSourceGRAW>(geometryFileName);
				myConfig.put("eventType", event_type::EventSourceGRAW);
				dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setFrameLoadRange(myConfig.get("frameLoadRange", 100));
				if (dataFileVec.size() > 1) {
					std::cerr << KRED << "Provided too many GRAW files. Expected 1. dataFile: " << RST << dataFileName << _endl_;
					return decltype(myEventSource)();
				}
			}
		}
		else if (dataFileVec.size() == 1 && boost::filesystem::is_directory(dataFileVec[0])) {
			myConfig.put("onlineFlag", true);
			if (myConfig.find("singleAsadGrawFile") != myConfig.not_found() && myConfig.get<bool>("singleAsadGrawFile")) {
				myEventSource = std::make_shared<EventSourceMultiGRAW>(geometryFileName);
				myConfig.put("eventType", event_type::EventSourceMultiGRAW);
			}
			else {
				myEventSource = std::make_shared<EventSourceGRAW>(geometryFileName);
				myConfig.put("eventType", event_type::EventSourceGRAW);
				dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setFrameLoadRange(myConfig.get("frameLoadRange", 10));
			}
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
			std::cerr << KRED << "and GRAW libraries not set." << RST << _endl_;
#endif
			exit(0);
		}

		if (myConfig.find("onlineFlag") != myConfig.not_found() || !myConfig.get<bool>("onlineFlag")) {
			myEventSource->loadDataFile(dataFileName);
			myEventSource->loadFileEntry(0);
		}
		return myEventSource;
	}
};
#endif // !EventSourceFactory_h