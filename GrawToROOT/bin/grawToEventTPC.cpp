#include <iostream>
#include <cstdlib>
#include <string>
#include <memory>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>


#include "GeometryTPC.h"
#include "EventTPC.h"
#include "PEventTPC.h"
#include "PedestalCalculator.h"
#include "EventSourceGRAW.h"
#include "EventSourceMultiGRAW.h"
#include "SigClusterTPC.h"

#include "TFile.h"
#include "TTree.h"

#include "colorText.h"

#include "utl/Logging.h"
#include "get/GDataSample.h"
#include "get/GDataChannel.h"
#include "get/GDataFrame.h"
#include "get/TGrawFile.h"
#include "mfm/FrameDictionary.h"

#ifdef DEBUG
#include "EventSourceROOT.h" 
#endif

/////////////////////////////////////
/////////////////////////////////////
std::string createROOTFileName(const  std::string & grawFileName){

  std::string rootFileName;
  unsigned int index = grawFileName.find(",");
  if(index!=std::string::npos){
    rootFileName = grawFileName.substr(0,index);
  }
  index = rootFileName.rfind("/");
  rootFileName = rootFileName.substr(index,-1);

  
  if(rootFileName.find("CoBo_ALL_AsAd_ALL")!=std::string::npos){
    rootFileName = rootFileName.replace(0,std::string("CoBo_ALL_AsAd_ALL").size()+1,"EventTPC");
  }
  else if(rootFileName.find("CoBo0_AsAd")!=std::string::npos){
    rootFileName = rootFileName.replace(0,std::string("CoBo0_AsAd").size()+2,"EventTPC");
  }
  else{
    std::cout<<KRED<<"File format unknown: "<<RST<<rootFileName<<std::endl;
    exit(1);
  }
  index = rootFileName.rfind("graw");
  rootFileName = rootFileName.replace(index,-1,"root");
  
  return rootFileName;
}
/////////////////////////////////////
/////////////////////////////////////
int convertGRAWFile(const  std::string & geometryFileName,
		    const  std::string & grawFileName);
/////////////////////////////////////
/////////////////////////////////////
boost::program_options::variables_map parseCmdLineArgs(int argc, char **argv){

  boost::program_options::options_description cmdLineOptDesc("Allowed options");
  cmdLineOptDesc.add_options()
    ("help", "produce help message")
    ("geometryFile",  boost::program_options::value<std::string>(), "string - path to the geometry file.")
    ("dataFile",  boost::program_options::value<std::string>(), "string - path to GRAW data file.");
  
  boost::program_options::variables_map varMap;        
  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cmdLineOptDesc), varMap);
  boost::program_options::notify(varMap); 

  if (varMap.count("help")) {
    std::cout<<cmdLineOptDesc<<std::endl;
    exit(1);
  }
  return varMap;
}
/////////////////////////////////////
/////////////////////////////////////
int main(int argc, char *argv[]) {

  std::string geometryFileName, dataFileName;
  boost::program_options::variables_map varMap = parseCmdLineArgs(argc, argv);
  boost::property_tree::ptree tree;
  if(argc<3){
    char text[] = "--help";
    char *argvTmp[] = {text, text};
    parseCmdLineArgs(2,argvTmp);
    return 1;
  }
  if (varMap.count("geometryFile")) {
    geometryFileName = varMap["geometryFile"].as<std::string>();
  }
  if (varMap.count("dataFile")) {
    dataFileName = varMap["dataFile"].as<std::string>();
  }

  if(dataFileName.size() && geometryFileName.size()){
    convertGRAWFile(geometryFileName, dataFileName);
  }
  else{
    std::cout<<KRED<<"Configuration not complete: "<<RST
	     <<" geometryFile: "<<geometryFileName<<"\n"
	     <<" dataFile: "<<dataFileName
	     <<std::endl;
  }
  return 0;
}
/////////////////////////////////////
/////////////////////////////////////
int convertGRAWFile(const  std::string & geometryFileName,
		    const  std::string & grawFileName){
  
  if(grawFileName.find(".graw") == std::string::npos ||
     geometryFileName.find(".dat") == std::string::npos){
    std::cout <<KRED<<"One or more of the input arguments is/are wrong. "<<RST<<std::endl
	      << "Check that GRAW and geometry files are correct. " << std::endl
	      << std::endl
	      << std::endl;
    return -1;
  }

  std::string rootFileName = createROOTFileName(grawFileName);
  TFile aFile(rootFileName.c_str(),"RECREATE");

  boost::property_tree::ptree tree;
  tree.put("minPedestalCell",5);
  tree.put("maxPedestalCell",25);
  tree.put("minSignalCell",5);
  tree.put("maxSignalCell",506);

  std::shared_ptr<EventSourceBase> myEventSource;
  if(grawFileName.find(",")!=std::string::npos){
    myEventSource = std::make_shared<EventSourceMultiGRAW>(geometryFileName);    
  }
  else{
    myEventSource = std::make_shared<EventSourceGRAW>(geometryFileName);
    dynamic_cast<EventSourceGRAW*>(myEventSource.get())->setFrameLoadRange(160);
  }
  dynamic_cast<EventSourceGRAW*>(myEventSource.get())->configurePedestal(tree);
  myEventSource->loadDataFile(grawFileName);
  std::cout << "File with " << myEventSource->numberOfEntries() << " frames opened." << std::endl;
  
  auto myEventPtr = myEventSource->getCurrentPEvent();

  #ifdef DEBUG
  std::cout << "==== GrawToEvenTPC INITIALIZATION: myPtr_EventTPC="
	    << myEventPtr << " ====" << std::endl;
  #endif

  TTree aTree("TPCData","");
  auto persistent_event = myEventPtr.get();
  Int_t bufsize=128000;
  int splitlevel=2;
  aTree.Branch("Event", &persistent_event, bufsize, splitlevel); 
  Long64_t currentEventId=-1;
  std::map<unsigned int, bool> eventIdMap;

  myEventSource->loadFileEntry(0);

  do {

#ifdef DEBUG
    std::cout << "==== GrawToEventTPC X-CHECK: EventSourceGRAW EventID= "
    	      << myEventSource->currentEventNumber()
    	      << ", EventTPC EventID="
    	      << myEventPtr->GetEventInfo().GetEventId()
     	      << "====" << std::endl;
#endif
    
    unsigned int eventId = myEventPtr->GetEventInfo().GetEventId();    
    if(eventIdMap.find(eventId)==eventIdMap.end()){
      eventIdMap[eventId] = true;

#ifdef DEBUG
      std::cout << "==== GrawToEventTPC LOOP: persistentPtr_EventTPC="
		<< persistent_event << " ====" << std::endl;
      std::cout << "---- EventTPC content start ----" << std::endl;
      std::cout << *persistent_event << std::endl;
      std::cout << "---- EventTPC content end ----" << std::endl;
#endif

      std::cout<< myEventPtr->GetEventInfo()<<std::endl;
      aTree.Fill();
      if(eventIdMap.size()%100==0) aTree.FlushBaskets();
    }

#ifdef DEBUG
    if( eventIdMap.size()==10) break;
#endif

    currentEventId=myEventSource->currentEventNumber();
    myEventSource->getNextEvent();
  }
  while(currentEventId!=(Long64_t)myEventSource->currentEventNumber());  
  aTree.Print();
  // build index based on: majorname=EventId, minorname=NONE
  aTree.BuildIndex("Event.myEventInfo.eventId");
  aTree.Write("", TObject::kOverwrite); // save only the new version of the tree
  //aFile.Close();

  return 0;
      
  
 #ifdef DEBUG
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
  #endif

  return 0;
}
/////////////////////////////////////
/////////////////////////////////////
