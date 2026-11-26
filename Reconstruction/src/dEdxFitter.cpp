#include "TPCReco/dEdxFitter.h"
#include "TPCReco/colorText.h"
#include "TPCReco/IonRangeCalculator.h"
#include <algorithm>
#include <cmath>
#include <limits>

#include <TFitResultPtr.h>
#include <Math/MinimizerOptions.h>
#include <TMath.h>
#include <TRandom3.h>

dEdxFitter::BraggTemplate dEdxFitter::theAlphaTemplate;
dEdxFitter::BraggTemplate dEdxFitter::theCarbonTemplate;
TGraph* dEdxFitter::braggGraph_alpha = nullptr;
TGraph* dEdxFitter::braggGraph_12C = nullptr;

constexpr double dEdxFitter::nominalPressure;
constexpr double dEdxFitter::nominalTemperature;
constexpr double dEdxFitter::defaultErrorDef;
double dEdxFitter::currentPressure = 190.0;
double dEdxFitter::currentTemperature = dEdxFitter::nominalTemperature;
double dEdxFitter::theNominalAlphaRange = 0.0;
double dEdxFitter::theNominalCarbonRange = 0.0;
bool dEdxFitter::usePhysicsConstraint = false;
double dEdxFitter::carbonRangeMargin = 0.1;
dEdxFitter::Kinematics dEdxFitter::currentKinematics = dEdxFitter::Kinematics::FREE;
std::unique_ptr<IonRangeCalculator> dEdxFitter::theRangeCalculator;
double dEdxFitter::theMaxAlphaRange = 0.0;
double dEdxFitter::theMaxCarbonRange = 0.0;
////////////////////////////////////////////////
////////////////////////////////////////////////
dEdxFitter::dEdxFitter(double aPressure, double aTemperature): dEdxFitter(TPCRECO_RESOURCE_DIR, aPressure, aTemperature){}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
dEdxFitter::dEdxFitter(std::string resources, double aPressure, double aTemperature){
  //initialize bragg graphs if they are null:
  if(!braggGraph_alpha){
    braggGraph_alpha = new TGraph((resources+"dEdx_corr_alpha_10MeV_CO2_250mbar.dat").c_str(), "%lg %lg");
  }
  if(!braggGraph_12C){
    braggGraph_12C = new TGraph((resources+"dEdx_corr_12C_5MeV_CO2_250mbar.dat").c_str(), "%lg %lg");
  }
  braggGraph_alpha->SetBit(TGraph::kIsSortedX);
  braggGraph_12C->SetBit(TGraph::kIsSortedX);
  if(theAlphaTemplate.s.empty()) theAlphaTemplate.set(*braggGraph_alpha);
  if(theCarbonTemplate.s.empty()) theCarbonTemplate.set(*braggGraph_12C);
  if(theNominalAlphaRange<=0 || theNominalCarbonRange<=0){
    IonRangeCalculator aCalculator(resources, gas_mixture_type::CO2, nominalPressure, nominalTemperature);
    theNominalAlphaRange = tableRange(theAlphaTemplate, aCalculator, pid_type::ALPHA);
    theNominalCarbonRange = tableRange(theCarbonTemplate, aCalculator, pid_type::CARBON_12);
  }
  setGasConditions(aPressure, aTemperature);

  TF1::DefaultAddToGlobalList(kFALSE);
  alpha_ionisation = new TF1("alpha_ionisation", bragg_alpha, 0, 600.0);
  carbon_ionisation = new TF1("carbon_ionisation",bragg_12C, 0,  200.0);

  carbon_alpha_model = new TF1("carbon_alpha_model",bragg_12C_alpha, -20, 350.0, 8);

  carbon_alpha_model->SetParName(0, "sigma");
  carbon_alpha_model->SetParName(1, "vertexOffset");
  carbon_alpha_model->SetParName(2, "alphaOffset");
  carbon_alpha_model->SetParName(3, "carbonOffset");
  carbon_alpha_model->SetParName(4, "alphaScale");
  carbon_alpha_model->SetParName(5, "carbonScale");
  carbon_alpha_model->SetParName(6, "commonScale");
  carbon_alpha_model->SetParName(7, "carbonRangeScale");

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
  alpha_model->FixParameter(7, 1.0);
 
  reset();
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::tableRange(const BraggTemplate &aTemplate, IonRangeCalculator &aCalculator, pid_type ion){

  // The range of the energy deposited by the table from position s to its end, taken from
  // the range table, is R(E(s)) = R0 - s with a constant R0, the range of the energy of the
  // full table. The hard-coded R0 (297.23 mm for the alpha: the range of 10.000 MeV) did not
  // match the alpha table, which deposits 10.022 MeV (R0 = 298.24 mm): the alpha ranges were
  // 1.0 mm (at 250 mbar) too short. R0 is the mean of R(E(s)) + s for the positions with
  // 10-90% of the table energy remaining (within the range table).
  const auto &s = aTemplate.s;
  const auto &a = aTemplate.a;
  if(s.size()<2) return 0.0;
  std::vector<double> remaining(s.size(), 0.0); // [keV]
  for(size_t i=s.size()-1;i>0;--i) remaining[i-1] = remaining[i] + 0.5*(a[i]+a[i-1])*(s[i]-s[i-1]);
  const double total = remaining.front();
  double sum = 0.0;
  int n = 0;
  for(size_t i=0;i<s.size();++i){
    if(remaining[i]<0.1*total || remaining[i]>0.9*total) continue;
    sum += aCalculator.getIonRangeMM(ion, remaining[i]*1E-3) + s[i];
    ++n;
  }
  if(!n){
    std::cout<<KRED<<"dEdxFitter: no range for the dE/dx table of "<<RST<<int(ion)<<std::endl;
    exit(1);
  }
  return sum/n;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::densityScale(){
  return (currentPressure/currentTemperature)/(nominalPressure/nominalTemperature);
}
////////////////////////////////////////////////
////////////////////////////////////////////////
void dEdxFitter::setGasConditions(double aPressure, double aTemperature) {

  currentPressure = aPressure;
  currentTemperature = aTemperature;

  minVtxOffset = -5;
  minAlphaOffset = 0;
  minCarbonOffset = 5;

  // table ranges at the current gas density
  maxAlphaOffset = theNominalAlphaRange/densityScale();
  maxCarbonOffset = theNominalCarbonRange/densityScale();
  theMaxAlphaRange = maxAlphaOffset;
  theMaxCarbonRange = maxCarbonOffset;
  theRangeCalculator = std::make_unique<IonRangeCalculator>(gas_mixture_type::CO2, currentPressure, currentTemperature);
}
////////////////////////////////////////////////
////////////////////////////////////////////////
void dEdxFitter::setPhysicsConstraint(bool enable, double margin){
  usePhysicsConstraint = enable;
  carbonRangeMargin = std::max(0.0, margin);
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::recoilRangeFromAlpha(double alphaRange, Kinematics kinematics){

  if(kinematics==Kinematics::FREE) return 0.0;
  const pid_type recoil = (kinematics==Kinematics::O18) ? pid_type::CARBON_14 : pid_type::CARBON_12;
  // cache: evaluated for every point of the profile with the same parameters
  static double lastAlphaRange = -1, lastRecoilRange = 0, lastPressure = -1;
  static Kinematics lastKinematics = Kinematics::FREE;
  static double lastTemperature = -1;
  if(alphaRange==lastAlphaRange && kinematics==lastKinematics && currentPressure==lastPressure &&
     currentTemperature==lastTemperature) return lastRecoilRange;
  double recoilRange = 0.0;
  if(alphaRange>0 && theRangeCalculator){
    const double alphaEnergy = theRangeCalculator->getIonEnergyMeV(pid_type::ALPHA, alphaRange);
    // decay at rest: equal momenta, E = p^2/2m
    const double recoilEnergy = alphaEnergy*theRangeCalculator->getIonMassMeV(pid_type::ALPHA)/
                                theRangeCalculator->getIonMassMeV(recoil);
    if(recoilEnergy>0) recoilRange = theRangeCalculator->getIonRangeMM(recoil, recoilEnergy);
  }
  lastAlphaRange = alphaRange;
  lastKinematics = kinematics;
  lastPressure = currentPressure;
  lastTemperature = currentTemperature;
  lastRecoilRange = recoilRange;
  return recoilRange;
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

  carbon_alpha_model->SetRange(fitRangeMin, maxAlphaOffset+maxCarbonOffset);
  carbon_alpha_model->SetParLimits(1, 0.0, maxVtxOffset);

  double minCarbonRange = 1.0; //mm (a 0.4 MeV carbon at 130 mbar is ~5 mm long)
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
				    1, 1, 5E-3, 1.0);
  if(currentKinematics!=Kinematics::FREE){
    // recoil range from the alpha range, scaled within +- margin
    carbon_alpha_model->FixParameter(3, carbon_alpha_model->GetParameter(3));
    carbon_alpha_model->SetParLimits(7, 1.0-carbonRangeMargin, 1.0+carbonRangeMargin);
  }
  else carbon_alpha_model->FixParameter(7, 1.0);
  if(hasVertexConstraint){
    carbon_alpha_model->SetParLimits(1, std::max(0.0, vertexConstraint-vertexConstraintHalfWidth), vertexConstraint+vertexConstraintHalfWidth);
    carbon_alpha_model->SetParameter(1, vertexConstraint);
  }

  alpha_model->SetRange(fitRangeMin, maxAlphaOffset);
  alpha_model->SetParLimits(1, minVtxOffset, maxVtxOffset);
  alpha_model->SetParLimits(2, minAlphaOffset, maxAlphaOffset);
  alpha_model->SetParameters((maxD + minD)/2.0,
			     (minVtxOffset+maxVtxOffset)/2.0,
			     (minAlphaOffset+maxAlphaOffset)/2.0,
			     0.0,
			     1.0, 0.0, 5E-3, 1.0);
  
  // initial step sizes: TF1 keeps the parameter errors of the previous fit and
  // ROOT uses them as initial step sizes, which made the fit result depend on
  // the previous fit (and event). Zero means the ROOT default step.
  setStepSizes(carbon_alpha_model, 0.0);
  setStepSizes(alpha_model, 0.0);

  theFitResult = TFitResult();
  theFittedModel = alpha_model;
  theFittedHisto = emptyHisto;
  bestFitEventType = pid_type::UNKNOWN;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
void dEdxFitter::setStepSizes(TF1 *fModel, double scale){

  for(int iPar=0;iPar<fModel->GetNpar();++iPar){
    double low = 0.0, high = 0.0;
    fModel->GetParLimits(iPar, low, high);
    // scale x 10% of the allowed range for limited parameters, 0 (ROOT default) otherwise
    const double step = (scale>0 && high>low) ? scale*0.1*(high-low) : 0.0;
    fModel->SetParError(iPar, step);
  }
}
////////////////////////////////////////////////
////////////////////////////////////////////////
void dEdxFitter::setVertexConstraint(double vxtOffset, double halfWidth){
  // applied in reset(), which sets the initial parameters before each fit
  hasVertexConstraint = true;
  vertexConstraint = vxtOffset;
  vertexConstraintHalfWidth = halfWidth;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
void dEdxFitter::setMaxFitAttempts(unsigned int nAttempts){
  maxFitAttempts = std::max(1u, nAttempts);
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
// The tables are given in [mm] at the nominal conditions. A length L at the
// current conditions corresponds to L*densityScale() in the table (density ~ p/T);
// the smearing width is converted in the same way.
double dEdxFitter::bragg_alpha(double *x, double *params) {
  const double scale = densityScale();
  const double sigma = scale*params[0];
  const double cut = scale*params[2];               // start of the track in the table
  const double pos = scale*(x[0]-params[1]) + cut;  // alpha towards larger x
  return theAlphaTemplate.smeared(pos, cut, sigma);
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::bragg_12C(double *x, double *params) {
  const double scale = densityScale();
  const double sigma = scale*params[0];
  double carbonOffset = params[3];
  if(currentKinematics!=Kinematics::FREE){
    // recoil range from the alpha range (momentum conservation) x scale
    const double carbonRange = recoilRangeFromAlpha(theMaxAlphaRange - params[2], currentKinematics)*params[7];
    carbonOffset = std::min(std::max(theMaxCarbonRange - carbonRange, 0.0), theMaxCarbonRange);
  }
  const double cut = scale*carbonOffset;            // start of the track in the table
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
std::vector<dEdxFitter::FitStart> dEdxFitter::fitStarts(const TF1 *fModel, double tkLength) const{

  // The default start of reset() (C12+alpha: vertex at maxCarbonOffset, half of the track for the
  // alpha) can end in the alpha-only local minimum: the carbon range shrinks to its lower limit and
  // the vertex goes to the carbon end of the track. Starts consistent with the track length are
  // added: the carbon from the track start to the vertex, the alpha from the vertex to the track end.
  const double nan = std::numeric_limits<double>::quiet_NaN();
  std::vector<FitStart> starts = {{nan, nan, nan}};
  if(fModel==carbon_alpha_model){
    if(hasVertexConstraint){
      const double v = std::max(0.0, vertexConstraint);
      starts.push_back({v, tkLength-v, v});
    }
    else{
      for(double fraction: {0.10, 0.17, 0.25}){
        const double v = fraction*tkLength;
        starts.push_back({v, tkLength-v, v});
      }
    }
  }
  else starts.push_back({0.0, tkLength, nan}); // alpha only: vertex at the track start
  return starts;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
void dEdxFitter::applyStart(TF1 *fModel, const FitStart &aStart) const{

  // value within the limits of a free parameter (TF1::FixParameter(i, 0) sets the limits to (1, 0))
  auto set = [fModel](int iPar, double value){
    if(std::isnan(value)) return;
    double low = 0.0, high = 0.0;
    fModel->GetParLimits(iPar, low, high);
    if(low>=high) return;
    fModel->SetParameter(iPar, std::min(std::max(value, low), high));
  };
  set(1, aStart.vertex);
  set(2, maxAlphaOffset - aStart.alphaRange);
  if(currentKinematics==Kinematics::FREE) set(3, maxCarbonOffset - aStart.carbonRange);
}
////////////////////////////////////////////////
////////////////////////////////////////////////
TFitResult dEdxFitter::fitHypothesis(TF1 *fModel, TH1F & aHisto){
  
  TFitResult theResult;
  if(!aHisto.GetEntries()) return theResult;

  double tkLength = aHisto.GetXaxis()->GetXmax()/1.2;
  // fit the whole profile (the fit range started at -20 mm, inside the profile of tracks longer than 100 mm)
  fitRangeMin = std::min(-20.0, aHisto.GetXaxis()->GetXmin());
  maxVtxOffset = 2*maxCarbonOffset;
  minAlphaOffset = std::max(0.0, maxAlphaOffset - tkLength);
  minCarbonOffset = std::max(0.0, maxCarbonOffset - tkLength);
  auto chargeFromHisto = aHisto.Integral("width");

  // Minuit ErrorDef matching the scale of the loss (see setErrorDef); the global default is restored
  const double defaultErrorDefROOT = ROOT::Math::MinimizerOptions::DefaultErrorDef();
  ROOT::Math::MinimizerOptions::SetDefaultErrorDef(errorDef);

  // Several starting points (fitStarts), each with deterministic restarts (maxFitAttempts,
  // default 1): the same starting point with the initial step sizes below (0: ROOT default,
  // otherwise scale x 10% of the parameter range). The restarts of a starting point stop at the
  // first valid fit with loss/Q^2 <= maxLossRatio. The best fit is kept: a valid one with the
  // lowest loss, else the one with the lowest loss.
  const std::vector<double> stepScales = {0.0, 1.0, 0.3, 3.0, 0.1, 0.5, 2.0, 0.05, 0.2, 5.0};
  const unsigned int nAttempts = std::min<size_t>(maxFitAttempts, stepScales.size());
  bool haveResult = false;
  for(const auto &aStart: fitStarts(fModel, tkLength)){
    for(unsigned int iAttempt=0;iAttempt<nAttempts;++iAttempt){
      const double scale = stepScales[iAttempt];
      reset();
      applyStart(fModel, aStart);
      setStepSizes(fModel, scale);
      TFitResultPtr aResultPtr = aHisto.Fit(fModel,"BRWWSQN"); // N: no graphics, would draw into gPad
      if(!aResultPtr.Get()) continue;
      const TFitResult & aResult = *aResultPtr.Get();
      const bool better = !haveResult ||
        (aResult.IsValid() && !theResult.IsValid()) ||
        (aResult.IsValid()==theResult.IsValid() && aResult.MinFcnValue()<theResult.MinFcnValue());
      if(better){
        theResult = aResult;
        haveResult = true;
      }
      const double ratio = aResult.MinFcnValue()/std::pow(chargeFromHisto,2);
      if(aResult.IsValid() && ratio<=maxLossRatio) break;
    }
  }
  ROOT::Math::MinimizerOptions::SetDefaultErrorDef(defaultErrorDefROOT);

  if(haveResult) fModel->SetParameters(theResult.Parameters().data());
  else std::cout<<KRED<<"No fit result"<<RST<<std::endl;
  return theResult;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
TFitResult dEdxFitter::fitHisto(const TH1F & aHisto){

  // Each hypothesis is fitted in both orientations of the profile (the track
  // direction is not known) and the better fit is kept. The orientation used
  // to be guessed from the position of the maximum, which for low energy
  // C12+alpha events is the alpha Bragg peak, not the carbon.
  struct HypothesisFit {
    TFitResult result;
    TH1F histo;
    bool reflected{false};
    bool done{false};
  };
  // a better than b: valid first, then lower loss
  auto better = [](const TFitResult &a, const TFitResult &b){
    if(a.IsValid()!=b.IsValid()) return a.IsValid();
    return a.MinFcnValue()<b.MinFcnValue();
  };
  auto fitBothOrientations = [&](TF1 *model){
    HypothesisFit best;
    for(int iOrientation=0;iOrientation<2;++iOrientation){
      if(forcedOrientation>=0 && iOrientation!=forcedOrientation) continue;
      TH1F histo = iOrientation ? reflectHisto(aHisto) : TH1F(aHisto);
      histo.SetDirectory(0);
      TFitResult result = fitHypothesis(model, histo);
      if(!best.done || better(result, best.result)){
        best.result = result;
        best.histo = histo;
        best.histo.SetDirectory(0);
        best.reflected = (iOrientation==1);
        best.done = true;
      }
    }
    return best;
  };

  // two prong hypothesis: free recoil range and, with the physics constraint,
  // the O16 -> C12 + alpha and O18 -> C14 + alpha kinematics; best fit kept
  std::vector<Kinematics> passes = {Kinematics::FREE};
  if(usePhysicsConstraint){
    passes.push_back(Kinematics::O16);
    passes.push_back(Kinematics::O18);
  }
  if(alphaOnly) passes.clear();
  HypothesisFit carbonAlpha;
  Kinematics carbonAlphaKinematics = Kinematics::FREE;
  for(auto kinematics: passes){
    currentKinematics = kinematics;
    HypothesisFit aFit = fitBothOrientations(carbon_alpha_model);
    if(!carbonAlpha.done || better(aFit.result, carbonAlpha.result)){
      carbonAlpha = aFit;
      carbonAlphaKinematics = kinematics;
    }
  }
  currentKinematics = carbonAlphaKinematics; // the model functions use it
  bestKinematics = carbonAlphaKinematics;
  HypothesisFit alpha = fitBothOrientations(alpha_model);

  /// select best hypothesis
  if(alphaOnly || better(alpha.result, carbonAlpha.result)){
    theFitResult = alpha.result;
    theFittedModel = alpha_model;
    theFittedHisto = alpha.histo;
    isReflected = alpha.reflected;
    bestFitEventType = pid_type::ALPHA;
  }
  else{
    theFitResult = carbonAlpha.result;
    theFittedModel = carbon_alpha_model;
    theFittedHisto = carbonAlpha.histo;
    isReflected = carbonAlpha.reflected;
    bestFitEventType = pid_type::C12_ALPHA;
  }
  theFittedHisto.SetDirectory(0);
  // parameters of the selected fit (the model was fitted again afterwards)
  if(theFitResult.NPar()>0) theFittedModel->SetParameters(theFitResult.Parameters().data());

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

  if(bestKinematics!=Kinematics::FREE){
    double alphaRange = maxAlphaOffset - theFittedModel->GetParameter("alphaOffset");
    double carbonRange = recoilRangeFromAlpha(alphaRange, bestKinematics)*theFittedModel->GetParameter("carbonRangeScale");
    // limited to the table, as in the model (bragg_12C)
    return std::min(std::max(carbonRange, 0.0), theMaxCarbonRange);
  }
  double carbonOffset = theFittedModel->GetParameter("carbonOffset");
  double carbonRange = maxCarbonOffset - carbonOffset;
  return carbonRange;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::getLoss() const{

  // the minimised sum of squared residuals grows with the number of bins in the fit range
  // (1.4 x track length / bin width, the bin width jumps by 2 with the track length)
  const unsigned int nBins = theFitResult.Ndf() + theFitResult.NFreeParameters();
  if(!nBins) return theFitResult.MinFcnValue();
  return theFitResult.MinFcnValue()/nBins;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
double dEdxFitter::getDiffusion() const{

  if(bestFitEventType==pid_type::UNKNOWN) return 0.0;
  return theFittedModel->GetParameter("sigma"); 
}
////////////////////////////////////////////////
////////////////////////////////////////////////
