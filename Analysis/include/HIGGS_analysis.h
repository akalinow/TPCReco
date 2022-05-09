#ifndef _HIGGS_analysis_H_
#define _HIGGS_analysis_H_

#include <string>
#include <vector>
#include <map>

#include "TH1F.h"
#include "TH2F.h"
#include "TProfile.h"
//////// DEBUG
//#include "TCanvas.h"
//////// DEBUG

class TH1F;
class TH2F;
class Track3D;
//////// DEBUG
class TCanvas;
//////// DEBUG
class GeometryTPC;

class HIGGS_analysis{

public:

  HIGGS_analysis(std::shared_ptr<GeometryTPC> aGeometryPtr,    // definition of LAB detector coordinates
		 float beamEnergy,                // nominal gamma beam energy [keV] in detector LAB frame
		 TVector3 beamDir);                 // nominal gamma beam direction in detector LAB frame

  ~HIGGS_analysis();

  void fillHistos(Track3D *aTrack);
  bool eventFilter(Track3D *aTrack); // 1 = pass, 0 = reject
  
 private:

  void bookHistos();

  void finalize();

  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr);  // definition of LAB detector coordinates
  void setBeamProperties(float beamEnergy, // nominal gamma beam energy [MeV] in detector LAB frame
			 TVector3 beamDir); // nominal gamma beam direction in detector LAB frame

  TFile *outputFile;
  //////// DEBUG
  //  TCanvas *outputCanvas; // DEBUG
  //////// DEBUG
  std::map<std::string, TH1F*> histos1D;
  std::map<std::string, TH2F*> histos2D;
  std::map<std::string, TProfile*> profiles1D;
  std::shared_ptr<GeometryTPC> myGeometryPtr; //! transient data member
  TVector3 gammaUnitVec; // dimensionless, Cartesian detector LAB frame
  float gammaEnergy;  // MeV

};

#endif
