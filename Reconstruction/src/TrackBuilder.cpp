#include <cstdlib>
#include <iostream>

#include "TVector3.h"
#include "TProfile.h"
#include "TObjArray.h"
#include "TF1.h"

#include <TPolyLine3D.h>
#include <Math/Functor.h>
#include <Math/Vector3D.h>
#include <Math/Minimizer.h>

#include "GeometryTPC.h"
#include "EventTPC.h"
#include "SigClusterTPC.h"

#include "TrackBuilder.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackBuilder::TrackBuilder() {

  myEvent = 0;

  nAccumulatorRhoBins = 200;//FIX ME move to configuarable
  nAccumulatorPhiBins = 200;//FIX ME move to configuarable

  myHistoInitialized = false;
  myAccumulators.resize(3);
  my2DSeeds.resize(3);
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
  double eventMaxCharge = myEvent->GetMaxCharge();
  double chargeThreshold = 0.2*eventMaxCharge;
  int delta_timecells = 2;
  int delta_strips = 2;

  myCluster = myEvent->GetOneCluster(chargeThreshold, delta_strips, delta_timecells);


  std::string hName, hTitle;
  if(!myHistoInitialized){     
    for(int iDir = 0; iDir<3;++iDir){
      std::shared_ptr<TH2D> hRawHits = myEvent->GetStripVsTimeInMM(getCluster(), iDir);
      double maxX = hRawHits->GetXaxis()->GetXmax();
      double maxY = hRawHits->GetYaxis()->GetXmax();
      double rho = sqrt( maxX*maxX + maxY*maxY);
      hName = "hAccumulator_"+std::to_string(iDir);
      hTitle = "Hough accumulator for direction: "+std::to_string(iDir)+";#theta;#rho";
      TH2D hAccumulator(hName.c_str(), hTitle.c_str(), nAccumulatorPhiBins, -M_PI, M_PI, nAccumulatorRhoBins, 0, rho);   
      myAccumulators[iDir] = hAccumulator;
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
    my2DSeeds[iDir] = findSegment2DCollection(iDir);    
  }

  myTrack3DSeed = buildSegment3D();
  myTrack3DSegmentsFitted.push_back(fitTrack3D(myTrack3DSeed));

  return;
  
  myTrack3DSeed = myTrack3DSegmentsFitted.back();
    
  myTrack3DSeed.setStartEnd(myTrack3DSeed.getEnd(),
			    myTrack3DSeed.getEnd() + 5*myTrack3DSeed.getTangent());

  myTrack3DSegmentsFitted.push_back(fitTrack3D(myTrack3DSeed)); 
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::makeRecHits(int iDir){ 

  std::shared_ptr<TH2D> hRawHits = myEvent->GetStripVsTimeInMM(getCluster(), iDir);
  TH2D & hRecHits = myRecHits[iDir];
  hRecHits.Reset();
  std::string tmpTitle(hRecHits.GetTitle());
  if(tmpTitle.find("Event")!=std::string::npos){
    tmpTitle.replace(tmpTitle.find("Event"), 20,"Rec hits"); 
    hRecHits.SetTitle(tmpTitle.c_str());
  }

  ///TEST
  //hRecHits = *hRawHits;
  //return;
  ///////

  int maxBin;
  double maxValue;
  double firstPeakPos;
  //double windowChargeSum;
  double pdfMean1, pdfMean2;
  double pdfNorm1, pdfNorm2;

  TH1D *hProj;
  for(int iBinY=1;iBinY<hRecHits.GetNbinsY();++iBinY){
    hProj = hRawHits->ProjectionX("hProj",iBinY, iBinY);
    maxBin = hProj->GetMaximumBin();
    maxValue = hProj->GetMaximum();

    if(maxValue<1) continue;
    
    firstPeakPos = hProj->GetBinCenter(maxBin);
  
    timeResponseShape->SetParameters(maxValue, firstPeakPos, 2.5,
				     maxValue, firstPeakPos, 2.5);

    timeResponseShape->SetParLimits(0,0, maxValue*200);
    timeResponseShape->SetParLimits(1,firstPeakPos-5, firstPeakPos+5);   
    timeResponseShape->SetParLimits(2,0.5,5);

    timeResponseShape->SetParLimits(3, 0, maxValue*200);
    timeResponseShape->SetParLimits(4, firstPeakPos-30, firstPeakPos+30);   
    timeResponseShape->SetParLimits(5,0.5,5);
    
    timeResponseShape->SetRange(firstPeakPos-50, firstPeakPos+50);

    hProj->Fit(timeResponseShape.get(), "QRB");

    pdfNorm1 = timeResponseShape->GetParameter(0);
    pdfNorm2 = timeResponseShape->GetParameter(3);

    pdfMean1 = timeResponseShape->GetParameter(1);
    pdfMean2 = timeResponseShape->GetParameter(4);
    
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
const TrackSegment2D & TrackBuilder::getSegment2D(int iDir, unsigned int iTrack) const{

  if(my2DSeeds[iDir].size()<iTrack) return dummySegment2D;  
  return my2DSeeds[iDir].at(iTrack);  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TrackSegment3D & TrackBuilder::getSegment3DSeed() const{

  return myTrack3DSeed; 
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TrackSegment3D & TrackBuilder::getSegment3DFitted(unsigned int iSegment) const{

  if(myTrack3DSegmentsFitted.size()>iSegment) return myTrack3DSegmentsFitted.at(iSegment);
  else return dummySegment3D;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::fillHoughAccumulator(int iDir){

  myAccumulators[iDir].Reset();
  
  const TH2D & hRecHits  = getRecHits2D(iDir);

  double theta = 0.0, rho = 0.0;
  double x = 0.0, y=0.0;
  int charge = 0;
  for(int iBinX=0;iBinX<hRecHits.GetNbinsX();++iBinX){
    for(int iBinY=0;iBinY<hRecHits.GetNbinsY();++iBinY){
      x = hRecHits.GetXaxis()->GetBinCenter(iBinX);
      y = hRecHits.GetYaxis()->GetBinCenter(iBinY);
      charge = hRecHits.GetBinContent(iBinX, iBinY);
      if(charge<1) continue;
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
TrackSegment2DCollection TrackBuilder::findSegment2DCollection(int iDir){

  TrackSegment2DCollection aTrackCollection;
  for(int iPeak=0;iPeak<2;++iPeak){
    TrackSegment2D aTrackSegment = findSegment2D(iDir, iPeak);
    aTrackCollection.push_back(aTrackSegment);
  }
  return aTrackCollection;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackSegment2D TrackBuilder::findSegment2D(int iDir, int iPeak) const{
  
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
  
  TrackSegment2D aSegment2D(iDir);
  aSegment2D.setBiasTangent(aBias, aTangent);  		    
  return aSegment2D;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackSegment3D TrackBuilder::buildSegment3D() const{

  int iTrack2DSeed = 0;
  const TrackSegment2D & segmentU = my2DSeeds[DIR_U][iTrack2DSeed];
  const TrackSegment2D & segmentV = my2DSeeds[DIR_V][iTrack2DSeed];
  const TrackSegment2D & segmentW = my2DSeeds[DIR_W][iTrack2DSeed];
  
  double bZ = segmentU.getBiasAtT0().X();  
  double bX = (segmentU.getBiasAtT0().Y())*cos(phiPitchDirection[DIR_U]);
  double bY_fromV = (segmentV.getBiasAtT0().Y() - bX*cos(phiPitchDirection[DIR_V]))/sin(phiPitchDirection[DIR_V]);
  double bY_fromW = (segmentW.getBiasAtT0().Y() - bX*cos(phiPitchDirection[DIR_W]))/sin(phiPitchDirection[DIR_W]);  
  double bY = (bY_fromV + bY_fromW)/2.0;
  TVector3 aBias(bX, bY, bZ);

  double tZ = segmentU.getTangentWithT1().X();
  double tX = segmentU.getTangentWithT1().Y()*cos(phiPitchDirection[DIR_U]);
  double tY_fromV = (segmentV.getTangentWithT1().Y() - tX*cos(phiPitchDirection[DIR_V]))/sin(phiPitchDirection[DIR_V]);
  double tY_fromW = (segmentW.getTangentWithT1().Y() - tX*cos(phiPitchDirection[DIR_W]))/sin(phiPitchDirection[DIR_W]);
  double tY = (tY_fromV + tY_fromW)/2.0;
  TVector3 aTangent(tX, tY, tZ);

  TrackSegment3D a3DSeed;
  a3DSeed.setBiasTangent(aBias, aTangent);
  a3DSeed.setRecHits(myRecHits);

  return a3DSeed;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackSegment3D TrackBuilder::fitTrack3D(const TrackSegment3D & aTrack) const{

  TrackSegment3D fittedCandidate = aTrack;
  TrackSegment3D fittedCandidateTmp = aTrack;
  
  std::vector<double> params = {aTrack.getStart().X(),
				aTrack.getStart().Y(),
				aTrack.getStart().Z(),
				///
				aTrack.getEnd().X(),
				aTrack.getEnd().Y(),
				aTrack.getEnd().Z()};

  int nParams = params.size();
  ROOT::Fit::Fitter fitter;
  //fitter.Config().MinimizerOptions().SetMinimizerType("Minuit2");
  //fitter.Config().MinimizerOptions().SetMaxFunctionCalls(15E3);
  //fitter.Config().MinimizerOptions().SetTolerance(10.0);
  //fitter.Config().MinimizerOptions().Print(std::cout);
  ROOT::Math::Functor fcn(aTrack, nParams);

  double minChi2Length = 999;

  TVector3 aTangent = fittedCandidate.getTangent();
  for(int iStep=0;iStep<30;++iStep){

    aTangent = fittedCandidate.getTangent();    
    params = {fittedCandidate.getStart().X(),
	      fittedCandidate.getStart().Y(),
	      fittedCandidate.getStart().Z(),
	      ///
	      (fittedCandidate.getEnd()+aTangent).X(),
	      (fittedCandidate.getEnd()+aTangent).Y(),
	      (fittedCandidate.getEnd()+aTangent).Z()};
    
    fitter.SetFCN(fcn, params.data());
    
    for (int iPar = 0; iPar < nParams; ++iPar){
      fitter.Config().ParSettings(iPar).SetStepSize(1);
      fitter.Config().ParSettings(iPar).SetLimits(-100, 100);
    }			        
    bool ok = fitter.FitFCN();

    if (!ok) {
      Error(__FUNCTION__, "Track3D Fit failed");
      fitter.Result().Print(std::cout);
      //TEST return fittedCandidate;
    }    
    const ROOT::Fit::FitResult & result = fitter.Result();
    for (int iPar = 0; iPar < nParams; ++iPar){
      params[iPar] = result.Parameter(iPar);
    }
    fittedCandidate(params.data());
    std::cout <<" result.MinFcnValue() " << result.MinFcnValue() 
	      <<" fittedCandidate.getLength(): "<<fittedCandidate.getLength()
	      <<std::endl;
    //result.Print(std::cout);
  
    if(result.MinFcnValue()<minChi2Length){
      minChi2Length =  result.MinFcnValue();
      fittedCandidateTmp =  fittedCandidate;
    }
  }

  return fittedCandidateTmp;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////



/*				
  double tangentTheta = aTrack.getTangent().Theta();
  double tangentPhi = aTrack.getTangent().Phi();

  TVector3 perpPlaneBaseUnitA, perpPlaneBaseUnitB;    
  perpPlaneBaseUnitA.SetMagThetaPhi(1.0, M_PI/2.0 + tangentTheta, tangentPhi);
  perpPlaneBaseUnitB.SetMagThetaPhi(1.0, M_PI/2.0, M_PI/2.0 + tangentPhi);
  
  double biasA = aTrack.getBias().Dot(perpPlaneBaseUnitA);
  double biasB = aTrack.getBias().Dot(perpPlaneBaseUnitB);

  std::cout<<"tangentTheta: "<<tangentTheta
	   <<" tangentPhi: "<<tangentPhi
	   <<" bias A: "<<biasA
	   <<" bias B: "<<biasB
	   <<std::endl;

  std::vector<double> params = {tangentTheta, tangentPhi, biasA, biasB};
  int nParams = params.size();
  

  ROOT::Math::Functor fcn(aTrack, nParams);  
  fitter.SetFCN(fcn, params.data());
  // set step sizes different than default ones (0.3 times parameter values)
  for (int iPar = 0; iPar < nParams; ++iPar){
    fitter.Config().ParSettings(iPar).SetStepSize(0.01);
 
  }
  fitter.Config().ParSettings(0).SetLimits(0, M_PI);
  fitter.Config().ParSettings(1).SetLimits(-M_PI, M_PI);
  fitter.Config().ParSettings(2).SetLimits(-100, 100);
  fitter.Config().ParSettings(3).SetLimits(-100, 100);
  */
