#include <cstdlib>
#include <iostream>

#include "TVector3.h"
#include "TProfile.h"
#include "TObjArray.h"
#include "TF1.h"
#include "TGraph2D.h"

#include "GeometryTPC.h"
#include "EventTPC.h"
#include "SigClusterTPC.h"

#include "TrackBuilder.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackBuilder::TrackBuilder() {

  myEvent = 0;

  nAccumulatorRhoBins = 100;//FIX ME move to configuarable
  nAccumulatorPhiBins = 100;//FIX ME move to configuarable

  myHistoInitialized = false;
  myAccumulators.resize(3);
  my2DTracks.resize(3);
  myRecHits.resize(3);

  timeResponseShape = std::make_shared<TF1>("timeResponseShape","gausn(0) + gausn(3)");

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackBuilder::~TrackBuilder() {

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr){
  
  myGeometryPtr = aGeometryPtr;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::setEvent(EventTPC* aEvent){

  myEvent = aEvent;
  std::string hName, hTitle;
 
  if(!myHistoInitialized){     
    for(int iDir = 0; iDir<3;++iDir){
      std::shared_ptr<TH2D> hProjection = myEvent->GetStripVsTime(iDir);
      double maxX = hProjection->GetXaxis()->GetXmax();
      double maxY = hProjection->GetYaxis()->GetXmax();
      double rho = sqrt( maxX*maxX + maxY*maxY);
      hName = "hAccumulator_"+std::to_string(iDir);
      hTitle = "Hough accumulator for direction: "+std::to_string(iDir)+";#theta;#rho";
      TH2D hAccumulator(hName.c_str(), hTitle.c_str(), nAccumulatorPhiBins, -M_PI, M_PI, nAccumulatorRhoBins, 0, rho);   
      myAccumulators[iDir] = hAccumulator;

      std::shared_ptr<TH2D> hRawHits = myEvent->GetStripVsTime(iDir);
      std::shared_ptr<TH2D> hRecHits = std::shared_ptr<TH2D>(new TH2D(*hRawHits.get()));
      hRecHits->Reset();
      myRecHits[iDir] = *hRawHits;
    }
    myHistoInitialized = true;
  } 
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::reconstruct(){

  for(int iDir=DIR_U;iDir<DIR_3D;++iDir){
    makeRecHits(iDir);
    fillHoughAccumulator(iDir);
    my2DTracks[iDir] = findTrack2DCollection(iDir);    
  }

  myTrack3DSeed = buildTrack3D();
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::makeRecHits(int iDir){ 

  std::shared_ptr<TH2D> hRawHits = myEvent->GetStripVsTime(iDir);
  TH2D & hRecHits = myRecHits[iDir];
  hRecHits.Reset();

  int maxBin;
  double maxValue;
  double firstPeakPos;
  double windowChargeSum;
  double pdfMean1, pdfMean2;
  double pdfNorm1, pdfNorm2;

  TH1D *hProj;
  for(int iBinY=1;iBinY<hRecHits.GetNbinsY();++iBinY){
    hProj = hRawHits->ProjectionX("hProj",iBinY, iBinY);
    maxBin = hProj->GetMaximumBin();
    maxValue = hProj->GetMaximum();
    firstPeakPos = hProj->GetBinCenter(maxBin);
  
    timeResponseShape->SetParameters(maxValue, firstPeakPos, 5,
				     maxValue, firstPeakPos, 5);

    timeResponseShape->SetParLimits(0,0, maxValue*200);
    timeResponseShape->SetParLimits(1,firstPeakPos-100, firstPeakPos+100);   
    timeResponseShape->SetParLimits(2,3,15);

    timeResponseShape->SetParLimits(3, 0, maxValue*200);
    timeResponseShape->SetParLimits(4, firstPeakPos-100, firstPeakPos+100);   
    timeResponseShape->SetParLimits(5, 3,15);
    
    timeResponseShape->SetRange(firstPeakPos-150, firstPeakPos+150);

    hProj->Fit(timeResponseShape.get(), "QRB");

    windowChargeSum = hProj->Integral(maxBin-20, maxBin+20);

    pdfNorm1 = timeResponseShape->GetParameter(0);
    pdfNorm2 = timeResponseShape->GetParameter(3);

    pdfMean1 = timeResponseShape->GetParameter(1);
    pdfMean2 = timeResponseShape->GetParameter(4);
    
    if(windowChargeSum<10) continue;
    if(pdfNorm1<20 && pdfNorm2<20) continue;  
    double y = hRawHits->GetYaxis()->GetBinCenter(iBinY);
    hRecHits.Fill(pdfMean1, y, pdfNorm1);
    hRecHits.Fill(pdfMean2, y, pdfNorm2);
    delete hProj;
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TH2D & TrackBuilder::getRecHits2D(int iDir) const{

  return myRecHits[iDir];
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TH2D & TrackBuilder::getHoughtTransform(int iDir) const{

  return myAccumulators[iDir];
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const Track3D & TrackBuilder::getTrack2D(int iDir, int iTrack) const{

  return my2DTracks[iDir][iTrack];
  //return myTrack2DProjections[iDir];
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const Track3D & TrackBuilder::getTrack3D() const{

  return myTrack3DSeed;
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TVector3 getGradient(std::shared_ptr<TH2D> aHisto, int iBinX, int iBinY){

  if(iBinX==1 || iBinY==1) return TVector3(0,0,0);

  double dX = aHisto->GetBinContent(iBinX, iBinY) - aHisto->GetBinContent(iBinX-1, iBinY);
  double dY = aHisto->GetBinContent(iBinX, iBinY) - aHisto->GetBinContent(iBinX, iBinY-1);
  double dZ = 0.0;
  
  return TVector3(dX, dY, dZ);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::fillHoughAccumulator(int iDir){

  myAccumulators[iDir].Reset();
  
  const TH2D & hRecHits  = getRecHits2D(iDir);
  double maxCharge = hRecHits.GetMaximum();
  
  double theta = 0.0, rho = 0.0;
  double x = 0.0, y=0.0;
  int charge = 0;
  int chargeTreshold = maxCharge*0.02;//FIX ME move to configuarable
  for(int iBinX=0;iBinX<hRecHits.GetNbinsX();++iBinX){
    for(int iBinY=0;iBinY<hRecHits.GetNbinsY();++iBinY){
      x = hRecHits.GetXaxis()->GetBinCenter(iBinX);
      y = hRecHits.GetYaxis()->GetBinCenter(iBinY);
      charge = hRecHits.GetBinContent(iBinX, iBinY);
      if(charge<chargeTreshold) continue;            
      for(int iBinTheta=1;iBinTheta<myAccumulators[iDir].GetNbinsX();++iBinTheta){
	theta = myAccumulators[iDir].GetXaxis()->GetBinCenter(iBinTheta);
	rho = x*cos(theta) + y*sin(theta);
	charge = 1.0;
	myAccumulators[iDir].Fill(theta, rho, charge);
      }
    }
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackCollection TrackBuilder::findTrack2DCollection(int iDir){

TrackCollection aTrackCollection;
for(int iPeak=0;iPeak<1;++iPeak){
  Track3D aTrack = findTrack2D(iDir, iPeak);
  aTrackCollection.push_back(aTrack);
 }
return aTrackCollection;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Track3D TrackBuilder::findTrack2D(int iDir, int iPeak) const{
  
  int iBinX = 0, iBinY = 0, iBinZ=0;
  int margin = 5;
  const TH2D & hAccumulator = myAccumulators[iDir];

  TH2D *hAccumulator_Clone = (TH2D*)hAccumulator.Clone("hAccumulator_clone");
  myAccumulators[iDir].GetMaximumBin(iBinX, iBinY, iBinZ);  
  for(int aPeak=0;aPeak<iPeak;++aPeak){
    for(int iDeltaX=-margin;iDeltaX<=margin;++iDeltaX){
      for(int iDeltaY=-margin;iDeltaY<=margin;++iDeltaY){
	hAccumulator_Clone->SetBinContent(iBinX+iDeltaX, iBinY+iDeltaY, 0,0);
      }
    }    
    hAccumulator_Clone->GetMaximumBin(iBinX, iBinY, iBinZ);
  }
  delete hAccumulator_Clone;
  
  TVector3 aTangent, aBias;
  double theta = hAccumulator.GetXaxis()->GetBinCenter(iBinX);
  double rho = hAccumulator.GetYaxis()->GetBinCenter(iBinY);
  double aX = rho*cos(theta);
  double aY = rho*sin(theta);
  aBias.SetXYZ(aX, aY, 0.0);
  aX = -rho*sin(theta);
  aY = rho*cos(theta);
  aTangent.SetXYZ(aX, aY, 0.0);
  
  ///Set tangent direction along time arrow.
  if(aTangent.X()<0) aTangent *= -1;
  ///Normalize to X=1, so vector components can be compared between projections.
  //FIX ME: aTangent.X()!=0 !!!
  aTangent *= 1.0/aTangent.X();

  Track3D a2DSeed(aTangent, aBias, 500, iDir);
  double trackStart=0.0, trackEnd=0.0;
  std::tie(trackStart, trackEnd) = findTrackStartEnd(a2DSeed, myRecHits[iDir]); 
  		    
  return a2DSeed;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Track3D TrackBuilder::buildTrack3D() const{

  myTrack2DProjections[0].getTangent();

  double bZ = myTrack2DProjections[0].getBiasAtX0().X();  
  double bX = (myTrack2DProjections[0].getBiasAtX0().Y() + stripOffset[0])*cos(phiPitchDirection[0]);
  double bY_fromV = (myTrack2DProjections[1].getBiasAtX0().Y()+stripOffset[1] -bX*cos(phiPitchDirection[1]))/sin(phiPitchDirection[1]);
  double bY_fromW = (myTrack2DProjections[2].getBiasAtX0().Y()+stripOffset[2] -bX*cos(phiPitchDirection[2]))/sin(phiPitchDirection[2]);
  double bY = (bY_fromV + bY_fromW)/2.0;
  TVector3 aBias(bX, bY, bZ);

  double tZ = myTrack2DProjections[0].getTangent().X();
  double tX = myTrack2DProjections[0].getTangent().Y()*cos(phiPitchDirection[0]);
  double tY_fromV = (myTrack2DProjections[1].getTangent().Y() - tX*cos(phiPitchDirection[1]))/sin(phiPitchDirection[1]);
  double tY_fromW = (myTrack2DProjections[2].getTangent().Y() - tX*cos(phiPitchDirection[2]))/sin(phiPitchDirection[2]);
  double tY = (tY_fromV + tY_fromW)/2.0;
  TVector3 aTangent(tX, tY, tZ);
  if(aTangent.Z()<0) aTangent *= -1;
  ///Normalize to X=1, so vector components can be compared between projections.
  //FIX ME: aTangent.X()!=0 !!!
  aTangent *= 1.0/aTangent.Z();

  Track3D a3DSeed(aTangent, aBias, 500);

  return a3DSeed;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::tuple<double, double> TrackBuilder::findTrackStartEnd(const Track3D & aTrack2D, const TH2D  & aHits) const{

  const TVector3 & bias = aTrack2D.getBias();
  const TVector3 & tangent = aTrack2D.getTangent();

  TH1D hCharge("hCharge","",125,0,500);

  double x=0, y=0;
  double charge = 0.0;
  double lambda = 0.0;
  double value = 0.0;
  //int sign = 0.0;
  TVector3 aPoint;
  TVector3 d;
  
  for(int iBinX=1;iBinX<aHits.GetNbinsX();++iBinX){
    for(int iBinY=1;iBinY<aHits.GetNbinsY();++iBinY){
      x = aHits.GetXaxis()->GetBinCenter(iBinX);
      y = aHits.GetYaxis()->GetBinCenter(iBinY);
      charge = aHits.GetBinContent(iBinX, iBinY);
      if(charge<10) charge = 0.0;
      aPoint.SetXYZ(x, y, 0.0);
      lambda = (aPoint - bias)*tangent/tangent.Mag2();      
      d = aPoint - bias - lambda*tangent;
      if(d.Mag()>1) continue;
      value = charge/(d.Mag() + 0.001);
      hCharge.Fill(lambda, value);
    }
  }

  TH1D *hDerivative = (TH1D*)hCharge.Clone("hDerivative");
  hDerivative->Reset();
  
  for(int iBinX=2; iBinX<hCharge.GetNbinsX();++iBinX){
    value = hCharge.GetBinContent(iBinX+1) - hCharge.GetBinContent(iBinX-1);
    hDerivative->SetBinContent(iBinX, value);
  }

  double start = 0.0;
  hDerivative->Scale(1.0/hDerivative->GetMaximum());
  for(int iBinX=1; iBinX<hCharge.GetNbinsX();++iBinX){
    value = hDerivative->GetBinContent(iBinX);
    if(value>0.2){
      start = hDerivative->GetBinCenter(iBinX);
      break;
    }		    
  }

  double end = 0.0;
  hDerivative->Scale(1.0/hDerivative->GetMaximum());
  for(int iBinX=hCharge.GetNbinsX(); iBinX>1;--iBinX){
    value = hDerivative->GetBinContent(iBinX);
    if(value<-0.2){
      end = hDerivative->GetBinCenter(iBinX);
      break;
    }		    
  }

  return std::make_tuple(start, end);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
