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
#include "TPCReco/Track3D.h"
#include "TPCReco/CoordinateConverter.h"

class TH1F;
class TH2F;
class Track3D;

class HIGGS_analysis{

 public:
  HIGGS_analysis(std::shared_ptr<GeometryTPC> aGeometryPtr, // definition of LAB detector coordinates
		 float beamEnergy,   // nominal gamma beam energy [keV] in detector LAB frame
		 TVector3 beamDir,   // nominal gamma beam direction in detector LAB frame
		 IonRangeCalculator ionRangeCalculator,
		 CoordinateConverter coordinateConverter);

  ~HIGGS_analysis();

  void fillHistos(Track3D *aTrack);
  
 private:

  void bookHistos();

  void finalize();

  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr);  // definition of detector coordinates in LAB reference frame
  void setBeamProperties(float gammaBeamEnergyInMeV, // nominal gamma beam energy [MeV] in LAB reference
			 TVector3 gammaBeamDir); // nominal gamma beam direction in LAB reference frame and detector coordinate system
  TVector3 getBetaVectorOfCMS(double nucleusMassInMeV); // dimensionless speed (c=1) of gamma-nucleus CMS reference frame wrt LAB reference frame in detector coordinate system
  double getBetaOfCMS(double nucleusMassInMeV); // dimensionless speed (c=1) of gamma-nucleus CMS reference frame wrt LAB reference frame in detector coordinate system

  TFile *outputFile;
  std::map<std::string, TH1F*> histos1D;
  std::map<std::string, TH2F*> histos2D;
  std::map<std::string, TProfile*> profiles1D;
  std::shared_ptr<GeometryTPC> myGeometryPtr; //! transient data member
  IonRangeCalculator myRangeCalculator;
  CoordinateConverter coordinateConverter;
  TVector3 photonUnitVec_DET_LAB; // dimensionless, LAB reference frame, detector coordinate system
  float photonEnergyInMeV_LAB{0};  // MeV
};
#endif
