#include <iostream>
#include <cstdlib>
#include <string>
#include <memory>

#include "GeometryTPC.h"
#include "EventTPC.h"
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

#ifdef DEBUG
#include "EventSourceROOT.h" // DEBUG - read back test
#include "EventTPC.h" // DEBUG - read back test
#endif

int main(int argc, char *argv[]) {

  if(argc!=4) {
    std::cerr << std::endl
	      << "Creates TTree \"TPCData\" with EventTPC objects out of the specified GRAW file." << std::endl << std::endl
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

  std::shared_ptr<EventSourceGRAW> myEventSource;
  if (dataFileName.find(".graw") != std::string::npos &&
      geometryFileName.find(".dat") != std::string::npos &&
      rootFileName.find(".root") != std::string::npos) {

    myEventSource = std::make_shared<EventSourceGRAW>(geometryFileName);
    //    dynamic_cast<EventSourceGRAW *>(myEventSource.get())
    //      ->setFrameLoadRange(160); // 160 frames
    myEventSource->setFrameLoadRange(160); // 160 frames
    myEventSource->setFillEventType(0); // EventTPC
    myEventSource->loadDataFile(dataFileName);

    std::cout << "File with " << myEventSource->numberOfEntries() << " frames opened."
              << std::endl;
  } else {
    std::cout << "One or more of the input arguments is/are weong. " << std::endl
	      << "Check that GRAW and geometry files are correct. " << std::endl
              << std::endl
              << std::endl;
    return -1;
  }

  //  EventSourceGRAW myEventSource(geomFileName);
  //  myEventSource.loadDataFile(dataFileName);
  std::shared_ptr<EventTPC> myEventPtr = myEventSource->getCurrentEvent();

#ifdef DEBUG
  ////// DEBUG
  std::cout << "==== GrawToEvenTPC INITIALIZATION: myPtr_EventTPC="
	    << myEventPtr << " ====" << std::endl;
  ////// DEBUG
#endif

  // Create ROOT Tree
  TFile aFile(rootFileName.c_str(),"RECREATE");
  TTree aTree("TPCData","");
  EventTPC *persistent_event = myEventPtr.get();
  aTree.Branch("Event", &persistent_event);
  
  // loop over ALL frames and fill "Event" tree with EventTPC objects
  Long64_t currentEventIdx=-1;
  std::map<unsigned int, bool> eventIdxMap;
  do {
    // load first frame and initialize eventId counter
    if(currentEventIdx==-1) {
      myEventSource->loadFileEntry(0);
    }

#ifdef DEBUG
    ////// DEBUG
    std::cout << "==== GrawToEventTPC X-CHECK: EventSourceGRAW EventID= "
    	      << myEventSource->currentEventNumber()
    	      << ", EventTPC EventID="
    	      << myEventPtr->GetEventId()
     	      << "====" << std::endl;
    ////// DEBUG
#endif
    
    // Skip empty events
    //    if(skipEmptyEvents && myEventPtr->GetMaxCharge()<100) continue;
    /////////////////////

    unsigned int eventIdx = myEventPtr->GetEventId();
    if(eventIdxMap.find(eventIdx)==eventIdxMap.end()){
      eventIdxMap[eventIdx] = true;

#ifdef DEBUG
      ///////// DEBUG
      std::cout << "==== GrawToEventTPC LOOP: persistentPtr_EventTPC="
		<< persistent_event << " ====" << std::endl;
      std::cout << "---- EventTPC content start ----" << std::endl;
      std::cout << *persistent_event << std::endl;
      std::cout << "---- EventTPC content end ----" << std::endl;
      ///////// DEBUG
#endif

      // temporarily reset geometry pointer while filling TTree
      //      std::shared_ptr<GeometryTPC> gPtr(myEventPtr->GetGeoPtr());
      myEventPtr->SetGeoPtr(0);
      
      aTree.Fill();
      //      myEventPtr->SetGeoPtr(gPtr);
    }
    
#ifdef DEBUG
    ////// DEBUG
    if( eventIdxMap.size()==100 ) break;
    ////// DEBUG
#endif
    
    // load next event (if any)
    currentEventIdx=myEventSource->currentEventNumber();
    myEventSource->getNextEvent();
  }
  while(currentEventIdx!=(Long64_t)myEventSource->currentEventNumber());
  /*
  long numberOfEntries = myEventSource.numberOfEntries();
  std::map<unsigned int, bool> eventIdxMap;
  for(long iFileEntry = 0; iFileEntry<numberOfEntries; ++iFileEntry){
    myEventSource.loadFileEntry(iFileEntry);
    ///Skip empty events
    if(skipEmptyEvents && myEventPtr->GetMaxCharge()<100) continue;
    /////////////////////
    unsigned int eventIdx = myEventPtr->GetEventId();
    if(eventIdxMap.find(eventIdx)==eventIdxMap.end()){
      eventIdxMap[eventIdx] = true;
      //Reset geometry pointer for writing to ROOT file
      myEventPtr->SetGeoPtr(0);
      aTree.Fill();
    }
    if( iFileEntry>10) break;
  }
  */
  aTree.Print();
  // build index based on: majorname=EventId, minorname=NONE
  aTree.BuildIndex("Event.event_id");
  aTree.Write("", TObject::kOverwrite); // save only the new version of the tree
  aFile.Close();
  
 #ifdef DEBUG
  ///// DEBUG - READ BACK TEST
  std::shared_ptr<EventSourceROOT> myEventSourceRoot;
  myEventSourceRoot = std::make_shared<EventSourceROOT>(geometryFileName);
  myEventSourceRoot->setReadEventType(EventSourceROOT::tpc); // EventTPC
  myEventSourceRoot->loadDataFile(rootFileName);
  std::cout<<"myEventSourceRoot.loadEventId(3)"<<std::endl;
  myEventSourceRoot->loadEventId(3);
  std::cout<<*(myEventSourceRoot->getCurrentEvent())<<std::endl;
  ///// DEBUG - READ BACK TEST
#endif
 /*
  std::cout<<"myEventSource.loadFileEntry(0)"<<std::endl;
  myEventSource.loadFileEntry(0);

  std::cout<<"myEventSource.loadEventId(1)"<<std::endl;
  myEventSource.loadEventId(0);

  //std::cout<<"myEventSource.getNextEvent()"<<std::endl;
  //myEventSource.getNextEvent();
  
  std::cout<<"myEventSource.loadEventId(3)"<<std::endl;
  myEventSource.loadEventId(3);
 
  std::cout<<"myEventSource.getPreviousEvent()"<<std::endl;
  myEventSource.getPreviousEvent();

  std::cout<<"myEventSource.getNextEvent() "<<std::endl;
  myEventSource.getNextEvent();

  std::cout<<" myEventSource.getLastEvent() "<<std::endl;
  myEventSource.getLastEvent();

  std::cout<<"myEventSource.getNextEvent() "<<std::endl;
  myEventSource.getNextEvent();

  std::cout<<"myEventSource.loadEventId(0)"<<std::endl;
  myEventSource.loadEventId(0);

  std::cout<<"myEventSource.getNextEvent()"<<std::endl;
  myEventSource.getNextEvent();

  std::cout<<"myEventSource.getPreviousEvent()"<<std::endl;
  myEventSource.getPreviousEvent();
  */

  return 0;
}
