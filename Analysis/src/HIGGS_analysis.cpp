#include <vector>
#include <iostream>

#include "TH1D.h"
#include "TH2D.h"
#include "TFile.h"
#include "TVector3.h"

#include "GeometryTPC.h"
#include "Track3D.h"
#include "HIGGS_analysis.h"

///////////////////////////////
///////////////////////////////
HIGGS_analysis::HIGGS_analysis(std::shared_ptr<GeometryTPC> aGeometryPtr, // definition of LAB detector coordinates
			       float beamEnergy,   // nominal gamma beam energy [MeV] in detector LAB frame
			       TVector3 beamDir) { // nominal gamma beam direction in detector LAB frame
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

  const std::string categoryPrefix[]={ "all", "1prong", "2prong", "3prong" };
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
  const float maxLengthMM = 200.0; // [mm]
  float xmin, xmax, ymin, ymax, zmin, zmax; // [mm]
  std::tie(xmin, xmax, ymin, ymax) = myGeometryPtr->rangeXY();
  std::tie(zmin, zmax) = myGeometryPtr->rangeZ();
  //  zmin = myGeometryPtr->GetDriftCageZmin();
  //  zmax = myGeometryPtr->GetDriftCageZmax();

  // GLOBAL HISTOGRAMS
  //
  // NTRACKS : ALL event categories
  histos1D["ntracks"] = new TH1F("ntracks","Number of tracks;Tracks per event;Event count", 5, 0, 5);

  // HISTOGRAMS PER CATEGORY
  //
  for(auto c=0U; c<4; ++c) {
    auto prefix=categoryPrefix[c];
    auto info=categoryInfo[c].c_str();
    auto perEventTitle="Event count / bin";
    auto perTrackTitle=(c==0 ? "Track count / bin" : perEventTitle );

    // VERTEX : per category
    histos1D[(prefix+"_vertexX").c_str()] = new TH1F((prefix+"_vertexX").c_str(),
						     Form("%s;Vertex position X_{DET} [mm];%s", info, perEventTitle),
						     (xmax-xmin)/binSizeMM, xmin, xmax);
    histos1D[(prefix+"_vertexY").c_str()] = new TH1F((prefix+"_vertexY").c_str(),
						    Form("%s;Vertex position Y_{DET} [mm];%s", info, perEventTitle),
						    (ymax-ymin)/binSizeMM, ymin, ymax);
    histos1D[(prefix+"_vertexZ").c_str()] = new TH1F((prefix+"_vertexZ").c_str(),
						    Form("%s;Vertex position Z_{DET} [mm];%s", info, perEventTitle),
						    (zmax-zmin)/binSizeMM, zmin, zmax);
    histos2D[(prefix+"_vertexXY").c_str()] = new TH2F((prefix+"_vertexXY").c_str(),
						      Form("%s;Vertex position X_{DET} [mm];Vertex position Y_{DET} [mm];%s", info, perEventTitle),
						      (xmax-xmin)/binSizeMM, xmin, xmax, (ymax-ymin)/binSizeMM, ymin, ymax);
    histos2D[(prefix+"_vertexYZ").c_str()] = new TH2F((prefix+"_vertexYZ").c_str(),
						      Form("%s;Vertex position Y_{DET} [mm];Vertex position Z_{DET} [mm];%s", info, perEventTitle),
						      (ymax-ymin)/binSizeMM, ymin, ymax, (zmax-zmin)/binSizeMM, zmin, zmax);

    // HISTOGRAMS PER TRACK
    for(auto t=0U; t<categoryPIDhname[c].size(); ++t) {
      auto pid=categoryPIDhname[c].at(t);
      auto pidLatex=categoryPIDlatex[c].at(t).c_str();

      // TRACK LENGTH : per category / per track
      histos1D[(prefix+pid+"_length").c_str()] = new TH1F((prefix+pid+"_length").c_str(),
							  Form("%s;%s track length [mm];%s", info, pidLatex, perTrackTitle),
							  maxLengthMM/binSizeMM, 0, maxLengthMM);
      // TRACK DELTA_X/Y/Z : per category / per track
      histos1D[(prefix+pid+"_deltaX").c_str()] = new TH1F((prefix+pid+"_deltaX").c_str(),
							  Form("%s;%s track #DeltaX_{DET} [mm];%s", info, pidLatex, perTrackTitle),
							  maxLengthMM/binSizeMM, -0.5*maxLengthMM, 0.5*maxLengthMM);
      histos1D[(prefix+pid+"_deltaY").c_str()] = new TH1F((prefix+pid+"_deltaY").c_str(),
							  Form("%s;%s track #DeltaY_{DET} [mm];%s", info, pidLatex, perTrackTitle),
							  maxLengthMM/binSizeMM, -0.5*maxLengthMM, 0.5*maxLengthMM);
      histos1D[(prefix+pid+"_deltaZ").c_str()] = new TH1F((prefix+pid+"_deltaZ").c_str(),
							  Form("%s;%s track #DeltaZ_{DET} [mm];%s", info, pidLatex, perTrackTitle),
							  maxLengthMM/binSizeMM, -0.5*maxLengthMM, 0.5*maxLengthMM);
      // TRACK END_X/Y/Z : per category / per track
      histos1D[(prefix+pid+"_endX").c_str()] = new TH1F((prefix+pid+"_endX").c_str(),
							Form("%s;%s track endpoint X_{DET} [mm];%s", info, pidLatex, perTrackTitle),
							(xmax-xmin)/binSizeMM, xmin, xmax);
      histos1D[(prefix+pid+"_endY").c_str()] = new TH1F((prefix+pid+"_endY").c_str(),
							Form("%s;%s track endpoint Y_{DET} [mm];%s", info, pidLatex, perTrackTitle),
							(ymax-ymin)/binSizeMM, ymin, ymax);
      histos1D[(prefix+pid+"_endZ").c_str()] = new TH1F((prefix+pid+"_endZ").c_str(),
							Form("%s;%s track endpoint Z_{DET} [mm];%s", info, pidLatex, perTrackTitle),
							(zmax-zmin)/binSizeMM, zmin, zmax);
      histos2D[(prefix+pid+"_endXY").c_str()] = new TH2F((prefix+pid+"_endXY").c_str(),
							 Form("%s;%s track endpoint X_{DET} [mm];%s track endpoint Y_{DET} [mm];%s", info, pidLatex, pidLatex, perTrackTitle),
							 (xmax-xmin)/binSizeMM, xmin, xmax, (ymax-ymin)/binSizeMM, ymin, ymax);
      // TRACK PHI_DET/THETA_DET/cos(THETA_DET) : per category / per track
      histos1D[(prefix+pid+"_phiDET").c_str()] = new TH1F((prefix+pid+"_phiDET").c_str(),
							  Form("%s;%s track #phi_{DET} [rad];%s", info, pidLatex, perTrackTitle),
							  100, -TMath::Pi(), TMath::Pi());
      histos1D[(prefix+pid+"_thetaDET").c_str()] = new TH1F((prefix+pid+"_thetaDET").c_str(),
							    Form("%s;%s track #theta_{DET} [rad];%s", info, pidLatex, perTrackTitle),
							    100, 0, TMath::Pi());
      histos1D[(prefix+pid+"_cosThetaDET").c_str()] = new TH1F((prefix+pid+"_cosThetaDET").c_str(),
							       Form("%s;%s track cos(#theta_{DET}) [rad];%s", info, pidLatex, perTrackTitle),
							       100, -1, 1);
      // TRACK PHI_BEAM/THETA_BEAM/cos(THETA_BEAM) : per category / per track
      histos1D[(prefix+pid+"_phiBEAM_LAB").c_str()] = new TH1F((prefix+pid+"_phiBEAM_LAB").c_str(),
							       Form("%s;%s track #phi_{BEAM}^{LAB} [rad];%s", info, pidLatex, perTrackTitle),
							       100, -TMath::Pi(), TMath::Pi());
      histos1D[(prefix+pid+"_thetaBEAM_LAB").c_str()] = new TH1F((prefix+pid+"_thetaBEAM_LAB").c_str(),
								 Form("%s;%s track #theta_{BEAM}^{LAB} [rad];%s", info, pidLatex, perTrackTitle),
								 100, 0, TMath::Pi());
      histos1D[(prefix+pid+"_cosThetaBEAM_LAB").c_str()] = new TH1F((prefix+pid+"_cosThetaBEAM_LAB").c_str(),
								    Form("%s;%s track cos(#theta_{BEAM}^{LAB});%s", info, pidLatex, perTrackTitle),
								    100, -1, 1);

      // HISTOGRAMS PER TRACK-TRACK PAIR
      for(auto t2=t+1; t2<categoryPIDhname[c].size(); ++t2) {
	auto pid2=categoryPIDhname[c].at(t2);
	auto pidLatex2=categoryPIDlatex[c].at(t2).c_str();

	// TRACK-TRACK THETA_DET/cos(THETA_DET) : per category / per track pair
	histos1D[(prefix+pid+pid2+"_delta_LAB").c_str()] = new TH1F((prefix+pid+pid2+"_delta_LAB").c_str(),
								   Form("%s;%s %s track pair #delta^{LAB} [rad];%s", info, pidLatex, pidLatex2, perTrackTitle),
								   100, 0, TMath::Pi());
	histos1D[(prefix+pid+pid2+"_cosDelta_LAB").c_str()] = new TH1F((prefix+pid+pid2+"_cosDelta_LAB").c_str(),
								 Form("%s;%s %s track pair cos(#delta^{LAB}) [rad];%s", info, pidLatex, pidLatex2, perTrackTitle),
								 100, -1, 1);

	// special histogram(s) for 2-prong events
	if(categoryPIDhname[c].size()==2) {
	  histos1D[(prefix+pid+pid2+"_lengthSUM").c_str()] = new TH1F((prefix+pid+pid2+"_lengthSUM").c_str(),
								      Form("%s;Sum of %s %s track lengths [mm];%s", info, pidLatex, pidLatex2, perTrackTitle),
								      maxLengthMM/binSizeMM, 0, maxLengthMM);
								      
	}
      }
    }
  }
}
///////////////////////////////
///////////////////////////////
void HIGGS_analysis::fillHistos(Track3D *aTrack){

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
  histos1D["all_vertexX"]->Fill(vertexPos.X());
  histos1D["all_vertexY"]->Fill(vertexPos.Y());
  histos1D["all_vertexZ"]->Fill(vertexPos.Z());
  histos2D["all_vertexXY"]->Fill(vertexPos.X(), vertexPos.Y());
  histos2D["all_vertexYZ"]->Fill(vertexPos.Y(), vertexPos.Z());
  for(auto & track: list) {
    const double len=track.getLength();
    histos1D["all_length"]->Fill(len);
    histos1D["all_deltaX"]->Fill(len*track.getTangent().X());
    histos1D["all_deltaY"]->Fill(len*track.getTangent().Y());
    histos1D["all_deltaZ"]->Fill(len*track.getTangent().Z());
    histos1D["all_endX"]->Fill(track.getEnd().X());
    histos1D["all_endY"]->Fill(track.getEnd().Y());
    histos1D["all_endZ"]->Fill(track.getEnd().Z());
    histos2D["all_endXY"]->Fill(track.getEnd().X(), track.getEnd().Y());
    histos1D["all_phiDET"]->Fill(track.getTangent().Phi());
    histos1D["all_thetaDET"]->Fill(track.getTangent().Theta());
    histos1D["all_cosThetaDET"]->Fill(track.getTangent().CosTheta());
    double phi_BEAM_LAB=atan2(track.getTangent().Z(), -track.getTangent().Y()); // [rad], azimuthal angle from horizontal axis
    double cosTheta_BEAM_LAB=track.getTangent()*gammaUnitVec; // polar angle wrt beam axis
    histos1D["all_phiBEAM_LAB"]->Fill(phi_BEAM_LAB);
    histos1D["all_thetaBEAM_LAB"]->Fill(acos(cosTheta_BEAM_LAB));
    histos1D["all_cosThetaBEAM_LAB"]->Fill(cosTheta_BEAM_LAB);
  }
  
  // 1-prong (alpha)
  if(ntracks==1) {
    histos1D["1prong_vertexX"]->Fill(vertexPos.X());
    histos1D["1prong_vertexY"]->Fill(vertexPos.Y());
    histos1D["1prong_vertexZ"]->Fill(vertexPos.Z());
    histos2D["1prong_vertexXY"]->Fill(vertexPos.X(), vertexPos.Y());
    auto track=list.front();
    const double len=track.getLength(); // [mm]
    histos1D["1prong_alpha_length"]->Fill(len);
    histos1D["1prong_alpha_deltaX"]->Fill(len*track.getTangent().X());
    histos1D["1prong_alpha_deltaY"]->Fill(len*track.getTangent().Y());
    histos1D["1prong_alpha_deltaZ"]->Fill(len*track.getTangent().Z());
    histos1D["1prong_alpha_endX"]->Fill(track.getEnd().X());
    histos1D["1prong_alpha_endY"]->Fill(track.getEnd().Y());
    histos1D["1prong_alpha_endZ"]->Fill(track.getEnd().Z());
    histos2D["1prong_alpha_endXY"]->Fill(track.getEnd().X(), track.getEnd().Y());
    histos1D["1prong_alpha_phiDET"]->Fill(track.getTangent().Phi());
    histos1D["1prong_alpha_thetaDET"]->Fill(track.getTangent().Theta());
    histos1D["1prong_alpha_cosThetaDET"]->Fill(track.getTangent().CosTheta());

    // calculate angles in DET/LAB frame
    double phi_BEAM_LAB=atan2(track.getTangent().Z(), -track.getTangent().Y()); // [rad], azimuthal angle from horizontal axis
    double cosTheta_BEAM_LAB=track.getTangent()*gammaUnitVec; // polar angle wrt beam axis
    histos1D["1prong_alpha_phiBEAM_LAB"]->Fill(phi_BEAM_LAB);
    histos1D["1prong_alpha_thetaBEAM_LAB"]->Fill(acos(cosTheta_BEAM_LAB));
    histos1D["1prong_alpha_cosThetaBEAM_LAB"]->Fill(cosTheta_BEAM_LAB);

    // reconstruct kinetic energy from particle range [mm]
    //    double T_LAB=aRangeCalculator->getIonEnergyMeV(IonRangeCalculator::ALPHA, len);
    //    double p_LAB=sqrt(T_LAB*(T_LAB+2*alphaMass));
    // construct TLorentzVector in DET/LAB frame
    //    TLorentzVector P4_LAB(p_LAB*track.getTangent(), alphaMass+T_LAB);
    // boost P4 from DET/LAB frame to CMS frame (see TLorentzVector::Boost() convention!)
    //    TLorentzVector P4_CMS(P4_DET); // TODO
    //    P4_CMS.Boost(-1.0*beta_DET); // TODO
    //    histos1D["1prong_alpha_E_LAB"]->Fill(T_LAB);
  }
  
  // 2-prong (alpha+carbon)
  if(ntracks==2) {
    histos1D["2prong_vertexX"]->Fill(vertexPos.X());
    histos1D["2prong_vertexY"]->Fill(vertexPos.Y());
    histos1D["2prong_vertexZ"]->Fill(vertexPos.Z());
    histos2D["2prong_vertexXY"]->Fill(vertexPos.X(), vertexPos.Y());

    const double alpha_len = list.front().getLength(); // longest = alpha
    const double carbon_len = list.back().getLength(); // shortest = carbon
    histos1D["2prong_alpha_length"]->Fill(alpha_len);
    histos1D["2prong_alpha_deltaX"]->Fill(alpha_len*list.front().getTangent().X());
    histos1D["2prong_alpha_deltaY"]->Fill(alpha_len*list.front().getTangent().Y());
    histos1D["2prong_alpha_deltaZ"]->Fill(alpha_len*list.front().getTangent().Z());
    histos1D["2prong_alpha_endX"]->Fill(list.front().getEnd().X());
    histos1D["2prong_alpha_endY"]->Fill(list.front().getEnd().Y());
    histos1D["2prong_alpha_endZ"]->Fill(list.front().getEnd().Z());
    histos2D["2prong_alpha_endXY"]->Fill(list.front().getEnd().X(), list.front().getEnd().Y());
    histos1D["2prong_alpha_phiDET"]->Fill(list.front().getTangent().Phi());
    histos1D["2prong_alpha_thetaDET"]->Fill(list.front().getTangent().Theta());
    histos1D["2prong_alpha_cosThetaDET"]->Fill(list.front().getTangent().CosTheta());

    histos1D["2prong_carbon_length"]->Fill(carbon_len);
    histos1D["2prong_carbon_deltaX"]->Fill(carbon_len*list.back().getTangent().X());
    histos1D["2prong_carbon_deltaY"]->Fill(carbon_len*list.back().getTangent().Y());
    histos1D["2prong_carbon_deltaZ"]->Fill(carbon_len*list.back().getTangent().Z());
    histos1D["2prong_carbon_endX"]->Fill(list.back().getEnd().X());
    histos1D["2prong_carbon_endY"]->Fill(list.back().getEnd().Y());
    histos1D["2prong_carbon_endZ"]->Fill(list.back().getEnd().Z());
    histos2D["2prong_carbon_endXY"]->Fill(list.back().getEnd().X(), list.back().getEnd().Y());
    histos1D["2prong_carbon_phiDET"]->Fill(list.back().getTangent().Phi());
    histos1D["2prong_carbon_thetaDET"]->Fill(list.back().getTangent().Theta());
    histos1D["2prong_carbon_cosThetaDET"]->Fill(list.back().getTangent().CosTheta());
    double delta=list.front().getTangent().Angle(list.back().getTangent()); // [rad]
    histos1D["2prong_alpha_carbon_delta_LAB"]->Fill(delta);
    histos1D["2prong_alpha_carbon_cosDelta_LAB"]->Fill(cos(delta));
    histos1D["2prong_alpha_carbon_lengthSUM"]->Fill(alpha_len+carbon_len);

    // calculate angles in DET/LAB frame
    double alpha_phi_BEAM_LAB=atan2(list.front().getTangent().Z(), -list.front().getTangent().Y()); // [rad], azimuthal angle from horizontal axis
    double carbon_phi_BEAM_LAB=atan2(list.back().getTangent().Z(), -list.back().getTangent().Y()); // [rad], azimuthal angle from horizontal axis
    double alpha_cosTheta_BEAM_LAB=list.front().getTangent()*gammaUnitVec; // polar angle wrt beam axis
    double carbon_cosTheta_BEAM_LAB=list.back().getTangent()*gammaUnitVec; // polar angle wrt beam axis
    histos1D["2prong_alpha_phiBEAM_LAB"]->Fill(alpha_phi_BEAM_LAB);
    histos1D["2prong_alpha_thetaBEAM_LAB"]->Fill(acos(alpha_cosTheta_BEAM_LAB));
    histos1D["2prong_alpha_cosThetaBEAM_LAB"]->Fill(alpha_cosTheta_BEAM_LAB);
    histos1D["2prong_carbon_phiBEAM_LAB"]->Fill(carbon_phi_BEAM_LAB);
    histos1D["2prong_carbon_thetaBEAM_LAB"]->Fill(acos(carbon_cosTheta_BEAM_LAB));
    histos1D["2prong_carbon_cosThetaBEAM_LAB"]->Fill(carbon_cosTheta_BEAM_LAB);

    // reconstruct kinetic energy from particle range [mm]
    //    double alpha_T_LAB=aRangeCalculator->getIonEnergyMeV(IonRangeCalculator::ALPHA, alpha_len);
    //    double carbon_T_LAB=aRangeCalculator->getIonEnergyMeV(IonRangeCalculator::CARBON_12, carbon_len);
    //    double alpha_p_LAB=sqrt(alpha_T_LAB*(alpha_T_LAB+2*alphaMass));
    //    double carbon_p_LAB=sqrt(carbon_T_LAB*(carbon_T_LAB+2*carbonMass));

    // construct TLorentzVector in DET/LAB frame
    //    TLorentzVector alphaP4_LAB(alpha_p_LAB*list.front().getTangent(), alphaMass+alpha_T_LAB);
    //    TLorentzVector carbonP4_LAB(p_alpha_LAB*list.back().getTangent(), carbonMass+carbon_T_LAB);
    //    histos1D["2prong_alpha_E_LAB"]->Fill(alpha_T_LAB);
    //    histos1D["2prong_carbon_E_LAB"]->Fill(carbon_T_LAB);
  }
  
  // 3-prong (triple alpha)
  if(ntracks==3) {
    histos1D["3prong_vertexX"]->Fill(vertexPos.X());
    histos1D["3prong_vertexY"]->Fill(vertexPos.Y());
    histos1D["3prong_vertexZ"]->Fill(vertexPos.Z());
    histos2D["3prong_vertexXY"]->Fill(vertexPos.X(), vertexPos.Y());

    for(auto i=0;i<3;i++) {
      auto track=list.at(i);
      const double alpha_len = track.getLength();
      histos1D[Form("3prong_alpha%d_length",i+1)]->Fill(alpha_len);
      histos1D[Form("3prong_alpha%d_deltaX",i+1)]->Fill(alpha_len*track.getTangent().X());
      histos1D[Form("3prong_alpha%d_deltaY",i+1)]->Fill(alpha_len*track.getTangent().Y());
      histos1D[Form("3prong_alpha%d_deltaZ",i+1)]->Fill(alpha_len*track.getTangent().Z());
      histos1D[Form("3prong_alpha%d_endX",i+1)]->Fill(track.getEnd().X());
      histos1D[Form("3prong_alpha%d_endY",i+1)]->Fill(track.getEnd().Y());
      histos1D[Form("3prong_alpha%d_endZ",i+1)]->Fill(track.getEnd().Z());
      histos2D[Form("3prong_alpha%d_endXY",i+1)]->Fill(track.getEnd().X(), track.getEnd().Y());
      histos1D[Form("3prong_alpha%d_phiDET",i+1)]->Fill(track.getTangent().Phi());
      histos1D[Form("3prong_alpha%d_thetaDET",i+1)]->Fill(track.getTangent().Theta());
      histos1D[Form("3prong_alpha%d_cosThetaDET",i+1)]->Fill(track.getTangent().CosTheta());
      
      // reconstruct kinetic energy from particle range [mm]
      //      double alpha_T_LAB=aRangeCalculator->getIonEnergyMeV(IonRangeCalculator::ALPHA, alpha_len);
      //      double alpha_p_LAB=sqrt(alpha_T_LAB*(alpha_T_LAB+2*alphaMass));
      // construct TLorentzVector in DET/LAB frame
      //      TLorentzVector P4_LAB(p_LAB*track.getTangent(), alphaMass+T_LAB);
      // boost P4 from DET/LAB frame to CMS frame (see TLorentzVector::Boost() convention!)
      //      TLorentzVector P4_CMS(P4_DET); // TODO
      //      P4_CMS.Boost(-1.0*beta_DET); // TODO
      //      histos1D[]->Fill(Form("3prong_alpha%d_E_LAB",i+1), T_LAB);
      
      for(auto i2=i+1;i2<3;i2++) {
	double delta=track.getTangent().Angle(list.at(i2).getTangent()); // [rad]
	histos1D[Form("3prong_alpha%d_alpha%d_delta_LAB",i+1,i2+1)]->Fill(delta);
	histos1D[Form("3prong_alpha%d_alpha%d_cosDelta_LAB",i+1,i2+1)]->Fill(cos(delta));
      }
    }

  }
}
///////////////////////////////
///////////////////////////////
void HIGGS_analysis::finalize(){

  outputFile->Write();
}
///////////////////////////////
///////////////////////////////
void HIGGS_analysis::setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr){
  
  myGeometryPtr = aGeometryPtr;
}
///////////////////////////////
///////////////////////////////
void HIGGS_analysis::setBeamProperties(float beamEnergy,   // nominal gamma beam energy [MeV] in detector LAB frame
				       TVector3 beamDir) { // nominal gamma beam direction in detector LAB frame
  gammaEnergy = fabs(beamEnergy);
  gammaUnitVec = beamDir.Unit();
}
