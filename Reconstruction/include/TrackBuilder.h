#ifndef _TrackBuilder_H_
#define _TrackBuilder_H_

#include <string>
#include <vector>
#include <memory>
#include <tuple>

#include <Fit/Fitter.h>

#include "TrackSegment2D.h"
#include "TrackSegment3D.h"
#include "Track3D.h"
#include "RecHitBuilder.h"

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

  void openOutputStream(const std::string & fileName);
  void closeOutputStream();
  void fillOutputStream();

  void setEvent(EventTPC* aEvent);

  void setEvent(std::shared_ptr<EventTPC> aEvent);

  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr);

  void reconstruct();

  const SigClusterTPC & getCluster() const { return myEvent->GetOneCluster();}

  const TH2D & getCluster2D(int iDir) const;

  const TH2D & getRecHits2D(int iDir) const;

  const TH1D & getRecHitsTimeProjection() const;

  const TH2D & getHoughtTransform(int iDir) const;
  
  const TrackSegment2D & getSegment2D(int iDir, unsigned int iTrack=0) const;

  void getSegment2DCollectionFromGUI(const std::vector<double> & segmentsXY);
  
  const TrackSegment3D & getSegment3DSeed() const;

  const Track3D & getTrack3D(unsigned int iSegment) const;

private:

  void makeRecHits(int iDir);

  void fillHoughAccumulator(int iDir);

  TrackSegment2DCollection findSegment2DCollection(int iDir);
  
  TrackSegment2D findSegment2D(int iDir, int iPeak) const;
  
  TrackSegment3D buildSegment3D(int iTrackSeed=0) const;

  void fitTrack3D(const Track3D & aTrackCandidate);
  
  Track3D fitTrackNodesStartEnd(const Track3D & aTrack) const;

  ROOT::Fit::FitResult fitTrackNodesBiasTangent(const Track3D & aTrack, double offset=0) const;

  std::tuple<double, double> getTimeProjectionEdges() const;
   
  EventTPC *myEvent;
  std::shared_ptr<GeometryTPC> myGeometryPtr;
  RecHitBuilder myRecHitBuilder;
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
  Track3D myFittedTrack;
  Track3D *myFittedTrackPtr;

  
  std::shared_ptr<TFile> myOutputFilePtr;
  std::shared_ptr<TTree> myOutputTreePtr;
  eventraw::EventInfo myEventInfo;

  mutable ROOT::Fit::Fitter fitter;
  
};
#endif

