// Tests of the TANGENT_ROTATION fit mode of Track3D: the fit parameters are
// two small rotation angles about the segment tangent at the start of the fit.
#include <cmath>
#include <map>
#include <memory>

#include <gtest/gtest.h>

#include "TPCReco/GeometryTPC.h"
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

TEST(Track3DTest, CrossingPointsAreFinite) {
  // lines parallel to a coordinate plane and dots (null tangent) gave +-inf or NaN
  const TVector3 bias(10, -5, 3);
  for (const TVector3 &tangent : {TVector3(1, 0, 0), TVector3(0, 1, 0), TVector3(0, 0, 1), TVector3(1, 2, 3)}) {
    TrackSegment3D aSegment;
    aSegment.setBiasTangent(bias, tangent);
    for (const TVector3 &p : {aSegment.getBiasAtX0(), aSegment.getBiasAtY0(), aSegment.getBiasAtZ0()}) {
      EXPECT_TRUE(std::isfinite(p.X()) && std::isfinite(p.Y()) && std::isfinite(p.Z()));
    }
  }
  TrackSegment3D aDot;
  aDot.setStartEnd(bias, bias);
  EXPECT_DOUBLE_EQ(aDot.getLength(), 0.0);
  EXPECT_TRUE(std::isfinite(aDot.getBiasAtX0().Mag()) && std::isfinite(aDot.getBiasAtZ0().Mag()));
  // crossing point of a generic line
  TrackSegment3D aSegment;
  aSegment.setBiasTangent(bias, TVector3(1, 2, 3));
  EXPECT_NEAR(aSegment.getBiasAtX0().X(), 0.0, 1E-12);
  EXPECT_NEAR(aSegment.getBiasAtZ0().Z(), 0.0, 1E-12);
}

TEST(Track3DTest, Segment2DDefaultLength) {
  // the length of a default segment was not initialised
  TrackSegment2D aSegment;
  EXPECT_DOUBLE_EQ(aSegment.getLength(), 0.0);
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

namespace {
// Charge profile of a straight track with uniform charge per unit length:
// for each projection, the charge of the line is smeared with a Gaussian
// (diffusion) and integrated over the (time bin x strip) cells.
TH1F uniformTrackProfile(std::shared_ptr<GeometryTPC> geo, const TVector3 &start,
                         const TVector3 &direction, double length, double sigma) {
  const double tw = geo->GetTimeBinWidth(), pitch = geo->GetStripPitch();
  const TVector3 dir = direction.Unit();
  auto cdf = [&](double a) { return 0.5 * std::erfc(-a / (std::sqrt(2.0) * sigma)); };
  std::vector<Hit2DCollection> hits(3);
  for (int iDir = 0; iDir < 3; ++iDir) {
    const TVector3 pitchDir = geo->GetStripPitchVector3D(iDir);
    std::map<std::pair<long, long>, double> cells;
    for (double l = 0; l < length; l += 0.05) {
      const TVector3 p = start + l * dir;
      const double z = p.Z(), u = p.Dot(pitchDir);
      for (long is = std::lround(std::floor((u - 4 * sigma) / pitch)); is <= std::lround(std::floor((u + 4 * sigma) / pitch)); ++is) {
        const double wu = cdf((is + 1) * pitch - u) - cdf(is * pitch - u);
        for (long it = std::lround(std::floor((z - 4 * sigma) / tw)); it <= std::lround(std::floor((z + 4 * sigma) / tw)); ++it) {
          cells[{it, is}] += 0.05 * wu * (cdf((it + 1) * tw - z) - cdf(it * tw - z));
        }
      }
    }
    for (const auto &cell : cells) {
      hits[iDir].push_back(Hit2D((cell.first.first + 0.5) * tw, (cell.first.second + 0.5) * pitch, 100 * cell.second));
    }
  }
  TrackSegment3D aSegment;
  aSegment.setGeometry(geo);
  aSegment.setStartEnd(start, start + length * dir);
  aSegment.setRecHits(hits);
  return aSegment.getChargeProfile();
}
}  // namespace

TEST(Track3DTest, ChargeProfileOfUniformTrackIsFlat) {
  // the cell extent along the track was too short for tracks with decreasing
  // time (dips of 13% for a steep track towards -z), and a hit inside one
  // histogram bin was counted with a wrong width
  auto geo = std::make_shared<GeometryTPC>(RESOURCES_DIR "geometry_ELITPC_130mbar_1372Vdrift_25MHz.dat", false);
  const TVector3 start(5, -3, 2);
  for (const TVector3 &direction : {TVector3(1, 0.3, 0), TVector3(-1, -0.3, 0), TVector3(0.1, 0.05, 1),
                                    TVector3(-0.1, -0.05, -1), TVector3(0.6, 0.3, -0.7), TVector3(0.6, 0.3, 0.7)}) {
    const double length = 50;
    const TH1F profile = uniformTrackProfile(geo, start, direction, length, 1.5);
    double minValue = 1E9, maxValue = 0;
    for (int iBin = 1; iBin <= profile.GetNbinsX(); ++iBin) {
      const double x = profile.GetBinCenter(iBin);
      if (x < 0.1 * length || x > 0.9 * length) continue;
      minValue = std::min(minValue, profile.GetBinContent(iBin));
      maxValue = std::max(maxValue, profile.GetBinContent(iBin));
    }
    EXPECT_GT(minValue, 0.97 * maxValue) << "direction (" << direction.X() << ", " << direction.Y() << ", " << direction.Z() << ")";
  }
}
