#ifndef _dEdxFitter_H_
#define _dEdxFitter_H_

#include <string>
#include <vector>

#include <TH1D.h>
#include <TGraph.h>
#include <TF1.h>
#include <TF1Convolution.h>
#include <TFitResult.h>

#include "TPCReco/CommonDefinitions.h"

class dEdxFitter{

public:
  dEdxFitter(std::string resources, double aPressure=190);
  // defaults resource directory to installed directory
  dEdxFitter(double aPressure=190);

  void setPressure(double aPressure); 

  TFitResult fitHisto(const TH1F & aHisto);

  const TH1F & getFittedHisto() const { return theFittedHisto;};

  double getChi2() const { return theFitResult.MinFcnValue();};

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
  
  
private:

  static TGraph *braggGraph_alpha;
  static TGraph *braggGraph_12C;
  static double currentPressure;
  static double nominalPressure;

  double minVtxOffset{0};
  double maxVtxOffset{0};
  double minAlphaOffset{0};
  double maxAlphaOffset{0};
  double minCarbonOffset{0};
  double maxCarbonOffset{0};
  
  double carbonScale{1};
  
  bool isReflected{false};

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

  static double bragg_alpha(double *x, double *params); //x in [mm], result in [keV/mm]
  static double bragg_12C(double *x, double *params); //x in [mm], result in [keV/mm]
  static double bragg_12C_alpha(double *x, double *params); //x in [mm], result in [keV/mm]

  TH1F reflectHisto(const TH1F &aHisto) const;
  
  TFitResult fitHypothesis(TF1 *fModel, TH1F & aHisto);

  pid_type bestFitEventType{pid_type::UNKNOWN};
};

#endif
