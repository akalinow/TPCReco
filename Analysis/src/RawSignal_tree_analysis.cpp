#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

#include "TFile.h"
#include "TTree.h"

#include "GeometryTPC.h"
#include "EventInfo.h"
#include "RawSignal_tree_analysis.h"
#include "RawSignal_tree_dataFormat.h"

#include "colorText.h"

///////////////////////////////
///////////////////////////////
RawSignal_tree_analysis::RawSignal_tree_analysis(std::shared_ptr<GeometryTPC> aGeometryPtr,
						 ClusterConfig &aClusterConfig) { // definition of LAB detector coordinates
  setGeometry(aGeometryPtr);
  myClusterConfig = aClusterConfig;
  initialize();
}
///////////////////////////////
///////////////////////////////
RawSignal_tree_analysis::~RawSignal_tree_analysis(){
  finalize();
}
///////////////////////////////
///////////////////////////////
void RawSignal_tree_analysis::initialize(){

  std::string fileName = "RawSignalTree.root";
  std::string treeName = "RawSignal";
  OutputFilePtr = std::make_shared<TFile>(fileName.c_str(),"RECREATE");
  if(!OutputFilePtr) {
    std::cout<<KRED<<"RawSignal_tree_analysis::open: Cannot create new ROOT file: "<<RST<<fileName
	     <<std::endl;
    return;
  }
  OutputFilePtr->cd();
  OutputTreePtr = std::make_shared<TTree>(treeName.c_str(),"");
  OutputTreePtr->Branch("data", &event_rawsignal_);

  std::cout << __FUNCTION__ << ": TTree current TFile ptr=" << OutputTreePtr->GetCurrentFile() << std::endl;
}
///////////////////////////////
///////////////////////////////
void RawSignal_tree_analysis::finalize(){
  if(!OutputFilePtr){
    std::cout<<KRED<<"RawSignal_tree_analysis::close: "<<RST
	     <<" pointer to output file not set!"
	     <<std::endl;
    return;
  }

  std::cout << __FUNCTION__ << ": TTree current TFile ptr=" << OutputTreePtr->GetCurrentFile() << std::endl;

  //  TFile *f=OutputTreePtr->GetCurrentFile();
  //  if(f) {
  //    f->Write("", TObject::kOverwrite);
  //    f->Close();
  //  }
  OutputFilePtr->cd();
  OutputTreePtr->Write("", TObject::kOverwrite);
  OutputFilePtr->Close();
}
///////////////////////////////
///////////////////////////////
void RawSignal_tree_analysis::fillTree(std::shared_ptr<EventTPC> aEventTPC){ // eventraw::EventInfo *aEventInfo){

  if(!OutputTreePtr){
    std::cout<<KRED<<"RawSignal_tree_analysis::fillTree"<<RST
	     <<" pointer to output tree not set!"
	     <<std::endl;
    return;
  }

  // FILL EVENT INFO
  
  static double last_timestamp = 0;
  event_rawsignal_->runID=(aEventTPC ? aEventTPC->GetRunId() : -1); // run number
  event_rawsignal_->eventID=(aEventTPC ? aEventTPC->GetEventId() : -1); // event number
  event_rawsignal_->unixTimeSec=(aEventTPC ? getUnixTimestamp( aEventTPC->GetRunId(), aEventTPC->GetEventTime() ) : -1); // absolute Unix time [s]
  event_rawsignal_->elapsedTimeSec=(aEventTPC ? (double)(aEventTPC->GetEventTime()*10.0e-9) : -1); // [s] converted from GET electronics timestamp (10ns units) to seconds
  event_rawsignal_->deltaTimeSec=(double)(aEventTPC ? aEventTPC->GetEventTime() : 0)*10.0e-9 - last_timestamp; // [s] time difference for rate measurements
  last_timestamp=event_rawsignal_->elapsedTimeSec;

  // FILL CLUSTER PARAMETERS
  
  event_rawsignal_->clusterFlag = myClusterConfig.clusterEnable; // is clustering enabled?
  event_rawsignal_->clusterThr = myClusterConfig.clusterThreshold; // clustering threshold in ADC units
  event_rawsignal_->clusterDeltaStrips = myClusterConfig.clusterDeltaStrips; // cluster size in +/- strip units 
  event_rawsignal_->clusterDeltaTimeCells = myClusterConfig.clusterDeltaTimeCells; // cluster size in +/- time cell units

  // MAKE A CLUSTER AND COMPUTE SOME STATISTICS

  if(myClusterConfig.clusterEnable) { // CLUSTER
    aEventTPC->MakeOneCluster(myClusterConfig.clusterThreshold, myClusterConfig.clusterDeltaStrips, myClusterConfig.clusterDeltaTimeCells);
    event_rawsignal_->nHits = aEventTPC->GetOneCluster().GetNhits(); // cluster size in time-strip hits
    event_rawsignal_->totalCharge = aEventTPC->GetOneCluster().GetTotalCharge(); // total charge integral from all strips
    event_rawsignal_->maxCharge = aEventTPC->GetOneCluster().GetMaxCharge(); // maximal pulse-height from all strips
    for(int strip_dir=0; strip_dir<3; strip_dir++) {
      event_rawsignal_->totalChargePerDir[strip_dir] = aEventTPC->GetOneCluster().GetTotalCharge(strip_dir); // charge integral per strip direction
      event_rawsignal_->maxChargePerDir[strip_dir] = aEventTPC->GetOneCluster().GetMaxCharge(strip_dir); // maximal pulse-height per strip direction
      bool err=false;
      event_rawsignal_->maxChargePositionPerDir[strip_dir] = myGeometryPtr->Strip2posUVW(strip_dir, aEventTPC->GetOneCluster().GetMaxChargeStrip(strip_dir), err); // [mm] postion of the maximal pulse-height per strip direction
    }
  } else { // WHOLE EVENT
    event_rawsignal_->nHits = aEventTPC->GetNhits(); // event size in time-strip hits
    event_rawsignal_->totalCharge = aEventTPC->GetTotalCharge(); // total charge integral from all strips
    event_rawsignal_->maxCharge = aEventTPC->GetMaxCharge(); // maximal pulse-height from all strips
    for(int strip_dir=0; strip_dir<3; strip_dir++) {
      event_rawsignal_->totalChargePerDir[strip_dir] = aEventTPC->GetTotalCharge(strip_dir); // charge integral per strip direction
      event_rawsignal_->maxChargePerDir[strip_dir] = aEventTPC->GetMaxCharge(strip_dir); // maximal pulse-height per strip direction
      bool err=false;
      event_rawsignal_->maxChargePositionPerDir[strip_dir] = myGeometryPtr->Strip2posUVW(strip_dir, aEventTPC->GetMaxChargeStrip(strip_dir), err); // [mm] postion of the maximal pulse-height per strip direction
    }
  }

  // update tree data
  OutputTreePtr->Fill();
} 
///////////////////////////////
///////////////////////////////
void RawSignal_tree_analysis::setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr){
  myGeometryPtr = aGeometryPtr;
  if(!myGeometryPtr) {
    std::cout<<KRED<<"RawSignal_tree_analysis::setGeometry: "<<RST
	     <<" pointer to TPC geometry not set!"
	     <<std::endl;
    exit(-1);
  }
}
///////////////////////////////
///////////////////////////////
double RawSignal_tree_analysis::getUnixTimestamp(time_t run_id, uint64_t elapsed_time_10ns){
  // Combines run ID with relative CoBo timestamp (10ns units)
  // to produce Unix time stamp.
  // NOTE: time zone is defined by runID convention (local time in most cases).

  // decode run ID from YYYYMMDDhhmmss decimal format
  auto year=(int)(run_id*1e-10);
  auto month=(int)(fmod(run_id, year*1e10)*1e-8);
  auto day=(int)(fmod(run_id, year*1e10+month*1e8)*1e-6);
  auto hour=(int)(fmod(run_id, year*1e10+month*1e8+day*1e6)*1e-4);
  auto minute=(int)(fmod(run_id, year*1e10+month*1e8+day*1e6+hour*1e4)*1e-2);
  auto sec=(int)(fmod(run_id, year*1e10+month*1e8+day*1e6+hour*1e4+minute*1e2)); 

  // create event Unix time point with millisecond precision
  std::tm tm{};
  tm.tm_year = year - 1900;
  tm.tm_mon = month - 1;
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min = minute;
  tm.tm_sec = sec;
  tm.tm_isdst = -1;
  return (double)(std::mktime(&tm) + elapsed_time_10ns*1e-5);
}
