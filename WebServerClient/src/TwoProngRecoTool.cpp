#include <algorithm>
#include <cmath>

#include "TPCReco/CommonDefinitions.h"
#include "TPCReco/TwoProngRecoTool.h"

using definitions::projection_type;

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TwoProngRecoTool::clearPoints(){

  for(auto & item: mySegments) item.clear();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool TwoProngRecoTool::hasPoints() const {

  for(const auto & item: mySegments) if(!item.empty()) return true;
  return false;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool TwoProngRecoTool::canFit() const {

  return !mySegments.at(projection_type::DIR_U).empty() &&
    isLastSegmentComplete(projection_type::DIR_U);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string TwoProngRecoTool::getHint() const {

  if(isAcceptingPoints()){
    std::string what = mySegments.at(projection_type::DIR_U).empty() ?
      "the vertex (start of all segments)" :
      "the end of segment "+std::to_string(mySegments.at(projection_type::DIR_U).size()
                                          +(isLastSegmentComplete(projection_type::DIR_U) ? 1 : 0));
    return "Click "+what+" on one U/V/W projection, then the same point on another projection.";
  }
  if(canFit()) return "Add another segment, or fit the segments.";
  return "";
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool TwoProngRecoTool::addPoint(const Point3P & aPoint){

  for(int iDir=projection_type::DIR_U;iDir<=projection_type::DIR_W;++iDir){
    std::vector<Segment> & aSegments = mySegments.at(iDir);
    const Point & aMarker = aPoint.at(iDir);

    if(aSegments.size() && !isLastSegmentComplete(iDir)){
      aSegments.back().x2 = aMarker.x;
      aSegments.back().y2 = aMarker.y;
      continue;
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
  // the first point (vertex) needs the end of the first segment
  return !isLastSegmentComplete(projection_type::DIR_U);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool TwoProngRecoTool::isLastSegmentComplete(int iDir) const {

  const std::vector<Segment> & aSegments = mySegments.at(iDir);
  return aSegments.empty() ||
    (!std::isnan(aSegments.back().x2) && !std::isnan(aSegments.back().y2));
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<double> TwoProngRecoTool::makeSegmentsForFit() const {

  std::vector<double> segmentsXY;
  size_t nSegments = mySegments.at(projection_type::DIR_U).size();
  for(const auto & item: mySegments) nSegments = std::min(nSegments, item.size());
  for(size_t iSegment=0;iSegment<nSegments;++iSegment){
    for(const auto & item: mySegments){
      const Segment & aSegment = item.at(iSegment);
      if(std::isnan(aSegment.x2) || std::isnan(aSegment.y2)) return segmentsXY;
      segmentsXY.insert(segmentsXY.end(), {aSegment.x1, aSegment.y1, aSegment.x2, aSegment.y2});
    }
  }
  return segmentsXY;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
