#ifndef TPCRECO_ANALYSIS_ENERGY_SCALE_ANALYSIS_H_
#define TPCRECO_ANALYSIS_ENERGY_SCALE_ANALYSIS_H_
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
//
// Analysis class for tuning reconstructed track lengths and/or energies
// by applying corrections on top of default IonRangeCalculator (i.e. initialized with length corrections:
// scale_length=1, offset_length=0). The following 3 correction types are implemented:
//
// 1. Linear re-scaling of track lengths per PID:
//       LEN_TRUE(LEN_MEAS) = LSCALE * LEN_MEAS + LOFFSET[mm]
// or:   LEN_MEAS(LEN_TRUE) = (LEN_TRUE - LOFFSET[mm]) / LSCALE
//
// 2. Linear re-scaling of track energies in CMS per PID:
//       EKIN_TRUE(EKIN_MEAS) = ESCALE * EKIN_MEAS + EOFFSET[MeV]
// or:   EKIN_MEAS(EKIN_TRUE) = (EKIN_TRUE - EOFFSET[MeV]) / ESCALE
//
// 3. Re-scaling both track energies and lengths in LAB per PID using Zenek's formula:
//      EKIN_TRUE( LEN_MEAS ) = ESCALE * EKIN0_SRIM( R0(LEN_MEAS) + LOFFSET[mm] ) [MeV]
// or:  EKIN_TRUE( EKIN_MEAS ) = ESCALE * ENERGY( LEN_MEAS + LOFFSET * T/T0 * p0/p )
//      EKIN_MEAS( EKIN_TRUE ) = ENERGY( RANGE( EKIN_TRUE/ESCALE ) - LOFFSET * p/p0 * T0/T )
//
// where:
//      LEN_MEAS = measured range at (p,T)
//      R0(LEN_MEAS) = LEN_MEAS * T0/T * p/p0 = reduced measured range corresponding to reference (p0,T0)
//      RANGE0_SRIM( EKIN_MEAS(LEN_MEAS) ) = R0
//      ENERGY(LENGTH) = EKIN0_SRIM( LENGTH * T0/T * p/p0 ) = kinetic energy curve at (p,T)
//      RANGE(ENERGY) = p0/p * T/T0 * RANGE0_SRIM(ENERGY) = range curve at (p,T)
// 
// Several collections of tracks corresponding to different 2-body decay
// hypotheses can be supplied as input.
// The class provides operator() for use with external minimization fitter, such as: MINUIT2/MIGRAD2, MINUIT2/FUMULI2.
//
// Fitting options:
// - Type of correction to be applied: length, kin.energy in LAB, kin.energy in CMS, Zenek's formula.
// - Use nominal gamma beam energy for LAB-to-CMS boost instead of reconstructed event-by-event one
// - Use only the leading track (e.g. PID=ALPHA) for caclulating properties in the CMS frame.
//   The second component will be deduced from energy and momentum conservation.
//   In this case the correction factors will be tuned only for PID corresponding to the leading track
//   (NOTE: this option is ignored for three-alpha "democratic" decay hypothesis)
// - Use only multiplicative scaling factors instead of scale + offset (i.e. fix offset=0).
//
//
// Mikolaj Cwiok (UW) - 5 July 2023
//
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

#include <vector>
#include <map>
#include <string>

#include <TVector3.h>
#include <TLorentzVector.h>
#include <TH1D.h>
#include <TF1.h>

#include "TPCReco/CommonDefinitions.h"

class IonRangeCalculator;

enum class escale_type{  // TO BE MOVED TO CommonDefinitins.h !!!!
  UNKNOWN=0,
  NONE,
  LENGTH,
  ENERGY_LAB,
  ENERGY_CMS,
  ZENEK
};

// _______________________________________
//
// function Object to be minimized
//
class EnergyScale_analysis {

 public:

  // _______________________________________
  //
  // helper struct for storing info about single track
  struct TrackData {
    pid_type pid;
    double phi_BEAM_LAB; // [rad]
    double theta_BEAM_LAB; // [rad]
    double length_uncorrected; // [mm]
  };
  
  // _______________________________________
  //
  // helper type for storing info about single event
  typedef std::vector<TrackData> TrackCollection;
  
  // _______________________________________
  //
  // helper struct for storing events that passed cuts for a given reaction hypothesis
  struct EventCollection {
    bool enabled{true}; // TRUE = use these data in global fit / FALSE = use these data only as a cross check
    std::string description{""}; // unique identifier
    reaction_type reaction{reaction_type::UNKNOWN}; // hypothesis type to be tested
    double photonEnergyInMeV_LAB{0}; // [MeV] - nominal energy of the gamma beam in LAB reference frame (eg. for nominal boost)
    double expectedExcitationEnergyPeakInMeV{0}; // [MeV] - excitation energy peak position from theoretical prediction (e.g. resonance)
    double expectedExcitationEnergySigmaInMeV{0}; // [MeV] - predicted peak width (sigma) excluding detector resolution effects
    double excitedMassDiffInMeV{0}; // [MeV/c^2] - excitation energy of the 2nd component in intermediate 2-body decay,
                                    //             eg. C-12 -> ALPHA + Be-8(*)
    std::shared_ptr<IonRangeCalculator> rangeCalc; // external IonRangeCalculator initialized for a given (p,T)
    std::vector<TrackCollection> events; // selected events for tuning
  };
  
  // _______________________________________
  //
  // helper struct for storing fit options
  struct FitOptionType {
    double tolerance{10}; // ROOT::Fit::Fitter TOLERANCE parameter
    double precision{1e-6}; // ROOT::Fit::Fitter PRECISION parameter
    double detectorExcitationEnergyResolutionInMeV{0.100}; // [MeV] - broadening of Ex peak due to detector resolution
    std::map<std::string, std::vector<pid_type>> tuned_pid_map; // index=(PID category name), value=(list of PIDs with same corrections)
    bool use_scale_only{false}; // TRUE = use only multiplicative correction instead of: scale + offset
    bool use_leading_track_only{true}; // TRUE = use only leading track information to compute observables in the CMS frame
    bool use_nominal_beam_energy{true}; // TRUE = use nominal gamma beam energy for LAB-CMS boosts
    bool assignMissingPIDs{true}; // TRUE = use best guess according to reaction hypothesis in case of mising PIDs
    escale_type correction_type{escale_type::NONE}; // scale track energy or length or none
    bool debug{false};
  };
  struct ResidualsType {
    TH1D dataHist;     // data sample Ex spectrum [MeV]
    TF1  fittedFunc;   // fitted Ex shape [MeV]
    double expectedEx; // expected Ex peak position [MeV]
    bool enabled;      // TRUE = sample included in the global fit / FALSE = control sample not included in the global fit
  };

  EnergyScale_analysis(const FitOptionType &aOption,
		       std::vector<EventCollection> &aSelection);

  size_t getNparams() const;
  size_t getNpoints() const;
  double getParameter(size_t ipar) const;
  bool isParameterFixed(size_t ipar) const;
 private:
  double getChi2(const bool debug_histos_flag);
 public:
  inline double getChi2() { return getChi2(false); } // do not expose internal parameter
  double operator() (const double *par);
  void plotExcitationEnergyFits(const std::string &filePrefix="ExcitationEnergyFits");
  void setMinimizerAlgorithm(const std::string &algorithm="MIGRAD");
  
 private:

  void reset();
  TVector3 getBetaVectorOfCMS_BEAM(double nucleusMassInMeV, double photonEnergyInMeV_LAB) const; // LAB reference frame, BEAM coordinate system
  double getBetaOfCMS(double nucleusMassInMeV, double photonEnergyInMeV_LAB) const; // LAB reference frame
  TH1D getOptimizedExcitationEnergyHistogram(EventCollection &selection);

  std::tuple<double, double, bool> getEventExcitationEnergy(double nominalBeamEnergyInMeV_LAB,
							    double expectedExcitationEnergyPeakInMeV_CMS,
							    reaction_type reaction,
							    double excitedMassDiffInMeV,
							    std::shared_ptr<IonRangeCalculator> rangeCalc,
							    TrackCollection &list) const;
  void initializeCorrections(const size_t npar, const double *par);
  double getCorrectionPerPID(pid_type pid, int ipar) const; // par[0]=offset [MeV] or [mm], par[1]=scale for LENGTH or ENERGY
  std::tuple<double, size_t> getSelectionChi2(EventCollection &selection, bool debug_histos_flag=false);
  double applyLinearCorrectionPerPID(pid_type pid, double observable) const;
  
  size_t myNparams{2}; // number of parameters to be fitted (minimal=2)
                       // 0 --> length offset [mm] of 1st PID
                       // 1 --> length scale of 1st PID
                       // 2 --> length offset [mm] of 2nd PID
                       // 3 --> length scale of 2nd PID, etc.
  std::vector<bool> isFixedPar;
  std::vector<double> initialPar;
  std::vector<double> lastPar;
  FitOptionType myOptions;
  std::vector<EventCollection> &mySelection;
  std::map<pid_type, std::tuple<double, double>> myCorrectionMap; // index = PID, value={OFFSET, SCALE} for LENGTH or ENERGY
  std::map<std::string, ResidualsType> myExcitationEnergyFitMap; // index = unique selection name
  std::map<std::string, double> myExcitationEnergyBinSizeMap; // index = unique selection name
  double myFactorChi2{1.0}; // additional scaling required by certain ROOT minimization algorithms
};

///////////////////////////////////////////////////////////////////// - TO BE MOVED TO CommonDefinitions
///////////////////////////////////////////////////////////////////// - TO BE MOVED TO CommonDefinitions
namespace enumDict {
    //conversion enum <--> string for escale_type
    escale_type GetEnergyScaleType(const std::string &scaleName);
    std::string GetEnergyScaleName(escale_type type);
}
///////////////////////////////////////////////////////////////////// - TO BE MOVED TO CommonDefinitions
///////////////////////////////////////////////////////////////////// - TO BE MOVED TO CommonDefinitions

#endif // TPCRECO_ANALYSIS_ENERGY_SCALE_ANALYSIS_H_
