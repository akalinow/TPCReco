#include <cstdlib>
#include <iostream>

#include "TVector3.h"
#include "TProfile.h"
#include "TObjArray.h"
#include "TF1.h"
#include "TFitResult.h"
#include "Math/Functor.h"

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

  fitter.Config().MinimizerOptions().SetMinimizerType("GSLSimAn");
  fitter.Config().MinimizerOptions().SetMaxFunctionCalls(1E4);
  fitter.Config().MinimizerOptions().SetMaxIterations(1E4);
  fitter.Config().MinimizerOptions().SetTolerance(1E-2);
  //fitter.Config().MinimizerOptions().Print(std::cout);

  ///An offset used for filling the Hough transformation.
  ///to avoid having very small rho parameters, as
  ///orignally manty track traverse close to X=0, Time=0
  ///point.
  aHoughOffest.SetX(20.0);
  aHoughOffest.SetY(40.0);
  aHoughOffest.SetZ(0.0);
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
  double chargeThreshold = 0.15*eventMaxCharge;
  int delta_timecells = 10;
  int delta_strips = 1;

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
  myFittedTrack = fitTrack3D(myTrack3DSeed);
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

  TH1D *hProj;
  double hitWirePos = -999.0;
  double hitTimePos = -999.0;
  double hitTimePosError = -999.0;
  double hitCharge = -999.0;
  for(int iBinY=1;iBinY<hRecHits.GetNbinsY();++iBinY){
    hProj = hRawHits->ProjectionX("hProj",iBinY, iBinY);
    TF1 timeResponseShape = fitTimeWindow(hProj);

    hitWirePos = hRawHits->GetYaxis()->GetBinCenter(iBinY);
    for(int iSet=0;iSet<timeResponseShape.GetNpar();iSet+=3){
      hitTimePos = timeResponseShape.GetParameter(iSet+1);
      hitTimePosError = timeResponseShape.GetParameter(iSet+2);
      hitCharge = timeResponseShape.GetParameter(iSet);
      hitCharge *= sqrt(2.0)*M_PI*hitTimePosError;//the gausian fits are made without normalisation factor
      hRecHits.Fill(hitTimePos, hitWirePos, hitCharge);
    }
    delete hProj;
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TF1 TrackBuilder::fitTimeWindow(TH1D* hProj){

   TFitResultPtr fitResult;
   TF1 timeResponseShape;
   TF1 bestTimeResponseShape;
   double bestChi2OverNDF = 1E10;

   int maxBin = hProj->GetMaximumBin();
   double maxValue = hProj->GetMaximum();
   double maxPos = hProj->GetBinCenter(maxBin);
   if(maxValue<1) return bestTimeResponseShape;

   std::string formula = "";
   for(int iComponent=0;iComponent<3;++iComponent){
     if(iComponent==0){
       formula = "gaus("+std::to_string(3*iComponent)+")";
     }
     else{
       formula += "+gaus("+std::to_string(3*iComponent)+")";

     }
     TF1 timeResponseShape("timeResponseShape",formula.c_str());        
     timeResponseShape.SetRange(maxPos-25, maxPos+25);  
   
     for(int iSet=0;iSet<timeResponseShape.GetNpar();iSet+=3){
       timeResponseShape.SetParameter(iSet, maxValue*2);
       timeResponseShape.SetParameter(iSet+1, maxPos);
       timeResponseShape.SetParameter(iSet+2, 2.0);
       ///     
       timeResponseShape.SetParLimits(iSet, 0.1*maxValue, maxValue*2);
       timeResponseShape.SetParLimits(iSet+1, maxPos-15, maxPos+15);   
       timeResponseShape.SetParLimits(iSet+2, 0.5, 8);
     }   
     fitResult = hProj->Fit(&timeResponseShape, "QRBSW");   

     double chi2 = 0.0;
     double x = 0.0;
     for(int iBinX=1;iBinX<hProj->GetNbinsX();++iBinX){
       x = hProj->GetBinCenter(iBinX);
       chi2 += std::pow(hProj->GetBinContent(iBinX) - timeResponseShape.Eval(x), 2);
     }
     /*
     std::cout<<"nComponents: "<<iComponent+1
	      <<" histogram chi2: "<<chi2
	      <<" ndf: "<<fitResult->Ndf()
	      <<" NFreeParameters: "<<fitResult->NFreeParameters()
	      <<" chi2/ndf: "<<chi2/fitResult->Ndf()
	      <<std::endl;
     */
     if(fitResult->Ndf() && chi2/fitResult->Ndf() < bestChi2OverNDF){
       bestChi2OverNDF = chi2/fitResult->Ndf();
       timeResponseShape.Copy(bestTimeResponseShape);
     }
   }
   return bestTimeResponseShape;
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
const Track3D & TrackBuilder::getTrack3D(unsigned int iSegment) const{

  return myFittedTrack;
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
      x = hRecHits.GetXaxis()->GetBinCenter(iBinX) + aHoughOffest.X();
      y = hRecHits.GetYaxis()->GetBinCenter(iBinY) + aHoughOffest.Y();
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

  aBias -= aHoughOffest.Dot(aBias.Unit())*aBias.Unit();
  
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
Track3D TrackBuilder::fitTrack3D(const TrackSegment3D & aTrackSegment) const{

  Track3D aTrackCandidate;
  aTrackCandidate.addSegment(aTrackSegment);
  aTrackCandidate.extendToWholeChamber();
  aTrackCandidate.shrinkToHits();
  aTrackCandidate.splitWorseChi2Segment(0.5);
  //aTrackCandidate.splitWorseChi2Segment(0.615);
  /*
  double bestSplit = fitTrackSplitPoint(aTrackCandidate);
  std::cout<<" chi2 from split: "<<aTrackCandidate.chi2FromSplitPoint(&bestSplit)<<std::endl;
  aTrackCandidate.splitWorseChi2Segment(bestSplit);
  std::cout<<aTrackCandidate<<std::endl;  
  //return aTrackCandidate;
  */
  
  Track3D aBestCandidate;

  double minChi2 = 1E10;
  for(unsigned int iStep=0;iStep<2;++iStep){
    
    std::cout<<__FUNCTION__<<" iStep: "<<iStep<<std::endl;      

    std::vector<double> params = aTrackCandidate.getSegmentsStartEndXYZ();
    int nParams = params.size();  
    ////
    std::cout<<"Pre-fit: "<<std::endl; 
    std::cout<<aTrackCandidate<<std::endl;
    //return aTrackCandidate;
    //continue;
    ////

    ROOT::Math::Functor fcn(&aTrackCandidate, &Track3D::chi2FromNodesList, nParams);
    fitter.SetFCN(fcn, params.data());

    for (int iPar = 0; iPar < nParams; ++iPar){
      fitter.Config().ParSettings(iPar).SetStepSize(0.1);
      fitter.Config().ParSettings(iPar).SetLimits(-100, 100);
    }
    bool fitStatus = fitter.FitFCN();
    if (!fitStatus) {
      Error(__FUNCTION__, "Track3D Fit failed");
      fitter.Result().Print(std::cout);
      return aTrackCandidate;
    }    
    const ROOT::Fit::FitResult & result = fitter.Result();
    aTrackCandidate(result.GetParams());

    std::cout<<"Post-fit: "<<std::endl; 
    std::cout<<aTrackCandidate<<std::endl;
    std::cout<<" result.MinFcnValue(): "<<result.MinFcnValue()<<std::endl;
    
    if(result.MinFcnValue()<minChi2){
      minChi2 =  result.MinFcnValue();
      aBestCandidate = aTrackCandidate;
    }
    aTrackCandidate.removeEmptySegments();
    //aTrackCandidate.extendToWholeChamber();
    //aTrackCandidate.shrinkToHits();
    aTrackCandidate.splitWorseChi2Segment(0.5);
  }

  aBestCandidate.removeEmptySegments();
  return aBestCandidate;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackBuilder::fitTrackSplitPoint(const Track3D& aTrack) const{

  Track3D aTrackCandidate = aTrack;
  double currentBestChi2 = aTrackCandidate.getChi2();
  double bestSplit = -1.0;

  int nSplitSteps = 400;
  double splitStep = 1.0/nSplitSteps;
  for(int iSplitStep=1; iSplitStep<nSplitSteps;++iSplitStep){

    std::cout<<"iSplitStep: "<<iSplitStep<<std::endl;
    
    aTrackCandidate = aTrack;
    aTrackCandidate.splitWorseChi2Segment(splitStep*iSplitStep);

    std::vector<double> params = aTrackCandidate.getSegmentsStartEndXYZ();
    int nParams = params.size();  
    ////
    //std::cout<<"Split "<<splitStep*iSplitStep<<" Pre-fit: "<<std::endl; 
    //std::cout<<aTrackCandidate<<std::endl;
    ////

    ROOT::Math::Functor fcn(&aTrackCandidate, &Track3D::chi2FromNodesList, nParams);
    fitter.SetFCN(fcn, params.data());

    for (int iPar = 0; iPar < nParams; ++iPar){
      fitter.Config().ParSettings(iPar).SetStepSize(0.01);
      fitter.Config().ParSettings(iPar).SetLimits(-100, 100);
    }
    bool fitStatus = fitter.FitFCN();
    if (!fitStatus) {
      Error(__FUNCTION__, "Track3D Fit failed");
      fitter.Result().Print(std::cout);
      return -1.0;
    }    
    const ROOT::Fit::FitResult & result = fitter.Result();
    if(result.MinFcnValue()<currentBestChi2){
      currentBestChi2 = result.MinFcnValue();
      bestSplit = splitStep*iSplitStep;
      std::cout<<aTrackCandidate<<std::endl;    
    }
    //aTrackCandidate(result.GetParams());
    //std::cout<<"Split Post-fit: "<<std::endl; 
    //std::cout<<aTrackCandidate<<std::endl;    
  }
  std::cout<<" currentBestChi2: "<<currentBestChi2<<" bestSplit: "<<bestSplit<<std::endl;
  return bestSplit;
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
