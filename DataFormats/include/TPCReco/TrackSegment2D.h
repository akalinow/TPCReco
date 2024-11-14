#ifndef _TrackSegment2D_H_
#define _TrackSegment2D_H_

#include <string>
#include <vector>
#include <memory>
#include <tuple>

#include <TVector3.h>
#include <TGraphErrors.h>
#include <TH1F.h>

#include "TPCReco/Hit2D.h"
#include "TPCReco/CommonDefinitions.h"

class GeometryTPC;

class TrackSegment2D{

public:

  TrackSegment2D(int strip_dir= (int)definitions::projection_type::DIR_U, std::shared_ptr<GeometryTPC> aGeometryPtr=0);

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

  double getLength() const { return myLenght;}

  TGraphErrors getChargeProfile(const Hit2DCollection & aRecHits, double radiusCut=4.0);

  double getIntegratedCharge(double lambda, const Hit2DCollection & aRecHits) const;

  ///Rec hits assigned to this projection.
  const Hit2DCollection & getRecHits() const {return myRecHits;}

  ///Return loss function for the segment.
  /// lossType== TANGENT returns variance of hit distances from the segment.
  /// lossType== BIAS returns sum of hit distances from the segment.
  /// lossType== TANGENT_BIAS returns sum of hit distances from the segment.
  double getLoss(const Hit2DCollection & aRecHits, definitions::fit_type lossType) const;

private:

  ///Calculate vector for different parametrization.
  void initialize();

   ///Return loss function for finding a line parallel to the segment.
  ///The loss function is a variance of hit distances from the segment. 
  double getParallelLineLoss(const Hit2DCollection & aRecHits) const;

  ///Return loss calculated as a sum of distance**2 from the segment.
  double getHitDistanceLoss(const Hit2DCollection & aRecHits) const;

  ///Calculate transverse distance from point to the segment, and doistance along the segment
  std::tuple<double,double> getPointLambdaAndDistance(const TVector3 & aPoint) const;

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

