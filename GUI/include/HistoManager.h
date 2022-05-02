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
#include "dEdxFitter.h"
#include "RecoOutput.h"

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

  void setRecoClusterParameters(bool recoClusterEnable, double recoClusterThreshold, int recoClusterDeltaStrips, int recoClusterDetlaTimeCells);

  void openOutputStream(const std::string & filePath);

  void writeRecoData(unsigned long  eventType);

  void toggleAutozoom() { doAutozoom = !doAutozoom;};
 
  void resetEventRateGraph();

  void drawRawHistos(TCanvas *aCanvas, bool isRateDisplayOn);

  void drawRecoHistos(TCanvas *aCanvas);

  void drawTechnicalHistos(TCanvas *aCanvas, int nAgetChips);

  void drawRecoFromMarkers(TCanvas *aCanvas, std::vector<double> * segmentsXY);

  void clearCanvas(TCanvas *aCanvas, bool isLogScaleOn);

  void clearTracks();

  std::shared_ptr<TH2D> getRawStripVsTime(int strip_dir);

  std::shared_ptr<TH2D> getClusterStripVsTimeInMM(int strip_dir); 


  void reconstruct();

  void reconstructSegmentsFromMarkers(std::vector<double> * segmentsXY);

  TGraph* getEventRateGraph();
  
  TH2Poly *getDetectorLayout() const;
  
  std::shared_ptr<TH1D> getRawTimeProjection();

  std::shared_ptr<TH1D> getRawTimeProjection(int strip_dir);

  std::shared_ptr<TH1D> getRawTimeProjectionInMM(); 

  std::shared_ptr<TH1D> getRawTimeProjectionInMM(int strip_dir);

  std::shared_ptr<TH1D> getRawStripProjection(int strip_dir);

  std::shared_ptr<TH1D> getRawStripProjectionInMM(int strip_dir);

  std::shared_ptr<TH2D> getRawStripVsTimeInMM(int strip_dir);

  std::shared_ptr<TH2D> getFilteredStripVsTime(int strip_dir);

  std::shared_ptr<TH2D> getRecHitStripVsTime(int strip_dir);

  std::shared_ptr<TH1D> getRecHitTimeProjection();

  std::shared_ptr<TH2D> getChannels(int cobo_id, int asad_id);

  std::shared_ptr<TH1D> getClusterTimeProjection();

  std::shared_ptr<TH1D> getClusterTimeProjectionInMM();

  std::shared_ptr<TH1D> getClusterTimeProjection(int strip_dir);

  std::shared_ptr<TH1D> getClusterTimeProjectionInMM(int strip_dir);

  std::shared_ptr<TH1D> getClusterStripProjection(int strip_dir);

  std::shared_ptr<TH1D> getClusterStripProjectionInMM(int strip_dir); 

  std::shared_ptr<TH2D> getClusterStripVsTime(int strip_dir);

  bool getRecoClusterEnable();
  double getRecoClusterThreshold();
  int getRecoClusterDeltaStrips();
  int getRecoClusterDeltaTimeCells();
  
  // Dot-like events usful for neutron flux monitoring
  void initializeDotFinder(unsigned int hitThr, // unsigned int maxStripsPerDir, unsigned int maxTimecellsPerDir,
			   unsigned int totalChargeThr, 
			   double matchRadiusInMM, const std::string & filePath);

  void runDotFinder();
  void finalizeDotFinder();

  private:

  TH3D* get3DReconstruction();

  TH2D* get2DReconstruction(int strip_dir);

  const TH2D & getHoughAccumulator(int strip_dir, int iPeak=0);

  void drawTrack2DSeed(int strip_dir, TVirtualPad *aPad);

  void drawTrack3D(TVirtualPad *aPad);

  void drawTrack3DProjectionTimeStrip(int strip_dir, TVirtualPad *aPad,  bool zoomIn = true);

  void drawTrack3DProjectionXY(TVirtualPad *aPad);

  void drawChargeAlongTrack3D(TVirtualPad *aPad);

  void drawDetLayout();

  void updateEventRateGraph();

  void makeAutozoom(TH1 * aHisto);

  void setDetLayout();
  void setDetLayoutVetoBand(double distance); // [mm]
      
  std::vector<TH2D*> projectionsInCartesianCoords;
  TH3D *h3DReco{0};
  TGraph *grEventRate{0};
  TrackBuilder myTkBuilder;
  RecoOutput myRecoOutput;
  DotFinder myDotFinder;
  dEdxFitter mydEdxFitter;

  std::shared_ptr<EventTPC> myEventPtr;
  std::shared_ptr<eventraw::EventInfo> myEventInfo;
  std::shared_ptr<GeometryTPC> myGeometryPtr;

  TH2F *hDetLayout{0};        // dummy histogram with optimal XY ranges
  TH2F *hPlotBackground{0};
  TGraph *grDetLayoutAll{0};  // polygon with convex hull of UVW area
  TGraph *grDetLayoutVeto{0}; // similar polygon with excluded outer veto band
  double grVetoBand{10.0};    // width [mm] of exclusion zone around UVW perimeter
  
  std::vector<TObject*> fObjClones;
  std::vector<TObject*> fTrackLines;

  bool doAutozoom{false};
  bool openOutputStreamInitialized{false};

  Long64_t previousEventTime{-1};
  Long64_t previousEventNumber{-1};

  bool recoClusterEnable{true};
  double recoClusterThreshold{35};
  int recoClusterDeltaStrips{2};
  int recoClusterDeltaTimeCells{5};

};
#endif

