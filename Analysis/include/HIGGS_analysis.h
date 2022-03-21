#ifndef _HIGGS_analysis_H_
#define _HIGGS_analysis_H_

#include <string>
#include <vector>
#include <map>

#include "TH1D.h"
#include "TH2D.h"

class TH1F;
class TH2F;
class Track3D;
class GeometryTPC;

class HIGGS_analysis{

public:

  HIGGS_analysis(std::shared_ptr<GeometryTPC> aGeometryPtr,    // definition of LAB detector coordinates
		 float beamEnergy,                // nominal gamma beam energy [keV] in detector LAB frame
		 TVector3 beamDir);                 // nominal gamma beam direction in detector LAB frame

  ~HIGGS_analysis();

  void fillHistos(Track3D *aTrack);
  
 private:

  void bookHistos();

  void finalize();

  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr);  // definition of LAB detector coordinates
  void setBeamProperties(float beamEnergy, // nominal gamma beam energy [keV] in detector LAB frame
			 TVector3 beamDir); // nominal gamma beam direction in detector LAB frame

  TFile *outputFile;
  std::map<std::string, TH1F*> histos1D;
  std::map<std::string, TH2F*> histos2D;
  std::shared_ptr<GeometryTPC> myGeometryPtr; //! transient data member
  TVector3 gammaUnitVec; // dimensionless, Cartesian detector LAB frame
  float gammaEnergy;  // keV

};

#endif
