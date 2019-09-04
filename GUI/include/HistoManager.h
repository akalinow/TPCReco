#ifndef _HistoManager_H_
#define _HistoManager_H_

#include <string>
#include <vector>
#include <memory>

#include "SigClusterTPC.h"
#include "TrackBuilder.h"

#include "TLine.h"
#include "TGraph.h"

#include "CommonDefinitions.h"

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

  std::shared_ptr<TH2D> getCartesianProjection(int aDir);

  std::shared_ptr<TH2D> getFilteredStripVsTime(int aDir);

  std::shared_ptr<TH2D> getRecHitStripVsTime(int aDir);

  TH3D* get3DReconstruction();

  const TH2D & getHoughAccumulator(int aDir, int iPeak=0);

  TLine get2DLine(int aDir, unsigned int iTrackSegment);

private:
    
  EventTPC *myEvent;

  std::vector<TH2D*> projectionsInCartesianCoords;
  TH3D *h3DReco;
  TrackBuilder myTkBuilder;
  
  std::shared_ptr<GeometryTPC> myGeometryPtr;

};
#endif

