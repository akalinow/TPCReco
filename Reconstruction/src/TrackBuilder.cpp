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
  fitTrack3D(myTrack3DSeed);
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::makeRecHits(int iDir){ 

  std::shared_ptr<TH2D> hRawHits = myEvent->GetStripVsTime(iDir);
  TH2D & hRecHits = myRecHits[iDir];
  hRecHits.Reset();
  std::string tmpTitle(hRecHits.GetTitle());
  if(tmpTitle.find("Event")!=std::string::npos){
    tmpTitle.replace(tmpTitle.find("Event"), 20,"Rec hits"); 
    hRecHits.SetTitle(tmpTitle.c_str());
  }

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

    //FIX ME optimize this, and move to configuretion
    if(windowChargeSum<100) continue;
    if(pdfNorm1+pdfNorm2<100) continue;
    if(pdfNorm1<20 && pdfNorm2<20) continue;
    /////
    
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
  int chargeTreshold = maxCharge*0.02;//FIX ME optimize and move to configuarable
  for(int iBinX=0;iBinX<hRecHits.GetNbinsX();++iBinX){
    for(int iBinY=0;iBinY<hRecHits.GetNbinsY();++iBinY){
      x = hRecHits.GetXaxis()->GetBinCenter(iBinX);
      y = hRecHits.GetYaxis()->GetBinCenter(iBinY);
      charge = hRecHits.GetBinContent(iBinX, iBinY);
      if(charge<chargeTreshold) continue;            
      for(int iBinTheta=1;iBinTheta<myAccumulators[iDir].GetNbinsX();++iBinTheta){
	theta = myAccumulators[iDir].GetXaxis()->GetBinCenter(iBinTheta);
	rho = x*cos(theta) + y*sin(theta);
	charge = 1.0; //FIX me study is how to include charge. 
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

  Track3D a2DSeed(aTangent, aBias, iDir);
  double trackStart=0.0, trackEnd=0.0;
  std::cout<<"iDir: "<<iDir<<std::endl;
  std::tie(trackStart, trackEnd) = findTrackStartEnd(a2DSeed, myRecHits[iDir]);
  a2DSeed.setStartEnd(trackStart, trackEnd);
  		    
  return a2DSeed;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Track3D TrackBuilder::fitTrack3D(const Track3D & aTrack) const{
  /*
  double chi2 = 0.0;
  for(int iDir=DIR_U;iDir<=DIR_W;++iDir){
    const Track3D & aTrack2DProjection = aTrack.get2DProjection(iDir);
    std::shared_ptr<TH2D> aRecHits = std::make_shared<TH2D>(&myRecHits[iDir]);
    chi2 += aTrack2DProjection.get2DProjectionRecHitChi2(aRecHits);    
  }
  std::cout<<"chi2: "<<chi2<<std::endl;
  */
  return Track3D();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Track3D TrackBuilder::buildTrack3D() const{

  int iTrack2DSeed = 0;
  const Track3D & segmentU = my2DTracks[DIR_U][iTrack2DSeed];
  const Track3D & segmentV = my2DTracks[DIR_V][iTrack2DSeed];
  const Track3D & segmentW = my2DTracks[DIR_W][iTrack2DSeed];
  
  double bZ = segmentU.getBiasAtX0().X();  
  double bX = (segmentU.getBiasAtX0().Y() + stripOffset[DIR_U])*cos(phiPitchDirection[DIR_U]);
  double bY_fromV = (segmentV.getBiasAtX0().Y()+stripOffset[DIR_V] - bX*cos(phiPitchDirection[DIR_V]))/sin(phiPitchDirection[DIR_V]);
  double bY_fromW = (segmentW.getBiasAtX0().Y()+stripOffset[DIR_W] - bX*cos(phiPitchDirection[DIR_W]))/sin(phiPitchDirection[DIR_W]);  
  double bY = (bY_fromV + bY_fromW)/2.0;
  TVector3 aBias(bX, bY, bZ);

  double tZ = segmentU.getTangent().X();
  double tX = segmentU.getTangent().Y()*cos(phiPitchDirection[DIR_U]);
  double tY_fromV = (segmentV.getTangent().Y() - tX*cos(phiPitchDirection[DIR_V]))/sin(phiPitchDirection[DIR_V]);
  double tY_fromW = (segmentW.getTangent().Y() - tX*cos(phiPitchDirection[DIR_W]))/sin(phiPitchDirection[DIR_W]);
  double tY = (tY_fromV + tY_fromW)/2.0;
  TVector3 aTangent(tX, tY, tZ);
  if(aTangent.Z()<0) aTangent *= -1;
  ///Z is the time direction
  ///Normalize to Z=1, so vector components can be compared between projections.
  ///FIX ME: aTangent.Z()!=0 !!!
  ///Use dynamic selection of normalised direction? Choose direction with longest projection?
  aTangent *= 1.0/aTangent.Z();

  Track3D a3DSeed(aTangent, aBias, DIR_3D);
  double startTime = 1/3.0*(segmentU.getStartTime() + segmentV.getStartTime() + segmentW.getStartTime());
  double endTime = 1/3.0*(segmentU.getEndTime() + segmentV.getEndTime() + segmentW.getEndTime());
  a3DSeed.setStartEnd(startTime, endTime);

  return a3DSeed;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::tuple<double, double> TrackBuilder::findTrackStartEnd(const Track3D & aTrack2D, const TH2D  & aHits) const{

  const TVector3 & bias = aTrack2D.getBiasAtX0();
  const TVector3 & tangent = aTrack2D.getTangent();

  TH1D hCharge("hCharge","",125,0,500);

  double x=0, y=0;
  double charge = 0.0;
  double lambda = 0.0;
  double value = 0.0;
  TVector3 aPoint;
  TVector3 d;
  
  for(int iBinX=1;iBinX<aHits.GetNbinsX();++iBinX){
    for(int iBinY=1;iBinY<aHits.GetNbinsY();++iBinY){
      x = aHits.GetXaxis()->GetBinCenter(iBinX);
      y = aHits.GetYaxis()->GetBinCenter(iBinY);
      charge = aHits.GetBinContent(iBinX, iBinY);
      if(charge<100) continue;//FIX ME move to configuration
      aPoint.SetXYZ(x, y, 0.0);
      lambda = (aPoint - bias)*tangent/tangent.Mag2();      
      d = aPoint - bias - lambda*tangent;
      if(d.Mag()>15) continue;//FIX ME move to configuration
      value = (charge>0)*d.Mag();
      hCharge.Fill(lambda, value);
    }
  }

  double start = -999;
  double end = 0.0;
  for(int iBinX=1; iBinX<hCharge.GetNbinsX();++iBinX){
    value = hCharge.GetBinContent(iBinX);
    if(start<0 && value) start = hCharge.GetBinCenter(iBinX);
    if(start>0 && value) end = hCharge.GetBinCenter(iBinX);
  }

  return std::make_tuple(start, end);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
