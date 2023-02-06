#include <vector>
#include <iostream>

#include "TH1D.h"
#include "TH2D.h"
//// TEST
#include "TProfile.h"
#include "TProfile2D.h"
//// TEST
#include "TFile.h"
#include "TVector3.h"

#include "GeometryTPC.h"
#include "Track3D.h"
#include "EventInfo.h"

#include "Comp_analysis.h"
#define MISSING_PID_FROM_LENGTH true // TEST

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
  //  histos1D["nTracks_diff"] = new TH1F("nTracks_diff","Difference of number of tracks;Tracks per event;Event count", 11, -5.5, 5.5);
  //TEST
  const auto maxNtracks=3;
  const auto maxLen_alpha=110.0; // mm
  const auto maxLen_carbon=25.0; // mm
  const auto maxLen_diff=20.0; // mm
  const auto binLen=0.5; // mm
  const auto binCosTheta=2.0/50; // unitless
  const auto binPhi=TMath::TwoPi()/50; // rad
  float xmin, xmax, ymin, ymax, zmin, zmax; // [mm]
  std::tie(xmin, xmax, ymin, ymax) = myGeometryPtr->rangeXY();
  std::tie(zmin, zmax) = myGeometryPtr->rangeZ();
  
  // NTRACKS : ALL event categories
  // mis-identification probability
  histos1D["nTracks_diff"]=
    new TH1F("nTracks_diff","Difference TEST-REF of number of tracks;#Delta(Tracks per event);Event count",
	     maxNtracks*2+1, -maxNtracks-0.5, maxNtracks+0.5);
  auto h2=(histos2D["nTracksTEST_vs_nTracksREF"]=
	   new TH2F("nTracksTEST_vs_nTracksREF","Number of tracks TEST vs REF;TEST tracks per event;REF tracks per event;Event count",
		    maxNtracks+1, -0.5, maxNtracks+0.5, maxNtracks+1, -0.5, maxNtracks+0.5));
  h2->SetOption("BOX TEXT");

  // VERTEX : All event categories
  histos1D["vertexX_diff"]=
    new TH1F("vertexX_diff","Difference TEST-REF of vertex X_{DET};#DeltaX_{DET} of vertex [mm];Event count",
	     (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2);
  histos1D["vertexY_diff"]=
    new TH1F("vertexY_diff","Difference TEST-REF of vertex Y_{DET};#DeltaY_{DET} of vertex [mm];Event count",
	     (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2);
  histos1D["vertexZ_diff"]=
    new TH1F("vertexZ_diff","Difference TEST-REF of vertex Z_{DET};#DeltaZ_{DET} of vertex [mm];Event count",
	     (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2);
  h2=(histos2D["nTracks_diff_vs_vertexXREF"]=
      new TH2F("nTracks_diff_vs_vertexXREF","Difference TEST-REF of number of tracks vs REF vertex X_{DET};#Delta(Tracks per event);REF vertex X_{DET} [mm];Event count",
	       2*maxNtracks+1, -maxNtracks-0.5, maxNtracks+0.5,
	       ((xmax-xmin)+binLen)/binLen, xmin-binLen/2, xmax+binLen/2));
  h2->SetOption("BOX");
  h2=(histos2D["nTracks_diff_vs_vertexYREF"]=
      new TH2F("nTracks_diff_vs_vertexYREF","Difference TEST-REF of number of tracks vs REF vertex Y_{DET};#Delta(Tracks per event);REF vertex Y_{DET} [mm];Event count",
	       2*maxNtracks+1, -maxNtracks-0.5, maxNtracks+0.5,
	       ((ymax-ymin)+binLen)/binLen, ymin-binLen/2, ymax+binLen/2));
  h2->SetOption("BOX");
  h2=(histos2D["nTracks_diff_vs_vertexZREF"]=
      new TH2F("nTracks_diff_vs_vertexZREF","Difference TEST-REF of number of tracks vs REF vertex Z_{DET};#Delta(Tracks per event);REF vertex Z_{DET} [mm];Event count",
	       2*maxNtracks+1, -maxNtracks-0.5, maxNtracks+0.5,
	       ((zmax-zmin)+binLen)/binLen, zmin-binLen/2, zmax+binLen/2));
  h2->SetOption("BOX");
  
  // both classified as 1-prong
  histos1D["1prong_alphaLen_diff"]=
    new TH1F("1prong_alphaLen_diff","1-prong: Difference TEST-REF of #alpha length;#Delta(#alpha length) [mm];Event count",
	     (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2);
  histos1D["1prong_alphaAngle_diff"]=
    new TH1F("1prong_alphaAngle_diff","1-prong: Angular difference TEST-REF of #alpha track;Residual angle between #alpha tracks [rad];Event count",
	     (TMath::Pi()+binPhi)/binPhi, -binPhi/2, TMath::Pi()+binPhi/2);
  h2=(histos2D["1prong_alphaLen_diff_vs_alphaLenREF"]=
      new TH2F("1prong_alphaLen_diff_vs_alphaLenREF","1-prong: Difference TEST-REF of #alpha length vs REF #alpha length;#Delta(#alpha length) [mm];REF #alpha length [mm];Event count",
	       (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2,
	       (maxLen_alpha+binLen)/binLen, -binLen/2, maxLen_alpha+binLen/2));
  h2->SetOption("BOX");
  histos1D["1prong_carbonLen_diff"]=
    new TH1F("1prong_carbonLen_diff","1-prong: Difference TEST-REF of ^{12}C length;#Delta(^{12}C length) [mm];Event count",
	     (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2);
  histos1D["1prong_carbonAngle_diff"]=
    new TH1F("1prong_carbonAngle_diff","1-prong: Angular difference TEST-REF of ^{12}C track;Residual angle between ^{12}C tracks [rad];Event count",
	     (TMath::Pi()+binPhi)/binPhi, -binPhi/2, TMath::Pi()+binPhi/2);
  h2=(histos2D["1prong_carbonLen_diff_vs_carbonLenREF"]=
      new TH2F("1prong_carbonLen_diff_vs_carbonLenREF","1-prong: Difference TEST-REF of ^{12}C length vs REF ^{12}C length;#Delta(^{12}C length) [mm];REF ^{12}C length [mm];Event count",
	       (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2,
	       (maxLen_carbon+binLen)/binLen, -binLen/2, maxLen_carbon+binLen/2));
  h2->SetOption("BOX");
  h2=(histos2D["1prong_alphaLenTEST_vs_alphaLenREF"]=
      new TH2F("1prong_alphaLenTEST_vs_alphaLenREF","1-prong: TEST vs REF of #alpha length;TEST #alpha length [mm];REF #alpha length [mm];Event count",
	       (2*maxLen_alpha+binLen)/binLen, -maxLen_alpha-binLen/2, maxLen_alpha+binLen/2,
	       (2*maxLen_alpha+binLen)/binLen, -maxLen_alpha-binLen/2, maxLen_alpha+binLen/2));
  h2->SetOption("BOX");
  h2=(histos2D["1prong_carbonLenTEST_vs_carbonLenREF"]=
      new TH2F("1prong_carbonLenTEST_vs_carbonLenREF","1-prong: TEST vs REF of ^{12}C length;TEST ^{12}C length [mm];REF ^{12}C length [mm];Event count",
	       (maxLen_carbon+binLen)/binLen, -binLen/2, maxLen_carbon+binLen/2,
	       (maxLen_carbon+binLen)/binLen, -binLen/2, maxLen_carbon+binLen/2));
  h2->SetOption("BOX");
  profiles1D["1prong_alphaLenTEST_vs_alphaLenREF_prof"]=
    new TProfile("1prong_alphaLenTEST_vs_alphaLenREF_prof","1-prong: TEST vs REF of #alpha length;TEST #alpha length [mm];REF #alpha length [mm];Event count",
		 (2*maxLen_alpha+binLen)/binLen, -maxLen_alpha-binLen/2, maxLen_alpha+binLen/2,
		 -maxLen_alpha-binLen/2, maxLen_alpha+binLen/2);
  profiles1D["1prong_carbonLenTEST_vs_carbonLenREF_prof"]=
    new TProfile("1prong_carbonLenTEST_vs_carbonLenREF_prof","1-prong: TEST vs REF of ^{12}C length;TEST ^{12}C length [mm];REF ^{12}C length [mm];Event count",
		 (maxLen_carbon+binLen)/binLen, -binLen/2, maxLen_carbon+binLen/2,
		 -binLen/2, maxLen_carbon+binLen/2);
  auto p2=(profiles2D["1prong_alphaPhiDetREF_vs_alphaCosThetaDetREF_vs_chi2TEST_prof"]=
	   new TProfile2D("1prong_alphaPhiDetREF_vs_alphaCosThetaDetREF_vs_chi2TEST_prof","1-prong: #chi^{2} of TEST fit vs REF #phi_{DET} x cos(#theta_{DET};#phi_{DET} of REF #alpha track [rad];cos(#theta_{DET}) of REF #alpha track;#chi^{2} of TEST fit",
			  TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi(),
			  (2.0+binCosTheta)/binCosTheta, -1.0-binCosTheta/2, 1.0+binCosTheta/2,
			  0.0, 100.0));
  p2->SetOption("COLZ");
  p2=(profiles2D["1prong_carbonPhiDetREF_vs_carbonCosThetaDetREF_vs_chi2TEST_prof"]=
      new TProfile2D("1prong_carbonPhiDetREF_vs_carbonCosThetaDetREF_vs_chi2TEST_prof","1-prong: #chi^{2} of TEST fit vs REF #phi_{DET} x cos#theta_{DET};#phi_{DET} of REF ^{12}C track [rad];cos(#theta_{DET}) of REF ^{12}C track;#chi^{2} of TEST fit",
		     TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi(),
		     (2.0+binCosTheta)/binCosTheta, -1.0-binCosTheta/2, 1.0+binCosTheta/2,
		     0.0, 100.0));
  p2->SetOption("COLZ");
  
  // both classified as 2-prong
  histos1D["2prong_alphaLen_diff"]=
    new TH1F("2prong_alphaLen_diff","2-prong: Difference TEST-REF of alpha length;#Delta(#alpha length) [mm];Event count",
	     (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2);
  histos1D["2prong_alphaAngle_diff"]=
    new TH1F("2prong_alphaAngle_diff","2-prong: Angular difference TEST-REF of #alpha track;Residual angle between #alpha tracks [rad];Event count",
	     (TMath::Pi()+binPhi)/binPhi, -binPhi/2, TMath::Pi()+binPhi/2);
  h2=(histos2D["2prong_alphaLen_diff_vs_alphaLenREF"]=
      new TH2F("2prong_alphaLen_diff_vs_alphaLenREF","2-prong: Difference TEST-REF of alpha length vs REF #alpha length;#Delta(#alpha length) [mm];REF #alpha length [mm];Event count",
	       (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2,
	       (maxLen_alpha+binLen)/binLen, -binLen/2, maxLen_alpha+binLen/2));
  h2->SetOption("BOX");
  histos1D["2prong_carbonLen_diff"]=
    new TH1F("2prong_carbonLen_diff","2-prong: Difference TEST-REF of ^{12}C length;#Delta(^{12}C length) [mm];Event count",
	     (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2);
  histos1D["2prong_carbonAngle_diff"]=
    new TH1F("2prong_carbonAngle_diff","2-prong: Angular difference TEST-REF of ^{12}C track;Residual angle between ^{12}C tracks [rad];Event count",
	     (TMath::Pi()+binPhi)/binPhi, -binPhi/2, TMath::Pi()+binPhi/2);
  h2=(histos2D["2prong_carbonLen_diff_vs_carbonLenREF"]=
      new TH2F("2prong_carbonLen_diff_vs_carbonLenREF","2-prong: Difference TEST-REF of ^{12}C length vs REF ^{12}C length;#Delta(^{12}C length) [mm];REF ^{12}C length [mm];Event count",
	       (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2,
	       (maxLen_carbon+binLen)/binLen, -binLen/2, maxLen_carbon+binLen/2));
  h2->SetOption("BOX");
  h2=(histos2D["2prong_alphaLenTEST_vs_alphaLenREF"]=
      new TH2F("2prong_alphaLenTEST_vs_alphaLenREF","2-prong: TEST vs REF of #alpha length;TEST #alpha length [mm];REF #alpha length [mm];Event count",
	       (2*maxLen_alpha+binLen)/binLen, -maxLen_alpha-binLen/2, maxLen_alpha+binLen/2,
	       (2*maxLen_alpha+binLen)/binLen, -maxLen_alpha-binLen/2, maxLen_alpha+binLen/2));
  h2->SetOption("BOX");
  h2=(histos2D["2prong_carbonLenTEST_vs_carbonLenREF"]=
      new TH2F("2prong_carbonLenTEST_vs_carbonLenREF","2-prong: TEST vs REF of ^{12}C length;TEST ^{12}C length [mm];REF ^{12}C length [mm];Event count",
	       (maxLen_carbon+binLen)/binLen, -binLen/2, maxLen_carbon+binLen/2,
	       (maxLen_carbon+binLen)/binLen, -binLen/2, maxLen_carbon+binLen/2));
  h2->SetOption("BOX");
  profiles1D["2prong_alphaLenTEST_vs_alphaLenREF_prof"]=
    new TProfile("2prong_alphaLenTEST_vs_alphaLenREF_prof","2-prong: TEST vs REF of #alpha length;TEST #alpha length [mm];REF #alpha length [mm];Event count",
		 (maxLen_alpha+binLen)/binLen, -binLen/2, maxLen_alpha+binLen/2,
		 -binLen/2, maxLen_alpha+binLen/2);
  profiles1D["2prong_carbonLenTEST_vs_carbonLenREF_prof"]=
    new TProfile("2prong_carbonLenTEST_vs_carbonLenREF_prof","2-prong: TEST vs REF of ^{12}C length;TEST ^{12}C length [mm];REF ^{12}C length [mm];Event count",
		 (maxLen_carbon+binLen)/binLen, -binLen/2, maxLen_carbon+binLen/2,
		 -binLen/2, maxLen_carbon+binLen/2);
  p2=(profiles2D["2prong_alphaPhiDetREF_vs_alphaCosThetaDetREF_vs_chi2TEST_prof"]=
      new TProfile2D("2prong_alphaPhiDetREF_vs_alphaCosThetaDetREF_vs_chi2TEST_prof","2-prong: #chi^{2} of TEST fit vs REF #phi_{DET} x cos#theta_{DET};#phi_{DET} of REF #alpha track [rad];cos(#theta_{DET}) of REF #alpha track;#chi^{2} of TEST fit",
		     TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi(),
		     (2.0+binCosTheta)/binCosTheta, -1.0-binCosTheta/2, 1.0+binCosTheta/2,
		     0.0, 100.0));
  p2->SetOption("COLZ");
  p2=(profiles2D["2prong_carbonPhiDetREF_vs_carbonCosThetaDetREF_vs_chi2TEST_prof"]=
      new TProfile2D("2prong_carbonPhiDetREF_vs_carbonCosThetaDetREF_vs_chi2TEST_prof","2-prong: #chi^{2} of TEST fit vs REF #phi_{DET} x cos#theta_{DET};#phi_{DET} of REF ^{12}C track [rad];cos(#theta_{DET}) of REF ^{12}C track;#chi^{2} of TEST fit",
		     TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi(),
		     (2.0+binCosTheta)/binCosTheta, -1.0-binCosTheta/2, 1.0+binCosTheta/2,
		     0.0, 100.0));
  p2->SetOption("COLZ");
  // TEST

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
  int nTestTracks = aTestTrack->getSegments().size();//aRefTrack->getSegments().size();

  //histos1D["nTracks_diff"]->Fill(nRefTracks-nTestTracks);
  //TEST
  histos1D["nTracks_diff"]->Fill(nTestTracks-nRefTracks);
  histos2D["nTracksTEST_vs_nTracksREF"]->Fill(nTestTracks, nRefTracks); 
  auto collRef=aRefTrack->getSegments();
  auto collTest=aTestTrack->getSegments();

#if(MISSING_PID_FROM_LENGTH)
  // get sorted list of tracks (descending order by track length)
    std::sort(collRef.begin(), collRef.end(),
    	      [](const TrackSegment3D& a, const TrackSegment3D& b) {
    		return a.getLength() > b.getLength();
    	      });
  std::sort(collTest.begin(), collTest.end(),
	    [](const TrackSegment3D& a, const TrackSegment3D& b) {
	      return a.getLength() > b.getLength();
	    });
  // HACK: assign missing PID to REF data file by track length
  if(collRef.size()==2 && (collRef.front().getPID()==UNKNOWN || collRef.back().getPID()==UNKNOWN)) {
    collRef.front().setPID(ALPHA);
    collRef.back().setPID(CARBON_12);
  }
  if(collRef.size()==1 && collRef.front().getPID()==UNKNOWN) {
    collRef.front().setPID(ALPHA);
  }
  // HACK: assign missing PID to TEST data file by track length
  if(collTest.size()==2 && (collTest.front().getPID()==UNKNOWN || collTest.back().getPID()==UNKNOWN)) {
    collTest.front().setPID(ALPHA);
    collTest.back().setPID(CARBON_12);
  }
  if(collTest.size()==1 && collTest.front().getPID()==UNKNOWN) {
    collTest.front().setPID(ALPHA);
  }
#endif

  // VERTEX : All event categories
  if(collRef.size() && collTest.size()) {
    histos1D["vertexX_diff"]->Fill(collTest.front().getStart().X()-collRef.front().getStart().X());
    histos1D["vertexY_diff"]->Fill(collTest.front().getStart().Y()-collRef.front().getStart().Y());
    histos1D["vertexZ_diff"]->Fill(collTest.front().getStart().Z()-collRef.front().getStart().Z());
    histos2D["nTracks_diff_vs_vertexXREF"]->Fill(nTestTracks-nRefTracks, collRef.front().getStart().X());
    histos2D["nTracks_diff_vs_vertexYREF"]->Fill(nTestTracks-nRefTracks, collRef.front().getStart().Y());
    histos2D["nTracks_diff_vs_vertexZREF"]->Fill(nTestTracks-nRefTracks, collRef.front().getStart().Z());
  }
  
  // both classified as 1-prong
  if(collRef.size()==1 && collTest.size()==1) {

    for(auto &aRefSeg: collRef) if(aRefSeg.getPID()==ALPHA) {
	auto found=false;
	for(auto &aTestSeg: collTest) if(aTestSeg.getPID()==aRefSeg.getPID()) {
	    histos1D["1prong_alphaLen_diff"]->Fill(aTestSeg.getLength()-aRefSeg.getLength());
	    histos1D["1prong_alphaAngle_diff"]->Fill(aTestSeg.getTangent().Angle(aRefSeg.getTangent()));
	    histos2D["1prong_alphaLen_diff_vs_alphaLenREF"]->Fill(aTestSeg.getLength()-aRefSeg.getLength(), aRefSeg.getLength());
	    histos2D["1prong_alphaLenTEST_vs_alphaLenREF"]->Fill(aTestSeg.getLength(), aRefSeg.getLength());
	    profiles1D["1prong_alphaLenTEST_vs_alphaLenREF_prof"]->Fill(aTestSeg.getLength(), aRefSeg.getLength());
	    profiles2D["1prong_alphaPhiDetREF_vs_alphaCosThetaDetREF_vs_chi2TEST_prof"]->Fill(aRefSeg.getTangent().Phi(), aRefSeg.getTangent().CosTheta(), aTestTrack->getChi2()); 
	    found=true;
	    break;
	  }
	if(found) break;
      }
    for(auto &aRefSeg: collRef) if(aRefSeg.getPID()==CARBON_12) {
	auto found=false;
	for(auto &aTestSeg: collTest) if(aTestSeg.getPID()==aRefSeg.getPID()) {
	    histos1D["1prong_carbonLen_diff"]->Fill(aTestSeg.getLength()-aRefSeg.getLength());
	    histos1D["1prong_carbonAngle_diff"]->Fill(aTestSeg.getTangent().Angle(aRefSeg.getTangent()));
	    histos2D["1prong_carbonLen_diff_vs_carbonLenREF"]->Fill(aTestSeg.getLength()-aRefSeg.getLength(), aRefSeg.getLength());
	    histos2D["1prong_carbonLenTEST_vs_carbonLenREF"]->Fill(aTestSeg.getLength(), aRefSeg.getLength());
	    profiles1D["1prong_carbonLenTEST_vs_carbonLenREF_prof"]->Fill(aTestSeg.getLength(), aRefSeg.getLength());
	    profiles2D["1prong_carbonPhiDetREF_vs_carbonCosThetaDetREF_vs_chi2TEST_prof"]->Fill(aRefSeg.getTangent().Phi(), aRefSeg.getTangent().CosTheta(), aTestTrack->getChi2()); 
	    found=true;
	    break;
	  }
	if(found) break;
      }
  }
  // both classified as 2-prong
  if(collRef.size()==2 && collTest.size()==2) {

    for(auto &aRefSeg: collRef) if(aRefSeg.getPID()==ALPHA) {
	auto found=false;
	for(auto &aTestSeg: collTest) if(aTestSeg.getPID()==aRefSeg.getPID()) {
	    histos1D["2prong_alphaLen_diff"]->Fill(aTestSeg.getLength()-aRefSeg.getLength());
	    histos1D["2prong_alphaAngle_diff"]->Fill(aTestSeg.getTangent().Angle(aRefSeg.getTangent()));
	    histos2D["2prong_alphaLen_diff_vs_alphaLenREF"]->Fill(aTestSeg.getLength()-aRefSeg.getLength(), aRefSeg.getLength());
	    histos2D["2prong_alphaLenTEST_vs_alphaLenREF"]->Fill(aTestSeg.getLength(), aRefSeg.getLength());
	    profiles1D["2prong_alphaLenTEST_vs_alphaLenREF_prof"]->Fill(aTestSeg.getLength(), aRefSeg.getLength());
	    profiles2D["2prong_alphaPhiDetREF_vs_alphaCosThetaDetREF_vs_chi2TEST_prof"]->Fill(aRefSeg.getTangent().Phi(), aRefSeg.getTangent().CosTheta(), aTestTrack->getChi2()); 
	    found=true;
	    break;
	  }
	if(found) break;
      }
    for(auto &aRefSeg: collRef) if(aRefSeg.getPID()==CARBON_12) {
	auto found=false;
	for(auto &aTestSeg: collTest) if(aTestSeg.getPID()==aRefSeg.getPID()) {
	    histos1D["2prong_carbonLen_diff"]->Fill(aTestSeg.getLength()-aRefSeg.getLength());
	    histos1D["2prong_carbonAngle_diff"]->Fill(aTestSeg.getTangent().Angle(aRefSeg.getTangent()));
	    histos2D["2prong_carbonLen_diff_vs_carbonLenREF"]->Fill(aTestSeg.getLength()-aRefSeg.getLength(), aRefSeg.getLength());
	    histos2D["2prong_carbonLenTEST_vs_carbonLenREF"]->Fill(aTestSeg.getLength(), aRefSeg.getLength());
	    profiles1D["2prong_carbonLenTEST_vs_carbonLenREF_prof"]->Fill(aTestSeg.getLength(), aRefSeg.getLength());
	    profiles2D["2prong_carbonPhiDetREF_vs_carbonCosThetaDetREF_vs_chi2TEST_prof"]->Fill(aRefSeg.getTangent().Phi(), aRefSeg.getTangent().CosTheta(), aTestTrack->getChi2()); 
	    found=true;
	    break;
	  }
	if(found) break;
      }
  }
  //TEST
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
