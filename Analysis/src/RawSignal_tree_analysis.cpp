#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

#include <boost/property_tree/ptree.hpp>

#include <TFile.h>
#include <TTree.h>

#include "TPCReco/GeometryTPC.h"
#include "TPCReco/EventInfo.h"
#include "TPCReco/RawSignal_tree_analysis.h"
#include "TPCReco/RawSignal_tree_dataFormat.h"

#include "TPCReco/colorText.h"

using std::chrono::duration;
using std::chrono::duration_cast;

///////////////////////////////
///////////////////////////////
RawSignal_tree_analysis::RawSignal_tree_analysis(std::shared_ptr<GeometryTPC> aGeometryPtr,
						 ClusterConfig &aClusterConfig,
                                                 std::string aOutputFileName)
{ // definition of LAB detector coordinates
  setGeometry(aGeometryPtr);
  myClusterConfig = aClusterConfig;
  myOutputFileName = aOutputFileName;
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

  std::string treeName = "RawSignal";
  myOutputFilePtr = std::make_shared<TFile>(myOutputFileName.c_str(),"RECREATE");
  if(!myOutputFilePtr) {
    std::cout<<KRED<<"RawSignal_tree_analysis::open: Cannot create new ROOT file: "<<RST<<myOutputFileName
	     <<std::endl;
    return;
  }
  myOutputFilePtr->cd();
  myOutputTreePtr = std::make_shared<TTree>(treeName.c_str(),"");
  myOutputTreePtr->Branch("data", &event_rawsignal_);

  std::cout << __FUNCTION__ << ": TTree current TFile ptr=" << myOutputTreePtr->GetCurrentFile() << std::endl;
}
///////////////////////////////
///////////////////////////////
void RawSignal_tree_analysis::finalize(){
  if(!myOutputFilePtr){
    std::cout<<KRED<<"RawSignal_tree_analysis::close: "<<RST
	     <<" pointer to output file not set!"
	     <<std::endl;
    return;
  }

  std::cout << __FUNCTION__ << ": TTree current TFile ptr=" << myOutputTreePtr->GetCurrentFile() << std::endl;

  //  TFile *f=myOutputTreePtr->GetCurrentFile();
  //  if(f) {
  //    f->Write("", TObject::kOverwrite);
  //    f->Close();
  //  }
  myOutputFilePtr->cd();
  myOutputTreePtr->Write("", TObject::kOverwrite);
  myOutputFilePtr->Close();
}
///////////////////////////////
///////////////////////////////
void RawSignal_tree_analysis::fillTree(std::shared_ptr<EventTPC> aEventTPC, bool & isFirst){ // eventraw::EventInfo *aEventInfo){

  if(!myOutputTreePtr){
    std::cout<<KRED<<"RawSignal_tree_analysis::fillTree"<<RST
	     <<" pointer to output tree not set!"
	     <<std::endl;
    return;
  }

  // FILL EVENT INFO

  event_rawsignal_->runId=(aEventTPC ? aEventTPC->GetEventInfo().GetRunId() : -1); // run number
  event_rawsignal_->eventId=(aEventTPC ? aEventTPC->GetEventInfo().GetEventId() : -1); // event number
  static double last_timestamp = 0;
  event_rawsignal_->unixTimeSec =
      (aEventTPC
           ? duration_cast<duration<long double>>(
                 tpcreco::eventAbsoluteTime(aEventTPC->GetEventInfo()).time_since_epoch())
                 .count()
           : -1); // absolute Unix time [s]  static double last_timestamp = 0;
  if(isFirst) {
    last_timestamp=event_rawsignal_->unixTimeSec;
    isFirst=false;
  }
  event_rawsignal_->runTimeSec =
      (aEventTPC ? duration_cast<duration<long double>>(
                        tpcreco::eventRelativeTime(aEventTPC->GetEventInfo()))
                        .count()
                  : -1); // [s]
  event_rawsignal_->deltaTimeSec=(aEventTPC ? event_rawsignal_->unixTimeSec - last_timestamp : -1); // [s] time difference for rate measurements
  last_timestamp=event_rawsignal_->unixTimeSec;

  // FILL CLUSTERING INPUT PARAMETERS
  
  event_rawsignal_->clusterFlag = myClusterConfig.clusterEnable; // is clustering enabled?
  event_rawsignal_->clusterThr = myClusterConfig.clusterThreshold; // clustering threshold in ADC units for seed hits
  event_rawsignal_->clusterDeltaStrips = myClusterConfig.clusterDeltaStrips; // clustering envelope size in +/- strip units around seed hits 
  event_rawsignal_->clusterDeltaTimeCells = myClusterConfig.clusterDeltaTimeCells; // clustering size in +/- time cell units around seed hits


  filter_type filterType = filter_type::none;
  if(myClusterConfig.clusterEnable) { // CLUSTER
    filterType = filter_type::threshold;
    boost::property_tree::ptree config;
    config.put("recoClusterThreshold", myClusterConfig.clusterThreshold);
    config.put("recoClusterDeltaStrips",myClusterConfig.clusterDeltaStrips);
    config.put("recoClusterDeltaTimeCells",myClusterConfig.clusterDeltaTimeCells);  
    aEventTPC->setHitFilterConfig(filterType, config);    
  }
  
  // MAKE A CLUSTER AND COMPUTE SOME STATISTICS
  event_rawsignal_->nHits = aEventTPC->GetMultiplicity(true, -1, -1, -1, filterType);
  event_rawsignal_->totalCharge = aEventTPC->GetTotalCharge(-1, -1, -1, -1, filterType);
  event_rawsignal_->maxCharge = aEventTPC->GetMaxCharge(-1, -1, -1, filterType); // maximal pulse-height from all strips
    for(int strip_dir=0; strip_dir<3; strip_dir++) {
      event_rawsignal_->nHitsPerDir[strip_dir] = aEventTPC->GetMultiplicity(true, strip_dir, -1, -1, filterType);
      event_rawsignal_->totalChargePerDir[strip_dir] = aEventTPC->GetTotalCharge(strip_dir, -1, -1, -1, filterType); // charge integral per strip direction
      event_rawsignal_->maxChargePerDir[strip_dir] = aEventTPC->GetMaxCharge(strip_dir, -1, -1, filterType); // maximal pulse-height per strip direction
      bool err=false;
      int maxChargeTime = 0, maxChargeStrip = 0;
      std::tie(maxChargeTime, maxChargeStrip) = aEventTPC->GetMaxChargePos(strip_dir, filterType);
      event_rawsignal_->maxChargePositionPerDir[strip_dir] = myGeometryPtr->Strip2posUVW(strip_dir, maxChargeStrip, err); // [mm] postion of the maximal pulse-height per strip direction
      int minStrip = myGeometryPtr->GetDirMinStripMerged(strip_dir),
	maxStrip = myGeometryPtr->GetDirMaxStripMerged(strip_dir),
	minTime = 0,
	maxTime = myGeometryPtr->GetAgetNtimecells()-1;
      std::tie(minTime, maxTime, minStrip, maxStrip) = aEventTPC->GetSignalRange(strip_dir, filterType);
      event_rawsignal_->horizontalWidthPerDir[strip_dir] = fabs( myGeometryPtr->Strip2posUVW(strip_dir, maxStrip, err) - myGeometryPtr->Strip2posUVW(strip_dir, minStrip, err) ) + myGeometryPtr->GetStripPitch(); // [mm] rounded up to full strip pitch
      event_rawsignal_->verticalWidthPerDir[strip_dir] = fabs( myGeometryPtr->Timecell2pos(maxTime, err) - myGeometryPtr->Timecell2pos(minTime, err) ) + myGeometryPtr->GetTimeBinWidth(); // [mm] rounded up to full time cell width

      // compute integrated charge per half-cluster / horizontal (strip domain)
      int mid_strip_num=(int)((minStrip+maxStrip)/2);
      double strip_charge_sum[2]={0,0};
      std::map<int, double> chargePerStrip;
      switch(filterType) {
      case filter_type::none: // NOTE: special case handled differently to speed up calculations
	{
	  auto h=aEventTPC->get1DProjection(static_cast<definitions::projection_type>(strip_dir), filter_type::none, scale_type::raw);
	  h->SetDirectory(0);
	  for(int strip_num=minStrip; strip_num<=maxStrip; strip_num++) {
	    chargePerStrip[strip_num]=h->GetBinContent(h->FindBin(strip_num));
	  }
	}
	break;
      case filter_type::threshold:
	for(int strip_num=minStrip; strip_num<=maxStrip; strip_num++) {
	  chargePerStrip[strip_num]=aEventTPC->GetTotalCharge(strip_dir, -1, strip_num, -1, filterType);
	}
	break;
      default:;
      }
      for(auto & it: chargePerStrip) {
	auto strip_num=it.first;
	auto strip_charge=it.second;
	if(strip_num<mid_strip_num) {
	  strip_charge_sum[0] += strip_charge;
	}
	if(strip_num>mid_strip_num) {
	  strip_charge_sum[1] += strip_charge;
	}
	if(strip_num==mid_strip_num) {
	  strip_charge_sum[0] += 0.5*strip_charge;
	  strip_charge_sum[1] += 0.5*strip_charge;
	}
      }
      event_rawsignal_->horizontalChargePerDirHalf[strip_dir][0]=strip_charge_sum[0];
      event_rawsignal_->horizontalChargePerDirHalf[strip_dir][1]=strip_charge_sum[1];
      // sort in descending order
      if(event_rawsignal_->horizontalChargePerDirHalf[strip_dir][1]>event_rawsignal_->horizontalChargePerDirHalf[strip_dir][0]) {
	std::swap(event_rawsignal_->horizontalChargePerDirHalf[strip_dir][0], event_rawsignal_->horizontalChargePerDirHalf[strip_dir][1]);
      }

      // compute integrated charge per half-cluster / vertical (time domain)
      int mid_time_cell=(int)((minTime+maxTime)/2);
      double time_charge_sum[2]={0,0};
      std::map<int, double> chargePerTimecell;
      switch(filterType) {
      case filter_type::none: // NOTE: special case handled differently to speed up calculations
	{
	  auto h=aEventTPC->get1DProjection(get2DProjectionType(strip_dir), filter_type::none, scale_type::raw);
	  h->SetDirectory(0);
	  for(int time_cell=minTime; time_cell<=maxTime; time_cell++) {
	    chargePerTimecell[time_cell]=h->GetBinContent(h->FindBin(time_cell));
	  }
	}
	break;
      case filter_type::threshold:
	for(int time_cell=minTime; time_cell<=maxTime; time_cell++) {
	  chargePerTimecell[time_cell]=aEventTPC->GetTotalCharge(strip_dir, -1, -1, time_cell, filterType);
	}
	break;
      default:;
      }
      for(auto & it: chargePerTimecell) {
	auto time_cell=it.first;
	auto time_cell_charge=it.second;
	if(time_cell<mid_time_cell) {
	  time_charge_sum[0] += time_cell_charge;
	}
	if(time_cell>mid_time_cell) {
	  time_charge_sum[1] += time_cell_charge;
	}
	if(time_cell==mid_time_cell) {
	  time_charge_sum[1] += 0.5*time_cell_charge;
	  time_charge_sum[1] += 0.5*time_cell_charge;
	}
      }
      event_rawsignal_->verticalChargePerDirHalf[strip_dir][0]=time_charge_sum[0];
      event_rawsignal_->verticalChargePerDirHalf[strip_dir][1]=time_charge_sum[1];
      // sort in descending order
      if(event_rawsignal_->verticalChargePerDirHalf[strip_dir][1]>event_rawsignal_->verticalChargePerDirHalf[strip_dir][0]) {
	std::swap(event_rawsignal_->verticalChargePerDirHalf[strip_dir][0], event_rawsignal_->verticalChargePerDirHalf[strip_dir][1]);
      }
    }

    // update tree data
    myOutputTreePtr->Fill();
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
