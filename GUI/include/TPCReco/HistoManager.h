#ifndef _HistoManager_H_
#define _HistoManager_H_

#include <string>
#include <vector>
#include <memory>

#include <boost/property_tree/ptree.hpp>

#include <TFile.h>
#include <TLine.h>
#include <TGraph.h>
#include <TH2Poly.h>
#include <RQ_OBJECT.h>

#include "TPCReco/SigClusterTPC.h"
#include "TPCReco/TrackBuilder.h"
//#include "TPCReco/DotFinder.h"
#include "TPCReco/dEdxFitter.h"
#include "TPCReco/RecoOutput.h"
#include "TPCReco/IonRangeCalculator.h"

#include "TPCReco/CommonDefinitions.h"

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

  void setConfig(const boost::property_tree::ptree &aConfig);

  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr);

  void setPressure(double aPressure);

  void openOutputStream(const std::string & filePath);

  void writeRecoData(unsigned long  eventType);

  void toggleAutozoom() { doAutozoom = !doAutozoom;};
 
  void resetEventRateGraph();

  void drawRawHistos(TCanvas *aCanvas, bool isRateDisplayOn);

  void drawRecoHistos(TCanvas *aCanvas);

  void drawDevelHistos(TCanvas *aCanvas);

  void drawTechnicalHistos(TCanvas *aCanvas, int nAgetChips);

  void drawRecoFromMarkers(TCanvas *aCanvas, std::vector<double> * segmentsXY);

  void clearCanvas(TCanvas *aCanvas, bool isLogScaleOn);

  void clearTracks();

  void clearObjects();

  void reconstruct();

  void reconstructSegmentsFromMarkers(std::vector<double> * segmentsXY);

  TGraph* getEventRateGraph();
  
  TH2Poly *getDetectorLayout() const;

  std::shared_ptr<TH1D> get1DProjection(definitions::projection_type projType,
					filter_type filterType,
					scale_type scaleType);

  std::shared_ptr<TH2D> get2DProjection(definitions::projection_type projType,
					filter_type filterType,
					scale_type scaleType);
    
  std::shared_ptr<TH2D> getRecHitStripVsTime(int strip_dir);

  std::shared_ptr<TH1D> getRecHitTimeProjection();

  std::shared_ptr<TH2D> getChannels(int cobo_id, int asad_id);

  const TH2D & getHoughAccumulator(int strip_dir, int iPeak=0);

   /// Move to private
   void createWirePlotDriftCage3D(std::unique_ptr<TPad> &aPad);

   void drawTrack3D(TVirtualPad *aPad);

private:

  void drawTrack2DSeed(int strip_dir, TVirtualPad *aPad);

  void drawTrack3DProjectionTimeStrip(int strip_dir, TVirtualPad *aPad,  bool zoomIn = true);

  void drawTrack3DProjectionXY(TVirtualPad *aPad);

  void drawChargeAlongTrack3D(TVirtualPad *aPad);

  void drawDetLayout();

  void updateEventRateGraph();

  void makeAutozoom(TH1 * aHisto);

  void setDetLayout();
  void setDetLayoutVetoBand(double distance); // [mm]

  boost::property_tree::ptree myConfig;

  IonRangeCalculator myRangeCalculator;
  std::vector<TH2D*> projectionsInCartesianCoords;
  TH3D *h3DReco{0};
  TGraph *grEventRate{0};
  TrackBuilder myTkBuilder;
  RecoOutput myRecoOutput;

  std::shared_ptr<EventTPC> myEventPtr;
  eventraw::EventInfo myEventInfo;
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
};
#endif

