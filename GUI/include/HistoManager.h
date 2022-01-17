#ifndef _HistoManager_H_
#define _HistoManager_H_

#include <string>
#include <vector>
#include <memory>

#include "TFile.h"
#include "TLine.h"
#include "TGraph.h"
#include "TH2Poly.h"
#include <RQ_OBJECT.h>

#include "SigClusterTPC.h"
#include "TrackBuilder.h"
#include "DotFinder.h"

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
 
  void resetEventRateGraph();

  TGraph* getEventRateGraph();

  TH2Poly *getDetectorLayout() const;
  
  std::shared_ptr<TH1D> getRawTimeProjection();

  std::shared_ptr<TH1D> getRawTimeProjection(int strip_dir);

  std::shared_ptr<TH1D> getRawTimeProjectionInMM(); // added by MC - 4 Aug 2021

  std::shared_ptr<TH1D> getRawTimeProjectionInMM(int strip_dir); // added by MC - 4 Aug 2021

  std::shared_ptr<TH1D> getRawStripProjection(int strip_dir);

  std::shared_ptr<TH1D> getRawStripProjectionInMM(int strip_dir); // added by MC - 4 Aug 2021

  std::shared_ptr<TH2D> getRawStripVsTime(int strip_dir);

  std::shared_ptr<TH2D> getRawStripVsTimeInMM(int strip_dir);

  std::shared_ptr<TH2D> getFilteredStripVsTime(int strip_dir);

  std::shared_ptr<TH2D> getRecHitStripVsTime(int strip_dir);

  std::shared_ptr<TH1D> getRecHitTimeProjection();

  std::shared_ptr<TH2D> getChannels(int cobo_id, int asad_id);

  std::shared_ptr<TH1D> getClusterTimeProjection(); // added by MC - 4 Aug 2021

  std::shared_ptr<TH1D> getClusterTimeProjectionInMM(); // added by MC - 4 Aug 2021

  std::shared_ptr<TH1D> getClusterTimeProjection(int strip_dir); // added by MC - 4 Aug 2021

  std::shared_ptr<TH1D> getClusterTimeProjectionInMM(int strip_dir); // added by MC - 4 Aug 2021

  std::shared_ptr<TH1D> getClusterStripProjection(int strip_dir); // added by MC - 4 Aug 2021

  std::shared_ptr<TH1D> getClusterStripProjectionInMM(int strip_dir); // added by MC - 4 Aug 2021

  std::shared_ptr<TH2D> getClusterStripVsTime(int strip_dir); // added by MC - 4 Aug 2021

  std::shared_ptr<TH2D> getClusterStripVsTimeInMM(int strip_dir); // added by MC - 4 Aug 2021

  TH3D* get3DReconstruction();

  TH2D* get2DReconstruction(int strip_dir);

  const TH2D & getHoughAccumulator(int strip_dir, int iPeak=0);

  void drawTrack2DSeed(int strip_dir, TVirtualPad *aPad);

  void drawTrack3D(TVirtualPad *aPad);

  void drawTrack3DProjectionTimeStrip(int strip_dir, TVirtualPad *aPad,  bool zoomIn = true);

  void drawTrack3DProjectionXY(TVirtualPad *aPad);

  void drawChargeAlongTrack3D(TVirtualPad *aPad);

  void drawHitDistanceAlongTrack3D(TVirtualPad *aPad);

  // Dot-like events usful for neutron flux monitoring
  void initializeDotFinder(unsigned int hitThr, // unsigned int maxStripsPerDir, unsigned int maxTimecellsPerDir,
			   unsigned int totalChargeThr, 
			   double matchRadiusInMM, const std::string & fileName);
  void runDotFinder();
  void finalizeDotFinder();

private:

  void updateEventRateGraph();

  void makeAutozoom(std::shared_ptr<TH2D> & aHisto);
      
  std::vector<TH2D*> projectionsInCartesianCoords;
  TH3D *h3DReco{0};
  TGraph *grEventRate{0};
  TrackBuilder myTkBuilder;
  DotFinder myDotFinder;

  std::shared_ptr<EventTPC> myEvent;
  std::shared_ptr<GeometryTPC> myGeometryPtr;

  bool doAutozoom;

  Long64_t previousEventTime{-1};
  Long64_t previousEventNumber{-1};

};
#endif

