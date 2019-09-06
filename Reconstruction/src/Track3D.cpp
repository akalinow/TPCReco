#include "Track3D.h"

#include <iostream>
#include <algorithm>

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void Track3D::addSegment(const TrackSegment3D & aSegment3D){

  mySegments.push_back(aSegment3D);
  updateChi2();
  
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
double Track3D::getChi2() const{

  double chi2 = 0.0;
  std::for_each(segmentChi2.begin(), segmentChi2.end(), [&](auto aItem){chi2 += aItem;});
  return chi2;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void Track3D::splitWorseChi2Segment(){

  updateChi2();
  if(!segmentChi2.size()) return;

  double maxChi2 = 0.0;
  unsigned int worseChi2Segment = 0;
  for(unsigned int iSegment=0;iSegment<mySegments.size();++iSegment){
    if(segmentChi2[iSegment]>maxChi2){
      maxChi2 = segmentChi2[iSegment];
      worseChi2Segment = iSegment;
    }
  }

  TrackSegment3D & aSegment = mySegments.at(worseChi2Segment);
  TVector3 aStep = aSegment.getLength()*aSegment.getTangent();
  aSegment.setStartEnd(aSegment.getStart(), aSegment.getStart() + 0.7*aStep);

  TrackSegment3D aNewSegment = mySegments.at(worseChi2Segment);
  aNewSegment.setStartEnd(aSegment.getEnd(), aSegment.getEnd() + 0.3*aStep);

  mySegments.insert(mySegments.begin()+worseChi2Segment+1, aNewSegment);

  updateChi2();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void Track3D::updateChi2(){

 segmentChi2.clear();
  for(auto aItem: mySegments){
    segmentChi2.push_back(aItem.getRecHitChi2());
  }    
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void Track3D::extend(){

  if(!mySegments.size()) return;

  TrackSegment3D aSegment = mySegments.front();
  TVector3 aStart = aSegment.getStart() - 100*aSegment.getTangent();
  TVector3 aEnd = aSegment.getStart();
  
  aSegment.setStartEnd(aStart, aEnd);
  mySegments.insert(mySegments.begin(), aSegment);

  aSegment = mySegments.back();
  aStart = aSegment.getEnd();
  aEnd = aSegment.getEnd() + 100*aSegment.getTangent();  
  aSegment.setStartEnd(aStart, aEnd);
  mySegments.insert(mySegments.end(), aSegment);

  updateChi2();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void Track3D::removeEmptySegments(){

  auto modifiedEnd = std::remove_if(mySegments.begin(), mySegments.end(), [](auto aItem){
									    std::vector<double> segmentParameters = aItem.getStartEndXYZ();
									    double segmentChi2 = aItem(segmentParameters.data());
									    return segmentChi2<1E-5;
									  });

  unsigned int newSize = modifiedEnd - mySegments.begin();
  mySegments.resize(newSize);

  updateChi2();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double Track3D::operator() (const double *par) {

  double result = 0.0;
  double tmpChi2 = 0.0;
  
  for(unsigned int iSegment=0;iSegment<mySegments.size();++iSegment){
    const double *segmentParameters = par+3*iSegment;
    tmpChi2 = mySegments.at(iSegment)(segmentParameters);
    result += tmpChi2;
  }
  updateChi2();
  
  return result;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::ostream & operator << (std::ostream &out, const Track3D &aTrack){

  out << "number of segments: "<<aTrack.getSegments().size()<<std::endl;
  if(!aTrack.getSegments().size()) return out;

  std::cout<<"\t Path: node [chi2]: ";
  for(auto aSegment: aTrack.getSegments()){
    const TVector3 & start = aSegment.getStart();    
    out<<"("<<start.X()<<", "<<start.Y()<<", "<<start.Z()<<")"
       <<"["<<aSegment.getRecHitChi2()<<"]"
       <<" -> ";
  }  
  const TVector3 & end = aTrack.getSegments().back().getEnd();  
  out<<"("<<end.X()<<", "<<end.Y()<<", "<<end.Z()<<") "<<std::endl;

  std::cout<<"\t Total track chi2: "<<aTrack.getChi2();  
  return out;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
