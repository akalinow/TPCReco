
#ifndef _TrackSegment3D_H_
#define _TrackSegment3D_H_

#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <numeric>
#include <atomic>
#include <execution>

#include "TVector3.h"
#include "TH2D.h"

#include "Hit2D.h"
#include "TrackSegment2D.h"
#include "CommonDefinitions.h"

class TrackSegment3D {

public:

  TrackSegment3D();

  ~TrackSegment3D() {};

  void setBiasTangent(const TVector3 & aBias, const TVector3 & aTangent);

  void setStartEnd(const TVector3 & aStart, const TVector3 & aEnd);

  void setStartEnd(const double *par);

  void setRecHits(const std::vector<TH2D> & aRecHits);

  ///Unit tangential vector along segment.
  const TVector3 & getTangent() const { return myTangent;}

  ///Bias vector perpendicular to tangent.
  const TVector3 & getBias() const { return myBias;}

  ///Bias vector with X=0.
  const TVector3 & getBiasAtX0() const { return myBiasAtX0;}

  ///Bias vector with Y=0.
  const TVector3 & getBiasAtY0() const { return myBiasAtY0;}

  ///Bias vector with Z=0.
  const TVector3 & getBiasAtZ0() const { return myBiasAtZ0;}

  ///Bias vector at the beggining of the segment.
  const TVector3 & getStart() const { return myStart;}

  ///Bias vector at the end of the segment.
  const TVector3 & getEnd() const { return myEnd;}

  ///Return packed cartesian coordinates of the segment start/end points.
  std::vector<double> getStartEndXYZ() const;

  ///Return 2D projection for strip_dir corresponding to start and end
  ///along the 3D segment.
  TrackSegment2D get2DProjection(projection strip_dir, double start, double end) const;

  ///Return the full lenght of the segment.
  double getLength() const { return myLenght;}

  double getIntegratedCharge(double lambda) const;

  double getIntegratedHitDistance(double lambda) const;

  const auto & getRecHits() const { return myRecHits;}

  double getRecHitChi2() const;
  
  ///Operator needed for fitting.
  double operator() (const double *par);

private:

  TVector3 getPointOn2DProjection(double lambda, projection strip_dir) const;

  ///Calculate vector for different parametrisations.
  void initialize();

  ///Calculate and store chi2 for all projections.
  void calculateRecHitChi2();
 
  TVector3 myTangent, myBias;
  TVector3 myBiasAtX0, myBiasAtY0, myBiasAtZ0;
  TVector3 myStart, myEnd;
  double myLenght;

  std::array<Hit2DCollection, 3> myRecHits;
  std::vector<double> myProjectionsChi2;
  
};

std::ostream & operator << (std::ostream &out, const TrackSegment3D &aSegment);

using TrackSegment3DCollection = std::vector<TrackSegment3D>;

#endif

