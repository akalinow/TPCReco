#ifndef __CommonDefinitions_h__
#define __CommonDefinitions_h__

#include <cmath>

enum projection{
		DIR_U=0,    // U-direction channel index
                DIR_V=1,    // V-direction channel index
                DIR_W=2,    // W-direction channel index
		DIR_XY=3,     // 2D projection on XY plane
		DIR_XZ=4,    // 2D projection on XZ plane
		DIR_YZ=5,    // 2D projection on YZ plane
		DIR_3D=6    // 3D reconstruction
};

enum pid_type{
	      UNKNOWN=0,
	      ALPHA=1,          // Helium 4
	      CARBON_12,        // Carbon 12
	      CARBON_14,        // Carbon 14
	      C12_ALPHA,        
	      HELIUM_4=ALPHA,   // alias
	      C_12=CARBON_12,   // alias
	      C_14=CARBON_14,   // alias
	      PID_MIN=ALPHA,    // alias
	      PID_MAX=CARBON_14 // alias
  };

enum gas_mixture_type{
		      CO2=1,            // Carbon dioxide
		      GAS_MIN=CO2,
		      GAS_MAX=CO2
};

#endif
