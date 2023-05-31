#ifdef WITH_GET
#include <iostream>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>
<<<<<<< HEAD
#include "colorText.h"
#include "TFile.h"
#include "GeometryTPC.h"
#include "EventSourceGRAW.h"
#include "EventSourceMultiGRAW.h"
#include "DotFinder.h"
#include "ConfigManager.h"
=======
#include "TPCReco/colorText.h"
#include <TFile.h>
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/EventSourceGRAW.h"
#include "TPCReco/EventSourceMultiGRAW.h"
#include "TPCReco/DotFinder.h"
>>>>>>> f354324fc0e2a0130807f8471dda39732124fe4f

void analyzeRawEvents(const boost::property_tree::ptree &aConfig);

/////////////////////////////////////
/////////////////////////////////////
int main(int argc, char **argv){
  ConfigManager cm;
  boost::property_tree::ptree tree = cm.getConfig(argc,argv);

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
