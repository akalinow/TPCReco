#ifndef _HIGGS_analysis_H_
#define _HIGGS_analysis_H_

#include <string>
#include <vector>
#include <map>

#include <TH1F.h>
#include <TH2F.h>
#include <TProfile.h>
#include <TH2Poly.h>
#include <TGraph.h>
#include <TVector2.h>

#include "TPCReco/GeometryTPC.h"
#include "TPCReco/IonRangeCalculator.h"
#include "TPCReco/RequirementsCollection.h"

class TH1F;
class TH2F;
class Track3D;


class HIGGS_analysis{

 public:
  HIGGS_analysis(std::shared_ptr<GeometryTPC> aGeometryPtr, // definition of LAB detector coordinates
		 float beamEnergy,   // nominal gamma beam energy [keV] in detector LAB frame
		 TVector3 beamDir,   // nominal gamma beam direction in detector LAB frame
		 double pressure,    // CO2 pressure [mbar]
		 double temperature);// CO2 temperature [K]

  ~HIGGS_analysis();

  void fillHistos(Track3D *aTrack);
  bool eventFilter(Track3D *aTrack); // 1 = pass, 0 = reject
  
 private:

  void bookHistos();

  void finalize();

  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr);  // definition of detector coordinates in LAB reference frame
  void setCuts(); // set event cuts // TODO - TO BE PARAMETERIZED!!!
  void setIonRangeCalculator(double pressure, double temperature); // CO2 pressure [mbar] and temperature [K]
  void setBeamProperties(float gammaBeamEnergyInMeV, // nominal gamma beam energy [MeV] in LAB reference
			 TVector3 gammaBeamDir); // nominal gamma beam direction in LAB reference frame and detector coordinate system
  TVector3 getBetaVectorOfCMS(double nucleusMassInMeV); // dimensionless speed (c=1) of gamma-nucleus CMS reference frame wrt LAB reference frame in detector coordinate system
  double getBetaOfCMS(double nucleusMassInMeV); // dimensionless speed (c=1) of gamma-nucleus CMS reference frame wrt LAB reference frame in detector coordinate system

  TFile *outputFile;
  //////// DEBUG
  //  TCanvas *outputCanvas; // DEBUG
  //////// DEBUG
  std::map<std::string, TH1F*> histos1D;
  std::map<std::string, TH2F*> histos2D;
  std::map<std::string, TProfile*> profiles1D;
  std::shared_ptr<GeometryTPC> myGeometryPtr; //! transient data member
  IonRangeCalculator myRangeCalculator;
  TVector3 photonUnitVec_DET_LAB; // dimensionless, LAB reference frame, detector coordinate system
  float photonEnergyInMeV_LAB{0};  // MeV
  float beam_slope{0}; // [rad], measured slope: Y_DET(X_DET)=offset+slope*X_DET
  float beam_offset{0}; // [mm], measured offset: Y_DET of beam axis at X_DET=0
  float beam_diameter{0}; // [mm] // TODO - TO BE PARAMETERIZED !!!
  RequirementsCollection<std::function<bool(Track3D*)>> cuts;
};
#endif
