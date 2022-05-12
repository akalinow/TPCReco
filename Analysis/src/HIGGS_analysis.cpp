#include <vector>
#include <iostream>

#include "TH1F.h"
#include "TH2F.h"
#include "TProfile.h"
#include "TFile.h"
#include "TVector3.h"
#include "TLorentzVector.h"
//////// DEBUG
//#include "TCanvas.h"
//#include "TView.h"
//#include "TPolyLine3D.h"
//////// DEBUG

#include "GeometryTPC.h"
#include "Track3D.h"
#include "HIGGS_analysis.h"

///////////////////////////////
///////////////////////////////
HIGGS_analysis::HIGGS_analysis(std::shared_ptr<GeometryTPC> aGeometryPtr, // definition of LAB detector coordinates
			       float beamEnergy, // nominal gamma beam energy [MeV] in detector LAB frame
			       TVector3 beamDir, // nominal gamma beam direction in detector LAB frame
			       double pressure){ // CO2 pressure [mbar]
  setGeometry(aGeometryPtr);
  setBeamProperties(beamEnergy, beamDir);
  setIonRangeCalculator(pressure);  
  bookHistos();
}
///////////////////////////////
///////////////////////////////
HIGGS_analysis::~HIGGS_analysis(){

  finalize();
  delete outputFile;
}
//////////////////////////
//////////////////////////
void HIGGS_analysis::setIonRangeCalculator(double pressure){ // CO2 pressure [mbar]

  // set current conditions: gas=CO2, pressure=190 mbar, temperature=20C
  myRangeCalculator.setGasConditions(IonRangeCalculator::CO2, fabs(pressure), 273.15+20);
}
///////////////////////////////
///////////////////////////////
void HIGGS_analysis::bookHistos(){

  const std::string categoryPrefix[]={ "h_all", "h_1prong", "h_2prong", "h_3prong" };
  const std::string categoryInfo[]={ "ALL", "1-prong (#alpha)", "2-prong (#alpha+C)", "3-prong (#alpha_{1}+#alpha_{2}+#alpha_{3})" };

  // PID for calculating the range, etc
  // NOTE: index=0 is deliberately empty
  std::vector<std::string> categoryPID[4]; 
  categoryPID[1].push_back("alpha"); // 1-prong alpha
  categoryPID[2].push_back("alpha"); // 2-prong alpha
  categoryPID[2].push_back("carbon");// 2-prong carbon
  categoryPID[3].push_back("alpha1"); // 3-prong 1st alpha
  categoryPID[3].push_back("alpha2"); // 3-prong 2nd alpha
  categoryPID[3].push_back("alpha3"); // 3-prong 3rd alpha

  // PID for creating unique histogram names
  std::vector<std::string> categoryPIDhname[4];; 
  categoryPIDhname[0].push_back(""); // any track
  categoryPIDhname[1].push_back("_alpha"); // 1-prong alpha
  categoryPIDhname[2].push_back("_alpha"); // 2-prong alpha
  categoryPIDhname[2].push_back("_carbon");// 2-prong carbon
  categoryPIDhname[3].push_back("_alpha1"); // 3-prong 1st alpha
  categoryPIDhname[3].push_back("_alpha2"); // 3-prong 2nd alpha
  categoryPIDhname[3].push_back("_alpha3"); // 3-prong 3rd alpha

  // PID for creating nice looking titles and axis labels
  std::vector<std::string> categoryPIDlatex[4]; 
  categoryPIDlatex[0].push_back("Any"); // any track
  categoryPIDlatex[1].push_back("#alpha"); // 1-prong alpha
  categoryPIDlatex[2].push_back("#alpha"); // 2-prong alpha
  categoryPIDlatex[2].push_back("C");// 2-prong carbon
  categoryPIDlatex[3].push_back("#alpha_{1}"); // 3-prong 1st alpha
  categoryPIDlatex[3].push_back("#alpha_{2}"); // 3-prong 2nd alpha
  categoryPIDlatex[3].push_back("#alpha_{3}"); // 3-prong 3rd alpha
  
  std::string outputFileName = "Histos.root";
  outputFile = new TFile(outputFileName.c_str(),"RECREATE");

  const float binSizeMM = 0.5; // [mm]
  const float binSizeMM_2d = 3.0; // [mm]
  const float binSizeMM_prof = 3.0; // [mm]
  const float maxLengthMM = 200.0; // [mm]
  const float minTotalEnergyMeV = 5e3; // [MeV]
  const float maxTotalEnergyMeV = 10e3; // [MeV]
  const float maxKineticEnergyMeV = 6.0; // [MeV]
  const float maxDeltaMomentumMeV = 20.0; // [MeV]
  float xmin, xmax, ymin, ymax, zmin, zmax; // [mm]
  std::tie(xmin, xmax, ymin, ymax) = myGeometryPtr->rangeXY();
  std::tie(zmin, zmax) = myGeometryPtr->rangeZ();
  //  zmin = myGeometryPtr->GetDriftCageZmin();
  //  zmax = myGeometryPtr->GetDriftCageZmax();

  ///////// DEBUG
  //  outputCanvas = new TCanvas("c", "all events", 500, 500);
  //  TView *view = TView::CreateView(1);
  //  view->SetRange(xmin-0.5*(xmax-xmin), ymin-0.5*(ymax-ymin), zmin-0.5*(zmax-zmin),
  //		 xmax+0.5*(xmax-xmin), ymax+0.5*(ymax-ymin), zmax+0.5*(zmax-zmin));
  //
  //  // plot active volume's faces
  //  TGraph gr=myGeometryPtr->GetActiveAreaConvexHull();
  //  auto l=new TPolyLine3D(5*(gr.GetN()-1));
  //  for(auto iedge=0; iedge<gr.GetN()-1; iedge++) {
  //    l->SetPoint(iedge*5+0,
  //		gr.GetX()[iedge],
  //		gr.GetY()[iedge],
  //		zmin);
  //    l->SetPoint(iedge*5+1,
  //		gr.GetX()[iedge+1],
  //		gr.GetY()[iedge+1],
  //		zmin);
  //    l->SetPoint(iedge*5+2,
  //		gr.GetX()[iedge+1],
  //		gr.GetY()[iedge+1],
  //		zmax);
  //   l->SetPoint(iedge*5+3,
  //		gr.GetX()[iedge],
  //		gr.GetY()[iedge],
  //		zmax);
  //    l->SetPoint(iedge*5+4,
  //		gr.GetX()[iedge],
  //		gr.GetY()[iedge],
  //		zmin);
  //  }
  //  l->SetLineColor(kBlue);
  //  l->Draw();
  //  outputCanvas->Update();
  //  outputCanvas->Modified();
  //  outputCanvas->Print("aaa.root");
  ///////// DEBUG

  // GLOBAL HISTOGRAMS
  //
  // NTRACKS : ALL event categories
  histos1D["ntracks"]=
    new TH1F("ntracks","Number of tracks;Tracks per event;Event count", 5, 0, 5);

  // HISTOGRAMS PER CATEGORY
  //
  for(auto c=0U; c<4; ++c) {
    auto prefix=categoryPrefix[c];
    auto info=categoryInfo[c].c_str();
    auto perEventTitle="Event count / bin";
    auto perTrackTitle=(c==0 ? "Track count / bin" : perEventTitle );

    // VERTEX : per category
    histos1D[(prefix+"_vertexX").c_str()]=
      new TH1F((prefix+"_vertexX").c_str(),
	       Form("%s;Vertex position X_{DET} [mm];%s", info, perEventTitle),
	       (xmax-xmin)/binSizeMM, xmin, xmax);
    histos1D[(prefix+"_vertexY").c_str()]=
      new TH1F((prefix+"_vertexY").c_str(),
	       Form("%s;Vertex position Y_{DET} [mm];%s", info, perEventTitle),
	       (ymax-ymin)/binSizeMM, ymin, ymax);
    histos1D[(prefix+"_vertexZ").c_str()]=
      new TH1F((prefix+"_vertexZ").c_str(),
	       Form("%s;Vertex position Z_{DET} [mm];%s", info, perEventTitle),
	       (zmax-zmin)/binSizeMM, zmin, zmax);
    histos2D[(prefix+"_vertexXY").c_str()]=
      new TH2F((prefix+"_vertexXY").c_str(),
	       Form("%s;Vertex position X_{DET} [mm];Vertex position Y_{DET} [mm];%s", info, perEventTitle),
	       (xmax-xmin)/binSizeMM_2d, xmin, xmax, (ymax-ymin)/binSizeMM_2d, ymin, ymax);
    histos2D[(prefix+"_vertexYZ").c_str()]=
      new TH2F((prefix+"_vertexYZ").c_str(),
	       Form("%s;Vertex position Y_{DET} [mm];Vertex position Z_{DET} [mm];%s", info, perEventTitle),
	       (ymax-ymin)/binSizeMM_2d, ymin, ymax, (zmax-zmin)/binSizeMM_2d, zmin, zmax);
    profiles1D[(prefix+"_vertexXY_prof").c_str()]=
      new TProfile((prefix+"_vertexXZ_prof").c_str(),
		   Form("%s;Vertex position X_{DET} [mm];Average vertex position Y_{DET} [mm];%s", info, perEventTitle),
		   (xmax-xmin)/binSizeMM_prof, xmin, xmax, ymin, ymax);
    
    // TOTAL OBSERVABLE : per category
    switch(categoryPID[c].size()) {
    case 2: // 2-prong
    case 3: // 3-prong
      histos1D[(prefix+"_lenSUM").c_str()]=
	new TH1F((prefix+"_lenSUM").c_str(),
		 Form("%s;Sum of track lengths [mm];%s", info, perTrackTitle),
		 maxLengthMM/binSizeMM, 0, maxLengthMM);
      histos1D[(prefix+"_total_PxBEAM_CMS").c_str()]=
	new TH1F((prefix+"_total_PxBEAM_CMS").c_str(),
		 Form("%s;Total momentum X_{BEAM} in CMS [MeV/c];%s", info, perEventTitle),
		 100, -maxDeltaMomentumMeV, maxDeltaMomentumMeV);
      histos1D[(prefix+"_total_PyBEAM_CMS").c_str()]=
	new TH1F((prefix+"_total_PyBEAM_CMS").c_str(),
		 Form("%s;Total momentum Y_{BEAM} in CMS [MeV/c];%s", info, perEventTitle),
		 100, -maxDeltaMomentumMeV, maxDeltaMomentumMeV);
      histos1D[(prefix+"_total_PzBEAM_CMS").c_str()]=
	new TH1F((prefix+"_total_PzBEAM_CMS").c_str(),
		 Form("%s;Total momentum Z_{BEAM} in CMS [MeV/c];%s", info, perEventTitle),
		 100, -maxDeltaMomentumMeV, maxDeltaMomentumMeV);
      histos1D[(prefix+"_total_E_CMS").c_str()]=
	new TH1F((prefix+"_total_E_CMS").c_str(),
		 Form("%s;Total energy in CMS [MeV];%s", info, perEventTitle),
		 1000, minTotalEnergyMeV, maxTotalEnergyMeV);
      histos1D[(prefix+"_invMass").c_str()]=
	new TH1F((prefix+"_invMass").c_str(),
		 Form("%s;Invariant mass [MeV];%s", info, perEventTitle),
		 1000, minTotalEnergyMeV, maxTotalEnergyMeV);
      histos1D[(prefix+"_Qvalue_CMS").c_str()]=
	new TH1F((prefix+"_Qvalue_CMS").c_str(),
		 Form("%s;Q value in CMS [MeV];%s", info, perEventTitle),
		 100, 0, maxKineticEnergyMeV);
      break;
    default: break;
    };

    // HISTOGRAMS PER TRACK
    for(auto t=0U; t<categoryPIDhname[c].size(); ++t) {
      auto pid=categoryPIDhname[c].at(t);
      auto pidLatex=categoryPIDlatex[c].at(t).c_str();

      // TRACK LENGTH : per category / per track
      histos1D[(prefix+pid+"_len").c_str()]=
	new TH1F((prefix+pid+"_len").c_str(),
		 Form("%s;%s track length [mm];%s", info, pidLatex, perTrackTitle),
		 maxLengthMM/binSizeMM, 0, maxLengthMM);
      // TRACK DELTA_X/Y/Z : per category / per track
      histos1D[(prefix+pid+"_deltaX").c_str()]=
	new TH1F((prefix+pid+"_deltaX").c_str(),
		 Form("%s;%s track #DeltaX_{DET} [mm];%s", info, pidLatex, perTrackTitle),
		 maxLengthMM/binSizeMM, -0.5*maxLengthMM, 0.5*maxLengthMM);
      histos1D[(prefix+pid+"_deltaY").c_str()]=
	new TH1F((prefix+pid+"_deltaY").c_str(),
		 Form("%s;%s track #DeltaY_{DET} [mm];%s", info, pidLatex, perTrackTitle),
		 maxLengthMM/binSizeMM, -0.5*maxLengthMM, 0.5*maxLengthMM);
      histos1D[(prefix+pid+"_deltaZ").c_str()]=
	new TH1F((prefix+pid+"_deltaZ").c_str(),
		 Form("%s;%s track #DeltaZ_{DET} [mm];%s", info, pidLatex, perTrackTitle),
		 maxLengthMM/binSizeMM, -0.5*maxLengthMM, 0.5*maxLengthMM);
      histos2D[(prefix+pid+"_deltaXY").c_str()]=
	new TH2F((prefix+pid+"_deltaXY").c_str(),
		 Form("%s;%s track #DeltaX_{DET} [mm];%s track #DeltaY_{DET} [mm];%s", info, pidLatex, pidLatex, perTrackTitle),
		 maxLengthMM/binSizeMM_2d, -0.5*maxLengthMM, 0.5*maxLengthMM,
		 maxLengthMM/binSizeMM_2d, -0.5*maxLengthMM, 0.5*maxLengthMM);
      histos2D[(prefix+pid+"_deltaXZ").c_str()]=
	new TH2F((prefix+pid+"_deltaXZ").c_str(),
		 Form("%s;%s track #DeltaX_{DET} [mm];%s track #DeltaZ_{DET} [mm];%s", info, pidLatex, pidLatex, perTrackTitle),
		 maxLengthMM/binSizeMM_2d, -0.5*maxLengthMM, 0.5*maxLengthMM,
		 maxLengthMM/binSizeMM_2d, -0.5*maxLengthMM, 0.5*maxLengthMM);
      histos2D[(prefix+pid+"_deltaYZ").c_str()]=
	new TH2F((prefix+pid+"_deltaYZ").c_str(),
		 Form("%s;%s track #DeltaY_{DET} [mm];%s track #DeltaZ_{DET} [mm];%s", info, pidLatex, pidLatex, perTrackTitle),
		 maxLengthMM/binSizeMM_2d, -0.5*maxLengthMM, 0.5*maxLengthMM,
		 maxLengthMM/binSizeMM_2d, -0.5*maxLengthMM, 0.5*maxLengthMM);
      
      // TRACK END_X/Y/Z : per category / per track
      histos1D[(prefix+pid+"_endX").c_str()]=
	new TH1F((prefix+pid+"_endX").c_str(),
		 Form("%s;%s track endpoint X_{DET} [mm];%s", info, pidLatex, perTrackTitle),
		 (xmax-xmin)/binSizeMM, xmin, xmax);
      histos1D[(prefix+pid+"_endY").c_str()]=
	new TH1F((prefix+pid+"_endY").c_str(),
		 Form("%s;%s track endpoint Y_{DET} [mm];%s", info, pidLatex, perTrackTitle),
		 (ymax-ymin)/binSizeMM, ymin, ymax);
      histos1D[(prefix+pid+"_endZ").c_str()]=
	new TH1F((prefix+pid+"_endZ").c_str(),
		 Form("%s;%s track endpoint Z_{DET} [mm];%s", info, pidLatex, perTrackTitle),
		 (zmax-zmin)/binSizeMM, zmin, zmax);
      histos2D[(prefix+pid+"_endXY").c_str()]=
	new TH2F((prefix+pid+"_endXY").c_str(),
		 Form("%s;%s track endpoint X_{DET} [mm];%s track endpoint Y_{DET} [mm];%s", info, pidLatex, pidLatex, perTrackTitle),
		 (xmax-xmin)/binSizeMM_2d, xmin, xmax, (ymax-ymin)/binSizeMM_2d, ymin, ymax);
      // TRACK PHI_DET/THETA_DET/cos(THETA_DET) : per category / per track
      histos1D[(prefix+pid+"_phiDET").c_str()]=
	new TH1F((prefix+pid+"_phiDET").c_str(),
		 Form("%s;%s track #phi_{DET} [rad];%s", info, pidLatex, perTrackTitle),
		 100, -TMath::Pi(), TMath::Pi());
      histos1D[(prefix+pid+"_thetaDET").c_str()]=
	new TH1F((prefix+pid+"_thetaDET").c_str(),
		 Form("%s;%s track #theta_{DET} [rad];%s", info, pidLatex, perTrackTitle),
		 100, 0, TMath::Pi());
      histos1D[(prefix+pid+"_cosThetaDET").c_str()]=
	new TH1F((prefix+pid+"_cosThetaDET").c_str(),
		 Form("%s;%s track cos(#theta_{DET}) [rad];%s", info, pidLatex, perTrackTitle),
		 100, -1, 1);

      // TRACK PHI_BEAM/THETA_BEAM/cos(THETA_BEAM) : per category / per track
      histos1D[(prefix+pid+"_phiBEAM_LAB").c_str()]=
	new TH1F((prefix+pid+"_phiBEAM_LAB").c_str(),
		 Form("%s;%s track #phi_{BEAM} in LAB [rad];%s", info, pidLatex, perTrackTitle),
		 100, -TMath::Pi(), TMath::Pi());
      histos1D[(prefix+pid+"_thetaBEAM_LAB").c_str()]=
	new TH1F((prefix+pid+"_thetaBEAM_LAB").c_str(),
		 Form("%s;%s track #theta_{BEAM} in LAB [rad];%s", info, pidLatex, perTrackTitle),
		 100, 0, TMath::Pi());
      histos1D[(prefix+pid+"_cosThetaBEAM_LAB").c_str()]=
	new TH1F((prefix+pid+"_cosThetaBEAM_LAB").c_str(),
		 Form("%s;%s track cos(#theta_{BEAM}) in LAB);%s", info, pidLatex, perTrackTitle),
		 100, -1, 1);
      histos2D[(prefix+pid+"_cosThetaBEAM_len_LAB").c_str()]=
	new TH2F((prefix+pid+"_cosThetaBEAM_len_LAB").c_str(),
		 Form("%s;%s track cos(#theta_{BEAM}) in LAB;%s track length [mm];%s", info, pidLatex,  pidLatex, perTrackTitle),
		 100, -1, 1,
		 maxLengthMM/binSizeMM, 0, maxLengthMM);
      profiles1D[(prefix+pid+"_cosThetaBEAM_len_LAB_prof").c_str()]=
	new TProfile((prefix+pid+"_cosThetaBEAM_len_LAB_prof").c_str(),
		     Form("%s;%s track cos(#theta_{BEAM}) in LAB;Average %s track length [mm];%s", info, pidLatex,  pidLatex, perTrackTitle),
		     100, -1, 1,
		     0, maxLengthMM);
      
      // TRACK OBSERVABLES IN CMS : per category / per track, only for 2,3-prong
      switch(categoryPID[c].size()) {
      case 3:
      case 2: // 2,3-prong
	histos1D[(prefix+pid+"_E_CMS").c_str()]=
	  new TH1F((prefix+pid+"_E_CMS").c_str(),
		   Form("%s;%s kinetic energy in CMS [MeV];%s", info, pidLatex, perTrackTitle),
		   100, 0, maxKineticEnergyMeV);
	histos1D[(prefix+pid+"_phiBEAM_CMS").c_str()]=
	  new TH1F((prefix+pid+"_phiBEAM_CMS").c_str(),
		   Form("%s;%s track #phi_{BEAM} in CMS [rad];%s", info, pidLatex, perTrackTitle),
		   100, -TMath::Pi(), TMath::Pi());
	histos1D[(prefix+pid+"_thetaBEAM_CMS").c_str()]=
	  new TH1F((prefix+pid+"_thetaBEAM_CMS").c_str(),
		   Form("%s;%s track #theta_{BEAM} in CMS [rad];%s", info, pidLatex, perTrackTitle),
		   100, 0, TMath::Pi());
	histos1D[(prefix+pid+"_cosThetaBEAM_CMS").c_str()]=
	  new TH1F((prefix+pid+"_cosThetaBEAM_CMS").c_str(),
		   Form("%s;%s track cos(#theta_{BEAM}) in CMS;%s", info, pidLatex, perTrackTitle),
		   100, -1, 1);
	histos2D[(prefix+pid+"_cosThetaBEAM_E_CMS").c_str()]=
	  new TH2F((prefix+pid+"_cosThetaBEAM_E_CMS").c_str(),
		   Form("%s;%s track cos(#theta_{BEAM}) in CMS;%s kinetic energy in CMS [MeV];%s", info, pidLatex, pidLatex, perTrackTitle),
		   100, -1, 1,
		   100, 0, maxKineticEnergyMeV);
	profiles1D[(prefix+pid+"_cosThetaBEAM_E_CMS_prof").c_str()]=
	  new TProfile((prefix+pid+"_cosThetaBEAM_E_CMS_prof").c_str(),
		       Form("%s;%s track cos(#theta_{BEAM}) in CMS;Average %s kinetic energy in CMS [MeV];%s", info, pidLatex, pidLatex, perTrackTitle),
		       100, -1, 1,
		       0, maxKineticEnergyMeV);
      case 1: // 1,2,3-prong
	histos1D[(prefix+pid+"_E_LAB").c_str()]=
	  new TH1F((prefix+pid+"_E_LAB").c_str(),
		   Form("%s;%s kinetic energy in LAB [MeV];%s", info, pidLatex, perTrackTitle),
		   100, 0, maxKineticEnergyMeV);
	histos2D[(prefix+pid+"_cosThetaBEAM_E_LAB").c_str()]=
	  new TH2F((prefix+pid+"_cosThetaBEAM_E_LAB").c_str(),
		   Form("%s;%s track cos(#theta_{BEAM}) in LAB;%s kinetic energy in LAB [MeV];%s", info, pidLatex, pidLatex, perTrackTitle),
		   100, -1, 1,
		   100, 0, maxKineticEnergyMeV);
	profiles1D[(prefix+pid+"_cosThetaBEAM_E_LAB_prof").c_str()]=
	  new TProfile((prefix+pid+"_cosThetaBEAM_E_LAB_prof").c_str(),
		       Form("%s;%s track cos(#theta_{BEAM}) in LAB;Average %s kinetic energy in LAB [MeV];%s", info, pidLatex, pidLatex, perTrackTitle),
		       100, -1, 1,
		       0, maxKineticEnergyMeV);
      default: break;
      };

      // HISTOGRAMS PER TRACK-TRACK PAIR
      for(auto t2=t+1; t2<categoryPIDhname[c].size(); ++t2) {
	auto pid2=categoryPIDhname[c].at(t2);
	auto pidLatex2=categoryPIDlatex[c].at(t2).c_str();

	// TRACK-TRACK DELTA(i,j) IN LAB FRAME : per category / per track pair
	histos1D[(prefix+pid+pid2+"_delta_LAB").c_str()]=
	  new TH1F((prefix+pid+pid2+"_delta_LAB").c_str(),
		   Form("%s;#delta(%s %s) angle in LAB [rad];%s", info, pidLatex, pidLatex2, perTrackTitle),
		   100, 0, TMath::Pi());
	histos1D[(prefix+pid+pid2+"_cosDelta_LAB").c_str()]=
	  new TH1F((prefix+pid+pid2+"_cosDelta_LAB").c_str(),
		   Form("%s;cos #delta(%s %s) in LAB [rad];%s", info, pidLatex, pidLatex2, perTrackTitle),
		   100, -1, 1);
	
	// TRACK-TRACK LENGTH(i) VS LENGTH(j) IN LAB FRAME : per category / per track pair
	histos2D[(prefix+pid+"_len"+pid2+"_len").c_str()]=
	  new TH2F((prefix+pid+"_len"+pid2+"_len").c_str(),
		   Form("%s;%s track length [mm];%s track length [mm];%s", info, pidLatex,  pidLatex2, perTrackTitle),
		   maxLengthMM/binSizeMM, 0, maxLengthMM,
		   maxLengthMM/binSizeMM, 0, maxLengthMM);

	// TRACK-TRACK E(i) VS E(j) IN LAB & CMS FRAME : per category / per track pair
	histos2D[(prefix+pid+"_E"+pid2+"_E_LAB").c_str()]=
	  new TH2F((prefix+pid+"_E"+pid2+"_E_LAB").c_str(),
		   Form("%s;%s kinetic energy in LAB [MeV];%s kinetic energy in LAB [MeV];%s", info, pidLatex,  pidLatex2, perTrackTitle),
		   100, 0, maxKineticEnergyMeV,
		   100, 0, maxKineticEnergyMeV);
	histos2D[(prefix+pid+"_E"+pid2+"_E_CMS").c_str()]=
	  new TH2F((prefix+pid+"_E"+pid2+"_E_CMS").c_str(),
		   Form("%s;%s kinetic energy in CMS [MeV];%s kinetic energy in CMS [MeV];%s", info, pidLatex,  pidLatex2, perTrackTitle),
		   100, 0, maxKineticEnergyMeV,
		   100, 0, maxKineticEnergyMeV);

	// TRACK-TRACK DELTA(i,j) IN CMS FRAME : per category / per track pair, only for 2,3-prong
	if(categoryPIDhname[c].size()>1) {
	  histos1D[(prefix+pid+pid2+"_delta_CMS").c_str()]=
	    new TH1F((prefix+pid+pid2+"_delta_CMS").c_str(),
		     Form("%s;#delta(%s %s) angle in CMS [rad];%s", info, pidLatex, pidLatex2, perTrackTitle),
		     100, 0, TMath::Pi());
	  histos1D[(prefix+pid+pid2+"_cosDelta_CMS").c_str()]=
	    new TH1F((prefix+pid+pid2+"_cosDelta_CMS").c_str(),
		     Form("%s;cos #delta(%s %s) in CMS [rad];%s", info, pidLatex, pidLatex2, perTrackTitle),
		     100, -1, 1);
	}
      }
    }
  }
}
///////////////////////////////
///////////////////////////////
void HIGGS_analysis::fillHistos(Track3D *aTrack){

  if( !eventFilter(aTrack) ) return;
  
  // The following assumptions are made:
  // - event is a collection of straight 3D segments
  // - 3D segments share common vertex (STARTING POINT of each segment)
  // - for 2-prong events: longer track is ALPHA, shorter is CARBON
  // - for 3-prong events: all tracks are ALPHAS, descending order by their energy/length
  
  const int ntracks = aTrack->getSegments().size();
  histos1D["ntracks"]->Fill(ntracks);
  if (ntracks==0) return;

  // get sorted list of tracks (descending order by track length)
  TrackSegment3DCollection list = aTrack->getSegments();
    std::sort(list.begin(), list.end(),
	    [](const TrackSegment3D& a, const TrackSegment3D& b) {
        return a.getLength() > b.getLength();
      });

  // ALL event categories
  TVector3 vertexPos = list.front().getStart();
  histos1D["h_all_vertexX"]->Fill(vertexPos.X());
  histos1D["h_all_vertexY"]->Fill(vertexPos.Y());
  histos1D["h_all_vertexZ"]->Fill(vertexPos.Z());
  histos2D["h_all_vertexXY"]->Fill(vertexPos.X(), vertexPos.Y());
  histos2D["h_all_vertexYZ"]->Fill(vertexPos.Y(), vertexPos.Z());
  profiles1D["h_all_vertexXY_prof"]->Fill(vertexPos.X(), vertexPos.Y());
  for(auto & track: list) {
    const double len=track.getLength();
    histos1D["h_all_len"]->Fill(len);
    histos1D["h_all_deltaX"]->Fill(len*track.getTangent().X());
    histos1D["h_all_deltaY"]->Fill(len*track.getTangent().Y());
    histos1D["h_all_deltaZ"]->Fill(len*track.getTangent().Z());
    histos2D["h_all_deltaXY"]->Fill(len*track.getTangent().X(), len*track.getTangent().Y());
    histos2D["h_all_deltaXZ"]->Fill(len*track.getTangent().X(), len*track.getTangent().Z());
    histos2D["h_all_deltaYZ"]->Fill(len*track.getTangent().Y(), len*track.getTangent().Z());
    histos1D["h_all_endX"]->Fill(track.getEnd().X());
    histos1D["h_all_endY"]->Fill(track.getEnd().Y());
    histos1D["h_all_endZ"]->Fill(track.getEnd().Z());
    histos2D["h_all_endXY"]->Fill(track.getEnd().X(), track.getEnd().Y());
    histos1D["h_all_phiDET"]->Fill(track.getTangent().Phi());
    histos1D["h_all_thetaDET"]->Fill(track.getTangent().Theta());
    histos1D["h_all_cosThetaDET"]->Fill(track.getTangent().CosTheta());

    // calculate angles in LAB reference frame in BEAM coordinate system
    // TODO
    // TODO switch to DET->BEAM dedicated converter class!!!
    // TODO
    // change DET-->BEAM coordinate transformation for the HIGS experiment
    // formulas below are valid provided that beam direction is anti-paralell to X_DET (HIGS case):
    // X_DET -> -Z_BEAM
    // Y_DET ->  X_BEAM
    // Z_DET -> -Y_BEAM
    double phi_BEAM_LAB=atan2(-track.getTangent().Z(), track.getTangent().Y()); // [rad], azimuthal angle from horizontal axis
    double cosTheta_BEAM_LAB=track.getTangent()*photonUnitVec_DET_LAB; // polar angle wrt beam axis
    histos1D["h_all_phiBEAM_LAB"]->Fill(phi_BEAM_LAB);
    histos1D["h_all_thetaBEAM_LAB"]->Fill(acos(cosTheta_BEAM_LAB));
    histos1D["h_all_cosThetaBEAM_LAB"]->Fill(cosTheta_BEAM_LAB);
    histos2D["h_all_cosThetaBEAM_len_LAB"]->Fill(cosTheta_BEAM_LAB, len);
    profiles1D["h_all_cosThetaBEAM_len_LAB_prof"]->Fill(cosTheta_BEAM_LAB, len);
  }

  // 1-prong (alpha)
  if(ntracks==1) {
    histos1D["h_1prong_vertexX"]->Fill(vertexPos.X());
    histos1D["h_1prong_vertexY"]->Fill(vertexPos.Y());
    histos1D["h_1prong_vertexZ"]->Fill(vertexPos.Z());
    histos2D["h_1prong_vertexXY"]->Fill(vertexPos.X(), vertexPos.Y());
    histos2D["h_1prong_vertexYZ"]->Fill(vertexPos.Y(), vertexPos.Z());
    profiles1D["h_1prong_vertexXY_prof"]->Fill(vertexPos.X(), vertexPos.Y());
    auto track=list.front();
    const double len=track.getLength(); // [mm]
    histos1D["h_1prong_alpha_len"]->Fill(len);
    histos1D["h_1prong_alpha_deltaX"]->Fill(len*track.getTangent().X());
    histos1D["h_1prong_alpha_deltaY"]->Fill(len*track.getTangent().Y());
    histos1D["h_1prong_alpha_deltaZ"]->Fill(len*track.getTangent().Z());
    histos2D["h_1prong_alpha_deltaXY"]->Fill(len*track.getTangent().X(), len*track.getTangent().Y());
    histos2D["h_1prong_alpha_deltaXZ"]->Fill(len*track.getTangent().X(), len*track.getTangent().Z());
    histos2D["h_1prong_alpha_deltaYZ"]->Fill(len*track.getTangent().Y(), len*track.getTangent().Z());
    histos1D["h_1prong_alpha_endX"]->Fill(track.getEnd().X());
    histos1D["h_1prong_alpha_endY"]->Fill(track.getEnd().Y());
    histos1D["h_1prong_alpha_endZ"]->Fill(track.getEnd().Z());
    histos2D["h_1prong_alpha_endXY"]->Fill(track.getEnd().X(), track.getEnd().Y());
    histos1D["h_1prong_alpha_phiDET"]->Fill(track.getTangent().Phi());
    histos1D["h_1prong_alpha_thetaDET"]->Fill(track.getTangent().Theta());
    histos1D["h_1prong_alpha_cosThetaDET"]->Fill(track.getTangent().CosTheta());

    // calculate angles in LAB reference frame in BEAM coordinate system
    // TODO
    // TODO switch to DET->BEAM dedicated converter class!!!
    // TODO
    // change DET-->BEAM coordinate transformation for the HIGS experiment
    // formulas below are valid provided that beam direction is anti-paralell to X_DET (HIGS case):
    // X_DET -> -Z_BEAM
    // Y_DET ->  X_BEAM
    // Z_DET -> -Y_BEAM
    double phi_BEAM_LAB=atan2(-track.getTangent().Z(), track.getTangent().Y()); // [rad], azimuthal angle from horizontal axis
    double cosTheta_BEAM_LAB=track.getTangent()*photonUnitVec_DET_LAB; // polar angle wrt beam axis
    histos1D["h_1prong_alpha_phiBEAM_LAB"]->Fill(phi_BEAM_LAB);
    histos1D["h_1prong_alpha_thetaBEAM_LAB"]->Fill(acos(cosTheta_BEAM_LAB));
    histos1D["h_1prong_alpha_cosThetaBEAM_LAB"]->Fill(cosTheta_BEAM_LAB);
    histos2D["h_1prong_alpha_cosThetaBEAM_len_LAB"]->Fill(cosTheta_BEAM_LAB, len);
    profiles1D["h_1prong_alpha_cosThetaBEAM_len_LAB_prof"]->Fill(cosTheta_BEAM_LAB, len);

    // reconstruct kinetic energy from particle range [mm]
    double T_LAB=myRangeCalculator.getIonEnergyMeV(IonRangeCalculator::ALPHA, len);
    //    double p_LAB=sqrt(T_LAB*(T_LAB+2*alphaMass));
    histos1D["h_1prong_alpha_E_LAB"]->Fill(T_LAB);
    histos2D["h_1prong_alpha_cosThetaBEAM_E_LAB"]->Fill(cosTheta_BEAM_LAB, T_LAB);
    profiles1D["h_1prong_alpha_cosThetaBEAM_E_LAB_prof"]->Fill(cosTheta_BEAM_LAB, T_LAB);
  }

  // 2-prong (alpha+carbon)
  if(ntracks==2) {
    histos1D["h_2prong_vertexX"]->Fill(vertexPos.X());
    histos1D["h_2prong_vertexY"]->Fill(vertexPos.Y());
    histos1D["h_2prong_vertexZ"]->Fill(vertexPos.Z());
    histos2D["h_2prong_vertexXY"]->Fill(vertexPos.X(), vertexPos.Y());
    histos2D["h_2prong_vertexYZ"]->Fill(vertexPos.Y(), vertexPos.Z());
    profiles1D["h_2prong_vertexXY_prof"]->Fill(vertexPos.X(), vertexPos.Y());

    const double alpha_len = list.front().getLength(); // longest = alpha
    const double carbon_len = list.back().getLength(); // shortest = carbon
    histos1D["h_2prong_alpha_len"]->Fill(alpha_len);
    histos2D["h_2prong_alpha_len_carbon_len"]->Fill(alpha_len, carbon_len);
    histos1D["h_2prong_lenSUM"]->Fill(alpha_len+carbon_len);
    histos1D["h_2prong_alpha_deltaX"]->Fill(alpha_len*list.front().getTangent().X());
    histos1D["h_2prong_alpha_deltaY"]->Fill(alpha_len*list.front().getTangent().Y());
    histos1D["h_2prong_alpha_deltaZ"]->Fill(alpha_len*list.front().getTangent().Z());
    histos2D["h_2prong_alpha_deltaXY"]->Fill(alpha_len*list.front().getTangent().X(), alpha_len*list.front().getTangent().Y());
    histos2D["h_2prong_alpha_deltaXZ"]->Fill(alpha_len*list.front().getTangent().X(), alpha_len*list.front().getTangent().Z());
    histos2D["h_2prong_alpha_deltaYZ"]->Fill(alpha_len*list.front().getTangent().Y(), alpha_len*list.front().getTangent().Z());
    histos1D["h_2prong_alpha_endX"]->Fill(list.front().getEnd().X());
    histos1D["h_2prong_alpha_endY"]->Fill(list.front().getEnd().Y());
    histos1D["h_2prong_alpha_endZ"]->Fill(list.front().getEnd().Z());
    histos2D["h_2prong_alpha_endXY"]->Fill(list.front().getEnd().X(), list.front().getEnd().Y());
    histos1D["h_2prong_alpha_phiDET"]->Fill(list.front().getTangent().Phi());
    histos1D["h_2prong_alpha_thetaDET"]->Fill(list.front().getTangent().Theta());
    histos1D["h_2prong_alpha_cosThetaDET"]->Fill(list.front().getTangent().CosTheta());
    histos1D["h_2prong_carbon_len"]->Fill(carbon_len);
    histos1D["h_2prong_carbon_deltaX"]->Fill(carbon_len*list.back().getTangent().X());
    histos1D["h_2prong_carbon_deltaY"]->Fill(carbon_len*list.back().getTangent().Y());
    histos1D["h_2prong_carbon_deltaZ"]->Fill(carbon_len*list.back().getTangent().Z());
    histos2D["h_2prong_carbon_deltaXY"]->Fill(carbon_len*list.back().getTangent().X(), carbon_len*list.back().getTangent().Y());
    histos2D["h_2prong_carbon_deltaXZ"]->Fill(carbon_len*list.back().getTangent().X(), carbon_len*list.back().getTangent().Z());
    histos2D["h_2prong_carbon_deltaYZ"]->Fill(carbon_len*list.back().getTangent().Y(), carbon_len*list.back().getTangent().Z());
    histos1D["h_2prong_carbon_endX"]->Fill(list.back().getEnd().X());
    histos1D["h_2prong_carbon_endY"]->Fill(list.back().getEnd().Y());
    histos1D["h_2prong_carbon_endZ"]->Fill(list.back().getEnd().Z());
    histos2D["h_2prong_carbon_endXY"]->Fill(list.back().getEnd().X(), list.back().getEnd().Y());

    // calculate angles in LAB reference frame in DET coordinate system
    double delta_LAB=list.front().getTangent().Angle(list.back().getTangent()); // [rad]
    histos1D["h_2prong_carbon_phiDET"]->Fill(list.back().getTangent().Phi());
    histos1D["h_2prong_carbon_thetaDET"]->Fill(list.back().getTangent().Theta());
    histos1D["h_2prong_carbon_cosThetaDET"]->Fill(list.back().getTangent().CosTheta());
    histos1D["h_2prong_alpha_carbon_delta_LAB"]->Fill(delta_LAB);
    histos1D["h_2prong_alpha_carbon_cosDelta_LAB"]->Fill(cos(delta_LAB));

    // calculate angles in LAB reference frame in BEAM coordinate system
    // TODO
    // TODO switch to DET->BEAM dedicated converter class!!!
    // TODO
    // change DET-->BEAM coordinate transformation for the HIGS experiment
    // formulas below are valid provided that beam direction is anti-paralell to X_DET (HIGS case):
    // X_DET -> -Z_BEAM
    // Y_DET ->  X_BEAM
    // Z_DET -> -Y_BEAM
    double alpha_phi_BEAM_LAB=atan2(-list.front().getTangent().Z(), list.front().getTangent().Y()); // [rad], azimuthal angle from horizontal axis
    double carbon_phi_BEAM_LAB=atan2(-list.back().getTangent().Z(), list.back().getTangent().Y()); // [rad], azimuthal angle from horizontal axis
    double alpha_cosTheta_BEAM_LAB=list.front().getTangent()*photonUnitVec_DET_LAB; // polar angle wrt beam axis
    double carbon_cosTheta_BEAM_LAB=list.back().getTangent()*photonUnitVec_DET_LAB; // polar angle wrt beam axis
    histos1D["h_2prong_alpha_phiBEAM_LAB"]->Fill(alpha_phi_BEAM_LAB);
    histos1D["h_2prong_alpha_thetaBEAM_LAB"]->Fill(acos(alpha_cosTheta_BEAM_LAB));
    histos1D["h_2prong_alpha_cosThetaBEAM_LAB"]->Fill(alpha_cosTheta_BEAM_LAB);
    histos2D["h_2prong_alpha_cosThetaBEAM_len_LAB"]->Fill(alpha_cosTheta_BEAM_LAB, alpha_len);
    profiles1D["h_2prong_alpha_cosThetaBEAM_len_LAB_prof"]->Fill(alpha_cosTheta_BEAM_LAB, alpha_len);
    histos1D["h_2prong_carbon_phiBEAM_LAB"]->Fill(carbon_phi_BEAM_LAB);
    histos1D["h_2prong_carbon_thetaBEAM_LAB"]->Fill(acos(carbon_cosTheta_BEAM_LAB));
    histos1D["h_2prong_carbon_cosThetaBEAM_LAB"]->Fill(carbon_cosTheta_BEAM_LAB);
    histos2D["h_2prong_carbon_cosThetaBEAM_len_LAB"]->Fill(carbon_cosTheta_BEAM_LAB, carbon_len);
    profiles1D["h_2prong_carbon_cosThetaBEAM_len_LAB_prof"]->Fill(carbon_cosTheta_BEAM_LAB, carbon_len);

    // reconstruct kinetic energy from particle range [mm]
    const double alphaMass=myRangeCalculator.getIonMassMeV(IonRangeCalculator::ALPHA);
    const double carbonMass=myRangeCalculator.getIonMassMeV(IonRangeCalculator::CARBON_12);
    const double alpha_T_LAB=myRangeCalculator.getIonEnergyMeV(IonRangeCalculator::ALPHA, alpha_len);
    const double carbon_T_LAB=myRangeCalculator.getIonEnergyMeV(IonRangeCalculator::CARBON_12, carbon_len);
    double alpha_p_LAB=sqrt(alpha_T_LAB*(alpha_T_LAB+2*alphaMass));
    double carbon_p_LAB=sqrt(carbon_T_LAB*(carbon_T_LAB+2*carbonMass));
    // construct TLorentzVector in DET/LAB frame
    TLorentzVector alphaP4_DET_LAB(alpha_p_LAB*list.front().getTangent(), alphaMass+alpha_T_LAB);
    TLorentzVector carbonP4_DET_LAB(carbon_p_LAB*list.back().getTangent(), carbonMass+carbon_T_LAB);
    // boost P4 from DET/LAB frame to CMS frame (see TLorentzVector::Boost() convention!)
    const TVector3 beta_DET_LAB=getBetaVectorOfCMS(myRangeCalculator.getIonMassMeV(IonRangeCalculator::OXYGEN_16));
    TLorentzVector alphaP4_CMS_DET(alphaP4_DET_LAB);
    TLorentzVector carbonP4_CMS_DET(carbonP4_DET_LAB);
    alphaP4_CMS_DET.Boost(-1.0*beta_DET_LAB); // see TLorentzVector::Boost for sign convention!
    carbonP4_CMS_DET.Boost(-1.0*beta_DET_LAB); // see TLorentzVector::Boost for sign convention!
    // TODO
    // TODO switch to DET->BEAM dedicated converter class!!!
    // TODO
    // change DET-->BEAM coordinate transformation for the HIGS experiment
    // formulas below are valid provided that beam direction is anti-paralell to X_DET (HIGS case):
    // X_DET -> -Z_BEAM
    // Y_DET ->  X_BEAM
    // Z_DET -> -Y_BEAM
    TLorentzVector alphaP4_BEAM_CMS(alphaP4_CMS_DET.Py(), -alphaP4_CMS_DET.Pz(), -alphaP4_CMS_DET.Px(), alphaP4_CMS_DET.E());
    TLorentzVector carbonP4_BEAM_CMS(carbonP4_CMS_DET.Py(), -carbonP4_CMS_DET.Pz(), -carbonP4_CMS_DET.Px(), carbonP4_CMS_DET.E());
    double alpha_T_CMS=alphaP4_BEAM_CMS.E()-alphaP4_BEAM_CMS.M(); // [MeV]
    double carbon_T_CMS=carbonP4_BEAM_CMS.E()-carbonP4_BEAM_CMS.M(); // [MeV]
    double invariantMass=(alphaP4_BEAM_CMS+carbonP4_BEAM_CMS).M();// [MeV/c^2]
    double totalEnergy_CMS=(alphaP4_BEAM_CMS+carbonP4_BEAM_CMS).E(); // [MeV], mass of stationary excited Oxygen state
    double oxygenMassExcited=totalEnergy_CMS;
    double Qvalue_CMS=oxygenMassExcited-alphaMass-carbonMass;
    histos1D["h_2prong_alpha_E_LAB"]->Fill(alpha_T_LAB);
    histos1D["h_2prong_carbon_E_LAB"]->Fill(carbon_T_LAB);
    histos1D["h_2prong_alpha_E_CMS"]->Fill(alpha_T_CMS);
    histos1D["h_2prong_carbon_E_CMS"]->Fill(carbon_T_CMS);
    histos1D["h_2prong_total_PxBEAM_CMS"]->Fill((alphaP4_BEAM_CMS+carbonP4_BEAM_CMS).Px());
    histos1D["h_2prong_total_PyBEAM_CMS"]->Fill((alphaP4_BEAM_CMS+carbonP4_BEAM_CMS).Py());
    histos1D["h_2prong_total_PzBEAM_CMS"]->Fill((alphaP4_BEAM_CMS+carbonP4_BEAM_CMS).Pz());
    histos1D["h_2prong_total_E_CMS"]->Fill(totalEnergy_CMS);
    histos1D["h_2prong_invMass"]->Fill(invariantMass);
    histos1D["h_2prong_Qvalue_CMS"]->Fill(Qvalue_CMS);

    // calculate angles in CMS reference frame in BEAM coordinate system
    double delta_CMS=alphaP4_BEAM_CMS.Angle(carbonP4_BEAM_CMS.Vect()); // [rad]
    double alpha_phi_BEAM_CMS=alphaP4_BEAM_CMS.Phi(); // [rad], azimuthal angle from X axis
    double carbon_phi_BEAM_CMS=carbonP4_BEAM_CMS.Phi(); // [rad], azimuthal angle from X axis
    double alpha_cosTheta_BEAM_CMS=alphaP4_BEAM_CMS.CosTheta(); // [rad], polar angle from Z axis
    double carbon_cosTheta_BEAM_CMS=carbonP4_BEAM_CMS.CosTheta(); // [rad], polar angle from Z axis
    histos1D["h_2prong_alpha_carbon_delta_CMS"]->Fill(delta_CMS);
    histos1D["h_2prong_alpha_carbon_cosDelta_CMS"]->Fill(cos(delta_CMS));
    histos1D["h_2prong_alpha_phiBEAM_CMS"]->Fill(alpha_phi_BEAM_CMS);
    histos1D["h_2prong_alpha_thetaBEAM_CMS"]->Fill(acos(alpha_cosTheta_BEAM_CMS));
    histos1D["h_2prong_alpha_cosThetaBEAM_CMS"]->Fill(alpha_cosTheta_BEAM_CMS);
    histos1D["h_2prong_carbon_phiBEAM_CMS"]->Fill(carbon_phi_BEAM_CMS);
    histos1D["h_2prong_carbon_thetaBEAM_CMS"]->Fill(acos(carbon_cosTheta_BEAM_CMS));
    histos1D["h_2prong_carbon_cosThetaBEAM_CMS"]->Fill(carbon_cosTheta_BEAM_CMS);
    histos2D["h_2prong_alpha_cosThetaBEAM_E_CMS"]->Fill(alpha_cosTheta_BEAM_CMS, alpha_T_CMS);
    profiles1D["h_2prong_alpha_cosThetaBEAM_E_CMS_prof"]->Fill(alpha_cosTheta_BEAM_CMS, alpha_T_CMS);
    histos2D["h_2prong_alpha_cosThetaBEAM_E_LAB"]->Fill(alpha_cosTheta_BEAM_LAB, alpha_T_LAB);
    profiles1D["h_2prong_alpha_cosThetaBEAM_E_LAB_prof"]->Fill(alpha_cosTheta_BEAM_LAB, alpha_T_LAB);
    histos2D["h_2prong_carbon_cosThetaBEAM_E_CMS"]->Fill(carbon_cosTheta_BEAM_CMS, carbon_T_CMS);
    profiles1D["h_2prong_carbon_cosThetaBEAM_E_CMS_prof"]->Fill(carbon_cosTheta_BEAM_CMS, carbon_T_CMS);
    histos2D["h_2prong_carbon_cosThetaBEAM_E_LAB"]->Fill(carbon_cosTheta_BEAM_LAB, carbon_T_LAB);
    profiles1D["h_2prong_carbon_cosThetaBEAM_E_LAB_prof"]->Fill(carbon_cosTheta_BEAM_LAB, carbon_T_LAB);
    histos2D["h_2prong_alpha_E_carbon_E_CMS"]->Fill(alpha_T_CMS, carbon_T_CMS);
    histos2D["h_2prong_alpha_E_carbon_E_LAB"]->Fill(alpha_T_LAB, carbon_T_LAB);
  }
  // 3-prong (triple alpha)
  if(ntracks==3) {
    histos1D["h_3prong_vertexX"]->Fill(vertexPos.X());
    histos1D["h_3prong_vertexY"]->Fill(vertexPos.Y());
    histos1D["h_3prong_vertexZ"]->Fill(vertexPos.Z());
    histos2D["h_3prong_vertexXY"]->Fill(vertexPos.X(), vertexPos.Y());
    histos2D["h_3prong_vertexYZ"]->Fill(vertexPos.Y(), vertexPos.Z());
    profiles1D["h_3prong_vertexXY_prof"]->Fill(vertexPos.X(), vertexPos.Y());

    const TVector3 beta_DET_LAB=getBetaVectorOfCMS(myRangeCalculator.getIonMassMeV(IonRangeCalculator::CARBON_12));
    const double alphaMass=myRangeCalculator.getIonMassMeV(IonRangeCalculator::ALPHA);

    // initialize array of track properties
    double alpha_len[3]; // [mm]
    double alpha_phi_BEAM_LAB[3]; // [rad]
    double alpha_phi_BEAM_CMS[3]; // [rad]
    double alpha_cosTheta_BEAM_LAB[3]; // [-1, 1]
    double alpha_cosTheta_BEAM_CMS[3]; // [-1, 1]
    double alpha_T_LAB[3]; // [MeV]
    double alpha_T_CMS[3]; // [MeV]
    double alpha_p_LAB[3]; // [MeV/c]
    TLorentzVector alphaP4_DET_LAB[3]; // [MeV]
    TLorentzVector alphaP4_DET_CMS[3]; // [MeV]
    TLorentzVector alphaP4_BEAM_CMS[3]; // [MeV]
    // initialize total sums
    double lengthSUM=0.0; // [mm]
    double massSUM=0.0; // [MeV]
    TLorentzVector sumP4_BEAM_CMS(0,0,0,0); // [MeV]

    // calculate array of track properties and total sums
    for(auto i=0;i<3;i++) {
      auto track=list.at(i);
      alpha_len[i] = track.getLength();
      
      // calculate angles in LAB reference frame in BEAM coordinate system
      // TODO
      // TODO switch to DET->BEAM dedicated converter class!!!
      // TODO
      // change DET-->BEAM coordinate transformation for the HIGS experiment
      // formulas below are valid provided that beam direction is anti-paralell to X_DET (HIGS case):
      // X_DET -> -Z_BEAM
      // Y_DET ->  X_BEAM
      // Z_DET -> -Y_BEAM
      alpha_phi_BEAM_LAB[i]=atan2(-track.getTangent().Z(), track.getTangent().Y()); // [rad], azimuthal angle from horizontal axis
      alpha_cosTheta_BEAM_LAB[i]=track.getTangent()*photonUnitVec_DET_LAB; // polar angle wrt beam axis
      
      // reconstruct kinetic energy from particle range [mm]
      alpha_T_LAB[i]=myRangeCalculator.getIonEnergyMeV(IonRangeCalculator::ALPHA, alpha_len[i]);
      alpha_p_LAB[i]=sqrt(alpha_T_LAB[i]*(alpha_T_LAB[i]+2*alphaMass));
      // construct TLorentzVector in DET/LAB frame
      alphaP4_DET_LAB[i]=TLorentzVector(alpha_p_LAB[i]*track.getTangent(), alphaMass+alpha_T_LAB[i]);
      // boost P4 from DET/LAB frame to CMS frame (see TLorentzVector::Boost() convention!)
      alphaP4_DET_CMS[i]=TLorentzVector(alphaP4_DET_LAB[i]);
      alphaP4_DET_CMS[i].Boost(-1.0*beta_DET_LAB);
      // TODO
      // TODO switch to DET->BEAM dedicated converter class!!!
      // TODO
      // change DET-->BEAM coordinate transformation for the HIGS experiment
      // formulas below are valid provided that beam direction is anti-paralell to X_DET (HIGS case):
      // X_DET -> -Z_BEAM
      // Y_DET ->  X_BEAM
      // Z_DET -> -Y_BEAM
      alphaP4_BEAM_CMS[i]=TLorentzVector(alphaP4_DET_CMS[i].Py(), -alphaP4_DET_CMS[i].Pz(), -alphaP4_DET_CMS[i].Px(), alphaP4_DET_CMS[i].E());
      alpha_T_CMS[i]=alphaP4_BEAM_CMS[i].E()-alphaP4_BEAM_CMS[i].M(); // [MeV]
      alpha_phi_BEAM_CMS[i]=alphaP4_BEAM_CMS[i].Phi(); // [rad], azimuthal angle from X axis
      alpha_cosTheta_BEAM_CMS[i]=alphaP4_BEAM_CMS[i].CosTheta(); // [rad], azimuthal angle from X axis
      
      // update total sums
      lengthSUM+=alpha_len[i];
      massSUM+=alphaMass;
      sumP4_BEAM_CMS+=alphaP4_BEAM_CMS[i];
    }

    // fill properties per track
    for(auto i=0;i<3;i++) {
      auto track=list.at(i);
      histos1D[Form("h_3prong_alpha%d_len",i+1)]->Fill(alpha_len[i]);
      histos1D[Form("h_3prong_alpha%d_deltaX",i+1)]->Fill(alpha_len[i]*track.getTangent().X());
      histos1D[Form("h_3prong_alpha%d_deltaY",i+1)]->Fill(alpha_len[i]*track.getTangent().Y());
      histos1D[Form("h_3prong_alpha%d_deltaZ",i+1)]->Fill(alpha_len[i]*track.getTangent().Z());
      histos2D[Form("h_3prong_alpha%d_deltaXY",i+1)]->Fill(alpha_len[i]*track.getTangent().X(), alpha_len[i]*track.getTangent().Y());
      histos2D[Form("h_3prong_alpha%d_deltaXZ",i+1)]->Fill(alpha_len[i]*track.getTangent().X(), alpha_len[i]*track.getTangent().Z());
      histos2D[Form("h_3prong_alpha%d_deltaYZ",i+1)]->Fill(alpha_len[i]*track.getTangent().Y(), alpha_len[i]*track.getTangent().Z());
      histos1D[Form("h_3prong_alpha%d_endX",i+1)]->Fill(track.getEnd().X());
      histos1D[Form("h_3prong_alpha%d_endY",i+1)]->Fill(track.getEnd().Y());
      histos1D[Form("h_3prong_alpha%d_endZ",i+1)]->Fill(track.getEnd().Z());
      histos2D[Form("h_3prong_alpha%d_endXY",i+1)]->Fill(track.getEnd().X(), track.getEnd().Y());
      histos1D[Form("h_3prong_alpha%d_phiDET",i+1)]->Fill(track.getTangent().Phi());
      histos1D[Form("h_3prong_alpha%d_thetaDET",i+1)]->Fill(track.getTangent().Theta());
      histos1D[Form("h_3prong_alpha%d_cosThetaDET",i+1)]->Fill(track.getTangent().CosTheta());

      // properties in LAB reference frame in BEAM coordinate system
      histos1D[Form("h_3prong_alpha%d_phiBEAM_LAB",i+1)]->Fill(alpha_phi_BEAM_LAB[i]);
      histos1D[Form("h_3prong_alpha%d_thetaBEAM_LAB",i+1)]->Fill(acos(alpha_cosTheta_BEAM_LAB[i]));
      histos1D[Form("h_3prong_alpha%d_cosThetaBEAM_LAB",i+1)]->Fill(alpha_cosTheta_BEAM_LAB[i]);
      histos2D[Form("h_3prong_alpha%d_cosThetaBEAM_len_LAB",i+1)]->Fill(alpha_cosTheta_BEAM_LAB[i], alpha_len[i]);
      profiles1D[Form("h_3prong_alpha%d_cosThetaBEAM_len_LAB_prof",i+1)]->Fill(alpha_cosTheta_BEAM_LAB[i], alpha_len[i]);
      histos2D[Form("h_3prong_alpha%d_cosThetaBEAM_E_LAB",i+1)]->Fill(alpha_cosTheta_BEAM_LAB[i], alpha_T_LAB[i]);
      profiles1D[Form("h_3prong_alpha%d_cosThetaBEAM_E_LAB_prof",i+1)]->Fill(alpha_cosTheta_BEAM_LAB[i], alpha_T_LAB[i]);
      histos1D[Form("h_3prong_alpha%d_E_LAB",i+1)]->Fill(alpha_T_LAB[i]);

      // properties in CMS reference frame in BEAM coordinate system
      histos1D[Form("h_3prong_alpha%d_phiBEAM_CMS",i+1)]->Fill(alpha_phi_BEAM_CMS[i]);
      histos1D[Form("h_3prong_alpha%d_thetaBEAM_CMS",i+1)]->Fill(acos(alpha_cosTheta_BEAM_CMS[i]));
      histos1D[Form("h_3prong_alpha%d_cosThetaBEAM_CMS",i+1)]->Fill(alpha_cosTheta_BEAM_CMS[i]);
      histos2D[Form("h_3prong_alpha%d_cosThetaBEAM_E_CMS",i+1)]->Fill(alpha_cosTheta_BEAM_CMS[i], alpha_T_CMS[i]);
      profiles1D[Form("h_3prong_alpha%d_cosThetaBEAM_E_CMS_prof",i+1)]->Fill(alpha_cosTheta_BEAM_CMS[i], alpha_T_CMS[i]);
      histos1D[Form("h_3prong_alpha%d_E_CMS",i+1)]->Fill(alpha_T_CMS[i]);
      
      // fill properties per track pair
      for(auto i2=i+1;i2<3;i2++) {

	// properties in LAB reference frame in DET coordinate system
	double delta_LAB=track.getTangent().Angle(list.at(i2).getTangent()); // [rad]
	histos1D[Form("h_3prong_alpha%d_alpha%d_delta_LAB",i+1,i2+1)]->Fill(delta_LAB);
	histos1D[Form("h_3prong_alpha%d_alpha%d_cosDelta_LAB",i+1,i2+1)]->Fill(cos(delta_LAB));
	histos2D[Form("h_3prong_alpha%d_len_alpha%d_len",i+1,i2+1)]->Fill(alpha_len[i], alpha_len[i2]);
	// properties in CMS reference frame in BEAM coordinate system
	double delta_CMS=alphaP4_BEAM_CMS[i].Angle(alphaP4_BEAM_CMS[i2].Vect()); // [rad]
	histos1D[Form("h_3prong_alpha%d_alpha%d_delta_CMS",i+1,i2+1)]->Fill(delta_CMS);
	histos1D[Form("h_3prong_alpha%d_alpha%d_cosDelta_CMS",i+1,i2+1)]->Fill(cos(delta_CMS));
	histos2D[Form("h_3prong_alpha%d_E_alpha%d_E_LAB",i+1,i2+1)]->Fill(alpha_T_LAB[i], alpha_T_LAB[i2]); 
	histos2D[Form("h_3prong_alpha%d_E_alpha%d_E_CMS",i+1,i2+1)]->Fill(alpha_T_CMS[i], alpha_T_CMS[i2]);
     }
    }

    // fill total sums
    double invariantMass=sumP4_BEAM_CMS.M(); // [MeV]
    double totalEnergy_CMS=sumP4_BEAM_CMS.E(); // [MeV], mass of stationary excited Carbon state
    double carbonMassExcited=totalEnergy_CMS;
    double Qvalue_CMS=carbonMassExcited-massSUM;
    histos1D["h_3prong_lenSUM"]->Fill(lengthSUM);
    histos1D["h_3prong_total_PxBEAM_CMS"]->Fill(sumP4_BEAM_CMS.Px());
    histos1D["h_3prong_total_PyBEAM_CMS"]->Fill(sumP4_BEAM_CMS.Py());
    histos1D["h_3prong_total_PzBEAM_CMS"]->Fill(sumP4_BEAM_CMS.Pz());
    histos1D["h_3prong_total_E_CMS"]->Fill(totalEnergy_CMS);
    histos1D["h_3prong_invMass"]->Fill(invariantMass);
    histos1D["h_3prong_Qvalue_CMS"]->Fill(Qvalue_CMS);
  }
  //////// DEBUG
  //  auto l = new TPolyLine3D(ntracks*3);
  //  for(auto i=0; i<ntracks; i++) {
  //    l->SetPoint(i*3, list.at(i).getStart().X(), list.at(i).getStart().Y(), list.at(i).getStart().Z());
  //    l->SetPoint(i*3+1, list.at(i).getEnd().X(), list.at(i).getEnd().Y(), list.at(i).getEnd().Z());
  //    l->SetPoint(i*3+2, list.at(i).getStart().X(), list.at(i).getStart().Y(), list.at(i).getStart().Z());
  //  }
  //  l->SetLineColor(kBlack);
  //  l->Draw();
  //////// DEBUG
}
///////////////////////////////
///////////////////////////////
bool HIGGS_analysis::eventFilter(Track3D *aTrack){

  //  return true; // always pass, no cuts

  // reject empty events
  TrackSegment3DCollection list = aTrack->getSegments();
  if(list.size()==0) return false;
  
  // vertex cuts per event
  TVector3 vertexPos = list.front().getStart();
  if( fabs(vertexPos.Y()) > 6.0 ) return false;
    
  // endpoint cuts per track per event
  for(auto &track: list) {  
    if( fabs(track.getEnd().X()) > 160.0 ) return false;
    //    if( fabs(track.getEnd().Y()) > 80.0 ) return false;
  }
  
  // length correlation cuts per track pair per event
  for(auto i=0u; i<list.size()-1; i++) {
    for(auto j=i; j<list.size(); j++) {
      if( fabs(list.at(i).getLength()*list.at(i).getTangent().Z() -
	       list.at(j).getLength()*list.at(j).getTangent().Z()) > 180.0 ) return false;
      if( list.at(i).getLength()+list.at(j).getLength() < 50.0 ) return false;
    }
  }
  return true; // event passed all cuts
}
///////////////////////////////
///////////////////////////////
void HIGGS_analysis::finalize(){

  for (auto &h : histos1D) {
    h.second->SetTitleOffset(1.3, "X");
    h.second->SetTitleOffset(1.4, "Y");
    h.second->SetOption("COLZ");
  }
  for (auto &h : histos2D) {
    h.second->SetTitleOffset(1.4, "X");
    h.second->SetTitleOffset(1.4, "Y");
    h.second->SetOption("COLZ");
  }
  for (auto &p : profiles1D) {
    p.second->SetTitleOffset(1.4, "X");
    p.second->SetTitleOffset(1.4, "Y");
  }
  ////////// DEBUG
  //  outputCanvas->Update();
  //  outputCanvas->Modified();
  //  outputCanvas->Write();
  ////////// DEBUG
  outputFile->Write();
}
///////////////////////////////
///////////////////////////////
void HIGGS_analysis::setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr){
  
  myGeometryPtr = aGeometryPtr;
}
///////////////////////////////
///////////////////////////////
void HIGGS_analysis::setBeamProperties(float beamEnergyInMeV, // nominal gamma beam energy [MeV] in LAB reference frame
				       TVector3 beamDir) { // nominal gamma beam direction in LAB reference frame and detector coordinate system
  photonEnergyInMeV_LAB = fabs(beamEnergyInMeV);
  photonUnitVec_DET_LAB = beamDir.Unit();
}
///////////////////////////////
///////////////////////////////
// dimensionless speed (c=1) of gamma-nucleus CMS reference frame wrt LAB reference frame in detector coordinate system
TVector3 HIGGS_analysis::getBetaVectorOfCMS(double nucleusMassInMeV) {
  return getBetaOfCMS(nucleusMassInMeV)*photonUnitVec_DET_LAB;
}
///////////////////////////////
///////////////////////////////
// dimensionless speed (c=1) of gamma-nucleus CMS reference frame wrt LAB reference frame in detector coordinate system
double HIGGS_analysis::getBetaOfCMS(double nucleusMassInMeV) {
  return photonEnergyInMeV_LAB/(photonEnergyInMeV_LAB+nucleusMassInMeV);
}
