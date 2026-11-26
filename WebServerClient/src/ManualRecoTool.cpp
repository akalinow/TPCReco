#include <cmath>
#include <iostream>

#include <TVector2.h>

#include "TPCReco/GeometryTPC.h"
#include "TPCReco/CommonDefinitions.h"
#include "TPCReco/ManualRecoTool.h"
#include "TPCReco/TwoProngRecoTool.h"
#include "TPCReco/ThreeProngRecoTool.h"

using definitions::projection_type;

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::unique_ptr<ManualRecoTool> ManualRecoTool::create(Mode aMode){

  if(aMode==Mode::ThreeProng) return std::make_unique<ThreeProngRecoTool>();
  return std::make_unique<TwoProngRecoTool>();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string ManualRecoTool::modeName(Mode aMode){

  return aMode==Mode::ThreeProng ? "3prong" : "2prong";
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::optional<ManualRecoTool::Mode> ManualRecoTool::modeFromName(const std::string & name){

  if(name=="2prong") return Mode::TwoProng;
  if(name=="3prong") return Mode::ThreeProng;
  return std::nullopt;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void ManualRecoTool::reset(bool isRecoModeOn){

  myPoints.clear();
  myTimeLines.clear();
  for(auto & item: myMarkers) item.reset();
  myFirstMarker.reset();
  myFirstDir = -1;
  isEnabled = isRecoModeOn;
  acceptPoints = false;
  isChanged = false;
  pointsChanged();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void ManualRecoTool::addSegment(){

  if(!isEnabled) return;
  acceptPoints = true;
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

  if(isChanged && !myPoints.empty()) return true;
  for(const auto & item: myMarkers) if(item) return true;
  return false;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool ManualRecoTool::click(int iDir, double x, double y){

  if(!acceptPoints) return false;
  if(iDir<projection_type::DIR_U || iDir>projection_type::DIR_W || myMarkers.at(iDir)) return false;

  if(!myFirstMarker){ // first projection of a new point
    myTimeLines.push_back({x, int(myTimeLines.size())+1}); // clicked time on the charge vs time panel
    myFirstMarker = Point{x, y};
    myFirstDir = iDir;
    myMarkers.at(iDir) = Point{x, y};
    return true;
  }
  // second projection: the time of the first click, the third projection from the geometry
  x = myFirstMarker->x;
  int missingDir = 3 - myFirstDir - iDir; // U+V+W = 0+1+2
  Point3P aPoint;
  aPoint.at(myFirstDir) = *myFirstMarker;
  aPoint.at(iDir) = Point{x, y};
  aPoint.at(missingDir) = Point{x, getMissingYCoordinate(myFirstDir, myFirstMarker->y, iDir, y, missingDir)};
  myPoints.push_back({aPoint, {myFirstDir, iDir}});

  for(auto & item: myMarkers) item.reset();
  myFirstMarker.reset();
  myFirstDir = -1;
  isChanged = true;
  pointsChanged();
  if(!needsMorePoints()) acceptPoints = false; // the segment / track is complete
  return true;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool ManualRecoTool::movePoint(size_t index, int iDir, double x, double y){

  if(!canMovePoints() || index>=myPoints.size()) return false;
  if(iDir<projection_type::DIR_U || iDir>projection_type::DIR_W) return false;

  ClickedPoint & aPoint = myPoints.at(index);
  // the other projection set by the user keeps its strip coordinate
  int keptDir = aPoint.clickedDirs[0]==iDir ? aPoint.clickedDirs[1] : aPoint.clickedDirs[0];
  if(aPoint.clickedDirs[0]!=iDir && aPoint.clickedDirs[1]!=iDir) keptDir = aPoint.clickedDirs[1];
  int missingDir = 3 - keptDir - iDir;
  double keptY = aPoint.p.at(keptDir).y;
  aPoint.p.at(iDir) = Point{x, y};
  aPoint.p.at(keptDir) = Point{x, keptY};
  aPoint.p.at(missingDir) = Point{x, getMissingYCoordinate(keptDir, keptY, iDir, y, missingDir)};
  aPoint.clickedDirs = {keptDir, iDir};
  if(index<myTimeLines.size()) myTimeLines.at(index).x = x;

  isChanged = true;
  pointsChanged();
  return true;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double ManualRecoTool::getMissingYCoordinate(int dirA, double posA, int dirB, double posB,
                                             int missingDir) const {
  if(!myGeometry) return 0.0;
  TVector2 point;
  if(!myGeometry->GetUVWCrossPointInMM(dirA, posA, dirB, posB, point)){
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

  std::vector<double> segmentsXY = makeSegmentsForFit();
  acceptPoints = false;
  isChanged = false;
  return segmentsXY;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
