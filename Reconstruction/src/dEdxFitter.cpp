#include "dEdxFitter.h"

#include "TFitResultPtr.h"

TGraph* dEdxFitter::braggGraph_alpha = new TGraph("dEdx_alpha_10000keV_250mbar_CO2_no_header.dat", "%lg %lg %*lg");
TGraph* dEdxFitter::braggGraph_12C = new TGraph("dEdx_12C_2500keV_250mbar_CO2_no_header.dat", "%lg %lg %*lg");
////////////////////////////////////////////////
////////////////////////////////////////////////
dEdxFitter::dEdxFitter(){
  
  alpha_ionisation = new TF1("alpha_ionisation", bragg_alpha, 0, 350.0, 0);
  carbon_ionisation = new TF1("carbon_ionisation",bragg_12C, 0, 120.0, 0);

  carbon_alpha_ionisation = new TF1("carbon_alpha_ionisation",bragg_12C_alpha, -10, 350.0, 4);
  carbon_alpha_ionisation->SetParName(0,"vertex");
  carbon_alpha_ionisation->SetParName(1,"alpha_shift");
  carbon_alpha_ionisation->SetParName(2,"alpha_scale");
  carbon_alpha_ionisation->SetParName(3,"carbon_scale");
  carbon_alpha_ionisation->SetParameters(10.0, 5, 1.0, 1.0);

  carbon_alpha_ionisation_smeared = new TF1Convolution("carbon_alpha_ionisation", "gaus",-20, 350, true);
  carbon_alpha_ionisation_smeared->SetRange(-20, 350);
  carbon_alpha_ionisation_smeared->SetNofPointsFFT(2000);
 
  carbon_alphaModel = new TF1("carbon_alphaModel", *carbon_alpha_ionisation_smeared,
		       -20, 350., carbon_alpha_ionisation_smeared->GetNpar());
  carbon_alphaModel->SetLineColor(1);
  carbon_alphaModel->SetLineStyle(2);
  carbon_alphaModel->SetLineWidth(3);
  
  carbon_alphaModel->SetParName(0, "carbonLength");
  carbon_alphaModel->SetParName(1, "alphaOffset");
  carbon_alphaModel->SetParName(2, "alphaScale");
  carbon_alphaModel->SetParName(3, "carbonScale");
  carbon_alphaModel->SetParName(4, "gaussNorm");
  carbon_alphaModel->SetParName(5, "gaussMean");
  carbon_alphaModel->SetParName(6, "gaussSigma");
  
  carbon_alphaModel->SetParLimits(0, 3, 13.0);
  carbon_alphaModel->SetParLimits(1, 3, 295.0);
  carbon_alphaModel->FixParameter(2, 1.0);
  carbon_alphaModel->SetParLimits(3, 0.45, 0.56);
  carbon_alphaModel->SetParLimits(4, 0, 1E-1);
  carbon_alphaModel->FixParameter(5, 0.0);
  carbon_alphaModel->SetParLimits(6, 1, 10);
  carbon_alphaModel->SetParameters(8.68, 244.324, 1.0, 0.5,  5.63216e-06, 0, 1.68);

  alphaModel = new TF1(*carbon_alphaModel);
  alphaModel->SetName("alphaModel");
  alphaModel->FixParameter(0, 0.0);
  alphaModel->FixParameter(3, 0.0);
  alphaModel->SetParameters(0, 210, 1.0, 0.0, 1E-2, 0, 1.7);
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::bragg_alpha(double *x, double *params) {
  return 1E7*braggGraph_alpha->Eval((x[0])*1E7);
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::bragg_12C(double *x, double *params) {
  return 1E7*braggGraph_12C->Eval((x[0])*1E7);
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::bragg_12C_alpha(double *x, double *params) {

  double value = 0.0;
  double vertex_pos = params[0];
  double alpha_shift = params[1];
  double alpha_scale = params[2];
  double carbon_scale = params[3];
  double xShifted[1];
  
  xShifted[0] = x[0] + alpha_shift-vertex_pos;
  value = alpha_scale*bragg_alpha(xShifted, params)*(x[0]>vertex_pos);

  xShifted[0] = -x[0]+vertex_pos;
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

  TFitResultPtr theResultPtr = aHisto.Fit(fModel,"BRWMS");
  TFitResult theResult;
  if(theResultPtr>=0) theResult = *theResultPtr.Get();
  return theResult;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
void dEdxFitter::reset(){

  carbon_alphaModel->SetParameters(8.68, 244.324, 1.0, 0.5,  5.63216e-06, 0, 1.68);
  alphaModel->SetParameters(0, 210, 1.0, 0.0, 1E-2, 0, 1.7);

  theFitResult = TFitResult();
  theFittedModel = alphaModel;
  theFittedHisto = emptyHisto;
  bestFitEventType = UNKNOWN;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
TFitResult dEdxFitter::fitHisto(TH1F & aHisto){

  reset();

  TH1F aReflectedHisto = reflectHisto(aHisto);
  
  TFitResult alphaResult = fitHypothesis(alphaModel, aHisto);
  if(alphaResult.IsEmpty()) return theFitResult;

  
  TFitResult alphaResult_reflected = fitHypothesis(alphaModel, aReflectedHisto);
									 
  TFitResult carbon_alphaResult = fitHypothesis(carbon_alphaModel, aHisto);
  TFitResult carbon_alphaResult_reflected = fitHypothesis(carbon_alphaModel, aReflectedHisto);

  theFitResult = alphaResult;
  theFittedModel = alphaModel;
  theFittedHisto = aHisto;
  bestFitEventType = ALPHA;
  
  if(alphaResult_reflected.MinFcnValue()<theFitResult.MinFcnValue()){
    theFitResult = alphaResult_reflected;
    theFittedModel = alphaModel;
    theFittedHisto = aReflectedHisto;
    bestFitEventType = ALPHA;
  }
  if(false && carbon_alphaResult.MinFcnValue()<theFitResult.MinFcnValue()){
    theFitResult = carbon_alphaResult;
    theFittedModel = carbon_alphaModel;
    theFittedHisto = aHisto;
    bestFitEventType = C12_ALPHA;
  }
  if(false && carbon_alphaResult_reflected.MinFcnValue()<theFitResult.MinFcnValue()){
    theFitResult = carbon_alphaResult_reflected;
    theFittedHisto = aReflectedHisto;
    theFittedModel = carbon_alphaModel;
    bestFitEventType = C12_ALPHA;
  }

  theFittedModel->SetParameters(theFitResult.Parameters().data());
  return theFitResult;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::getAlphaEnergy() const{

  if(bestFitEventType!=ALPHA) return -1;

  double alphaOffset =  theFittedModel->GetParameter("alphaOffset");
  double alpha_energy = 9.98908e+06 - alpha_ionisation->Integral(0, alphaOffset, 1.0);  
  return alpha_energy;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::getCarbonEnergy() const{

  if(bestFitEventType!=C12_ALPHA) return -1;
  
  double carbonRange = theFittedModel->GetParameter("carbonLength");
  double carbonOffset = 14.4 - carbonRange;  
  double carbon_energy =  2.45828e+06 - carbon_ionisation->Integral(0, carbonOffset, 1.0);
  carbon_energy =  carbon_ionisation->Integral(0, 100, 1.0);
  return carbon_energy;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
