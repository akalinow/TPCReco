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

#ifdef WITH_GET
#include "EventSourceGRAW.h"
#include "EventSourceMultiGRAW.h"
#endif
#include "EventSourceROOT.h"
#include "EventSourceMC.h"

#include "MainFrame.h"

inline std::shared_ptr<EventSourceBase> EventSourceFactory::makeEventSourceObject(boost::property_tree::ptree myConfig, MainFrame& mf) {
	std::string dataFileName = myConfig.get("dataFile", "");
	std::string geometryFileName = myConfig.get("geometryFile", "");

	std::shared_ptr<EventSourceBase> myEventSource;

	if (dataFileName.empty() || geometryFileName.empty()) {
		std::cerr << "No data or geometry file path provided." << _endl_;
		exit(1);
		return decltype(myEventSource)();
	}
	FileStat_t stat;

	// parse dataFile string for comma separated files
	std::vector<std::string> dataFileVec;
	//  vector<std::string> basenameVec;
	//  vector<std::string> dirnameVec;
	TString list = dataFileName;
	TObjArray* token = list.Tokenize(",");
	//  token->Print();
#ifdef WITH_GET
	bool all_graw = false;
#endif
	for (Int_t itoken = 0; itoken < token->GetEntries(); itoken++) {
		//    TString list2=((TObjString *)(token->At(itoken)))->String(); // path + name of single file
		std::string list2(((TObjString*)(token->At(itoken)))->String().Data());
		if (gSystem->GetPathInfo(list2.c_str(), stat) != 0) {
			if (list2.find("_MC_") != std::string::npos) {
				dataFileVec.push_back(list2);
				continue;
			}
			std::cerr << KRED << "Invalid data path. No such file or directory: " << RST << list2 << _endl_;
			exit(1);
		}
#ifdef WITH_GET
		if (((stat.fMode & EFileModeMask::kS_IFREG) == EFileModeMask::kS_IFREG) && list2.find(".graw") != std::string::npos) {
			all_graw = true;
		}
		else {
			all_graw = false;
		}
#endif
		dataFileVec.push_back(list2);
		//   TObjArray *token2 = list2.Tokenize("/");
		//    TString dirName;
		//    for (Int_t itoken2=0; itoken2 < token2->GetEntries()-1; itoken2++) {
		//      dirName=((TObjString *)(token2->At(itoken2)))->String()+"/"; // path
		//    }
		//    TString baseName;
		//    if(token2->GetEntries()-1 > 0) baseName=((TObjString *)(token2->At(token2-GetEntries()-1)))->String(); // basename
		//    dirnameVec.push_back(dirName);
		//    basenameVec.push_back(baseName);
	}

	//  if(gSystem->GetPathInfo(dataFileName.c_str(), stat) != 0){
	//    std::cerr<<KRED<<"Invalid data path. No such file or directory: "<<RST<<dataFileName<<_endl_;
	//   return;
	//  }

	std::cout << "dataFileVec.size(): " << dataFileVec.size()
		<< " ((stat.fMode & EFileModeMask::kS_IFREG) == EFileModeMask::kS_IFREG): " << ((stat.fMode & EFileModeMask::kS_IFREG) == EFileModeMask::kS_IFREG)
		<< "dataFileName.find(_MC_)!=std::string::npos: " << (dataFileName.find("_MC_") != std::string::npos)
		<< _endl_;

	if (dataFileVec.size() == 1 && ((stat.fMode & EFileModeMask::kS_IFREG) == EFileModeMask::kS_IFREG) && dataFileName.find(".root") != std::string::npos) {
		mf.myWorkMode = M_OFFLINE_ROOT_MODE;
		myEventSource = std::make_shared<EventSourceROOT>(geometryFileName);
	}
	else if (dataFileVec.size() == 1 && dataFileName.find("_MC_") != std::string::npos) {
		mf.myWorkMode = M_OFFLINE_MC_MODE;
		myEventSource = std::make_shared<EventSourceMC>(geometryFileName);
	}

#ifdef WITH_GET
	else if (all_graw) { //(stat.fMode & EFileModeMask::kS_IFREG) == EFileModeMask::kS_IFREG) && dataFileName.find(".graw")!=std::string::npos){

		mf.myWorkMode = M_OFFLINE_GRAW_MODE;
		if (myConfig.find("singleAsadGrawFile") != myConfig.not_found()) {
			bool singleAsadGrawFile = myConfig.get<bool>("singleAsadGrawFile");
			if (singleAsadGrawFile) {
				mf.myWorkMode = M_OFFLINE_NGRAW_MODE;
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
			{ unsigned int AsadNboards = dynamic_cast<EventSourceGRAW*>(myEventSource.get())->getGeometry()->GetAsadNboards();
			if (dataFileVec.size() > AsadNboards) {
				std::cerr << KRED << "Provided too many GRAW files. Expected up to " << AsadNboards << ".dataFile: " << RST << dataFileName << _endl_;
				return decltype(myEventSource)();
			}
			}
			break;
		default:;
		};

	}
	else if (dataFileVec.size() == 1 && (stat.fMode & EFileModeMask::kS_IFDIR) == EFileModeMask::kS_IFDIR) {

		mf.myWorkMode = M_ONLINE_GRAW_MODE;
		if (myConfig.find("singleAsadGrawFile") != myConfig.not_found()) {
			bool singleAsadGrawFile = myConfig.get<bool>("singleAsadGrawFile");
			if (singleAsadGrawFile) {
				mf.myWorkMode = M_ONLINE_NGRAW_MODE;
			}
		}
		switch (mf.myWorkMode) {
		case M_ONLINE_GRAW_MODE:
			myEventSource = std::make_shared<EventSourceGRAW>(geometryFileName);
			dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setFrameLoadRange(myConfig.get("frameLoadRange", 10));
			break;
		case M_ONLINE_NGRAW_MODE:
			myEventSource = std::make_shared<EventSourceMultiGRAW>(geometryFileName);
			break;
		default:;
		};
		mf.fileWatchThread = std::thread(&DirectoryWatch::watch, &mf.myDirWatch, dataFileName);
		if (myConfig.find("updateInterval") != myConfig.not_found()) {
			int updateInterval = myConfig.get<int>("updateInterval");
			mf.myDirWatch.setUpdateInterval(updateInterval);
		}
		mf.myDirWatch.Connect("Message(const char *)", "MainFrame", this, "ProcessMessage(const char *)");
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
	if (mf.myWorkMode != M_ONLINE_GRAW_MODE && mf.myWorkMode != M_ONLINE_NGRAW_MODE) {
		myEventSource->loadDataFile(dataFileName);
		myEventSource->loadFileEntry(0);
	}
	return myEventSource;
}

inline EventSourceFactory& EventSourceFactory::getFactory() {
	static EventSourceFactory ESF;
	return ESF;
}