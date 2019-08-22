#ifndef __CommonDefinitions_h__
#define __CommonDefinitions_h__

//#define DIR_U    0    // U-direction channel index
//#define DIR_V    1    // V-direction channel index
//#define DIR_W    2    // W-direction channel index

enum projection{
		DIR_U=0,    // U-direction channel index
                DIR_V=1,    // V-direction channel index
                DIR_W=2,    // W-direction channel index
		DIR_3D=3    // 3D reconstruction
};

//const std::vector<int> stripOffset = {-71, 0, -55};//strip/time bin units
const std::vector<int> stripOffset = {0, 0, 0};/// mm units
  
//#### Angles of U/V/W unit vectors wrt X-axis [deg]
//#ANGLES: 90.0 -30.0 30.0
const std::vector<double> phiPitchDirection = {M_PI, -M_PI/6.0 + M_PI/2.0, M_PI/6.0 - M_PI/2.0};

#endif
