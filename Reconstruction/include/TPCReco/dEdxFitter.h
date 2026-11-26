#ifndef _dEdxFitter_H_
#define _dEdxFitter_H_

#include <memory>
#include <string>
#include <vector>

#include <TH1D.h>
#include <TGraph.h>
#include <TF1.h>
#include <TF1Convolution.h>
#include <TFitResult.h>

#include "TPCReco/CommonDefinitions.h"

class IonRangeCalculator;

class dEdxFitter{

public:
  dEdxFitter(std::string resources, double aPressure=190, double aTemperature=nominalTemperature);
  // defaults resource directory to installed directory
  dEdxFitter(double aPressure=190, double aTemperature=nominalTemperature);

  /// Gas conditions: the dE/dx tables are scaled with the gas density, p/T.
  void setGasConditions(double aPressure, double aTemperature);
  void setPressure(double aPressure) { setGasConditions(aPressure, currentTemperature);}
  void setTemperature(double aTemperature) { setGasConditions(currentPressure, aTemperature);}

  /// Ranges [mm] at the current gas conditions of the energies deposited by the full
  /// alpha and carbon dE/dx tables (range-table range of the integral of the table).
  static double getMaxAlphaRange() { return theMaxAlphaRange;}
  static double getMaxCarbonRange() { return theMaxCarbonRange;}

  /// Set range of fitted effective charge diffusion
  void setDiffusionRange(double minD=0.5, double maxD=1.5);

  /// Constrain the vertex position of the C12+alpha hypothesis to
  /// vxtOffset +- halfWidth (position along the charge profile, mm)
  void setVertexConstraint(double vxtOffset, double halfWidth=2.0);

  /// Orientations of the profile fitted by fitHisto: -1 both (default), 0 only the profile as
  /// given, 1 only the reflected profile.
  void setOrientation(int anOrientation) { forcedOrientation = anOrientation; }

  /// fitHisto fits the alpha hypothesis only (no 12C + alpha), e.g. one track of a 3-alpha event
  void setAlphaOnly(bool flag) { alphaOnly = flag; }

  /// Remove the vertex constraint
  void clearVertexConstraint() { hasVertexConstraint = false;}

  /// Number of fit attempts per starting point of a hypothesis, with different
  /// initial step sizes (1 to 10, default 1)
  void setMaxFitAttempts(unsigned int nAttempts);

  /// Restarts stop at the first valid fit with loss/Q^2 <= maxLossRatio (Q: integral of the
  /// charge profile normalised to its maximum).
  void setMaxLossRatio(double aRatio) { maxLossRatio = aRatio; }

  double getMaxLossRatio() const { return maxLossRatio; }

  /// Minuit ErrorDef of the fits (default defaultErrorDef). The loss is the sum of squared
  /// residuals of the profile normalised to its maximum, ~5E-4 per bin for a good fit: with the
  /// ROOT default ErrorDef=1 Migrad stopped before the minimum (EDM < 2E-3 x tolerance x ErrorDef).
  void setErrorDef(double anErrorDef) { errorDef = anErrorDef; }
  double getErrorDef() const { return errorDef; }

  /// Kinematics of the two prong (recoil + alpha) hypothesis: free recoil
  /// range, or the recoil range from momentum conservation (equal momenta,
  /// E_recoil = E_alpha*m_alpha/m_recoil) in O16 -> C12 + alpha or
  /// O18 -> C14 + alpha, within +- margin.
  enum class Kinematics {FREE, O16, O18};

  /// Physics constraint (default: off, margin 10%): the two prong hypothesis is
  /// fitted three times, with FREE, O16 and O18 kinematics, and the best fit
  /// is kept. Off: FREE only.
  static void setPhysicsConstraint(bool enable, double margin=0.1);

  /// Kinematics used by the model functions (bragg_12C, bragg_12C_alpha) when they are evaluated
  /// outside of fitHisto, which leaves it at the kinematics of the best fit.
  static void setModelKinematics(Kinematics aKinematics) { currentKinematics = aKinematics; }
  static bool getPhysicsConstraint() { return usePhysicsConstraint;}

  /// Recoil range [mm] for a given alpha range [mm] (current pressure) from
  /// the momentum conservation for given kinematics (0 for FREE). The C14 dE/dx
  /// shape and range curve are those of C12 (as in IonRangeCalculator).
  static double recoilRangeFromAlpha(double alphaRange, Kinematics kinematics);
  static double carbonRangeFromAlpha(double alphaRange) { return recoilRangeFromAlpha(alphaRange, Kinematics::O16);}

  /// Kinematics of the selected two prong fit, and the recoil PID
  /// (CARBON_14 for O18, CARBON_12 otherwise)
  Kinematics getKinematics() const { return bestKinematics;}
  pid_type getRecoilPID() const { return bestKinematics==Kinematics::O18 ? pid_type::CARBON_14 : pid_type::CARBON_12;}

  TFitResult fitHisto(const TH1F & aHisto);

  const TH1F & getFittedHisto() const { return theFittedHisto;};

  /// Loss normalised to the number of fitted bins: mean squared residual of the charge
  /// profile normalised to its maximum. Independent of the profile binning.
  double getLoss() const;

  /// Sum of squared residuals over the fitted bins (the minimised function).
  double getRawLoss() const { return theFitResult.MinFcnValue();};

  const TF1 & getFittedModel() const { return *theFittedModel;};

  TF1* getAlphaIonisation() const { return alpha_ionisation;}

  TF1* get12CIonisation() const { return carbon_ionisation;}

  TF1* get12CAlphaIonisation() const { return carbon_alpha_model;}

  TF1* getAlphaModel() const { return alpha_model;}

  bool getIsReflected() const { return isReflected;}

  double getVertexOffset() const;

  double getAlphaRange() const;

  double getCarbonRange() const;

  double getDiffusion() const;
    
  pid_type getBestFitEventType() const { return bestFitEventType;}
  
  
  /// dE/dx curve given by a table (linear between the points) and its
  /// convolution with a Gaussian, computed exactly (closed form per segment).
  struct BraggTemplate {
    std::vector<double> s; ///< positions along the track [mm at nominal pressure]
    std::vector<double> a; ///< dE/dx at the positions
    std::vector<double> k; ///< slope of the segment [s[i], s[i+1]]

    void set(const TGraph &aGraph);

    /// Table values for positions in [cut, end of table] (zero elsewhere:
    /// before the start of the track and after the stopping point), smeared
    /// with a Gaussian of width sigma, at position pos. All in table units.
    double smeared(double pos, double cut, double sigma) const;
  };

  static const BraggTemplate & alphaTemplate() { return theAlphaTemplate;}
  static const BraggTemplate & carbonTemplate() { return theCarbonTemplate;}

private:

  static BraggTemplate theAlphaTemplate;
  static BraggTemplate theCarbonTemplate;

  static TGraph *braggGraph_alpha;
  static TGraph *braggGraph_12C;
  static double currentPressure;
  static double currentTemperature;
  /// gas density at the current conditions / density of the tables
  static double densityScale();
  /// ranges [mm] of the full tables at the nominal conditions (computed from the tables)
  static double theNominalAlphaRange, theNominalCarbonRange;
  static double tableRange(const BraggTemplate &aTemplate, IonRangeCalculator &aCalculator, pid_type ion);

  static bool usePhysicsConstraint;
  static double carbonRangeMargin;
  /// kinematics used by the model functions (set for each fit pass)
  static Kinematics currentKinematics;
  Kinematics bestKinematics{Kinematics::FREE};
  static std::unique_ptr<IonRangeCalculator> theRangeCalculator;
  /// maximal (table) alpha and carbon ranges at the current pressure [mm]
  static double theMaxAlphaRange, theMaxCarbonRange;

  double minVtxOffset{0};
  double maxVtxOffset{0};
  double minAlphaOffset{0};
  double maxAlphaOffset{0};
  double minCarbonOffset{0};
  double maxCarbonOffset{0};
  
  double carbonScale{1};
  
  bool isReflected{false};

  /// lower edge of the fit range: the lower edge of the fitted profile
  double fitRangeMin{-20};

  bool hasVertexConstraint{false};
  double vertexConstraint{0};
  double vertexConstraintHalfWidth{2};
  int forcedOrientation{-1};
  bool alphaOnly{false};
  unsigned int maxFitAttempts{1};
  double maxLossRatio{defaultMaxLossRatio};
  double errorDef{defaultErrorDef};

  /// Starting point of a fit: vertex position along the profile, alpha and carbon ranges [mm].
  /// NaN: the default of reset().
  struct FitStart { double vertex, alphaRange, carbonRange; };
  std::vector<FitStart> fitStarts(const TF1 *fModel, double tkLength) const;
  void applyStart(TF1 *fModel, const FitStart &aStart) const;

  double maxCarbonRange, maxAlphaRange;

  TF1 dummyFunc;
  TF1 *alpha_ionisation{0}; 
  TF1 *carbon_ionisation{0};
  TF1 *alpha_model{0};
  TF1 *carbon_alpha_model{0};

  TFitResult theFitResult;
  TH1F theFittedHisto;
  TH1F emptyHisto;
  TF1 *theFittedModel{0};

  void reset();

  /// Initial step sizes of the free parameters (stored by TF1 as parameter errors):
  /// scale x 10% of the parameter range, 0 for the ROOT default.
  static void setStepSizes(TF1 *fModel, double scale);

public:
  /// Conditions of the dE/dx and range tables
  static constexpr double nominalPressure{250.0};     // [mbar]
  static constexpr double nominalTemperature{293.15}; // [K]

  /// Default Minuit ErrorDef of the fits (see setErrorDef).
  static constexpr double defaultErrorDef{1E-3};
  /// Default stop threshold of the restarts (see setMaxLossRatio).
  static constexpr double defaultMaxLossRatio{1E-5}; // was 5E-4: accepted wrong minima (loss ~0.7 for an 80 mm track)

  /// Models of the charge profile along the track, x in [mm], result in [keV/mm]
  /// (dE/dx table units). Parameters, lengths in [mm] at the current pressure:
  ///  [0] sigma: Gaussian smearing (diffusion), [1] vertex position,
  ///  [2] alphaOffset, [3] carbonOffset: distance from the start of the full
  ///      energy curve (10 MeV alpha, 5 MeV 12C at nominal conditions) to the
  ///      start of the track, i.e. range = getMaxAlphaRange() - offset,
  ///  [4] alpha scale, [5] carbon scale, [6] common scale (bragg_12C_alpha only),
  ///  [7] carbon range scale: with O16 or O18 kinematics the recoil range is
  ///      recoilRangeFromAlpha(alpha range) x [7] and [3] is not used.
  /// The alpha goes from the vertex towards larger x, the carbon towards smaller x.
  static double bragg_alpha(double *x, double *params);
  static double bragg_12C(double *x, double *params);
  static double bragg_12C_alpha(double *x, double *params);
private:

  TH1F reflectHisto(const TH1F &aHisto) const;
  
  TFitResult fitHypothesis(TF1 *fModel, TH1F & aHisto);

  pid_type bestFitEventType{pid_type::UNKNOWN};
};

#endif
