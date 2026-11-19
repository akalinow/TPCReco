#ifndef _GainTable_H_
#define _GainTable_H_

#include <string>
#include <vector>

/// Charge gain of the detector per setup (run range and pressure).
///
/// Measured from 16O(gamma,alpha)12C tracks (WarsawTPC_Claude/code/4_length_charge.ipynb, section
/// "Gain table for TPCReco"), with the hit charges corrected by the gain map (GainMap.h):
///  * G_cal [ADC/MeV]: track charge per MeV of calibrated total kinetic energy (charge -> MeV);
///  * G_raw [ADC/MeV]: track charge per MeV of the SRIM energy of the reconstructed alpha and carbon
///    lengths, E_dep = T_alpha(L_alpha) + T_C(L_C). R = Q/(G_raw E_dep) is ~1 for a correct dE/dx fit.
///
/// File format (text): "run_first run_last pressure_mbar G_cal G_raw" per line, run IDs as YYYYMMDDhhmmss
/// (EventInfo::GetRunId); empty lines and text after '#' are ignored. A run matches the first row
/// with run_first <= runId <= run_last and |pressure - pressure_mbar| <= pressureTolerance.
class GainTable {

public:

  struct Entry {
    long runFirst{0}, runLast{0};
    double pressure{0}, gainCal{0}, gainRaw{0};
  };

  GainTable() = default;

  /// Reads the table; throws std::runtime_error if the file cannot be read or is malformed.
  explicit GainTable(const std::string & fileName);

  bool empty() const { return myEntries.empty(); }

  /// Entry for a run and pressure [mbar]; false if there is none.
  bool find(long runId, double pressure, Entry & entry) const;

  const std::vector<Entry> & getEntries() const { return myEntries; }

  const std::string & getFileName() const { return myFileName; }

  static constexpr double pressureTolerance{5.0}; // [mbar]

private:

  std::string myFileName;
  std::vector<Entry> myEntries;
};

#endif
