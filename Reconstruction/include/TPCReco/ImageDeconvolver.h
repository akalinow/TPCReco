#ifndef _ImageDeconvolver_H_
#define _ImageDeconvolver_H_

#include <string>
#include <vector>
#include <array>

#include <TH2D.h>

/// Richardson-Lucy deconvolution of the detector response from a strip x time projection
/// (e.g. the clean cluster used by the track fit, RecHitBuilder::makeCleanCluster).
///
/// The response (point spread function) is separable: strip kernel (horizontal diffusion + pad geometry,
/// averaged over the charge position within a strip) x time kernel (vertical diffusion convolved with the
/// AGET shaping). Both are read from a StripResponseCalculator file (StripResponseModel_*.root, the one used
/// by the MC digitizer for the same sampling rate, drift velocity and peaking time), or set directly.
///
/// The histogram bins are used as they are: X = time cell, Y = strip (mm or raw scale, one bin per cell/strip).
/// The time kernel is one sided (shaping only delays the charge), so the deconvolved image is earlier in time
/// by the mean AGET delay (~7.75 time cells for 232 ns at 25 MHz).
///
/// Iterations stop when the rms data residual |K*x - y| improves by less than stopEps (relative) per
/// iteration, then the image is smoothed with a Gaussian of the target resolution (the RL "sieve").
/// MC study (graw_deconvolution/MC_image_deconvolution.ipynb): stopEps=0.02, target (1 cell, 0.5 strip).
class ImageDeconvolver {

public:

  ImageDeconvolver() = default;

  /// read kernels from a StripResponseModel ROOT file; returns false (and stays disabled) on error
  bool loadResponseFile(const std::string & fileName);

  /// kernels for a strip direction (U=0, V=1, W=2); odd sizes, centred on the middle element; normalised here
  void setKernels(int dir, const std::vector<double> & stripKernel, const std::vector<double> & timeKernel);

  void setStopEps(double eps) { stopEps = eps; }
  void setMaxIterations(int n) { maxIterations = n; }
  void setTargetSigma(double timeBins, double stripBins) { targetSigmaTime = timeBins; targetSigmaStrip = stripBins; }
  void setZeroFraction(double f) { zeroFraction = f; }
  /// additional Gaussian spread [mm] added to the response kernels, e.g. the drift diffusion D*sqrt(d) at a typical
  /// drift distance d (not in the StripResponseModel file). Converted to bins with the bin widths of the deconvolved
  /// histogram (mm scale). 0: none.
  void setExtraSigmaMM(double timeMM, double stripMM) { extraSigmaTimeMM = timeMM; extraSigmaStripMM = stripMM; }
  void setMargins(int timeBins, int stripBins) { marginTime = timeBins; marginStrip = stripBins; }

  bool hasKernels(int dir) const;

  /// deconvolved copy of the histogram (same binning). Works on the bounding box of the non-zero bins
  /// extended by the margins; bins below zeroFraction x maximum are set to 0. Without kernels for dir
  /// the input is returned unchanged.
  TH2D deconvolve(const TH2D & hInput, int dir);

  int getLastIterations() const { return lastIterations; }

  /// Richardson-Lucy on a plain image, row-major [nStrip][nTime]; returns the number of iterations done
  int richardsonLucy(std::vector<double> & image, int nStrip, int nTime, int dir) const;

private:

  struct Kernel {
    std::vector<double> values;
    int centre{0};  // index of the zero offset
    int first{0};   // first index with a non-negligible value
    int last{-1};   // last index with a non-negligible value
  };
  static Kernel makeKernel(const std::vector<double> & values);

  /// separable convolution (flip=false) or correlation (flip=true) with zero boundary
  void convolve(const std::vector<double> & in, std::vector<double> & out, int nStrip, int nTime,
                int dir, bool flip) const;
  static void gaussianSmooth(std::vector<double> & image, int nStrip, int nTime, double sigmaStrip, double sigmaTime);
  static void convolve1D(const std::vector<double> & in, std::vector<double> & out, int nStrip, int nTime,
                         const Kernel & k, bool alongTime, bool flip);

  /// effective kernels = base kernels convolved with the extra Gaussian (sigma in bins)
  void updateKernels(int dir, double sigmaTimeBins, double sigmaStripBins);
  static std::vector<double> broaden(const std::vector<double> & values, double sigma);

  std::array<std::vector<double>, 3> baseStripKernels;
  std::array<std::vector<double>, 3> baseTimeKernels;
  std::array<Kernel, 3> stripKernels;   // effective
  std::array<Kernel, 3> timeKernels;    // effective
  std::array<double, 3> kernelSigmaTime{{0, 0, 0}};   // extra sigma [bins] of the effective kernels
  std::array<double, 3> kernelSigmaStrip{{0, 0, 0}};

  double extraSigmaTimeMM{0};
  double extraSigmaStripMM{0};

  double stopEps{0.02};
  int maxIterations{300};
  double targetSigmaTime{1.0};   // [time bins]
  double targetSigmaStrip{0.5};  // [strip bins]
  double zeroFraction{0.01};
  int marginTime{25};
  int marginStrip{5};
  int lastIterations{0};
};

#endif
