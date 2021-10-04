
#include "TFile.h"

#include "HistoManager.h"
//#include "EventSourceROOT.h"
#include "EventSourceGRAW.h"

#include <boost/property_tree/json_parser.hpp>

int main(int argc, char *argv[]) {
  //  if (argc < 8) {
  //  if (argc < 6) {
  if (argc < 7) {
    std::cout << "Wrong number of arguments. Should be:" << std::endl
              << argv[0]
              << " <input_file.graw> " 
      //	      << "<hit_threshold> <max_strips_per_dir> <max_timecells_per_dir> <matching_R_in_mm> "
      //      	      << "<hit_threshold> <matching_R_in_mm> "
	      << "<hit_threshold> <total_charge_threshold> <matching_R_in_mm> "
	      << "<geometry_file.dat> <result_file.root>"
              << std::endl
              << std::endl;
    return -1;
  }

  std::string dataFileName(argv[1]);
  unsigned int hitThr = atoi(argv[2]);
  //  unsigned int maxStripsPerDir = atoi(argv[3]);
  //  unsigned int maxTimecellsPerDir = atoi(argv[4]);
  //  double matchRadiusInMM = atoi(argv[5]);
  //  std::string geometryFileName(argv[6]);
  //  std::string rootFileName(argv[7]);
  unsigned int totalChargeThr = atoi(argv[3]);
  double matchRadiusInMM = atoi(argv[4]);
  std::string geometryFileName(argv[5]);
  std::string rootFileName(argv[6]);

  //  std::shared_ptr<EventSourceBase> myEventSource;
  std::shared_ptr<EventSourceGRAW> myEventSource;
  if (dataFileName.find(".graw") != std::string::npos &&
      geometryFileName.find(".dat") != std::string::npos &&
      rootFileName.find(".root") != std::string::npos) {

    myEventSource = std::make_shared<EventSourceGRAW>(geometryFileName);
      //    myEventSource->loadGeometry(geometryFileName);

    dynamic_cast<EventSourceGRAW *>(myEventSource.get())
      ->setFrameLoadRange(160); // 160 frames

    myEventSource->loadDataFile(dataFileName);
    
    std::cout << "File with " << myEventSource->numberOfEntries() << " frames loaded."
              << std::endl;
  } else {
    std::cout << "One or more of the input arguments is/are weong. " << std::endl
	      << "Check that GRAW and geometry files are correct. " << std::endl
	      << "The output ROOT file must not be present."
              << std::endl
              << std::endl;
    return -1;
  }

  //  TFile aFile(rootFileName.c_str(), "RECREATE");
  HistoManager myHistoManager;
  myHistoManager.setGeometry(myEventSource->getGeometry());
  myHistoManager.initializeDotFinder(hitThr, /* maxStripsPerDir, maxTimecellsPerDir, */ totalChargeThr, matchRadiusInMM, rootFileName);

  // loop over ALL events and fill various histograms per RUN
  Long64_t currentEventIdx=-1;

  ////// DEBUG
  Long64_t counter=0;
  ////// DEBUG

  do {
    // load first event
    if(currentEventIdx==-1) {
      myEventSource->loadFileEntry(0);
    }

    std::cout << "EventID: " << myEventSource->currentEventNumber() << std::endl;
    myHistoManager.setEvent(myEventSource->getCurrentEvent());

    // fill statistical histograms per run (before & after user-defined cuts)
    myHistoManager.runDotFinder();
    
    // load next event (if any)
    currentEventIdx=myEventSource->currentEventNumber(); //myCurrentEvent->GetEventId();
    myEventSource->getNextEvent();

    ////// DEBUG
    if(++counter==5) break;
    ////// DEBUG
  }
  while(currentEventIdx!=(Long64_t)myEventSource->currentEventNumber()); // myCurrentEvent->GetEventId());

  // write histograms to ROOTFILE
  myHistoManager.finalizeDotFinder();  
  //  aFile.Close();

  return 0;
}
