#include <cstdlib>
#include <iostream>

#include "TVector3.h"
#include "TProfile.h"
#include "TObjArray.h"
#include "TF1.h"
#include "TTree.h"
#include "TFile.h"
#include "TFitResult.h"
#include "Math/Functor.h"

#include "GeometryTPC.h"
#include "EventTPC.h"
#include "SigClusterTPC.h"

#include "TrackBuilder.h"
#include "colorText.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackBuilder::TrackBuilder() {

  myEvent = 0;

  nAccumulatorRhoBins = 50;//FIX ME move to configuarable
  nAccumulatorPhiBins = 2.0*M_PI/0.1;//FIX ME move to configuarable

  myHistoInitialized = false;
  myAccumulators.resize(3);
  my2DSeeds.resize(3);
  myRecHits.resize(3);

  fitter.Config().MinimizerOptions().SetMinimizerType("GSLSimAn");
  //fitter.Config().MinimizerOptions().SetMinimizerType("Minuit2");
  fitter.Config().MinimizerOptions().SetMaxFunctionCalls(1E4);
  fitter.Config().MinimizerOptions().SetMaxIterations(1E4);
  fitter.Config().MinimizerOptions().SetTolerance(1E-3);
  fitter.Config().MinimizerOptions().Print(std::cout);
  fitter.Config().MinimizerOptions().SetPrintLevel(0);

  ///An offset used for filling the Hough transformation.
  ///to avoid having very small rho parameters, as
  ///orignally many tracks traverse close to X=0, Time=0
  ///point.
  aHoughOffest.SetX(50.0);
  aHoughOffest.SetY(50.0);
  aHoughOffest.SetZ(0.0);

  myFittedTrackPtr = &myFittedTrack;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackBuilder::~TrackBuilder() {
  closeOutputStream();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::openOutputStream(const std::string & fileName){

  if(!myFittedTrackPtr){
    std::cout<<KRED<<__FUNCTION__<<RST
	     <<" pointer to fitted track not set!"
	     <<std::endl;
    return;
  }
  std::string treeName = "TPCRecoData";
  myOutputFilePtr = std::make_shared<TFile>(fileName.c_str(),"RECREATE");
  myOutputTreePtr = std::make_shared<TTree>(treeName.c_str(),"");
  myOutputTreePtr->Branch("RecoEvent", "Track3D", &myFittedTrackPtr);
  //myOutputTreePtr->Branch("DetEvent", "EventTPC", &myEvent);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::closeOutputStream(){

  if(!myOutputFilePtr){
     std::cout<<KRED<<__FUNCTION__<<RST
	     <<" pointer to output file not set!"
	     <<std::endl;
     return;
  }
  myOutputFilePtr->Close();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::fillOutputStream(){

  if(!myOutputTreePtr){
     std::cout<<KRED<<__FUNCTION__<<RST
	     <<" pointer to output tree not set!"
	     <<std::endl;
     return;
  }
  myOutputTreePtr->Fill();
  myOutputTreePtr->Write();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr){
  
  myGeometryPtr = aGeometryPtr;
  phiPitchDirection.resize(3);
  phiPitchDirection[DIR_U] = myGeometryPtr->GetStripPitchVector(DIR_U).Phi();
  phiPitchDirection[DIR_V] = myGeometryPtr->GetStripPitchVector(DIR_V).Phi();
  phiPitchDirection[DIR_W] = myGeometryPtr->GetStripPitchVector(DIR_W).Phi();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::setEvent(std::shared_ptr<EventTPC> aEvent){
  setEvent(aEvent.get());
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::setEvent(EventTPC* aEvent){

  myEvent = aEvent;

  double chargeThreshold = 35;//0.15*myEvent->GetMaxCharge();
  int delta_timecells = 25;
  int delta_strips = 3;
  
  myCluster = myEvent->GetOneCluster(chargeThreshold, delta_strips, delta_timecells);

  std::string hName, hTitle;
  if(!myHistoInitialized){     
    for(int iDir = 0; iDir<3;++iDir){
      std::shared_ptr<TH2D> hRawHits = myEvent->GetStripVsTimeInMM(/*myCluster*/ getCluster(), iDir);    
      double minX = hRawHits->GetXaxis()->GetXmin();
      double minY = hRawHits->GetYaxis()->GetXmin();
      double maxX = hRawHits->GetXaxis()->GetXmax();
      double maxY = hRawHits->GetYaxis()->GetXmax();
      double rho1 = sqrt( maxX*maxX + maxY*maxY);
      double rho2 = sqrt( minX*minX + maxY*maxY);
      double rho3 = sqrt( maxX*maxX + minY*minY);
      double rho4 = sqrt( minX*minX + minY*minY);
      double rhoMAX=rho1;
      double rhoMIN=rho1;
      if(rho1>rhoMAX) rhoMAX=rho1;
      if(rho2>rhoMAX) rhoMAX=rho2;
      if(rho3>rhoMAX) rhoMAX=rho3;
      if(rho4>rhoMAX) rhoMAX=rho4;
      if(rho1<rhoMIN) rhoMIN=rho1;
      if(rho2<rhoMIN) rhoMIN=rho2;
      if(rho3>rhoMIN) rhoMIN=rho3;
      if(rho4>rhoMIN) rhoMIN=rho4;
      // for rhoMIN: check if (0,0) is inside the rectangle [minX, maxX] x [minY, maxY]
      //  1 | 2 | 1
      // ---+---+---                   +-----+
      //  3 | 4 | 5                    |     |
      // ---+---+---       (0,0)+      +-----+
      //  1 | 6 | 1       
      if(minX<=0.0 && maxX>=0.0 && minY<=0.0 && maxY>=0.0) {
	rhoMIN=0.0; // case 4
      } else if(minX<0.0 && maxX<0.0 && minY<=0.0 && maxY>=0.0) {
	rhoMIN=fabs(maxX); // case 5
      } else if(minX>0.0 && maxX>0.0 && minY<=0.0 && maxY>=0.0) {
	rhoMIN=fabs(minX); // case 3
      } else if(minX<=0.0 && maxX>=0.0 && minY<0.0 && maxY<0.0) {
	rhoMIN=fabs(maxY); // case 6
      } else if(minX<=0.0 && maxX>=0.0 && minY>0.0 && maxY>0.0) {
	rhoMIN=fabs(minY); // case 2
      }     
      hName = "hAccumulator_"+std::to_string(iDir);
      hTitle = "Hough accumulator for direction: "+std::to_string(iDir)+";#theta;#rho";
      TH2D hAccumulator(hName.c_str(), hTitle.c_str(), nAccumulatorPhiBins,
			-M_PI, M_PI, nAccumulatorRhoBins, rhoMIN, rhoMAX);
      //////// DEBUG
      std::cout << ": rhoMIN/rhoMAX[mm]=" << rhoMIN << "/" << rhoMAX << std::endl;
      //////// DEBUG
      // modified by MC - 8 Aug 2021

      myAccumulators[iDir] = hAccumulator;
      myRecHits[iDir] = *hRawHits;
    }
    myHistoInitialized = true;
  } 
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::reconstruct(){
  
  for(int iDir=DIR_U;iDir<=DIR_W;++iDir){
    makeRecHits(iDir);
    fillHoughAccumulator(iDir);
    my2DSeeds[iDir] = findSegment2DCollection(iDir);    
  }
  myTrack3DSeed = buildSegment3D();
  *(&myFittedTrack) = fitTrack3D(myTrack3DSeed);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::makeRecHits(int iDir){ 

  std::shared_ptr<TH2D> hRawHits = myEvent->GetStripVsTimeInMM(getCluster(), iDir);
  TH2D & hRecHits = myRecHits[iDir];
  hRecHits.Reset();
  std::string tmpTitle(hRawHits->GetTitle());
  if(tmpTitle.find("from")!=std::string::npos){
    std::string eventNumber = tmpTitle.substr(0,tmpTitle.find(":"));
    tmpTitle.replace(0,tmpTitle.find("from"),"");
    tmpTitle = eventNumber+": Reco hits "+tmpTitle;
    hRecHits.SetTitle(tmpTitle.c_str());
  }
  hRecHits.Add(hRawHits.get());//TEST
  return;//TEST

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
      hitCharge *= sqrt(2.0)*M_PI*hitTimePosError;//the gausian fits are made without the normalisation factor
      hRecHits.Fill(hitTimePos, hitWirePos, hitCharge);
    }      
    delete hProj;
  }
  double maxCharge = hRecHits.GetMaximum();
  double threshold = 0.1*maxCharge;//FIX ME optimize threshold

  for(int iBin=0;iBin<hRecHits.GetNcells();++iBin){
    if(hRecHits.GetBinContent(iBin)<threshold){
      hRecHits.SetBinContent(iBin, 0.0);
    }
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TF1 TrackBuilder::fitTimeWindow(TH1D* hProj){

   TFitResultPtr fitResult;
   TF1 timeResponseShape;
   TF1 bestTimeResponseShape;
   double bestChi2OverNDF = 1E10;

   if(!hProj){
     std::cerr<<__FUNCTION__<<" NULL hProj"<<std::endl; 
     return bestTimeResponseShape;
   }
   if(!myGeometryPtr){
     std::cerr<<__FUNCTION__<<" NULL myGeometryPtr"<<std::endl; 
     return bestTimeResponseShape;
   }

   double sampling_rate = myGeometryPtr->GetSamplingRate();
   double vdrift = myGeometryPtr->GetDriftVelocity()*10.0;// 10.0: [cm] -> [mm]
   double timeBinToCartesianScale = 1.0/sampling_rate*vdrift;

   int maxBin = hProj->GetMaximumBin();
   double maxValue = hProj->GetMaximum();
   double maxPos = hProj->GetBinCenter(maxBin);
   double windowIntegral = hProj->Integral(maxBin-25*timeBinToCartesianScale, maxBin+25*timeBinToCartesianScale);
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
     timeResponseShape.SetRange(maxPos-25*timeBinToCartesianScale, maxPos+25*timeBinToCartesianScale);   
     for(int iSet=0;iSet<timeResponseShape.GetNpar();iSet+=3){
       timeResponseShape.SetParameter(iSet, maxValue*2);
       timeResponseShape.SetParameter(iSet+1, maxPos);
       timeResponseShape.SetParameter(iSet+2, 2.0*timeBinToCartesianScale);
       ///     
       timeResponseShape.SetParLimits(iSet, 0.1*maxValue, maxValue*2);
       timeResponseShape.SetParLimits(iSet+1, maxPos-15*timeBinToCartesianScale, maxPos+15*timeBinToCartesianScale);   
       timeResponseShape.SetParLimits(iSet+2, 0.5*timeBinToCartesianScale, 8*timeBinToCartesianScale);
     }   
     fitResult = hProj->Fit(&timeResponseShape, "QRBSWN");   

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
  double maxCharge = hRecHits.GetMaximum();
  double theta = 0.0, rho = 0.0;
  double x = 0.0, y=0.0;
  int charge = 0;
  for(int iBinX=1;iBinX<hRecHits.GetNbinsX();++iBinX){
    for(int iBinY=1;iBinY<hRecHits.GetNbinsY();++iBinY){
      x = hRecHits.GetXaxis()->GetBinCenter(iBinX) + aHoughOffest.X();
      y = hRecHits.GetYaxis()->GetBinCenter(iBinY) + aHoughOffest.Y();
      charge = hRecHits.GetBinContent(iBinX, iBinY);
      if(charge<0.2*maxCharge) continue;
      for(int iBinTheta=1;iBinTheta<myAccumulators[iDir].GetNbinsX();++iBinTheta){
	theta = myAccumulators[iDir].GetXaxis()->GetBinCenter(iBinTheta);
	rho = x*cos(theta) + y*sin(theta);
	charge = 1.0; //FIXME: study how to include the charge. 
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
  
  TrackSegment2D aSegment2D(iDir);
  aSegment2D.setBiasTangent(aBias, aTangent);
  aSegment2D.setNAccumulatorHits(nHits);
  return aSegment2D;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::getSegment2DCollectionFromGUI(const std::vector<double> & segmentsXY){
  
  if(segmentsXY.size()%3 || (segmentsXY.size()/3)%4){
    std::cout<<KRED<<__FUNCTION__<<RST<<"Wrong number on segment endpoints: "
	     <<segmentsXY.size()<<std::endl;      
  }
  std::for_each(my2DSeeds.begin(), my2DSeeds.end(),
		[](TrackSegment2DCollection &item){item.resize(0);});

  Track3D aTrackCandidate;
  
  TVector3 aStart, aEnd;
  double x=0.0, y=0.0;
  int nSegments = segmentsXY.size()/3/4;
  for(int iSegment=0;iSegment<nSegments;++iSegment){
    for(int iDir = DIR_U; iDir<=DIR_W;++iDir){
      TrackSegment2D aSegment2D(iDir);
      x = segmentsXY.at(iDir*4 + iSegment*12);
      y = segmentsXY.at(iDir*4 + iSegment*12 + 1);
      aStart.SetXYZ(x,y,0.0);
      x = segmentsXY.at(iDir*4 + iSegment*12 + 2);
      y = segmentsXY.at(iDir*4 + iSegment*12 + 3);
      aEnd.SetXYZ(x,y,0.0);
      aSegment2D.setStartEnd(aStart, aEnd);
      aSegment2D.setNAccumulatorHits(1);
      my2DSeeds[iDir].push_back(aSegment2D);
    }
    TrackSegment3D a3DSeed = buildSegment3D(iSegment);
    double startTime = my2DSeeds.at(DIR_U).at(iSegment).getStart().X();    
    double endTime = my2DSeeds.at(DIR_U).at(iSegment).getEnd().X();
    double lambdaStartTime = a3DSeed.getLambdaAtZ(startTime);
    double lambdaEndTime = a3DSeed.getLambdaAtZ(endTime);
    TVector3 start =  a3DSeed.getStart() + lambdaStartTime*a3DSeed.getTangent();
    TVector3 end =  a3DSeed.getStart() + lambdaEndTime*a3DSeed.getTangent();
    a3DSeed.setStartEnd(start, end);
    aTrackCandidate.addSegment(a3DSeed);
  }

  myFittedTrack = aTrackCandidate;
  myFittedTrackPtr = &myFittedTrack;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackSegment3D TrackBuilder::buildSegment3D(int iTrack2DSeed) const{
	     
  const TrackSegment2D & segmentU = my2DSeeds[DIR_U][iTrack2DSeed];
  const TrackSegment2D & segmentV = my2DSeeds[DIR_V][iTrack2DSeed];
  const TrackSegment2D & segmentW = my2DSeeds[DIR_W][iTrack2DSeed];

  int nHits_U = segmentU.getNAccumulatorHits();
  int nHits_V = segmentV.getNAccumulatorHits();
  int nHits_W = segmentW.getNAccumulatorHits();

  TVector2 biasXY_fromUV;
  bool res1=myGeometryPtr->GetUVWCrossPointInMM(segmentU.getStripDir(), segmentU.getBiasAtT0().Y(),
						segmentV.getStripDir(), segmentV.getBiasAtT0().Y(),
						biasXY_fromUV);
  TVector2 biasXY_fromVW;
  bool res2=myGeometryPtr->GetUVWCrossPointInMM(segmentV.getStripDir(), segmentV.getBiasAtT0().Y(),
						segmentW.getStripDir(), segmentW.getBiasAtT0().Y(),
						biasXY_fromVW);
  TVector2 biasXY_fromWU;
  bool res3=myGeometryPtr->GetUVWCrossPointInMM(segmentW.getStripDir(), segmentW.getBiasAtT0().Y(),
						segmentU.getStripDir(), segmentU.getBiasAtT0().Y(),
						biasXY_fromWU);
  assert(res1|res2|res3);
  
  const TVector2 biasXY=( (int)res1*biasXY_fromUV + (int)res2*biasXY_fromVW + (int)res3*biasXY_fromWU)/( (int)res1 + (int)res2 + (int)res3 );
  double biasZ_fromU = segmentU.getBiasAtT0().X();
  double biasZ_fromV = segmentV.getBiasAtT0().X();
  double biasZ_fromW = segmentW.getBiasAtT0().X();
  double biasZ = (biasZ_fromU*nHits_U + biasZ_fromV*nHits_V + biasZ_fromW*nHits_W)/(nHits_U+nHits_V+nHits_W);
  TVector3 aBias( biasXY.X(), biasXY.Y(), biasZ);

  
  TVector2 tangentXY_fromUV;
  bool res4=myGeometryPtr->GetUVWCrossPointInMM(segmentU.getStripDir(), segmentU.getTangentWithT1().Y(),
						segmentV.getStripDir(), segmentV.getTangentWithT1().Y(),
						tangentXY_fromUV);
  TVector2 tangentXY_fromVW;
  bool res5=myGeometryPtr->GetUVWCrossPointInMM(segmentV.getStripDir(), segmentV.getTangentWithT1().Y(),
						segmentW.getStripDir(), segmentW.getTangentWithT1().Y(),
						tangentXY_fromVW);
  TVector2 tangentXY_fromWU;
  bool res6=myGeometryPtr->GetUVWCrossPointInMM(segmentW.getStripDir(), segmentW.getTangentWithT1().Y(),
						segmentU.getStripDir(), segmentU.getTangentWithT1().Y(),
						tangentXY_fromWU);
  assert(res4|res5|res6);

  const TVector2 tangentXY=( (int)res4*tangentXY_fromUV + (int)res5*tangentXY_fromVW + (int)res6*tangentXY_fromWU)/( (int)res4 + (int)res5 + (int)res6 );
  double tangentZ_fromU = segmentU.getTangentWithT1().X();
  double tangentZ_fromV = segmentV.getTangentWithT1().X();
  double tangentZ_fromW = segmentW.getTangentWithT1().X();
  double tangentZ = (tangentZ_fromU*nHits_U + tangentZ_fromV*nHits_V + tangentZ_fromW*nHits_W)/(nHits_U+nHits_V+nHits_W);
  TVector3 aTangent( tangentXY.X(), tangentXY.Y(), tangentZ);

  TrackSegment3D a3DSeed;
  a3DSeed.setGeometry(myGeometryPtr);
  a3DSeed.setBiasTangent(aBias, aTangent);
  a3DSeed.setRecHits(myRecHits);
  return a3DSeed;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Track3D TrackBuilder::fitTrack3D(const TrackSegment3D & aTrackSegment) const{

  Track3D aTrackCandidate;
  aTrackCandidate.addSegment(aTrackSegment);
  aTrackCandidate.extendToZMinMax(myGeometryPtr->GetDriftCageZmin(),  myGeometryPtr->GetDriftCageZmax());
  aTrackCandidate = fitTrackNodes(aTrackCandidate);
  aTrackCandidate.shrinkToHits();  
  return aTrackCandidate;//TEST
  
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
  
  std::cout<<"after split: "<<std::endl;
  std::cout<<aTrackCandidate<<std::endl;

  aTrackCandidate = fitTrackNodes(aTrackCandidate);
  return aTrackCandidate;
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

  double minChi2 = 1E10;
  for(unsigned int iStep=1;iStep<2;++iStep){
    
    std::cout<<__FUNCTION__<<" iStep: "<<iStep<<std::endl;   
    params = aTrackCandidate.getSegmentsStartEndXYZ();
    nParams = params.size();
    for (int iPar = 0; iPar < nParams; ++iPar){
      fitter.Config().ParSettings(iPar).SetValue(params[iPar]);
      fitter.Config().ParSettings(iPar).SetStepSize(1.0/(2*iStep));
      fitter.Config().ParSettings(iPar).SetLimits(params[iPar]-20.0/iStep, params[iPar]+20.0/iStep);
    }  
    std::cout<<"Pre-fit: "<<std::endl; 
    std::cout<<aTrackCandidate<<std::endl;
    
    bool fitStatus = fitter.FitFCN();
    if (!fitStatus) {
      Error(__FUNCTION__, "Track3D Fit failed");
      fitter.Result().Print(std::cout);
      return aTrack;
    }
    const ROOT::Fit::FitResult & result = fitter.Result();
    aTrackCandidate.chi2FromNodesList(result.GetParams());

    std::cout<<"Post-fit: "<<std::endl;
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
    aTrackCandidate.extendToZMinMax(myGeometryPtr->GetDriftCageZmin(),  myGeometryPtr->GetDriftCageZmax());
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
