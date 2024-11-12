#include <vector>
#include <iostream>
#include <iomanip>

#include <TH1F.h>
#include <TH2F.h>
#include <TProfile.h>
#include <TFile.h>
#include <TVector3.h>
#include <TLorentzVector.h>

#include "TPCReco/GeometryTPC.h"
#include "TPCReco/Track3D.h"
#include "TPCReco/CommonDefinitions.h"
#include "TPCReco/EventInfo.h"
#include "TPCReco/HIGGS_analysis.h"
#include "TPCReco/colorText.h"
#include "TPCReco/Cuts.h"
#include "TPCReco/UtilsMath.h"

using std::chrono::duration;
using std::chrono::duration_cast;

///////////////////////////////
///////////////////////////////
HIGGS_analysis::HIGGS_analysis(std::shared_ptr<GeometryTPC> aGeometryPtr, // definition of LAB detector coordinates
			       float beamEnergy,   // nominal gamma beam energy [MeV] in detector LAB frame
			       TVector3 beamDir,   // nominal gamma beam direction in detector LAB frame
			       IonRangeCalculator ionRangeCalculator,
			       CoordinateConverter coordinateConverter,
			       bool nominalBoostFlag)
  : myRangeCalculator(ionRangeCalculator),
    coordinateConverter(coordinateConverter),
    useNominalPhotonEnergyForBoost(nominalBoostFlag) {
  setGeometry(aGeometryPtr);
  setBeamProperties(beamEnergy, beamDir);
  bookHistos();
}
///////////////////////////////
///////////////////////////////
HIGGS_analysis::~HIGGS_analysis(){

  finalize();
  delete outputFile;
}
///////////////////////////////
///////////////////////////////
void HIGGS_analysis::bookHistos(){

  const std::string categoryPrefix[]={ "h_all", "h_1prong", "h_2prong", "h_3prong" };
  const std::string categoryInfo[]={ "All tracks", "1-prong (#alpha)", "2-prong (#alpha+C)", "3-prong (#alpha_{1}+#alpha_{2}+#alpha_{3})" };

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
  //  const float binSizeMM_2d = 3.0; // [mm]
  const float binSizeMM_2dXZ = binSizeMM;
  const float binSizeMM_2dYZ = binSizeMM;
  const float binSizeMM_2dXY = binSizeMM;
  const float binSizeMM_prof = 3.0; // [mm]
  const float maxLengthMM = 200.0; // [mm]
  const float minTotalEnergyMeV = 11e3; // [MeV] // C-12 = 12u
  const float maxTotalEnergyMeV = 17e3; // [MeV] // O-18 = 18u
  const float maxKineticEnergyMeV = 10.0; // [MeV]
  const float maxDeltaMomentumMeV = 20.0; // [MeV]
  const float minBeamEnergyMeV = photonEnergyInMeV_LAB - 0.5*photonEnergyInMeV_LAB; // min histogram range
  const float maxBeamEnergyMeV = photonEnergyInMeV_LAB + 0.5*photonEnergyInMeV_LAB; // max histogram range
  const float binSizeMeV_kineticEnergy = 0.005; // [MeV] // 5 keV
  const float binSizeMeV_beamEnergy = binSizeMeV_kineticEnergy; // [MeV] // 5 keV

  ///// DEBUG - special histograms with rate evolution
  const float binSizeSec = 30; // 30 sec resolution
  const float binSizeDeltaSec = 0.1; // 0.1 sec resolution
  const float maxRunTimeSec = 1*3600; // 1 h time-span
  const float maxDeltaTimeSec = 2*60; // 2 min time-span
  ///// DEBUG - special histograms with rate evolution

  float xmin, xmax, ymin, ymax, zmin, zmax; // [mm]
  std::tie(xmin, xmax, ymin, ymax) = myGeometryPtr->rangeXY();
  std::tie(zmin, zmax) = myGeometryPtr->rangeZ();

  // GLOBAL HISTOGRAMS
  //
  // NTRACKS : ALL event categories
  histos1D["h_ntracks"]=
    new TH1F("h_ntracks","Number of tracks;Tracks per event;Event count", 5, 0, 5);

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
	       (xmax-xmin)/binSizeMM_2dXY, xmin, xmax, (ymax-ymin)/binSizeMM_2dXY, ymin, ymax);
    histos2D[(prefix+"_vertexYZ").c_str()]=
      new TH2F((prefix+"_vertexYZ").c_str(),
	       Form("%s;Vertex position Y_{DET} [mm];Vertex position Z_{DET} [mm];%s", info, perEventTitle),
	       (ymax-ymin)/binSizeMM_2dYZ, ymin, ymax, (zmax-zmin)/binSizeMM_2dYZ, zmin, zmax);
    profiles1D[(prefix+"_vertexXY_prof").c_str()]=
      new TProfile((prefix+"_vertexXY_prof").c_str(),
		   Form("%s;Vertex position X_{DET} [mm];Average vertex position Y_{DET} [mm];%s", info, perEventTitle),
		   (xmax-xmin)/binSizeMM_prof, xmin, xmax, ymin, ymax);
    histos1D[(prefix+"_vertexXBEAM").c_str()]=
      new TH1F((prefix+"_vertexXBEAM").c_str(),
	       Form("%s;Vertex position X_{BEAM} [mm];%s", info, perEventTitle),
	       (ymax-ymin)/binSizeMM, ymin, ymax); // X_BEAM -> Y_DET
    histos1D[(prefix+"_vertexYBEAM").c_str()]=
      new TH1F((prefix+"_vertexYBEAM").c_str(),
	       Form("%s;Vertex position Y_{BEAM} [mm];%s", info, perEventTitle),
	       (zmax-zmin)/binSizeMM, zmin, zmax); // Y_BEAM -> -Z_DET
    histos1D[(prefix+"_vertexZBEAM").c_str()]=
      new TH1F((prefix+"_vertexZBEAM").c_str(),
	       Form("%s;Vertex position Z_{BEAM} [mm];%s", info, perEventTitle),
	       (xmax-xmin)/binSizeMM, xmin, xmax); // Z_BEAM -> -X_DET
    histos2D[(prefix+"_vertexZXBEAM").c_str()]=
      new TH2F((prefix+"_vertexZXBEAM").c_str(),
	       Form("%s;Vertex position Z_{BEAM} [mm];Vertex position X_{BEAM} [mm];%s", info, perEventTitle),
	       (xmax-xmin)/binSizeMM_2dXY, xmin, xmax, (ymax-ymin)/binSizeMM_2dXY, ymin, ymax); // ZX_BEAM -> XY_DET
    histos2D[(prefix+"_vertexXYBEAM").c_str()]=
      new TH2F((prefix+"_vertexXYBEAM").c_str(),
	       Form("%s;Vertex position X_{BEAM} [mm];Vertex position Y_{BEAM} [mm];%s", info, perEventTitle),
	       (ymax-ymin)/binSizeMM_2dYZ, ymin, ymax, (zmax-zmin)/binSizeMM_2dYZ, zmin, zmax); // XY_BEAM -> YZ_DET
    profiles1D[(prefix+"_vertexZXBEAM_prof").c_str()]=
      new TProfile((prefix+"_vertexZXBEAM_prof").c_str(),
		   Form("%s;Vertex position Z_{BEAM} [mm];Average vertex position X_{BEAM} [mm];%s", info, perEventTitle),
		   (xmax-xmin)/binSizeMM_prof, xmin, xmax, ymin, ymax); // ZX_BEAM -> XY_DET
    
    // TOTAL OBSERVABLE : per category
    switch(categoryPID[c].size()) {
    case 3: // DALITZ PLOTS FOR 3-BODY DECAY OF CARBON IN CMS FRAME : symmetrized track pairs, 3-prong only
      histos2D[(prefix+"_Dalitz1_CMS").c_str()]=
	new TH2F((prefix+"_Dalitz1_CMS").c_str(),
		 Form("%s - Dalitz plot;#it{m_{i,j}} [MeV/c^{2}];#it{m_{i,k}} [MeV/c^{2}];Probability [arb.u.]", info),
		 100, 2*myRangeCalculator.getIonMassMeV(ALPHA),
		 myRangeCalculator.getIonMassMeV(CARBON_12)-myRangeCalculator.getIonMassMeV(ALPHA),
		 100, 2*myRangeCalculator.getIonMassMeV(ALPHA),
		 myRangeCalculator.getIonMassMeV(CARBON_12)-myRangeCalculator.getIonMassMeV(ALPHA) );
      // Special version of Dalitz plot for identical masses, centered at (chi=0, psi=0):
      // abscissa : chi = (eps1+2*eps2-1)/sqrt(3) = (T2-T3)/sqrt(3)/Q
      // ordinate : psi = eps1-1/3 = (2*T1-T2-T3)/3/Q
      // where: eps_i=T_i/Q, Q=T1+T2+T3
      // Reference: K.L.Laursen et al., Eur. Phys. J. A 62 (2016) 271.
      histos2D[(prefix+"_Dalitz2_CMS").c_str()]=
	new TH2F((prefix+"_Dalitz2_CMS").c_str(),
		 Form("%s - Dalitz plot;#chi;#psi;Probability [arb.u.]", info),
		 200, -0.50, 0.50, 200, -0.50, 0.50);
      // PLOTS FOR TRIPLE-ALPHA BREAKUP OF CARBON IN CMS FRAME : three entries per horizontal line
      // Reference: C.Au.Diget at al., Phys. Rev. C 80 (2009) 034316.
      histos2D[(prefix+"_Dalitz3_CMS").c_str()]=
	new TH2F((prefix+"_Dalitz3_CMS").c_str(),
		 Form("%s - Dalitz plot;#alpha kinetic energy in CMS [MeV];^{12}C excitation energy above g.s. in CMS [MeV];Probability [arb.u.]", info),
		 200, 0, maxKineticEnergyMeV,
		 300, 0, maxKineticEnergyMeV*3);
      histos2D[(prefix+"_Dalitz4_CMS").c_str()]=
	new TH2F((prefix+"_Dalitz4_CMS").c_str(),
		 Form("%s - Dalitz plot;#alpha kinetic energy in CMS [MeV];Kinetic energy sum in CMS [MeV];Probability [arb.u.]", info),
		 200, 0, maxKineticEnergyMeV,
		 200, 0, maxKineticEnergyMeV*2);
    case 2: // VALID FOR 2-prongs and 3-prongs
      histos1D[(prefix+"_lenSum").c_str()]=
	new TH1F((prefix+"_lenSum").c_str(),
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
      histos1D[(prefix+"_total_PxBEAM_LAB").c_str()]=
	new TH1F((prefix+"_total_PxBEAM_LAB").c_str(),
		 Form("%s;Total momentum X_{BEAM} in LAB [MeV/c];%s", info, perEventTitle),
		 100, -maxDeltaMomentumMeV, maxDeltaMomentumMeV);
      histos1D[(prefix+"_total_PyBEAM_LAB").c_str()]=
	new TH1F((prefix+"_total_PyBEAM_LAB").c_str(),
		 Form("%s;Total momentum Y_{BEAM} in LAB [MeV/c];%s", info, perEventTitle),
		 100, -maxDeltaMomentumMeV, maxDeltaMomentumMeV);
      histos1D[(prefix+"_total_PzBEAM_LAB").c_str()]=
	new TH1F((prefix+"_total_PzBEAM_LAB").c_str(),
		 Form("%s;Total momentum Z_{BEAM} in LAB [MeV/c];%s", info, perEventTitle),
		 100, -maxDeltaMomentumMeV, maxDeltaMomentumMeV);
      histos1D[(prefix+"_total_E_CMS").c_str()]=
	new TH1F((prefix+"_total_E_CMS").c_str(),
		 Form("%s;Total energy in CMS [MeV];%s", info, perEventTitle),
		 (int)(maxTotalEnergyMeV-minTotalEnergyMeV), minTotalEnergyMeV, maxTotalEnergyMeV);
      histos1D[(prefix+"_Qvalue_CMS").c_str()]=
	new TH1F((prefix+"_Qvalue_CMS").c_str(),
		 Form("%s;Q value in CMS [MeV];%s", info, perEventTitle),
		 maxKineticEnergyMeV/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV); // 200, 0, maxKineticEnergyMeV);
      histos1D[(prefix+"_excitation_E_CMS").c_str()]=
	new TH1F((prefix+"_excitation_E_CMS").c_str(),
		 Form("%s;Excitation energy above g.s. in CMS [MeV];%s", info, perEventTitle),
		 maxKineticEnergyMeV*3/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV*3); // 300, 0, maxKineticEnergyMeV*3);
      histos1D[(prefix+"_gamma_E_LAB").c_str()]=
	new TH1F((prefix+"_gamma_E_LAB").c_str(),
		 Form("%s;Gamma beam energy in LAB [MeV];%s", info, perEventTitle),
		 (maxBeamEnergyMeV-minBeamEnergyMeV)/binSizeMeV_beamEnergy, minBeamEnergyMeV, maxBeamEnergyMeV); // 100, minBeamEnergyMeV, maxBeamEnergyMeV);

      ///////////// DEBUG - valid for 2-prong only
      //
      if(categoryPID[c].size()==2) {
	histos1D[(prefix+"_excitation_E_CMS_fromAlpha").c_str()]=
	  new TH1F((prefix+"_excitation_E_CMS_fromAlpha").c_str(),
		   Form("%s (from #alpha E_{KIN}^{LAB});Excitation energy above g.s. in CMS [MeV];%s", info, perEventTitle),
		   maxKineticEnergyMeV*3/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV*3); // 300, 0, maxKineticEnergyMeV*3);
	histos1D[(prefix+"_gamma_E_LAB_fromAlpha").c_str()]=
	  new TH1F((prefix+"_gamma_E_LAB_fromAlpha").c_str(),
		   Form("%s (from #alpha E_{KIN}^{LAB});Gamma beam energy in LAB [MeV];%s", info, perEventTitle),
		   (maxBeamEnergyMeV-minBeamEnergyMeV)/binSizeMeV_beamEnergy, minBeamEnergyMeV, maxBeamEnergyMeV); // 100, minBeamEnergyMeV, maxBeamEnergyMeV);
	histos1D[(prefix+"_excitation_E_CMS_fromAlpha_CutO16").c_str()]=
	  new TH1F((prefix+"_excitation_E_CMS_fromAlpha_CutO16").c_str(),
		   Form("%s (from #alpha E_{KIN}^{CMS} with #alpha,C E_{KIN}^{CMS} cut);Excitation energy above g.s. in CMS [MeV];%s", info, perEventTitle),
		   maxKineticEnergyMeV*3/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV*3); // 300, 0, maxKineticEnergyMeV*3);
	histos1D[(prefix+"_gamma_E_LAB_fromAlpha_CutO16").c_str()]=
	  new TH1F((prefix+"_gamma_E_LAB_fromAlpha_CutO16").c_str(),
		   Form("%s (from #alpha E_{KIN}^{LAB} with #alpha,C E_{KIN}^{CMS} cut);Gamma beam energy in LAB [MeV];%s", info, perEventTitle),
		   (maxBeamEnergyMeV-minBeamEnergyMeV)/binSizeMeV_beamEnergy, minBeamEnergyMeV, maxBeamEnergyMeV); // 100, minBeamEnergyMeV, maxBeamEnergyMeV);
	histos1D[(prefix+"_runTime").c_str()]=
	  new TH1F((prefix+"_runTime").c_str(),
		   Form("%s;Run time [s];%s", info, perEventTitle),
		   maxRunTimeSec/binSizeSec, 0, maxRunTimeSec);
	histos1D[(prefix+"_runTime_CutO16").c_str()]=
	  new TH1F((prefix+"_runTime_CutO16").c_str(),
		   Form("%s (with #alpha,C E_{KIN}^{CMS} cut);Run time [s];%s", info, perEventTitle),
		   maxRunTimeSec/binSizeSec, 0, maxRunTimeSec);
	histos1D[(prefix+"_deltaTime").c_str()]=
	  new TH1F((prefix+"_deltaTime").c_str(),
		   Form("%s;Time difference [s];%s", info, perEventTitle),
		   maxDeltaTimeSec/binSizeDeltaSec, 0, maxDeltaTimeSec);
	histos1D[(prefix+"_deltaTime_CutO16").c_str()]=
	  new TH1F((prefix+"_deltaTime_CutO16").c_str(),
		   Form("%s (with #alpha,C E_{KIN}^{CMS} cut);Time difference [s];%s", info, perEventTitle),
		   maxDeltaTimeSec/binSizeDeltaSec, 0, maxDeltaTimeSec);
      }
      //
      ///////////// DEBUG - valid for 2-prong only

      histos1D[(prefix+"_E_CMS").c_str()]=
	new TH1F((prefix+"_E_CMS").c_str(),
		 Form("%s;Kinetic energy sum in CMS [MeV];%s", info, perEventTitle),
		 maxKineticEnergyMeV/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV); // 200, 0, maxKineticEnergyMeV);
      histos1D[(prefix+"_E_LAB").c_str()]=
	new TH1F((prefix+"_E_LAB").c_str(),
		 Form("%s;Kinetic energy sum in LAB [MeV];%s", info, perEventTitle),
		 maxKineticEnergyMeV/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV); // 200, 0, maxKineticEnergyMeV);
      // SPECIAL PLOTS: check dependence of gamma beam energy on vertex position (Y_DET) horizontal & perpendicular to the gamma beam axis
      histos2D[(prefix+"_vertexX_lenSum").c_str()]=
	new TH2F((prefix+"_vertexX_lenSum").c_str(),
		 Form("%s;Vertex position X_{DET} [mm];Sum of track lengths [mm];%s", info, perEventTitle),
		 (xmax-xmin)/binSizeMM, xmin, xmax,
		 maxLengthMM/binSizeMM, 0, maxLengthMM);
      histos2D[(prefix+"_vertexY_lenSum").c_str()]=
	new TH2F((prefix+"_vertexY_lenSum").c_str(),
		 Form("%s;Vertex position Y_{DET} [mm];Sum of track lengths [mm];%s", info, perEventTitle),
		 (ymax-ymin)/binSizeMM, ymin, ymax,
		 maxLengthMM/binSizeMM, 0, maxLengthMM);
      histos2D[(prefix+"_vertexZ_lenSum").c_str()]=
	new TH2F((prefix+"_vertexZ_lenSum").c_str(),
		 Form("%s;Vertex position Z_{DET} [mm];Sum of track lengths [mm];%s", info, perEventTitle),
		 (zmax-zmin)/binSizeMM, zmin, zmax,
		 maxLengthMM/binSizeMM, 0, maxLengthMM);
      histos2D[(prefix+"_vertexY_gamma_E_LAB").c_str()]=
	new TH2F((prefix+"_vertexY_gamma_E_LAB").c_str(),
		 Form("%s;Vertex position Y_{DET} [mm];Beam energy in LAB [MeV];%s", info, perEventTitle),
		 (ymax-ymin)/binSizeMM, ymin, ymax,
		 (maxBeamEnergyMeV-minBeamEnergyMeV)/binSizeMeV_beamEnergy, minBeamEnergyMeV, maxBeamEnergyMeV); // 100, minBeamEnergyMeV, maxBeamEnergyMeV);
      histos2D[(prefix+"_vertexY_Qvalue_CMS").c_str()]=
	new TH2F((prefix+"_vertexY_Qvalue_CMS").c_str(),
		 Form("%s;Vertex position Y_{DET} [mm];Q value in CMS [MeV];%s", info, perEventTitle),
		 (ymax-ymin)/binSizeMM, ymin, ymax,
		 maxKineticEnergyMeV/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV); // 200, 0, maxKineticEnergyMeV);
      profiles1D[(prefix+"_vertexX_lenSum_prof").c_str()]=
	new TProfile((prefix+"_vertexX_lenSum_prof").c_str(),
		     Form("%s;Vertex position X_{DET} [mm];Average sum of track lengths [mm]", info),
		     (xmax-xmin)/binSizeMM, xmin, xmax,
		     0, maxLengthMM);
      profiles1D[(prefix+"_vertexY_lenSum_prof").c_str()]=
	new TProfile((prefix+"_vertexY_lenSum_prof").c_str(),
		     Form("%s;Vertex position Y_{DET} [mm];Average sum of track lengths [mm]", info),
		     (ymax-ymin)/binSizeMM, ymin, ymax,
		     0, maxLengthMM);
      profiles1D[(prefix+"_vertexZ_lenSum_prof").c_str()]=
	new TProfile((prefix+"_vertexZ_lenSum_prof").c_str(),
		     Form("%s;Vertex position Z_{DET} [mm];Average sum of track lengths [mm]", info),
		     (zmax-zmin)/binSizeMM, zmin, zmax,
		     0, maxLengthMM);
      profiles1D[(prefix+"_vertexY_gamma_E_LAB_prof").c_str()]=
	new TProfile((prefix+"_vertexY_gamma_E_LAB_prof").c_str(),
		     Form("%s;Vertex position Y_{DET} [mm];Average beam energy in LAB [MeV]", info),
		     (ymax-ymin)/binSizeMM, ymin, ymax,
		     minBeamEnergyMeV, maxBeamEnergyMeV);
      profiles1D[(prefix+"_vertexY_Qvalue_CMS_prof").c_str()]=
	new TProfile((prefix+"_vertexY_Qvalue_CMS_prof").c_str(),
		     Form("%s;Vertex position Y_{DET} [mm];Average Q value in CMS [MeV]", info),
		     (ymax-ymin)/binSizeMM, ymin, ymax,
		     0, maxKineticEnergyMeV);
      // SPECIAL PLOTS: check dependence of gamma beam energy on vertex position (X_BEAM) horizontal & perpendicular to the gamma beam axis
      histos2D[(prefix+"_vertexXBEAM_gamma_E_LAB").c_str()]=
	new TH2F((prefix+"_vertexXBEAM_gamma_E_LAB").c_str(),
		 Form("%s;Vertex position X_{BEAM} [mm];Beam energy in LAB [MeV];%s", info, perEventTitle),
		 (ymax-ymin)/binSizeMM, ymin, ymax, // X_BEAM -> Y_DET
		 (maxBeamEnergyMeV-minBeamEnergyMeV)/binSizeMeV_beamEnergy, minBeamEnergyMeV, maxBeamEnergyMeV); // 100, minBeamEnergyMeV, maxBeamEnergyMeV);
      histos2D[(prefix+"_vertexXBEAM_Qvalue_CMS").c_str()]=
	new TH2F((prefix+"_vertexXBEAM_Qvalue_CMS").c_str(),
		 Form("%s;Vertex position X_{BEAM} [mm];Q value in CMS [MeV];%s", info, perEventTitle),
		 (ymax-ymin)/binSizeMM, ymin, ymax, // X_BEAM -> Y_DET
		 maxKineticEnergyMeV/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV); // 200, 0, maxKineticEnergyMeV);
      profiles1D[(prefix+"_vertexXBEAM_gamma_E_LAB_prof").c_str()]=
	new TProfile((prefix+"_vertexXBEAM_gamma_E_LAB_prof").c_str(),
		     Form("%s;Vertex position X_{BEAM} [mm];Average beam energy in LAB [MeV]", info),
		     (ymax-ymin)/binSizeMM, ymin, ymax, // X_BEAM -> Y_DET
		     minBeamEnergyMeV, maxBeamEnergyMeV);
      profiles1D[(prefix+"_vertexXBEAM_Qvalue_CMS_prof").c_str()]=
	new TProfile((prefix+"_vertexXBEAM_Qvalue_CMS_prof").c_str(),
		     Form("%s;Vertex position X_{BEAM} [mm];Average Q value in CMS [MeV]", info),
		     (ymax-ymin)/binSizeMM, ymin, ymax, // X_BEAM -> Y_DET
		     0, maxKineticEnergyMeV);
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
		 maxLengthMM/binSizeMM_2dXY, -0.5*maxLengthMM, 0.5*maxLengthMM,
		 maxLengthMM/binSizeMM_2dXY, -0.5*maxLengthMM, 0.5*maxLengthMM);
      histos2D[(prefix+pid+"_deltaXZ").c_str()]=
	new TH2F((prefix+pid+"_deltaXZ").c_str(),
		 Form("%s;%s track #DeltaX_{DET} [mm];%s track #DeltaZ_{DET} [mm];%s", info, pidLatex, pidLatex, perTrackTitle),
		 maxLengthMM/binSizeMM_2dXZ, -0.5*maxLengthMM, 0.5*maxLengthMM,
		 maxLengthMM/binSizeMM_2dXZ, -0.5*maxLengthMM, 0.5*maxLengthMM);
      histos2D[(prefix+pid+"_deltaYZ").c_str()]=
	new TH2F((prefix+pid+"_deltaYZ").c_str(),
		 Form("%s;%s track #DeltaY_{DET} [mm];%s track #DeltaZ_{DET} [mm];%s", info, pidLatex, pidLatex, perTrackTitle),
		 maxLengthMM/binSizeMM_2dYZ, -0.5*maxLengthMM, 0.5*maxLengthMM,
		 maxLengthMM/binSizeMM_2dYZ, -0.5*maxLengthMM, 0.5*maxLengthMM);
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
		 (xmax-xmin)/binSizeMM_2dXY, xmin, xmax, (ymax-ymin)/binSizeMM_2dXY, ymin, ymax);

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
		 Form("%s;%s track cos(#theta_{DET});%s", info, pidLatex, perTrackTitle),
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
      
      // TRACK OBSERVABLES IN CMS/LAB : per category / per track, only for 2,3-prong
      switch(categoryPID[c].size()) {
      case 3:
      case 2: // 2,3-prong
	histos1D[(prefix+pid+"_E_CMS").c_str()]=
	  new TH1F((prefix+pid+"_E_CMS").c_str(),
		   Form("%s;%s kinetic energy in CMS [MeV];%s", info, pidLatex, perTrackTitle),
		   maxKineticEnergyMeV/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV); // 200, 0, maxKineticEnergyMeV);
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
		   maxKineticEnergyMeV/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV); // 200, 0, maxKineticEnergyMeV);
	profiles1D[(prefix+pid+"_cosThetaBEAM_E_CMS_prof").c_str()]=
	  new TProfile((prefix+pid+"_cosThetaBEAM_E_CMS_prof").c_str(),
		       Form("%s;%s track cos(#theta_{BEAM}) in CMS;Average %s kinetic energy in CMS [MeV];%s", info, pidLatex, pidLatex, perTrackTitle),
		       100, -1, 1,
		       0, maxKineticEnergyMeV);

	///////////// DEBUG - valid for 2-prong only, after additional ID cuts
	//
	if(categoryPID[c].size()==2) {
	  histos1D[(prefix+pid+"_E_CMS_CutO16").c_str()]=
	    new TH1F((prefix+pid+"_E_CMS_CutO16").c_str(),
		     Form("%s (with #alpha,C E_{KIN}^{CMS} cut);%s kinetic energy in CMS [MeV];%s", info, pidLatex, perTrackTitle),
		     maxKineticEnergyMeV/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV); // 200, 0, maxKineticEnergyMeV);
	  histos1D[(prefix+pid+"_phiBEAM_CMS_CutO16").c_str()]=
	    new TH1F((prefix+pid+"_phiBEAM_CMS_CutO16").c_str(),
		     Form("%s (with #alpha,C E_{KIN}^{CMS} cut);%s track #phi_{BEAM} in CMS [rad];%s", info, pidLatex, perTrackTitle),
		     100, -TMath::Pi(), TMath::Pi());
	  histos1D[(prefix+pid+"_thetaBEAM_CMS_CutO16").c_str()]=
	    new TH1F((prefix+pid+"_thetaBEAM_CMS_CutO16").c_str(),
		     Form("%s (with #alpha,C E_{KIN}^{CMS} cut);%s track #theta_{BEAM} in CMS [rad];%s", info, pidLatex, perTrackTitle),
		     100, 0, TMath::Pi());
	  histos1D[(prefix+pid+"_cosThetaBEAM_CMS_CutO16").c_str()]=
	    new TH1F((prefix+pid+"_cosThetaBEAM_CMS_CutO16").c_str(),
		     Form("%s (with #alpha,C E_{KIN}^{CMS} cut);%s track cos(#theta_{BEAM}) in CMS;%s", info, pidLatex, perTrackTitle),
		     100, -1, 1);
	  histos2D[(prefix+pid+"_cosThetaBEAM_E_CMS_CutO16").c_str()]=
	    new TH2F((prefix+pid+"_cosThetaBEAM_E_CMS_CutO16").c_str(),
		     Form("%s (with #alpha,C E_{KIN}^{CMS} cut);%s track cos(#theta_{BEAM}) in CMS;%s kinetic energy in CMS [MeV];%s", info, pidLatex, pidLatex, perTrackTitle),
		     100, -1, 1,
		     maxKineticEnergyMeV/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV); // 200, 0, maxKineticEnergyMeV);
	  histos2D[(prefix+pid+"_deltaYZ_CutO16").c_str()]=
	    new TH2F((prefix+pid+"_deltaYZ_CutO16").c_str(),
		     Form("%s (with #alpha,C E_{KIN}^{CMS} cut);%s track #DeltaY_{DET} [mm];%s track #DeltaZ_{DET} [mm];%s", info, pidLatex, pidLatex, perTrackTitle),
		     maxLengthMM/binSizeMM_2dYZ, -0.5*maxLengthMM, 0.5*maxLengthMM,
		     maxLengthMM/binSizeMM_2dYZ, -0.5*maxLengthMM, 0.5*maxLengthMM);
	}
	//
	///////////// DEBUG - valid for 2-prong only, after additional ID cuts

	// SPECIAL PLOTS: check dependence of gamma beam energy on vertex position position (Y_DET) horizontal & perpendicular to the gamma beam axis
	histos2D[(prefix+"_vertexY"+pid+"_len").c_str()]=
	  new TH2F((prefix+"_vertexY"+pid+"_len").c_str(),
		   Form("%s;Vertex position Y_{DET} [mm];%s track length [mm];%s", info, pidLatex, perTrackTitle),
		   (ymax-ymin)/binSizeMM, ymin, ymax,
		   maxLengthMM/binSizeMM, 0, maxLengthMM);
	histos2D[(prefix+"_vertexY"+pid+"_E_LAB").c_str()]=
	  new TH2F((prefix+"_vertexY"+pid+"_E_LAB").c_str(),
		   Form("%s;Vertex position Y_{DET} [mm];%s kinetic energy in LAB [MeV];%s", info, pidLatex, perTrackTitle),
		   (ymax-ymin)/binSizeMM, ymin, ymax,
		   maxKineticEnergyMeV/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV); // 200, 0, maxKineticEnergyMeV);
	histos2D[(prefix+"_vertexY"+pid+"_E_CMS").c_str()]=
	  new TH2F((prefix+"_vertexY"+pid+"_E_CMS").c_str(),
		   Form("%s;Vertex position Y_{DET} [mm];%s kinetic energy in CMS [MeV];%s", info, pidLatex, perTrackTitle),
		   (ymax-ymin)/binSizeMM, ymin, ymax,
		   maxKineticEnergyMeV/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV); // 200, 0, maxKineticEnergyMeV);
	histos2D[(prefix+"_vertexY"+pid+"_cosThetaBEAM_LAB").c_str()]=
	  new TH2F((prefix+"_vertexY"+pid+"_cosThetaBEAM_LAB").c_str(),
		   Form("%s;Vertex position Y_{DET} [mm];%s track cos(#theta_{BEAM}) in LAB;%s", info, pidLatex, perTrackTitle),
		   (ymax-ymin)/binSizeMM, ymin, ymax,
		   100, -1, 1);
	histos2D[(prefix+"_vertexY"+pid+"_cosThetaBEAM_CMS").c_str()]=
	  new TH2F((prefix+"_vertexY"+pid+"_cosThetaBEAM_CMS").c_str(),
		   Form("%s;Vertex position Y_{DET} [mm];%s track cos(#theta_{BEAM}) in CMS;%s", info, pidLatex, perTrackTitle),
		   (ymax-ymin)/binSizeMM, ymin, ymax,
		   100, -1, 1);
	profiles1D[(prefix+"_vertexY"+pid+"_len_prof").c_str()]=
	  new TProfile((prefix+"_vertexY"+pid+"_len_prof").c_str(),
		       Form("%s;Vertex position Y_{DET} [mm];Average %s track length [mm]", info, pidLatex),
		       (ymax-ymin)/binSizeMM, ymin, ymax,
		       0, maxLengthMM);
	profiles1D[(prefix+"_vertexY"+pid+"_E_LAB_prof").c_str()]=
	  new TProfile((prefix+"_vertexY"+pid+"_E_LAB_prof").c_str(),
		       Form("%s;Vertex position Y_{DET} [mm];Average %s kinetic energy in LAB [MeV]", info, pidLatex),
		       (ymax-ymin)/binSizeMM, ymin, ymax,
		       0, maxKineticEnergyMeV);
	profiles1D[(prefix+"_vertexY"+pid+"_E_CMS_prof").c_str()]=
	  new TProfile((prefix+"_vertexY"+pid+"_E_CMS_prof").c_str(),
		       Form("%s;Vertex position Y_{DET} [mm];Average %s kinetic energy in CMS [MeV]", info, pidLatex),
		       (ymax-ymin)/binSizeMM, ymin, ymax,
		       0, maxKineticEnergyMeV);
	// SPECIAL PLOTS: check dependence of gamma beam energy on vertex position position (X_BEAM) horizontal & perpendicular to the gamma beam axis
	histos2D[(prefix+"_vertexXBEAM"+pid+"_E_LAB").c_str()]=
	  new TH2F((prefix+"_vertexXBEAM"+pid+"_E_LAB").c_str(),
		   Form("%s;Vertex position X_{BEAM} [mm];%s kinetic energy in LAB [MeV];%s", info, pidLatex, perTrackTitle),
		   (ymax-ymin)/binSizeMM, ymin, ymax, // X_BEAM -> Y_DET
		   maxKineticEnergyMeV/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV); // 200, 0, maxKineticEnergyMeV);
	histos2D[(prefix+"_vertexXBEAM"+pid+"_E_CMS").c_str()]=
	  new TH2F((prefix+"_vertexXBEAM"+pid+"_E_CMS").c_str(),
		   Form("%s;Vertex position X_{BEAM} [mm];%s kinetic energy in CMS [MeV];%s", info, pidLatex, perTrackTitle),
		   (ymax-ymin)/binSizeMM, ymin, ymax, // X_BEAM -> Y_DET
		   maxKineticEnergyMeV/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV); // 200, 0, maxKineticEnergyMeV);
	histos2D[(prefix+"_vertexXBEAM"+pid+"_cosThetaBEAM_LAB").c_str()]=
	  new TH2F((prefix+"_vertexXBEAM"+pid+"_cosThetaBEAM_LAB").c_str(),
		   Form("%s;Vertex position X_{BEAM} [mm];%s track cos(#theta_{BEAM}) in LAB;%s", info, pidLatex, perTrackTitle),
		   (ymax-ymin)/binSizeMM, ymin, ymax, // X_BEAM -> Y_DET
		   100, -1, 1);
	histos2D[(prefix+"_vertexXBEAM"+pid+"_cosThetaBEAM_CMS").c_str()]=
	  new TH2F((prefix+"_vertexXBEAM"+pid+"_cosThetaBEAM_CMS").c_str(),
		   Form("%s;Vertex position X_{BEAM} [mm];%s track cos(#theta_{BEAM}) in CMS;%s", info, pidLatex, perTrackTitle),
		   (ymax-ymin)/binSizeMM, ymin, ymax, // X_BEAM -> Y_DET
		   100, -1, 1);
	profiles1D[(prefix+"_vertexXBEAM"+pid+"_E_LAB_prof").c_str()]=
	  new TProfile((prefix+"_vertexXBEAM"+pid+"_E_LAB_prof").c_str(),
		       Form("%s;Vertex position X_{BEAM} [mm];Average %s kinetic energy in LAB [MeV]", info, pidLatex),
		       (ymax-ymin)/binSizeMM, ymin, ymax, // X_BEAM -> Y_DET
		       0, maxKineticEnergyMeV);
	profiles1D[(prefix+"_vertexXBEAM"+pid+"_E_CMS_prof").c_str()]=
	  new TProfile((prefix+"_vertexXBEAM"+pid+"_E_CMS_prof").c_str(),
		       Form("%s;Vertex position X_{BEAM} [mm];Average %s kinetic energy in CMS [MeV]", info, pidLatex),
		       (ymax-ymin)/binSizeMM, ymin, ymax, // X_BEAM -> Y_DET
		       0, maxKineticEnergyMeV);
      case 1: // 1,2,3-prong
	histos1D[(prefix+pid+"_E_LAB").c_str()]=
	  new TH1F((prefix+pid+"_E_LAB").c_str(),
		   Form("%s;%s kinetic energy in LAB [MeV];%s", info, pidLatex, perTrackTitle),
		   maxKineticEnergyMeV/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV); // 200, 0, maxKineticEnergyMeV);
	histos2D[(prefix+pid+"_cosThetaBEAM_E_LAB").c_str()]=
	  new TH2F((prefix+pid+"_cosThetaBEAM_E_LAB").c_str(),
		   Form("%s;%s track cos(#theta_{BEAM}) in LAB;%s kinetic energy in LAB [MeV];%s", info, pidLatex, pidLatex, perTrackTitle),
		   100, -1, 1,
		   maxKineticEnergyMeV/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV); // 200, 0, maxKineticEnergyMeV);
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
		   maxKineticEnergyMeV/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV, // 200, 0, maxKineticEnergyMeV,
		   maxKineticEnergyMeV/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV); // 200, 0, maxKineticEnergyMeV);
	histos2D[(prefix+pid+"_E"+pid2+"_E_CMS").c_str()]=
	  new TH2F((prefix+pid+"_E"+pid2+"_E_CMS").c_str(),
		   Form("%s;%s kinetic energy in CMS [MeV];%s kinetic energy in CMS [MeV];%s", info, pidLatex,  pidLatex2, perTrackTitle),
		   maxKineticEnergyMeV/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV, // 200, 0, maxKineticEnergyMeV,
		   maxKineticEnergyMeV/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV); // 200, 0, maxKineticEnergyMeV);

	///////////// DEBUG - valid for 2-prong only, after additional ID cuts
	//
	if(categoryPID[c].size()==2) {
	  histos2D[(prefix+pid+"_E"+pid2+"_E_CMS_CutO16").c_str()]=
	    new TH2F((prefix+pid+"_E"+pid2+"_E_CMS_CutO16").c_str(),
		     Form("%s (with #alpha,C E_{KIN}^{CMS} cut);%s kinetic energy in CMS [MeV];%s kinetic energy in CMS [MeV];%s", info, pidLatex,  pidLatex2, perTrackTitle),
		     maxKineticEnergyMeV/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV, // 200, 0, maxKineticEnergyMeV,
		     maxKineticEnergyMeV/binSizeMeV_kineticEnergy, 0, maxKineticEnergyMeV); // 200, 0, maxKineticEnergyMeV);
	}
	//
	///////////// DEBUG - valid for 2-prong only, after additional ID cuts

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
  // dump list of histogram names
  std::cout<<__FUNCTION__<<": List of booked 1D histograms:"<<std::endl;
  for (auto &h : histos1D) {
    std::cout<<std::left<<std::setw(50)<<std::setfill('.')
	     <<h.first<<"T = "<<h.second->GetTitle()<<std::endl
	     <<std::setfill(' ')<<std::right<<std::setw(50+4)
	     <<"X = "<<std::left<<h.second->GetXaxis()->GetTitle()<<std::endl
	     <<std::right<<std::setw(50+4)
	     <<"Y = "<<std::left<<h.second->GetYaxis()->GetTitle()<<std::endl;
  }
  std::cout<<__FUNCTION__<<": List of booked 1D profile histograms:"<<std::endl;
  for (auto &h : profiles1D) {
    std::cout<<std::left<<std::setw(50)<<std::setfill('.')
	     <<h.first<<"T = "<<h.second->GetTitle()<<std::endl
	     <<std::setfill(' ')<<std::right<<std::setw(50+4)
	     <<"X = "<<std::left<<h.second->GetXaxis()->GetTitle()<<std::endl
	     <<std::right<<std::setw(50+4)
	     <<"Y = "<<std::left<<h.second->GetYaxis()->GetTitle()<<std::endl;
  }
  std::cout<<__FUNCTION__<<": List of booked 2D histograms:"<<std::endl;
  for (auto &h : histos2D) {
    std::cout<<std::left<<std::setw(50)<<std::setfill('.')
	     <<h.first<<"T = "<<h.second->GetTitle()<<std::endl
	     <<std::setfill(' ')<<std::right<<std::setw(50+4)
	     <<"X = "<<std::left<<h.second->GetXaxis()->GetTitle()<<std::endl
	     <<std::right<<std::setw(50+4)
	     <<"Y = "<<std::left<<h.second->GetYaxis()->GetTitle()<<std::endl
	     <<std::right<<std::setw(50+4)
	     <<"Z = "<<std::left<<h.second->GetZaxis()->GetTitle()<<std::endl;
  }
}
///////////////////////////////
///////////////////////////////
void HIGGS_analysis::fillHistos(Track3D *aTrack, eventraw::EventInfo *aEventInfo, bool & isFirst){
  
  // The following assumptions are made:
  // - event is a collection of straight 3D segments
  // - 3D segments share common vertex (STARTING POINT of each segment)
  // - for 2-prong events: longer track is ALPHA, shorter is CARBON
  // - for 3-prong events: all tracks are ALPHAS, descending order by their energy/length

  ///// DEBUG - elapsed run time
  long double unixTimeSec =
    (aEventInfo ? duration_cast<duration<long double>>(tpcreco::eventAbsoluteTime(*aEventInfo).time_since_epoch()).count() : -1); // absolute Unix time [s]
  long double runTimeSec =
    (aEventInfo ? duration_cast<duration<long double>>(tpcreco::eventRelativeTime(*aEventInfo)).count() : -1); // [s]
  static double last_timestamp = 0;
  if(isFirst) {
    last_timestamp=unixTimeSec;
    isFirst=false;
  }
  long double deltaTimeSec= (aEventInfo ? unixTimeSec - last_timestamp : -1); // [s] time difference for rate measurements
  last_timestamp=unixTimeSec;
  ///// DEBUG - elapsed run time

  const int ntracks = aTrack->getSegments().size();
  histos1D["h_ntracks"]->Fill(ntracks);
  if (ntracks==0) return;
  
  // get sorted list of tracks (descending order by track length)
  TrackSegment3DCollection list = aTrack->getSegments();
  std::sort(list.begin(), list.end(),
	    [](const TrackSegment3D& a, const TrackSegment3D& b) {
	      return a.getLength() > b.getLength();
	    });
  // ALL event categories
  TVector3 vertexPos = list.front().getStart(); // DET coordintate system, LAB reference frame
  TVector3 vertexPos_BEAM_LAB = coordinateConverter.detToBeamWithOffset(vertexPos);
  histos1D["h_all_vertexX"]->Fill(vertexPos.X());
  histos1D["h_all_vertexY"]->Fill(vertexPos.Y());
  histos1D["h_all_vertexZ"]->Fill(vertexPos.Z());
  histos2D["h_all_vertexXY"]->Fill(vertexPos.X(), vertexPos.Y());
  histos2D["h_all_vertexYZ"]->Fill(vertexPos.Y(), vertexPos.Z());
  profiles1D["h_all_vertexXY_prof"]->Fill(vertexPos.X(), vertexPos.Y());
  histos1D["h_all_vertexXBEAM"]->Fill(vertexPos_BEAM_LAB.X());
  histos1D["h_all_vertexYBEAM"]->Fill(vertexPos_BEAM_LAB.Y());
  histos1D["h_all_vertexZBEAM"]->Fill(vertexPos_BEAM_LAB.Z());
  histos2D["h_all_vertexZXBEAM"]->Fill(vertexPos_BEAM_LAB.Z(), vertexPos_BEAM_LAB.X());
  histos2D["h_all_vertexXYBEAM"]->Fill(vertexPos_BEAM_LAB.X(), vertexPos_BEAM_LAB.Y());
  profiles1D["h_all_vertexZXBEAM_prof"]->Fill(vertexPos_BEAM_LAB.Z(), vertexPos_BEAM_LAB.X());
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

    auto tangent_BEAM_LAB = coordinateConverter.detToBeam(track.getTangent());
    double phi_BEAM_LAB = tangent_BEAM_LAB.Phi();
    double cosTheta_BEAM_LAB = tangent_BEAM_LAB.CosTheta();
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
    histos1D["h_1prong_vertexXBEAM"]->Fill(vertexPos_BEAM_LAB.X());
    histos1D["h_1prong_vertexYBEAM"]->Fill(vertexPos_BEAM_LAB.Y());
    histos1D["h_1prong_vertexZBEAM"]->Fill(vertexPos_BEAM_LAB.Z());
    histos2D["h_1prong_vertexZXBEAM"]->Fill(vertexPos_BEAM_LAB.Z(), vertexPos_BEAM_LAB.X());
    histos2D["h_1prong_vertexXYBEAM"]->Fill(vertexPos_BEAM_LAB.X(), vertexPos_BEAM_LAB.Y());
    profiles1D["h_1prong_vertexZXBEAM_prof"]->Fill(vertexPos_BEAM_LAB.Z(), vertexPos_BEAM_LAB.X());
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

    auto tangent_BEAM_LAB = coordinateConverter.detToBeam(track.getTangent());
    double phi_BEAM_LAB = tangent_BEAM_LAB.Phi();
    double cosTheta_BEAM_LAB = tangent_BEAM_LAB.CosTheta();
    histos1D["h_1prong_alpha_phiBEAM_LAB"]->Fill(phi_BEAM_LAB);
    histos1D["h_1prong_alpha_thetaBEAM_LAB"]->Fill(acos(cosTheta_BEAM_LAB));
    histos1D["h_1prong_alpha_cosThetaBEAM_LAB"]->Fill(cosTheta_BEAM_LAB);
    histos2D["h_1prong_alpha_cosThetaBEAM_len_LAB"]->Fill(cosTheta_BEAM_LAB, len);
    profiles1D["h_1prong_alpha_cosThetaBEAM_len_LAB_prof"]->Fill(cosTheta_BEAM_LAB, len);

    // reconstruct kinetic energy from particle range [mm]
    double T_LAB=myRangeCalculator.getIonEnergyMeV(/*IonRangeCalculator::*/ALPHA, len);
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
    histos1D["h_2prong_vertexXBEAM"]->Fill(vertexPos_BEAM_LAB.X());
    histos1D["h_2prong_vertexYBEAM"]->Fill(vertexPos_BEAM_LAB.Y());
    histos1D["h_2prong_vertexZBEAM"]->Fill(vertexPos_BEAM_LAB.Z());
    histos2D["h_2prong_vertexZXBEAM"]->Fill(vertexPos_BEAM_LAB.Z(), vertexPos_BEAM_LAB.X());
    histos2D["h_2prong_vertexXYBEAM"]->Fill(vertexPos_BEAM_LAB.X(), vertexPos_BEAM_LAB.Y());
    profiles1D["h_2prong_vertexZXBEAM_prof"]->Fill(vertexPos_BEAM_LAB.Z(), vertexPos_BEAM_LAB.X());

    const double alpha_len = list.front().getLength(); // longest = alpha
    const double carbon_len = list.back().getLength(); // shortest = carbon
    histos1D["h_2prong_alpha_len"]->Fill(alpha_len);
    histos2D["h_2prong_alpha_len_carbon_len"]->Fill(alpha_len, carbon_len);
    histos1D["h_2prong_lenSum"]->Fill(alpha_len+carbon_len);
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

    auto alpha_tangent_BEAM_LAB = coordinateConverter.detToBeam(list.front().getTangent());
    auto carbon_tangent_BEAM_LAB = coordinateConverter.detToBeam(list.back().getTangent());
    double alpha_phi_BEAM_LAB = alpha_tangent_BEAM_LAB.Phi();
    double carbon_phi_BEAM_LAB = carbon_tangent_BEAM_LAB.Phi();
    double alpha_cosTheta_BEAM_LAB = alpha_tangent_BEAM_LAB.CosTheta();
    double carbon_cosTheta_BEAM_LAB = carbon_tangent_BEAM_LAB.CosTheta();
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
    const double alphaMass=myRangeCalculator.getIonMassMeV(/*IonRangeCalculator::*/ALPHA);
    const double carbonMass=myRangeCalculator.getIonMassMeV(/*IonRangeCalculator::*/CARBON_12);
    const double alpha_T_LAB=myRangeCalculator.getIonEnergyMeV(/*IonRangeCalculator::*/ALPHA, alpha_len);
    const double carbon_T_LAB=myRangeCalculator.getIonEnergyMeV(/*IonRangeCalculator::*/CARBON_12, carbon_len);
    double alpha_p_LAB=sqrt(alpha_T_LAB*(alpha_T_LAB+2*alphaMass));
    double carbon_p_LAB=sqrt(carbon_T_LAB*(carbon_T_LAB+2*carbonMass));
    // construct TLorentzVector in DET/LAB frame
    TLorentzVector alphaP4_DET_LAB(alpha_p_LAB*list.front().getTangent(), alphaMass+alpha_T_LAB);
    TLorentzVector carbonP4_DET_LAB(carbon_p_LAB*list.back().getTangent(), carbonMass+carbon_T_LAB);
    TLorentzVector sumP4_DET_LAB = alphaP4_DET_LAB + carbonP4_DET_LAB;
    // boost P4 from DET/LAB frame to CMS frame (see TLorentzVector::Boost() convention!)
    const double oxygenMassGroundState=myRangeCalculator.getIonMassMeV(/*IonRangeCalculator::*/OXYGEN_16);

    double photon_E_LAB=sumP4_DET_LAB.E()-oxygenMassGroundState; // reconstructed gamma beam energy in LAB
    auto beta_DET_LAB=TVector3(0,0,0);
    if(useNominalPhotonEnergyForBoost) {
      // assume nominal direction and nominal gamma beam energy
      beta_DET_LAB=getBetaVectorOfCMS(oxygenMassGroundState);
    } else {
      // assume nominal beam direction, but use reconstructed gamma beam energy per event
      beta_DET_LAB=getBetaVectorOfCMS(oxygenMassGroundState).Unit()*(photon_E_LAB/(photon_E_LAB+oxygenMassGroundState));
    }

    TLorentzVector alphaP4_BEAM_LAB = coordinateConverter.detToBeam(alphaP4_DET_LAB);
    TLorentzVector carbonP4_BEAM_LAB = coordinateConverter.detToBeam(carbonP4_DET_LAB);
    TLorentzVector alphaP4_DET_CMS(alphaP4_DET_LAB);
    TLorentzVector carbonP4_DET_CMS(carbonP4_DET_LAB);
    alphaP4_DET_CMS.Boost(-1.0*beta_DET_LAB);
    carbonP4_DET_CMS.Boost(-1.0*beta_DET_LAB);

    TLorentzVector alphaP4_BEAM_CMS = coordinateConverter.detToBeam(alphaP4_DET_CMS);
    TLorentzVector carbonP4_BEAM_CMS = coordinateConverter.detToBeam(carbonP4_DET_CMS);
    double alpha_T_CMS=alphaP4_BEAM_CMS.E()-alphaP4_BEAM_CMS.M(); // [MeV]
    double carbon_T_CMS=carbonP4_BEAM_CMS.E()-carbonP4_BEAM_CMS.M(); // [MeV]
    //    double invariantMass=(alphaP4_BEAM_CMS+carbonP4_BEAM_CMS).M();// [MeV/c^2]
    double totalEnergy_CMS=(alphaP4_BEAM_CMS+carbonP4_BEAM_CMS).E(); // [MeV], mass of stationary excited Oxygen state
    double oxygenMassExcited=totalEnergy_CMS;
    double oxygenExcitationEnergy=oxygenMassExcited-oxygenMassGroundState;
    double Qvalue_CMS=oxygenMassExcited-alphaMass-carbonMass;

    histos1D["h_2prong_alpha_E_LAB"]->Fill(alpha_T_LAB);
    histos1D["h_2prong_carbon_E_LAB"]->Fill(carbon_T_LAB);
    histos1D["h_2prong_alpha_E_CMS"]->Fill(alpha_T_CMS);
    histos1D["h_2prong_carbon_E_CMS"]->Fill(carbon_T_CMS);
    histos1D["h_2prong_total_PxBEAM_CMS"]->Fill((alphaP4_BEAM_CMS+carbonP4_BEAM_CMS).Px());
    histos1D["h_2prong_total_PyBEAM_CMS"]->Fill((alphaP4_BEAM_CMS+carbonP4_BEAM_CMS).Py());
    histos1D["h_2prong_total_PzBEAM_CMS"]->Fill((alphaP4_BEAM_CMS+carbonP4_BEAM_CMS).Pz());
    histos1D["h_2prong_total_PxBEAM_LAB"]->Fill((alphaP4_BEAM_LAB+carbonP4_BEAM_LAB).Px());
    histos1D["h_2prong_total_PyBEAM_LAB"]->Fill((alphaP4_BEAM_LAB+carbonP4_BEAM_LAB).Py());
    histos1D["h_2prong_total_PzBEAM_LAB"]->Fill((alphaP4_BEAM_LAB+carbonP4_BEAM_LAB).Pz());
    histos1D["h_2prong_total_E_CMS"]->Fill(totalEnergy_CMS);
    histos1D["h_2prong_excitation_E_CMS"]->Fill(oxygenExcitationEnergy);
    histos1D["h_2prong_Qvalue_CMS"]->Fill(Qvalue_CMS);
    histos1D["h_2prong_gamma_E_LAB"]->Fill(photon_E_LAB);
    histos1D["h_2prong_E_CMS"]->Fill(alpha_T_CMS+carbon_T_CMS);
    histos1D["h_2prong_E_LAB"]->Fill(alpha_T_LAB+carbon_T_LAB);

    ///////////// DEBUG
    //
    // for automatic RECO:
    const double cut_center_alpha_T_CMS = 1.85; // MeV // TODO - TO BE PARAMETERIZED
    const double cut_center_carbon_T_CMS = 0.53; // MeV // TODO - TO BE PARAMETERIZED
    const double cut_ellipse_alpha_T_CMS = 0.15; // MeV // TODO - TO BE PARAMETERIZED
    const double cut_ellipse_carbon_T_CMS = 0.15; // MeV // TODO - TO BE PARAMETERIZED
    // for clicked RECO:
    // const double cut_center_alpha_T_CMS = 1.82; // MeV // TODO - TO BE PARAMETERIZED
    // const double cut_center_carbon_T_CMS = 0.50; // MeV // TODO - TO BE PARAMETERIZED
    // const double cut_ellipse_alpha_T_CMS = 0.15; // MeV // TODO - TO BE PARAMETERIZED
    // const double cut_ellipse_carbon_T_CMS = 0.15; // MeV // TODO - TO BE PARAMETERIZED
    const bool passed_O16_idCut=(pow((alpha_T_CMS-cut_center_alpha_T_CMS)/cut_ellipse_alpha_T_CMS, 2)+
                                 pow((carbon_T_CMS-cut_center_carbon_T_CMS)/cut_ellipse_carbon_T_CMS, 2) < 1); // elliptical cut
    TLorentzVector carbonP4_BEAM_CMS_fromAlpha;
    carbonP4_BEAM_CMS_fromAlpha.SetVectM(-alphaP4_BEAM_CMS.Vect(), carbonMass); // corrected P4 in CMS
    double oxygenMassExcited_fromAlpha=(alphaP4_BEAM_CMS+carbonP4_BEAM_CMS_fromAlpha).E(); // MeV
    double oxygenExcitationEnergy_fromAlpha=oxygenMassExcited_fromAlpha-oxygenMassGroundState; // MeV
    double oxygenExcitationEnergy_fromAlpha_XCHECK=alpha_T_CMS*(1+sqrt(1+2*alphaMass/alpha_T_CMS+pow(carbonMass/alpha_T_CMS,2))-carbonMass/alpha_T_CMS)-(oxygenMassGroundState-alphaMass-carbonMass); // MeV
    double photon_E_CMS_fromAlpha=oxygenExcitationEnergy_fromAlpha*(1-0.5*oxygenExcitationEnergy_fromAlpha/(oxygenMassGroundState+oxygenExcitationEnergy_fromAlpha)); // MeV
    double photon_E_LAB_fromAlpha=photon_E_CMS_fromAlpha*(photon_E_CMS_fromAlpha/oxygenMassGroundState+sqrt(pow(photon_E_CMS_fromAlpha/oxygenMassGroundState, 2)+1)); // MeV
    double photon_E_LAB_fromAlpha_XCHECK=0.5*(pow(oxygenMassGroundState+oxygenExcitationEnergy_fromAlpha, 2)/oxygenMassGroundState-oxygenMassGroundState); // MeV

    std::cout << __FUNCTION__ << ": 2-prong X-CHECK: Ex(method #1)=" << oxygenExcitationEnergy_fromAlpha
	     << " Ex(method #2)=" << oxygenExcitationEnergy_fromAlpha_XCHECK
	     << " diff=" << oxygenExcitationEnergy_fromAlpha-oxygenExcitationEnergy_fromAlpha_XCHECK << std::endl;
    std::cout << __FUNCTION__ << ": 2-prong X-CHECK: Eg_LAB(method #1)=" << photon_E_LAB_fromAlpha
	     << " Eg_LAB(method #2)=" << photon_E_LAB_fromAlpha_XCHECK
	     << " diff=" << photon_E_LAB_fromAlpha-photon_E_LAB_fromAlpha_XCHECK << std::endl;
    histos1D["h_2prong_excitation_E_CMS_fromAlpha"]->Fill(oxygenExcitationEnergy_fromAlpha);
    histos1D["h_2prong_gamma_E_LAB_fromAlpha"]->Fill(photon_E_LAB_fromAlpha);
    histos1D["h_2prong_runTime"]->Fill(runTimeSec);
      histos1D["h_2prong_deltaTime"]->Fill(deltaTimeSec);
    // DEBUG - after additional ID cuts
    if(passed_O16_idCut) {
      histos1D["h_2prong_excitation_E_CMS_fromAlpha_CutO16"]->Fill(oxygenExcitationEnergy_fromAlpha);
      histos1D["h_2prong_gamma_E_LAB_fromAlpha_CutO16"]->Fill(photon_E_LAB_fromAlpha);
      histos1D["h_2prong_runTime_CutO16"]->Fill(runTimeSec);
      histos1D["h_2prong_deltaTime_CutO16"]->Fill(deltaTimeSec);
    }
    //
    ///////////// DEBUG

    // calculate angles in CMS reference frame in BEAM coordinate system
    double delta_CMS=alphaP4_BEAM_CMS.Angle(carbonP4_BEAM_CMS.Vect()); // [rad]
    double alpha_phi_BEAM_CMS=alphaP4_BEAM_CMS.Phi(); // [rad], azimuthal angle from X axis
    double carbon_phi_BEAM_CMS=carbonP4_BEAM_CMS.Phi(); // [rad], azimuthal angle from X axis
    double alpha_cosTheta_BEAM_CMS=alphaP4_BEAM_CMS.CosTheta(); // [rad], polar angle from Z axis
    double carbon_cosTheta_BEAM_CMS=carbonP4_BEAM_CMS.CosTheta(); // [rad], polar angle from Z axis

    // alpha particle
    histos1D["h_2prong_alpha_phiBEAM_CMS"]->Fill(alpha_phi_BEAM_CMS);
    histos1D["h_2prong_alpha_thetaBEAM_CMS"]->Fill(acos(alpha_cosTheta_BEAM_CMS));
    histos1D["h_2prong_alpha_cosThetaBEAM_CMS"]->Fill(alpha_cosTheta_BEAM_CMS);
    histos2D["h_2prong_alpha_cosThetaBEAM_E_CMS"]->Fill(alpha_cosTheta_BEAM_CMS, alpha_T_CMS);
    histos2D["h_2prong_alpha_cosThetaBEAM_E_LAB"]->Fill(alpha_cosTheta_BEAM_LAB, alpha_T_LAB);
    profiles1D["h_2prong_alpha_cosThetaBEAM_E_CMS_prof"]->Fill(alpha_cosTheta_BEAM_CMS, alpha_T_CMS);
    profiles1D["h_2prong_alpha_cosThetaBEAM_E_LAB_prof"]->Fill(alpha_cosTheta_BEAM_LAB, alpha_T_LAB);
    // carbon recoil
    histos1D["h_2prong_carbon_phiBEAM_CMS"]->Fill(carbon_phi_BEAM_CMS);
    histos1D["h_2prong_carbon_thetaBEAM_CMS"]->Fill(acos(carbon_cosTheta_BEAM_CMS));
    histos1D["h_2prong_carbon_cosThetaBEAM_CMS"]->Fill(carbon_cosTheta_BEAM_CMS);
    histos2D["h_2prong_carbon_cosThetaBEAM_E_CMS"]->Fill(carbon_cosTheta_BEAM_CMS, carbon_T_CMS);
    histos2D["h_2prong_carbon_cosThetaBEAM_E_LAB"]->Fill(carbon_cosTheta_BEAM_CMS, carbon_T_LAB);
    profiles1D["h_2prong_carbon_cosThetaBEAM_E_CMS_prof"]->Fill(carbon_cosTheta_BEAM_CMS, carbon_T_CMS);
    profiles1D["h_2prong_carbon_cosThetaBEAM_E_LAB_prof"]->Fill(carbon_cosTheta_BEAM_LAB, carbon_T_LAB);
    // alpha-carbon correlations
    histos1D["h_2prong_alpha_carbon_delta_CMS"]->Fill(delta_CMS);
    histos1D["h_2prong_alpha_carbon_cosDelta_CMS"]->Fill(cos(delta_CMS));
    histos2D["h_2prong_alpha_E_carbon_E_CMS"]->Fill(alpha_T_CMS, carbon_T_CMS);
    histos2D["h_2prong_alpha_E_carbon_E_LAB"]->Fill(alpha_T_LAB, carbon_T_LAB);

    ///////////// DEBUG - after additional ID cuts
    //
    if(passed_O16_idCut) {
      // alpha particle - after additional ID cuts
      histos1D["h_2prong_alpha_E_CMS_CutO16"]->Fill(alpha_T_CMS);
      histos1D["h_2prong_alpha_phiBEAM_CMS_CutO16"]->Fill(alpha_phi_BEAM_CMS);
      histos1D["h_2prong_alpha_thetaBEAM_CMS_CutO16"]->Fill(acos(alpha_cosTheta_BEAM_CMS));
      histos1D["h_2prong_alpha_cosThetaBEAM_CMS_CutO16"]->Fill(alpha_cosTheta_BEAM_CMS);
      histos2D["h_2prong_alpha_cosThetaBEAM_E_CMS_CutO16"]->Fill(alpha_cosTheta_BEAM_CMS, alpha_T_CMS);
      histos2D["h_2prong_alpha_deltaYZ_CutO16"]->Fill(alpha_len*list.front().getTangent().Y(), alpha_len*list.front().getTangent().Z());
      // carbon recoil - after additional ID cuts
      histos1D["h_2prong_carbon_E_CMS_CutO16"]->Fill(carbon_T_CMS);
      histos1D["h_2prong_carbon_phiBEAM_CMS_CutO16"]->Fill(carbon_phi_BEAM_CMS);
      histos1D["h_2prong_carbon_thetaBEAM_CMS_CutO16"]->Fill(acos(carbon_cosTheta_BEAM_CMS));
      histos1D["h_2prong_carbon_cosThetaBEAM_CMS_CutO16"]->Fill(carbon_cosTheta_BEAM_CMS);
      histos2D["h_2prong_carbon_cosThetaBEAM_E_CMS_CutO16"]->Fill(carbon_cosTheta_BEAM_CMS, carbon_T_CMS);
      histos2D["h_2prong_carbon_deltaYZ_CutO16"]->Fill(carbon_len*list.back().getTangent().Y(), carbon_len*list.back().getTangent().Z());
      // alpha-carbon correlations - after additional ID cuts
      histos2D["h_2prong_alpha_E_carbon_E_CMS_CutO16"]->Fill(alpha_T_CMS, carbon_T_CMS);
    }
    //
    ///////////// DEBUG - after additional ID cuts

    // SPECIAL PLOTS: check dependence of gamma beam energy on vertex position (Y_DET) horizontal & perpendicular to the gamma beam axis
    histos2D["h_2prong_vertexX_lenSum"]->Fill(vertexPos.X(), alpha_len+carbon_len);
    histos2D["h_2prong_vertexY_lenSum"]->Fill(vertexPos.Y(), alpha_len+carbon_len);
    histos2D["h_2prong_vertexZ_lenSum"]->Fill(vertexPos.Z(), alpha_len+carbon_len);
    histos2D["h_2prong_vertexY_alpha_len"]->Fill(vertexPos.Y(), alpha_len);
    histos2D["h_2prong_vertexY_carbon_len"]->Fill(vertexPos.Y(), carbon_len);
    histos2D["h_2prong_vertexY_alpha_E_LAB"]->Fill(vertexPos.Y(), alpha_T_LAB);
    histos2D["h_2prong_vertexY_carbon_E_LAB"]->Fill(vertexPos.Y(), carbon_T_LAB);
    histos2D["h_2prong_vertexY_alpha_E_CMS"]->Fill(vertexPos.Y(), alpha_T_CMS);
    histos2D["h_2prong_vertexY_carbon_E_CMS"]->Fill(vertexPos.Y(), carbon_T_CMS);
    histos2D["h_2prong_vertexY_alpha_cosThetaBEAM_LAB"]->Fill(vertexPos.Y(), alpha_cosTheta_BEAM_LAB);
    histos2D["h_2prong_vertexY_carbon_cosThetaBEAM_LAB"]->Fill(vertexPos.Y(), carbon_cosTheta_BEAM_LAB);
    histos2D["h_2prong_vertexY_alpha_cosThetaBEAM_CMS"]->Fill(vertexPos.Y(), alpha_cosTheta_BEAM_CMS);
    histos2D["h_2prong_vertexY_carbon_cosThetaBEAM_CMS"]->Fill(vertexPos.Y(), carbon_cosTheta_BEAM_CMS);
    histos2D["h_2prong_vertexY_gamma_E_LAB"]->Fill(vertexPos.Y(), photon_E_LAB);
    histos2D["h_2prong_vertexY_Qvalue_CMS"]->Fill(vertexPos.Y(), Qvalue_CMS);
    profiles1D["h_2prong_vertexX_lenSum_prof"]->Fill(vertexPos.X(), alpha_len+carbon_len);
    profiles1D["h_2prong_vertexY_lenSum_prof"]->Fill(vertexPos.Y(), alpha_len+carbon_len);
    profiles1D["h_2prong_vertexZ_lenSum_prof"]->Fill(vertexPos.Z(), alpha_len+carbon_len);
    profiles1D["h_2prong_vertexY_alpha_len_prof"]->Fill(vertexPos.Y(), alpha_len);
    profiles1D["h_2prong_vertexY_carbon_len_prof"]->Fill(vertexPos.Y(), carbon_len);
    profiles1D["h_2prong_vertexY_alpha_E_LAB_prof"]->Fill(vertexPos.Y(), alpha_T_LAB);
    profiles1D["h_2prong_vertexY_carbon_E_LAB_prof"]->Fill(vertexPos.Y(), carbon_T_LAB);
    profiles1D["h_2prong_vertexY_alpha_E_CMS_prof"]->Fill(vertexPos.Y(), alpha_T_CMS);
    profiles1D["h_2prong_vertexY_carbon_E_CMS_prof"]->Fill(vertexPos.Y(), carbon_T_CMS);
    profiles1D["h_2prong_vertexY_gamma_E_LAB_prof"]->Fill(vertexPos.Y(), photon_E_LAB);
    profiles1D["h_2prong_vertexY_Qvalue_CMS_prof"]->Fill(vertexPos.Y(), Qvalue_CMS);
    // SPECIAL PLOTS: check dependence of gamma beam energy on vertex position (X_BEAM) horizontal & perpendicular to the gamma beam axis
    histos2D["h_2prong_vertexXBEAM_alpha_E_LAB"]->Fill(vertexPos_BEAM_LAB.X(), alpha_T_LAB);
    histos2D["h_2prong_vertexXBEAM_carbon_E_LAB"]->Fill(vertexPos_BEAM_LAB.X(), carbon_T_LAB);
    histos2D["h_2prong_vertexXBEAM_alpha_E_CMS"]->Fill(vertexPos_BEAM_LAB.X(), alpha_T_CMS);
    histos2D["h_2prong_vertexXBEAM_carbon_E_CMS"]->Fill(vertexPos_BEAM_LAB.X(), carbon_T_CMS);
    histos2D["h_2prong_vertexXBEAM_alpha_cosThetaBEAM_LAB"]->Fill(vertexPos_BEAM_LAB.X(), alpha_cosTheta_BEAM_LAB);
    histos2D["h_2prong_vertexXBEAM_carbon_cosThetaBEAM_LAB"]->Fill(vertexPos_BEAM_LAB.X(), carbon_cosTheta_BEAM_LAB);
    histos2D["h_2prong_vertexXBEAM_alpha_cosThetaBEAM_CMS"]->Fill(vertexPos_BEAM_LAB.X(), alpha_cosTheta_BEAM_CMS);
    histos2D["h_2prong_vertexXBEAM_carbon_cosThetaBEAM_CMS"]->Fill(vertexPos_BEAM_LAB.X(), carbon_cosTheta_BEAM_CMS);
    histos2D["h_2prong_vertexXBEAM_gamma_E_LAB"]->Fill(vertexPos_BEAM_LAB.X(), photon_E_LAB);
    histos2D["h_2prong_vertexXBEAM_Qvalue_CMS"]->Fill(vertexPos_BEAM_LAB.X(), Qvalue_CMS);
    profiles1D["h_2prong_vertexXBEAM_alpha_E_LAB_prof"]->Fill(vertexPos_BEAM_LAB.X(), alpha_T_LAB);
    profiles1D["h_2prong_vertexXBEAM_carbon_E_LAB_prof"]->Fill(vertexPos_BEAM_LAB.X(), carbon_T_LAB);
    profiles1D["h_2prong_vertexXBEAM_alpha_E_CMS_prof"]->Fill(vertexPos_BEAM_LAB.X(), alpha_T_CMS);
    profiles1D["h_2prong_vertexXBEAM_carbon_E_CMS_prof"]->Fill(vertexPos_BEAM_LAB.X(), carbon_T_CMS);
    profiles1D["h_2prong_vertexXBEAM_gamma_E_LAB_prof"]->Fill(vertexPos_BEAM_LAB.X(), photon_E_LAB);
    profiles1D["h_2prong_vertexXBEAM_Qvalue_CMS_prof"]->Fill(vertexPos_BEAM_LAB.X(), Qvalue_CMS);
  }
  // 3-prong (triple alpha)
  if(ntracks==3) {
    histos1D["h_3prong_vertexX"]->Fill(vertexPos.X());
    histos1D["h_3prong_vertexY"]->Fill(vertexPos.Y());
    histos1D["h_3prong_vertexZ"]->Fill(vertexPos.Z());
    histos2D["h_3prong_vertexXY"]->Fill(vertexPos.X(), vertexPos.Y());
    histos2D["h_3prong_vertexYZ"]->Fill(vertexPos.Y(), vertexPos.Z());
    profiles1D["h_3prong_vertexXY_prof"]->Fill(vertexPos.X(), vertexPos.Y());
    histos1D["h_3prong_vertexXBEAM"]->Fill(vertexPos_BEAM_LAB.X());
    histos1D["h_3prong_vertexYBEAM"]->Fill(vertexPos_BEAM_LAB.Y());
    histos1D["h_3prong_vertexZBEAM"]->Fill(vertexPos_BEAM_LAB.Z());
    histos2D["h_3prong_vertexZXBEAM"]->Fill(vertexPos_BEAM_LAB.Z(), vertexPos_BEAM_LAB.X());
    histos2D["h_3prong_vertexXYBEAM"]->Fill(vertexPos_BEAM_LAB.X(), vertexPos_BEAM_LAB.Y());
    profiles1D["h_3prong_vertexZXBEAM_prof"]->Fill(vertexPos_BEAM_LAB.Z(), vertexPos_BEAM_LAB.X());

    const double carbonMassGroundState=myRangeCalculator.getIonMassMeV(/*IonRangeCalculator::*/CARBON_12);
    const double alphaMass=myRangeCalculator.getIonMassMeV(/*IonRangeCalculator::*/ALPHA);

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
    TLorentzVector alphaP4_BEAM_LAB[3]; // [MeV]
    TLorentzVector alphaP4_DET_CMS[3]; // [MeV]
    TLorentzVector alphaP4_BEAM_CMS[3]; // [MeV]
    // initialize total sums
    double lengthSUM=0.0; // [mm]
    double massSUM=0.0; // [MeV]
    TLorentzVector sumP4_BEAM_CMS(0,0,0,0); // [MeV]
    TLorentzVector sumP4_DET_LAB(0,0,0,0); // [MeV]
    TLorentzVector sumP4_BEAM_LAB(0,0,0,0); // [MeV]

    // calculate array of track properties and total sums in LAB
    for(auto i=0;i<3;i++) {
      auto track=list.at(i);
      alpha_len[i] = track.getLength();

      auto alpha_tangent_BEAM_LAB = coordinateConverter.detToBeam(track.getTangent());
      alpha_phi_BEAM_LAB[i] = alpha_tangent_BEAM_LAB.Phi();
      alpha_cosTheta_BEAM_LAB[i] = alpha_tangent_BEAM_LAB.CosTheta();
      
      // reconstruct kinetic energy from particle range [mm]
      alpha_T_LAB[i]=myRangeCalculator.getIonEnergyMeV(/*IonRangeCalculator::*/ALPHA, alpha_len[i]);
      alpha_p_LAB[i]=sqrt(alpha_T_LAB[i]*(alpha_T_LAB[i]+2*alphaMass));
      // construct TLorentzVector in DET/LAB frame
      alphaP4_DET_LAB[i]=TLorentzVector(alpha_p_LAB[i]*track.getTangent(), alphaMass+alpha_T_LAB[i]);
      // TODO
      // TODO switch to DET->BEAM dedicated converter class!!!
      // TODO
      // change DET-->BEAM coordinate transformation for the HIGS experiment
      // formulas below are valid provided that beam direction is anti-paralell to X_DET (HIGS case):
      // X_DET -> -Z_BEAM
      // Y_DET ->  X_BEAM
      // Z_DET -> -Y_BEAM
      //      alphaP4_BEAM_LAB[i]=TLorentzVector(alphaP4_DET_LAB[i].Py(), -alphaP4_DET_LAB[i].Pz(), -alphaP4_DET_LAB[i].Px(), alphaP4_DET_LAB[i].E());
      alphaP4_BEAM_LAB[i] = coordinateConverter.detToBeam(alphaP4_DET_LAB[i]);

      // update total sums
      lengthSUM+=alpha_len[i];
      massSUM+=alphaMass;
      sumP4_DET_LAB+=alphaP4_DET_LAB[i];
      sumP4_BEAM_LAB+=alphaP4_BEAM_LAB[i];
    }

    double photon_E_LAB=sumP4_DET_LAB.E()-carbonMassGroundState; // reconstructed gamma beam energy in LAB
    auto beta_DET_LAB=TVector3(0,0,0);
    if(useNominalPhotonEnergyForBoost) {
      // assume nominal direction and nominal gamma beam energy
      beta_DET_LAB=getBetaVectorOfCMS(carbonMassGroundState);
    } else {
      // assume nominal beam direction, but use reconstructed gamma beam energy per event
      beta_DET_LAB=getBetaVectorOfCMS(carbonMassGroundState).Unit()*(photon_E_LAB/(photon_E_LAB+carbonMassGroundState));
    }

    // calculate array of track properties and total sums in CMS
    for(auto i=0;i<3;i++) {
      auto track=list.at(i);

      // boost P4 from DET/LAB frame to CMS frame (see TLorentzVector::Boost() convention!)
      alphaP4_DET_CMS[i]=TLorentzVector(alphaP4_DET_LAB[i]);
      alphaP4_DET_CMS[i].Boost(-1.0*beta_DET_LAB);
      alphaP4_BEAM_CMS[i] = coordinateConverter.detToBeam(alphaP4_DET_CMS[i]);
      alpha_T_CMS[i]=alphaP4_BEAM_CMS[i].E()-alphaP4_BEAM_CMS[i].M(); // [MeV]
      alpha_phi_BEAM_CMS[i]=alphaP4_BEAM_CMS[i].Phi(); // [rad], azimuthal angle from X axis
      alpha_cosTheta_BEAM_CMS[i]=alphaP4_BEAM_CMS[i].CosTheta(); // [rad], azimuthal angle from X axis

      // update total sums
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

      // SPECIAL PLOTS: check dependence of gamma beam energy on vertex position (Y_DET) horizontal & perpendicular to the gamma beam axis
      histos2D[Form("h_3prong_vertexY_alpha%d_E_LAB",i+1)]->Fill(vertexPos.Y(), alpha_T_LAB[i]);
      histos2D[Form("h_3prong_vertexY_alpha%d_E_CMS",i+1)]->Fill(vertexPos.Y(), alpha_T_CMS[i]);
      histos2D[Form("h_3prong_vertexY_alpha%d_len",i+1)]->Fill(vertexPos.Y(), alpha_len[i]);
      histos2D[Form("h_3prong_vertexY_alpha%d_cosThetaBEAM_LAB",i+1)]->Fill(vertexPos.Y(), alpha_cosTheta_BEAM_LAB[i]);
      histos2D[Form("h_3prong_vertexY_alpha%d_cosThetaBEAM_CMS",i+1)]->Fill(vertexPos.Y(), alpha_cosTheta_BEAM_CMS[i]);
      profiles1D[Form("h_3prong_vertexY_alpha%d_E_LAB_prof",i+1)]->Fill(vertexPos.Y(), alpha_T_LAB[i]);
      profiles1D[Form("h_3prong_vertexY_alpha%d_E_CMS_prof",i+1)]->Fill(vertexPos.Y(), alpha_T_CMS[i]);
      profiles1D[Form("h_3prong_vertexY_alpha%d_len_prof",i+1)]->Fill(vertexPos.Y(), alpha_len[i]);
      // SPECIAL PLOTS: check dependence of gamma beam energy on vertex position (X_BEAM) horizontal & perpendicular to the gamma beam axis
      histos2D[Form("h_3prong_vertexXBEAM_alpha%d_E_LAB",i+1)]->Fill(vertexPos_BEAM_LAB.X(), alpha_T_LAB[i]);
      histos2D[Form("h_3prong_vertexXBEAM_alpha%d_E_CMS",i+1)]->Fill(vertexPos_BEAM_LAB.X(), alpha_T_CMS[i]);
      histos2D[Form("h_3prong_vertexXBEAM_alpha%d_cosThetaBEAM_LAB",i+1)]->Fill(vertexPos_BEAM_LAB.X(), alpha_cosTheta_BEAM_LAB[i]);
      histos2D[Form("h_3prong_vertexXBEAM_alpha%d_cosThetaBEAM_CMS",i+1)]->Fill(vertexPos_BEAM_LAB.X(), alpha_cosTheta_BEAM_CMS[i]);
      profiles1D[Form("h_3prong_vertexXBEAM_alpha%d_E_LAB_prof",i+1)]->Fill(vertexPos_BEAM_LAB.X(), alpha_T_LAB[i]);
      profiles1D[Form("h_3prong_vertexXBEAM_alpha%d_E_CMS_prof",i+1)]->Fill(vertexPos_BEAM_LAB.X(), alpha_T_CMS[i]);

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
    double totalEnergy_CMS=sumP4_BEAM_CMS.E(); // [MeV], mass of stationary excited Carbon state
    double carbonMassExcited=totalEnergy_CMS;
    double carbonExcitationEnergy=carbonMassExcited-carbonMassGroundState;
    double Qvalue_CMS=carbonMassExcited-massSUM;
    histos1D["h_3prong_lenSum"]->Fill(lengthSUM);
    histos1D["h_3prong_total_PxBEAM_CMS"]->Fill(sumP4_BEAM_CMS.Px());
    histos1D["h_3prong_total_PyBEAM_CMS"]->Fill(sumP4_BEAM_CMS.Py());
    histos1D["h_3prong_total_PzBEAM_CMS"]->Fill(sumP4_BEAM_CMS.Pz());
    histos1D["h_3prong_total_PxBEAM_LAB"]->Fill(sumP4_BEAM_LAB.Px());
    histos1D["h_3prong_total_PyBEAM_LAB"]->Fill(sumP4_BEAM_LAB.Py());
    histos1D["h_3prong_total_PzBEAM_LAB"]->Fill(sumP4_BEAM_LAB.Pz());
    histos1D["h_3prong_total_E_CMS"]->Fill(totalEnergy_CMS);
    histos1D["h_3prong_excitation_E_CMS"]->Fill(carbonExcitationEnergy);
    histos1D["h_3prong_Qvalue_CMS"]->Fill(Qvalue_CMS);
    histos1D["h_3prong_gamma_E_LAB"]->Fill(photon_E_LAB);
    histos1D["h_3prong_E_CMS"]->Fill(alpha_T_LAB[0]+alpha_T_LAB[1]+alpha_T_LAB[2]);
    histos1D["h_3prong_E_LAB"]->Fill(alpha_T_CMS[0]+alpha_T_CMS[1]+alpha_T_CMS[2]);

    // SPECIAL PLOTS: check dependence of gamma beam energy on vertex position (Y_DET) horizontal & perpendicular to the gamma beam axis
    histos2D["h_3prong_vertexX_lenSum"]->Fill(vertexPos.X(), lengthSUM);
    histos2D["h_3prong_vertexY_lenSum"]->Fill(vertexPos.Y(), lengthSUM);
    histos2D["h_3prong_vertexZ_lenSum"]->Fill(vertexPos.Z(), lengthSUM);
    histos2D["h_3prong_vertexY_gamma_E_LAB"]->Fill(vertexPos.Y(), photon_E_LAB);
    histos2D["h_3prong_vertexY_Qvalue_CMS"]->Fill(vertexPos.Y(), Qvalue_CMS);
    profiles1D["h_3prong_vertexX_lenSum_prof"]->Fill(vertexPos.X(), lengthSUM);
    profiles1D["h_3prong_vertexY_lenSum_prof"]->Fill(vertexPos.Y(), lengthSUM);
    profiles1D["h_3prong_vertexZ_lenSum_prof"]->Fill(vertexPos.Z(), lengthSUM);
    profiles1D["h_3prong_vertexY_gamma_E_LAB_prof"]->Fill(vertexPos.Y(), photon_E_LAB);
    profiles1D["h_3prong_vertexY_Qvalue_CMS_prof"]->Fill(vertexPos.Y(), Qvalue_CMS);
    // SPECIAL PLOTS: check dependence of gamma beam energy on vertex position (X_BEAM) horizontal & perpendicular to the gamma beam axis
    histos2D["h_3prong_vertexXBEAM_gamma_E_LAB"]->Fill(vertexPos_BEAM_LAB.X(), photon_E_LAB);
    histos2D["h_3prong_vertexXBEAM_Qvalue_CMS"]->Fill(vertexPos_BEAM_LAB.X(), Qvalue_CMS);
    profiles1D["h_3prong_vertexXBEAM_gamma_E_LAB_prof"]->Fill(vertexPos_BEAM_LAB.X(), photon_E_LAB);
    profiles1D["h_3prong_vertexXBEAM_Qvalue_CMS_prof"]->Fill(vertexPos_BEAM_LAB.X(), Qvalue_CMS);

    // fill symmetrized Dalitz plots
    for(auto i1=0; i1<3; i1++) {

      // triple-alpha coincidence plots => 3 entries per event
      histos2D["h_3prong_Dalitz3_CMS"]->Fill(alpha_T_CMS[i1], carbonExcitationEnergy);
      histos2D["h_3prong_Dalitz4_CMS"]->Fill(alpha_T_CMS[i1], alpha_T_CMS[0]+alpha_T_CMS[1]+alpha_T_CMS[2]);

      auto i2=(i1+1)%3;
      auto i3=(i1+2)%3;

      // consider even and odd permutations of {0,1,2} set => 6 entries per event
      auto mass1=(alphaP4_BEAM_CMS[i1]+alphaP4_BEAM_CMS[i2]).M(); // [MeV/c^2]
      auto mass2=(alphaP4_BEAM_CMS[i1]+alphaP4_BEAM_CMS[i3]).M(); // [MeV/c^2]
      histos2D["h_3prong_Dalitz1_CMS"]->Fill(mass1, mass2);
      histos2D["h_3prong_Dalitz1_CMS"]->Fill(mass2, mass1);

      // consider even and odd permutations of {0,1,2} set => 6 entries per event
      auto eps1=alpha_T_CMS[i1]/Qvalue_CMS;
      auto eps2=alpha_T_CMS[i2]/Qvalue_CMS;
      auto eps3=alpha_T_CMS[i3]/Qvalue_CMS;
      histos2D["h_3prong_Dalitz2_CMS"]->Fill( (eps1+2*eps2-1)/sqrt(3.), // chi
					      eps1-1/3. ); // psi
      histos2D["h_3prong_Dalitz2_CMS"]->Fill( (eps1+2*eps3-1)/sqrt(3.), // chi
					      eps1-1/3. ); // psi
      
    }
  }
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
  outputFile->Write();
}
///////////////////////////////
///////////////////////////////
void HIGGS_analysis::setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr){
  myGeometryPtr = aGeometryPtr;
  if(!myGeometryPtr) {
    std::cout<<KRED<<"HIGS_trees_analysis::setGeometry: "<<RST
	     <<" pointer to TPC geometry not set!"
	     <<std::endl;
    exit(-1);
  }
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
