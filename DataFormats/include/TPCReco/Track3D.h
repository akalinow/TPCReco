#ifndef _Track3D_H_
#define _Track3D_H_

#include <TGraph.h>

#include <vector>

#include "TPCReco/TrackSegment3D.h"
#include "TPCReco/CommonDefinitions.h"

class Track3D{

 public:

  Track3D();

  virtual ~Track3D(){};

  void addSegment(const TrackSegment3D & aSegment3D);

  const TrackSegment3DCollection & getSegments() const { return mySegments;}
  TrackSegment3DCollection & getSegments() { return mySegments;}

  std::vector<double> getSegmentsBiasTangentCoords() const;

  std::vector<double> getSegmentsStartEndXYZ() const;

  double getLength() const { return myLenght;}

  double getSegmentLambda(double lambda, unsigned int iSegment) const;

  void setChargeProfile(const TH1F &hProfile) {myChargeProfile = hProfile;}

  double getIntegratedCharge(double lambda) const;

  double getMaxCharge() const;

  TH1F getChargeProfile() const { return myChargeProfile;}

  double getLoss() const;

  /// Chamber is modelled as a sphere of radius r.
  void extendToChamberRange(double r);

  /// Shrink track to actual hits range.
  void shrinkToHits();

  void removeEmptySegments();

  void enableProjectionForLoss(int iProjection);

  /// Set the fit mode, i.e. the meaning of the parameters of updateAndGetLoss.
  /// TANGENT_ROTATION: the current segment tangents are the reference
  /// directions, the parameters are small rotations about them.
  void setFitMode(definitions::fit_type fitType);

  void setHypothesisFitLoss(double Loss){ hypothesisFitLoss = Loss;};

  double getHypothesisFitLoss() const {return hypothesisFitLoss;}

  /// Charge gain G_cal [ADC/MeV] of the run (GainTable); 0 if not known.
  void setChargeGain(double aGain){ chargeGain = aGain;}

  double getChargeGain() const { return chargeGain;}

  /// Track charge converted to calibrated kinetic energy [MeV]; 0 if the gain is not known.
  double getChargeEnergy() const;

  /// R = Q/(G_raw E_dep) of the dE/dx fit (E_dep from the alpha and carbon ranges); 0 if not computed.
  void setChargeRatio(double aRatio){ chargeRatio = aRatio;}

  double getChargeRatio() const { return chargeRatio;}

  double hitDistanceFromBias(const double *par);

  double updateAndGetLoss(const double *par);

  void update();

 private:

  void updateLoss();

  double getSegmentsLoss() const;

  ///  Find the lambda of the track that will end on the sphere of radius r.
  std::tuple<double, double> getLambdaOnSphere(const TrackSegment3D & aSegment, double r) const;

  definitions::fit_type myFitType{definitions::fit_type::TANGENT};
  int iProjectionForLoss{-1};
  double myLenght{0}, myLoss{0};
  double stepLengthAlongTrack{0.5}; //[mm]

  std::vector<double> segmentLoss;

  TrackSegment3DCollection mySegments;
  TH1F myChargeProfile;
  double hypothesisFitLoss{0};
  double chargeGain{0};  //! G_cal [ADC/MeV], set by the TrackBuilder
  double chargeRatio{0}; //! R of the dE/dx fit, set by the TrackBuilder
  std::vector<TVector3> myReferenceTangents; //! segment tangents when the TANGENT_ROTATION fit mode was set
};

typedef std::vector<Track3D> Track3DCollection;

std::ostream & operator << (std::ostream &out, const Track3D &aTrack);

#endif
