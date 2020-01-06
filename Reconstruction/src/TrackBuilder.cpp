#include "TrackBuilder.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackBuilder::TrackBuilder() {

  nAccumulatorRhoBins = 100;//FIX ME move to configuarable
  nAccumulatorPhiBins = 100;//FIX ME move to configuarable

  myHistoInitialized = false;
  myAccumulators.resize(3);
  my2DSeeds.resize(3);
  myRecHits.resize(3);

  fitter.Config().MinimizerOptions().SetMinimizerType("GSLSimAn");
  fitter.Config().MinimizerOptions().SetMaxFunctionCalls(1E6);
  fitter.Config().MinimizerOptions().SetMaxIterations(1E6);
  fitter.Config().MinimizerOptions().SetTolerance(1E-2);
  fitter.Config().MinimizerOptions().Print(std::cout);

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
void TrackBuilder::setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr){
  
  myGeometryPtr = aGeometryPtr;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::setEvent(std::shared_ptr<EventTPC> aEvent){

  myEvent = aEvent;
  double eventMaxCharge = myEvent->GetMaxCharge();
  double chargeThreshold = 0.15*eventMaxCharge;
  int delta_timecells = 15;
  int delta_strips = 1;

  myCluster = myEvent->GetOneCluster(chargeThreshold, delta_strips, delta_timecells);

  std::string hName, hTitle;
  if(!myHistoInitialized){     
      std::shared_ptr<TH2D> hRawHits;
    for(int iDir = 0; iDir<3;++iDir){
      hRawHits = myEvent->GetStripVsTimeInMM(getCluster(), projection(iDir));
      double maxX = hRawHits->GetXaxis()->GetXmax();
      double maxY = hRawHits->GetYaxis()->GetXmax();
      double rho = std::hypot(maxX, maxY);
      hName = "hAccumulator_"+std::to_string(iDir);
      hTitle = "Hough accumulator for direction: "+std::to_string(iDir)+";#theta;#rho";
      TH2D hAccumulator(hName.c_str(), hTitle.c_str(), nAccumulatorPhiBins, -pi, pi, nAccumulatorRhoBins, 0, rho);   
      myAccumulators[iDir] = hAccumulator;
      myRecHits[iDir] = *hRawHits;
    }
    myHistoInitialized = true;
  } 
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::reconstruct(){

  for(auto&& iDir : proj_vec_UVW){
    makeRecHits(int(iDir));
    fillHoughAccumulator(int(iDir));
    my2DSeeds[int(iDir)] = findSegment2DCollection(int(iDir));    
  }
  myTrack3DSeed = buildSegment3D();
  myFittedTrack = fitTrack3D(myTrack3DSeed);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::makeRecHits(int iDir){ 

  auto hRawHits = myEvent->GetStripVsTimeInMM(getCluster(), projection(iDir));
  TH2D & hRecHits = myRecHits[iDir];
  hRecHits.Reset();
  std::string tmpTitle(hRecHits.GetTitle());
  if(tmpTitle.find("Event")!=std::string::npos){
    tmpTitle.replace(tmpTitle.find("Event"), 20,"Rec hits"); 
    hRecHits.SetTitle(tmpTitle.c_str());
  }

  TH1D *hProj;
  double hitWirePos = -999.0;
  for(int iBinY=1;iBinY<hRecHits.GetNbinsY();++iBinY){
    hProj = hRawHits->ProjectionX("hProj",iBinY, iBinY);
    TF1 timeResponseShape = fitTimeWindow(hProj);

    hitWirePos = hRawHits->GetYaxis()->GetBinCenter(iBinY);
    for(int iSet=0;iSet<timeResponseShape.GetNpar();iSet+=3){
      double hitTimePos = timeResponseShape.GetParameter(iSet+1);
      double hitTimePosError = timeResponseShape.GetParameter(iSet+2);
      double hitCharge = timeResponseShape.GetParameter(iSet);
      hitCharge *= sqrt(2.0)*pi*hitTimePosError;//the gausian fits are made without the normalisation factor
      if(hitCharge>50) hRecHits.Fill(hitTimePos, hitWirePos, hitCharge);//FIXME optimize, use dynamic threshold?
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
   double windowIntegral = hProj->Integral(maxBin-25, maxBin+25);
   if(maxValue<25 || windowIntegral<50) return bestTimeResponseShape;//FIXME how to choose the thresholds?

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
     for(int iBinX=1;iBinX<hProj->GetNbinsX();++iBinX){
       double x = hProj->GetBinCenter(iBinX);
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
  for(int iBinX=1;iBinX<hRecHits.GetNbinsX();++iBinX){
    for(int iBinY=1;iBinY<hRecHits.GetNbinsY();++iBinY){
      x = hRecHits.GetXaxis()->GetBinCenter(iBinX) + aHoughOffest.X();
      y = hRecHits.GetYaxis()->GetBinCenter(iBinY) + aHoughOffest.Y();
      charge = hRecHits.GetBinContent(iBinX, iBinY);
      if(charge<1) continue;
      for(int iBinTheta=1;iBinTheta<myAccumulators[iDir].GetNbinsX();++iBinTheta){
	theta = myAccumulators[iDir].GetXaxis()->GetBinCenter(iBinTheta);
	rho = x*cos(theta) + y*sin(theta);
	charge = 1.0; //FIX me study how to include the charge. 
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
  int nHits = hAccumulator.GetBinContent(iBinX, iBinY);
  double theta = hAccumulator.GetXaxis()->GetBinCenter(iBinX);
  double rho = hAccumulator.GetYaxis()->GetBinCenter(iBinY);
  double aX = rho*cos(theta);
  double aY = rho*sin(theta);
  aBias.SetXYZ(aX, aY, 0.0);

  aBias -= aHoughOffest.Dot(aBias.Unit())*aBias.Unit();
  
  aX = -rho*sin(theta);
  aY = rho*cos(theta);
  aTangent.SetXYZ(aX, aY, 0.0);
  
  TrackSegment2D aSegment2D{ projection(iDir) };
  aSegment2D.setBiasTangent(aBias, aTangent);
  aSegment2D.setNAccumulatorHits(nHits);
  return aSegment2D;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackSegment3D TrackBuilder::buildSegment3D() const{

  int iTrack2DSeed = 0;
  auto& segmentU = my2DSeeds[int(projection::DIR_U)][iTrack2DSeed];
  auto& segmentV = my2DSeeds[int(projection::DIR_V)][iTrack2DSeed];
  auto& segmentW = my2DSeeds[int(projection::DIR_W)][iTrack2DSeed];

  int64_t nHits_U = segmentU.getNAccumulatorHits();
  int64_t nHits_V = segmentV.getNAccumulatorHits();
  int64_t nHits_W = segmentW.getNAccumulatorHits();
  
  double bZ_fromU = segmentU.getBiasAtT0().X();
  double bZ_fromV = segmentV.getBiasAtT0().X();
  double bZ_fromW = segmentW.getBiasAtT0().X();
  double bZ = (bZ_fromU*nHits_U + bZ_fromV*nHits_V + bZ_fromW*nHits_W)/(nHits_U+nHits_V+nHits_W);
  
  double bX_fromU = (segmentU.getBiasAtT0().Y())*cos(phiPitchDirection[int(projection::DIR_U)]);
  double bY_fromV = (segmentV.getBiasAtT0().Y() - bX_fromU*cos(phiPitchDirection[int(projection::DIR_V)]))/sin(phiPitchDirection[int(projection::DIR_V)]);
  double bY_fromW = (segmentW.getBiasAtT0().Y() - bX_fromU*cos(phiPitchDirection[int(projection::DIR_W)]))/sin(phiPitchDirection[int(projection::DIR_W)]);  
  double bY = (bY_fromV*nHits_V + bY_fromW*nHits_W)/(nHits_V+nHits_W);
  double bX = bX_fromU;
  TVector3 aBias(bX, bY, bZ);

  double tZ_fromU = segmentU.getTangentWithT1().X();
  double tZ_fromV = segmentV.getTangentWithT1().X();
  double tZ_fromW = segmentW.getTangentWithT1().X();
  double tZ = (tZ_fromU*nHits_U + tZ_fromV*nHits_V + tZ_fromW*nHits_W)/(nHits_U+nHits_V+nHits_W);

  double tX_fromU = segmentU.getTangentWithT1().Y()*cos(phiPitchDirection[int(projection::DIR_U)]);
  double tY_fromV = (segmentV.getTangentWithT1().Y() - tX_fromU*cos(phiPitchDirection[int(projection::DIR_V)]))/sin(phiPitchDirection[int(projection::DIR_V)]);
  double tY_fromW = (segmentW.getTangentWithT1().Y() - tX_fromU*cos(phiPitchDirection[int(projection::DIR_W)]))/sin(phiPitchDirection[int(projection::DIR_W)]);
  double tY = (tY_fromV*nHits_V + tY_fromW*nHits_W)/(nHits_V+nHits_W);
  double tX = tX_fromU;
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
  aTrackCandidate = fitTrackNodes(aTrackCandidate);
  
  TGraph aGraph = aTrackCandidate.getHitDistanceProfile();
  double maxValue = 0.0;
  double bestSplit = 0.5;
  int nDivisions = 200;
  for(int iDivision=1;iDivision<nDivisions;++iDivision){
    double val = aGraph.Eval((double)iDivision/nDivisions*aTrackCandidate.getLength());
    if(val>maxValue){
      maxValue = val;
      bestSplit = (double)iDivision/nDivisions;
    }
  }
  std::cout<<"bestSplit: "<<bestSplit<<std::endl;
  aTrackCandidate.splitWorseChi2Segment(bestSplit);
  
  for(int iSplit = 0;iSplit<0;++iSplit){ //FIX ME
    unsigned int nSegments = aTrackCandidate.getSegments().size();
    for(unsigned int iSegment=0;iSegment<nSegments;iSegment+=2){
      aTrackCandidate.splitSegment(iSegment, 0.5);
      nSegments = aTrackCandidate.getSegments().size();
    }
  }
  
  
  aTrackCandidate = fitTrackNodes(aTrackCandidate);
  return aTrackCandidate;//TEST
  /*
  if(aTrackCandidate.getLength()<1.0) return aTrackCandidate;//FIX me move threshold to configuration

  bestSplit = fitTrackSplitPoint(aTrackCandidate);
  aTrackCandidate.splitWorseChi2Segment(bestSplit);
  aTrackCandidate = fitTrackNodes(aTrackCandidate);
  
  bestSplit = fitTrackSplitPoint(aTrackCandidate);
  aTrackCandidate.splitWorseChi2Segment(bestSplit);
  aTrackCandidate = fitTrackNodes(aTrackCandidate);
  
  return aTrackCandidate;
  */
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Track3D TrackBuilder::fitTrackNodes(const Track3D & aTrack) const{

  Track3D aTrackCandidate = aTrack;
  std::vector<double> bestParams = aTrackCandidate.getSegmentsStartEndXYZ();
  std::vector<double> params = aTrackCandidate.getSegmentsStartEndXYZ();
  int nParams = params.size();

  ROOT::Math::Functor fcn(&aTrackCandidate, &Track3D::chi2FromNodesList, nParams);
  fitter.SetFCN(fcn, params.data());

  for (int iPar = 0; iPar < nParams; ++iPar){
    fitter.Config().ParSettings(iPar).SetStepSize(0.1);
    fitter.Config().ParSettings(iPar).SetLimits(-100, 100);
  }

  double minChi2 = 1E10;
  for(unsigned int iStep=0;iStep<1;++iStep){ //FIX ME
    
    std::cout<<__FUNCTION__<<" iStep: "<<iStep<<std::endl;

    aTrackCandidate.extendToWholeChamber();
    aTrackCandidate.shrinkToHits();

    params = aTrackCandidate.getSegmentsStartEndXYZ();
  
    ////
    std::cout<<"Pre-fit: "<<std::endl; 
    std::cout<<aTrackCandidate<<std::endl;
    //return aTrackCandidate;
    //continue;
    ////
    
    bool fitStatus = fitter.FitFCN();
    if (!fitStatus) {
      Error(__FUNCTION__, "Track3D Fit failed");
      fitter.Result().Print(std::cout);
      return aTrack;
    }
    const ROOT::Fit::FitResult & result = fitter.Result();
    aTrackCandidate.chi2FromNodesList(result.GetParams());
    aTrackCandidate.removeEmptySegments();
    aTrackCandidate.extendToWholeChamber();
    aTrackCandidate.shrinkToHits();

    std::cout<<"Post-fit: "<<std::endl;
    aTrackCandidate.removeEmptySegments();
    std::cout<<aTrackCandidate<<std::endl;
    
    if(aTrackCandidate.getChi2()<minChi2){
      minChi2 =  aTrackCandidate.getChi2();
      bestParams = aTrackCandidate.getSegmentsStartEndXYZ();
    }
  }
  aTrackCandidate.chi2FromNodesList(bestParams.data());
  return aTrackCandidate;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackBuilder::fitTrackSplitPoint(const Track3D& aTrack) const{

  Track3D aTrackCandidate = aTrack;
  double currentBestChi2 = aTrackCandidate.getChi2();
  aTrackCandidate.splitWorseChi2Segment(0.5);

  std::vector<double> params0 = aTrackCandidate.getSegmentsStartEndXYZ();
  std::vector<double> params = params0;
  int nParams = params.size();
  ROOT::Math::Functor fcn(&aTrackCandidate, &Track3D::chi2FromNodesList, nParams);

  double bestSplit = -1.0;
  int nSplitSteps = 20;
  double splitStep = 1.0/nSplitSteps;

  for(int iSplitStep=1; iSplitStep<nSplitSteps;++iSplitStep){
    aTrackCandidate.chi2FromNodesList(params0.data());
    //aTrackCandidate.splitWorseChi2Segment(splitStep*iSplitStep);
    params = aTrackCandidate.getSegmentsStartEndXYZ();
    ////
    std::cout<<"Split "<<splitStep*iSplitStep<<" Pre-fit: "<<std::endl; 
    std::cout<<aTrackCandidate<<std::endl;
    ////
    fitter.SetFCN(fcn, params.data());

    for (int iPar = 0; iPar < nParams; ++iPar){
      fitter.Config().ParSettings(iPar).SetStepSize(0.1);
      fitter.Config().ParSettings(iPar).SetLimits(-100, 100);
    }
    bool fitStatus = fitter.FitFCN();
    if (!fitStatus) {
      Error(__FUNCTION__, "Track3D Fit failed");
      fitter.Result().Print(std::cout);
      return -1.0;
    }    
    const ROOT::Fit::FitResult & result = fitter.Result();
    aTrackCandidate.chi2FromNodesList(result.GetParams());
    aTrackCandidate.removeEmptySegments();
    aTrackCandidate.extendToWholeChamber();
    aTrackCandidate.shrinkToHits();
    
    if(aTrackCandidate.getChi2()<currentBestChi2){
      currentBestChi2 = aTrackCandidate.getChi2();
      bestSplit = splitStep*iSplitStep;
      std::cout<<"Better split: "<<bestSplit<<" chi2: "<<currentBestChi2<<std::endl;
      std::cout<<aTrackCandidate<<std::endl;
      std::cout<<"-----"<<std::endl;
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
  perpPlaneBaseUnitA.SetMagThetaPhi(1.0, pi/2.0 + tangentTheta, tangentPhi);
  perpPlaneBaseUnitB.SetMagThetaPhi(1.0, pi/2.0, pi/2.0 + tangentPhi);
  
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
  fitter.Config().ParSettings(0).SetLimits(0, pi);
  fitter.Config().ParSettings(1).SetLimits(-pi, pi);
  fitter.Config().ParSettings(2).SetLimits(-100, 100);
  fitter.Config().ParSettings(3).SetLimits(-100, 100);
  */
