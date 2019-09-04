#include "Track3D.h"

#include <iostream>

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void Track3D::addSegment(const TrackSegment3D & aSegment3D){

  mySegments.push_back(aSegment3D);
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void Track3D::initialize(){

  myLenght = 0.0;
  for(auto &aSegment: mySegments){
    myLenght += aSegment.getLength();
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<double> Track3D::getSegmentsStartEndXYZ() const{

  std::vector<double> coordinates;
  if(!mySegments.size()) return coordinates;
  
  for(auto &aSegment: mySegments){
    std::vector<double> segmentCoordinates = aSegment.getStartEndXYZ();
    coordinates.insert(coordinates.end(), segmentCoordinates.begin(), segmentCoordinates.begin()+3);
  }
  std::vector<double> segmentCoordinates = mySegments.back().getStartEndXYZ();
  coordinates.insert(coordinates.end(), segmentCoordinates.begin()+3, segmentCoordinates.end());
  
  return coordinates;  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void Track3D::splitSegment(unsigned int iSegment){

  if(iSegment>=mySegments.size()) return;

  TrackSegment3D & aSegment = mySegments.at(iSegment);
  TVector3 aStep = aSegment.getLength()/2.0*aSegment.getTangent();
  aSegment.setStartEnd(aSegment.getStart(), aSegment.getStart() + aStep);

  TrackSegment3D aNewSegment = mySegments.at(iSegment);
  aNewSegment.setStartEnd(aSegment.getEnd(), aSegment.getEnd() + aStep);

  mySegments.insert(mySegments.begin()+iSegment+1, aNewSegment);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double Track3D::operator() (const double *par) {

  double result = 0.0;

  for(unsigned int iSegment=0;iSegment<mySegments.size();++iSegment){
    const double *segmentParameters = par+3*iSegment;
    result += mySegments.at(iSegment)(segmentParameters);
  }
  return result;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
