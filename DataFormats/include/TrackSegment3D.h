#ifndef _TrackSegment3D_H_
#define _TrackSegment3D_H_

#include <string>
#include <vector>
#include <memory>

#include "TVector3.h"
#include "TH2D.h"

#include "Hit2D.h"
#include "TrackSegment2D.h"
#include "CommonDefinitions.h"

class GeometryTPC;

class TrackSegment3D{

 public:

  TrackSegment3D();

  virtual ~TrackSegment3D(){};

  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr);

  std::shared_ptr<GeometryTPC> getGeometry() const;

  void setBiasTangent(const TVector3 & aBias, const TVector3 & aTangent);

  void setBiasTangent(const double *par);

  void setStartEnd(const TVector3 & aStart, const TVector3 & aEnd);

  void setStartEnd(const double *par);

  void setRecHits(const std::vector<TH2D> & aRecHits);

  void setRecHits(const std::vector<Hit2DCollection> & aRecHits) {myRecHits = aRecHits;}

  void setPID(pid_type aPID){ pid = aPID;}

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

  ///Lambda value for given X with current start and stop points
  double getLambdaAtX(double x) const;

  ///Lambda value for given Y with current start and stop points
  double getLambdaAtY(double y) const;

  ///Lambda value for given Z (corresponding to time) with current start and stop points
  double getLambdaAtZ(double z) const;

  ///Bias vector at the beggining of the segment.
  const TVector3 & getStart() const { return myStart;}

  ///Bias vector at the end of the segment.
  const TVector3 & getEnd() const { return myEnd;}

  ///Return packed cartesian coordinates of the segment start/end points.
  std::vector<double> getStartEndXYZ() const;

  ///Return packed coordinates of the segment bias (x, Y, Z) and tangent(Theta, Phi).
  std::vector<double> getBiasTangentCoords() const;

  ///Return 2D projection for stripPitchDirection corresponding to start and end lambdas
  ///along the 3D segment.
  TrackSegment2D get2DProjection(int strip_dir, double start, double end) const;

  ///Return the full lenght of the segment.
  double getLength() const { return myLenght;}

  ///Return the particle identification.
  pid_type getPID() const { return pid;}

  ///Return charge profile along the track.
  TH1F getChargeProfile() const;

  double getIntegratedCharge(double lambda) const;

  const std::vector<Hit2DCollection> & getRecHits() const { return myRecHits;}

  double getRecHitChi2(int iProjection=-1) const;
  
  ///Operator needed for fitting.
  double operator() (const double *par);

 private:

  ///Return point on 2D projection for stripPitchDirection corresponding to given lambda.
  TVector3 getPointOn2DProjection(double lambda, const TVector3 & stripPitchDirection) const;

  ///Calculate vector for different parametrisations.
  void initialize();

  ///Calculate and store chi2 for all projections.
  void calculateRecHitChi2();

  void addProjection(TH1F &histo, TGraphErrors &graph) const;

  std::shared_ptr<GeometryTPC> myGeometryPtr; //! transient data member
  TVector3 myTangent, myBias;
  TVector3 myBiasAtX0, myBiasAtY0, myBiasAtZ0;
  TVector3 myStart, myEnd;
  double myLenght{0};
  pid_type pid{pid_type::UNKNOWN};

  std::vector<Hit2DCollection> myRecHits; //! transient data member
  std::vector<double> myProjectionsChi2;
};

std::ostream & operator << (std::ostream &out, const TrackSegment3D &aSegment);

typedef std::vector<TrackSegment3D> TrackSegment3DCollection;

#endif
