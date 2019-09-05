#ifndef _Track3D_H_
#define _Track3D_H_

#include <vector>

#include "TrackSegment3D.h"
#include "CommonDefinitions.h"

class Track3D{

public:

  Track3D(){};

  ~Track3D(){};

  void initialize();

  void addSegment(const TrackSegment3D & aSegment3D);

  const TrackSegment3DCollection & getSegments() const { return mySegments;}

  std::vector<double> getSegmentsStartEndXYZ() const;

  double getLength() const { return myLenght;}

  double getChi2() const;

  void splitWorseChi2Segment();

  void extend();

  void removeEmptySegments();

  ///Operator needed for fitting.
  double operator() (const double *par);

private:

  void updateChi2();

  double myLenght, myChi2;
  std::vector<double> segmentChi2;
  TrackSegment3DCollection mySegments;
  
};

typedef std::vector<Track3D> Track3DCollection;

std::ostream & operator << (std::ostream &out, const Track3D &aTrack);

#endif

