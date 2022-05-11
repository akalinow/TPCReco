#include <vector>
#include <string>
#include <iostream>

#include "TFile.h"
#include "TTree.h"
#include "TVector3.h"

#include "GeometryTPC.h"
#include "Track3D.h"
#include "EventInfo.h"
#include "HIGS_trees_analysis.h"

#include "colorText.h"

///////////////////////////////
///////////////////////////////
HIGS_trees_analysis::HIGS_trees_analysis(std::shared_ptr<GeometryTPC> aGeometryPtr, // definition of LAB detector coordinates
			       float beamEnergy,   // nominal gamma beam energy [MeV] in detector LAB frame
			       TVector3 beamDir) { // nominal gamma beam direction in detector LAB frame
  setGeometry(aGeometryPtr);
  setBeamProperties(beamEnergy, beamDir);
  open();
  
}
///////////////////////////////
///////////////////////////////
HIGS_trees_analysis::~HIGS_trees_analysis(){
  Output1prongTreePtr->Write("", TObject::kOverwrite);
  Output2prongTreePtr->Write("", TObject::kOverwrite);
  Output3prongTreePtr->Write("", TObject::kOverwrite);
  close();
  
}


void HIGS_trees_analysis::open(){

  std::string fileName = "Trees.root";
  
  std::string tree1Name = "Tree_1prong_events";
  std::string tree2Name = "Tree_2prong_events";
  std::string tree3Name = "Tree_3prong_events";
  OutputFilePtr = std::make_shared<TFile>(fileName.c_str(),"RECREATE");
  if(!OutputFilePtr) {
    std::cout<<KRED<<"HIGS_trees_analysis::open: Cannot create new ROOT file: "<<RST<<fileName
	     <<std::endl;
    return;
  }
  Output1prongTreePtr = std::make_shared<TTree>(tree1Name.c_str(),"");
  Output2prongTreePtr = std::make_shared<TTree>(tree2Name.c_str(),"");
  Output3prongTreePtr = std::make_shared<TTree>(tree3Name.c_str(),"");
  
  Output1prongTreePtr->Branch("data", &event1prong_);
  Output2prongTreePtr->Branch("data", &event2prong_);
  Output3prongTreePtr->Branch("data", &event3prong_);
    
}
///////////////////////////////
///////////////////////////////
void HIGS_trees_analysis::close(){
  if(!OutputFilePtr){
    std::cout<<KRED<<"HIGS_trees_analysis::close: "<<RST
	     <<" pointer to output file not set!"
	     <<std::endl;
    return;
  }
  Output2prongTreePtr->Write("", TObject::kOverwrite);
  OutputFilePtr->Close();
}
///////////////////////////////
///////////////////////////////
void HIGS_trees_analysis::setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr){
  myGeometryPtr = aGeometryPtr;
}
///////////////////////////////
///////////////////////////////
void HIGS_trees_analysis::setBeamProperties(float beamEnergy,   // nominal gamma beam energy [MeV] in detector LAB frame
					    TVector3 beamDir) { // nominal gamma beam direction in detector LAB frame
  gammaEnergy = fabs(beamEnergy);
  gammaUnitVec = beamDir.Unit();
}
///////////////////////////////
///////////////////////////////
void HIGS_trees_analysis::fillTrees(Track3D *aTrack, eventraw::EventInfo *aEventInfo){
  
  // The following assumptions are made:
  // - event is a collection of straight 3D segments
  // - 3D segments share common vertex (STARTING POINT of each segment)
  // - for 2-prong events: longer track is ALPHA, shorter is CARBON
  // - for 3-prong events: all tracks are ALPHAS, descending order by their energy/length
  
  const int ntracks = aTrack->getSegments().size();
  if (ntracks==0) return;

  if (ntracks==1)
      fillTrees1prong(aTrack, aEventInfo);
  
  if (ntracks==2)
      fillTrees2prong(aTrack, aEventInfo);
  
  if (ntracks==3)
      fillTrees3prong(aTrack, aEventInfo);  
}
///////////////////////////////
///////////////////////////////
void HIGS_trees_analysis::fillTrees1prong(Track3D *aTrack, eventraw::EventInfo *aEventInfo){

  if(!Output1prongTreePtr){
    std::cout<<KRED<<"HIGS_trees_analysis::fillTrees1prong"<<RST
	     <<" pointer to 1 prong output tree not set!"
	     <<std::endl;
    return;
  }
  
  TrackSegment3DCollection list = aTrack->getSegments();
  auto track=list.front();
  static double last_timestamp = 0;
  event1prong_->runID=(aEventInfo ? aEventInfo->GetRunId() : -1);
  event1prong_->eventID=(aEventInfo ? aEventInfo->GetEventId() : -1);
  event1prong_->timestamp=(aEventInfo ? aEventInfo->GetEventTimestamp() : -1);
  event1prong_->delta_timestamp=( (double)(aEventInfo ? aEventInfo->GetEventTimestamp() : 0)-last_timestamp)*1e-8; // sec 
  last_timestamp=event1prong_->timestamp;
  event1prong_->vertexPos = list.front().getStart();
  event1prong_->endPos = track.getEnd();
  event1prong_->length = track.getLength(); // [mm]
  event1prong_->phiDET = track.getTangent().Phi();
  event1prong_->cosThetaDET = cos(track.getTangent().Theta());
  event1prong_->phiBEAM = atan2(track.getTangent().Z(), -track.getTangent().Y()); // [rad], azimuthal angle from horizontal axis
  event1prong_->cosThetaBEAM = track.getTangent()*gammaUnitVec;; // cos of polar angle wrt beam axis
  
  Output1prongTreePtr->Fill();
}
///////////////////////////////
///////////////////////////////
void HIGS_trees_analysis::fillTrees2prong(Track3D *aTrack, eventraw::EventInfo *aEventInfo){

  if(!Output2prongTreePtr){
    std::cout<<KRED<<"HIGS_trees_analysis::fillTrees"<<RST
	     <<" pointer to 2 prong output tree not set!"
	     <<std::endl;
    return;
  }

  // get sorted list of tracks (descending order by track length)
  TrackSegment3DCollection list = aTrack->getSegments();
  std::sort(list.begin(), list.end(),
	    [](const TrackSegment3D& a, const TrackSegment3D& b) {
	      return a.getLength() > b.getLength();
	    });
  static double last_timestamp = 0;
  event2prong_->runID=(aEventInfo ? aEventInfo->GetRunId() : -1);
  event2prong_->eventID=(aEventInfo ? aEventInfo->GetEventId() : -1);
  event2prong_->timestamp=(aEventInfo ? aEventInfo->GetEventTimestamp() : -1);
  event2prong_->delta_timestamp=( (double)(aEventInfo ? aEventInfo->GetEventTimestamp() : 0)-last_timestamp)*1e-8; // sec 
  last_timestamp=event2prong_->timestamp;
  event2prong_->vertexPos = list.front().getStart();
  event2prong_->alpha_endPos = list.front().getEnd();
  event2prong_->alpha_length = list.front().getLength(); // longest = alpha
  event2prong_->alpha_phiDET = list.front().getTangent().Phi();
  event2prong_->alpha_cosThetaDET = cos(list.front().getTangent().Theta());
  event2prong_->alpha_phiBEAM = atan2(list.front().getTangent().Z(), -list.front().getTangent().Y()); // [rad], azimuthal angle from horizontal axis;
  event2prong_->alpha_cosThetaBEAM = list.front().getTangent()*gammaUnitVec; // polar angle wrt beam axis
  event2prong_->carbon_endPos = list.back().getEnd();
  event2prong_->carbon_length = list.back().getLength(); // shortest = carbon
  event2prong_->carbon_phiDET = list.back().getTangent().Phi();
  event2prong_->carbon_cosThetaDET = cos(list.back().getTangent().Theta());
  event2prong_->carbon_phiBEAM = atan2(list.back().getTangent().Z(), -list.back().getTangent().Y()); // [rad], azimuthal angle from horizontal axis
  event2prong_->carbon_cosThetaBEAM = list.back().getTangent()*gammaUnitVec; // polar angle wrt beam axis
  
  Output2prongTreePtr->Fill();
}
///////////////////////////////
///////////////////////////////
void HIGS_trees_analysis::fillTrees3prong(Track3D *aTrack, eventraw::EventInfo *aEventInfo){

  if(!Output3prongTreePtr){
    std::cout<<KRED<<"HIGS_trees_analysis::fillTrees"<<RST
	     <<" pointer to 3 prong output tree not set!"
	     <<std::endl;
    return;
  }

  // get sorted list of tracks (descending order by track length)
  TrackSegment3DCollection list = aTrack->getSegments();
  std::sort(list.begin(), list.end(),
	    [](const TrackSegment3D& a, const TrackSegment3D& b) {
	      return a.getLength() > b.getLength();
	    });  
  static double last_timestamp = 0;
  event3prong_->runID=(aEventInfo ? aEventInfo->GetRunId() : -1);
  event3prong_->eventID=(aEventInfo ? aEventInfo->GetEventId() : -1);
  event3prong_->timestamp=(aEventInfo ? aEventInfo->GetEventTimestamp() : -1);
  event3prong_->delta_timestamp=( (double)(aEventInfo ? aEventInfo->GetEventTimestamp() : 0)-last_timestamp)*1e-8; // sec 
  last_timestamp=event3prong_->timestamp;
  event3prong_->vertexPos = list.front().getStart();
  auto track1=list.at(0);
  event3prong_->alpha1_endPos = track1.getEnd();
  event3prong_->alpha1_length = track1.getLength(); // longest alpha
  event3prong_->alpha1_phiDET = track1.getTangent().Phi();
  event3prong_->alpha1_cosThetaDET = cos(track1.getTangent().Theta());
  event3prong_->alpha1_phiBEAM = atan2(track1.getTangent().Z(), -track1.getTangent().Y()); // [rad], azimuthal angle from horizontal axis;
  event3prong_->alpha1_cosThetaBEAM = track1.getTangent()*gammaUnitVec; // cos polar angle wrt beam axis
  auto track2=list.at(1);
  event3prong_->alpha2_endPos = track2.getEnd();
  event3prong_->alpha2_length = track2.getLength(); // middle alpha
  event3prong_->alpha2_phiDET = track2.getTangent().Phi();
  event3prong_->alpha2_cosThetaDET = cos(track2.getTangent().Theta());
  event3prong_->alpha2_phiBEAM = atan2(track2.getTangent().Z(), -track2.getTangent().Y()); // [rad], azimuthal angle from horizontal axis;
  event3prong_->alpha2_cosThetaBEAM = track2.getTangent()*gammaUnitVec; // cos polar angle wrt beam axis
  auto track3=list.at(2);
  event3prong_->alpha3_endPos = track3.getEnd();
  event3prong_->alpha3_length = track3.getLength(); // shortest alpha
  event3prong_->alpha3_phiDET = track3.getTangent().Phi();
  event3prong_->alpha3_cosThetaDET = cos(track3.getTangent().Theta());
  event3prong_->alpha3_phiBEAM = atan2(track3.getTangent().Z(), -track3.getTangent().Y()); // [rad], azimuthal angle from horizontal axis;
  event3prong_->alpha3_cosThetaBEAM = track3.getTangent()*gammaUnitVec; // polar angle wrt beam axis
  
  Output3prongTreePtr->Fill();
}
