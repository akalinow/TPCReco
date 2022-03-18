#ifndef _dEdxFitter_H_
#define _dEdxFitter_H_

#include <string>
#include <vector>

#include "TH1D.h"
#include "TGraph.h"
#include "TF1.h"
#include "TF1Convolution.h"
#include "TFitResult.h"

class dEdxFitter{

public:

  enum event_type{
		  UNKNOWN=0,
		  C12_ALPHA=1,
		  ALPHA=2
  };

  dEdxFitter();

  TFitResult fitHisto(TH1F & aHisto);

  const TH1F & getFittedHisto() const { return theFittedHisto;};

  const TF1 & getFittedModel() const; { return *theFittedModel;};

  TF1* getAlphaIonisation() const { return alpha_ionisation;}

  TF1* get12CIonisation() const { return carbon_ionisation;}

  TF1* get12CAlphaIonisation() const { return carbon_alpha_ionisation;}

  TF1* getAlphaModel() const { return alphaModel;}

  double getAlphaEnergy() const;

  double getCarbonEnergy() const;
  
  event_type getBestFitEventType() const { return bestFitEventType;}
  
  
private:

  static TGraph *braggGraph_alpha;
  static TGraph *braggGraph_12C;

  TF1 *alpha_ionisation; 
  TF1 *carbon_ionisation;
  TF1 *carbon_alpha_ionisation;

  TF1 *alphaModel;
  TF1 *carbon_alphaModel;

  TF1Convolution *carbon_alpha_ionisation_smeared;
  TF1Convolution *alpha_ionisation_smeared;

  TFitResult theFitResult;
  TH1F theFittedHisto;
  TH1F emptyHisto;
  TF1 *theFittedModel;

  void reset();

  static double bragg_alpha(double *x, double *params); //x in [mm], result in [keV/mm]
  static double bragg_12C(double *x, double *params); //x in [mm], result in [keV/mm]
  static double bragg_12C_alpha(double *x, double *params); //x in [mm], result in ??

  TH1F reflectHisto(const TH1F &aHisto) const;
  
  TFitResult fitHypothesis(TF1 *fModel, TH1F & aHisto);

  event_type bestFitEventType{UNKNOWN};
};

#endif
