#ifndef _HistoManager_H_
#define _HistoManager_H_

#include <string>
#include <vector>
#include <memory>

#include "SigClusterTPC.h"
#include "TrackBuilder.h"

#include "TLine.h"
#include "TGraph.h"

class TH2D;
class TH3D;

class GeometryTPC;
class EventTPC;

class HistoManager {
public:
  
  HistoManager();
  
  ~HistoManager();

  void setEvent(EventTPC* aEvent);

  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr);

  std::shared_ptr<TH2D> getRawStripVsTime(int aDir);

  std::shared_ptr<TH2D> getFilteredStripVsTime(int aDir);

  std::shared_ptr<TH2D> getRecHitStripVsTime(int aDir);

  TH3D* get3DReconstruction();

  const TH2D & getHoughAccumulator(int aDir, int iPeak=0);

  TLine getTrack2D(int aDir, int iTrack);

  TLine getTrack3DProjection(int aDir);

  TH1D getChargeAlong2DTrack(int aDir);
   
private:
    
  EventTPC *myEvent;
  SigClusterTPC aCluster;
  TH3D *h3DReco;
  TrackBuilder myTkBuilder;

  std::vector<int> stripOffset = {-71, 0, -55};  
  //#### Angles of U/V/W unit vectors wrt X-axis [deg]
  //#ANGLES: 90.0 -30.0 30.0
  std::vector<double> phiPitchDirection = {M_PI, -M_PI/6.0 + M_PI/2.0, M_PI/6.0 - M_PI/2.0};
  
  std::shared_ptr<GeometryTPC> myGeometryPtr;

};
#endif

