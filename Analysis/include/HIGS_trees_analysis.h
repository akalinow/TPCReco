#ifndef _HIGS_trees_analysis_H_
#define _HIGS_trees_analysis_H_

#include <string>
#include <vector>

#include "TVector3.h"
#include "TFile.h"
#include "TTree.h"
#include "TH2Poly.h"

#include "colorText.h"
#include "HIGS_trees_dataFormat.h"
#include "Track3D.h"
#include "EventInfo.h"
#include "IonRangeCalculator.h"

class Track3D;
class GeometryTPC;
class HorizontalCheck;
class VerticalCheck;

class HIGS_trees_analysis{

public:

  HIGS_trees_analysis(std::shared_ptr<GeometryTPC> aGeometryPtr, // definition of LAB detector coordinates
		      float beamEnergy, // nominal gamma beam energy [keV] in detector LAB frame
		      TVector3 beamDir, // nominal gamma beam direction in detector LAB frame
		      double pressure); // CO2 pressure [mbar]
  
  ~HIGS_trees_analysis();
  
  void open();
  
  void close();
  
  void fillTrees(Track3D *aTrack, eventraw::EventInfo *aEventInfo);
  void fillTrees1prong(Track3D *aTrack, eventraw::EventInfo *aEventInfo, bool & isFirst);
  void fillTrees2prong(Track3D *aTrack, eventraw::EventInfo *aEventInfo, bool & isFirst);
  void fillTrees3prong(Track3D *aTrack, eventraw::EventInfo *aEventInfo, bool & isFirst);

 private:
     
  Event_1prong *event1prong_ = new Event_1prong;
  Event_2prong *event2prong_ = new Event_2prong;
  Event_3prong *event3prong_ = new Event_3prong;

  std::shared_ptr<TFile> OutputFilePtr;
  std::shared_ptr<TTree> Output1prongTreePtr;
  std::shared_ptr<TTree> Output2prongTreePtr;
  std::shared_ptr<TTree> Output3prongTreePtr;
  
  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr);  // definition of LAB detector coordinates
  void setBeamProperties(float beamEnergy, // nominal gamma beam energy [MeV] in detector LAB frame
			 TVector3 beamDir); // nominal gamma beam direction in detector LAB frame
  void setIonRangeCalculator(double pressure); // CO2 pressure [mbar]

  std::shared_ptr<GeometryTPC> myGeometryPtr; //! transient data member
  TVector3 gammaUnitVec; // dimensionless, Cartesian detector LAB frame
  float gammaEnergy;  // MeV
  IonRangeCalculator myRangeCalculator;

  // set of checks to measure how well event is contained within TPC active volume
  /*
  float driftLength; // physical length of TPC drift cage [mm]
  std::vector<std::pair<float, float> > zRange; // allowed {zmin, zmax} range [mm], for: 2/5/10mm
  */
  std::vector<HorizontalCheck> xyArea;
  std::vector<VerticalCheck> zRange;
};

// helper class
class HorizontalCheck{
 public:
  inline HorizontalCheck(std::shared_ptr<GeometryTPC> aGeometryPtr, double veto) {
    auto g=new TGraph(aGeometryPtr->GetActiveAreaConvexHull(veto));
    bin=new TH2PolyBin(g, 1);
  }
  inline bool IsInside(double x, double y) { // [mm]
    return bin->IsInside(x, y);
  }
  // private:
  TH2PolyBin* bin={0};
};

// helper class
class VerticalCheck{
 public:
  inline VerticalCheck(std::shared_ptr<GeometryTPC> aGeometryPtr, double veto) {
    std::tie(lowerLimit, upperLimit)=aGeometryPtr->rangeZ();
    lengthLimit=upperLimit-lowerLimit;
    lowerLimit+=fabs(veto);
    upperLimit-=fabs(veto);
  }
  inline bool IsInside(double z) { // [mm]
    return z>=lowerLimit && z<=upperLimit;
  }
  inline bool IsInside(double z1, double z2) { // [mm]
    return IsInside(z1) && IsInside(z2) && fabs(z2-z1)<lengthLimit;
  }
  // private:
  double lowerLimit{0}; // Z upper limit with safety band [mm]
  double upperLimit{0}; // Z lower limit with safety band [mm]
  double lengthLimit{0}; // Z drift cage length [mm]
};

#endif
