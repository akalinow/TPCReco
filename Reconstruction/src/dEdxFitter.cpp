#include "dEdxFitter.h"
#include "colorText.h"

#include "TFitResultPtr.h"
#include "Math/MinimizerOptions.h"

TGraph* dEdxFitter::braggGraph_alpha = new TGraph("dEdx_alpha_10000keV_190mbar_CO2.dat", "%lg %lg %*lg");
TGraph* dEdxFitter::braggGraph_12C = new TGraph("dEdx_12C_5000keV_190mbar_CO2.dat", "%lg %lg %*lg");

double dEdxFitter::currentPressure = 190.0;
double dEdxFitter::nominalPressure = 190.0;

////////////////////////////////////////////////
////////////////////////////////////////////////
dEdxFitter::dEdxFitter(double aPressure){

  setPressure(aPressure);

  braggGraph_alpha->SetBit(TGraph::kIsSortedX);
  braggGraph_alpha->SetBit(TGraph::kIsSortedX);

  alpha_ionisation = new TF1("alpha_ionisation", bragg_alpha, 0, 600.0);
  carbon_ionisation = new TF1("carbon_ionisation",bragg_12C, 0,  200.0);

  carbon_alpha_ionisation = new TF1("carbon_alpha_ionisation",bragg_12C_alpha, -20, 350.0, 5);

  carbon_alpha_ionisation_smeared = new TF1Convolution("carbon_alpha_ionisation", "gausn",-20, 350, true);
  carbon_alpha_ionisation_smeared->SetRange(-20, 350);
  carbon_alpha_ionisation_smeared->SetNofPointsFFT(1000);
 
  carbon_alphaModel = new TF1("carbon_alphaModel", *carbon_alpha_ionisation_smeared,
			      -20, 350., carbon_alpha_ionisation_smeared->GetNpar());
    
  carbon_alphaModel->SetParName(0, "vertexOffset");
  carbon_alphaModel->SetParName(1, "alphaOffset");
  carbon_alphaModel->SetParName(2, "carbonOffset");
  carbon_alphaModel->SetParName(3, "alphaScale");
  carbon_alphaModel->SetParName(4, "carbonScale");
  carbon_alphaModel->SetParName(5, "gaussNorm");
  carbon_alphaModel->SetParName(6, "gaussMean");
  carbon_alphaModel->SetParName(7, "gaussSigma");

  carbon_alphaModel->SetParLimits(0, minVtxOffset, maxVtxOffset);
  carbon_alphaModel->SetParLimits(1, minAlphaOffset, maxAlphaOffset);
  carbon_alphaModel->SetParLimits(2, minCarbonOffset, maxCarbonOffset);
  carbon_alphaModel->FixParameter(3, 1.0);
  carbon_alphaModel->FixParameter(4, 1.0);  
  carbon_alphaModel->SetParLimits(5, 1E-6, 2E-5);
  carbon_alphaModel->FixParameter(6, 0.0);
  carbon_alphaModel->SetParLimits(7, 1.0, 3.0);
 
  alphaModel = new TF1(*carbon_alphaModel);
  alphaModel->SetName("alphaModel");
  alphaModel->FixParameter(2, 0.0);
  alphaModel->FixParameter(4, 0.0);
  
  ROOT::Math::MinimizerOptions::SetDefaultMinimizer("Minuit2");
  //ROOT::Math::MinimizerOptions::SetDefaultMinimizer("Minuit" ,"Scan");
  //ROOT::Math::MinimizerOptions::SetDefaultStrategy(0);
  //ROOT::Math::MinimizerOptions::SetDefaultPrintLevel(10);
  //ROOT::Math::MinimizerOptions::PrintDefault("",std::cout);

  reset();
}
////////////////////////////////////////////////
////////////////////////////////////////////////
void dEdxFitter::setPressure(double aPressure) {
  
  currentPressure = aPressure;

  minVtxOffset = -5;

  minAlphaOffset = 0;
  maxAlphaOffset = (385.99)*(190.0/currentPressure); 

  minCarbonOffset = 0;
  maxCarbonOffset = (28.64)*(190.0/currentPressure);
}
////////////////////////////////////////////////
////////////////////////////////////////////////
void dEdxFitter::reset(){

  carbon_alphaModel->SetRange(-20, 350);
  carbon_alphaModel->SetParLimits(0, 0.0, maxVtxOffset);
  carbon_alphaModel->SetParLimits(1, minAlphaOffset, maxAlphaOffset);
  carbon_alphaModel->SetParLimits(2, minCarbonOffset, maxCarbonOffset);

  alphaModel->SetRange(-20, 350);
  alphaModel->SetParLimits(0, minVtxOffset, maxVtxOffset);
  alphaModel->SetParLimits(1, minAlphaOffset, maxAlphaOffset);
  
  carbon_alphaModel->SetParameters(maxVtxOffset/2.0,
				   (minAlphaOffset+maxAlphaOffset)/2.0,
				   (minCarbonOffset+maxCarbonOffset)/2.0,
				   1.0, 1.0,				   
				   9E-6, 0.0, 1.8);
  
  alphaModel->SetParameters((minVtxOffset+maxVtxOffset)/2.0,
			    (minAlphaOffset+maxAlphaOffset)/2.0,
			    0.0,
			    1.0, 0.0,
			    9E-6, 0.0, 1.8);

  theFitResult = TFitResult();
  theFittedModel = alphaModel;
  theFittedHisto = emptyHisto;
  bestFitEventType = pid_type::UNKNOWN;
  bestFitChi2 = 999.0;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::bragg_alpha(double *x, double *params) {
  
  return 1E7*braggGraph_alpha->Eval((currentPressure/nominalPressure)*x[0]*1E7);
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::bragg_12C(double *x, double *params) {
  return 1E7*braggGraph_12C->Eval((currentPressure/nominalPressure)*x[0]*1E7);
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::bragg_12C_alpha(double *x, double *params) {

  double value = 0.0;
  double vertex_pos = params[0];
  double alpha_shift = params[1];
  double carbon_shift = params[2];
  double alpha_scale = params[3];
  double carbon_scale = params[4];
  double xShifted[1];
  
  xShifted[0] = (x[0] - vertex_pos) + alpha_shift;
  value = alpha_scale*bragg_alpha(xShifted, params)*(x[0]>vertex_pos);

  xShifted[0] = (vertex_pos-x[0]) + carbon_shift;
  value += carbon_scale*bragg_12C(xShifted, params)*(xShifted[0]>0)*(x[0]>0)*(x[0]<vertex_pos);
                 
  return value;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
TH1F dEdxFitter::reflectHisto(const TH1F &aHisto) const{

  TH1F hReflected(aHisto);
  hReflected.Reset();
 
  double value=0.0;
  for(int iBin=0;iBin<=aHisto.GetNbinsX()+1;++iBin){
    value = aHisto.GetBinContent(iBin);
    hReflected.SetBinContent(aHisto.GetNbinsX()-iBin, value);
    value = aHisto.GetBinError(iBin);
    hReflected.SetBinError(aHisto.GetNbinsX()-iBin, value);
    
  }
  return hReflected;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
TFitResult dEdxFitter::fitHypothesis(TF1 *fModel, TH1F & aHisto){

  double tkLength = aHisto.GetXaxis()->GetXmax();
  maxVtxOffset = tkLength/2.0;
  minAlphaOffset = std::max(0.0, maxAlphaOffset - tkLength);
  minCarbonOffset = std::max(0.0, maxCarbonOffset - tkLength);
  
  reset();

  TFitResult theResult;
  if(!aHisto.GetEntries()) return theResult;

  std::cout<<"---------------------------------"<<std::endl;
  TFitResultPtr theResultPtr = aHisto.Fit(fModel,"BRWSM");
  if(theResultPtr.Get()) theResult = *theResultPtr.Get();
  else{
    std::cout<<KRED<<"No fit result"<<RST<<std::endl;
  }
  return theResult;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
TFitResult dEdxFitter::fitHisto(TH1F & aHisto){

  int maxBin = aHisto.GetMaximumBin();
  if(maxBin>aHisto.GetNbinsX()/2.0){
    aHisto = reflectHisto(aHisto);
    isReflected = true;
  }
  else{
    isReflected = false;
  }

  //TH1F aReflectedHisto = reflectHisto(aHisto);
 
  TFitResult carbon_alphaResult = fitHypothesis(carbon_alphaModel, aHisto);

  int iteration = 0;
  int maxIterations = 15;
  while( (!carbon_alphaResult.IsValid() || carbon_alphaResult.MinFcnValue()>1.0)
	 && iteration<maxIterations){
    carbon_alphaResult = fitHypothesis(carbon_alphaModel, aHisto);
    ++iteration;
  }

  
  //TFitResult carbon_alphaResult_reflected = fitHypothesis(carbon_alphaModel, aReflectedHisto);
  //TFitResult alphaResult = fitHypothesis(alphaModel, aHisto);
  //TFitResult alphaResult_reflected = fitHypothesis(alphaModel, aReflectedHisto);

  bestFitChi2 = carbon_alphaResult.MinFcnValue();
  theFitResult = carbon_alphaResult;
  theFittedModel = carbon_alphaModel;
  theFittedHisto = aHisto;
  bestFitEventType = pid_type::C12_ALPHA;
  if(getCarbonRange()<1){
    bestFitEventType = pid_type::ALPHA;
  }

  theFittedModel->SetParameters(theFitResult.Parameters().data());
  return theFitResult;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::getVertexOffset() const{

  return theFittedModel->GetParameter("vertexOffset");
  
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::getAlphaRange() const{

  if(bestFitEventType==pid_type::UNKNOWN) return 0.0;
    
  double alphaOffset = theFittedModel->GetParameter("alphaOffset");
  double alphaRange = maxAlphaOffset - alphaOffset;
  return alphaRange;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::getCarbonRange() const{

  if(bestFitEventType!=pid_type::C12_ALPHA) return 0.0;

  double carbonOffset = theFittedModel->GetParameter("carbonOffset");
  double carbonRange = maxCarbonOffset - carbonOffset;
  return carbonRange;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
