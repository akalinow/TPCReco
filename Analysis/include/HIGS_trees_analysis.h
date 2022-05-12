#ifndef _HIGS_trees_analysis_H_
#define _HIGS_trees_analysis_H_

#include <string>
#include <vector>

#include "TVector3.h"
#include "TFile.h"
#include "TTree.h"

#include "colorText.h"
#include "HIGS_trees_dataFormat.h"
#include "Track3D.h"
#include "EventInfo.h"

class Track3D;
class GeometryTPC;

class HIGS_trees_analysis{

public:

  HIGS_trees_analysis(std::shared_ptr<GeometryTPC> aGeometryPtr, // definition of LAB detector coordinates
		      float beamEnergy,  // nominal gamma beam energy [keV] in detector LAB frame
		      TVector3 beamDir); // nominal gamma beam direction in detector LAB frame
  
  ~HIGS_trees_analysis();
  
  void open();
  
  void close();
  
  void fillTrees(Track3D *aTrack, eventraw::EventInfo *aEventInfo);
  void fillTrees1prong(Track3D *aTrack, eventraw::EventInfo *aEventInfo);
  void fillTrees2prong(Track3D *aTrack, eventraw::EventInfo *aEventInfo);
  void fillTrees3prong(Track3D *aTrack, eventraw::EventInfo *aEventInfo);

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

  std::shared_ptr<GeometryTPC> myGeometryPtr; //! transient data member
  TVector3 gammaUnitVec; // dimensionless, Cartesian detector LAB frame
  float gammaEnergy;  // MeV

};

#endif
