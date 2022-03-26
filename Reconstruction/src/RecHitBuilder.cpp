#include "TH2D.h"
#include "TF1.h"

#include "GeometryTPC.h"
#include "RecHitBuilder.h"

#include "colorText.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
RecHitBuilder::RecHitBuilder(){

  noiseShape = TF1("noiseShape","pol0");
  signalShape = TF1("signalShape","gaus");
  emptyShape = TF1("emptyShape","0");

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
RecHitBuilder::~RecHitBuilder(){}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void RecHitBuilder::setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr){

  myGeometryPtr = aGeometryPtr;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TH2D & RecHitBuilder::makeRecHits(const TH2D & hProjection){

  hRecHits = hProjection;
  hRecHits.Reset();
  hRecHits.SetTitle(adaptHistoTitle(hProjection.GetTitle()).c_str());

  if(!myGeometryPtr){
    std::cerr<<__FUNCTION__<<KRED<<" NULL myGeometryPtr"<<RST<<std::endl; 
    return hRecHits;
  }

  TH2D hCleanClusters = makeCleanCluster(hProjection);
  makeTimeProjectionRecHits(hCleanClusters);
  double recHitsSum = hRecHits.Integral();
  double clusterSum = hProjection.Integral();
  double ratio = recHitsSum/clusterSum;
  if(ratio<0.2) makeStripProjectionRecHits(hProjection);
  cleanRecHits();
  return hRecHits;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TH2D & RecHitBuilder::makeTimeProjectionRecHits(const TH2D & hProjection){

  TH1D *h1DProj;
  double hitStripPos = -999.0;
  double hitTimePos = -999.0;
  double hitTimePosError = -999.0;
  double hitCharge = -999.0;
  double initialSigma = 2*myGeometryPtr->GetTimeBinWidth();//1* for 12.5MHz, 2* for 25.0 MHz
  for(int iBinY=1;iBinY<=hProjection.GetNbinsY();++iBinY){
    h1DProj = hProjection.ProjectionX("h1DProjX",iBinY, iBinY);
    const TF1 &fittedShape = fit1DProjection(h1DProj, initialSigma);
    if(fittedShape.GetNpar()<3) continue;
    hitCharge = fittedShape.GetParameter(0);
    hitTimePos = fittedShape.GetParameter(1);
    hitTimePosError = fittedShape.GetParameter(2);
    hitStripPos = hProjection.GetYaxis()->GetBinCenter(iBinY);    
    hitCharge *= sqrt(2.0)*M_PI*hitTimePosError;
    hRecHits.Fill(hitTimePos, hitStripPos, hitCharge);    
    delete h1DProj;
  }
  return hRecHits;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TH2D & RecHitBuilder::makeStripProjectionRecHits(const TH2D & hProjection){

  TH1D *h1DProj;
  double hitStripPos = -999.0;
  double hitTimePos = -999.0;
  double hitStripPosError = -999.0;
  double hitCharge = -999.0;
  double initialSigma = myGeometryPtr->GetStripPitch();
  for(int iBinX=1;iBinX<=hProjection.GetNbinsX();++iBinX){
    h1DProj = hProjection.ProjectionY("h1DProjY",iBinX, iBinX);
    const TF1 &fittedShape = fit1DProjection(h1DProj, initialSigma);
    if(fittedShape.GetNpar()<3) continue;
    hitCharge = fittedShape.GetParameter(0);
    hitStripPos = fittedShape.GetParameter(1);
    hitStripPosError = fittedShape.GetParameter(2);
    hitTimePos = hProjection.GetXaxis()->GetBinCenter(iBinX);    
    hitCharge *= sqrt(2.0)*M_PI*hitStripPosError;
    hRecHits.Fill(hitTimePos, hitStripPos, hitCharge);      
    delete h1DProj;
  }
  return hRecHits;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TF1 & RecHitBuilder::fit1DProjection(TH1D* hProj, double initialSigma){

  int maxValueBin = hProj->GetMaximumBin();
  double maxValue = hProj->GetBinContent(maxValueBin);
  
  int lowBin = FindFirstBinAbove(hProj, maxValue/4.0, 1, maxValueBin-projection1DHalfSize);
  int highBin = FindLastBinAbove(hProj, maxValue/4.0, 1, maxValueBin+projection1DHalfSize);
  if(lowBin<0) lowBin = maxValueBin-projection1DHalfSize;
  if(highBin<0) highBin = maxValueBin+projection1DHalfSize;
  int delta = std::max(std::abs(lowBin-maxValueBin),
		       std::abs(highBin-maxValueBin));
  
  lowBin = maxValueBin-delta;
  highBin= maxValueBin+delta;
  
  if(lowBin<0) lowBin = 1;
  if(highBin>hProj->GetNbinsX()) highBin = hProj->GetNbinsX();
  
  double minX = hProj->GetBinCenter(lowBin);
  double maxX = hProj->GetBinCenter(highBin);
  double windowIntegral = hProj->Integral(lowBin, highBin);

  hProj->GetXaxis()->SetRange(lowBin, highBin);
  hProj->SetMaximum(1.1*maxValue);

  if(maxValue<maxValueThr || windowIntegral<windowIntegralThr) return emptyShape;
  
  const TF1 & noiseFit = fitNoise(hProj, minX, maxX);
  const TF1 & singleHitFit = fitSingleHit(hProj, minX, maxX, maxValue, initialSigma);

  double noiseMSE = getMSE(*hProj, noiseFit);
  double singleHitMSE = getMSE(*hProj, singleHitFit);
 
  if(singleHitMSE/noiseMSE<0.9) return singleHitFit;
  return noiseFit;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TF1 & RecHitBuilder::fitNoise(TH1D* hProj, double minX, double maxX){

  noiseShape.SetRange(minX, maxX);
  TFitResultPtr noiseFitResult = hProj->Fit(&noiseShape, "QRBSWN");
  return noiseShape;  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TF1 & RecHitBuilder::fitSingleHit(TH1D* hProj,
					double minX, double maxX,
					double initialMax,
					double initialSigma){
 
  double meanX = (minX+maxX)/2.0;
  double minMeanX = meanX - (maxX - minX)*0.5*0.8;
  double maxMeanX = meanX + (maxX - minX)*0.5*0.8;
  signalShape.SetRange(minX, maxX);
  
  signalShape.SetParameter(0, initialMax);
  signalShape.SetParameter(1, meanX);
  signalShape.SetParameter(2, initialSigma);

  signalShape.SetParLimits(0, 0.5*initialMax, initialMax*1.5);
  signalShape.SetParLimits(1, minMeanX, maxMeanX);   
  signalShape.SetParLimits(2, initialSigma, 2.0*initialSigma);
  
  TFitResultPtr fitResult = hProj->Fit(&signalShape, "QBRSWN");
  return signalShape;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void RecHitBuilder::cleanRecHits(){

  double maxCharge = hRecHits.GetMaximum();
  double threshold = 0.1*maxCharge+1E-3;//FIX ME optimize threshold
  int nEntries = 0;
  for(int iBin=0;iBin<hRecHits.GetNcells();++iBin){
    if(hRecHits.GetBinContent(iBin)<threshold){
      hRecHits.SetBinContent(iBin, 0.0);
    }
    else{
      ++nEntries;
    }
  }
  hRecHits.SetEntries(nEntries);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double RecHitBuilder::getMSE(const TH1D &hProj, const TF1 & aFunc) const{

  double mse = 0.0;
  double value = 0.0;
  double x = 0.0;
  int nBins = hProj.GetXaxis()->GetLast() - hProj.GetXaxis()->GetFirst() + 1;
  for(int iBinX=hProj.GetXaxis()->GetFirst();
      iBinX<=hProj.GetXaxis()->GetLast();++iBinX){
    x = hProj.GetBinCenter(iBinX);
    value = hProj.GetBinContent(iBinX);
    mse += (value>0)*std::pow(value - aFunc.Eval(x), 2);
  }
  
  return mse/nBins; 
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<int> RecHitBuilder::fillClusterAndGetBonduary(const std::vector<int> & neighboursBinsIndices,
						     const TH2D & aHisto,
						     TH2D & aCluster){

  std::vector<int> bounduaryBins;
  double binValue = 0.0;
  
  for(auto iGlobalBin: neighboursBinsIndices){
    binValue = aHisto.GetBinContent(iGlobalBin);
    if(aCluster.GetBinContent(iGlobalBin)<emptyBinThreshold && binValue>emptyBinThreshold){
      aCluster.SetBinContent(iGlobalBin, binValue);
      bounduaryBins.push_back(iGlobalBin);
    }
  }
  return bounduaryBins;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<int> RecHitBuilder::getKernelIndices(int iBin, const TH2D & aHisto){

  std::vector<int> kernelIndices;
  int windowSizeX = 3;
  int windowSizeY = 3;

  int iGlobalBin;
  int iBinX, iBinY, iBinZ;
  aHisto.GetBinXYZ(iBin, iBinX, iBinY, iBinZ);
  
  for(int iStepX=-windowSizeX/2;iStepX<=windowSizeX/2;++iStepX){
    for(int iStepY=-windowSizeY/2;iStepY<=windowSizeY/2;++iStepY){
      iGlobalBin = aHisto.GetBin(iBinX+iStepX,
				 iBinY+iStepY,
				 iBinZ);
      kernelIndices.push_back(iGlobalBin);
    }
  }
  return kernelIndices;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double RecHitBuilder::getKernelSum(const std::vector<int> & kernelBins, const TH2D & aHisto){

  double sum = 0.0;
  for(auto iBin :kernelBins){
    sum+=aHisto.GetBinContent(iBin);
  }
  return sum;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TH2D RecHitBuilder::makeCleanCluster(const TH2D & aHisto){

  TH2D aClusterHisto(aHisto);
  aClusterHisto.Reset();

  std::vector<int> tmpVec;
  std::vector<int> bounduaryBins;
  std::vector<int> newBounduaryBins;
  std::vector<int> kernelBins;
  double kernelSum = 0.0;
  int maxValueBin = aHisto.GetMaximumBin();
  bounduaryBins.push_back(maxValueBin);

  while(bounduaryBins.size()){
    for(auto iGlobalBin: bounduaryBins){
      kernelBins = getKernelIndices(iGlobalBin, aHisto);
      kernelSum = getKernelSum(kernelBins, aHisto);
      if(kernelSum<kernelSumThreshold) continue;
      tmpVec = fillClusterAndGetBonduary(kernelBins, aHisto, aClusterHisto);
      newBounduaryBins.insert(newBounduaryBins.end(),tmpVec.begin(),tmpVec.end());	    
    }
    bounduaryBins = newBounduaryBins;
    newBounduaryBins.clear();
  }

  double value = 0.0;
  for(auto iBin=0;iBin<aClusterHisto.GetNcells();++iBin){
    value = aClusterHisto.GetBinContent(iBin);
    if(value<emptyBinThreshold) aClusterHisto.SetBinContent(iBin, 1E-10);
  }

  return aClusterHisto;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string RecHitBuilder::adaptHistoTitle(const std::string title) const{

  std::string adaptedTitle = title;
  if(adaptedTitle.find("from")!=std::string::npos){
    std::string eventNumber = title.substr(0,title.find(":"));
    adaptedTitle.replace(0, title.find("from"),"");
    adaptedTitle = eventNumber+": Reco hits "+adaptedTitle;
  }
  return adaptedTitle;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Int_t RecHitBuilder::FindFirstBinAbove(TH1* histo, Double_t threshold,
				       Int_t axis, Int_t firstBin, Int_t lastBin) const{

  if(!histo) return -1;

  if (firstBin < 1) {
    firstBin = 1;
  }
  
  if (axis == 1) {
    if (lastBin < 0 || lastBin > histo->GetNbinsX()) {
      lastBin = histo->GetNbinsX();
    }
    for (Int_t binx = firstBin; binx <= lastBin; binx++) {
      if (histo->GetBinContent(binx) > threshold) return binx;
    }
  }
  return -1;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Int_t RecHitBuilder::FindLastBinAbove(TH1* histo, Double_t threshold,
				      Int_t axis, Int_t firstBin, Int_t lastBin) const{

  if(!histo) return -1;

  if (firstBin < 1) {
    firstBin = 1;
  }

  if (lastBin < 0 || lastBin > histo->GetNbinsX()) {
    lastBin = histo->GetNbinsX();
  }
  for (Int_t binx = lastBin; binx >= firstBin; binx--) {
    if (histo->GetBinContent(binx) > threshold) return binx;
  } 
  return -1;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
