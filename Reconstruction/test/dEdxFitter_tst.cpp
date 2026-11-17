// Tests of the dE/dx models of dEdxFitter: exact Gaussian smearing of the
// tabulated dE/dx curves (BraggTemplate) and the bragg_alpha / bragg_12C models.
#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>

#include <gtest/gtest.h>

#include "TPCReco/dEdxFitter.h"

namespace {

// linear interpolation of the table, zero outside [cut, end of table]
double tableValue(const dEdxFitter::BraggTemplate &t, double x, double cut) {
  if (x < std::max(cut, t.s.front()) || x > t.s.back()) return 0.0;
  size_t i = std::upper_bound(t.s.begin(), t.s.end(), x) - t.s.begin();
  i = std::min(i > 0 ? i - 1 : 0, t.s.size() - 2);
  return t.a[i] + t.k[i] * (x - t.s[i]);
}

// numerical convolution with a Gaussian (midpoint rule, fine steps)
double numericalSmearing(const dEdxFitter::BraggTemplate &t, double pos, double cut, double sigma) {
  const double step = 1E-3;
  double sum = 0.0;
  for (double x = pos - 8 * sigma; x < pos + 8 * sigma; x += step) {
    const double xm = x + 0.5 * step;
    const double u = (xm - pos) / sigma;
    sum += tableValue(t, xm, cut) * std::exp(-0.5 * u * u) / (std::sqrt(2 * M_PI) * sigma) * step;
  }
  return sum;
}

double alphaModel(double x, double sigma, double vertex, double alphaOffset) {
  double params[7] = {sigma, vertex, alphaOffset, 0.0, 1.0, 0.0, 1.0};
  return dEdxFitter::bragg_alpha(&x, params);
}

double carbonModel(double x, double sigma, double vertex, double carbonOffset) {
  double params[7] = {sigma, vertex, 0.0, carbonOffset, 0.0, 1.0, 1.0};
  return dEdxFitter::bragg_12C(&x, params);
}

// half of the 16%-84% rise distance of the model at the vertex edge [mm]
double edgeWidth(double sigma, double vertex, double alphaOffset) {
  const double plateau = alphaModel(vertex + 6 * sigma, sigma, vertex, alphaOffset);
  double x16 = 0, x84 = 0;
  for (double x = vertex - 6 * sigma; x < vertex + 6 * sigma; x += 1E-3) {
    const double value = alphaModel(x, sigma, vertex, alphaOffset);
    if (!x16 && value > 0.16 * plateau) x16 = x;
    if (!x84 && value > 0.84 * plateau) x84 = x;
  }
  return 0.5 * (x84 - x16);
}

class dEdxFitterTest : public ::testing::Test {
protected:
  void SetUp() override { fitter = std::make_unique<dEdxFitter>(RESOURCES_DIR, 250); }
  std::unique_ptr<dEdxFitter> fitter;
};

}  // namespace

TEST_F(dEdxFitterTest, TemplatesLoaded) {
  EXPECT_EQ(dEdxFitter::alphaTemplate().s.size(), 300u);
  EXPECT_EQ(dEdxFitter::carbonTemplate().s.size(), 95u);
}

TEST_F(dEdxFitterTest, SmallSigmaReproducesTable) {
  for (const auto *t : {&dEdxFitter::alphaTemplate(), &dEdxFitter::carbonTemplate()}) {
    const double end = t->s.back();
    for (double pos : {0.3 * end, 0.5 * end, 0.77 * end, 0.95 * end}) {
      EXPECT_NEAR(t->smeared(pos, 0.0, 1E-4), tableValue(*t, pos, 0.0), 1E-3 * tableValue(*t, pos, 0.0))
          << "pos " << pos;
    }
  }
}

TEST_F(dEdxFitterTest, SmearingEqualsNumericalConvolution) {
  const auto &alpha = dEdxFitter::alphaTemplate();
  const double cut = 50.0;
  for (double sigma : {0.5, 1.5, 3.0}) {
    // around the start of the track, along the track, at the Bragg peak and at the stopping point
    for (double pos : {cut - 2.0, cut, cut + 1.0, 200.0, 286.0, 297.0, 299.0, 302.0}) {
      EXPECT_NEAR(alpha.smeared(pos, cut, sigma), numericalSmearing(alpha, pos, cut, sigma), 1E-3)
          << "alpha: sigma " << sigma << " pos " << pos;
    }
  }
  const auto &carbon = dEdxFitter::carbonTemplate();
  for (double sigma : {0.5, 1.5}) {
    for (double pos : {-1.0, 0.0, 2.0, 4.25, 12.0, 20.0, 23.5, 25.0}) {
      EXPECT_NEAR(carbon.smeared(pos, 0.0, sigma), numericalSmearing(carbon, pos, 0.0, sigma), 1E-2)
          << "carbon: sigma " << sigma << " pos " << pos;
    }
  }
}

TEST_F(dEdxFitterTest, SmearingConservesEnergy) {
  const auto &alpha = dEdxFitter::alphaTemplate();
  const double cut = 100.0, sigma = 2.0;
  double table = 0.0, smeared = 0.0;
  for (double x = cut - 20; x < alpha.s.back() + 20; x += 0.01) {
    table += tableValue(alpha, x, cut) * 0.01;
    smeared += alpha.smeared(x, cut, sigma) * 0.01;
  }
  EXPECT_NEAR(smeared, table, 1E-4 * table);
}

TEST_F(dEdxFitterTest, AlphaModelIsContinuous) {
  // the old model switched from the smeared edge to the table at 3 sigma,
  // with a step of up to 6% at 130 mbar
  for (double pressure : {250.0, 190.0, 130.0}) {
    fitter->setPressure(pressure);
    const double sigma = 1.0, vertex = 10.0, offset = 150.0;
    const double maxValue = alphaModel(vertex + 20, sigma, vertex, offset);
    double maxStep = 0.0;
    for (double x = vertex - 5; x < vertex + 10; x += 1E-3) {
      maxStep = std::max(maxStep, std::abs(alphaModel(x + 1E-3, sigma, vertex, offset) -
                                           alphaModel(x, sigma, vertex, offset)));
    }
    EXPECT_LT(maxStep, 1E-3 * maxValue) << "pressure " << pressure;
  }
}

TEST_F(dEdxFitterTest, SigmaInPhysicalUnits) {
  // the vertex edge width is sigma [mm] at every pressure
  // (it was sigma*nominalPressure/pressure before)
  for (double pressure : {250.0, 190.0, 130.0}) {
    fitter->setPressure(pressure);
    for (double sigma : {0.7, 1.5}) {
      EXPECT_NEAR(edgeWidth(sigma, 10.0, 150.0), sigma, 0.05 * sigma)
          << "pressure " << pressure << " sigma " << sigma;
    }
  }
}

TEST_F(dEdxFitterTest, BraggPeakSmeared) {
  // the end of the alpha track depends on sigma (it did not before)
  fitter->setPressure(250);
  const double vertex = 0.0, offset = 100.0;
  const double end = 299.0 - offset;  // stopping point [mm]
  const double peakNarrow = alphaModel(end - 13, 0.5, vertex, offset);
  const double peakWide = alphaModel(end - 13, 3.0, vertex, offset);
  EXPECT_GT(peakNarrow, peakWide);
  EXPECT_GT(alphaModel(end + 2, 3.0, vertex, offset), alphaModel(end + 2, 0.5, vertex, offset));
  EXPECT_GT(alphaModel(end + 2, 3.0, vertex, offset), 0.0);
}

TEST_F(dEdxFitterTest, CarbonModelUsesTable) {
  // carbon goes from the vertex towards smaller x; for sigma -> 0 the model is the table
  fitter->setPressure(250);
  const auto &carbon = dEdxFitter::carbonTemplate();
  const double vertex = 30.0;
  for (double d : {1.0, 4.25, 10.0, 18.0}) {
    EXPECT_NEAR(carbonModel(vertex - d, 1E-4, vertex, 0.0), tableValue(carbon, d, 0.0), 1E-2);
  }
  EXPECT_DOUBLE_EQ(carbonModel(vertex + 5, 1E-4, vertex, 0.0), 0.0);
}
