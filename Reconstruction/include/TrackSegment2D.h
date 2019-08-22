#ifndef _TrackSegment2D_H_
#define _TrackSegment2D_H_

#include <string>
#include <vector>
#include <memory>

#include "TVector3.h"
#include "TH2D.h"
#include "CommonDefinitions.h"

class TrackSegment2D{

public:

  TrackSegment2D(int strip_dir){ myStripDir = strip_dir};

  ~TrackSegment2D() {};

  void setBiasTangent(const TVector3 & aBias, const TVector3 & aTangent);

  void setStartEnd(const TVector3 & aStart, const TVector3 & aEnd);

  void setRecHits(const TH2D & aRecHits) {myRecHits = aRecHits;}

  ///Bias vector with T=0.
  const TVector3 & getBiasAtT0() const { return myBiasAtT0;}

  ///Bias vector with Wire=0.
  const TVector3 & getBiasAtZ0() const { return myBiasAtWire0;}

  ///Rec hits assigned to this projection.
  const TH2D & getRecHits() const {return myRecHits;}

  ///Return rec hits chi2.
  double getRecHitChi2() const;

private:

  int myStripDir;

  TVector3 myTangent, myBias;
  TVector3 myStart, myEnd;    
  TVector3 myBiasAtT0, myBiasAtWire0;

  TH2D myRecHits;
    
};


typedef std::vector<TrackSegment2D> TrackSegment2DCollection;

#endif

