#ifndef COORDINATE_CONVERTER_H
#define COORDINATE_CONVERTER_H

#include <ostream>

#include "TVector3.h"
#include "TRotation.h"

/*
3D Rotation is made using the Euler angles in the X-convention:

1. rotation about the Z-axis with angle phi
2. rotation about the new X-axis with angle theta
3. rotation about the new Z-axis with angle psi
*/
    
class CoordinateConverter{

public:

  CoordinateConverter(double aPhi=0.0, double aTheta=0.0, double aPsi=0.0);

  ~CoordinateConverter();

  TVector3 detToBeam(const TVector3 & aVec) const;

  TVector3 detToBeam(double x, double y, double z) const;

  TVector3 beamToDet(const TVector3 & aVec) const;

  TVector3 beamToDet(double x, double y, double z) const;

  void printRotation(std::ostream &out) const;

private:

  TRotation detToNominalBeamRotation;
  TRotation nominalToActualBeamRotation;
  TRotation totalRotation;

};

std::ostream & operator << (std::ostream &out, const CoordinateConverter &aConverter);

#endif 
