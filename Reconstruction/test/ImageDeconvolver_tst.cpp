// Tests of ImageDeconvolver: Richardson-Lucy deconvolution of a separable strip x time response
// with a one-sided (shaping-like) time kernel.
#include <cmath>
#include <vector>

#include <gtest/gtest.h>
#include <TH2D.h>

#include "TPCReco/ImageDeconvolver.h"

namespace {

const int nStrips = 60, nCells = 200;

// one-sided time kernel ~ AGET shape exp(-3x) x^3 sin(x), x = t/tau, tau = 6 cells; offsets -30..30
std::vector<double> timeKernel() {
  std::vector<double> k(61, 0.0);
  for (int i = 0; i <= 30; ++i) {
    const double x = (i + 0.5) / 6.0;
    k[30 + i] = x < M_PI ? std::exp(-3 * x) * std::pow(x, 3) * std::sin(x) : 0.0;
  }
  return k;
}
std::vector<double> stripKernel() { return {0.02, 0.12, 0.72, 0.12, 0.02}; }

// image (strip, cell) blurred with the separable kernels; X = time cell, Y = strip
TH2D blurred(const TH2D &truth) {
  const auto kT = timeKernel(), kS = stripKernel();
  double sT = 0, sS = 0;
  for (auto v : kT) sT += v;
  for (auto v : kS) sS += v;
  TH2D out(truth);
  out.Reset();
  for (int ix = 1; ix <= nCells; ++ix)
    for (int iy = 1; iy <= nStrips; ++iy) {
      const double v = truth.GetBinContent(ix, iy);
      if (v == 0) continue;
      for (int j = 0; j < (int)kT.size(); ++j)
        for (int l = 0; l < (int)kS.size(); ++l) {
          const int tx = ix + j - (int)kT.size() / 2, ty = iy + l - (int)kS.size() / 2;
          if (tx >= 1 && tx <= nCells && ty >= 1 && ty <= nStrips)
            out.SetBinContent(tx, ty, out.GetBinContent(tx, ty) + v * kT[j] / sT * kS[l] / sS);
        }
    }
  return out;
}

// truth: a straight track from (cell 60, strip 20) to (cell 120, strip 40) plus a dense blob at its start
TH2D truthImage() {
  TH2D h("hTruth", "", nCells, -0.5, nCells - 0.5, nStrips, 0.5, nStrips + 0.5);
  for (int i = 0; i <= 60; ++i) h.Fill(60 + i, 20 + i / 3.0, 100.0);
  h.Fill(60, 20, 2000.0);
  return h;
}

double meanX(const TH2D &h) { return h.GetMean(1); }

TEST(ImageDeconvolver, NoKernelsReturnsInput) {
  ImageDeconvolver d;
  auto truth = truthImage();
  auto out = d.deconvolve(truth, 0);
  EXPECT_DOUBLE_EQ(out.Integral(), truth.Integral());
  EXPECT_EQ(d.getLastIterations(), 0);
}

TEST(ImageDeconvolver, EmptyImage) {
  ImageDeconvolver d;
  d.setKernels(0, stripKernel(), timeKernel());
  TH2D h("hEmpty", "", nCells, -0.5, nCells - 0.5, nStrips, 0.5, nStrips + 0.5);
  auto out = d.deconvolve(h, 0);
  EXPECT_DOUBLE_EQ(out.Integral(), 0.0);
}

TEST(ImageDeconvolver, RecoversTimeShiftChargeAndWidth) {
  ImageDeconvolver d;
  d.setKernels(1, stripKernel(), timeKernel());
  const auto truth = truthImage();
  const auto data = blurred(truth);
  const auto rec = d.deconvolve(data, 1);

  EXPECT_GT(d.getLastIterations(), 1);
  EXPECT_LT(d.getLastIterations(), 300);
  // total charge kept (small loss from the zero suppression)
  EXPECT_NEAR(rec.Integral() / truth.Integral(), 1.0, 0.03);
  // the one-sided time kernel delays the data by several cells; the deconvolution moves it back
  EXPECT_GT(meanX(data) - meanX(truth), 5.0);
  EXPECT_NEAR(meanX(rec), meanX(truth), 0.5);
  EXPECT_NEAR(rec.GetMean(2), truth.GetMean(2), 0.1);
  // the time profile of the blob strip (20) gets narrower
  auto rms = [](const TH2D &h) {
    auto p = h.ProjectionX("p", 20, 20);
    const double r = p->GetRMS();
    delete p;
    return r;
  };
  EXPECT_LT(rms(rec), 0.8 * rms(data));
}

} // namespace
