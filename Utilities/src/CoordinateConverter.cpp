#include "TPCReco/CoordinateConverter.h"
#include "TPCReco/colorText.h"

////////////////////////////////////////////
////////////////////////////////////////////
CoordinateConverter::CoordinateConverter(EulerAngles nominal,
                                         EulerAngles correction,
					 TVector3 offset) {

  auto detToNominalBeamRotation =
      TRotation{}.SetXEulerAngles(nominal.phi, nominal.theta, nominal.psi);
  auto nominalToActualBeamRotation = TRotation{}.SetXEulerAngles(
      correction.phi, correction.theta, correction.psi);
  rotation = nominalToActualBeamRotation * detToNominalBeamRotation;
  beamOriginInDet = offset;
}

////////////////////////////////////////////
////////////////////////////////////////////
TVector3 CoordinateConverter::detToBeam(const TVector3 &vector) const {
  return rotation * vector;
}

////////////////////////////////////////////
////////////////////////////////////////////
TVector3 CoordinateConverter::detToBeamWithOffset(const TVector3 &vector) const {
  return rotation * (vector - beamOriginInDet);
}

////////////////////////////////////////////
////////////////////////////////////////////
TLorentzVector CoordinateConverter::detToBeam(const TLorentzVector &vector) const {
  return TLorentzVector{rotation * vector.Vect(), vector.T()};
}

////////////////////////////////////////////
////////////////////////////////////////////
TVector3 CoordinateConverter::beamToDet(const TVector3 &vector) const {
  return rotation.Inverse() * vector;
}

////////////////////////////////////////////
////////////////////////////////////////////
TVector3 CoordinateConverter::beamToDetWithOffset(const TVector3 &vector) const {
  return rotation.Inverse() * vector + beamOriginInDet;
}

////////////////////////////////////////////
////////////////////////////////////////////
TLorentzVector CoordinateConverter::beamToDet(const TLorentzVector &vector) const {
  return TLorentzVector{rotation.Inverse() * vector.Vect(), vector.T()};
}

////////////////////////////////////////////
////////////////////////////////////////////
std::ostream &operator<<(std::ostream &out,
                         const CoordinateConverter &converter) {
  auto vectors = {TVector3{1, 0, 0}, TVector3{0, 1, 0}, TVector3{0, 0, 1}};

  out << KBLU << "DETECTOR --> BEAM" << RST << std::endl;
  for (auto vector : vectors) {
    auto result = converter.detToBeam(vector);
    out << "(" << vector.X() << ", " << vector.Y() << ", " << vector.Z() << ")"
        << KBLU << " --> " << RST << "(" << result.X() << ", " << result.Y()
        << ", " << result.Z() << ")" << std::endl;
  }
  out << KBLU << "BEAM --> DETECTOR" << RST << std::endl;
  for (auto vector : vectors) {
    auto result = converter.beamToDet(vector);
    out << "(" << vector.X() << ", " << vector.Y() << ", " << vector.Z() << ")"
        << KBLU << " --> " << RST << "(" << result.X() << ", " << result.Y()
        << ", " << result.Z() << ")" << std::endl;
  }
  return out;
}
////////////////////////////////////////////
////////////////////////////////////////////
