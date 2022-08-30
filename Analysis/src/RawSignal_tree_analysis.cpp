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
  event_rawsignal_->timestamp=(aEventTPC ? aEventTPC->GetEventTime() : -1); // GET electronics timestamp in 10ns units
  event_rawsignal_->delta_timestamp=( (double)(aEventTPC ? aEventTPC->GetEventTime() : 0)-last_timestamp)*1e-8; // in seconds 
  last_timestamp=event_rawsignal_->timestamp;

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
