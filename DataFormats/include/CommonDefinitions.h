#ifndef __CommonDefinitions_h__
#define __CommonDefinitions_h__
#include <cmath>
const double pi = 4 * atan(1);
//#define DIR_U    0    // U-direction channel index
//#define DIR_V    1    // V-direction channel index
//#define DIR_W    2    // W-direction channel index

enum class projection : int {
		DIR_U=0,    // U-direction channel index
        DIR_V=1,    // V-direction channel index
        DIR_W=2,    // W-direction channel index
		DIR_XY=3,     // 2D projection on XY plane
		DIR_XZ=4,    // 2D projection on XZ plane
		DIR_YZ=5,    // 2D projection on YZ plane
		DIR_3D=6    // 3D reconstruction
};

inline bool IsDIR_UVW(projection DIR_) {
	return DIR_ == projection::DIR_U || DIR_ == projection::DIR_V || DIR_ == projection::DIR_W;
}

const auto proj_vec_UVW = std::vector<projection>{ projection::DIR_U,projection::DIR_V,projection::DIR_W };
  
//#### Angles of U/V/W unit vectors wrt X-axis [deg]
//#ANGLES: 90.0 -30.0 30.0
const std::vector<double> phiPitchDirection = {pi, -pi/6.0 + pi/2.0, pi/6.0 - pi/2.0};

#endif
