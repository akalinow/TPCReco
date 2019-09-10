#ifndef _Track3D_H_
#define _Track3D_H_

#include "TGraph.h"

#include <vector>

#include "TrackSegment3D.h"
#include "CommonDefinitions.h"

class Track3D{

public:

  Track3D(){};

  ~Track3D(){};

  void addSegment(const TrackSegment3D & aSegment3D);

  const TrackSegment3DCollection & getSegments() const { return mySegments;}

  std::vector<double> getSegmentsStartEndXYZ() const;

  double getLength() const { return myLenght;}

  double getSegmentLambda(double lambda) const;

  double getIntegratedCharge(double lambda) const;

  const TGraph & getChargeProfile() const { return myChargeProfile;}

  double getChi2() const;

  void splitWorseChi2Segment(double lenghtFraction);

  void extendToWholeChamber();

  ///Shrink track to actual hits range.
  void shrinkToHits();

  void removeEmptySegments();

  ///Operator needed for fitting.
  double operator() (const double *par);

  double chi2FromNodesList(const double *par);

  double chi2FromSplitPoint(const double *par);
  

private:

  void update();

  void updateChi2();

  void updateChargeProfile();

  double myLenght, myChi2;
  std::vector<double> segmentChi2;
  TrackSegment3DCollection mySegments;
  TGraph myChargeProfile;
  
};

typedef std::vector<Track3D> Track3DCollection;

std::ostream & operator << (std::ostream &out, const Track3D &aTrack);

#endif

