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
  double params[8] = {sigma, vertex, alphaOffset, 0.0, 1.0, 0.0, 1.0, 1.0};
  return dEdxFitter::bragg_alpha(&x, params);
}

double carbonModel(double x, double sigma, double vertex, double carbonOffset) {
  double params[8] = {sigma, vertex, 0.0, carbonOffset, 0.0, 1.0, 1.0, 1.0};
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
  // the model tests set the carbon range directly: physics constraint off
  void SetUp() override {
    fitter = std::make_unique<dEdxFitter>(RESOURCES_DIR, 250);
    dEdxFitter::setPhysicsConstraint(false);
  }
  void TearDown() override { dEdxFitter::setPhysicsConstraint(true); }
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

namespace {
// charge profile of a C12+alpha event, as made by TrackBuilder: x from the
// track start, histogram range 1.2 x track length
TH1F makeProfile(const char *name, double vertex, double alphaRange, double carbonRange, double pressure) {
  const double scale = 250.0 / pressure;  // nominal/current pressure
  const double maxAlpha = 297.23 * scale, maxCarbon = 23.43 * scale;
  const double length = vertex + alphaRange;
  TH1F h(name, "", int(1.2 * length), 0, 1.2 * length);
  double params[8] = {1.0, vertex, maxAlpha - alphaRange, maxCarbon - carbonRange, 1.0, 1.0, 5E-3, 1.0};
  for (int i = 1; i <= h.GetNbinsX(); ++i) {
    double x = h.GetBinCenter(i);
    // deterministic "noise" so that the fit has a non-zero loss
    h.SetBinContent(i, dEdxFitter::bragg_12C_alpha(&x, params) * (1.0 + 0.05 * std::sin(1.7 * i)));
  }
  return h;
}

struct FitOutput {
  double loss, vertex, alphaRange, carbonRange, sigma;
  pid_type type;
};

FitOutput fit(dEdxFitter &aFitter, const TH1F &h) {
  aFitter.setDiffusionRange(0.5, 2.0);
  aFitter.fitHisto(h);
  return {aFitter.getLoss(), aFitter.getVertexOffset(), aFitter.getAlphaRange(),
          aFitter.getCarbonRange(), aFitter.getDiffusion(), aFitter.getBestFitEventType()};
}

void expectSame(const FitOutput &a, const FitOutput &b) {
  EXPECT_EQ(a.type, b.type);
  EXPECT_DOUBLE_EQ(a.loss, b.loss);
  EXPECT_DOUBLE_EQ(a.vertex, b.vertex);
  EXPECT_DOUBLE_EQ(a.alphaRange, b.alphaRange);
  EXPECT_DOUBLE_EQ(a.carbonRange, b.carbonRange);
  EXPECT_DOUBLE_EQ(a.sigma, b.sigma);
}
}  // namespace

TEST_F(dEdxFitterTest, FitIsDeterministic) {
  // the result of a fit must not depend on earlier fits: the parameter errors
  // left in the TF1 by a previous fit were used as initial step sizes
  const double pressure = 190.0;
  const TH1F first = makeProfile("first", 12.0, 45.0, 9.0, pressure);
  const TH1F second = makeProfile("second", 8.0, 70.0, 12.0, pressure);

  dEdxFitter fresh(RESOURCES_DIR, pressure);
  const FitOutput reference = fit(fresh, second);

  dEdxFitter used(RESOURCES_DIR, pressure);
  fit(used, first);
  expectSame(fit(used, second), reference);   // after another event
  expectSame(fit(used, second), reference);   // repeated
  expectSame(fit(fresh, second), reference);  // refit with the same fitter

  // the same with several fit attempts
  dEdxFitter freshMulti(RESOURCES_DIR, pressure), usedMulti(RESOURCES_DIR, pressure);
  freshMulti.setMaxFitAttempts(10);
  usedMulti.setMaxFitAttempts(10);
  const FitOutput referenceMulti = fit(freshMulti, second);
  fit(usedMulti, first);
  expectSame(fit(usedMulti, second), referenceMulti);
}

TEST_F(dEdxFitterTest, VertexConstraint) {
  // the constraint was overwritten by the initial parameters set before each fit
  const double pressure = 190.0;
  const TH1F profile = makeProfile("profile", 12.0, 45.0, 9.0, pressure);
  dEdxFitter aFitter(RESOURCES_DIR, pressure);
  for (double constraint : {5.0, 20.0}) {
    aFitter.setVertexConstraint(constraint);
    const FitOutput result = fit(aFitter, profile);
    // the C12+alpha fit respects the constraint (with a wrong constraint the
    // alpha hypothesis may be selected, so check the C12+alpha model itself)
    (void)result;
    const double vertex = aFitter.get12CAlphaIonisation()->GetParameter(1);
    EXPECT_GE(vertex, constraint - 2.0 - 1E-6) << "constraint " << constraint;
    EXPECT_LE(vertex, constraint + 2.0 + 1E-6) << "constraint " << constraint;
  }
  aFitter.clearVertexConstraint();
  dEdxFitter fresh(RESOURCES_DIR, pressure);
  expectSame(fit(aFitter, profile), fit(fresh, profile));
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

TEST_F(dEdxFitterTest, FitDoesNotDependOnOrientation) {
  // both orientations are fitted: a reflected profile gives the same result
  // (the orientation was guessed from the position of the maximum)
  const double pressure = 130.0;
  const TH1F profile = makeProfile("profile", 6.0, 44.0, 5.0, pressure);
  TH1F reflected(profile);
  reflected.SetDirectory(0);
  for (int i = 1; i <= profile.GetNbinsX(); ++i)
    reflected.SetBinContent(profile.GetNbinsX() + 1 - i, profile.GetBinContent(i));
  dEdxFitter aFitter(RESOURCES_DIR, pressure);
  const FitOutput a = fit(aFitter, profile), b = fit(aFitter, reflected);
  EXPECT_EQ(a.type, b.type);
  EXPECT_NEAR(a.loss, b.loss, 1E-6 * (1 + a.loss));
  EXPECT_NEAR(a.alphaRange, b.alphaRange, 1E-3);
  EXPECT_NEAR(a.carbonRange, b.carbonRange, 1E-3);
}

TEST_F(dEdxFitterTest, PhysicsConstraint) {
  // C12+alpha profile with the carbon range from momentum conservation;
  // with the constraint the fitted carbon range is within +-10% of it
  const double pressure = 130.0;
  dEdxFitter aFitter(RESOURCES_DIR, pressure);
  const double alphaRange = 44.0;
  const double carbonRange = dEdxFitter::carbonRangeFromAlpha(alphaRange);
  EXPECT_GT(carbonRange, 1.0);
  EXPECT_LT(carbonRange, 0.3 * alphaRange);  // E_C = E_alpha/3, heavier ion
  const TH1F profile = makeProfile("profile", 6.0, alphaRange, carbonRange, pressure);

  dEdxFitter::setPhysicsConstraint(true, 0.1);
  const FitOutput result = fit(aFitter, profile);
  EXPECT_EQ(result.type, pid_type::C12_ALPHA);
  EXPECT_NEAR(result.alphaRange, alphaRange, 2.0);
  EXPECT_NEAR(result.carbonRange, dEdxFitter::carbonRangeFromAlpha(result.alphaRange), 0.1 * result.carbonRange + 1E-6);
  EXPECT_NEAR(result.carbonRange, carbonRange, 0.15 * carbonRange);
}
