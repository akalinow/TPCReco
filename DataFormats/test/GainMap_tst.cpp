// Tests of the relative gain map g(X_DET) and of its per-hit application in TrackSegment3D.
#include <cmath>
#include <cstdio>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#include "TPCReco/GainMap.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/TrackSegment3D.h"

namespace {

std::string writeFile(const std::string &name, const std::string &content) {
  const std::string path = std::string(::testing::TempDir()) + name;
  std::ofstream(path) << content;
  return path;
}

}  // namespace

TEST(GainMapTest, EmptyMapIsUniform) {
  GainMap aMap;
  EXPECT_TRUE(aMap.empty());
  EXPECT_DOUBLE_EQ(aMap.getGain(-100.0), 1.0);
  EXPECT_DOUBLE_EQ(aMap.getGain(37.0), 1.0);
}

TEST(GainMapTest, InterpolationAndClamping) {
  GainMap aMap({-10.0, 10.0, 30.0}, {0.9, 1.1, 1.0});
  EXPECT_DOUBLE_EQ(aMap.getGain(-10.0), 0.9);
  EXPECT_DOUBLE_EQ(aMap.getGain(0.0), 1.0);     // half way between 0.9 and 1.1
  EXPECT_DOUBLE_EQ(aMap.getGain(20.0), 1.05);
  EXPECT_DOUBLE_EQ(aMap.getGain(-50.0), 0.9);   // constant beyond the outer centres
  EXPECT_DOUBLE_EQ(aMap.getGain(99.0), 1.0);
}

TEST(GainMapTest, ReadsFile) {
  const std::string path = writeFile("gainmap_ok.dat",
                                     "# comment\n\n -20 0 0.95  # trailing comment\n0 20 1.05\n");
  GainMap aMap(path);
  EXPECT_FALSE(aMap.empty());
  EXPECT_EQ(aMap.getFileName(), path);
  EXPECT_DOUBLE_EQ(aMap.getGain(-10.0), 0.95);  // bin centres at -10 and +10
  EXPECT_DOUBLE_EQ(aMap.getGain(10.0), 1.05);
  EXPECT_DOUBLE_EQ(aMap.getGain(0.0), 1.0);
  std::remove(path.c_str());
}

TEST(GainMapTest, RejectsBadInput) {
  EXPECT_THROW(GainMap("/nonexistent/gain_map.dat"), std::runtime_error);
  for (const std::string &content : {std::string("# only comments\n"), std::string("0 10 abc\n"),
                                     std::string("10 0 1.0\n"), std::string("0 10 0.0\n"),
                                     std::string("10 20 1.0\n0 10 1.0\n")}) {
    const std::string path = writeFile("gainmap_bad.dat", content);
    EXPECT_THROW(GainMap aMap(path), std::runtime_error) << content;
    std::remove(path.c_str());
  }
  EXPECT_THROW(GainMap({0.0, 1.0}, {1.0}), std::runtime_error);
}

TEST(GainMapTest, DistributedResourceMapIsValid) {
  GainMap aMap(RESOURCES_DIR "gainMap_HIgS2022_xDET.dat");
  EXPECT_FALSE(aMap.empty());
  for (double x = -170; x <= 170; x += 5) {
    EXPECT_GT(aMap.getGain(x), 0.9);
    EXPECT_LT(aMap.getGain(x), 1.1);
  }
}

namespace {
// Straight track with charge per unit length gain(x) (uniform energy deposit seen through a
// position-dependent gain): the line charge is smeared with a Gaussian (diffusion) and
// integrated over the (time bin x strip) cells of each projection.
TrackSegment3D makeSegment(std::shared_ptr<GeometryTPC> geo, const TVector3 &start, const TVector3 &direction,
                           double length, const std::function<double(double)> &gain) {
  const double tw = geo->GetTimeBinWidth(), pitch = geo->GetStripPitch(), sigma = 1.0, step = 0.05;
  const TVector3 dir = direction.Unit();
  auto cdf = [&](double a) { return 0.5 * std::erfc(-a / (std::sqrt(2.0) * sigma)); };
  std::vector<Hit2DCollection> hits(3);
  for (int iDir = 0; iDir < 3; ++iDir) {
    const TVector3 pitchDir = geo->GetStripPitchVector3D(iDir);
    std::map<std::pair<long, long>, double> cells;
    for (double l = 0.5 * step; l < length; l += step) {
      const TVector3 p = start + l * dir;
      const double z = p.Z(), u = p.Dot(pitchDir), q = step * gain(p.X());
      for (long is = std::lround(std::floor((u - 4 * sigma) / pitch)); is <= std::lround(std::floor((u + 4 * sigma) / pitch)); ++is) {
        const double wu = cdf((is + 1) * pitch - u) - cdf(is * pitch - u);
        for (long it = std::lround(std::floor((z - 4 * sigma) / tw)); it <= std::lround(std::floor((z + 4 * sigma) / tw)); ++it) {
          cells[{it, is}] += q * wu * (cdf((it + 1) * tw - z) - cdf(it * tw - z));
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
  return aSegment;
}

// mean and relative RMS of the charge profile in the central part of the track
std::pair<double, double> profileFlatness(const TH1F &h, double length) {
  double sum = 0, sum2 = 0;
  int n = 0;
  for (int i = 1; i <= h.GetNbinsX(); ++i) {
    const double d = h.GetBinCenter(i);
    if (d < 0.15 * length || d > 0.85 * length) continue;
    sum += h.GetBinContent(i);
    sum2 += h.GetBinContent(i) * h.GetBinContent(i);
    ++n;
  }
  const double mean = sum / n;
  return {mean, std::sqrt(std::max(sum2 / n - mean * mean, 0.0)) / mean};
}
}  // namespace

TEST(GainMapTest, SegmentChargeIsCorrected) {
  auto geo = std::make_shared<GeometryTPC>(RESOURCES_DIR "geometry_ELITPC_130mbar_1372Vdrift_25MHz.dat", false);
  ASSERT_TRUE(geo->IsOK());
  auto gain = [](double x) { return 1.0 + 0.002 * x; };   // 0.8 at x=-100, 1.1 at x=+50
  const TVector3 start(-100, 5, -10);
  for (const TVector3 &direction : {TVector3(1, 0.2, 0.1), TVector3(1, -0.5, -0.6), TVector3(-1, 0.3, 0.4)}) {
    const double length = 120;
    const TVector3 st = direction.X() > 0 ? start : TVector3(50, 5, -10);
    TrackSegment3D uniform = makeSegment(geo, st, direction, length, [](double) { return 1.0; });
    TrackSegment3D scaled = makeSegment(geo, st, direction, length, gain);
    const double q0 = uniform.getIntegratedCharge(length);
    const double q0Half = uniform.getIntegratedCharge(0.5 * length);

    // without a map the scaled track has the average gain along it (gain is linear in x)
    const double meanGain = gain(st.X() + 0.5 * length * direction.Unit().X());
    const double qRaw = scaled.getIntegratedCharge(length);
    EXPECT_NEAR(qRaw / q0, meanGain, 0.01);
    // ~15% linear ramp over the central 70% of the track: relative RMS ~ 0.15/sqrt(12) = 0.04
    EXPECT_GT(profileFlatness(scaled.getChargeProfile(), length).second, 0.03);

    // with the map: the uniform charge is recovered (integrated charge and flat profile)
    std::vector<double> centres, gains;
    for (double x = -200; x <= 200; x += 10) {
      centres.push_back(x);
      gains.push_back(gain(x));
    }
    geo->SetGainMap(std::make_shared<const GainMap>(centres, gains));
    EXPECT_NEAR(scaled.getIntegratedCharge(length) / q0, 1.0, 0.005) << direction.X();
    EXPECT_NEAR(scaled.getIntegratedCharge(0.5 * length) / q0Half, 1.0, 0.005);
    EXPECT_LT(profileFlatness(scaled.getChargeProfile(), length).second, 0.02);
    // the uniform track is changed only by the map itself
    geo->SetGainMap(nullptr);
    EXPECT_DOUBLE_EQ(uniform.getIntegratedCharge(length), q0);
  }
}
