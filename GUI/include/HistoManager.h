#ifndef _HistoManager_H_
#define _HistoManager_H_

#include <string>
#include <vector>
#include <memory>

#include "SigClusterTPC.h"

#include "TSpectrum2.h"
#include "TVector2.h"
#include "TLine.h"

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

  TH3D* get3DReconstruction();

  TH2D* getHoughAccumulator(int aDir, int iPeak=0);

  TLine getTrackSeed(int aDir);

  TLine getLineProjection(int aDir);
   
private:
    
  EventTPC *myEvent;
  SigClusterTPC aCluster;
  TH3D *h3DReco;
  
  std::shared_ptr<GeometryTPC> myGeometryPtr;

  TSpectrum2 peakFinder;
  std::vector<TVector2> seedBias, seedTangent;

};
#endif

