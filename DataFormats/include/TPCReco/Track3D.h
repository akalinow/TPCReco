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

  TH1F getChargeProfile() const { return myChargeProfile;}

  double getLoss() const;

  /// Chamber is modelled as a sphere of radius r.
  void extendToChamberRange(double r);

  /// Shrink track to actual hits range.
  void shrinkToHits();

  void removeEmptySegments();

  void enableProjectionForLoss(int iProjection);

  void setFitMode(definitions::fit_type fitType);

  void setHypothesisFitLoss(double Loss){ hypothesisFitLoss = Loss;};

  double getHypothesisFitLoss() const {return hypothesisFitLoss;}

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
};

typedef std::vector<Track3D> Track3DCollection;

std::ostream & operator << (std::ostream &out, const Track3D &aTrack);

#endif
