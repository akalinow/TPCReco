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

  void splitSegment(unsigned int iSegment);

  ///Operator needed for fitting.
  double operator() (const double *par);

private:

  double myLenght;

  TrackSegment3DCollection mySegments;
  
};

typedef std::vector<Track3D> Track3DCollection;

#endif

