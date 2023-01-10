#ifndef _HIGGS_analysis_H_
#define _HIGGS_analysis_H_

#include <string>
#include <vector>
#include <map>

#include "TH1F.h"
#include "TH2F.h"
#include "TProfile.h"
#include "TH2Poly.h"
#include "TGraph.h"
#include "TVector2.h"
//////// DEBUG
//#include "TCanvas.h"
//////// DEBUG

#include "GeometryTPC.h"
#include "IonRangeCalculator.h"

class TH1F;
class TH2F;
class Track3D;
//////// DEBUG
//class TCanvas;
//////// DEBUG

class HIGGS_analysis{

 private:
  //////////////////////////////////////////////////////////////////////////////////////////
  // helper class verifies that:
  // - all XY points are inside UVW active area with required safety margin
  // - to be initialized after TPC geometry, but before starting event processing loop
  class HorizontalCheck{
  public:
    inline HorizontalCheck() { ; }
    inline HorizontalCheck(std::shared_ptr<GeometryTPC> aGeometryPtr, double safetyMargin) {
      initialize(aGeometryPtr, safetyMargin);
    }
    inline void initialize(std::shared_ptr<GeometryTPC> aGeometryPtr, double safetyMargin) {
      if(bin) {
	delete bin;
	bin=0;
      }
      auto g=new TGraph(aGeometryPtr->GetActiveAreaConvexHull(safetyMargin)); // [mm]
      bin=new TH2PolyBin(g, 1);
    }
    inline bool IsInside(double x, double y) { // [mm]
      return bin->IsInside(x, y);
    }
    inline bool IsInside(TVector2 &v) { // [mm]
      return bin->IsInside(v.X(), v.Y());
    }
  private:
    TH2PolyBin* bin={0};
  };

  //////////////////////////////////////////////////////////////////////////////////////////
  // helper class verifies that:
  // - vertical projection length is below physical drift cage length
  // - there is enough room close to pedestal exclusion zone
  // - there is enough room close to end of history buffer
  // - to be initialized after TPC geometry, but before starting event processing loop
  class VerticalCheck{
  public:
    inline VerticalCheck() { ; }
    inline VerticalCheck(std::shared_ptr<GeometryTPC> aGeometryPtr, double lowerSafetyMarginTimecells, double upperSafetyMarginTimecells) {
      initialize(aGeometryPtr, lowerSafetyMarginTimecells, upperSafetyMarginTimecells);
    }
    inline void initialize(std::shared_ptr<GeometryTPC> aGeometryPtr, double lowerSafetyMarginTimecells, double upperSafetyMarginTimecells) {
      auto err=false;
      lengthLimit=0.99*(aGeometryPtr->GetDriftCageZmax()-aGeometryPtr->GetDriftCageZmin());
      lowerLimit=aGeometryPtr->Timecell2pos(fabs(lowerSafetyMarginTimecells), err);
      upperLimit=aGeometryPtr->Timecell2pos(aGeometryPtr->GetAgetNtimecells()-fabs(upperSafetyMarginTimecells), err);
    }
    inline bool IsInside(double z1, double z2) { // [mm]
      return IsInside(z1) && IsInside(z2) && fabs(z2-z1)<lengthLimit;
    }
  private:
    inline bool IsInside(double z) { // [mm]
      return z>lowerLimit && z<upperLimit;
    }
    double lowerLimit{0}; // Z upper limit with safety band [mm]
    double upperLimit{0}; // Z lower limit with safety band [mm]
    double lengthLimit{0}; // Z drift cage length [mm]
  };

  //////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////
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
  HorizontalCheck xyAreaCut;
  VerticalCheck zRangeCut;
};
#endif
