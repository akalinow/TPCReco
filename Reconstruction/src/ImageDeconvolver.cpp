#include <cmath>
#include <iostream>
#include <memory>
#include <numeric>
#include <algorithm>

#include <TFile.h>
#include <TH1D.h>
#include <TString.h>

#include "TPCReco/ImageDeconvolver.h"
#include "TPCReco/colorText.h"

namespace {
  // histogram name suffix used by StripResponseCalculator: 0, plusN, minusN
  std::string relName(int i){
    if(i==0) return "0";
    return (i<0 ? "minus" : "plus")+std::to_string(std::abs(i));
  }
  const char dirNames[3] = {'U', 'V', 'W'};
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
ImageDeconvolver::Kernel ImageDeconvolver::makeKernel(const std::vector<double> & values){

  Kernel k;
  k.values = values;
  const double sum = std::accumulate(values.begin(), values.end(), 0.0);
  if(values.empty() || values.size()%2==0 || !(sum>0)) return Kernel();
  for(auto & v: k.values) v /= sum;
  k.centre = values.size()/2;
  const double maxValue = *std::max_element(k.values.begin(), k.values.end());
  k.first = 0;
  k.last = k.values.size()-1;
  while(k.first<k.last && k.values[k.first]<1E-6*maxValue) ++k.first;
  while(k.last>k.first && k.values[k.last]<1E-6*maxValue) --k.last;
  return k;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void ImageDeconvolver::setKernels(int dir, const std::vector<double> & stripKernel,
                                  const std::vector<double> & timeKernel){
  if(dir<0 || dir>2) return;
  baseStripKernels[dir] = stripKernel;
  baseTimeKernels[dir] = timeKernel;
  updateKernels(dir, 0.0, 0.0);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<double> ImageDeconvolver::broaden(const std::vector<double> & values, double sigma){

  if(!(sigma>0) || values.empty()) return values;
  const int n = std::max(1, (int)std::ceil(4.0*sigma));
  std::vector<double> out(values.size()+2*n, 0.0);  // stays odd and centred
  for(size_t i=0; i<values.size(); ++i)
    for(int j=-n; j<=n; ++j) out[i+n+j] += values[i]*std::exp(-0.5*j*j/sigma/sigma);
  return out;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void ImageDeconvolver::updateKernels(int dir, double sigmaTimeBins, double sigmaStripBins){
  stripKernels[dir] = makeKernel(broaden(baseStripKernels[dir], sigmaStripBins));
  timeKernels[dir] = makeKernel(broaden(baseTimeKernels[dir], sigmaTimeBins));
  kernelSigmaTime[dir] = sigmaTimeBins;
  kernelSigmaStrip[dir] = sigmaStripBins;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool ImageDeconvolver::hasKernels(int dir) const{
  return dir>=0 && dir<=2 && !stripKernels[dir].values.empty() && !timeKernels[dir].values.empty();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool ImageDeconvolver::loadResponseFile(const std::string & fileName){

  std::unique_ptr<TFile> f(TFile::Open(fileName.c_str(), "READ"));
  if(!f || f->IsZombie()){
    std::cout<<KRED<<"ImageDeconvolver: cannot open response file "<<RST<<fileName<<std::endl;
    return false;
  }
  // time response: h_respZ_<offset>, bins = charge position within the reference time cell -> average
  if(!f->Get("h_respZ_0")){
    std::cout<<KRED<<"ImageDeconvolver: no h_respZ_* histograms in "<<RST<<fileName<<std::endl;
    return false;
  }
  int nT = 0;
  while(f->Get(("h_respZ_"+relName(-(nT+1))).c_str())) ++nT;
  std::vector<double> timeKernel(2*nT+1, 0.0);
  for(int i=-nT; i<=nT; ++i){
    auto h = dynamic_cast<TH1*>(f->Get(("h_respZ_"+relName(i)).c_str()));
    if(!h) return false;
    double sum = 0.0;
    for(int b=1; b<=h->GetNbinsX(); ++b) sum += h->GetBinContent(b);
    timeKernel[i+nT] = sum/h->GetNbinsX();
  }
  // strip response: h_respXY_<dir><offset>, bins = charge XY position wrt the reference node -> average
  for(int dir=0; dir<3; ++dir){
    std::vector<double> stripKernel;
    int nS = 0;
    while(f->Get(Form("h_respXY_%c%s", dirNames[dir], relName(-(nS+1)).c_str()))) ++nS;
    for(int i=-nS; i<=nS; ++i){
      auto h = dynamic_cast<TH2*>(f->Get(Form("h_respXY_%c%s", dirNames[dir], relName(i).c_str())));
      if(!h) return false;
      double sum = 0.0;
      for(int bx=1; bx<=h->GetNbinsX(); ++bx)
        for(int by=1; by<=h->GetNbinsY(); ++by) sum += h->GetBinContent(bx, by);
      stripKernel.push_back(sum/(h->GetNbinsX()*h->GetNbinsY()));
    }
    setKernels(dir, stripKernel, timeKernel);
    if(!hasKernels(dir)){
      std::cout<<KRED<<"ImageDeconvolver: invalid kernels for direction "<<dirNames[dir]<<" in "<<RST<<fileName<<std::endl;
      return false;
    }
  }
  std::cout<<KBLU<<"ImageDeconvolver: response from "<<RST<<fileName
           <<KBLU<<" (time kernel "<<timeKernel.size()<<" cells, strip kernel "
           <<stripKernels[0].values.size()<<" strips)"<<RST<<std::endl;
  return true;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void ImageDeconvolver::convolve1D(const std::vector<double> & in, std::vector<double> & out, int nStrip, int nTime,
                                  const Kernel & k, bool alongTime, bool flip){
  // out[i] = sum_j k[j] in[i-(j-c)] (convolution) or sum_j k[j] in[i+(j-c)] (correlation, flip)
  out.assign(in.size(), 0.0);
  for(int s=0; s<nStrip; ++s){
    for(int t=0; t<nTime; ++t){
      const double v = in[s*nTime+t];
      if(v==0.0) continue;
      // scatter form: in[pos] contributes to out[pos + (j-c)] (convolution) or out[pos - (j-c)] (correlation)
      for(int j=k.first; j<=k.last; ++j){
        const int shift = flip ? -(j-k.centre) : (j-k.centre);
        if(alongTime){
          const int tt = t+shift;
          if(tt>=0 && tt<nTime) out[s*nTime+tt] += k.values[j]*v;
        }
        else{
          const int ss = s+shift;
          if(ss>=0 && ss<nStrip) out[ss*nTime+t] += k.values[j]*v;
        }
      }
    }
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void ImageDeconvolver::convolve(const std::vector<double> & in, std::vector<double> & out, int nStrip, int nTime,
                                int dir, bool flip) const{
  std::vector<double> tmp;
  convolve1D(in, tmp, nStrip, nTime, timeKernels[dir], true, flip);
  convolve1D(tmp, out, nStrip, nTime, stripKernels[dir], false, flip);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void ImageDeconvolver::gaussianSmooth(std::vector<double> & image, int nStrip, int nTime,
                                      double sigmaStrip, double sigmaTime){
  auto gaussKernel = [](double sigma){
    std::vector<double> v;
    if(!(sigma>0)) return std::vector<double>{1.0};
    const int n = std::max(1, (int)std::ceil(4.0*sigma));
    for(int i=-n; i<=n; ++i) v.push_back(std::exp(-0.5*i*i/sigma/sigma));
    return v;
  };
  std::vector<double> tmp;
  convolve1D(image, tmp, nStrip, nTime, makeKernel(gaussKernel(sigmaTime)), true, false);
  convolve1D(tmp, image, nStrip, nTime, makeKernel(gaussKernel(sigmaStrip)), false, false);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int ImageDeconvolver::richardsonLucy(std::vector<double> & image, int nStrip, int nTime, int dir) const{

  if(!hasKernels(dir) || nStrip<=0 || nTime<=0) return 0;
  // pad by the kernel extent, so the charge moved out of the window is not lost during the iterations
  const Kernel & kS = stripKernels[dir];
  const Kernel & kT = timeKernels[dir];
  const int padS = std::max(kS.centre-kS.first, kS.last-kS.centre);
  const int padT = std::max(kT.centre-kT.first, kT.last-kT.centre);
  const int nS = nStrip+2*padS, nT = nTime+2*padT;

  std::vector<double> y(nS*nT, 0.0);
  double ySum = 0.0;
  for(int s=0; s<nStrip; ++s)
    for(int t=0; t<nTime; ++t){
      const double v = std::max(0.0, image[s*nTime+t]);
      y[(s+padS)*nT+t+padT] = v;
      ySum += v;
    }
  if(!(ySum>0)) return 0;

  std::vector<double> x(nS*nT, ySum/(nS*nT)), est, ratio(nS*nT), corr;
  double previousResidual = -1.0;
  int iteration = 0;
  for(iteration=0; iteration<maxIterations; ++iteration){
    convolve(x, est, nS, nT, dir, false);
    // rms residual of the current estimate inside the original window
    double residual = 0.0;
    for(int s=0; s<nStrip; ++s)
      for(int t=0; t<nTime; ++t){
        const int i = (s+padS)*nT+t+padT;
        residual += std::pow(est[i]-image[s*nTime+t], 2);
      }
    residual = std::sqrt(residual/(nStrip*nTime));
    if(previousResidual>0 && (previousResidual-residual)/previousResidual<stopEps) break;
    previousResidual = residual;
    for(size_t i=0; i<y.size(); ++i) ratio[i] = y[i]/std::max(est[i], 1E-12);
    convolve(ratio, corr, nS, nT, dir, true);
    for(size_t i=0; i<x.size(); ++i) x[i] *= corr[i];
  }
  gaussianSmooth(x, nS, nT, targetSigmaStrip, targetSigmaTime);
  for(int s=0; s<nStrip; ++s)
    for(int t=0; t<nTime; ++t) image[s*nTime+t] = x[(s+padS)*nT+t+padT];
  return iteration;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TH2D ImageDeconvolver::deconvolve(const TH2D & hInput, int dir){

  lastIterations = 0;
  TH2D hOutput(hInput);
  if(!hasKernels(dir)) return hOutput;

  // extra Gaussian in bins of this histogram (X: time, Y: strip; mm scale)
  const double sigmaTime = extraSigmaTimeMM>0 ? extraSigmaTimeMM/hInput.GetXaxis()->GetBinWidth(1) : 0.0;
  const double sigmaStrip = extraSigmaStripMM>0 ? extraSigmaStripMM/hInput.GetYaxis()->GetBinWidth(1) : 0.0;
  if(std::abs(sigmaTime-kernelSigmaTime[dir])>1E-9 || std::abs(sigmaStrip-kernelSigmaStrip[dir])>1E-9)
    updateKernels(dir, sigmaTime, sigmaStrip);

  const int nX = hInput.GetNbinsX(), nY = hInput.GetNbinsY();
  int minX = nX+1, maxX = 0, minY = nY+1, maxY = 0;
  for(int ix=1; ix<=nX; ++ix)
    for(int iy=1; iy<=nY; ++iy){
      if(hInput.GetBinContent(ix, iy)==0.0) continue;
      minX = std::min(minX, ix); maxX = std::max(maxX, ix);
      minY = std::min(minY, iy); maxY = std::max(maxY, iy);
    }
  if(maxX<minX) return hOutput;
  minX = std::max(1, minX-marginTime); maxX = std::min(nX, maxX+marginTime);
  minY = std::max(1, minY-marginStrip); maxY = std::min(nY, maxY+marginStrip);

  const int nTime = maxX-minX+1, nStrip = maxY-minY+1;
  std::vector<double> image(nStrip*nTime);
  for(int s=0; s<nStrip; ++s)
    for(int t=0; t<nTime; ++t) image[s*nTime+t] = hInput.GetBinContent(minX+t, minY+s);

  lastIterations = richardsonLucy(image, nStrip, nTime, dir);

  const double maxValue = *std::max_element(image.begin(), image.end());
  hOutput.Reset();
  for(int s=0; s<nStrip; ++s)
    for(int t=0; t<nTime; ++t){
      const double v = image[s*nTime+t];
      if(v>zeroFraction*maxValue) hOutput.SetBinContent(minX+t, minY+s, v);
    }
  return hOutput;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
