#include <cmath>
#include <iostream>

#include <TVector2.h>

#include "TPCReco/GeometryTPC.h"
#include "TPCReco/CommonDefinitions.h"
#include "TPCReco/ManualRecoTool.h"

using definitions::projection_type;

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void ManualRecoTool::reset(bool isRecoModeOn){

  myTimeLines.clear();
  resetMarkers(true);
  for(auto & item: mySegments) item.clear();
  if(!isRecoModeOn) addEnabled = fitEnabled = clearEnabled = false;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void ManualRecoTool::resetMarkers(bool force){

  for(auto & item: myMarkers) item.reset();
  myFirstMarker.reset();
  if(force || isLastSegmentComplete(projection_type::DIR_U)){
    acceptPoints = false;
    addEnabled = true;
    clearEnabled = true;
    fitEnabled = !force;
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void ManualRecoTool::addSegment(){

  acceptPoints = true;
  addEnabled = false;
  fitEnabled = false;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::optional<double> ManualRecoTool::getFixedTime() const {

  if(myFirstMarker) return myFirstMarker->x;
  return std::nullopt;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool ManualRecoTool::hasClickedPoints() const {

  for(const auto & item: mySegments) if(!item.empty()) return true;
  for(const auto & item: myMarkers) if(item) return true;
  return false;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool ManualRecoTool::click(int iDir, double x, double y){

  if(!acceptPoints) return false;
  if(iDir<projection_type::DIR_U || iDir>projection_type::DIR_W || myMarkers.at(iDir)) return false;

  // mark the clicked time on the charge vs time panel
  if(!myFirstMarker) myTimeLines.push_back({x, int(myTimeLines.size())+1});

  if(myFirstMarker) x = myFirstMarker->x; // all projections of a point share the time coordinate
  myMarkers.at(iDir) = Point{x, y};

  if(!myFirstMarker){
    myFirstMarker = Point{x, y};
    return true;
  }

  int missingDir = findMissingMarkerDir();
  if(missingDir<0) return true;
  myMarkers.at(missingDir) = Point{x, getMissingYCoordinate(missingDir)};
  for(int strip_dir=projection_type::DIR_U;strip_dir<=projection_type::DIR_W;++strip_dir){
    updateSegments(strip_dir);
  }
  resetMarkers(false);
  return true;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void ManualRecoTool::updateSegments(int iDir){

  std::vector<Segment> & aSegments = mySegments.at(iDir);
  const Point & aMarker = *myMarkers.at(iDir);

  if(aSegments.size() && !isLastSegmentComplete(iDir)){
    aSegments.back().x2 = aMarker.x;
    aSegments.back().y2 = aMarker.y;
    return;
  }
  Segment aSegment{aMarker.x, aMarker.y, std::nan(""), std::nan(""), int(2+aSegments.size())};
  if(aSegments.size()){ // all segments start at the common vertex
    aSegment.x1 = aSegments.front().x1;
    aSegment.y1 = aSegments.front().y1;
    aSegment.x2 = aMarker.x;
    aSegment.y2 = aMarker.y;
  }
  aSegments.push_back(aSegment);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool ManualRecoTool::isLastSegmentComplete(int iDir) const {

  const std::vector<Segment> & aSegments = mySegments.at(iDir);
  return aSegments.empty() ||
    (!std::isnan(aSegments.back().x2) && !std::isnan(aSegments.back().y2));
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int ManualRecoTool::findMissingMarkerDir() const {

  for(int strip_dir=projection_type::DIR_U;strip_dir<=projection_type::DIR_W;++strip_dir){
    if(!myMarkers.at(strip_dir)) return strip_dir;
  }
  return -1;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double ManualRecoTool::getMissingYCoordinate(int missingDir) const {

  if(!myGeometry) return 0.0;
  int dir[2] = {-1, -1};
  double pos[2] = {0.0, 0.0};
  int counter = 0;
  for(int strip_dir=projection_type::DIR_U;strip_dir<=projection_type::DIR_W;++strip_dir){
    if(!myMarkers.at(strip_dir) || strip_dir==missingDir) continue;
    if(counter==2) break;
    dir[counter] = strip_dir;
    pos[counter] = myMarkers.at(strip_dir)->y;
    ++counter;
  }
  TVector2 point;
  if(counter!=2 || !myGeometry->GetUVWCrossPointInMM(dir[0], pos[0], dir[1], pos[1], point)){
    std::cout<<"ManualRecoTool: cannot calculate the intersection point for direction "
             <<missingDir<<std::endl;
    return 0.0;
  }
  bool err_flag = false;
  double strip_posUVW = myGeometry->Cartesian2posUVW(point, missingDir, err_flag);
  return err_flag ? 0.0 : strip_posUVW;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<double> ManualRecoTool::takeSegmentsForFit(){

  std::vector<double> segmentsXY;
  size_t nSegments = mySegments.at(projection_type::DIR_U).size();
  for(const auto & item: mySegments) nSegments = std::min(nSegments, item.size());
  for(size_t iSegment=0;iSegment<nSegments;++iSegment){
    for(const auto & item: mySegments){
      const Segment & aSegment = item.at(iSegment);
      segmentsXY.insert(segmentsXY.end(), {aSegment.x1, aSegment.y1, aSegment.x2, aSegment.y2});
    }
  }
  reset(true);
  return segmentsXY;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
