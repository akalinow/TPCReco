#ifndef _TrackBuilder_H_
#define _TrackBuilder_H_

#include <string>
#include <vector>
#include <memory>
#include <tuple>

#include "TrackSegment2D.h"
#include "TrackSegment3D.h"

class TH2D;
class TF1;

class GeometryTPC;
class EventTPC;

class TrackBuilder {
public:
  
  TrackBuilder();
  
  ~TrackBuilder();

  void setEvent(EventTPC* aEvent);

  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr);

  void reconstruct();

  const SigClusterTPC & getCluster() const { return myCluster;}

  const TH2D & getRecHits2D(int iDir) const;

  const TH2D & getHoughtTransform(int iDir) const;
  
  const TrackSegment2D & getTrackSegment2DSeed(int iDir) const;

  const Track3D & getTrackSegment3DSeed() const;

  const Track3D & getTrack3DFitted() const;

private:

  void makeRecHits(int iDir);
 
  void fillHoughAccumulator(int iDir);

  Track3D findTrack2D(int iDir, int iPeak) const;

  Track3D fitTrack3D(const Track3D & aTrack) const;

  std::tuple<double, double> findTrackStartEnd(const Track3D & aTrack2D, const TH2D  & aHits) const;

  TrackCollection findTrack2DCollection(int iDir);

  Track3D buildTrack3D() const;
    
  EventTPC *myEvent;
  SigClusterTPC myCluster;
  std::shared_ptr<GeometryTPC> myGeometryPtr;

  bool myHistoInitialized;
  int nAccumulatorRhoBins, nAccumulatorPhiBins;

  std::vector<TH2D> myAccumulators;
  std::vector<TH2D> myRecHits;
  std::vector<TrackSegment2DCollection> my2DSeeds;
  
  TrackSegment3D myTrack3DSeed, dummyTrack;
  TrackSegment3D myTrack3DFitted;

  std::shared_ptr<TF1> timeResponseShape;
  
};
#endif

