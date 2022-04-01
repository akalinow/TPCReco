#include <vector>
#include <iostream>

#include "TH1D.h"
#include "TH2D.h"
#include "TFile.h"
#include "TVector3.h"

#include "GeometryTPC.h"
#include "Track3D.h"
#include "EventInfo.h"

#include "Comp_analysis.h"

///////////////////////////////
///////////////////////////////
Comp_analysis::Comp_analysis(std::shared_ptr<GeometryTPC> aGeometryPtr){

  setGeometry(aGeometryPtr);
  bookHistos();
  
}
///////////////////////////////
///////////////////////////////
Comp_analysis::~Comp_analysis(){

  finalize();
  delete outputFile;
}
///////////////////////////////
///////////////////////////////
void Comp_analysis::bookHistos(){

  std::string outputFileName = "ComparisonHistos.root";
  outputFile = new TFile(outputFileName.c_str(),"RECREATE");

  // GLOBAL HISTOGRAMS
  //
  // NTRACKS : ALL event categories
  histos1D["nTracks_diff"] = new TH1F("nTracks_diff","Difference of number of tracks;Tracks per event;Event count", 11, -5.5, 5.5);

}
///////////////////////////////
///////////////////////////////
void Comp_analysis::fillHistos(Track3D *aRefTrack, eventraw::EventInfo *aRefEventInfo,
			       Track3D *aTestTrack, eventraw::EventInfo *aTestEventInfo){

  bool eventMissingInRef = false;
  bool eventMissingInTest = false;
  if(aRefEventInfo->GetRunId()==0){
    std::cout<<"Event: "<<aTestEventInfo->GetEventId()<<" missing in the REFERENCE file"<<std::endl;
    eventMissingInRef = true;
  }
  if(aTestEventInfo->GetRunId()==0){
    std::cout<<"Event: "<<aRefEventInfo->GetEventId()<<" missing in the TEST file"<<std::endl;
    eventMissingInTest = true;
  }
  
  if(eventMissingInRef || eventMissingInTest) return;

  int nRefTracks = aRefTrack->getSegments().size();
  int nTestTracks = aRefTrack->getSegments().size();

  histos1D["nTracks_diff"]->Fill(nRefTracks-nTestTracks);
}
///////////////////////////////
///////////////////////////////
void Comp_analysis::finalize(){

  outputFile->Write();
}
///////////////////////////////
///////////////////////////////
void Comp_analysis::setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr){
  
  myGeometryPtr = aGeometryPtr;
}
///////////////////////////////
///////////////////////////////
