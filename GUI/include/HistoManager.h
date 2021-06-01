#ifndef _HistoManager_H_
#define _HistoManager_H_

#include <string>
#include <vector>
#include <memory>

#include "TLine.h"
#include "TGraph.h"
#include "TH2Poly.h"
#include <RQ_OBJECT.h>

#include "SigClusterTPC.h"
#include "TrackBuilder.h"

#include "CommonDefinitions.h"

class TH2D;
class TH3D;

class GeometryTPC;
class EventTPC;

class HistoManager {

  RQ_OBJECT("HistoManager")

public:
  
  HistoManager();
  
  ~HistoManager();

  void setEvent(EventTPC* aEvent);

  void setEvent(std::shared_ptr<EventTPC> aEvent);

  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr);

  void openOutputStream(const std::string & fileName);

  void writeSegments();

  void toggleAutozoom() { doAutozoom = !doAutozoom;};

  void reconstruct();

  void reconstructSegmentsFromMarkers(std::vector<double> * segmentsXY);
 
  TGraph* getEventRateGraph();

  TH2Poly *getDetectorLayout() const;
  
  std::shared_ptr<TH1D> getRawTimeProjection();

  std::shared_ptr<TH1D> getRawTimeProjection(int strip_dir);

  std::shared_ptr<TH1D> getRawStripProjection(int strip_dir);

  std::shared_ptr<TH2D> getRawStripVsTime(int strip_dir);

  std::shared_ptr<TH2D> getRawStripVsTimeInMM(int strip_dir);

  std::shared_ptr<TH2D> getFilteredStripVsTime(int strip_dir);

  std::shared_ptr<TH2D> getRecHitStripVsTime(int strip_dir);

  std::shared_ptr<TH2D> getChannels(int cobo_id, int asad_id);

  TH3D* get3DReconstruction();

  TH2D* get2DReconstruction(int strip_dir);

  const TH2D & getHoughAccumulator(int strip_dir, int iPeak=0);

  void drawTrack2DSeed(int strip_dir, TVirtualPad *aPad);

  void drawTrack3D(TVirtualPad *aPad);

  void drawTrack3DProjectionTimeStrip(int strip_dir, TVirtualPad *aPad,  bool zoomIn = true);

  void drawTrack3DProjectionXY(TVirtualPad *aPad);

  void drawChargeAlongTrack3D(TVirtualPad *aPad);

private:

  void updateEventRateGraph();

  void makeAutozoom(std::shared_ptr<TH2D> & aHisto);
      
  std::vector<TH2D*> projectionsInCartesianCoords;
  TH3D *h3DReco{0};
  TGraph *grEventRate{0};
  TrackBuilder myTkBuilder;

  std::shared_ptr<EventTPC> myEvent;
  std::shared_ptr<GeometryTPC> myGeometryPtr;

  bool doAutozoom;

  Long64_t previousEventTime{-1};
  Long64_t previousEventNumber{-1};

};
#endif

