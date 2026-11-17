#include "TPCReco/dEdxFitter.h"
#include "TPCReco/colorText.h"
#include <algorithm>
#include <cmath>

#include <TFitResultPtr.h>
#include <Math/MinimizerOptions.h>
#include <TMath.h>
#include <TRandom3.h>

dEdxFitter::BraggTemplate dEdxFitter::theAlphaTemplate;
dEdxFitter::BraggTemplate dEdxFitter::theCarbonTemplate;
TGraph* dEdxFitter::braggGraph_alpha = nullptr;
TGraph* dEdxFitter::braggGraph_12C = nullptr;

double dEdxFitter::nominalPressure = 250.0;
double dEdxFitter::currentPressure = 190.0;
////////////////////////////////////////////////
////////////////////////////////////////////////
dEdxFitter::dEdxFitter(double aPressure): dEdxFitter(TPCRECO_RESOURCE_DIR, aPressure){}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
dEdxFitter::dEdxFitter(std::string resources, double aPressure){
  //initialize bragg graphs if they are null:
  if(!braggGraph_alpha){
    braggGraph_alpha = new TGraph((resources+"dEdx_corr_alpha_10MeV_CO2_250mbar.dat").c_str(), "%lg %lg");
  }
  if(!braggGraph_12C){
    braggGraph_12C = new TGraph((resources+"dEdx_corr_12C_5MeV_CO2_250mbar.dat").c_str(), "%lg %lg");
  }
  setPressure(aPressure);

  braggGraph_alpha->SetBit(TGraph::kIsSortedX);
  braggGraph_12C->SetBit(TGraph::kIsSortedX);
  if(theAlphaTemplate.s.empty()) theAlphaTemplate.set(*braggGraph_alpha);
  if(theCarbonTemplate.s.empty()) theCarbonTemplate.set(*braggGraph_12C);

  TF1::DefaultAddToGlobalList(kFALSE);
  alpha_ionisation = new TF1("alpha_ionisation", bragg_alpha, 0, 600.0);
  carbon_ionisation = new TF1("carbon_ionisation",bragg_12C, 0,  200.0);

  carbon_alpha_model = new TF1("carbon_alpha_model",bragg_12C_alpha, -20, 350.0, 7);

  carbon_alpha_model->SetParName(0, "sigma");
  carbon_alpha_model->SetParName(1, "vertexOffset");
  carbon_alpha_model->SetParName(2, "alphaOffset");
  carbon_alpha_model->SetParName(3, "carbonOffset");
  carbon_alpha_model->SetParName(4, "alphaScale");
  carbon_alpha_model->SetParName(5, "carbonScale");
  carbon_alpha_model->SetParName(6, "commonScale");

  carbon_alpha_model->SetParLimits(0, 0.5, 1.5);
  carbon_alpha_model->SetParLimits(6, 1E-4, 2E-2);
  
  carbon_alpha_model->FixParameter(4, 1);
  carbon_alpha_model->FixParameter(5, 1);  
 
  alpha_model = new TF1(*carbon_alpha_model);
  alpha_model->SetName("alpha_model");
  alpha_model->FixParameter(1, 0.0);
  alpha_model->FixParameter(3, 0.0);
  alpha_model->FixParameter(4, 0.0);
  alpha_model->FixParameter(5, 0.0);
 
  reset();
}
////////////////////////////////////////////////
////////////////////////////////////////////////
void dEdxFitter::setPressure(double aPressure) {
  
  currentPressure = aPressure;

  minVtxOffset = -5;
  minAlphaOffset = 0; 
  minCarbonOffset = 5;

  //190 mbar, 10 MeV alpha, 2.5 MeV C
  if(std::abs(nominalPressure-190)<1E-3){
    maxAlphaOffset = (385.99)*(nominalPressure/currentPressure); 
    maxCarbonOffset = (28.64)*(nominalPressure/currentPressure);
  }
  //250 mbar, 10 MeV alpha, 5 MeV C
  else if(std::abs(nominalPressure-250)<1E-3){
    maxAlphaOffset = (297.23)*(nominalPressure/currentPressure);
    maxCarbonOffset = (23.43)*(nominalPressure/currentPressure);
  }
  else{
    std::cout<<KRED<<"dEdxFitter: nominal pressure: "<<RST<<nominalPressure
	     <<" does not correspond to any dEdx data files."<<std::endl;
    exit(0);
  }
}
////////////////////////////////////////////////
////////////////////////////////////////////////
void dEdxFitter::setDiffusionRange(double minD, double maxD){

  carbon_alpha_model->SetParLimits(0, minD, maxD);
  alpha_model->SetParLimits(0, minD, maxD);
}
////////////////////////////////////////////////
////////////////////////////////////////////////
void dEdxFitter::reset(){

  carbon_alpha_model->SetRange(-20, maxAlphaOffset+maxCarbonOffset);
  carbon_alpha_model->SetParLimits(1, 0.0, maxVtxOffset);

  double minCarbonRange = 5.0; //mm
  double minD = 0.5, maxD = 1.5;
  carbon_alpha_model->GetParLimits(0, minD, maxD);

  carbon_alpha_model->SetParLimits(2, minAlphaOffset, maxAlphaOffset-minCarbonRange);
  carbon_alpha_model->SetParLimits(3, minCarbonOffset, maxCarbonOffset-minCarbonRange);
  carbon_alpha_model->SetParameters((maxD + minD)/2.0,
				    (minVtxOffset+maxVtxOffset)/2.0,
            (minAlphaOffset+maxAlphaOffset)/2.0,
            (minCarbonOffset+maxCarbonOffset)/2.0,
				    //maxAlphaOffset-85,  // [mm] --> HACK valid for 13.1 MeV
				    //maxCarbonOffset-11,  // [mm] --> HACK valid for 13.1 MeV
				    1, 1, 5E-3);

  alpha_model->SetRange(-20, maxAlphaOffset);
  alpha_model->SetParLimits(1, minVtxOffset, maxVtxOffset);
  alpha_model->SetParLimits(2, minAlphaOffset, maxAlphaOffset);
  alpha_model->SetParameters((maxD + minD)/2.0,
			     (minVtxOffset+maxVtxOffset)/2.0,
			     (minAlphaOffset+maxAlphaOffset)/2.0,
			     0.0,
			     1.0, 0.0, 5E-3);
  
  theFitResult = TFitResult();
  theFittedModel = alpha_model;
  theFittedHisto = emptyHisto;
  bestFitEventType = pid_type::UNKNOWN;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
void dEdxFitter::setVertexConstraint(double vxtOffset){

carbon_alpha_model->SetParLimits(1, vxtOffset-2, vxtOffset+2);
carbon_alpha_model->SetParameter(1, vxtOffset);

}
////////////////////////////////////////////////
////////////////////////////////////////////////
void dEdxFitter::BraggTemplate::set(const TGraph &aGraph){

  s.assign(aGraph.GetX(), aGraph.GetX()+aGraph.GetN());
  a.assign(aGraph.GetY(), aGraph.GetY()+aGraph.GetN());
  k.assign(s.size(), 0.0);
  for(size_t i=0;i+1<s.size();++i){
    if(s[i+1]>s[i]) k[i] = (a[i+1]-a[i])/(s[i+1]-s[i]);
  }
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::BraggTemplate::smeared(double pos, double cut, double sigma) const{

  // Gaussian tails beyond nSigma are neglected (relative contribution < 1E-8)
  const double nSigma = 6.0;
  if(s.size()<2) return 0.0;
  sigma = std::max(sigma, 1E-6);
  const double lo = std::max({cut, pos-nSigma*sigma, s.front()});
  const double hi = std::min(pos+nSigma*sigma, s.back());
  if(lo>=hi) return 0.0;

  const double invSqrt2 = 1.0/std::sqrt(2.0);
  const double invSqrt2Pi = 1.0/std::sqrt(2.0*M_PI);
  // segment containing lo
  size_t i = std::upper_bound(s.begin(), s.end(), lo) - s.begin();
  i = (i>0 ? i-1 : 0);
  double sum = 0.0;
  // integral over [t1, t2] of (a_i + k_i*(t-s_i)) * Gauss(t-pos, sigma) dt, with u = (t-pos)/sigma:
  // (a_i + k_i*(pos-s_i))*(Phi(u2)-Phi(u1)) + k_i*sigma*(phi(u1)-phi(u2))
  for(;i+1<s.size() && s[i]<hi;++i){
    const double t1 = std::max(s[i], lo);
    const double t2 = std::min(s[i+1], hi);
    if(t2<=t1) continue;
    const double u1 = (t1-pos)/sigma, u2 = (t2-pos)/sigma;
    const double dPhi = 0.5*(std::erf(u2*invSqrt2) - std::erf(u1*invSqrt2));
    const double dphi = invSqrt2Pi*(std::exp(-0.5*u1*u1) - std::exp(-0.5*u2*u2));
    sum += (a[i] + k[i]*(pos-s[i]))*dPhi + k[i]*sigma*dphi;
  }
  return sum;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
// The tables are given in [mm] at the nominal pressure. A length L at the
// current pressure corresponds to L*currentPressure/nominalPressure in the table;
// the smearing width is converted in the same way.
double dEdxFitter::bragg_alpha(double *x, double *params) {
  const double scale = currentPressure/nominalPressure;
  const double sigma = scale*params[0];
  const double cut = scale*params[2];               // start of the track in the table
  const double pos = scale*(x[0]-params[1]) + cut;  // alpha towards larger x
  return theAlphaTemplate.smeared(pos, cut, sigma);
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::bragg_12C(double *x, double *params) {
  const double scale = currentPressure/nominalPressure;
  const double sigma = scale*params[0];
  const double cut = scale*params[3];               // start of the track in the table
  const double pos = scale*(params[1]-x[0]) + cut;  // carbon towards smaller x
  return theCarbonTemplate.smeared(pos, cut, sigma);
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::bragg_12C_alpha(double *x, double *params) {

  double value = 0.0;
  double alpha_scale = params[4];
  double carbon_scale = params[5];
  double common_scale = params[6];
  
  value = alpha_scale*bragg_alpha(x, params);
  value += carbon_scale*bragg_12C(x, params);
                 
  return common_scale*value;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
TH1F dEdxFitter::reflectHisto(const TH1F &aHisto) const{

  TH1F hReflected(aHisto);
  hReflected.Reset();
 
  double value=0.0;
  for(int iBin=0;iBin<=aHisto.GetNbinsX()+1;++iBin){
    value = aHisto.GetBinContent(iBin);
    hReflected.SetBinContent(aHisto.GetNbinsX()+1-iBin, value);
    value = aHisto.GetBinError(iBin);
    hReflected.SetBinError(aHisto.GetNbinsX()+1-iBin, value);
    
  }
  return hReflected;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
TFitResult dEdxFitter::fitHypothesis(TF1 *fModel, TH1F & aHisto){
  
  TFitResult theResult;
  if(!aHisto.GetEntries()) return theResult;

  double tkLength = aHisto.GetXaxis()->GetXmax()/1.2;
  maxVtxOffset = 2*maxCarbonOffset;
  minAlphaOffset = std::max(0.0, maxAlphaOffset - tkLength);
  minCarbonOffset = std::max(0.0, maxCarbonOffset - tkLength);
  int fitCounter = 0;
  auto chargeFromHisto = aHisto.Integral("width");
  double ratio = 1.0;
  reset();

  TFitResultPtr theResultPtr;
  do{
    reset();
    theResultPtr = aHisto.Fit(fModel,"BRWWSQ");
    if(!theResultPtr.Get()) break;
    ratio = theResultPtr->MinFcnValue()/std::pow(chargeFromHisto,2);
    ++fitCounter;
  }while((!theResultPtr->IsValid() || ratio>5E-4) && fitCounter<10);
  
  if(theResultPtr.Get()) theResult = *theResultPtr.Get();
  else{
    std::cout<<KRED<<"No fit result"<<RST<<std::endl;
  }
  return theResult;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
TFitResult dEdxFitter::fitHisto(const TH1F & aHisto){

  /// C12+alpha hypothesis
  TH1F fittedHisto_for_C12_alpha = aHisto;
  bool reflection_for_C12_alpha = false;
  int maxBin = aHisto.GetMaximumBin();
  if(maxBin>aHisto.GetNbinsX()/2.0){
    fittedHisto_for_C12_alpha = reflectHisto(aHisto);
    reflection_for_C12_alpha = true;
  }
  TFitResult carbon_alphaResult = fitHypothesis(carbon_alpha_model, fittedHisto_for_C12_alpha);

  /// alpha hypothesis
  bool reflection_for_alpha = false;
  TH1F fittedHisto_for_alpha = aHisto;
  maxBin = aHisto.GetMaximumBin();
  if(maxBin<aHisto.GetNbinsX()/2.0){
    fittedHisto_for_alpha = reflectHisto(aHisto);
    reflection_for_alpha = true;
  }
  TFitResult alphaResult = fitHypothesis(alpha_model, fittedHisto_for_alpha);

  /// select best hypothesis
  if(alphaResult.MinFcnValue()<carbon_alphaResult.MinFcnValue()){
    theFitResult = alphaResult;
    theFittedModel = alpha_model;
    theFittedHisto = fittedHisto_for_alpha;
    isReflected = reflection_for_alpha;
    bestFitEventType = pid_type::ALPHA;
  }
  else{
    theFitResult = carbon_alphaResult;
    theFittedModel = carbon_alpha_model;
    theFittedHisto = fittedHisto_for_C12_alpha;
    isReflected = reflection_for_C12_alpha;
    bestFitEventType = pid_type::C12_ALPHA;
  }  
  if(theFitResult.IsValid()) theFittedModel->SetParameters(theFitResult.Parameters().data());

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
double dEdxFitter::getDiffusion() const{

  if(bestFitEventType==pid_type::UNKNOWN) return 0.0;
  return theFittedModel->GetParameter("sigma"); 
}
////////////////////////////////////////////////
////////////////////////////////////////////////
