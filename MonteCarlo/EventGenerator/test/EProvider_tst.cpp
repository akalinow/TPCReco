#include "TPCReco/EProvider.h"
#include "TPCReco/Provider.h"
#include "TRandom.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include "gtest/gtest.h"

namespace {
std::vector<double> sample(double meanE, double sigmaE, double resE, double width, int n = 100000) {
    gRandom->SetSeed(12345);
    EProviderGausBW p;
    p.SetParams({{"meanE", meanE}, {"sigmaE", sigmaE}, {"resonanceE", resE}, {"resonanceWidth", width}});
    std::vector<double> e(n);
    for (auto &x: e) x = p.GetEnergy();
    std::sort(e.begin(), e.end());
    return e;
}
}

// narrow resonance (0.6 keV, as 16O 2+ at 9.84 MeV) in a 0.18 MeV beam: all reactions at the resonance
TEST(EProviderGausBW, NarrowResonance) {
    auto e = sample(9.875, 0.183, 9.848, 0.000625, 20000);
    EXPECT_NEAR(e[e.size() / 2], 9.848, 0.0005);
    EXPECT_NEAR(e[3 * e.size() / 4] - e[e.size() / 4], 0.000625, 0.0002); // interquartile range of a Lorentzian = Gamma
}

// resonance much wider than the beam: the beam Gaussian
TEST(EProviderGausBW, WideResonance) {
    auto e = sample(13.06, 0.2, 13.096, 100.0);
    double mean = 0, var = 0;
    for (double x: e) mean += x;
    mean /= e.size();
    for (double x: e) var += (x - mean) * (x - mean);
    EXPECT_NEAR(mean, 13.06, 0.005);
    EXPECT_NEAR(std::sqrt(var / e.size()), 0.2, 0.005);
}

// 16O 1- at 13.09 MeV (Gamma 130 keV) in a 0.2 MeV beam centred 36 keV below: median between the two, much narrower
TEST(EProviderGausBW, Resonance13MeV) {
    auto e = sample(13.06, 0.2, 13.096, 0.130);
    double median = e[e.size() / 2], iqr = e[3 * e.size() / 4] - e[e.size() / 4];
    EXPECT_GT(median, 13.06);
    EXPECT_LT(median, 13.096);
    EXPECT_LT(iqr, 0.130);
}
