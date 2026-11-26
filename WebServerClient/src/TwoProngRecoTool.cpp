#include <cmath>

#include "TPCReco/CommonDefinitions.h"
#include "TPCReco/TwoProngRecoTool.h"

using definitions::projection_type;

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string TwoProngRecoTool::getHint() const {

  const size_t nPoints = getPoints().size();
  if(isAcceptingPoints()){
    std::string what = nPoints==0 ? "the vertex (start of all segments)"
                                  : "the end of segment "+std::to_string(nPoints);
    return "Click "+what+" on one U/V/W projection, then the same point on another projection.";
  }
  if(isFitEnabled()) return "Add another segment, or fit the segments. Drag a point to correct it.";
  if(nPoints>=2) return "Drag a point to correct it, then fit the segments again.";
  return "";
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::array<std::vector<ManualRecoTool::Segment>, 3> TwoProngRecoTool::getSegments() const {

  // segment i: from the vertex (point 0) to point i
  std::array<std::vector<Segment>, 3> segments;
  const auto & points = getPoints();
  if(points.empty()) return segments;
  for(int iDir=projection_type::DIR_U;iDir<=projection_type::DIR_W;++iDir){
    const Point & V = points.front().p.at(iDir);
    if(points.size()==1){
      segments.at(iDir).push_back({V.x, V.y, std::nan(""), std::nan(""), 2});
      continue;
    }
    for(size_t iPoint=1;iPoint<points.size();++iPoint){
      const Point & E = points.at(iPoint).p.at(iDir);
      segments.at(iDir).push_back({V.x, V.y, E.x, E.y, int(1+iPoint)});
    }
  }
  return segments;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<double> TwoProngRecoTool::makeSegmentsForFit() const {

  std::vector<double> segmentsXY;
  const auto & points = getPoints();
  for(size_t iPoint=1;iPoint<points.size();++iPoint){
    for(int iDir=projection_type::DIR_U;iDir<=projection_type::DIR_W;++iDir){
      const Point & V = points.front().p.at(iDir);
      const Point & E = points.at(iPoint).p.at(iDir);
      segmentsXY.insert(segmentsXY.end(), {V.x, V.y, E.x, E.y});
    }
  }
  return segmentsXY;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
