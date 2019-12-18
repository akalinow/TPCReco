
#ifndef _TrackBuilder_H_
#define _TrackBuilder_H_

#include <string>
#include <vector>
#include <memory>
#include <tuple>
#include <cstdlib>
#include <iostream>

#include <Fit/Fitter.h>

#include "TVector3.h"
#include "TProfile.h"
#include "TObjArray.h"
#include "TF1.h"
#include "TFitResult.h"
#include "Math/Functor.h"
#include "TH1D.h"

#include "GeometryTPC.h"
#include "EventTPC.h"
#include "SigClusterTPC.h"

#include "TrackSegment2D.h"
#include "TrackSegment3D.h"
#include "Track3D.h"

class TH2D;
class TF1;

class GeometryTPC;
class EventTPC;

class TrackBuilder {
public:
  
  TrackBuilder();
  
  ~TrackBuilder();

  void setEvent(std::shared_ptr<EventTPC> aEvent);

  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr);

  void reconstruct();

  const SigClusterTPC & getCluster() const { return myCluster;}

  const TH2D & getRecHits2D(int iDir) const;

  const TH2D & getHoughtTransform(int iDir) const;
  
  const TrackSegment2D & getSegment2D(int iDir, unsigned int iTrack=0) const;
  
  const TrackSegment3D & getSegment3DSeed() const;

  const Track3D & getTrack3D(unsigned int iSegment) const;

private:

  void makeRecHits(int iDir);

  TF1 fitTimeWindow(TH1D* hProj);
 
  void fillHoughAccumulator(int iDir);

  TrackSegment2DCollection findSegment2DCollection(int iDir);
  
  TrackSegment2D findSegment2D(int iDir, int iPeak) const;
  
  TrackSegment3D buildSegment3D() const;
  
  Track3D fitTrack3D(const TrackSegment3D & aTrackSeedSegment) const;

  Track3D fitTrackNodes(const Track3D & aTrack) const;

  double fitTrackSplitPoint(const Track3D& aTrackCandidate) const;

    
  std::shared_ptr<EventTPC> myEvent;
  SigClusterTPC myCluster;
  std::shared_ptr<GeometryTPC> myGeometryPtr;

  bool myHistoInitialized;
  int nAccumulatorRhoBins, nAccumulatorPhiBins;

  TVector3 aHoughOffest;
  std::vector<TH2D> myAccumulators;
  std::vector<TH2D> myRecHits;
  std::vector<TrackSegment2DCollection> my2DSeeds;

  TrackSegment2D dummySegment2D;
  
  TrackSegment3D myTrack3DSeed, dummySegment3D;

  Track3D myFittedTrack;

  mutable ROOT::Fit::Fitter fitter;
  
};
#endif

