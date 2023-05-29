/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
//
// Analysis class for tuning reconstructed track lengths
// by applying linear correction (scale + offset) in IonRangeCalculator:
//
//    CORRECTED_LENGTH = UNCORRECTED_LENGTH * SCALE + OFFSET.
// 
// Several collections of tracks corresponding to different 2-body decay
// hypotheses can be supplied as input.
// Provides operator() for use with external minimization fitter.
//
// Fitting options:
// - Use nominal gamma beam energy for LAB-to-CMS boost instead of reconstructed event-by-event one
// - Use only the leading track (e.g. PID=ALPHA) for caclulating properties in the CMS frame.
//   The second component will be deduced from energy and momentum conservation.
//   In this case the correction factors will be tuned only for PID corresponding to the leading track.
// - Use only multiplicative scaling factors instead of scale + offset.
//
//
// Mikolaj Cwiok (UW) - 27 May 2023
//
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

#include <vector>

#include <TVector3.h>
#include <TLorentzVector.h>

#include "TPCReco/CommonDefinitions.h"

using namespace ROOT::Math;

class GeometryTPC;
class IonRangeCalculator;

// _______________________________________
//
// function Object to be minimized
//
class EnergyScale_analysis {

 public:
  // _______________________________________
  //
  // helper struct for storing single track info
  typedef struct {
    definitions::pid_type pid;
    double phi_BEAM_LAB; // [rad]
    double theta_BEAM_LAB; // [rad]
    double uncorrected_length; // [mm]
  } TrackData;
  
  // _______________________________________
  //
  // helper type for storing single event info
  typedef std::vector<TrackData> TrackCollection; // collection of SORTED tracks to be fitted (longest first)
  
  // _______________________________________
  //
  // helper struct for storing events that passed cuts for a given reaction hypothesis
  typedef struct {
    std::string cut_name{""}; // unique identifier
    definitions::reaction_type {reaction_type::UNKNOWN}; // hypothesis type to be tested
    double photonEnergyInMeV_LAB{0}; // [MeV] - nominal energy of the gamma beam in LAB reference frame
    double expectedGammaPeakInMeV_LAB{0}; // [MeV] - peak position from theoretical prediction (e.g. resonance)
    double expectedGammaSigmaInMeV_LAB{0}; // [MeV] - peak RMS from theoretical prediction (excluding detector resolution)
    bool excitedRecoilFlag{false}; // TRUE=2nd component of 2-body decay is in excited state (e.g. valid for: C-12 -> ALPHA + Be-8(*) )
    double excitedRecoilEnergyInMeV{0}; // [MeV] - excitation energy above g.s. (not used until excitedRecoilFlag=TRUE)
    std::vector<TrackCollection> events; // selected events for tuning
  } EventCollection;
  
  // _______________________________________
  //
  // helper struct for storing fit options
  typedef struct {
    double tolerance{1e-4}; // ROOT::Fit::Fitter TOLERANCE parameter
    double expectedResolutionInMeV{0.100}; // [MeV] - broadening of gamma energy peak due to detector resolution
    std::vector<pid_type> tuned_pid_list{pid_type::ALPHA}; // list of PIDs to be tuned (affects IonrangeCalculator)
    bool use_scale_only{false}; // TRUE = use only multiplicative correction instead of: scale + offset
    bool use_leading_track_only{true}; // TRUE = use only leading track information to compute observables in the CMS frame
    bool use_nominal_beam_energy{true}; // TRUE = use nominal gamma beam energy for LAB-CMS boosts
  } FitOptionType;
  
  EnergyScale_analysis(const FitOptionType &aOption,
		       const std::vector<EventCollection> &aCollection,
		       const std::shared_ptr<GeometryTPC> &aGeometry,
		       const std::shared_prt<IonRangeCalculator> &arangeCalc); 
  int getNparams() const;
  double getChi2() const;
  double operator() (const double *par);
  
 private:

  void setCalculator(const std::shared_ptr<IonRangeCalculator> &aRangeCalc);
  void reset();
  TVector3 getBetaVectorOfCMS_BEAM(double nucleusMassInMeV, double photonEnergyInMeV_LAB) const; // LAB reference frame, BEAM coordinate system
  double getBetaOfCMS(double nucleusMassInMeV, double photonEnergyInMeV_LAB) const; // LAB reference frame
  double getGammaEnergyInMeV_LAB(double nominalBeamEnergyInMeV_LAB, definitions::reaction_type reaction, TrackCollection &list) const;
  void initializeCorrections(const int npar, const double *par);
  void initializeRmsSum();
  double getSelectionChi2(EventCollection &selection) const;
  
  std::shared_ptr<IonRangeCalculator> myRangeCalc; // external calculator (must be properly initialized beforehand)
  std::shared_ptr<GeometryTPC> myGeometryPtr; // external TPC geometry and run conditions
  int myNparams{2}; // number of parameters to be fitted (minimal=2)
                    // 0 --> length offset [mm] of 1st PID
                    // 1 --> length scale of 1st PID
                    // 2 --> length offset [mm] of 2nd PID
                    // 3 --> length scale of 2nd PID, etc.
  std::vector<bool> isFixedPar;
  std::vector<double> initialPar;
  FitOptionType myOptions;
  std::vector<EventCollection> &myCollection;
};
