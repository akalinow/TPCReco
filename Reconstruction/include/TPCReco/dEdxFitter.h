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
  dEdxFitter(std::string resources, double aPressure=190);
  // defaults resource directory to installed directory
  dEdxFitter(double aPressure=190);

  void setPressure(double aPressure); 

  /// Set range of fitted effective charge diffusion
  void setDiffusionRange(double minD=0.5, double maxD=1.5);

  /// Constrain the vertex position of the C12+alpha hypothesis to
  /// vxtOffset +- 2 mm (position along the charge profile)
  void setVertexConstraint(double vxtOffset);

  /// Remove the vertex constraint
  void clearVertexConstraint() { hasVertexConstraint = false;}

  /// Number of fit attempts per hypothesis, with different initial step
  /// sizes (1 to 10, default 1)
  void setMaxFitAttempts(unsigned int nAttempts);

  /// Physics constraint of the C12+alpha hypothesis (default: on, margin 10%):
  /// the carbon range follows from the alpha range by momentum conservation
  /// in O16 -> C12 + alpha (equal momenta, E_C = E_alpha*m_alpha/m_C12), within
  /// +- margin (fitted scale of the carbon range in [1-margin, 1+margin]).
  /// Off: the carbon range is a free parameter.
  static void setPhysicsConstraint(bool enable, double margin=0.1);
  static bool getPhysicsConstraint() { return usePhysicsConstraint;}

  /// Carbon range [mm] for a given alpha range [mm] (current pressure) from
  /// the momentum conservation in O16 -> C12 + alpha.
  static double carbonRangeFromAlpha(double alphaRange);

  TFitResult fitHisto(const TH1F & aHisto);

  const TH1F & getFittedHisto() const { return theFittedHisto;};

  double getLoss() const { return theFitResult.MinFcnValue();};

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
  static double nominalPressure;

  static bool usePhysicsConstraint;
  static double carbonRangeMargin;
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

  bool hasVertexConstraint{false};
  double vertexConstraint{0};
  unsigned int maxFitAttempts{1};

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
  /// Models of the charge profile along the track, x in [mm], result in [keV/mm]
  /// (dE/dx table units). Parameters, lengths in [mm] at the current pressure:
  ///  [0] sigma: Gaussian smearing (diffusion), [1] vertex position,
  ///  [2] alphaOffset, [3] carbonOffset: distance from the start of the full
  ///      energy curve (10 MeV alpha, 5 MeV 12C at nominal pressure) to the
  ///      start of the track, i.e. range = maximal range - offset,
  ///  [4] alpha scale, [5] carbon scale, [6] common scale (bragg_12C_alpha only),
  ///  [7] carbon range scale: with the physics constraint the carbon range is
  ///      carbonRangeFromAlpha(alpha range) x [7] and [3] is not used.
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
