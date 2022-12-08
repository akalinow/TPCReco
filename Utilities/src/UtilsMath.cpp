#include "UtilsMath.h"
#include <ctime>

////////////////////
////////////////////
//
// calculates intersection of 2D non-parallel edges of finite length
// input:   * offset1   - absolute position of start-point of the 1st edge
//          * tangent1  - relative position of end-point wrt start-point (length of the 1st edge)
//          * offset2   - absolute position of start-point of the 2nd edge
//          * tangent2  - relative position of end-point wrt start-point (length of the 2nd edge)
// output:  returns TRUE upon success and updates variable:
//          * point     - absolute position of resulting intersection point
//
bool Utils::intersectionTwoEdges(TVector2 offset1, TVector2 tangent1,
				       TVector2 offset2, TVector2 tangent2, TVector2 &point) {
  if(!Utils::intersectionTwoLines(offset1, tangent1, offset2, tangent2, point) ||
     (point-offset1).Mod2()+(point-offset1-tangent1).Mod2()>tangent1.Mod2() ||
     (point-offset2).Mod2()+(point-offset2-tangent2).Mod2()>tangent2.Mod2()) return false;
  return true; // point is located between the endpoints of each edge
}

////////////////////
////////////////////
//
// calculates intersection of 2D non-parallel infinte lines
// input:   * offset1   - absolute position of a point on the 1st line
//          * tangent1  - tangent vector of the 1st line (not normalized to 1)
//          * offset2   - absolute position of a point on the 2nd line
//          * tangent2  - tangent vector of the 2nd line (not normalized to 1)
// output:  returns TRUE upon success and updates variable:
//          * point     - absolute position of resulting intersection point
//
bool Utils::intersectionTwoLines(TVector2 offset1, TVector2 tangent1,
				       TVector2 offset2, TVector2 tangent2, TVector2 &point) {

  const TVector2 tangent_vec[2] = { tangent1, tangent2 };
  const TVector2 offset_vec[2] = { offset1, offset2 };

  // sanity check (not parallel AND not empty)
  double W = -tangent_vec[0].X() * tangent_vec[1].Y() + tangent_vec[0].Y() * tangent_vec[1].X();
  if (fabs(W) < Utils::NUMERICAL_TOLERANCE)
    return false;

  const double offset[2] = {offset_vec[1].X() - offset_vec[0].X(),
			    offset_vec[1].Y() - offset_vec[0].Y()};
  double W1 = -offset[0] * tangent_vec[1].Y() + offset[1] * tangent_vec[1].X();
  // double W2 = tangent_vec[0].X() * offset[1] - tangent_vec[0].Y() * offset[0]; // not needed
  double parameter1 = W1 / W;
  //  double parameter2 = W2 / W; // not needed
  point = offset_vec[0] + tangent_vec[0]*parameter1; // postion wrt ORIGIN POINT (0,0)
  return true;
}
////////////////////
////////////////////
//
// calculates all intersections of 2D edge of finite length with 2D closed polygon
// input:   * offset    - absolute position of start-point of the edge
//          * tangent   - relative position of end-point wrt start-point (length of the edge)
//          * poly      - vertices of a closed polygon (first and last point are the same)
// output:  returns TRUE upons success and updates variables:
//          * point_vec - resulting list of intersection points
//          * index_vec - resulting list of polygon edge numbers (from 0 to N-1)
//
bool Utils::intersectionEdgePolygon(TVector2 offset, TVector2 tangent, TGraph poly,
					std::vector<TVector2> &point_vec,
					std::vector<int> &index_vec) {
  point_vec.resize(0);
  index_vec.resize(0);
  TVector2 p;
  for (auto i=0; i<poly.GetN()-1; i++) { // skip very last (duplicated) point that closes the loop
    const TVector2 start(poly.GetX()[i], poly.GetY()[i]);
    const TVector2 end(poly.GetX()[i+1], poly.GetY()[i+1]);
    if(Utils::intersectionTwoEdges(offset, tangent, start, start-end, p)) {
      point_vec.push_back(p);
      index_vec.push_back(i);
    }
  }
  return (index_vec.size()>0);
}
////////////////////
////////////////////
//
// calculates intersection of 3D infinite line with infinite plane
// input:   * offset    - absolute position of a point on the line
//          * tangent   - tangent vector of the line (not normalized to 1)
//          * basepoint - absolute position of a point on the plane
//          * span1,2   - two non-parallel vectors that define orientation of the plane
// output:  returns TRUE upon success and updates variables:
//          * point     - absolute position of resulting intersection point
//          * t1,2      - coefficients of linear combination (point-basepoint)=t1*span1+t2*span2
// NOTE: if line is embedded in the plane the specified offset point will be returned
//
////////////////////
bool Utils::intersectionLinePlane(TVector3 offset, TVector3 tangent,
				  TVector3 basepoint, TVector3 span1, TVector3 span2,
				  TVector3 &point, double &t1, double &t2) {

  // Intersection point fulfills:  offset + k3*tangent = basepoint + k1*span1 + k2*span2
  // Equivalent form:              k1*span1 + k2*span2 - k3*tangent = offset-basepoint
  // Equations set (1):            k1*A + k2*B + (-k3)*C = D
  const TVector3 A=span1, B=span2, C=tangent, D=offset-basepoint;
  const double W=A.X()*B.Y()*C.Z()+B.X()*C.Y()*A.Z()+C.X()*A.Y()*B.Z()-
    A.X()*C.Y()*B.Z()-B.X()*A.Y()*C.Z()-C.X()*B.Y()*A.Z();
  const double W1=D.X()*B.Y()*C.Z()+B.X()*C.Y()*D.Z()+C.X()*D.Y()*B.Z()-
    D.X()*C.Y()*B.Z()-B.X()*D.Y()*C.Z()-C.X()*B.Y()*D.Z();
  const double W2=A.X()*D.Y()*C.Z()+D.X()*C.Y()*A.Z()+C.X()*A.Y()*D.Z()-
    A.X()*C.Y()*D.Z()-D.X()*A.Y()*C.Z()-C.X()*D.Y()*A.Z();
  const double W3=A.X()*B.Y()*D.Z()+B.X()*D.Y()*A.Z()+D.X()*A.Y()*B.Z()-
    A.X()*D.Y()*B.Z()-B.X()*A.Y()*D.Z()-D.X()*B.Y()*A.Z();

  // sanity check
  if(fabs(W)<Utils::NUMERICAL_TOLERANCE) {
    if(fabs(W1)<Utils::NUMERICAL_TOLERANCE &&
       fabs(W2)<Utils::NUMERICAL_TOLERANCE &&
       fabs(W3)<Utils::NUMERICAL_TOLERANCE) {
      // Assuming that point=offset: r:=(point-basepoint) and t1*span1+t2*span2=r
      //                             t1 + t2*(span1*span2) = r*span1
      //                             t1*(span1*span2) + t2 = r*span2
      // Equation set (2):           t1*AA + t2*BB = CC
      const double dot=span1*span2;
      const TVector2 AA(1, dot), BB(dot, 1), CC(D*span1, D*span2);
      const double WW=AA.X()*BB.Y()-AA.Y()*BB.X();
      const double WW1=CC.X()*BB.Y()-CC.Y()*BB.X();
      const double WW2=AA.X()*CC.Y()-AA.Y()*CC.X();
      if(fabs(WW)<Utils::NUMERICAL_TOLERANCE) {
	return false;
      }
      point=offset;
      t1 = WW1/WW;
      t2 = WW2/WW;
      return true;
    }
    return false;
  }
  point = offset - W3/W * tangent; // see equation (1) for sign convention
  t1 = W1/W;
  t2 = W2/W;
  return true;
}
////////////////////
////////////////////
//
// calculates intersection of 3D infinite line with infinite plane
// input:   * offset    - absolute position of a point on the line
//          * tangent   - tangent vector of the line (not normalized to 1)
//          * basepoint - absolute position of a point on the plane
//          * span1,2   - two non-parallel vectors that define orientation of the plane
// output:  returns TRUE upon success and updates variable:
//          * point     - absolute position of resulting intersection point
// NOTE: if line is embedded in the plane the specified offset point will be returned
//
////////////////////
bool Utils::intersectionLinePlane(TVector3 offset, TVector3 tangent,
				      TVector3 basepoint, TVector3 span1, TVector3 span2,
				      TVector3 &point) {
  double t1, t2; // dummy results
  return Utils::intersectionLinePlane(offset, tangent, basepoint, span1, span2, point, t1, t2);
}
////////////////////
////////////////////
//
// calculates intersection of 3D edge of finite length with infinite plane
// inputs:  * offset    - absolute position of start-point of the edge
//          * tangent   - relative position of end-point wrt start-point (length of the edge)
//          * basepoint - absolute position of a point on the plane
//          * span1,2   - two non-parallel vectors that define orientation of the plane
// output:  return TRUE upon success and updated variable:
//          * point     - absolute position of resulting intersection point
//          * t1,2      - coefficients of linear combination (point-basepoint)=t1*span1+t2*span2
// NOTE: if edge is embedded in the plane its start-point will be returned
//
bool Utils::intersectionEdgePlane(TVector3 offset, TVector3 tangent,
				  TVector3 basepoint, TVector3 span1, TVector3 span2,
				  TVector3 &point, double &t1, double &t2) {
  // check angle between plane's normal angle and relative position
  // of start/end points wrt basepoint
  auto normal = (span1.Cross(span2)).Unit();
  auto dot1 = (offset-basepoint)*normal; // proportional to cos(V1, VB)
  auto dot2 = (offset+tangent-basepoint)*normal; // proportional to cos(V2, VB)
  TVector3 result=point; // no change
  auto found=false;
  if( dot1*dot2 < 0.0 ) { // < -Utils::NUMERICAL_TOLERANCE ) { // find intersection point
    found=Utils::intersectionLinePlane(offset, tangent, basepoint, span1, span2, point, t1, t2);
    return found;
  } else if( fabs(dot1)<Utils::NUMERICAL_TOLERANCE ) { // start-point will do
    result=offset; // calculate t1, t2 later
    found=true;
  } else if( fabs(dot2)<Utils::NUMERICAL_TOLERANCE ) { // end-point will do
    result=offset+tangent; // calculate t1, t2 later
    found=true;
  }
  if(found) {
    // Assuming that point=result: r:=(result-basepoint) and t1*span1+t2*span2=r
    //                             t1 + t2*(span1*span2) = r*span1
    //                             t1*(span1*span2) + t2 = r*span2
    // Equation set (1):           t1*A + t2*B = C
    const double dot=span1*span2;
    const TVector3 D=result-basepoint;
    const TVector2 A(1, dot), B(dot, 1), C(D*span1, D*span2);
    const double W=A.X()*B.Y()-A.Y()*B.X();
    const double W1=C.X()*B.Y()-C.Y()*B.X();
    const double W2=A.X()*C.Y()-A.Y()*C.X();
    if(fabs(W)<Utils::NUMERICAL_TOLERANCE) {
      return false;
    }
    point=result;
    t1 = W1/W;
    t2 = W2/W;
    return true;
  }
  // no solution
  return false;
}
////////////////////
////////////////////
//
// calculates intersection of 3D edge of finite length with infinite plane
// inputs:  * offset    - absolute position of start-point of the edge
//          * tangent   - relative position of end-point wrt start-point (length of the edge)
//          * basepoint - absolute position of a point on the plane
//          * span1,2   - two non-parallel vectors that define orientation of the plane
// output:  return TRUE upon success and updated variable:
//          * point     - absolute position of resulting intersection point
// NOTE: if edge is embedded in the plane its start-point will be returned
//
bool Utils::intersectionEdgePlane(TVector3 offset, TVector3 tangent,
				      TVector3 basepoint, TVector3 span1, TVector3 span2,
				      TVector3 &point) {
  double t1, t2; // dummy results
  return Utils::intersectionEdgePlane(offset, tangent, basepoint, span1, span2, point, t1, t2);
}
////////////////////
////////////////////
//
// calculates distance of 2D point from 2D infinite line
// input:   * offset    - absolute position of a point on the line
//          * tangent   - tangent vector of the line (not normalized to 1)
//          * point     - absolute position of tested point
// output:  returns distance from the closest point on the line
//
double Utils::distancePointLine(TVector2 offset, TVector2 tangent, TVector2 point) {
  return Utils::distancePointLine(TVector3(offset.X(), offset.Y(), 0),
				      TVector3(tangent.X(), tangent.Y(), 0),
				      TVector3(point.X(), point.Y(), 0));
}
////////////////////
////////////////////
//
// calculates distance of 2D point from 2D edge of finite length
// input:   * offset    - absolute position of start-point of the edge
//          * tangent   - relative position of end-point wrt start-point (length of the edge)
//          * point     - absolute position of tested point
// output:  returns distance from the closest point on the edge
//
double Utils::distancePointEdge(TVector2 offset, TVector2 tangent, TVector2 point) {
  return Utils::distancePointEdge(TVector3(offset.X(), offset.Y(), 0),
				      TVector3(tangent.X(), tangent.Y(), 0),
				      TVector3(point.X(), point.Y(), 0));
}
////////////////////
////////////////////
//
// calculates distance of 3D point from 3D infinite line
// input:   * offset    - absolute position of a point on the line
//          * tangent   - tangent vector of the line (not normalized to 1)
//          * point     - absolute position of tested point
// output:  returns distance from the closest point on the line
//
double Utils::distancePointLine(TVector3 offset, TVector3 tangent, TVector3 point) {
  const TVector3 unit_vec=tangent.Unit();
  if(unit_vec.Mag2()<Utils::NUMERICAL_TOLERANCE) return (point-offset).Mag();
  const TVector3 relative_vec=point-offset;
  return (relative_vec-(relative_vec*unit_vec)*unit_vec).Mag();
}
////////////////////
////////////////////
//
// calculates distance of 3D point from 3D edge of finite length
// input:   * offset    - absolute position of start-point of the edge
//          * tangent   - relative position of end-point wrt start-point (length of the edge)
//          * point     - absolute position of tested point
// output:  returns distance from the closest point on the edge
//
double Utils::distancePointEdge(TVector3 offset, TVector3 tangent, TVector3 point) {
  const TVector3 unit_vec=tangent.Unit();
  const TVector3 relative_vec[2] = { point-offset, point-offset-tangent };
  const double len[2] = { unit_vec*relative_vec[0], unit_vec*relative_vec[1] };
  const double len2=tangent.Mag2();
  if(len[0]*len[0]>len2) return relative_vec[1].Mag(); // distance from end-point
  if(len[1]*len[1]>len2) return relative_vec[0].Mag(); // distance from start-point
  return Utils::distancePointLine(offset, tangent, point); // distance from line
}
////////////////////
////////////////////
//
// calculates distance of 3D point from 3D infinite plane
// input:   * basepoint - absolute position of a point on the plane
//          * span1,2   - two non-parallel vectors that define orientation of the plane
//          * point     - absolute position of tested point
// output:  returns distance from the closest point on the plane
//
double Utils::distancePointPlane(TVector3 basepoint, TVector3 span1, TVector3 span2, TVector3 point) {
  return fabs(Utils::signedDistancePointPlane(basepoint, span1, span2, point));
}
////////////////////
////////////////////
//
// calculates signed distance (projection onto normal vector) of 3D point from 3D infinite plane
// input:   * basepoint - absolute position of a point on the plane
//          * span1,2   - two non-parallel vectors that define orientation of the plane
//                        cross product span1 x span2 defines normal vector of the plane
//          * point     - absolute position of tested point
// output:  returns signed distance from the closest point on the plane
//
double Utils::signedDistancePointPlane(TVector3 basepoint, TVector3 span1, TVector3 span2, TVector3 point) {
  auto normal = (span1.Cross(span2)).Unit();
  return (point-basepoint)*normal;
}
////////////////////
////////////////////
//
// combines run ID with relative CoBo timestamp (10ns units)
// NOTE: time zone is defined by runID convention (local time in most cases).
//
// input:   * run_id - run ID encoded as 14-digit decimal number (YYYYmmDDhhMMss)
//          * elapsed_time_10ns - elepsed run time from the GET counter (10ns units)
// output:  return Unix timestamp [s] with millisecond precision.
//
double Utils::getUnixTimestamp(long run_id, uint64_t elapsed_time_10ns) {

  // decode run ID from YYYYMMDDhhmmss decimal format
  auto year=(int)(run_id*1e-10);
  auto month=(int)(fmod(run_id, year*1e10)*1e-8);
  auto day=(int)(fmod(run_id, year*1e10+month*1e8)*1e-6);
  auto hour=(int)(fmod(run_id, year*1e10+month*1e8+day*1e6)*1e-4);
  auto minute=(int)(fmod(run_id, year*1e10+month*1e8+day*1e6+hour*1e4)*1e-2);
  auto sec=(int)(fmod(run_id, year*1e10+month*1e8+day*1e6+hour*1e4+minute*1e2)); 

  // create event Unix time point with millisecond precision
  std::tm tm{};
  tm.tm_year = year - 1900;
  tm.tm_mon = month - 1;
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min = minute;
  tm.tm_sec = sec;
  tm.tm_isdst = -1;
  return (double)(std::mktime(&tm) + elapsed_time_10ns*10.0e-9);
}
