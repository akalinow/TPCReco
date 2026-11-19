#ifndef _TrackSegment3D_H_
#define _TrackSegment3D_H_

#include <string>
#include <vector>
#include <memory>

#include <TVector3.h>
#include <TH2D.h>

#include "TPCReco/Hit2D.h"
#include "TPCReco/TrackSegment2D.h"
#include "TPCReco/CommonDefinitions.h"

class GeometryTPC;

class TrackSegment3D{

 public:

  TrackSegment3D();

  virtual ~TrackSegment3D(){};

  void setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr);

  std::shared_ptr<GeometryTPC> getGeometry() const;

  void setBias(const TVector3 & aBias);

  void setBiasTangent(const TVector3 & aBias, const TVector3 & aTangent);

  void setBiasTangent(const double *par);

  void setStartEnd(const TVector3 & aStart, const TVector3 & aEnd);

  void setStartEnd(const double *par);

  void setRecHits(const std::vector<TH2D> & aRecHits);

  void setRecHits(const std::vector<Hit2DCollection> & aRecHits) {myRecHits = aRecHits; calculateLoss();}

  void setPID(pid_type aPID){ pid = aPID;}

  void setDiffusion(double aDiffusion){ myDiffusion = aDiffusion;}

  /// The stored projection losses are kept up to date by all setters
  /// (geometry, rec hits, line parameters), so the loss is recalculated
  /// only if the loss type changes.
  void setLossType(definitions::fit_type lossType){
    if(lossType==myLossType) return;
    myLossType = lossType;
    calculateLoss();
  }

  definitions::fit_type getLossType() const { return myLossType;}

  ///Unit tangential vector along segment.
  const TVector3 & getTangent() const { return myTangent;}

  ///Bias vector perpendicular to tangent.
  const TVector3 & getBias() const { return myBias;}

  ///Point of the line with X=0 (the bias if the line is (nearly) parallel to the X=0 plane).
  const TVector3 & getBiasAtX0() const { return myBiasAtX0;}

  ///Point of the line with Y=0 (the bias if the line is (nearly) parallel to the Y=0 plane).
  const TVector3 & getBiasAtY0() const { return myBiasAtY0;}

  ///Point of the line with Z=0 (the bias if the line is (nearly) parallel to the Z=0 plane).
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

  ///Return the diffusion fitted in dE/dx.
  double getDiffusion() const { return myDiffusion;}

  ///Return the particle identification.
  pid_type getPID() const { return pid;}

  ///Return charge profile along the track.
  TH1F getChargeProfile() const;

  ///Return maximum projection width.
  ///The width is a sqrt of variance of hit distances from the segment. 
  ///Projections without hits close to the segment are ignored; 0 if there is none.
  double getMaxProjWidth() const;

  ///Charge of hits along [0, lambda], corrected by the gain map of the geometry (if set).
  double getIntegratedCharge(double lambda) const;

  double getMaxCharge() const;

  const std::vector<Hit2DCollection> & getRecHits() const { return myRecHits;}

  double getLoss(int iProjection=-1) const;
  
  ///Operator needed for fitting.
  double operator() (const double *par);

 private:

  ///Return point on 2D projection for stripPitchDirection corresponding to given lambda.
  TVector3 getPointOn2DProjection(double lambda, const TVector3 & stripPitchDirection) const;

  ///Calculate vector for different parametrization.
  void initialize();

  ///Calculate and store loss for all projections.
  void calculateLoss();

  ///Per-hit gain correction 1/g(X_DET) for 2D projections of [0, lambdaEnd]; empty if no gain map.
  TrackSegment2D::ChargeWeight getGainCorrection(double lambdaEnd) const;

  void addProjection(TH1F &histo, TGraphErrors &graph) const;

  std::shared_ptr<GeometryTPC> myGeometryPtr; //! transient data member
  TVector3 myTangent, myBias;
  TVector3 myBiasAtX0, myBiasAtY0, myBiasAtZ0;
  TVector3 myStart, myEnd;
  double myLenght{0}, myDiffusion{0};
  pid_type pid{pid_type::UNKNOWN};
  definitions::fit_type myLossType{definitions::fit_type::TANGENT_BIAS};

  std::vector<Hit2DCollection> myRecHits;
  std::vector<double> myProjectionsLoss;
};

std::ostream & operator << (std::ostream &out, const TrackSegment3D &aSegment);

typedef std::vector<TrackSegment3D> TrackSegment3DCollection;

#endif
