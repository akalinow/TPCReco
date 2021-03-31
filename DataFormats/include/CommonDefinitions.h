#ifndef __CommonDefinitions_h__
#define __CommonDefinitions_h__

#include <cmath>

//#define DIR_U    0    // U-direction channel index
//#define DIR_V    1    // V-direction channel index
//#define DIR_W    2    // W-direction channel index

enum projection{
		DIR_U=0,    // U-direction channel index
                DIR_V=1,    // V-direction channel index
                DIR_W=2,    // W-direction channel index
		DIR_XY=3,     // 2D projection on XY plane
		DIR_XZ=4,    // 2D projection on XZ plane
		DIR_YZ=5,    // 2D projection on YZ plane
		DIR_3D=6    // 3D reconstruction
};

//#### Angles of U/V/W unit vectors wrt X-axis [deg]
//#ANGLES: 90.0 -30.0 30.0
const std::vector<double> phiPitchDirection = {M_PI, -M_PI/6.0 + M_PI/2.0, M_PI/6.0 - M_PI/2.0};

#endif
