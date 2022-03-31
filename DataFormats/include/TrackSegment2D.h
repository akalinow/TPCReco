#ifndef _TrackSegment2D_H_
#define _TrackSegment2D_H_

#include <string>
#include <vector>
#include <memory>
#include <tuple>

#include "TVector3.h"
#include "TGraphErrors.h"
#include "TH1F.h"

#include "Hit2D.h"
#include "CommonDefinitions.h"

class GeometryTPC;

class TrackSegment2D{

public:

  TrackSegment2D(int strip_dir=DIR_U, std::shared_ptr<GeometryTPC> aGeometryPtr=0);

  ~TrackSegment2D() {};

  void setBiasTangent(const TVector3 & aBias, const TVector3 & aTangent);

  void setStartEnd(const TVector3 & aStart, const TVector3 & aEnd);

  void setRecHits(const Hit2DCollection & aRecHits) {myRecHits = aRecHits;}

  void setNAccumulatorHits(int nHits){nAccumulatorHits = nHits;}

  int getStripDir() const {return myStripDir;}

  int getNAccumulatorHits() const { return nAccumulatorHits;}

  ///Unit tangential vector along segment.
  const TVector3 & getTangent() const { return myTangent;}

  ///Bias vector perpendicular to tangential vector.
  TVector3 getMinBias() const;

  ///Bias vector in the middle of the segment.
  const TVector3 & getBias() const { return myBias;}

   ///Bias vector at the beggining of the segment.
  const TVector3 & getStart() const { return myStart;}

  ///Bias vector at the end of the segment.
  const TVector3 & getEnd() const { return myEnd;}

  ///Bias vector with fixed T.
  TVector3 getBiasAtT(double time) const;

  ///Tangent vector normalised to unit value along time it time component non zero
  ///other wise assume time component is zero, and normalise to 1 along X.
  TVector3 getNormalisedTangent() const;

  double getLength() const { return myLenght;}

  TGraphErrors getChargeProfile(const Hit2DCollection & aRecHits, double radiusCut=4.0);

  double getIntegratedCharge(double lambda, const Hit2DCollection & aRecHits) const;

  ///Rec hits assigned to this projection.
  const Hit2DCollection & getRecHits() const {return myRecHits;}

  ///Return rec hits chi2.
  double getRecHitChi2(const Hit2DCollection & aRecHits) const;

  ///Calculate transverse distance from point to the segment, and doistance along the segment
  std::tuple<double,double> getPointLambdaAndDistance(const TVector3 & aPoint) const;

private:

  ///Calculate vector for different parametrisations.
  void initialize();

  std::shared_ptr<GeometryTPC> myGeometryPtr; //! transient data member

  int myStripDir;
  double myLenght;

  TVector3 myTangent, myBias;
  TVector3 myStart, myEnd;    

  int nAccumulatorHits{0};
  Hit2DCollection  myRecHits;
    
};

std::ostream & operator << (std::ostream &out, const TrackSegment2D &aSegment);

typedef std::vector<TrackSegment2D> TrackSegment2DCollection;

#endif

