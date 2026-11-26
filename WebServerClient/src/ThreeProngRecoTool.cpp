#include <algorithm>
#include <cmath>
#include <cstdio>

#include <TVector2.h>

#include "TPCReco/GeometryTPC.h"
#include "TPCReco/CommonDefinitions.h"
#include "TPCReco/ThreeProngRecoTool.h"

using definitions::projection_type;

namespace {
  // track points closer than this in a projection do not define a line there [mm]
  constexpr double minLineLength = 0.5;
  // value for printing with one decimal: no "-0.0"
  double print1(double value){ return std::abs(value)<0.05 ? 0.0 : value; }
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::optional<ThreeProngRecoTool::Vertex>
ThreeProngRecoTool::fitVertex(const std::vector<TrackPoints> & tracks, const PitchVectors & pitch){

  if(tracks.size()<2) return std::nullopt;

  // Every track line in every projection: points (t, s) with n.(t - tA, s - sA) = 0, n = unit
  // normal of the line. The vertex projection is (t, s = X*px + Y*py): the distance from the line
  // is linear in u = (t, X, Y): g.u - c, g = (nt, ns*px, ns*py), c = n.A. Minimise the sum of
  // squared distances: M u = v, M = sum g g^T, v = sum g c.
  double M[3][3] = {{0}}, v[3] = {0};
  struct Line { double g[3], c; };
  std::vector<Line> lines;
  for(const auto & aTrack: tracks){
    for(int iDir=projection_type::DIR_U;iDir<=projection_type::DIR_W;++iDir){
      const Point & A = aTrack[0].at(iDir);
      const Point & B = aTrack[1].at(iDir);
      double dt = B.x - A.x, ds = B.y - A.y;
      double length = std::sqrt(dt*dt + ds*ds);
      if(length<minLineLength) continue;
      double nt = -ds/length, ns = dt/length;
      Line aLine{{nt, ns*pitch[iDir][0], ns*pitch[iDir][1]}, nt*A.x + ns*A.y};
      for(int i=0;i<3;++i){
        for(int j=0;j<3;++j) M[i][j] += aLine.g[i]*aLine.g[j];
        v[i] += aLine.g[i]*aLine.c;
      }
      lines.push_back(aLine);
    }
  }
  if(lines.size()<3) return std::nullopt;

  auto det3 = [](const double a[3][3]){
    return a[0][0]*(a[1][1]*a[2][2]-a[1][2]*a[2][1])
         - a[0][1]*(a[1][0]*a[2][2]-a[1][2]*a[2][0])
         + a[0][2]*(a[1][0]*a[2][1]-a[1][1]*a[2][0]);
  };
  double det = det3(M);
  // the rows of M are sums of unit-scale vectors: a tiny determinant means (nearly) parallel lines
  double scale = M[0][0] + M[1][1] + M[2][2];
  if(!(std::abs(det) > 1e-6*scale*scale*scale)) return std::nullopt;

  double u[3];
  for(int k=0;k<3;++k){ // Cramer's rule
    double Mk[3][3];
    for(int i=0;i<3;++i) for(int j=0;j<3;++j) Mk[i][j] = (j==k) ? v[i] : M[i][j];
    u[k] = det3(Mk)/det;
  }
  double sum2 = 0.0;
  for(const auto & aLine: lines){
    double r = aLine.g[0]*u[0] + aLine.g[1]*u[1] + aLine.g[2]*u[2] - aLine.c;
    sum2 += r*r;
  }
  return Vertex{u[0], u[1], u[2], std::sqrt(sum2/lines.size()), int(lines.size())};
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void ThreeProngRecoTool::clearPoints(){

  myTracks.clear();
  myVertex.reset();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void ThreeProngRecoTool::onAdd(){

  // a new track, unless the last one has not got any point yet
  if(myTracks.empty() || !myTracks.back().empty()) myTracks.emplace_back();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool ThreeProngRecoTool::addPoint(const Point3P & aPoint){

  if(myTracks.empty()) myTracks.emplace_back();
  myTracks.back().push_back(aPoint);
  if(myTracks.back().size()<2) return true; // the second point of the track is needed
  updateVertex();
  return false;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool ThreeProngRecoTool::hasPoints() const {

  for(const auto & item: myTracks) if(!item.empty()) return true;
  return false;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool ThreeProngRecoTool::canFit() const {

  return myVertex && completeTracks().size()>=2;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<ThreeProngRecoTool::TrackPoints> ThreeProngRecoTool::completeTracks() const {

  std::vector<TrackPoints> tracks;
  for(const auto & item: myTracks){
    if(item.size()==2) tracks.push_back({item[0], item[1]});
  }
  return tracks;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void ThreeProngRecoTool::updateVertex(){

  myVertex.reset();
  if(!myGeometry) return;
  for(int iDir=projection_type::DIR_U;iDir<=projection_type::DIR_W;++iDir){
    bool err_flag = false;
    myPitch[iDir][0] = myGeometry->Cartesian2posUVW(TVector2(1.0, 0.0), iDir, err_flag);
    myPitch[iDir][1] = myGeometry->Cartesian2posUVW(TVector2(0.0, 1.0), iDir, err_flag);
    if(err_flag) return;
  }
  myVertex = fitVertex(completeTracks(), myPitch);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
ManualRecoTool::Point ThreeProngRecoTool::vertexProjection(int iDir) const {

  return Point{myVertex->t, myVertex->X*myPitch[iDir][0] + myVertex->Y*myPitch[iDir][1]};
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::optional<ManualRecoTool::Point> ThreeProngRecoTool::getVertex(int iDir) const {

  if(!myVertex || iDir<projection_type::DIR_U || iDir>projection_type::DIR_W) return std::nullopt;
  return vertexProjection(iDir);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int ThreeProngRecoTool::farPointIndex(const TrackPoints & aTrack) const {

  double distance2[2] = {0.0, 0.0};
  for(int iPoint=0;iPoint<2;++iPoint){
    for(int iDir=projection_type::DIR_U;iDir<=projection_type::DIR_W;++iDir){
      Point aVertex = vertexProjection(iDir);
      double dt = aTrack[iPoint].at(iDir).x - aVertex.x;
      double ds = aTrack[iPoint].at(iDir).y - aVertex.y;
      distance2[iPoint] += dt*dt + ds*ds;
    }
  }
  return distance2[1]>distance2[0] ? 1 : 0;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::array<std::vector<ManualRecoTool::Segment>, 3> ThreeProngRecoTool::getSegments() const {

  std::array<std::vector<Segment>, 3> segments;
  const double nan = std::nan("");
  for(size_t iTrack=0;iTrack<myTracks.size();++iTrack){
    const auto & aTrack = myTracks[iTrack];
    int color = 2 + iTrack;
    for(int iDir=projection_type::DIR_U;iDir<=projection_type::DIR_W;++iDir){
      if(aTrack.size()==1){
        const Point & A = aTrack[0].at(iDir);
        segments.at(iDir).push_back({A.x, A.y, nan, nan, color});
      }
      else if(aTrack.size()==2 && myVertex){ // from the vertex to the track end
        Point V = vertexProjection(iDir);
        const Point & E = aTrack[farPointIndex({aTrack[0], aTrack[1]})].at(iDir);
        segments.at(iDir).push_back({V.x, V.y, E.x, E.y, color});
      }
      else if(aTrack.size()==2){ // no vertex yet: the line through the clicked points
        const Point & A = aTrack[0].at(iDir);
        const Point & B = aTrack[1].at(iDir);
        segments.at(iDir).push_back({A.x, A.y, B.x, B.y, color});
      }
    }
  }
  return segments;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<ManualRecoTool::ColoredPoint> ThreeProngRecoTool::getClickedPoints(int iDir) const {

  std::vector<ColoredPoint> points;
  if(iDir<projection_type::DIR_U || iDir>projection_type::DIR_W) return points;
  for(size_t iTrack=0;iTrack<myTracks.size();++iTrack){
    for(const auto & aPoint: myTracks[iTrack]){
      points.push_back({aPoint.at(iDir).x, aPoint.at(iDir).y, int(2+iTrack)});
    }
  }
  return points;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string ThreeProngRecoTool::getHint() const {

  char text[256];
  if(isAcceptingPoints()){
    size_t nPoints = myTracks.empty() ? 0 : myTracks.back().size();
    std::snprintf(text, sizeof(text),
                  "Track %zu: click point %zu of 2 on one U/V/W projection, then the same point on another projection.",
                  std::max<size_t>(myTracks.size(), 1), nPoints+1);
    return text;
  }
  size_t nTracks = completeTracks().size();
  if(nTracks==0) return "";
  if(nTracks<2) return "Add the next track: the vertex needs at least two tracks.";
  if(!myVertex) return "The track lines do not cross in one point (parallel tracks?): add a track or reset the event.";
  std::snprintf(text, sizeof(text),
                "Vertex: t = %.1f mm, X = %.1f mm, Y = %.1f mm (track lines %.1f mm RMS from it). Add a track or fit the segments.",
                print1(myVertex->t), print1(myVertex->X), print1(myVertex->Y), myVertex->rms);
  return text;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<double> ThreeProngRecoTool::makeSegmentsForFit() const {

  std::vector<double> segmentsXY;
  if(!myVertex) return segmentsXY;
  for(const auto & aTrack: completeTracks()){
    const Point3P & anEnd = aTrack[farPointIndex(aTrack)];
    for(int iDir=projection_type::DIR_U;iDir<=projection_type::DIR_W;++iDir){
      Point V = vertexProjection(iDir);
      segmentsXY.insert(segmentsXY.end(), {V.x, V.y, anEnd.at(iDir).x, anEnd.at(iDir).y});
    }
  }
  return segmentsXY;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
