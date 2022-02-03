#ifndef _Track3D_H_
#define _Track3D_H_

#include "TGraph.h"

#include <vector>

#include "TrackSegment3D.h"
#include "CommonDefinitions.h"

class Track3D{

public:

  enum fit_modes{FIT_START_STOP,
		 FIT_BIAS_TANGENT
  };

  Track3D();

  virtual ~Track3D(){};

  void addSegment(const TrackSegment3D & aSegment3D);

  const TrackSegment3DCollection & getSegments() const { return mySegments;}

  std::vector<double> getSegmentsBiasTangentCoords() const;

  std::vector<double> getSegmentsStartEndXYZ() const;

  double getLength() const { return myLenght;}

  double getSegmentLambda(double lambda, unsigned int iSegment) const;

  TGraph getChargeProfile() const { return mySegments.front().getChargeProfile();}

  double getIntegratedCharge(double lambda) const;

  const TGraph & getIntegratedChargeProfile() const { return myIntegratedChargeProfile;}

  //const TGraph & getChargeProfile() const { return myChargeProfile;}

  double getChi2() const;

  void splitWorseChi2Segment(double lenghtFraction);

  void extendToZRange(double zMin, double zMax);

  ///Shrink track to actual hits range.
  void shrinkToHits();

  void removeEmptySegments();

  void enableProjectionForChi2(int iProjection);

  void setFitMode(fit_modes aMode){myFitMode = aMode;}
  
  double chi2FromNodesList(const double *par);

  double chi2FromSplitPoint(const double *par);

  double getNodeHitsChi2(unsigned int iNode) const;

  double getNodeAngleChi2(unsigned int iNode) const;

  void splitSegment(unsigned int iSegment,  double lengthFraction);

private:

  void update();

  void updateChi2();

  void updateNodesChi2(int strip_dir);

  void updateChargeProfile();

  void updateHitDistanceProfile();

  double getSegmentsChi2() const;

  double getNodesChi2() const;

  fit_modes myFitMode{FIT_START_STOP};
  int iProjectionForChi2{-1};
  double myLenght, myChi2;
  double stepLengthAlongTrack{1}; //[mm]

  std::vector<double> segmentChi2;
  std::vector<double> nodeHitsChi2;
  std::vector<double> nodeAngleChi2;
  
  TrackSegment3DCollection mySegments;
  TGraph myChargeProfile, myIntegratedChargeProfile;              
};

typedef std::vector<Track3D> Track3DCollection;

std::ostream & operator << (std::ostream &out, const Track3D &aTrack);

#endif

