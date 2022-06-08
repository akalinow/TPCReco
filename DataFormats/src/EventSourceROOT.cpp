#include <cstdlib>
#include <iostream>

#include "TFile.h"
#include "TTree.h"

#include "colorText.h"
#include "EventSourceROOT.h"
#include "PedestalCalculator.h"

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceROOT::EventSourceROOT(const std::string & geometryFileName) {

  loadGeometry(geometryFileName);
  myPedestalCalculator.SetGeometryAndInitialize(myGeometryPtr);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceROOT::setTreePointers(const std::string & fileName) {

  readEventType = EventType::raw;
  if(fileName.find("EventTPC")!=std::string::npos) readEventType = EventType::tpc;

  switch(readEventType) {
  case EventType::raw: 
    treeName = "TPCDataRaw";
    aPtrEventInfo = (eventraw::EventInfo*)(myCurrentEventRaw.get());
    aPtrEventData = (eventraw::EventData*)(myCurrentEventRaw.get());
    aPtr=NULL;
    break;
  case EventType::tpc: 
  default:
    treeName = "TPCData";
    aPtr = myCurrentPEvent.get();
    aPtrEventInfo=NULL;
    aPtrEventData=NULL;
    break;
  };

#ifdef DEBUG
  ///// DEBUG
  std::cout << __FUNCTION__
	    << " aPtr=" << aPtr
	    << ", aPtrEventInfo=" << aPtrEventInfo
	    << ", aPtrEventData=" << aPtrEventData << std::endl;
  ///// DEBUG
#endif
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EventSourceROOT::~EventSourceROOT() { }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceROOT::setRemovePedestal(bool aFlag){
  removePedestal = aFlag;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceROOT::configurePedestal(const boost::property_tree::ptree &config){
  auto parser=[this, &config](std::string &&parameter, void (PedestalCalculator::*setter)(int)){
    if(config.find(parameter)!=config.not_found()){
      (this->myPedestalCalculator.*setter)(config.get<int>(parameter));
    }
  };
  parser("minPedestalCell", &PedestalCalculator::SetMinPedestalCell);
  parser("maxPedestalCell", &PedestalCalculator::SetMaxPedestalCell);
  parser("minSignalCell", &PedestalCalculator::SetMinSignalCell);
  parser("maxSignalCell", &PedestalCalculator::SetMaxSignalCell);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceROOT::loadDataFile(const std::string & fileName){

  EventSourceBase::loadDataFile(fileName);

  myFile = std::make_shared<TFile>(fileName.c_str(),"READ");
  if(!myFile){
    std::cerr<<KRED<<"Can not open file: "<<RST<<fileName<<KRED<<"!"<<RST<<std::endl;
    exit(1);
  }

  setTreePointers(fileName);
  myTree.reset((TTree*)myFile->Get(treeName.c_str()));
  if(!myTree){
    std::cout<<KRED<<"ERROR "<<RST<<"TTree with name: "<<treeName
	     <<" not found in TFile. "<<std::endl;
    std::cout<<"TFile content is: "<<std::endl;
    myFile->ls();
    exit(0);
  }

  switch(readEventType) {
  case EventType::raw:
    myTree->SetBranchAddress("EventInfo", &aPtrEventInfo);
    myTree->SetBranchAddress("EventData", &aPtrEventData);
    break;

  case EventType::tpc:
  default:
    myTree->SetBranchAddress("Event", &aPtr);
    break;
  };
  
  nEntries = myTree->GetEntries();

#ifdef DEBUG
  ///// DEBUG
  std::cout << __FUNCTION__
	    << ": aPtr=" << aPtr
	    << ", aPtrEventInfo=" << aPtrEventInfo
	    << ", aPtrEventData=" << aPtrEventData
	    << ", entries=" << nEntries << std::endl;
  ///// DEBUG
#endif
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceROOT::loadFileEntry(unsigned long int iEntry){

  if(!myTree){
    std::cerr<<"ROOT tree not available!"<<std::endl;
    return;
  }
  if((long int)iEntry>=myTree->GetEntries()) iEntry = myTree->GetEntries() - 1;

  switch(readEventType) {
  case EventType::raw:
    myTree->GetEntry(iEntry);
    fillEventFromEventRaw();
    break;

  case EventType::tpc: 
  default:
    myCurrentEvent->SetGeoPtr(0);
    myTree->GetEntry(iEntry);
    myCurrentEvent->SetGeoPtr(myGeometryPtr);
    break;
  };
  
  myCurrentEntry = iEntry;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
unsigned long int EventSourceROOT::numberOfEvents() const{ return nEntries; }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> EventSourceROOT::getNextEvent(){

  if(nEntries>0 && myCurrentEntry<nEntries-1){
    loadFileEntry(++myCurrentEntry);
  }
  return myCurrentEvent;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<EventTPC> EventSourceROOT::getPreviousEvent(){

  if(myCurrentEntry>0 && nEntries>0){
    loadFileEntry(--myCurrentEntry);
  }
  return myCurrentEvent;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceROOT::loadEventId(unsigned long int iEvent){

  if(!myTree){
    std::cerr<<"ROOT tree not available!"<<std::endl;
    return;
  }
  // assumes that TTree::BuildIndex() was performed while storing myTree
  //loadFileEntry(myTree->GetEntryNumberWithIndex(iEvent));
  
  unsigned long int iEntry = 0;
  while(currentEventNumber()!=iEvent && iEntry<nEntries){  
    loadFileEntry(iEntry);
    ++iEntry;
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceROOT::loadGeometry(const std::string & fileName){

  EventSourceBase::loadGeometry(fileName);
  myPedestalCalculator.SetGeometryAndInitialize(myGeometryPtr);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EventSourceROOT::fillEventFromEventRaw(){

  // sanity checks
  if(readEventType!=EventType::raw) {
    std::cout << KRED << __FUNCTION__
	      << ": Read flag mismatch! "
	      << "Expected EventRaw instead of EventTPC. Entry skipped." << RST << std::endl;
    return;
  }
  if(aPtrEventInfo==NULL || aPtrEventData==NULL) {
    std::cout << KRED << __FUNCTION__
	      << ": Undefined TBranch pointers! Entry skipped." << RST << std::endl;
    return;
  }

  // calculate pedestals
  if(removePedestal) myPedestalCalculator.CalculateEventPedestals(myCurrentEventRaw);
  // update event header
  myCurrentEvent->Clear();

#ifdef DEBUG
  std::cout << __FUNCTION__
	    << ": eventId=" << myCurrentEventRaw->GetEventId()
	    << ", time=" << myCurrentEventRaw->GetEventTimestamp() << std::endl;
#endif
  
  myCurrentEvent->SetEventId(myCurrentEventRaw->GetEventId());
  myCurrentEvent->SetEventTime(myCurrentEventRaw->GetEventTimestamp());
  myCurrentEvent->SetGeoPtr(myGeometryPtr);
  
  // loop over AGET chips
  for(auto it=myCurrentEventRaw->data.begin(); it!=myCurrentEventRaw->data.end(); it++) {

    const unsigned int COBO_idx = (unsigned int)(it->first).key1;
    const unsigned int ASAD_idx = (unsigned int)(it->first).key2;
    const unsigned int AGET_idx = (unsigned int)(it->first).key3;

    // sanity checks
    if(ASAD_idx >= (unsigned int)myGeometryPtr->GetAsadNboards()){
      std::cout << KRED << __FUNCTION__
		<<": Data format mismatch! ASAD="<<ASAD_idx
		<<", number of ASAD boards in geometry="<<myGeometryPtr->GetAsadNboards()
		<<". Entry skipped."
		<< RST << std::endl;
      return;
    }


    // loop over AGET channels
    const eventraw::AgetRaw araw=it->second;
    unsigned int CHANNEL_idx = 0; // channel NUMBER
    unsigned int idx=0; // channelData vector index

    for(auto it2=araw.channelMask.begin(); it2!=araw.channelMask.end(); it2++) {

      const unsigned char bitmask=*it2;
      for(auto bit=0; bit<8; bit++) {

	if ( (bitmask>>bit)&1 ) { // raw channel CHANNEL_idx is present

	  // loop over TIME CELLS of a given channel
	  const eventraw::ChannelRaw craw=araw.channelData[idx];
	  unsigned int TIMECELL_idx = 0; // time cell NUMBER
	  unsigned int idx2=0; // cellData vector index 
	  
	  for(auto it3=craw.cellMask.begin(); it3!=craw.cellMask.end(); it3++) {

	    const unsigned char bitmask=*it3;
	    for(auto bit=0; bit<8; bit++) {
	      if ( (bitmask>>bit)&1 ) { // cell TIMECELL_idx is present

		if(TIMECELL_idx<2 || TIMECELL_idx>509 ||
		   TIMECELL_idx<(unsigned int)myPedestalCalculator.GetMinSignalCell() ||
		   TIMECELL_idx>(unsigned int)myPedestalCalculator.GetMaxSignalCell()) continue;
		// get stored value
		Double_t corrVal = craw.cellData[idx2];

		// subtract pedestal
		if(removePedestal){
		  corrVal -= myPedestalCalculator.GetPedestalCorrection(COBO_idx, ASAD_idx, AGET_idx, CHANNEL_idx, TIMECELL_idx);
		}

		// add new hit to EventTPC
		myCurrentEvent->AddValByAgetChannel(COBO_idx, ASAD_idx, AGET_idx, CHANNEL_idx, TIMECELL_idx, corrVal);
		
		idx2++; // set next cellData index
	      } // if ((bitmask...
	      TIMECELL_idx++; // set next time cell NUMBER
	    } // scan of CELL mask bits
	  } // scan of CELL mask bytes
	  idx++; // set next channelData index
	} // if ((bitmask...
	CHANNEL_idx++; // set next channel NUMBER
      } // scan of CHANNEL mask bits
    } // scan of CHANNEL mask bytes

    /*
#ifdef DEBUG
    ////// DEBUG
    std::shared_ptr<TProfile> tp=myPedestalCalculator.GetPedestalProfilePerAsad(COBO_idx, ASAD_idx);
    std::cout << __FUNCTION__ << ": TProfile[Cobo=" << COBO_idx
	      << ", Asad=" << ASAD_idx
	      << "]=" << tp->GetName()
	      << std::endl << std::flush;
    ////// DEBUG
#endif
    */  
    
  } // loop over AGET chips

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
