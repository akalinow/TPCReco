#ifndef _Track3D_H_
#define _Track3D_H_

#include "TGraph.h"

#include <vector>
#include <execution>

#include "TrackSegment3D.h"
#include "CommonDefinitions.h"

class Track3D{

public:

  Track3D() = default;

  ~Track3D() = default;

  void addSegment(const TrackSegment3D & aSegment3D);

  const TrackSegment3DCollection & getSegments() const { return mySegments;}

  std::vector<double> getSegmentsStartEndXYZ() const;

  double getLength() const { return myLenght;}

  double getSegmentLambda(double lambda) const;

  double getIntegratedCharge(double lambda) const;

  double getIntegratedHitDistance(double lambda) const;

  const TGraph & getChargeProfile() const { return myChargeProfile;}

  const TGraph & getHitDistanceProfile() const { return myHitDistanceProfile;}

  double getChi2() const;

  void splitWorseChi2Segment(double lenghtFraction);

  void extendToWholeChamber();

  ///Shrink track to actual hits range.
  void shrinkToHits();

  void removeEmptySegments();

  double chi2FromNodesList(const double *par);

  double chi2FromSplitPoint(const double *par);

  double getNodeChi2(unsigned int iNode) const;

  void splitSegment(unsigned int iSegment,  double lengthFraction);

private:

  void update();

  void updateChi2();

  void updateNodesChi2(projection strip_dir);

  void updateChargeProfile();

  void updateHitDistanceProfile();

  double getSegmentsChi2() const;

  double getNodesChi2() const;

  double myLenght = 0.0, myChi2 = 0.0;
  std::vector<double> segmentChi2;
  std::vector<double> nodeHitsChi2;
  std::vector<double> nodeAngleChi2;
  
  TrackSegment3DCollection mySegments;
  TGraph myChargeProfile;
  TGraph myHitDistanceProfile;
  
};

using Track3DCollection = std::vector<Track3D>;

std::ostream & operator << (std::ostream &out, const Track3D &aTrack);

#endif

