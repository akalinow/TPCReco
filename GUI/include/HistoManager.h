#ifndef _HistoManager_H_
#define _HistoManager_H_

#include <string>
#include <vector>
#include <memory>

#include "SigClusterTPC.h"
#include "TrackBuilder.h"

#include "TLine.h"
#include "TGraph.h"
#include "TH2Poly.h"

#include "CommonDefinitions.h"

class TH2D;
class TH3D;

class GeometryTPC;
class EventTPC;

class HistoManager {
public:
  
  HistoManager() = default;
  
  ~HistoManager() = default;

  void setEvent(std::shared_ptr<EventTPC> aEvent);

  std::shared_ptr<TH2D> getRawStripVsTime(direction strip_dir);

  std::shared_ptr<TH2D> getCartesianProjection(direction strip_dir);

  std::shared_ptr<TH2D> getRecHitStripVsTime(direction strip_dir);

  Reconstr_hist getReconstruction(bool force);

  std::shared_ptr<TH3D> get3DReconstruction(bool force = false);

  std::shared_ptr<TH2D> get2DReconstruction(bool force = false);

  void drawTrack2DSeed(direction strip_dir, TVirtualPad *aPad);

  void drawTrack3D(TVirtualPad *aPad);

  void drawTrack3DProjectionTimeStrip(direction strip_dir, TVirtualPad *aPad);

  void drawTrack3DProjectionXY(TVirtualPad *aPad);

  void drawChargeAlongTrack3D(TVirtualPad *aPad) const;

private:
    
    std::shared_ptr<EventTPC> myEvent;

  std::vector<TH2D*> directionsInCartesianCoords;
  std::shared_ptr<TH3D> h3DReco;
  TrackBuilder myTkBuilder;

  Reconstr_hist reconstruction;

  bool reconstruction_done = false;

};
#endif

