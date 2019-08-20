#ifndef _TrackBuilder_H_
#define _TrackBuilder_H_

#include <string>
#include <vector>
#include <memory>
#include <tuple>

#include "Track3D.h"

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

  const TH2D & getRecHits2D(int iDir) const;

  const TH2D & getHoughtTransform(int iDir) const;
  
  const Track3D & getTrack2D(int iDir, int iTrack=0) const;

  const Track3D & getTrack3D() const;

  std::tuple<double, double> findTrackStartEndTime(int aDir);



private:

  void makeRecHits(int iDir);
 
  void fillHoughAccumulator(int iDir);

  Track3D findTrack2D(int iDir, int iPeak) const;

  Track3D fitTrack3D(const Track3D & aTrack) const;

  std::tuple<double, double> findTrackStartEnd(const Track3D & aTrack2D, const TH2D  & aHits) const;

  TrackCollection findTrack2DCollection(int iDir);

  Track3D buildTrack3D() const;
    
  EventTPC *myEvent;  
  std::shared_ptr<GeometryTPC> myGeometryPtr;

  bool myHistoInitialized;
  int nAccumulatorRhoBins, nAccumulatorPhiBins;

  std::vector<TH2D> myAccumulators;
  std::vector<TH2D> myRecHits;
  std::vector<TrackCollection> my2DTracks;
  
  Track3D myTrack3DSeed;

  std::shared_ptr<TF1> timeResponseShape;
  
};
#endif

