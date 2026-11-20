// Tests of the time (Z_DET) part of StripResponseCalculator: Gaussian longitudinal diffusion combined with
// the AGET shaping (peaking time). Before the fix of 2026-11-20 the shaping step dropped the time-cell index of
// the diffused charge, so with peaking_time>0 the time response did not depend on sigma_z at all.
#include <cmath>
#include <memory>

#include <gtest/gtest.h>

#include "TPCReco/GeometryTPC.h"
#include "TPCReco/StripResponseCalculator.h"

namespace {

const int nTimeCells = 50;

struct Moments { double sum, mean, rms; }; // in time cells

// charge fractions over the relative time cells for the charge at sub-cell bin ibin of the reference cell
Moments timeMoments(StripResponseCalculator &calc, int ibin) {
  double s = 0, m = 0, m2 = 0;
  for (int k = -nTimeCells; k <= nTimeCells; ++k) {
    const double v = calc.getTimeResponseHistogram(k)->GetBinContent(ibin);
    s += v; m += v * k; m2 += v * k * k;
  }
  m /= s;
  return {s, m, std::sqrt(m2 / s - m * m)};
}

std::shared_ptr<GeometryTPC> geometry() {
  return std::make_shared<GeometryTPC>(RESOURCES_DIR "geometry_ELITPC_130mbar_1372Vdrift_25MHz.dat", false);
}

std::unique_ptr<StripResponseCalculator> calculator(std::shared_ptr<GeometryTPC> geo, double sigmaZ, double peakingTime) {
  // small XY part (1 neighbour strip): only the time part is tested
  auto calc = std::make_unique<StripResponseCalculator>(geo, 1, nTimeCells, 2, 0.5, sigmaZ, peakingTime);
  calc->initializeTimeResponse(200000, 20);
  return calc;
}

} // namespace

TEST(StripResponseCalculator, GaussianTimeResponseWidth) {
  auto geo = geometry();
  const double cell = geo->GetTimeBinWidth(); // [mm]
  for (double sigmaZ : {0.5, 2.0}) {
    auto calc = calculator(geo, sigmaZ, 0);
    const auto mom = timeMoments(*calc, 10);
    EXPECT_NEAR(mom.sum, 1.0, 1E-3) << "sigmaZ=" << sigmaZ;
    EXPECT_NEAR(mom.rms, sigmaZ / cell, 0.05 * sigmaZ / cell + 0.3) << "sigmaZ=" << sigmaZ;
  }
}

TEST(StripResponseCalculator, ShapedTimeResponseKeepsDiffusion) {
  auto geo = geometry();
  const double cell = geo->GetTimeBinWidth(); // [mm]
  const double peakingTime = 232; // [ns]
  // shaping alone: sigma_z small compared with the shaping width
  const auto shapeOnly = timeMoments(*calculator(geo, 0.05, peakingTime), 10);
  for (double sigmaZ : {0.5, 2.0}) {
    const auto shaped = timeMoments(*calculator(geo, sigmaZ, peakingTime), 10);
    const double expected = std::sqrt(std::pow(sigmaZ / cell, 2) + std::pow(shapeOnly.rms, 2));
    EXPECT_NEAR(shaped.sum, 1.0, 2E-3) << "sigmaZ=" << sigmaZ;
    EXPECT_NEAR(shaped.mean, shapeOnly.mean, 0.2) << "sigmaZ=" << sigmaZ; // shaping delay unchanged
    EXPECT_NEAR(shaped.rms, expected, 0.05 * expected) << "sigmaZ=" << sigmaZ; // widths add in quadrature
  }
}
