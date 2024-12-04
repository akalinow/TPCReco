#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

#include <TFile.h>
#include <TTree.h>
#include <TVector3.h>
#include <TH2Poly.h>

#include "TPCReco/GeometryTPC.h"
#include "TPCReco/Track3D.h"
#include "TPCReco/TrackSegment3D.h"
#include "TPCReco/EventInfo.h"
#include "TPCReco/HIGS_trees_analysis.h"

#include "TPCReco/colorText.h"

using std::chrono::duration;
using std::chrono::duration_cast;

///////////////////////////////
///////////////////////////////
HIGS_trees_analysis::HIGS_trees_analysis(std::shared_ptr<GeometryTPC> aGeometryPtr, // definition of LAB detector coordinates
					 float beamEnergy,   // nominal gamma beam energy [MeV] in detector LAB frame
					 TVector3 beamDir){   // nominal gamma beam direction in detector LAB frame

  setGeometry(aGeometryPtr);
  setBeamProperties(beamEnergy, beamDir);
  open();
  /*
  auto gback=(TGraph*)(xyArea.back().bin->GetPolygon());
  std::cout << "CONSTRUCTOR AFTER: graph 10mm: TH2PolyBin.GetPolygon=" << gback
	    <<  " (npoints=" << (gback ? gback->GetN() : 0)
	    << ")" << std::endl;

  auto zback=zRange.back();
  std::cout << "CONSTRUCTOR AFTER: Zrange 10mm: drift_cage=" << zback.lengthLimit
	    << ", lowerLimit=" << zback.lowerLimit << ", upperLimit=" << zback.upperLimit
	    << std::endl;
  */
}
///////////////////////////////
///////////////////////////////
HIGS_trees_analysis::~HIGS_trees_analysis(){
  Output1prongTreePtr->Write("", TObject::kOverwrite);
  Output2prongTreePtr->Write("", TObject::kOverwrite);
  Output3prongTreePtr->Write("", TObject::kOverwrite);
  close();
}
///////////////////////////////
///////////////////////////////
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
  if(!myGeometryPtr) {
    std::cout<<KRED<<"HIGS_trees_analysis::setGeometry: "<<RST
	     <<" pointer to TPC geometry not set!"
	     <<std::endl;
    exit(-1);
  }
  xyArea.push_back(HorizontalCheck(myGeometryPtr, 2.0)); // 2 mm safety band
  xyArea.push_back(HorizontalCheck(myGeometryPtr, 5.0)); // 5 mm safety band
  xyArea.push_back(HorizontalCheck(myGeometryPtr, 10.0)); // 10 mm safety band

  zRange.push_back(VerticalCheck(myGeometryPtr, 2.0)); // 2 mm safety band
  zRange.push_back(VerticalCheck(myGeometryPtr, 5.0)); // 5 mm safety band
  zRange.push_back(VerticalCheck(myGeometryPtr, 10.0)); // 10 mm safety band
}
///////////////////////////////
///////////////////////////////
void HIGS_trees_analysis::setBeamProperties(float beamEnergy,   // nominal gamma beam energy [MeV] in detector LAB frame
					    TVector3 beamDir) { // nominal gamma beam direction in detector LAB frame
  gammaEnergy = fabs(beamEnergy);
  gammaUnitVec = beamDir.Unit();
}
//////////////////////////
//////////////////////////
void HIGS_trees_analysis::setIonRangeCalculator(const IonRangeCalculator & aIonRangeCalculator){ 

  myIonRangeCalculator = aIonRangeCalculator;
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

  static bool isFirst_1prong=true;
  if (ntracks==1)
    fillTrees1prong(aTrack, aEventInfo, isFirst_1prong);
  
  static bool isFirst_2prong=true;
  if (ntracks==2)
    fillTrees2prong(aTrack, aEventInfo, isFirst_2prong);
  
  static bool isFirst_3prong=true;
  if (ntracks==3)
    fillTrees3prong(aTrack, aEventInfo, isFirst_3prong);  
}
///////////////////////////////
///////////////////////////////
void HIGS_trees_analysis::fillTrees1prong(Track3D *aTrack, eventraw::EventInfo *aEventInfo, bool & isFirst){

  if(!Output1prongTreePtr){
    std::cout<<KRED<<"HIGS_trees_analysis::fillTrees1prong"<<RST
	     <<" pointer to 1 prong output tree not set!"
	     <<std::endl;
    return;
  }
  
  TrackSegment3DCollection list = aTrack->getSegments();
  auto track=list.front();
  event1prong_->runId=(aEventInfo ? aEventInfo->GetRunId() : -1);
  event1prong_->eventId=(aEventInfo ? aEventInfo->GetEventId() : -1);
  event1prong_->eventType = aEventInfo ? aEventInfo->GetEventType().to_ulong() : 0;
  event1prong_->unixTimeSec =
      (aEventInfo
           ? duration_cast<duration<long double>>(
                 tpcreco::eventAbsoluteTime(*aEventInfo).time_since_epoch())
                 .count()
           : -1); // absolute Unix time [s]
  // NOTE: for irregular runs with runId=[0, 999999] (6 figures) the parser returns absolute Unix time = 0
  static double last_timestamp = 0;
  if(isFirst) {
    last_timestamp=event1prong_->unixTimeSec;
    isFirst=false;
  }
  event1prong_->runTimeSec =
      (aEventInfo ? duration_cast<duration<long double>>(
                        tpcreco::eventRelativeTime(*aEventInfo))
                        .count()
                  : -1); // [s]
  event1prong_->deltaTimeSec= (aEventInfo ? event1prong_->unixTimeSec - last_timestamp : -1); // [s] time difference for rate measurements
  last_timestamp=event1prong_->unixTimeSec;

  event1prong_->vertexPos = list.front().getStart();
  event1prong_->endPos = track.getEnd();
  event1prong_->length = track.getLength(); // [mm]
  event1prong_->phiDET = track.getTangent().Phi();
  event1prong_->cosThetaDET = cos(track.getTangent().Theta());
  event1prong_->phiBEAM = atan2(-track.getTangent().Z(), track.getTangent().Y()); // [rad], azimuthal angle from horizontal axis
  event1prong_->cosThetaBEAM = track.getTangent()*gammaUnitVec;; // cos of polar angle wrt beam axis
  event1prong_->chargeProfile = track.getChargeProfile();

  // check containment in Z_DET
  event1prong_->Zmargin2mm = true;
  event1prong_->Zmargin5mm = true;
  event1prong_->Zmargin10mm = true;
  std::for_each(list.begin(), list.end(),
		[&](const TrackSegment3D& a) mutable {
		  event1prong_->Zmargin2mm &= zRange[0].IsInside(a.getStart().Z(), a.getEnd().Z());
		  event1prong_->Zmargin5mm &= zRange[1].IsInside(a.getStart().Z(), a.getEnd().Z());
		  event1prong_->Zmargin10mm &= zRange[2].IsInside(a.getStart().Z(), a.getEnd().Z());
		});
  
  // check containment in XY_DET
  event1prong_->XYmargin2mm = true;
  event1prong_->XYmargin5mm = true;
  event1prong_->XYmargin10mm = true;
  std::for_each(list.begin(), list.end(),
		[&](const TrackSegment3D& a) mutable {
		  event1prong_->XYmargin2mm &=
		    xyArea[0].IsInside(a.getStart().X(), a.getStart().Y()) &
		    xyArea[0].IsInside(a.getEnd().X(), a.getEnd().Y());
		  event1prong_->XYmargin5mm &=
		    xyArea[1].IsInside(a.getStart().X(), a.getStart().Y()) &
		    xyArea[1].IsInside(a.getEnd().X(), a.getEnd().Y());
		  event1prong_->XYmargin10mm &=
		    xyArea[2].IsInside(a.getStart().X(), a.getStart().Y()) &
		    xyArea[2].IsInside(a.getEnd().X(), a.getEnd().Y());
		});

  Output1prongTreePtr->Fill();
}
///////////////////////////////
///////////////////////////////
void HIGS_trees_analysis::fillTrees2prong(Track3D *aTrack, eventraw::EventInfo *aEventInfo, bool & isFirst){

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

  event2prong_->runId=(aEventInfo ? aEventInfo->GetRunId() : -1);
  event2prong_->eventId=(aEventInfo ? aEventInfo->GetEventId() : -1);
  event2prong_->eventType = aEventInfo ? aEventInfo->GetEventType().to_ulong() : 0;
  event2prong_->unixTimeSec =
      (aEventInfo
           ? duration_cast<duration<long double>>(
                 tpcreco::eventAbsoluteTime(*aEventInfo).time_since_epoch())
                 .count()
           : -1); // absolute Unix time [s]
  // NOTE: for irregular runs with runId=[0, 999999] (6 figures) the parser returns absolute Unix time = 0
  static double last_timestamp = 0;
  if(isFirst) {
    last_timestamp=event2prong_->unixTimeSec;
    isFirst=false;
  }
  event2prong_->runTimeSec =
      (aEventInfo ? duration_cast<duration<long double>>(
                        tpcreco::eventRelativeTime(*aEventInfo))
                        .count()
                  : -1); // [s]
  event2prong_->deltaTimeSec=(aEventInfo ? event2prong_->unixTimeSec - last_timestamp : -1); // [s] time difference for rate measurements
  last_timestamp=event2prong_->unixTimeSec;

  event2prong_->vertexPos = list.front().getStart();
  event2prong_->alpha_endPos = list.front().getEnd();
  event2prong_->alpha_length = list.front().getLength(); // longest = alpha
  event2prong_->alpha_energy = myIonRangeCalculator.getIonEnergyMeV(pid_type::ALPHA, list.front().getLength()); // [MeV]
  event2prong_->alpha_phiDET = list.front().getTangent().Phi();
  event2prong_->alpha_cosThetaDET = cos(list.front().getTangent().Theta());
  event2prong_->alpha_phiBEAM = atan2(-list.front().getTangent().Z(), list.front().getTangent().Y()); // [rad], azimuthal angle from horizontal axis;
  event2prong_->alpha_cosThetaBEAM = list.front().getTangent()*gammaUnitVec; // polar angle wrt beam axis
  event2prong_->alpha_chargeProfile = list.front().getChargeProfile();

  event2prong_->carbon_endPos = list.back().getEnd();
  event2prong_->carbon_length = list.back().getLength(); // shortest = carbon
  event2prong_->carbon_energy = myIonRangeCalculator.getIonEnergyMeV(pid_type::CARBON_12, list.back().getLength()); // [MeV]
  event2prong_->carbon_phiDET = list.back().getTangent().Phi();
  event2prong_->carbon_cosThetaDET = cos(list.back().getTangent().Theta());
  event2prong_->carbon_phiBEAM = atan2(-list.back().getTangent().Z(), list.back().getTangent().Y()); // [rad], azimuthal angle from horizontal axis
  event2prong_->carbon_cosThetaBEAM = list.back().getTangent()*gammaUnitVec; // polar angle wrt beam axis
  event2prong_->carbon_chargeProfile = list.back().getChargeProfile();

  // check containment in Z_DET
  event2prong_->Zmargin2mm = true;
  event2prong_->Zmargin5mm = true;
  event2prong_->Zmargin10mm = true;
  std::for_each(list.begin(), list.end(),
		[&](const TrackSegment3D& a) mutable {
		  event2prong_->Zmargin2mm &= zRange[0].IsInside(a.getStart().Z(), a.getEnd().Z());
		  event2prong_->Zmargin5mm &= zRange[1].IsInside(a.getStart().Z(), a.getEnd().Z());
		  event2prong_->Zmargin10mm &= zRange[2].IsInside(a.getStart().Z(), a.getEnd().Z());
		});
  
  // check containment in XY_DET
  event2prong_->XYmargin2mm = true;
  event2prong_->XYmargin5mm = true;
  event2prong_->XYmargin10mm = true;
  std::for_each(list.begin(), list.end(),
		[&](const TrackSegment3D& a) mutable {
		  event2prong_->XYmargin2mm &=
		    xyArea[0].IsInside(a.getStart().X(), a.getStart().Y()) &
		    xyArea[0].IsInside(a.getEnd().X(), a.getEnd().Y());
		  event2prong_->XYmargin5mm &=
		    xyArea[1].IsInside(a.getStart().X(), a.getStart().Y()) &
		    xyArea[1].IsInside(a.getEnd().X(), a.getEnd().Y());
		  event2prong_->XYmargin10mm &=
		    xyArea[2].IsInside(a.getStart().X(), a.getStart().Y()) &
		    xyArea[2].IsInside(a.getEnd().X(), a.getEnd().Y());
		});

  Output2prongTreePtr->Fill();
}
///////////////////////////////
///////////////////////////////
void HIGS_trees_analysis::fillTrees3prong(Track3D *aTrack, eventraw::EventInfo *aEventInfo, bool & isFirst){

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
  event3prong_->runId=(aEventInfo ? aEventInfo->GetRunId() : -1);
  event3prong_->eventId=(aEventInfo ? aEventInfo->GetEventId() : -1);
  event3prong_->eventType = aEventInfo ? aEventInfo->GetEventType().to_ulong() : 0;
  event3prong_->unixTimeSec =
      (aEventInfo
           ? duration_cast<duration<long double>>(
                 tpcreco::eventAbsoluteTime(*aEventInfo).time_since_epoch())
                 .count()
           : -1); // absolute Unix time [s]
  // NOTE: for irregular runs with runId=[0, 999999] (6 figures) the parser returns absolute Unix time = 0
  static double last_timestamp = 0;
  if(isFirst) {
    last_timestamp=event3prong_->unixTimeSec;
    isFirst=false;
  }
  event1prong_->runTimeSec =
      (aEventInfo ? duration_cast<duration<long double>>(
                        tpcreco::eventRelativeTime(*aEventInfo))
                        .count()
                  : -1); // [s]
  event3prong_->deltaTimeSec=(aEventInfo ? event3prong_->unixTimeSec - last_timestamp : -1); // [s] time difference for rate measurements
  last_timestamp=event3prong_->unixTimeSec;

  event3prong_->vertexPos = list.front().getStart();
  auto track1=list.at(0);
  event3prong_->alpha1_endPos = track1.getEnd();
  event3prong_->alpha1_length = track1.getLength(); // longest alpha
  event3prong_->alpha1_phiDET = track1.getTangent().Phi();
  event3prong_->alpha1_cosThetaDET = cos(track1.getTangent().Theta());
  event3prong_->alpha1_phiBEAM = atan2(-track1.getTangent().Z(), track1.getTangent().Y()); // [rad], azimuthal angle from horizontal axis;
  event3prong_->alpha1_cosThetaBEAM = track1.getTangent()*gammaUnitVec; // cos polar angle wrt beam axis
  event3prong_->alpha1_chargeProfile = track1.getChargeProfile();

  auto track2=list.at(1);
  event3prong_->alpha2_endPos = track2.getEnd();
  event3prong_->alpha2_length = track2.getLength(); // middle alpha
  event3prong_->alpha2_phiDET = track2.getTangent().Phi();
  event3prong_->alpha2_cosThetaDET = cos(track2.getTangent().Theta());
  event3prong_->alpha2_phiBEAM = atan2(-track2.getTangent().Z(), track2.getTangent().Y()); // [rad], azimuthal angle from horizontal axis;
  event3prong_->alpha2_cosThetaBEAM = track2.getTangent()*gammaUnitVec; // cos polar angle wrt beam axis
  event3prong_->alpha2_chargeProfile = track2.getChargeProfile();

  auto track3=list.at(2);
  event3prong_->alpha3_endPos = track3.getEnd();
  event3prong_->alpha3_length = track3.getLength(); // shortest alpha
  event3prong_->alpha3_phiDET = track3.getTangent().Phi();
  event3prong_->alpha3_cosThetaDET = cos(track3.getTangent().Theta());
  event3prong_->alpha3_phiBEAM = atan2(-track3.getTangent().Z(), track3.getTangent().Y()); // [rad], azimuthal angle from horizontal axis;
  event3prong_->alpha3_cosThetaBEAM = track3.getTangent()*gammaUnitVec; // polar angle wrt beam axis
  event3prong_->alpha3_chargeProfile = track3.getChargeProfile();
  
  // check containment in Z_DET
  event3prong_->Zmargin2mm = true;
  event3prong_->Zmargin5mm = true;
  event3prong_->Zmargin10mm = true;
  std::for_each(list.begin(), list.end(),
		[&](const TrackSegment3D& a) mutable {
		  event3prong_->Zmargin2mm &= zRange[0].IsInside(a.getStart().Z(), a.getEnd().Z());
		  event3prong_->Zmargin5mm &= zRange[1].IsInside(a.getStart().Z(), a.getEnd().Z());
		  event3prong_->Zmargin10mm &= zRange[2].IsInside(a.getStart().Z(), a.getEnd().Z());
		});
  
  // check containment in XY_DET
  event3prong_->XYmargin2mm = true;
  event3prong_->XYmargin5mm = true;
  event3prong_->XYmargin10mm = true;
  std::for_each(list.begin(), list.end(),
		[&](const TrackSegment3D& a) mutable {
		  event3prong_->XYmargin2mm &=
		    xyArea[0].IsInside(a.getStart().X(), a.getStart().Y()) &
		    xyArea[0].IsInside(a.getEnd().X(), a.getEnd().Y());
		  event3prong_->XYmargin5mm &=
		    xyArea[1].IsInside(a.getStart().X(), a.getStart().Y()) &
		    xyArea[1].IsInside(a.getEnd().X(), a.getEnd().Y());
		  event3prong_->XYmargin10mm &=
		    xyArea[2].IsInside(a.getStart().X(), a.getStart().Y()) &
		    xyArea[2].IsInside(a.getEnd().X(), a.getEnd().Y());
		});

  Output3prongTreePtr->Fill();
}
///////////////////////////////
///////////////////////////////
