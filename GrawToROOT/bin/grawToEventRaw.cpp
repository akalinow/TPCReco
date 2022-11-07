#include <iostream>
#include <cstdlib>
#include <string>
#include <memory>

#include "GeometryTPC.h"
#include "EventRaw.h"
#include "PedestalCalculator.h"
#include "EventSourceGRAW.h"

#include "TFile.h"
#include "TTree.h"

#include "utl/Logging.h"
#include "get/GDataSample.h"
#include "get/GDataChannel.h"
#include "get/GDataFrame.h"
#include "get/TGrawFile.h"
#include "mfm/FrameDictionary.h"

#include "colorText.h"

#ifdef DEBUG
#include "EventSourceROOT.h" // DEBUG - read back test
#include "EventTPC.h" // DEBUG - read back test
#endif

int main(int argc, char *argv[]) {

  if(argc!=4) {
    std::cerr << std::endl
	      << "Creates TTree \"TPCDataRaw\" with EventRaw objects out of the specified GRAW file." << std::endl << std::endl
	      << "Usage: " << std::endl
	      << argv[0] << " <input_file.graw> <geometry_file.dat> <result_file.root>" << std::endl << std::endl
	      << "where:" << std::endl
	      << " - input_file.graw = input GRAW file name in \"name_NNNN.graw\" format" << std::endl
	      << " - geometry_file.dat = TPC geometry file name" << std::endl
	      << " - result_file.root = output ROOT file name" << std::endl << std::endl;
    return -1;
  }
  
  //  bool skipEmptyEvents = false;
  
  ///Load TPC geometry
  std::string dataFileName; //  =  "/data/edaq/CoBo_2018-05-11T14:23:14.736_0000.graw";
  std::string geometryFileName; //  = "geometry_mini_eTPC.dat";
  std::string rootFileName; //  = "";

  dataFileName = std::string(argv[1]);
  std::cout<<"dataFileName: "<<dataFileName<<std::endl;
  
  geometryFileName = std::string(argv[2]);
  std::cout<<"geometryFileName: "<<geometryFileName<<std::endl;
  
  rootFileName = std::string(argv[3]);
  std::cout<<"rootFileName: "<<rootFileName<<std::endl;


  if (dataFileName.find(".graw") != std::string::npos &&
      geometryFileName.find(".dat") != std::string::npos &&
      rootFileName.find(".root") != std::string::npos) {
  } else {
    std::cout << "One or more of the input arguments is/are weong. " << std::endl
	      << "Check that GRAW and geometry files are correct. " << std::endl
              << std::endl
              << std::endl;
    return -1;
  }
      
  ///Create ROOT Tree
  TFile aFile(rootFileName.c_str(),"RECREATE");

  auto myEventSource = std::make_shared<EventSourceGRAW>(geometryFileName);
  myEventSource->setFrameLoadRange(160);
  myEventSource->setFillEventType(EventType::raw);
  myEventSource->loadDataFile(dataFileName);
  std::cout << "File with " << myEventSource->numberOfEntries() << " frames opened." << std::endl;
  
  std::shared_ptr<eventraw::EventRaw> myEventRawPtr = myEventSource->getCurrentEventRaw();

  #ifdef DEBUG
  ////// DEBUG
  std::cout << "==== GrawToEventRaw INITIALIZATION: myPtr_EventRaw="
	    << myEventRawPtr << " ====" << std::endl;
  ////// DEBUG
  #endif

  TTree aTree("TPCDataRaw","");
  eventraw::EventRaw  *persistent_eventRaw = myEventRawPtr.get();
  eventraw::EventInfo *persistent_eventInfo = (eventraw::EventInfo*)persistent_eventRaw;
  eventraw::EventData *persistent_eventData = (eventraw::EventData*)persistent_eventRaw;
  aTree.Branch("EventInfo", &persistent_eventInfo);
  aTree.Branch("EventData", &persistent_eventData);
  //aTree.Branch("EventRaw", persistent_eventRaw);

  // loop over ALL frames and fill "EventRaw" tree with EventRaw objects
  Long64_t currentEventIdx=-1;
  std::map<unsigned int, bool> eventIdxMap;
  do {
    // load first frame and initialize eventId counter
    if(currentEventIdx==-1) {
      myEventSource->loadFileEntry(0);
    }

#ifdef DEBUG
    ////// DEBUG
    std::cout << "==== GrawToEventRaw X-CHECK: EventSourceGRAW EventID="
    	      << myEventSource->currentEventNumber()
    	      << ", EventRaw EventID="
    	      << *myEventRawPtr
    	      << " ====" << std::endl;
    ////// DEBUG
#endif
    
    unsigned int eventIdx = myEventSource->currentEventNumber();//myEventPtrRaw->eventId;
    if(eventIdxMap.find(eventIdx)==eventIdxMap.end()){
      eventIdxMap[eventIdx] = true;

#ifdef DEBUG
      ///////// DEBUG
      std::cout << "==== GrawToEventRaw LOOP: persistentPtr_EventRaw="
		<< persistent_eventRaw << " ====" << std::endl;
      std::cout << "---- EventRaw content start ----" << std::endl;
      std::cout << *persistent_eventRaw << std::endl;
      std::cout << "---- EventRaw content end ----" << std::endl;
      //      std::cout << *persistent_eventInfo << std::endl;
      //      std::cout << *persistent_eventData << std::endl;
      ///////// DEBUG
#endif

      aTree.Fill();
    }

#ifdef DEBUG
    ////// DEBUG
    if( eventIdxMap.size()==3 ) break;
    ////// DEBUG
#endif

    // load next event (if any)
    currentEventIdx=myEventSource->currentEventNumber();
    myEventSource->getNextEvent();
  }
  while(currentEventIdx!=(Long64_t)myEventSource->currentEventNumber());
  /*
  long numberOfEntries = myEventSource.numberOfEntries();
  std::cout << "Number of entries = " << numberOfEntries << std::endl;
  std::map<uint64_t, bool> eventIdxMap;
  for(long iFileEntry = 0; iFileEntry<numberOfEntries; ++iFileEntry){
    myEventSource.loadFileEntry(iFileEntry);

    uint64_t eventIdx = myEventPtrRaw->eventId;
    if(eventIdxMap.find(eventIdx)==eventIdxMap.end()){
      eventIdxMap[eventIdx] = true;
      aTree.Fill();
    }
    //    break;
    if (iFileEntry>10) break;
  }
  */
  aTree.Print();

  // build index based on: majorname=EventId, minorname=NONE
  //aTree.BuildIndex("eventId");
  aTree.Write("", TObject::kOverwrite); // save only the new version of the tree
  aFile.Close();

#ifdef DEBUG
  ///// DEBUG - READ BACK TEST
  std::shared_ptr<EventSourceROOT> myEventSourceRoot;
  myEventSourceRoot = std::make_shared<EventSourceROOT>(geometryFileName);
  myEventSourceRoot->loadDataFile(rootFileName);
  std::cout<<"myEventSourceRoot.loadEventId(3)"<<std::endl;
  myEventSourceRoot->loadEventId(3);
  std::cout<<*(myEventSourceRoot->getCurrentEvent())<<std::endl;
  double chargeThreshold = 35;
  int delta_timecells = 25;
  int delta_strips = 5;
  myEventSourceRoot->getCurrentEvent()->MakeOneCluster(chargeThreshold, delta_strips, delta_timecells);
  ///// DEBUG - READ BACK TEST
#endif
  
  return 0;
}
