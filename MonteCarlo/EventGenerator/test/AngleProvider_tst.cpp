#include "TPCReco/AngleProvider.h"
#include "TPCReco/Provider.h"
#include "TRandom.h"
#include "TF1.h"
#include <cmath>
#include <memory>
#include "gtest/gtest.h"

namespace {
// mean of cos^2(theta) of n angles from AngleProviderE1E2
double meanCos2(double sE1, double sE2, int n = 200000) {
    gRandom->SetSeed(12345);
    AngleProviderE1E2 p;
    p.SetParams({{"sigmaE1", sE1}, {"sigmaE2", sE2}, {"phaseE1E2", M_PI / 2}, {"phaseCosSign", 1}});
    double sum = 0;
    for (int i = 0; i < n; i++) { double c = std::cos(p.GetAngle()); sum += c * c; }
    return sum / n;
}
}

// W(theta) is a distribution per solid angle: pure E1 ~ sin^2 -> <cos^2> = 1/5; pure E2 ~ sin^2 cos^2 -> <cos^2> = 3/7
TEST(AngleProviderE1E2, PureE1) { EXPECT_NEAR(meanCos2(1, 0), 1. / 5., 0.004); }
TEST(AngleProviderE1E2, PureE2) { EXPECT_NEAR(meanCos2(0, 1), 3. / 7., 0.004); }

// 1 + f cos(2(phi - phi0)): <cos(2(phi - phi0))> = f/2
TEST(AngleProviderPhi, Polarization) {
    gRandom->SetSeed(12345);
    AngleProviderPhi p;
    const double f = 0.36, phi0 = 0.7;
    p.SetParams({{"polDegree", f}, {"polAngle", phi0}});
    const int n = 200000;
    double sum = 0;
    for (int i = 0; i < n; i++) sum += std::cos(2 * (p.GetAngle() - phi0));
    EXPECT_NEAR(sum / n, f / 2, 0.005);
}
