// Tests of the TANGENT_ROTATION fit mode of Track3D: the fit parameters are
// two small rotation angles about the segment tangent at the start of the fit.
#include <cmath>

#include <gtest/gtest.h>

#include "TPCReco/Track3D.h"

namespace {

Track3D makeTrack(const TVector3 &bias, const TVector3 &tangent) {
  TrackSegment3D aSegment;
  aSegment.setBiasTangent(bias, tangent);
  Track3D aTrack;
  aTrack.addSegment(aSegment);
  aTrack.setFitMode(definitions::fit_type::TANGENT_ROTATION);
  return aTrack;
}

void setRotation(Track3D &aTrack, double alpha, double beta) {
  const double par[3] = {alpha, beta, 0.0};
  aTrack.updateAndGetLoss(par);
}

}  // namespace

TEST(Track3DTest, RotationZeroKeepsTangent) {
  const TVector3 bias(10, -5, 3), tangent = TVector3(1, 2, 3).Unit();
  Track3D aTrack = makeTrack(bias, tangent);
  setRotation(aTrack, 0.0, 0.0);
  const auto &aSegment = aTrack.getSegments().front();
  EXPECT_NEAR((aSegment.getTangent() - tangent).Mag(), 0.0, 1E-12);
  EXPECT_NEAR((aSegment.getBias() - bias).Mag(), 0.0, 1E-12);
}

TEST(Track3DTest, RotationAngleAndAxes) {
  // also for tangents along the z axis, where theta, phi are singular
  for (const TVector3 &tangent : {TVector3(0, 0, 1), TVector3(0, 0, -1), TVector3(1, -1, 0.5).Unit()}) {
    const TVector3 bias(1, 2, 3);
    for (double alpha : {0.05, -0.1}) {
      Track3D aTrack = makeTrack(bias, tangent);
      setRotation(aTrack, alpha, 0.0);
      const TVector3 t1 = aTrack.getSegments().front().getTangent();
      setRotation(aTrack, 0.0, alpha);
      const TVector3 t2 = aTrack.getSegments().front().getTangent();
      // rotation by atan(|alpha|) away from the reference direction
      EXPECT_NEAR(t1.Angle(tangent), std::atan(std::abs(alpha)), 1E-12);
      EXPECT_NEAR(t2.Angle(tangent), std::atan(std::abs(alpha)), 1E-12);
      // the two angles rotate about perpendicular axes: the components of
      // t1, t2 perpendicular to the reference direction are perpendicular
      const TVector3 p1 = t1 - t1.Dot(tangent) * tangent, p2 = t2 - t2.Dot(tangent) * tangent;
      EXPECT_NEAR(p1.Dot(p2), 0.0, 1E-12);
      EXPECT_GT(p1.Mag(), 1E-3);
      // rotation about the bias point
      EXPECT_NEAR((aTrack.getSegments().front().getBias() - bias).Mag(), 0.0, 1E-12);
    }
  }
}

TEST(Track3DTest, RotationIsRelativeToStartOfFit) {
  // the reference direction is fixed when the fit mode is set, so the same
  // parameters always give the same tangent
  const TVector3 tangent = TVector3(0.3, 0.4, 0.8).Unit();
  Track3D aTrack = makeTrack(TVector3(), tangent);
  setRotation(aTrack, 0.1, -0.05);
  const TVector3 first = aTrack.getSegments().front().getTangent();
  setRotation(aTrack, 0.02, 0.02);
  setRotation(aTrack, 0.1, -0.05);
  EXPECT_NEAR((aTrack.getSegments().front().getTangent() - first).Mag(), 0.0, 1E-12);
}
