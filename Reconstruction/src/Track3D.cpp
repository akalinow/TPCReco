#include "Track3D.h"

#include <iostream>
#include <algorithm>

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
void Track3D::splitWorseChi2Segment(){

  if(!segmentChi2.size()) return;

  double maxChi2 = 0.0;
  unsigned int worseChi2Segment = -1;
  for(unsigned int iSegment=0;iSegment<mySegments.size();++iSegment){
    std::cout<<__FUNCTION__<<" segment chi2: "<<segmentChi2[iSegment]<<std::endl;
    if(segmentChi2[iSegment]>maxChi2){
      maxChi2 = segmentChi2[iSegment];
      worseChi2Segment = iSegment;
    }
  }

  std::cout<<__FUNCTION__<<" worse chi2 segment: "<<worseChi2Segment<<std::endl;

  TrackSegment3D & aSegment = mySegments.at(worseChi2Segment);
  TVector3 aStep = aSegment.getLength()/2.0*aSegment.getTangent();
  aSegment.setStartEnd(aSegment.getStart(), aSegment.getStart() + aStep);

  TrackSegment3D aNewSegment = mySegments.at(worseChi2Segment);
  aNewSegment.setStartEnd(aSegment.getEnd(), aSegment.getEnd() + aStep);

  mySegments.insert(mySegments.begin()+worseChi2Segment+1, aNewSegment);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void Track3D::removeEmptySegments(){

  auto modifiedEnd = std::remove_if(mySegments.begin(), mySegments.end(), [](auto aItem){
									    std::vector<double> segmentParameters = aItem.getStartEndXYZ();
									    double segmentChi2 = aItem(segmentParameters.data());
									    return segmentChi2<0;
									  });

  unsigned int newSize = modifiedEnd - mySegments.begin();
  mySegments.resize(newSize);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double Track3D::operator() (const double *par) {

  double result = 0.0;
  double tmpChi2 = 0.0;
  segmentChi2.clear();
  
  for(unsigned int iSegment=0;iSegment<mySegments.size();++iSegment){
    const double *segmentParameters = par+3*iSegment;
    tmpChi2 = mySegments.at(iSegment)(segmentParameters);
    segmentChi2.push_back(tmpChi2);
    result += tmpChi2;
  }
  return result;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
