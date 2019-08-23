#ifndef _TrackBuilder_H_
#define _TrackBuilder_H_

#include <string>
#include <vector>
#include <memory>
#include <tuple>

#include <Fit/Fitter.h>

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
  
  const TrackSegment2D & getSegment2D(int iDir, unsigned int iTrack=0) const;
  
  const TrackSegment3D & getSegment3DSeed() const;

  const TrackSegment3D & getSegment3DFitted() const;

private:

  void makeRecHits(int iDir);
 
  void fillHoughAccumulator(int iDir);

  TrackSegment2DCollection findSegment2DCollection(int iDir);
  
  TrackSegment2D findSegment2D(int iDir, int iPeak) const;
  
  TrackSegment3D buildSegment3D() const;

  
  TrackSegment3D fitTrack3D(const TrackSegment3D & aTrack) const;

    
  EventTPC *myEvent;
  SigClusterTPC myCluster;
  std::shared_ptr<GeometryTPC> myGeometryPtr;

  bool myHistoInitialized;
  int nAccumulatorRhoBins, nAccumulatorPhiBins;

  std::vector<TH2D> myAccumulators;
  std::vector<TH2D> myRecHits;
  std::vector<TrackSegment2DCollection> my2DSeeds;

  TrackSegment2D dummySegment2D;
  
  TrackSegment3D myTrack3DSeed, dummySegment3D;
  TrackSegment3D myTrack3DFitted;

  std::shared_ptr<TF1> timeResponseShape;


  
};
#endif

