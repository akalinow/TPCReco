#include <vector>
#include <iostream>

#include <TH1D.h>
#include <TH2D.h>
#include <TProfile.h>
#include <TProfile2D.h>
#include <TFile.h>
#include <TVector3.h>
#include <TLorentzVector.h>

#include "TPCReco/CommonDefinitions.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/EventInfo.h"
#include "TPCReco/Track3D.h"
#include "TPCReco/IonRangeCalculator.h"
#include "TPCReco/Comp_analysis.h"

#define MISSING_PID_REPLACEMENT_ENABLE true // TODO - to be parameterized
#define MISSING_PID_1PRONG             CARBON_12 // TODO - to be parameterized
#define MISSING_PID_2PRONG_LEADING     ALPHA // TODO - to be parameterized
#define MISSING_PID_2PRONG_TRAILING    CARBON_12 // TODO - to be parameterized
#define MISSING_PID_3PRONG_LEADING     ALPHA // TODO - to be parameterized
#define MISSING_PID_3PRONG_MIDDLE      ALPHA // TODO - to be parameterized
#define MISSING_PID_3PRONG_TRAILING    ALPHA // TODO - to be parameterized

///////////////////////////////
///////////////////////////////
Comp_analysis::Comp_analysis(std::shared_ptr<GeometryTPC> aGeometryPtr,
			     double pressure,    // CO2 pressure [mbar]
			     double temperature){// CO2 temperature [K]
  setGeometry(aGeometryPtr);
  setIonRangeCalculator(pressure, temperature);
  bookHistos();
  
}
///////////////////////////////
///////////////////////////////
Comp_analysis::~Comp_analysis(){

  finalize();
  delete outputFile;
}
//////////////////////////
//////////////////////////
void Comp_analysis::setIonRangeCalculator(double pressure, double temperature){ // CO2 pressure [mbar] and temperature [K]

  // set current conditions: gas=CO2, arbitrary temperature [K] and pressure [mbar]
  myRangeCalculator.setGasConditions(CO2, fabs(pressure), fabs(temperature));
}
///////////////////////////////
///////////////////////////////
void Comp_analysis::bookHistos(){

  std::string outputFileName = "ComparisonHistos.root";
  outputFile = new TFile(outputFileName.c_str(),"RECREATE");

  const auto maxNtracks=3;
  const auto maxLen_alpha=110.0; // mm
  const auto maxLen_carbon=25.0; // mm
  const auto maxLen_diff=20.0; // mm
  const auto maxMomentum_diff=20.0; // MeV/c
  const auto maxEnergy_diff=20.0; // MeV
  const auto maxLoss=10.0; // unitless
  const auto binLen=0.5; // mm
  const auto binCosTheta=2.0/50; // unitless
  const auto binPhi=TMath::TwoPi()/50; // rad
  const auto binMomentum=maxMomentum_diff/50; // MeV/c
  const auto binEnergy=maxEnergy_diff/50; // MeV
  const auto binLoss=maxLoss/50; // unitless
  float xmin, xmax, ymin, ymax, zmin, zmax; // [mm]
  std::tie(xmin, xmax, ymin, ymax) = myGeometryPtr->rangeXY();
  std::tie(zmin, zmax) = myGeometryPtr->rangeZ();

  // GLOBAL HISTOGRAMS
  //
  // NTRACKS : ALL event categories
  // mis-identification probability
  histos1D["h_nTracks_diff"]=
    new TH1F("h_nTracks_diff","Difference TEST-REF of number of tracks;#Delta#(){Tracks per event};Event count",
	     maxNtracks*2+1, -maxNtracks-0.5, maxNtracks+0.5);
  auto h2=(histos2D["h_TESTnTracks_vs_REFnTracks"]=
	   new TH2F("h_TESTnTracks_vs_REFnTracks","Number of tracks TEST vs REF;TEST tracks per event;REF tracks per event;Event count",
		    maxNtracks+1, -0.5, maxNtracks+0.5, maxNtracks+1, -0.5, maxNtracks+0.5));
  h2->SetOption("BOX TEXT");

  // CHI2 : All event categories
  histos1D["h_all_loss_diff"]=
    new TH1F("h_all_loss_diff","Difference TEST-REF of total #chi^{2} fit;#Delta#(){Total #chi^{2}};Event count",
	     (2*maxLoss+binLoss)/binLoss, -maxLoss-binLoss/2, maxLoss+binLoss/2);
  histos1D["h_all_TESTloss"]=
    new TH1F("h_all_TESTloss","Distribution of total #chi^{2} of TEST fit;Total #chi^{2} of TEST fit;Event count",
	     maxLoss/binLoss, 0.0, maxLoss);
  histos1D["h_TEST1prong_TESTloss"]=
    new TH1F("h_TEST1prong_TESTloss","TEST 1-prong: Total #chi^{2} of TEST fit;Total #chi^{2} of TEST fit;TEST 1-prong event count",
	     maxLoss/binLoss, 0.0, maxLoss);
  histos1D["h_TEST2prong_TESTloss"]=
    new TH1F("h_TEST2prong_TESTloss","TEST 2-prong: Total #chi^{2} of TEST fit;Total #chi^{2} of TEST fit;TEST 2-prong event count",
	     maxLoss/binLoss, 0.0, maxLoss);

  // VERTEX : All event categories
  histos1D["h_all_vertexX_diff"]=
    new TH1F("h_all_vertexX_diff","Difference TEST-REF of vertex X_{DET};#DeltaX_{DET} of vertex [mm];Event count",
	     (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2);
  histos1D["h_all_vertexY_diff"]=
    new TH1F("h_all_vertexY_diff","Difference TEST-REF of vertex Y_{DET};#DeltaY_{DET} of vertex [mm];Event count",
	     (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2);
  histos1D["h_all_vertexZ_diff"]=
    new TH1F("h_all_vertexZ_diff","Difference TEST-REF of vertex Z_{DET};#DeltaZ_{DET} of vertex [mm];Event count",
	     (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2);
  h2=(histos2D["h_all_nTracks_diff_vs_REFvertexX"]=
      new TH2F("h_all_nTracks_diff_vs_REFvertexX","Difference TEST-REF of number of tracks vs REF vertex X_{DET};#Delta#(){Tracks per event};REF vertex X_{DET} [mm];Event count",
	       2*maxNtracks+1, -maxNtracks-0.5, maxNtracks+0.5,
	       ((xmax-xmin)+binLen)/binLen, xmin-binLen/2, xmax+binLen/2));
  h2->SetOption("BOX");
  h2=(histos2D["h_all_nTracks_diff_vs_REFvertexY"]=
      new TH2F("h_all_nTracks_diff_vs_REFvertexY","Difference TEST-REF of number of tracks vs REF vertex Y_{DET};#Delta#(){Tracks per event};REF vertex Y_{DET} [mm];Event count",
	       2*maxNtracks+1, -maxNtracks-0.5, maxNtracks+0.5,
	       ((ymax-ymin)+binLen)/binLen, ymin-binLen/2, ymax+binLen/2));
  h2->SetOption("BOX");
  h2=(histos2D["h_all_nTracks_diff_vs_REFvertexZ"]=
      new TH2F("h_all_nTracks_diff_vs_REFvertexZ","Difference TEST-REF of number of tracks vs REF vertex Z_{DET};#Delta#(){Tracks per event};REF vertex Z_{DET} [mm];Event count",
	       2*maxNtracks+1, -maxNtracks-0.5, maxNtracks+0.5,
	       ((zmax-zmin)+binLen)/binLen, zmin-binLen/2, zmax+binLen/2));
  h2->SetOption("BOX");
  h2=(histos2D["h_all_nTracks_diff_vs_REFleadingPhiDET"]=
      new TH2F("h_all_nTracks_diff_vs_REFleadingPhiDET","Difference TEST-REF of number of tracks vs REF leading track #it{#varphi}_{DET};#Delta#(){Tracks per event};REF leading track #it{#varphi}_{DET} [rad];Event count",
	       2*maxNtracks+1, -maxNtracks-0.5, maxNtracks+0.5,
	       TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi()));
  h2->SetOption("COLZ");
  h2=(histos2D["h_all_nTracks_diff_vs_REFleadingCosThetaDET"]=
      new TH2F("h_all_nTracks_diff_vs_REFleadingCosThetaDET","Difference TEST-REF of number of tracks vs REF leading track cos#it{#theta}_{DET};#Delta#(){Tracks per event};REF leading track cos#it{#theta}_{DET};Event count",
	       2*maxNtracks+1, -maxNtracks-0.5, maxNtracks+0.5,
	       2.0/binCosTheta, -1.0, 1.0));
  h2->SetOption("COLZ");

  // ANGLES : All event categories
  histos1D["h_all_REFleadingPhiDET"]=
    new TH1F("h_all_REFleadingPhiDET",
	     "Distribution of REF leading track #it{#varphi}_{DET};REF leading track #it{#varphi}_{DET} [rad];Event count",
	     TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi());
  histos1D["h_all_REFleadingCosThetaDET"]=
    new TH1F("h_all_REFleadingCosThetaDET",
	     "Distribution of REF leading track cos#it{#theta}_{DET};REF leading track cos#it{#theta}_{DET};Event count",
	     2.0/binCosTheta, -1.0, 1.0);
  h2=(histos2D["h_all_REFleadingPhiDET_vs_REFleadingCosThetaDET"]=
      new TH2F("h_all_REFleadingPhiDET_vs_REFleadingCosThetaDET",
		     "Distribution of REF leading track #it{#varphi}_{DET} x cos#it{#theta}_{DET};REF leading track #it{#varphi}_{DET} [rad];REF leading track cos#it{#theta}_{DET};Event count",
	       TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi(),
	       2.0/binCosTheta, -1.0, 1.0));
  h2->SetOption("COLZ");
  h2=(histos2D["h_TEST1prong_REFleadingPhiDET_vs_REFleadingCosThetaDET"]=
      new TH2F("h_TEST1prong_REFleadingPhiDET_vs_REFleadingCosThetaDET",
		     "TEST 1-prong: Distribution of REF leading track #it{#varphi}_{DET} x cos#it{#theta}_{DET};REF leading track #it{#varphi}_{DET} [rad];REF leading track cos#it{#theta}_{DET};TEST 1-prong event count",
	       TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi(),
	       2.0/binCosTheta, -1.0, 1.0));
  h2->SetOption("COLZ");
  histos1D["h_TEST1prong_REFleadingPhiDET"]=
    new TH1F("h_TEST1prong_REFleadingPhiDET",
	     "TEST 1-prong: Distribution of REF leading track #it{#varphi}_{DET};REF leading track #it{#varphi}_{DET} [rad];TEST 1-prong event count",
	     TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi());
  histos1D["h_TEST1prong_REFleadingCosThetaDET"]=
    new TH1F("h_TEST1prong_REFleadingCosThetaDET",
	     "TEST 1-prong: Distribution of REF leading track cos#it{#theta}_{DET};REF leading track cos#it{#theta}_{DET};TEST 1-prong event count",
	     2.0/binCosTheta, -1.0, 1.0);
  h2=(histos2D["h_TEST2prong_REFleadingPhiDET_vs_REFleadingCosThetaDET"]=
      new TH2F("h_TEST2prong_REFleadingPhiDET_vs_REFleadingCosThetaDET",
		     "TEST 2-prong: Distribution of REF leading track #it{#varphi}_{DET} x cos#it{#theta}_{DET};REF leading track #it{#varphi}_{DET} [rad];REF leading track cos#it{#theta}_{DET};TEST 2-prong event count",
	       TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi(),
	       2.0/binCosTheta, -1.0, 1.0));
  h2->SetOption("COLZ");
  histos1D["h_TEST2prong_REFleadingPhiDET"]=
    new TH1F("h_TEST2prong_REFleadingPhiDET",
	     "TEST 2-prong: Distribution of REF leading track #it{#varphi}_{DET};REF leading track #it{#varphi}_{DET} [rad];TEST 2-prong event count",
	     TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi());
  histos1D["h_TEST2prong_REFleadingCosThetaDET"]=
    new TH1F("h_TEST2prong_REFleadingCosThetaDET",
	     "TEST 2-prong: Distribution of REF leading track cos#it{#theta}_{DET};REF leading track cos#it{#theta}_{DET};TEST 2-prong event count",
	     2.0/binCosTheta, -1.0, 1.0);
  h2=(histos2D["h_all_REFleadingPhiDET_vs_TESTloss"]=
      new TH2F("h_all_REFleadingPhiDET_vs_TESTloss",
	       "Total #chi^{2} of TEST fit vs REF leading track #it{#varphi}_{DET};REF leading track #it{#varphi}_{DET} [rad];Total #chi^{2} of TEST fit;Event count",
	       TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi(),
	       (2*maxLoss+binLoss)/binLoss, -maxLoss-binLoss/2, maxLoss+binLoss/2));
  h2->SetOption("COLZ");
  h2=(histos2D["h_all_REFleadingCosThetaDET_vs_TESTloss"]=
      new TH2F("h_all_REFleadingCosThetaDET_vs_TESTloss",
	       "Total #chi^{2} of TEST fit vs REF leading track cos#it{#theta}_{DET};REF leading track cos#it{#theta}_{DET} [rad];Total #chi^{2} of TEST fit;Event count",
	       2.0/binCosTheta, -1.0, 1.0,
	       (2*maxLoss+binLoss)/binLoss, -maxLoss-binLoss/2, maxLoss+binLoss/2));
  h2->SetOption("COLZ");
  auto p2=(profiles2D["h_all_REFleadingPhiDET_vs_REFleadingCosThetaDET_vs_nTracks_diff_prof"]=
      new TProfile2D("h_all_REFleadingPhiDET_vs_REFleadingCosThetaDET_vs_nTracks_diff_prof",
		     "Difference TEST-REF of number of tracks vs REF leading track #it{#varphi}_{DET} x cos#it{#theta}_{DET};REF leading track #it{#varphi}_{DET} [rad];REF leading track cos#it{#theta}_{DET};#bar{#Delta#(){Tracks per event}}",
		     TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi(),
		     2.0/binCosTheta, -1.0, 1.0,
		     -maxNtracks-0.5, maxNtracks+0.5));
  p2->SetOption("COLZ");
  p2=(profiles2D["h_all_REFleadingPhiDET_vs_REFleadingCosThetaDET_vs_leadingLen_diff_prof"]=
      new TProfile2D("h_all_REFleadingPhiDET_vs_REFleadingCosThetaDET_vs_leadingLen_diff_prof",
		     "Difference TEST-REF of leading track length vs REF leading track #it{#varphi}_{DET} x cos#it{#theta}_{DET};REF leading track #it{#varphi}_{DET} [rad];REF leading track cos#it{#theta}_{DET};#bar{#Delta#(){leading track length}} [mm]",
		     TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi(),
		     2.0/binCosTheta, -1.0, 1.0,
		     -maxLen_diff-binLen/2, maxLen_diff+binLen/2));
  p2->SetOption("COLZ");
  p2=(profiles2D["h_all_REFleadingPhiDET_vs_REFleadingCosThetaDET_vs_leadingAngle_diff_prof"]=
      new TProfile2D("h_all_REFleadingPhiDET_vs_REFleadingCosThetaDET_vs_leadingAngle_diff_prof",
		     "Angular difference TEST-REF of leading track vs REF leading track #it{#varphi}_{DET} x cos#it{#theta}_{DET};REF leading track #it{#varphi}_{DET} [rad];REF leading track cos#it{#theta}_{DET};#bar{Residual angle between leading tracks} [rad]",
		     TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi(),
		     2.0/binCosTheta, -1.0, 1.0,
		     -binPhi/2, TMath::Pi()+binPhi/2));
  p2->SetOption("COLZ");

  // both classified as 1-prong
  histos1D["h_1prong_TESTloss"]=
    new TH1F("h_1prong_TESTloss","1-prong: Total #chi^{2} of TEST fit;Total #chi^{2} of TEST fit;Event count",
	     maxLoss/binLoss, 0.0, maxLoss);
  histos1D["h_1prong_loss_diff"]=
    new TH1F("h_1prong_loss_diff","1-prong: Difference TEST-REF of total #chi^{2} fit;#Delta#(){Total #chi^{2}};Event count",
	     (2*maxLoss+binLoss)/binLoss, -maxLoss-binLoss/2, maxLoss+binLoss/2);
  histos1D["h_1prong_total_PxBEAM_LAB_diff"]=
    new TH1F("h_1prong_total_PxBEAM_LAB_diff","1-prong: Difference TEST-REF of total momentum X_{BEAM} in LAB;#Delta#(){Total momentum X_{BEAM} in LAB} [MeV/c];Event count",
	     (2*maxMomentum_diff+binMomentum)/binMomentum, -maxMomentum_diff-binMomentum/2, maxMomentum_diff+binMomentum/2);
  histos1D["h_1prong_total_PyBEAM_LAB_diff"]=
    new TH1F("h_1prong_total_PyBEAM_LAB_diff","1-prong: Difference TEST-REF of total momentum Y_{BEAM} in LAB;#Delta#(){Total momentum Y_{BEAM} in LAB} [MeV/c];Event count",
	     (2*maxMomentum_diff+binMomentum)/binMomentum, -maxMomentum_diff-binMomentum/2, maxMomentum_diff+binMomentum/2);
  histos1D["h_1prong_total_PzBEAM_LAB_diff"]=
    new TH1F("h_1prong_total_PzBEAM_LAB_diff","1-prong: Difference TEST-REF of total momentum Z_{BEAM} in LAB;#Delta#(){Total momentum Z_{BEAM} in LAB} [MeV/c];Event count",
	     (2*maxMomentum_diff+binMomentum)/binMomentum, -maxMomentum_diff-binMomentum/2, maxMomentum_diff+binMomentum/2);
  histos1D["h_1prong_total_PxDET_LAB_diff"]=
    new TH1F("h_1prong_total_PxDET_LAB_diff","1-prong: Difference TEST-REF of total momentum X_{DET} in LAB;#Delta#(){Total momentum X_{DET} in LAB} [MeV/c];Event count",
	     (2*maxMomentum_diff+binMomentum)/binMomentum, -maxMomentum_diff-binMomentum/2, maxMomentum_diff+binMomentum/2);
  histos1D["h_1prong_total_PyDET_LAB_diff"]=
    new TH1F("h_1prong_total_PyDET_LAB_diff","1-prong: Difference TEST-REF of total momentum Y_{DET} in LAB;#Delta#(){Total momentum Y_{DET} in LAB} [MeV/c];Event count",
	     (2*maxMomentum_diff+binMomentum)/binMomentum, -maxMomentum_diff-binMomentum/2, maxMomentum_diff+binMomentum/2);
  histos1D["h_1prong_total_PzDET_LAB_diff"]=
    new TH1F("h_1prong_total_PzDET_LAB_diff","1-prong: Difference TEST-REF of total momentum Z_{DET} in LAB;#Delta#(){Total momentum Z_{DET} in LAB} [MeV/c];Event count",
	     (2*maxMomentum_diff+binMomentum)/binMomentum, -maxMomentum_diff-binMomentum/2, maxMomentum_diff+binMomentum/2);
  histos1D["h_1prong_alphaLen_diff"]=
    new TH1F("h_1prong_alphaLen_diff","1-prong: Difference TEST-REF of #alpha length;#Delta#(){#alpha length} [mm];Event count",
	     (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2);
  histos1D["h_1prong_alphaAngle_diff"]=
    new TH1F("h_1prong_alphaAngle_diff","1-prong: Angular difference TEST-REF of #alpha track;Residual angle between #alpha tracks [rad];Event count",
	     TMath::Pi()/binPhi, 0.0, TMath::Pi());
  h2=(histos2D["h_1prong_alphaLen_diff_vs_REFalphaLen"]=
      new TH2F("h_1prong_alphaLen_diff_vs_REFalphaLen","1-prong: Difference TEST-REF of #alpha length vs REF #alpha length;#Delta#(){#alpha length} [mm];REF #alpha length [mm];Event count",
	       (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2,
	       (maxLen_alpha+binLen)/binLen, -binLen/2, maxLen_alpha+binLen/2));
  h2->SetOption("BOX");
  histos1D["h_1prong_carbonLen_diff"]=
    new TH1F("h_1prong_carbonLen_diff","1-prong: Difference TEST-REF of ^{12}C length;#Delta#(){^{12}C length} [mm];Event count",
	     (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2);
  histos1D["h_1prong_carbonAngle_diff"]=
    new TH1F("h_1prong_carbonAngle_diff","1-prong: Angular difference TEST-REF of ^{12}C track;Residual angle between ^{12}C tracks [rad];Event count",
	     TMath::Pi()/binPhi, 0.0, TMath::Pi());
  h2=(histos2D["h_1prong_carbonLen_diff_vs_REFcarbonLen"]=
      new TH2F("h_1prong_carbonLen_diff_vs_REFcarbonLen","1-prong: Difference TEST-REF of ^{12}C length vs REF ^{12}C length;#Delta#(){^{12}C length} [mm];REF ^{12}C length [mm];Event count",
	       (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2,
	       (maxLen_carbon+binLen)/binLen, -binLen/2, maxLen_carbon+binLen/2));
  h2->SetOption("BOX");
  h2=(histos2D["h_1prong_TESTalphaLen_vs_REFalphaLen"]=
      new TH2F("h_1prong_TESTalphaLen_vs_REFalphaLen","1-prong: TEST vs REF of #alpha length;TEST #alpha length [mm];REF #alpha length [mm];Event count",
	       (2*maxLen_alpha+binLen)/binLen, -maxLen_alpha-binLen/2, maxLen_alpha+binLen/2,
	       (2*maxLen_alpha+binLen)/binLen, -maxLen_alpha-binLen/2, maxLen_alpha+binLen/2));
  h2->SetOption("BOX");
  h2=(histos2D["h_1prong_TESTcarbonLen_vs_REFcarbonLen"]=
      new TH2F("h_1prong_TESTcarbonLen_vs_REFcarbonLen","1-prong: TEST vs REF of ^{12}C length;TEST ^{12}C length [mm];REF ^{12}C length [mm];Event count",
	       (maxLen_carbon+binLen)/binLen, -binLen/2, maxLen_carbon+binLen/2,
	       (maxLen_carbon+binLen)/binLen, -binLen/2, maxLen_carbon+binLen/2));
  h2->SetOption("BOX");
  profiles1D["h_1prong_TESTalphaLen_vs_REFalphaLen_prof"]=
    new TProfile("h_1prong_TESTalphaLen_vs_REFalphaLen_prof","1-prong: TEST vs REF of #alpha length;TEST #alpha length [mm];REF #alpha length [mm];Event count",
		 (2*maxLen_alpha+binLen)/binLen, -maxLen_alpha-binLen/2, maxLen_alpha+binLen/2,
		 -maxLen_alpha-binLen/2, maxLen_alpha+binLen/2);
  profiles1D["h_1prong_TESTcarbonLen_vs_REFcarbonLen_prof"]=
    new TProfile("h_1prong_TESTcarbonLen_vs_REFcarbonLen_prof","1-prong: TEST vs REF of ^{12}C length;TEST ^{12}C length [mm];REF ^{12}C length [mm];Event count",
		 (maxLen_carbon+binLen)/binLen, -binLen/2, maxLen_carbon+binLen/2,
		 -binLen/2, maxLen_carbon+binLen/2);
  p2=(profiles2D["h_1prong_REFalphaPhiDET_vs_REFalphaCosThetaDET_vs_TESTloss_prof"]=
	   new TProfile2D("h_1prong_REFalphaPhiDET_vs_REFalphaCosThetaDET_vs_TESTloss_prof","1-prong: Total #chi^{2} of TEST fit vs REF #it{#varphi}_{DET} x cos(#theta_{DET};#it{#varphi}_{DET} of REF #alpha track [rad];cos#it{#theta}_{DET} of REF #alpha track;Total #chi^{2} of TEST fit",
			  TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi(),
			  2.0/binCosTheta, -1.0, 1.0,
			  0.0, 100.0));
  p2->SetOption("COLZ");
  p2=(profiles2D["h_1prong_REFcarbonPhiDET_vs_REFcarbonCosThetaDET_vs_TESTloss_prof"]=
      new TProfile2D("h_1prong_REFcarbonPhiDET_vs_REFcarbonCosThetaDET_vs_TESTloss_prof","1-prong: Total #chi^{2} of TEST fit vs REF #it{#varphi}_{DET} x cos#it{#theta}_{DET};#it{#varphi}_{DET} of REF ^{12}C track [rad];cos#it{#theta}_{DET} of REF ^{12}C track;Total #chi^{2} of TEST fit",
		     TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi(),
		     2.0/binCosTheta, -1.0, 1.0,
		     0.0, 100.0));
  p2->SetOption("COLZ");
  
  // both classified as 2-prong
  histos1D["h_2prong_TESTloss"]=
    new TH1F("h_2prong_TESTloss","2-prong: Total #chi^{2} of TEST fit;Total #chi^{2} of TEST fit;Event count",
	     maxLoss/binLoss, 0.0, maxLoss);
  histos1D["h_2prong_loss_diff"]=
    new TH1F("h_2prong_loss_diff","2-prong: Difference TEST-REF of total #chi^{2} fit;#Delta#(){Total #chi^{2}};Event count",
	     (2*maxLoss+binLoss)/binLoss, -maxLoss-binLoss/2, maxLoss+binLoss/2);
  histos1D["h_2prong_Egamma_LAB_diff"]=
    new TH1F("h_2prong_Egamma_LAB_diff","2-prong: Difference TEST-REF of measured E_{#gamma} in LAB;#Delta#(){E_{#gamma} in LAB} [MeV];Event count",
	     (2*maxEnergy_diff+binEnergy)/binEnergy, -maxEnergy_diff-binEnergy/2, maxEnergy_diff+binEnergy/2);
  histos1D["h_2prong_total_PxBEAM_LAB_diff"]=
    new TH1F("h_2prong_total_PxBEAM_LAB_diff","2-prong: Difference TEST-REF of total momentum X_{BEAM} in LAB;#Delta#(){Total momentum X_{BEAM} in LAB} [MeV/c];Event count",
	     (2*maxMomentum_diff+binMomentum)/binMomentum, -maxMomentum_diff-binMomentum/2, maxMomentum_diff+binMomentum/2);
  histos1D["h_2prong_total_PyBEAM_LAB_diff"]=
    new TH1F("h_2prong_total_PyBEAM_LAB_diff","2-prong: Difference TEST-REF of total momentum Y_{BEAM} in LAB;#Delta#(){Total momentum Y_{BEAM} in LAB} [MeV/c];Event count",
	     (2*maxMomentum_diff+binMomentum)/binMomentum, -maxMomentum_diff-binMomentum/2, maxMomentum_diff+binMomentum/2);
  histos1D["h_2prong_total_PzBEAM_LAB_diff"]=
    new TH1F("h_2prong_total_PzBEAM_LAB_diff","2-prong: Difference TEST-REF of total momentum Z_{BEAM} in LAB;#Delta#(){Total momentum Z_{BEAM} in LAB} [MeV/c];Event count",
	     (2*maxMomentum_diff+binMomentum)/binMomentum, -maxMomentum_diff-binMomentum/2, maxMomentum_diff+binMomentum/2);
  histos1D["h_2prong_total_PxDET_LAB_diff"]=
    new TH1F("h_2prong_total_PxDET_LAB_diff","2-prong: Difference TEST-REF of total momentum X_{DET} in LAB;#Delta#(){Total momentum X_{DET} in LAB} [MeV/c];Event count",
	     (2*maxMomentum_diff+binMomentum)/binMomentum, -maxMomentum_diff-binMomentum/2, maxMomentum_diff+binMomentum/2);
  histos1D["h_2prong_total_PyDET_LAB_diff"]=
    new TH1F("h_2prong_total_PyDET_LAB_diff","2-prong: Difference TEST-REF of total momentum Y_{DET} in LAB;#Delta#(){Total momentum Y_{DET} in LAB} [MeV/c];Event count",
	     (2*maxMomentum_diff+binMomentum)/binMomentum, -maxMomentum_diff-binMomentum/2, maxMomentum_diff+binMomentum/2);
  histos1D["h_2prong_total_PzDET_LAB_diff"]=
    new TH1F("h_2prong_total_PzDET_LAB_diff","2-prong: Difference TEST-REF of total momentum Z_{DET} in LAB;#Delta#(){Total momentum Z_{DET} in LAB} [MeV/c];Event count",
	     (2*maxMomentum_diff+binMomentum)/binMomentum, -maxMomentum_diff-binMomentum/2, maxMomentum_diff+binMomentum/2);
  histos1D["h_2prong_alphaLen_diff"]=
    new TH1F("h_2prong_alphaLen_diff","2-prong: Difference TEST-REF of #alpha length;#Delta#(){#alpha length} [mm];Event count",
	     (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2);
  histos1D["h_2prong_alphaAngle_diff"]=
    new TH1F("h_2prong_alphaAngle_diff","2-prong: Angular difference TEST-REF of #alpha track;Residual angle between #alpha tracks [rad];Event count",
	     TMath::Pi()/binPhi, 0.0, TMath::Pi());
  h2=(histos2D["h_2prong_alphaLen_diff_vs_REFalphaLen"]=
      new TH2F("h_2prong_alphaLen_diff_vs_REFalphaLen","2-prong: Difference TEST-REF of #alpha length vs REF #alpha length;#Delta#(){#alpha length} [mm];REF #alpha length [mm];Event count",
	       (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2,
	       (maxLen_alpha+binLen)/binLen, -binLen/2, maxLen_alpha+binLen/2));
  h2->SetOption("BOX");
  histos1D["h_2prong_carbonLen_diff"]=
    new TH1F("h_2prong_carbonLen_diff","2-prong: Difference TEST-REF of ^{12}C length;#Delta#(){^{12}C length} [mm];Event count",
	     (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2);
  histos1D["h_2prong_carbonAngle_diff"]=
    new TH1F("h_2prong_carbonAngle_diff","2-prong: Angular difference TEST-REF of ^{12}C track;Residual angle between ^{12}C tracks [rad];Event count",
	     TMath::Pi()/binPhi, 0.0, TMath::Pi());
  h2=(histos2D["h_2prong_carbonLen_diff_vs_REFcarbonLen"]=
      new TH2F("h_2prong_carbonLen_diff_vs_REFcarbonLen","2-prong: Difference TEST-REF of ^{12}C length vs REF ^{12}C length;#Delta#(){^{12}C length} [mm];REF ^{12}C length [mm];Event count",
	       (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2,
	       (maxLen_carbon+binLen)/binLen, -binLen/2, maxLen_carbon+binLen/2));
  h2->SetOption("BOX");
  h2=(histos2D["h_2prong_alphaLen_diff_vs_carbonLen_diff"]=
      new TH2F("h_2prong_alphaLen_diff_vs_carbonLen_diff","2-prong: Difference TEST-REF of #alpha length vs difference TEST-REF of ^{12}C length;#Delta#(){#alpha length} [mm];#Delta#(){^{12}C length} [mm];Event count",
	       (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2,
	       (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2));
  h2->SetOption("COLZ");
  profiles1D["h_2prong_alphaLen_diff_vs_carbonLen_diff_prof"]=
    new TProfile("h_2prong_alphaLen_diff_vs_carbonLen_diff_prof","2-prong: Difference TEST-REF of #alpha length vs difference TEST-REF of ^{12}C length;#Delta#(){#alpha length} [mm];#bar{#Delta#(){^{12}C length}} [mm]",
		 (2*maxLen_diff+binLen)/binLen, -maxLen_diff-binLen/2, maxLen_diff+binLen/2,
		 -maxLen_diff-binLen/2, maxLen_diff+binLen/2);
  h2=(histos2D["h_2prong_TESTalphaLen_vs_REFalphaLen"]=
      new TH2F("h_2prong_TESTalphaLen_vs_REFalphaLen","2-prong: TEST vs REF of #alpha length;TEST #alpha length [mm];REF #alpha length [mm];Event count",
	       (2*maxLen_alpha+binLen)/binLen, -maxLen_alpha-binLen/2, maxLen_alpha+binLen/2,
	       (2*maxLen_alpha+binLen)/binLen, -maxLen_alpha-binLen/2, maxLen_alpha+binLen/2));
  h2->SetOption("BOX");
  h2=(histos2D["h_2prong_TESTcarbonLen_vs_REFcarbonLen"]=
      new TH2F("h_2prong_TESTcarbonLen_vs_REFcarbonLen","2-prong: TEST vs REF of ^{12}C length;TEST ^{12}C length [mm];REF ^{12}C length [mm];Event count",
	       (maxLen_carbon+binLen)/binLen, -binLen/2, maxLen_carbon+binLen/2,
	       (maxLen_carbon+binLen)/binLen, -binLen/2, maxLen_carbon+binLen/2));
  h2->SetOption("BOX");
  profiles1D["h_2prong_TESTalphaLen_vs_REFalphaLen_prof"]=
    new TProfile("h_2prong_TESTalphaLen_vs_REFalphaLen_prof","2-prong: TEST vs REF of #alpha length;TEST #alpha length [mm];REF #alpha length [mm];Event count",
		 (maxLen_alpha+binLen)/binLen, -binLen/2, maxLen_alpha+binLen/2,
		 -binLen/2, maxLen_alpha+binLen/2);
  profiles1D["h_2prong_TESTcarbonLen_vs_REFcarbonLen_prof"]=
    new TProfile("h_2prong_TESTcarbonLen_vs_REFcarbonLen_prof","2-prong: TEST vs REF of ^{12}C length;TEST ^{12}C length [mm];REF ^{12}C length [mm];Event count",
		 (maxLen_carbon+binLen)/binLen, -binLen/2, maxLen_carbon+binLen/2,
		 -binLen/2, maxLen_carbon+binLen/2);
  p2=(profiles2D["h_2prong_REFalphaPhiDET_vs_REFalphaCosThetaDET_vs_TESTloss_prof"]=
      new TProfile2D("h_2prong_REFalphaPhiDET_vs_REFalphaCosThetaDET_vs_TESTloss_prof","2-prong: Total #chi^{2} of TEST fit vs REF #it{#varphi}_{DET} x cos#it{#theta}_{DET};#it{#varphi}_{DET} of REF #alpha track [rad];cos#it{#theta}_{DET} of REF #alpha track;Total #chi^{2} of TEST fit",
		     TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi(),
		     2.0/binCosTheta, -1.0, 1.0,
		     0.0, 100.0));
  p2->SetOption("COLZ");
  histos1D["h_2prong_deltaAlphaCarbon_diff"]=
      new TH1F("h_2prong_deltaAlphaCarbon_diff","2-prong: Difference TEST-RECO of angle #it{#delta}_{#alpha,^{12}C};#Delta#(){#it{#delta}_{#alpha,^{12}C}} [rad];Event count",
	       TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi());
  histos1D["h_2prong_TESTdeltaAlphaCarbon"]=
      new TH1F("h_2prong_TESTdeltaAlphaCarbon","2-prong: Distribution of TEST angle #it{#delta}_{#alpha,^{12}C};TEST #it{#delta}_{#alpha,^{12}C} [rad];Event count",
	       TMath::Pi()/binPhi, 0.0, TMath::Pi());
  h2=(histos2D["h_2prong_REFalphaPhiDET_vs_deltaAlphaCarbon_diff"]=
      new TH2F("h_2prong_REFalphaPhiDET_vs_deltaAlphaCarbon_diff","2-prong: Difference TEST-RECO of angle #it{#delta}_{#alpha,^{12}C} vs REF #alpha track #it{#varphi}_{DET};#it{#varphi}_{DET} of REF #alpha track [rad];#Delta#(){#it{#delta}_{#alpha,^{12}C}} [rad]",
	       TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi(),
	       TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi()));
  h2->SetOption("COLZ");
  h2=(histos2D["h_2prong_REFalphaCosThetaDET_vs_deltaAlphaCarbon_diff"]=
      new TH2F("h_2prong_REFalphaCosThetaDET_vs_deltaAlphaCarbon_diff","2-prong: Difference TEST-RECO of angle #it{#delta}_{#alpha,^{12}C} vs REF #alpha track cos#it{#theta}_{DET};cos#it{#theta}_{DET} of REF #alpha track;#Delta#(){#it{#delta}_{#alpha,^{12}C}} [rad]",
	       2.0/binCosTheta, -1.0, 1.0,
	       TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi()));
  h2->SetOption("COLZ");
  h2=(histos2D["h_2prong_REFalphaPhiDET_vs_TESTdeltaAlphaCarbon"]=
      new TH2F("h_2prong_REFalphaPhiDET_vs_TESTdeltaAlphaCarbon","2-prong: Difference TEST-RECO of angle #it{#delta}_{#alpha,^{12}C} vs REF #alpha track #it{#varphi}_{DET};#it{#varphi}_{DET} of REF #alpha track [rad];TEST #it{#delta}_{#alpha,^{12}C} [rad]",
	       TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi(),
	       TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi()));
  h2->SetOption("COLZ");
  h2=(histos2D["h_2prong_REFalphaCosThetaDET_vs_TESTdeltaAlphaCarbon"]=
      new TH2F("h_2prong_REFalphaCosThetaDET_vs_TESTdeltaAlphaCarbon","2-prong: Difference TEST-RECO of angle #it{#delta}_{#alpha,^{12}C} vs REF #alpha track cos#it{#theta}_{DET};cos#it{#theta}_{DET} of REF #alpha track;TEST #it{#delta}_{#alpha,^{12}C} [rad]",
	       2.0/binCosTheta, -1.0, 1.0,
	       TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi()));
  h2->SetOption("COLZ");
  p2=(profiles2D["h_2prong_REFalphaPhiDET_vs_REFalphaCosThetaDET_vs_deltaAlphaCarbon_diff_prof"]=
      new TProfile2D("h_2prong_REFalphaPhiDET_vs_REFalphaCosThetaDET_vs_deltaAlphaCarbon_diff_prof","2-prong: Difference TEST-RECO of angle #it{#delta}_{#alpha,^{12}C} vs REF #alpha track #it{#varphi}_{DET} x cos#it{#theta}_{DET};#it{#varphi}_{DET} of REF #alpha track [rad];cos#it{#theta}_{DET} of REF #alpha track;#bar{#Delta#(){#it{#delta}_{#alpha,^{12}C}}} [rad]",
		     TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi(),
		     2.0/binCosTheta, -1.0, 1.0,
		     -TMath::Pi(), TMath::Pi()));
  p2->SetOption("COLZ");
  p2=(profiles2D["h_2prong_REFalphaPhiDET_vs_REFalphaCosThetaDET_vs_TESTdeltaAlphaCarbon_prof"]=
      new TProfile2D("h_2prong_REFalphaPhiDET_vs_REFalphaCosThetaDET_vs_TESTdeltaAlphaCarbon_prof","2-prong: TEST angle #it{#delta}_{#alpha,^{12}C} vs REF #alpha track #it{#varphi}_{DET} x cos#it{#theta}_{DET};#it{#varphi}_{DET} of REF #alpha track [rad];cos#it{#theta}_{DET} of REF #alpha track;#bar{#Delta#(){#it{#delta}_{#alpha,^{12}C}}} [rad]",
		     TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi(),
		     2.0/binCosTheta, -1.0, 1.0,
		     0.0, TMath::Pi()));
  p2->SetOption("COLZ");
  p2=(profiles2D["h_2prong_REFcarbonPhiDET_vs_REFcarbonCosThetaDET_vs_TESTloss_prof"]=
      new TProfile2D("h_2prong_REFcarbonPhiDET_vs_REFcarbonCosThetaDET_vs_TESTloss_prof","2-prong: Total #chi^{2} of TEST fit vs REF #it{#varphi}_{DET} x cos#it{#theta}_{DET};#it{#varphi}_{DET} of REF ^{12}C track [rad];cos#it{#theta}_{DET} of REF ^{12}C track;Total #chi^{2} of TEST fit",
		     TMath::TwoPi()/binPhi, -TMath::Pi(), TMath::Pi(),
		     2.0/binCosTheta, -1.0, 1.0,
		     0.0, 100.0));
  p2->SetOption("COLZ");
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

  int nTracksRef = aRefTrack->getSegments().size();
  int nTracksTest = aTestTrack->getSegments().size();

  auto collRef=aRefTrack->getSegments();
  auto collTest=aTestTrack->getSegments();

  // get sorted list of tracks (descending order by track length)
  std::sort(collRef.begin(), collRef.end(),
	    [](const TrackSegment3D& a, const TrackSegment3D& b) {
	      return a.getLength() > b.getLength();
	    });
  std::sort(collTest.begin(), collTest.end(),
	    [](const TrackSegment3D& a, const TrackSegment3D& b) {
	      return a.getLength() > b.getLength();
	    });
#if(MISSING_PID_REPLACEMENT_ENABLE) // TODO - to be parameterized
  // assign missing PID to REF data file by track length
  switch(nTracksRef) {
  case 3:
    if(collRef.front().getPID()==UNKNOWN) collRef.front().setPID(MISSING_PID_3PRONG_LEADING); // TODO - to be parameterized
    if(collRef.at(1).getPID()==UNKNOWN) collRef.front().setPID(MISSING_PID_3PRONG_MIDDLE); // TODO - to be parameterized
    if(collRef.back().getPID()==UNKNOWN) collRef.back().setPID(MISSING_PID_3PRONG_TRAILING); // TODO - to be parameterized
    break;
  case 2:
    if(collRef.front().getPID()==UNKNOWN) collRef.front().setPID(MISSING_PID_2PRONG_LEADING); // TODO - to be parameterized
    if(collRef.back().getPID()==UNKNOWN) collRef.back().setPID(MISSING_PID_2PRONG_TRAILING); // TODO - to be parameterized
    break;
  case 1:
    if(collRef.front().getPID()==UNKNOWN) collRef.front().setPID(MISSING_PID_1PRONG); // TODO - to be parameterized
    break;
  default:;
  };
  // assign missing PID to TEST data file by track length
  switch(nTracksTest) {
  case 3:
    if(collTest.front().getPID()==UNKNOWN) collTest.front().setPID(MISSING_PID_3PRONG_LEADING); // TODO - to be parameterized
    if(collTest.at(1).getPID()==UNKNOWN) collTest.front().setPID(MISSING_PID_3PRONG_MIDDLE); // TODO - to be parameterized
    if(collTest.back().getPID()==UNKNOWN) collTest.back().setPID(MISSING_PID_3PRONG_TRAILING); // TODO - to be parameterized
    break;
  case 2:
    if(collTest.front().getPID()==UNKNOWN) collTest.front().setPID(MISSING_PID_2PRONG_LEADING); // TODO - to be parameterized
    if(collTest.back().getPID()==UNKNOWN) collTest.back().setPID(MISSING_PID_2PRONG_TRAILING); // TODO - to be parameterized
    break;
  case 1:
    if(collTest.front().getPID()==UNKNOWN) collTest.front().setPID(MISSING_PID_1PRONG); // TODO - to be parameterized
    break;
  default:;
  };
#endif

  // DEFICIT OF TRACKS : All event categories
  histos1D["h_nTracks_diff"]->Fill(nTracksTest-nTracksRef);
  histos2D["h_TESTnTracks_vs_REFnTracks"]->Fill(nTracksTest, nTracksRef);

  // global fit quality
  double fit_loss[2]={aRefTrack->getLoss(), aTestTrack->getLoss()}; // REF, TEST

  histos1D["h_all_Loss_diff"]->Fill(fit_loss[1]-fit_loss[0]);
  histos1D["h_all_TESTloss"]->Fill(fit_loss[1]);

  // leading REF track must be present
  if(nTracksRef>0) {
    auto aRefSeg=collRef.front();
    histos1D["h_all_REFleadingPhiDET"]->Fill(aRefSeg.getTangent().Phi());
    histos1D["h_all_REFleadingCosThetaDET"]->Fill(aRefSeg.getTangent().CosTheta());
    histos2D["h_all_REFleadingPhiDET_vs_REFleadingCosThetaDET"]->Fill(aRefSeg.getTangent().Phi(), aRefSeg.getTangent().CosTheta());
    profiles2D["h_all_REFleadingPhiDET_vs_REFleadingCosThetaDET_vs_nTracks_diff_prof"]->Fill(aRefSeg.getTangent().Phi(), aRefSeg.getTangent().CosTheta(), nTracksTest-nTracksRef);
    histos2D["h_all_nTracks_diff_vs_REFvertexX"]->Fill(nTracksTest-nTracksRef, aRefSeg.getStart().X());
    histos2D["h_all_nTracks_diff_vs_REFvertexY"]->Fill(nTracksTest-nTracksRef, aRefSeg.getStart().Y());
    histos2D["h_all_nTracks_diff_vs_REFvertexZ"]->Fill(nTracksTest-nTracksRef, aRefSeg.getStart().Z());
    histos2D["h_all_nTracks_diff_vs_REFleadingPhiDET"]->Fill(nTracksTest-nTracksRef, aRefSeg.getTangent().Phi());
    histos2D["h_all_nTracks_diff_vs_REFleadingCosThetaDET"]->Fill(nTracksTest-nTracksRef, aRefSeg.getTangent().CosTheta());
  }

  // both leading REF and leading TEST tracks must be present
  if(nTracksRef>0 && nTracksTest>0) {
    auto aRefSeg=collRef.front();
    auto aTestSeg=collTest.front();
    histos2D["h_all_REFleadingPhiDET_vs_TESTloss"]->Fill(aRefSeg.getTangent().Phi(), fit_loss[1]);
    histos2D["h_all_REFleadingCosThetaDET_vs_TESTloss"]->Fill(aRefSeg.getTangent().CosTheta(), fit_loss[1]);
    profiles2D["h_all_REFleadingPhiDET_vs_REFleadingCosThetaDET_vs_leadingLen_diff_prof"]->Fill(aRefSeg.getTangent().Phi(), aRefSeg.getTangent().CosTheta(), aTestSeg.getLength()-aRefSeg.getLength());
    profiles2D["h_all_REFleadingPhiDET_vs_REFleadingCosThetaDET_vs_leadingAngle_diff_prof"]->Fill(aRefSeg.getTangent().Phi(), aRefSeg.getTangent().CosTheta(), aTestSeg.getTangent().Angle(aRefSeg.getTangent()));
    histos1D["h_all_vertexX_diff"]->Fill(aTestSeg.getStart().X()-aRefSeg.getStart().X());
    histos1D["h_all_vertexY_diff"]->Fill(aTestSeg.getStart().Y()-aRefSeg.getStart().Y());
    histos1D["h_all_vertexZ_diff"]->Fill(aTestSeg.getStart().Z()-aRefSeg.getStart().Z());
  }

  // leading REF track present and TEST classified as 1-prong
  if(nTracksRef>0 && nTracksTest==1) {
    auto aRefSeg=collRef.front();
    histos1D["h_TEST1prong_REFleadingPhiDET"]->Fill(aRefSeg.getTangent().Phi());
    histos1D["h_TEST1prong_REFleadingCosThetaDET"]->Fill(aRefSeg.getTangent().CosTheta());
    histos2D["h_TEST1prong_REFleadingPhiDET_vs_REFleadingCosThetaDET"]->Fill(aRefSeg.getTangent().Phi(), aRefSeg.getTangent().CosTheta());
    histos1D["h_TEST1prong_TESTloss"]->Fill(fit_loss[1]);
  }

  // leading REF track present and TEST classified as 2-prong
  if(nTracksRef>0 && nTracksTest==2) {
    auto aRefSeg=collRef.front();
    histos1D["h_TEST2prong_REFleadingPhiDET"]->Fill(aRefSeg.getTangent().Phi());
    histos1D["h_TEST2prong_REFleadingCosThetaDET"]->Fill(aRefSeg.getTangent().CosTheta());
    histos2D["h_TEST2prong_REFleadingPhiDET_vs_REFleadingCosThetaDET"]->Fill(aRefSeg.getTangent().Phi(), aRefSeg.getTangent().CosTheta());
    histos1D["h_TEST2prong_TESTloss"]->Fill(fit_loss[1]);
  }

  // both classified as 1-prong
  if(nTracksRef==1 && nTracksTest==nTracksRef) {

    histos1D["h_1prong_loss_diff"]->Fill(fit_loss[1]-fit_loss[0]);
    histos1D["h_1prong_TESTloss"]->Fill(fit_loss[1]);

    for(auto &aRefSeg: collRef) if(aRefSeg.getPID()==ALPHA) {
	auto found=false;
	for(auto &aTestSeg: collTest) if(aTestSeg.getPID()==aRefSeg.getPID()) {
	    // calculate properties for 1-prong category: single ALPHA track
	    const double alphaMass=myRangeCalculator.getIonMassMeV(ALPHA);
	    double alpha_len[2]={ aRefSeg.getLength(), aTestSeg.getLength() }; // REF, TEST
	    double alpha_T_LAB[2]={ myRangeCalculator.getIonEnergyMeV(ALPHA, alpha_len[0]), // REF
				    myRangeCalculator.getIonEnergyMeV(ALPHA, alpha_len[1]) }; // TEST
	    double alpha_p_LAB[2]={ sqrt(alpha_T_LAB[0]*(alpha_T_LAB[0]+2*alphaMass)), // REF
				    sqrt(alpha_T_LAB[1]*(alpha_T_LAB[1]+2*alphaMass)) }; // TEST
	    TLorentzVector alphaP4_DET_LAB[2]={ TLorentzVector(alpha_p_LAB[0]*aRefSeg.getTangent(), alphaMass+alpha_T_LAB[0]), // REF
						TLorentzVector(alpha_p_LAB[1]*aTestSeg.getTangent(), alphaMass+alpha_T_LAB[1]) }; // TEST
	    // calculate angles in LAB reference frame in BEAM coordinate system
	    // TODO
	    // TODO switch to DET->BEAM dedicated converter class!!!
	    // TODO
	    // change DET-->BEAM coordinate transformation for the HIGS experiment
	    // formulas below are valid provided that beam direction is anti-paralell to X_DET (HIGS case):
	    // X_DET -> -Z_BEAM
	    // Y_DET ->  X_BEAM
	    // Z_DET -> -Y_BEAM
	    TLorentzVector alphaP4_BEAM_LAB[2]={ TLorentzVector( alphaP4_DET_LAB[0].Vect().Py(),
								 -alphaP4_DET_LAB[0].Vect().Pz(),
								 -alphaP4_DET_LAB[0].Vect().Px(),
								 alphaMass+alpha_T_LAB[0] ), // REF
						 TLorentzVector( alphaP4_DET_LAB[1].Vect().Py(),
								 -alphaP4_DET_LAB[1].Vect().Pz(),
								 -alphaP4_DET_LAB[1].Vect().Px(),
								 alphaMass+alpha_T_LAB[1]) }; // TEST
	    histos1D["h_1prong_alphaLen_diff"]->Fill(alpha_len[1]-alpha_len[0]);
	    histos1D["h_1prong_alphaAngle_diff"]->Fill(aTestSeg.getTangent().Angle(aRefSeg.getTangent()));
	    histos2D["h_1prong_alphaLen_diff_vs_REFalphaLen"]->Fill(alpha_len[1]-alpha_len[0], alpha_len[0]);
	    histos2D["h_1prong_TESTalphaLen_vs_REFalphaLen"]->Fill(alpha_len[1], alpha_len[0]);
	    profiles1D["h_1prong_TESTalphaLen_vs_REFalphaLen_prof"]->Fill(alpha_len[1], alpha_len[0]);
	    profiles2D["h_1prong_REFalphaPhiDET_vs_REFalphaCosThetaDET_vs_TESTloss_prof"]->Fill(aRefSeg.getTangent().Phi(), aRefSeg.getTangent().CosTheta(), fit_loss[1]);
	    histos1D["h_1prong_total_PxBEAM_LAB_diff"]->Fill(alphaP4_BEAM_LAB[1].Vect().Px()-alphaP4_BEAM_LAB[0].Vect().Px());
	    histos1D["h_1prong_total_PyBEAM_LAB_diff"]->Fill(alphaP4_BEAM_LAB[1].Vect().Py()-alphaP4_BEAM_LAB[0].Vect().Py());
	    histos1D["h_1prong_total_PzBEAM_LAB_diff"]->Fill(alphaP4_BEAM_LAB[1].Vect().Pz()-alphaP4_BEAM_LAB[0].Vect().Pz());
	    histos1D["h_1prong_total_PxDET_LAB_diff"]->Fill(alphaP4_DET_LAB[1].Vect().Px()-alphaP4_DET_LAB[0].Vect().Px());
	    histos1D["h_1prong_total_PyDET_LAB_diff"]->Fill(alphaP4_DET_LAB[1].Vect().Py()-alphaP4_DET_LAB[0].Vect().Py());
	    histos1D["h_1prong_total_PzDET_LAB_diff"]->Fill(alphaP4_DET_LAB[1].Vect().Pz()-alphaP4_DET_LAB[0].Vect().Pz());
	    found=true;
	    break;
	  }
	if(found) break;
      }
    for(auto &aRefSeg: collRef) if(aRefSeg.getPID()==CARBON_12) {
	auto found=false;
	for(auto &aTestSeg: collTest) if(aTestSeg.getPID()==aRefSeg.getPID()) {
	    // calculate properties for 1-prong category: single C-12 track
	    const double carbonMass=myRangeCalculator.getIonMassMeV(CARBON_12);
	    double carbon_len[2]={ aRefSeg.getLength(), aTestSeg.getLength() }; // REF, TEST
	    double carbon_T_LAB[2]={ myRangeCalculator.getIonEnergyMeV(CARBON_12, carbon_len[0]), // REF
				     myRangeCalculator.getIonEnergyMeV(CARBON_12, carbon_len[1]) }; // TEST
	    double carbon_p_LAB[2]={ sqrt(carbon_T_LAB[0]*(carbon_T_LAB[0]+2*carbonMass)), // REF
				     sqrt(carbon_T_LAB[1]*(carbon_T_LAB[1]+2*carbonMass)) }; // TEST
	    TLorentzVector carbonP4_DET_LAB[2]={ TLorentzVector(carbon_p_LAB[0]*aRefSeg.getTangent(), carbonMass+carbon_T_LAB[0]), // REF
						 TLorentzVector(carbon_p_LAB[1]*aTestSeg.getTangent(), carbonMass+carbon_T_LAB[1]) }; // TEST
	    // calculate angles in LAB reference frame in BEAM coordinate system
	    // TODO
	    // TODO switch to DET->BEAM dedicated converter class!!!
	    // TODO
	    // change DET-->BEAM coordinate transformation for the HIGS experiment
	    // formulas below are valid provided that beam direction is anti-paralell to X_DET (HIGS case):
	    // X_DET -> -Z_BEAM
	    // Y_DET ->  X_BEAM
	    // Z_DET -> -Y_BEAM
	    TLorentzVector carbonP4_BEAM_LAB[2]={TLorentzVector( carbonP4_DET_LAB[0].Vect().Py(),
								 -carbonP4_DET_LAB[0].Vect().Pz(),
								 -carbonP4_DET_LAB[0].Vect().Px(),
								 carbonMass+carbon_T_LAB[0] ), // REF
						 TLorentzVector( carbonP4_DET_LAB[1].Vect().Py(),
								 -carbonP4_DET_LAB[1].Vect().Pz(),
								 -carbonP4_DET_LAB[1].Vect().Px(),
								 carbonMass+carbon_T_LAB[1]) }; // TEST
	    histos1D["h_1prong_carbonLen_diff"]->Fill(carbon_len[1]-carbon_len[0]);
	    histos1D["h_1prong_carbonAngle_diff"]->Fill(aTestSeg.getTangent().Angle(aRefSeg.getTangent()));
	    histos2D["h_1prong_carbonLen_diff_vs_REFcarbonLen"]->Fill(carbon_len[1]-carbon_len[0], carbon_len[0]);
	    histos2D["h_1prong_TESTcarbonLen_vs_REFcarbonLen"]->Fill(carbon_len[1], carbon_len[0]);
	    profiles1D["h_1prong_TESTcarbonLen_vs_REFcarbonLen_prof"]->Fill(carbon_len[1], carbon_len[0]);
	    profiles2D["h_1prong_REFcarbonPhiDET_vs_REFcarbonCosThetaDET_vs_TESTloss_prof"]->Fill(aRefSeg.getTangent().Phi(), aRefSeg.getTangent().CosTheta(), fit_loss[1]);
	    histos1D["h_1prong_total_PxBEAM_LAB_diff"]->Fill(carbonP4_BEAM_LAB[1].Vect().Px()-carbonP4_BEAM_LAB[0].Vect().Px());
	    histos1D["h_1prong_total_PyBEAM_LAB_diff"]->Fill(carbonP4_BEAM_LAB[1].Vect().Py()-carbonP4_BEAM_LAB[0].Vect().Py());
	    histos1D["h_1prong_total_PzBEAM_LAB_diff"]->Fill(carbonP4_BEAM_LAB[1].Vect().Pz()-carbonP4_BEAM_LAB[0].Vect().Pz());
	    histos1D["h_1prong_total_PxDET_LAB_diff"]->Fill(carbonP4_DET_LAB[1].Vect().Px()-carbonP4_DET_LAB[0].Vect().Px());
	    histos1D["h_1prong_total_PyDET_LAB_diff"]->Fill(carbonP4_DET_LAB[1].Vect().Py()-carbonP4_DET_LAB[0].Vect().Py());
	    histos1D["h_1prong_total_PzDET_LAB_diff"]->Fill(carbonP4_DET_LAB[1].Vect().Pz()-carbonP4_DET_LAB[0].Vect().Pz());
	    found=true;
	    break;
	  }
	if(found) break;
      }
  } // end of 1-prong category

  // both classified as 2-prong
  if(nTracksRef==2 && nTracksTest==nTracksRef) {

    histos1D["h_2prong_loss_diff"]->Fill(fit_loss[1]-fit_loss[0]);
    histos1D["h_2prong_TESTloss"]->Fill(fit_loss[1]);

    // calculate properties for 2-prong category: O-16 disintegration hypothesis
    const double alphaMass=myRangeCalculator.getIonMassMeV(ALPHA);
    const double carbonMass=myRangeCalculator.getIonMassMeV(CARBON_12);
    const double oxygenMassGroundState=myRangeCalculator.getIonMassMeV(OXYGEN_16);
    bool found_alpha[2]={false, false}; // REF, TEST
    bool found_carbon[2]={false, false}; // REF, TEST
    double alpha_len[2]={0., 0.}; // REF, TEST
    double carbon_len[2]={0., 0.}; // REF, TEST
    double alpha_T_LAB[2]={0., 0.}; // REF, TEST
    double carbon_T_LAB[2]={0., 0.}; // REF, TEST
    double alpha_p_LAB[2]={0., 0.}; // REF, TEST
    double carbon_p_LAB[2]={0., 0.}; // REF, TEST
    double photon_E_LAB[2]={0., 0.}; // REF, TEST
    TLorentzVector alphaP4_DET_LAB[2]={TLorentzVector(0,0,0,0), TLorentzVector(0,0,0,0)}; // REF, TEST
    TLorentzVector carbonP4_DET_LAB[2]={TLorentzVector(0,0,0,0), TLorentzVector(0,0,0,0)}; // REF, TEST
    TLorentzVector sumP4_DET_LAB[2]={TLorentzVector(0,0,0,0), TLorentzVector(0,0,0,0)}; // REF, TEST

    auto index=0U; // REF
    decltype(collRef) aCollArray[2]={collRef, collTest};
    for(auto &aColl: aCollArray) {
      for(auto &aSeg: aColl) {
	if(aSeg.getPID()==ALPHA && !found_alpha[index]) {
	  found_alpha[index] = true;
	  // reconstruct kinetic energy from particle range [mm]
	  alpha_len[index]=aSeg.getLength();
	  alpha_T_LAB[index]=myRangeCalculator.getIonEnergyMeV(ALPHA, alpha_len[index]);
	  // construct TLorentzVector in DET/LAB frame
	  alpha_p_LAB[index]=sqrt(alpha_T_LAB[index]*(alpha_T_LAB[index]+2*alphaMass));
	  alphaP4_DET_LAB[index]=TLorentzVector(alpha_p_LAB[index]*aSeg.getTangent(), alphaMass+alpha_T_LAB[index]);
	  sumP4_DET_LAB[index] += alphaP4_DET_LAB[index];
	}
	if(aSeg.getPID()==CARBON_12 && !found_carbon[index]) {
	  found_carbon[index] = true;
	  // reconstruct kinetic energy from particle range [mm]
	  carbon_len[index]=aSeg.getLength();
	  carbon_T_LAB[index]=myRangeCalculator.getIonEnergyMeV(CARBON_12, carbon_len[index]);
	  // construct TLorentzVector in DET/LAB frame
	  carbon_p_LAB[index]=sqrt(carbon_T_LAB[index]*(carbon_T_LAB[index]+2*carbonMass));
	  carbonP4_DET_LAB[index]=TLorentzVector(carbon_p_LAB[index]*aSeg.getTangent(), carbonMass+carbon_T_LAB[index]);
	  sumP4_DET_LAB[index] += carbonP4_DET_LAB[index];
	}
      }
      // reconstructed gamma beam energy in LAB
      photon_E_LAB[index]=sumP4_DET_LAB[index].E()-oxygenMassGroundState;
      index++;
    }

    // fill TEST-REF difference histograms per ALPHA
    if(found_alpha[0] && found_alpha[1]) {
      histos1D["h_2prong_alphaLen_diff"]->Fill(alpha_len[1]-alpha_len[0]);
      histos1D["h_2prong_alphaAngle_diff"]->Fill(alphaP4_DET_LAB[1].Vect().Angle(alphaP4_DET_LAB[0].Vect()));
      histos2D["h_2prong_alphaLen_diff_vs_REFalphaLen"]->Fill(alpha_len[1]-alpha_len[0], alpha_len[0]);
      histos2D["h_2prong_TESTalphaLen_vs_REFalphaLen"]->Fill(alpha_len[1], alpha_len[0]);
      profiles1D["h_2prong_TESTalphaLen_vs_REFalphaLen_prof"]->Fill(alpha_len[1], alpha_len[0]);
      profiles2D["h_2prong_REFalphaPhiDET_vs_REFalphaCosThetaDET_vs_TESTloss_prof"]->Fill(alphaP4_DET_LAB[0].Vect().Phi(), alphaP4_DET_LAB[0].Vect().CosTheta(), fit_loss[1]);
    }
    // fill TEST-REF difference histograms per CARBON_12
    if(found_carbon[0] && found_carbon[1]) {
      histos1D["h_2prong_carbonLen_diff"]->Fill(carbon_len[1]-carbon_len[0]);
      histos1D["h_2prong_carbonAngle_diff"]->Fill(carbonP4_DET_LAB[1].Vect().Angle(carbonP4_DET_LAB[0].Vect()));
      histos2D["h_2prong_carbonLen_diff_vs_REFcarbonLen"]->Fill(carbon_len[1]-carbon_len[0], carbon_len[0]);
      histos2D["h_2prong_TESTcarbonLen_vs_REFcarbonLen"]->Fill(carbon_len[1], carbon_len[0]);
      profiles1D["h_2prong_TESTcarbonLen_vs_REFcarbonLen_prof"]->Fill(carbon_len[1], carbon_len[0]);
      profiles2D["h_2prong_REFcarbonPhiDET_vs_REFcarbonCosThetaDET_vs_TESTloss_prof"]->Fill(carbonP4_DET_LAB[0].Vect().Phi(), carbonP4_DET_LAB[0].Vect().CosTheta(), fit_loss[1]);
    }
    // fill TEST-REF difference histograms per event
    if(found_alpha[0] && found_alpha[1] && found_carbon[0] && found_carbon[1]) {
      double angle[2]={alphaP4_DET_LAB[0].Vect().Angle(carbonP4_DET_LAB[0].Vect()), // REF
		       alphaP4_DET_LAB[1].Vect().Angle(carbonP4_DET_LAB[1].Vect()) }; // TEST
      histos1D["h_2prong_deltaAlphaCarbon_diff"]->Fill(angle[1]-angle[0]);
      histos1D["h_2prong_TESTdeltaAlphaCarbon"]->Fill(angle[1]);
      histos2D["h_2prong_REFalphaPhiDET_vs_deltaAlphaCarbon_diff"]->Fill(alphaP4_DET_LAB[0].Vect().Phi(), angle[1]-angle[0]);
      histos2D["h_2prong_REFalphaCosThetaDET_vs_deltaAlphaCarbon_diff"]->Fill(alphaP4_DET_LAB[0].Vect().CosTheta(), angle[1]-angle[0]);
      histos2D["h_2prong_REFalphaPhiDET_vs_TESTdeltaAlphaCarbon"]->Fill(alphaP4_DET_LAB[0].Vect().Phi(), angle[1]);
      histos2D["h_2prong_REFalphaCosThetaDET_vs_TESTdeltaAlphaCarbon"]->Fill(alphaP4_DET_LAB[0].Vect().CosTheta(), angle[1]);
      profiles2D["h_2prong_REFalphaPhiDET_vs_REFalphaCosThetaDET_vs_deltaAlphaCarbon_diff_prof"]->Fill(alphaP4_DET_LAB[0].Vect().Phi(), alphaP4_DET_LAB[0].Vect().CosTheta(), angle[1]-angle[0]);
      profiles2D["h_2prong_REFalphaPhiDET_vs_REFalphaCosThetaDET_vs_TESTdeltaAlphaCarbon_prof"]->Fill(alphaP4_DET_LAB[0].Vect().Phi(), alphaP4_DET_LAB[0].Vect().CosTheta(), angle[1]);
      histos2D["h_2prong_alphaLen_diff_vs_carbonLen_diff"]->Fill(alpha_len[1]-alpha_len[0], carbon_len[1]-carbon_len[0]);
      profiles1D["h_2prong_alphaLen_diff_vs_carbonLen_diff_prof"]->Fill(alpha_len[1]-alpha_len[0], carbon_len[1]-carbon_len[0]);
    }
    histos1D["h_2prong_Egamma_LAB_diff"]->Fill(photon_E_LAB[1]-photon_E_LAB[0]);
    histos1D["h_2prong_total_PxDET_LAB_diff"]->Fill(sumP4_DET_LAB[1].Vect().Px()-sumP4_DET_LAB[0].Vect().Px());
    histos1D["h_2prong_total_PyDET_LAB_diff"]->Fill(sumP4_DET_LAB[1].Vect().Py()-sumP4_DET_LAB[0].Vect().Py());
    histos1D["h_2prong_total_PzDET_LAB_diff"]->Fill(sumP4_DET_LAB[1].Vect().Pz()-sumP4_DET_LAB[0].Vect().Pz());
    // calculate angles in LAB reference frame in BEAM coordinate system
    // TODO
    // TODO switch to DET->BEAM dedicated converter class!!!
    // TODO
    // change DET-->BEAM coordinate transformation for the HIGS experiment
    // formulas below are valid provided that beam direction is anti-paralell to X_DET (HIGS case):
    // X_DET -> -Z_BEAM
    // Y_DET ->  X_BEAM
    // Z_DET -> -Y_BEAM
    TLorentzVector sumP4_BEAM_LAB[2]={TLorentzVector( sumP4_DET_LAB[0].Vect().Py(),
						      -sumP4_DET_LAB[0].Vect().Pz(),
						      -sumP4_DET_LAB[0].Vect().Px(),
						      sumP4_DET_LAB[0].E() ), // REF
				      TLorentzVector( sumP4_DET_LAB[1].Vect().Py(),
						     -sumP4_DET_LAB[1].Vect().Pz(),
						      -sumP4_DET_LAB[1].Vect().Px(),
						      sumP4_DET_LAB[1].E() ) }; // TEST
    histos1D["h_2prong_total_PxBEAM_LAB_diff"]->Fill(sumP4_BEAM_LAB[1].Vect().Px()-sumP4_BEAM_LAB[0].Vect().Px());
    histos1D["h_2prong_total_PyBEAM_LAB_diff"]->Fill(sumP4_BEAM_LAB[1].Vect().Py()-sumP4_BEAM_LAB[0].Vect().Py());
    histos1D["h_2prong_total_PzBEAM_LAB_diff"]->Fill(sumP4_BEAM_LAB[1].Vect().Pz()-sumP4_BEAM_LAB[0].Vect().Pz());
  } // end of 2-prong category
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
