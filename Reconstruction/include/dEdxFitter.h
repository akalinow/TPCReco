#ifndef _dEdxFitter_H_
#define _dEdxFitter_H_

#include <string>
#include <vector>

#include "TH1D.h"
#include "TGraph.h"
#include "TF1.h"
#include "TF1Convolution.h"
#include "TFitResult.h"

#include "CommonDefinitions.h"

class dEdxFitter{

public:

  dEdxFitter(double aPressure=250);

  void setPressure(double aPressure) {pressure = aPressure;}

  TFitResult fitHisto(TH1F & aHisto);

  const TH1F & getFittedHisto() const { return theFittedHisto;};

  const TF1 & getFittedModel() const { return *theFittedModel;};

  TF1* getAlphaIonisation() const { return alpha_ionisation;}

  TF1* get12CIonisation() const { return carbon_ionisation;}

  TF1* get12CAlphaIonisation() const { return carbon_alpha_ionisation;}

  TF1* getAlphaModel() const { return alphaModel;}

  double getChi2() const { return bestFitChi2;}

  bool getIsReflected() const { return isReflected;}

  double getAlphaEnergy() const;

  double getCarbonEnergy() const;

  double getAlphaRange() const;

  double getCarbonRange() const;
    
  pid_type getBestFitEventType() const { return bestFitEventType;}
  
  
private:

  static TGraph *braggGraph_alpha;
  static TGraph *braggGraph_12C;
  static double pressure;
  bool isReflected{false};
  double bestFitChi2{999.0};

  TF1 dummyFunc;
  TF1 *alpha_ionisation{0}; 
  TF1 *carbon_ionisation{0};
  TF1 *carbon_alpha_ionisation{0};

  TF1 *alphaModel{0};
  TF1 *carbon_alphaModel{0};

  TF1Convolution *carbon_alpha_ionisation_smeared{0};
  TF1Convolution *alpha_ionisation_smeared{0};

  TFitResult theFitResult;
  TH1F theFittedHisto;
  TH1F emptyHisto;
  TF1 *theFittedModel{0};

  void reset();

  static double bragg_alpha(double *x, double *params); //x in [mm], result in [keV/mm]
  static double bragg_12C(double *x, double *params); //x in [mm], result in [keV/mm]
  static double bragg_12C_alpha(double *x, double *params); //x in [mm], result in ??

  TH1F reflectHisto(const TH1F &aHisto) const;
  
  TFitResult fitHypothesis(TF1 *fModel, TH1F & aHisto);

  pid_type bestFitEventType{pid_type::UNKNOWN};
};

#endif
