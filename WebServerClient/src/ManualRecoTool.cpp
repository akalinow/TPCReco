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

  myTimeLines.clear();
  for(auto & item: myMarkers) item.reset();
  myFirstMarker.reset();
  clearPoints();
  acceptPoints = false;
  addEnabled = clearEnabled = isRecoModeOn;
  fitEnabled = false;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void ManualRecoTool::resetMarkers(){

  for(auto & item: myMarkers) item.reset();
  myFirstMarker.reset();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void ManualRecoTool::addSegment(){

  acceptPoints = true;
  addEnabled = false;
  fitEnabled = false;
  onAdd();
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

  if(hasPoints()) return true;
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
  Point3P aPoint{*myMarkers.at(projection_type::DIR_U),
                 *myMarkers.at(projection_type::DIR_V),
                 *myMarkers.at(projection_type::DIR_W)};
  resetMarkers();
  if(!addPoint(aPoint)){ // the segment / track is complete
    acceptPoints = false;
    addEnabled = true;
    clearEnabled = true;
    fitEnabled = canFit();
  }
  return true;
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

  std::vector<double> segmentsXY = makeSegmentsForFit();
  reset(true);
  return segmentsXY;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
