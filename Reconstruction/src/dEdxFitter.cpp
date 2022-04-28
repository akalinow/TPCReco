#include "dEdxFitter.h"
#include "colorText.h"

#include "TFitResultPtr.h"
#include "Math/MinimizerOptions.h"

TGraph* dEdxFitter::braggGraph_alpha = new TGraph("dEdx_alpha_10000keV_250mbar_CO2.dat", "%lg %lg %*lg");
TGraph* dEdxFitter::braggGraph_12C = new TGraph("dEdx_12C_2500keV_250mbar_CO2.dat", "%lg %lg %*lg");
double dEdxFitter::pressure = 250.0;
////////////////////////////////////////////////
////////////////////////////////////////////////
dEdxFitter::dEdxFitter(double aPressure){

  pressure = aPressure;
  
  alpha_ionisation = new TF1("alpha_ionisation", bragg_alpha, 0, 350.0);
  carbon_ionisation = new TF1("carbon_ionisation",bragg_12C, 0, 120.0);

  carbon_alpha_ionisation = new TF1("carbon_alpha_ionisation",bragg_12C_alpha, -10, 350.0, 4);
  carbon_alpha_ionisation->SetParName(1,"vertex");
  carbon_alpha_ionisation->SetParName(2,"alpha_shift");
  carbon_alpha_ionisation->SetParName(3,"alpha_scale");
  carbon_alpha_ionisation->SetParName(4,"carbon_scale");
  carbon_alpha_ionisation->SetParameters(pressure, 10.0, 5, 1.0, 1.0);

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

  carbon_alphaModel->SetParLimits(0, 10, 20.0);
  carbon_alphaModel->SetParLimits(1, 240, 290.0);
  carbon_alphaModel->FixParameter(2, 1.0);
  carbon_alphaModel->FixParameter(3, 0.46);
  //carbon_alphaModel->SetParLimits(4, 2E-6, 3E-6);//2.2366e-06
  carbon_alphaModel->FixParameter(4, 2.2E-6);//2.2366e-06
  carbon_alphaModel->FixParameter(5, 0.0);
  carbon_alphaModel->FixParameter(6, 1.5);
  carbon_alphaModel->SetParLimits(6, 1, 3);
  carbon_alphaModel->SetParameters(5, 250, 1.0, 0.5,  2.2E-6, 0, 1.28);

  alphaModel = new TF1(*carbon_alphaModel);
  alphaModel->SetName("alphaModel");
  alphaModel->FixParameter(3, 0.0);
  alphaModel->SetParameters(0, 250, 1.0, 0.0, 2.2E-6, 0, 1.28);


  //ROOT::Math::MinimizerOptions::SetMinimizerType("Minuit2");
  ROOT::Math::MinimizerOptions::SetDefaultMinimizer("Minuit" ,"Scan");
  ROOT::Math::MinimizerOptions::SetDefaultStrategy(0);
  ROOT::Math::MinimizerOptions::PrintDefault("",std::cout);

  reset();
}
////////////////////////////////////////////////
////////////////////////////////////////////////
void dEdxFitter::reset(){

  carbon_alphaModel->SetParameters(10, 270, 1.0, 0.46,  2.2E-6, 0, 1.28);
  alphaModel->SetParameters(10, 250, 1.0, 0.0, 2.2E-6, 0, 1.28);
  theFitResult = TFitResult();
  theFittedModel = alphaModel;
  theFittedHisto = emptyHisto;
  bestFitEventType = pid_type::UNKNOWN;
  isReflected = false;
  bestFitChi2 = 999.0;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::bragg_alpha(double *x, double *params) {
  return 1E7*braggGraph_alpha->Eval((pressure/250.0)*x[0]*1E7);
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::bragg_12C(double *x, double *params) {
  return 1E7*braggGraph_12C->Eval((pressure/250.0)*x[0]*1E7);
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

  TH1F aReflectedHisto = reflectHisto(aHisto);
  
  ///TEST
  /*
  TFitResult aResult = fitHypothesis(carbon_alphaModel, aHisto);
  bestFitEventType = C12_ALPHA;
  theFittedHisto = aHisto;
  theFittedModel = carbon_alphaModel;
  theFittedModel->SetParameters(aResult.Parameters().data());
  return theFitResult;
  */
  ///////
  
  TFitResult alphaResult = fitHypothesis(alphaModel, aHisto);
  if(alphaResult.IsEmpty()) return theFitResult;


  TFitResult alphaResult_reflected = fitHypothesis(alphaModel, aReflectedHisto);									 
  TFitResult carbon_alphaResult = fitHypothesis(carbon_alphaModel, aHisto);
  TFitResult carbon_alphaResult_reflected = fitHypothesis(carbon_alphaModel, aReflectedHisto);

  bestFitChi2 = alphaResult.MinFcnValue();
  theFitResult = alphaResult;
  theFittedModel = alphaModel;
  theFittedHisto = aHisto;
  bestFitEventType = pid_type::ALPHA;
  isReflected = false;
  
  if(alphaResult_reflected.MinFcnValue()<theFitResult.MinFcnValue()){
    bestFitChi2 = alphaResult_reflected.MinFcnValue();
    theFitResult = alphaResult_reflected;
    theFittedModel = alphaModel;
    theFittedHisto = aReflectedHisto;
    bestFitEventType = pid_type::ALPHA;
    isReflected = true;
  }
  if(carbon_alphaResult.MinFcnValue()<theFitResult.MinFcnValue()){
    bestFitChi2 = carbon_alphaResult.MinFcnValue();
    theFitResult = carbon_alphaResult;
    theFittedModel = carbon_alphaModel;
    theFittedHisto = aHisto;
    bestFitEventType = pid_type::C12_ALPHA;
    isReflected = false;
  }
  if(carbon_alphaResult_reflected.MinFcnValue()<theFitResult.MinFcnValue()){
    bestFitChi2 = carbon_alphaResult_reflected.MinFcnValue();
    theFitResult = carbon_alphaResult_reflected;
    theFittedHisto = aReflectedHisto;
    theFittedModel = carbon_alphaModel;
    bestFitEventType = pid_type::C12_ALPHA;
    isReflected = true;
  }

  theFittedModel->SetParameters(theFitResult.Parameters().data());
  return theFitResult;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::getAlphaEnergy() const{

  double alphaOffset = theFittedModel->GetParameter("alphaOffset");
  double alphaEnergy = alpha_ionisation->Integral(alphaOffset, 350.0*(250.0/pressure), 0.1);
  return alphaEnergy;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::getCarbonEnergy() const{

  if(bestFitEventType!=C12_ALPHA) return -1;
  
  double carbonOffset = 14.4 - theFittedModel->GetParameter("carbonLength");
  double carbonEnergy =  carbon_ionisation->Integral(carbonOffset, 15.0, 0.1); 
  return carbonEnergy;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::getAlphaRange() const{

  if(bestFitEventType==pid_type::UNKNOWN) return 0.0;
    
  double alphaOffset = theFittedModel->GetParameter("alphaOffset");
  double alphaRange = 289.745*(250.0/190) - alphaOffset;
  return alphaRange;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::getCarbonRange() const{

  if(bestFitEventType!=pid_type::C12_ALPHA) return 0.0;
  
  double carbonRange = theFittedModel->GetParameter("carbonLength");
  return carbonRange;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
