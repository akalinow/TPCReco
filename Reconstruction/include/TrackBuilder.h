#ifndef _TrackBuilder_H_
#define _TrackBuilder_H_

#include <string>
#include <vector>
#include <memory>
#include <tuple>

#include <boost/property_tree/ptree.hpp>

#include <Fit/Fitter.h>

#include "TrackSegment2D.h"
#include "TrackSegment3D.h"
#include "Track3D.h"
#include "RecHitBuilder.h"
#include "dEdxFitter.h"

#include "EventTPC.h"
#include "EventInfo.h"

class TH2D;
class TF1;
class TTree;
class TFile;

class GeometryTPC;
class SigClusterTPC;

class TrackBuilder {
public:
  
  TrackBuilder();
  
  ~TrackBuilder();

  void setEvent(std::shared_ptr<EventTPC> aEvent);

  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr);

  void setPressure(double aPressure);

  void reconstruct();

  const TH2D & getCluster2D(int iDir) const;

  const TH2D & getRecHits2D(int iDir) const;

  const TH1D & getRecHitsTimeProjection() const;

  const TH2D & getHoughtTransform(int iDir) const;
  
  const TrackSegment2D & getSegment2D(int iDir, unsigned int iTrack=0) const;

  void getSegment2DCollectionFromGUI(const std::vector<double> & segmentsXY);
  
  const TrackSegment3D & getSegment3DSeed() const;

  const Track3D & getTrack3D(unsigned int iSegment) const;

  TF1 getdEdx() const {return mydEdxFitter.getFittedModel();};

private:

  void makeRecHits(int iDir);

  void fillHoughAccumulator(int iDir);

  TrackSegment2DCollection findSegment2DCollection(int iDir);
  
  TrackSegment2D findSegment2D(int iDir, int iPeak) const;
  
  TrackSegment3D buildSegment3D(int iTrackSeed=0) const;

  Track3D fitTrack3D(const Track3D & aTrackCandidate);

  Track3D fitEventHypothesis(const Track3D & aTrackCandidate);
  
  Track3D fitTrackNodesStartEnd(const Track3D & aTrack) const;

  ROOT::Fit::FitResult fitTrackNodesBiasTangent(const Track3D & aTrack, double offset=0) const;

  std::tuple<double, double> getTimeProjectionEdges() const;

  std::shared_ptr<EventTPC> myEventPtr;
  std::shared_ptr<GeometryTPC> myGeometryPtr;
  RecHitBuilder myRecHitBuilder;
  dEdxFitter mydEdxFitter;
  double myPressure{190};
  
  std::vector<double> phiPitchDirection;

  bool myHistoInitialized;
  int nAccumulatorRhoBins, nAccumulatorPhiBins;

  TVector3 aHoughOffest;
  std::vector<TH2D> myAccumulators;
  std::vector<TH2D> myRecHits, myRawHits;
  TH1D hTimeProjection;
  std::vector<TrackSegment2DCollection> my2DSeeds;
  std::tuple<double, double> myZRange;

  TrackSegment2D dummySegment2D;
  TrackSegment3D myTrack3DSeed, dummySegment3D;  
  Track3D myTmpTrack, myFittedTrack;
  
  mutable ROOT::Fit::Fitter fitter;
  
};
#endif

