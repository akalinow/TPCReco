#ifndef _GainMap_H_
#define _GainMap_H_

#include <string>
#include <vector>

/// Relative gain of the readout plane as a function of the DET X coordinate.
///
/// The map is measured from the collected charge of tracks with known energy deposits
/// (WarsawTPC_Claude/code/gain_x_map.py). It is applied per hit after the 3D track fit
/// (charge profile for the dE/dx fit and the integrated charge): the charge of a hit at
/// X_DET is divided by getGain(X_DET).
///
/// File format (text): one bin per line, "x_lo x_hi gain" with x in [mm] (DET frame);
/// empty lines and lines starting with '#' are ignored. Bins must be ordered in x.
/// The gain is interpolated linearly between bin centres and is constant beyond the
/// outermost centres.
class GainMap {

public:

  GainMap() = default;

  /// Reads the map; throws std::runtime_error if the file cannot be read or is malformed.
  explicit GainMap(const std::string & fileName);

  /// Map from bin centres and gains (centres strictly increasing, gains > 0).
  GainMap(const std::vector<double> & xCentres, const std::vector<double> & gains);

  bool empty() const { return myGains.empty(); }

  /// Relative gain at X_DET [mm]; 1 for an empty map.
  double getGain(double x) const;

  const std::string & getFileName() const { return myFileName; }

private:

  void validate() const;

  std::string myFileName;
  std::vector<double> myCentres;
  std::vector<double> myGains;
};

#endif
