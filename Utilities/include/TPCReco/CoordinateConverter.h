#ifndef COORDINATE_CONVERTER_H
#define COORDINATE_CONVERTER_H

#include <TRotation.h>
#include <TVector3.h>
#include <TLorentzVector.h>
#include <ostream>

/*
3D Rotation is made using the Euler angles in the X-convention:

1. rotation about the Z-axis with angle phi
2. rotation about the new X-axis with angle theta
3. rotation about the new Z-axis with angle psi
*/
struct EulerAngles {
  double phi = 0;
  double theta = 0;
  double psi = 0;
};

class CoordinateConverter {
public:
  CoordinateConverter(EulerAngles nominal, EulerAngles correction = {}, TVector3 offset = {0, 0, 0});

  TVector3 detToBeam(const TVector3 &vector) const;
  TVector3 detToBeamWithOffset(const TVector3 &vector) const; // only valid for point positions
  TLorentzVector detToBeam(const TLorentzVector &vector) const;
  TVector3 beamToDet(const TVector3 &vector) const;
  TVector3 beamToDetWithOffset(const TVector3 &vector) const; // only valid for point positions
  TLorentzVector beamToDet(const TLorentzVector &vector) const;

private:
  TRotation rotation; // rotation matrix from DET coordinates to BEAM coordinates
  TVector3 beamOriginInDet; // position [mm] of the origin of the BEAM coordinate system expressed in DET coordinates
};

std::ostream &operator<<(std::ostream &out,
                         const CoordinateConverter &converter);

#endif
