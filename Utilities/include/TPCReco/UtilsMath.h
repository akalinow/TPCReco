#ifndef UTILITIES_UTILS_MATH_H_
#define UTILITIES_UTILS_MATH_H_

#include <vector>
#include <TVector2.h>
#include <TVector3.h>
#include <TGraph.h>

namespace Utils {

  const double NUMERICAL_TOLERANCE=1e-5;

  // calculates intersection of 2D non-parallel edges of finite length
  bool intersectionTwoEdges(TVector2 offset1, TVector2 tangent1,
			    TVector2 offset2, TVector2 tangent2,
			    TVector2 &point);

  // calculates intersection of 2D non-parallel infinte lines
  bool intersectionTwoLines(TVector2 offset1, TVector2 tangent1,
			    TVector2 offset2, TVector2 tangent2,
			    TVector2 &point);

  // calculates intersection of 3D edge of finite length with infinite plane
  bool intersectionEdgePlane(TVector3 offset, TVector3 tangent,
			     TVector3 basepoint, TVector3 span1, TVector3 span2,
			     TVector3 &point);

  // calculates all intersections of 2D edge of finite length with 2D closed polygon
  bool intersectionEdgePolygon(TVector2 offset, TVector2 tangent, TGraph poly,
			       std::vector<TVector2> &point_vec, std::vector<int> &index_vec);

  // calculates intersection of 3D infinite line with infinite plane
  bool intersectionLinePlane(TVector3 offset, TVector3 tangent,
			     TVector3 basepoint, TVector3 span1, TVector3 span2,
			     TVector3 &point);

  // calculates intersection of 3D edge of finite length with infinite plane
  bool intersectionEdgePlane(TVector3 offset, TVector3 tangent,
			     TVector3 basepoint, TVector3 span1, TVector3 span2,
			     TVector3 &point);

  // calculates intersection of 3D infinite line with infinite plane
  // plus coefficients t1, t2 for linear combination (point-basepoint)=t1*span1+t2*span2
  bool intersectionLinePlane(TVector3 offset, TVector3 tangent,
			     TVector3 basepoint, TVector3 span1, TVector3 span2,
			     TVector3 &point, double &t1, double &t2);

  // calculates intersection of 3D edge of finite length with infinite plane
  // plus coefficients t1, t2 for linear combination (point-basepoint)=t1*span1+t2*span2
  bool intersectionEdgePlane(TVector3 offset, TVector3 tangent,
			     TVector3 basepoint, TVector3 span1, TVector3 span2,
			     TVector3 &point, double &t1, double &t2);

  // calculates distance of 2D point from 2D infinite line
  double distancePointLine(TVector2 offset, TVector2 tangent, TVector2 point);

  // calculates distance of 2D point from 2D edge of finite length
  double distancePointEdge(TVector2 offset, TVector2 tangent, TVector2 point);

  // calculates distance of 3D point from 3D infinite line
  double distancePointLine(TVector3 offset, TVector3 tangent, TVector3 point);

  // calculates distance of 3D point from 3D edge of finite length
  double distancePointEdge(TVector3 offset, TVector3 tangent, TVector3 point);

  // calculates distance of 3D point from 3D infinite plane
  double distancePointPlane(TVector3 basepoint, TVector3 span1, TVector3 span2, TVector3 point);

  // calculates signed distance (projection onto normal vector) of 3D point from 3D infinite plane
  double signedDistancePointPlane(TVector3 basepoint, TVector3 span1, TVector3 span2, TVector3 point);
};
#endif // UTILITIES_UTILS_MATH_H_
