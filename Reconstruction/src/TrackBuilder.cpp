#include <cstdlib>
#include <iostream>
#include <algorithm>

#include <TVector3.h>
#include <TProfile.h>
#include <TObjArray.h>
#include <TF1.h>
#include <TTree.h>
#include <TFile.h>
#include <TFitResult.h>
#include <Math/Functor.h>
#include <Math/Polar2D.h>
#include <Math/DisplacementVector2D.h>
#include <Math/VectorUtil.h>
#include <Minuit2/Minuit2Minimizer.h>

#include "TPCReco/GeometryTPC.h"

#include "TPCReco/TrackBuilder.h"
#include "TPCReco/colorText.h"

#ifndef M_PI
#define M_PI std::atan(1.0)*4;
#endif // M_PI
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackBuilder::TrackBuilder() {

  //  nAccumulatorRhoBins = 50;//FIX ME move to configuarable
  //  nAccumulatorPhiBins = 2.0*M_PI/0.025;//FIX ME move to configuarable
  //  nAccumulatorPhiBins = 2.0*M_PI/0.1;//FIX ME move to configuarable
  nAccumulatorRhoBins = 200; //FIX ME large enough to keep RHO bin size below 0.5*STRIP_PITCH
  nAccumulatorPhiBins = 400; //FIX ME move to configuarable

  myHistoInitialized = false;
  myAccumulators.resize(3);
  my2DSeeds.resize(3);
  myRecHits.resize(3);
  myRawHits.resize(3);

  fitter.Config().MinimizerOptions().SetMinimizerType("Minuit2");
  fitter.Config().MinimizerOptions().SetMaxFunctionCalls(2000);
  fitter.Config().MinimizerOptions().Print(std::cout);
  fitter.Config().MinimizerOptions().SetPrintLevel(0);
  //fitter.Config().MinimizerOptions().SetMinimizerAlgorithm("Scan");
  
  ///An offset used for filling the Hough transformation.
  ///to avoid having very small rho parameters, as
  ///originally many tracks traverse close to STRIP_POS[mm]=0, TIME_POS[mm]=0
  ///point.
  // aHoughOffest.SetX(50.0);
  // aHoughOffest.SetY(50.0);
  // aHoughOffest.SetZ(0.0);
  myHoughOffset.resize(3);
  for(auto &offset: myHoughOffset) offset.SetXYZ(0,0,0);

  setPressure(myPressure);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackBuilder::~TrackBuilder() { }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr){
  
  myGeometryPtr = aGeometryPtr;
  myRecHitBuilder.setGeometry(aGeometryPtr);
  
  phiPitchDirection.resize(3);
  phiPitchDirection[definitions::projection_type::DIR_U] = myGeometryPtr->GetStripPitchVector(definitions::projection_type::DIR_U).Phi();
  phiPitchDirection[definitions::projection_type::DIR_V] = myGeometryPtr->GetStripPitchVector(definitions::projection_type::DIR_V).Phi();
  phiPitchDirection[definitions::projection_type::DIR_W] = myGeometryPtr->GetStripPitchVector(definitions::projection_type::DIR_W).Phi();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::setPressure(double aPressure) {

  myPressure = aPressure;
  mydEdxFitter.setPressure(myPressure);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::setEvent(std::shared_ptr<EventTPC> aEvent){

  myEventPtr = aEvent;
  myFittedTrack = Track3D();
  
  std::string hName, hTitle;
  if(!myHistoInitialized){
    for(int iDir=definitions::projection_type::DIR_U;iDir<=definitions::projection_type::DIR_W;++iDir){
      std::shared_ptr<TH2D> hRawHits = myEventPtr->get2DProjection(get2DProjectionType(iDir),
							   filter_type::none,
//								   filter_type::threshold,
//							           filter_type::fraction,
								   scale_type::mm);
      ///// DEBUG
      //
      myHoughOffset[iDir].SetX(0.0);
      myHoughOffset[iDir].SetY(0.0);
      double rhoMIN=0.0;
      double rhoMAX=0.0;
      for(auto iter=0; iter<2; iter++) {
      // double minX = hRawHits->GetXaxis()->GetXmin();
      // double minY = hRawHits->GetYaxis()->GetXmin();
      // double maxX = hRawHits->GetXaxis()->GetXmax();
      // double maxY = hRawHits->GetYaxis()->GetXmax();
      double minX = hRawHits->GetXaxis()->GetXmin() + myHoughOffset[iDir].X();
      double minY = hRawHits->GetYaxis()->GetXmin() + myHoughOffset[iDir].Y();
      double maxX = hRawHits->GetXaxis()->GetXmax() + myHoughOffset[iDir].X();
      double maxY = hRawHits->GetYaxis()->GetXmax() + myHoughOffset[iDir].Y();
      double rho1 = sqrt( maxX*maxX + maxY*maxY);
      double rho2 = sqrt( minX*minX + maxY*maxY);
      double rho3 = sqrt( maxX*maxX + minY*minY);
      double rho4 = sqrt( minX*minX + minY*minY);
      // double rhoMAX=rho1;
      // double rhoMIN=rho1;
      // std::cout<<__FUNCTION__<<": BEFORE dir="<<iDir<<" iter="<<iter<<" rho1="<<rho1<<" rho2="<<rho2<<" rho3="<<rho3<<" rho4="<<rho4<<std::endl;
      rhoMAX=0.0;
      if(rho1>rhoMAX) rhoMAX=rho1;
      if(rho2>rhoMAX) rhoMAX=rho2;
      if(rho3>rhoMAX) rhoMAX=rho3;
      if(rho4>rhoMAX) rhoMAX=rho4;
      rhoMIN=rhoMAX;
      if(rho1<rhoMIN) rhoMIN=rho1;
      if(rho2<rhoMIN) rhoMIN=rho2;
      if(rho3<rhoMIN) rhoMIN=rho3;
      if(rho4<rhoMIN) rhoMIN=rho4;
      // std::cout<<__FUNCTION__<<": AFTER dir="<<iDir<<" iter="<<iter<<" rhoMIN="<<rhoMIN<<" rhoMAX="<<rhoMAX<<std::endl;
      // for rhoMIN: check if (0,0) is inside the rectangle [minX, maxX] x [minY, maxY]
      //  1 | 2 | 1
      // ---+---+---                   +-----+
      //  3 | 4 | 5                    |     |
      // ---+---+---       (0,0)+      +-----+
      //  1 | 6 | 1
      int rho_case=1;
      if(minX<=0.0 && maxX>=0.0 && minY<=0.0 && maxY>=0.0) {
	rhoMIN=0.0; // case 4
	rho_case=4;
      } else if(minX<0.0 && maxX<0.0 && minY<=0.0 && maxY>=0.0) {
	rhoMIN=std::min(std::min(fabs(minY), fabs(maxY)), fabs(maxX)); // case 5
	rho_case=5;
      } else if(minX>0.0 && maxX>0.0 && minY<=0.0 && maxY>=0.0) {
	rhoMIN=std::min(std::min(fabs(minY), fabs(maxY)), fabs(minX)); // case 3
	rho_case=3;
      } else if(minX<=0.0 && maxX>=0.0 && minY<0.0 && maxY<0.0) {
	rhoMIN=std::min(std::min(fabs(minX), fabs(maxX)), fabs(maxY)); // case 2
	rho_case=6;
      } else if(minX<=0.0 && maxX>=0.0 && minY>0.0 && maxY>0.0) {
	rhoMIN=std::min(std::min(fabs(minX), fabs(maxX)), fabs(minY)); // case 6
	rho_case=2;
      }

      const double limit = 0.1*rhoMAX;
      if(iter==0 && rhoMIN<limit) { // adjust offset to avoid rho=0 region
	switch(rho_case) {
	case 1:
	  if(minX<=0.0)      myHoughOffset[iDir].SetX(-limit);
	  else if(maxX>=0.0) myHoughOffset[iDir].SetX(limit);
	  if(minY<=0.0)      myHoughOffset[iDir].SetY(-limit);
	  else if(maxY>=0.0) myHoughOffset[iDir].SetY(limit);
	  break;
	case 2:
	  myHoughOffset[iDir].SetY(limit);
	  break;
	case 3:
	  myHoughOffset[iDir].SetX(-limit);
	  break;
	case 4:
	  myHoughOffset[iDir].SetX(-limit-fabs(minX));
	  myHoughOffset[iDir].SetY(-limit-fabs(minY));
	  break;
	case 5:
	  myHoughOffset[iDir].SetX(limit);
	  break;
	case 6:
	  myHoughOffset[iDir].SetY(-limit);
	  break;
	default:;
	};
      }
      //std::cout<< __FUNCTION__<<": dir="<<iDir<<" iter="<<iter<<" rho_case="<<rho_case<<" rhoMIN="<<rhoMIN<<" rhoMAX="<<rhoMAX<<std::endl;
      }
      //
      ///// DEBUG

      hName = "hAccumulator_"+std::to_string(iDir);
      hTitle = "Hough accumulator for direction: "+std::to_string(iDir)+";#theta [rad];#rho [mm]";
      const auto phiBinWidth=2*M_PI/nAccumulatorPhiBins;
      TH2D hAccumulator(hName.c_str(), hTitle.c_str(), nAccumulatorPhiBins,
			-M_PI-0.5*phiBinWidth, M_PI+0.5*phiBinWidth,
			nAccumulatorRhoBins, rhoMIN, rhoMAX);
      myAccumulators[iDir] = hAccumulator;
      myRawHits[iDir] = *hRawHits;
      if(iDir==definitions::projection_type::DIR_U) hTimeProjection = *hRawHits->ProjectionX();
    }
    myHistoInitialized = true;
  } 
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::reconstruct(){

  hTimeProjection.Reset();  
  for(int iDir=definitions::projection_type::DIR_U;iDir<=definitions::projection_type::DIR_W;++iDir){
    makeRecHits(iDir);
    //fillHoughAccumulator(iDir);
    //my2DSeeds[iDir] = findSegment2DCollection(iDir);    
  }
  myZRange = getProjectionEdges(hTimeProjection);
  myTrack3DSeed = buildSegment3D();
  
  Track3D aTrackCandidate;
  aTrackCandidate.addSegment(myTrack3DSeed);
  aTrackCandidate = fitTrack3D(aTrackCandidate);
  if(aTrackCandidate.getLength()>minTkLenghtWithHypothesis) aTrackCandidate = fitEventHypothesis(aTrackCandidate);
  myFittedTrack = aTrackCandidate;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::makeRecHits(int iDir){

  std::shared_ptr<TH2D> hProj = myEventPtr->get2DProjection(get2DProjectionType(iDir),
							    filter_type::threshold,
//							    filter_type::fraction,
							    scale_type::mm);
  //myRecHits[iDir] = myRecHitBuilder.makeRecHits(*hProj);
  myRawHits[iDir] = myRecHitBuilder.makeCleanCluster(*hProj);
  myRecHits[iDir] = myRawHits[iDir];
  hTimeProjection.Add(myRecHits[iDir].ProjectionX("hTimeProjection"));
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::tuple<double, double> TrackBuilder::getProjectionEdges(const TH1D & hProj, int binMargin) const{
  
  int iBinStart = 0;
  int iBinEnd = hProj.GetNbinsX();
  double histoSum = hProj.Integral();
  double sum = 0.0;
  double threshold = 0.01;

  for(auto iBin=0;iBin<hProj.GetNbinsX();++iBin){
    sum += hProj.GetBinContent(iBin); 
    if(sum/histoSum>threshold && iBinStart==0) iBinStart = iBin;
    else if(sum/histoSum>1.0-threshold && iBinEnd==hProj.GetNbinsX()){ 
      iBinEnd = iBin;
      break;
    }
  }
  
  double start = hProj.GetXaxis()->GetBinCenter(iBinStart-binMargin);
  double end =  hProj.GetXaxis()->GetBinCenter(iBinEnd+binMargin);  
  return std::make_tuple(start, end);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TH2D & TrackBuilder::getCluster2D(int iDir) const{return myRawHits[iDir];}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TH2D & TrackBuilder::getRecHits2D(int iDir) const{return myRecHits[iDir];}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TH1D & TrackBuilder::getRecHitsTimeProjection() const{return hTimeProjection;}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TH2D & TrackBuilder::getHoughtTransform(int iDir) const{return myAccumulators[iDir];  }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TrackSegment2D & TrackBuilder::getSegment2D(int iDir, unsigned int iTrack) const{

  if(my2DSeeds[iDir].size()<iTrack) return dummySegment2D;  
  return my2DSeeds[iDir].at(iTrack);  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TrackSegment3D & TrackBuilder::getSegment3DSeed() const{return myTrack3DSeed; }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const Track3D & TrackBuilder::getTrack3D(unsigned int iSegment) const{return myFittedTrack;}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::fillHoughAccumulator(int iDir){

  myAccumulators[iDir].Reset();
  
  const TH2D & hRecHits  = getRecHits2D(iDir);
  double maxCharge = hRecHits.GetMaximum();
  double maxChargeFraction = 0.05;
  double theta = 0.0, rho = 0.0;
  double x = 0.0, y=0.0;
  int charge = 0;
  for(int iBinX=1;iBinX<hRecHits.GetNbinsX();++iBinX){
    for(int iBinY=1;iBinY<hRecHits.GetNbinsY();++iBinY){
      x = hRecHits.GetXaxis()->GetBinCenter(iBinX) + myHoughOffset[iDir].X();
      y = hRecHits.GetYaxis()->GetBinCenter(iBinY) + myHoughOffset[iDir].Y();
      charge = hRecHits.GetBinContent(iBinX, iBinY);
      if(charge<maxChargeFraction*maxCharge) continue;
      for(int iBinTheta=1;iBinTheta<myAccumulators[iDir].GetNbinsX();++iBinTheta){
        theta = myAccumulators[iDir].GetXaxis()->GetBinCenter(iBinTheta);
        rho = x*cos(theta) + y*sin(theta);
        myAccumulators[iDir].Fill(theta, rho, charge);
      }
    }
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackSegment2DCollection TrackBuilder::findSegment2DCollection(int iDir){

  TrackSegment2DCollection aTrackCollection;
  int nPeaks = 1;
  for(int iPeak=0;iPeak<nPeaks;++iPeak){
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

	///// DEBUG
	// take into account theta periodicity modulo 2*pi and overflow bins
	int binx=iBinX+iDeltaX;
	if(binx>myAccumulators[iDir].GetNbinsX()) binx=binx%myAccumulators[iDir].GetNbinsX();
	else if(binx<1) binx=myAccumulators[iDir].GetNbinsX()+binx%myAccumulators[iDir].GetNbinsX();
	int biny=iBinY+iDeltaY;
	if(biny>myAccumulators[iDir].GetNbinsY()) biny=biny%myAccumulators[iDir].GetNbinsY();
	else if(biny<1) biny=myAccumulators[iDir].GetNbinsY()+biny%myAccumulators[iDir].GetNbinsY();
	hAccumulator_Clone->SetBinContent(binx, biny, 0.0);
	//
	///// DEBUG
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
  aBias -= myHoughOffset[iDir].Dot(aBias.Unit())*aBias.Unit();
  
  aX = -rho*sin(theta);
  aY = rho*cos(theta);
  aTangent.SetXYZ(aX, aY, 0.0);

  TrackSegment2D aSegment2D(iDir, myGeometryPtr);
  aSegment2D.setBiasTangent(aBias, aTangent);
  aSegment2D.setNAccumulatorHits(nHits);
  return aSegment2D;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::getSegment2DCollectionFromGUI(const std::vector<double> & segmentsXY){

  ///FIXME code duplication
  hTimeProjection.Reset();  
  for(int iDir=definitions::projection_type::DIR_U;iDir<=definitions::projection_type::DIR_W;++iDir){
    makeRecHits(iDir);   
  }
  /////////////////
  
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
    for(int iDir = definitions::projection_type::DIR_U; iDir<=definitions::projection_type::DIR_W;++iDir){
      TrackSegment2D aSegment2D(iDir, myGeometryPtr);
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
    TrackSegment3D a3DSeed = buildSegment3D(iSegment, true);
    double startTime = my2DSeeds.at(definitions::projection_type::DIR_U).at(iSegment).getStart().X();    
    double endTime = my2DSeeds.at(definitions::projection_type::DIR_U).at(iSegment).getEnd().X();
    double lambdaStartTime = a3DSeed.getLambdaAtZ(startTime);
    double lambdaEndTime = a3DSeed.getLambdaAtZ(endTime);
    TVector3 start =  a3DSeed.getStart() + lambdaStartTime*a3DSeed.getTangent();
    TVector3 end =  a3DSeed.getStart() + lambdaEndTime*a3DSeed.getTangent();
    a3DSeed.setStartEnd(start, end);
    aTrackCandidate.addSegment(a3DSeed);
  }
  std::cout<<KBLU<<"Hand clicked track: "<<RST<<std::endl;
  std::cout<<aTrackCandidate<<std::endl;
  
  myFittedTrack = aTrackCandidate;
  myFittedTrack.setHypothesisFitLoss(0);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackBuilder::getLengthProjectionFromProfile(definitions::projection_type iProj, 
                                                    definitions::projection_type auxProj) const{

   definitions::projection_type iProjTmp = iProj;
   if(iProj==definitions::projection_type::DIR_TIME) iProjTmp = auxProj;                                                 

   TH1D *hProfile = nullptr; 
   if(iProj==definitions::projection_type::DIR_TIME) hProfile = getRecHits2D(iProjTmp).ProfileY();
   else hProfile = getRecHits2D(iProjTmp).ProfileX();
   int firstBin = -1;
   int lastBin = -1;
   double value = 0.0;
  
   for(int iBin=-1;iBin<=hProfile->GetNbinsX()+1;++iBin){
     hProfile->SetBinError(iBin, 1.0);
     value = hProfile->GetBinContent(iBin);
     if(std::abs(value)>0.0 && firstBin==-1) firstBin = iBin;
     else if(std::abs(value)>0.0) lastBin = iBin;
   }
   double minPos = hProfile->GetBinCenter(firstBin);
   double maxPos = hProfile->GetBinCenter(lastBin);
   if(std::abs(lastBin-firstBin)<1) return 0.0;

   auto aPol = new TF1("aFit", "[0]+[1]*x", minPos, maxPos);
   aPol->SetParameter(0, (minPos+maxPos)/2.0);
   aPol->SetParameter(1, 0.0);
   TFitResultPtr aFitResult = hProfile->Fit(aPol,"BRWWSQ");
   double slope = aFitResult->GetParams()[1];
   double length = (maxPos - minPos)*slope;
   delete aPol;
   delete hProfile;
   return std::abs(length);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackBuilder::getSignedLengthProjection(definitions::projection_type iProj,
                                               definitions::projection_type auxProj,
                                               int iTrack2DSeed) const{

   if(iTrack2DSeed>=0 and iTrack2DSeed<int(my2DSeeds[auxProj].size())){
     if(iProj==definitions::projection_type::DIR_TIME){
       double value = my2DSeeds[auxProj].at(iTrack2DSeed).getEnd().X() - my2DSeeds[auxProj].at(iTrack2DSeed).getStart().X();
      return value;
     }
     else {
      double value = my2DSeeds[auxProj].at(iTrack2DSeed).getEnd().Y() - my2DSeeds[auxProj].at(iTrack2DSeed).getStart().Y();
      return value;
     }
  }
  else if(iTrack2DSeed>=0) return 0.0;

   const TH1D* hProj = &hTimeProjection;

   definitions::projection_type iProjTmp = iProj;
   if(iProj==definitions::projection_type::DIR_TIME) iProjTmp = auxProj;
   else hProj = getRecHits2D(iProj).ProjectionY("hProj");                                       

  double minStrip=0, maxStrip=0;
  std::tie(minStrip, maxStrip) = getProjectionEdges(*hProj, stripDiffusionMargin);
  double minTimeBin=0, maxTimeBin=0;
  std::tie(minTimeBin, maxTimeBin) = getProjectionEdges(hTimeProjection, timeDiffusionMargin);

   /// Have to get maximum bin from full 2D histogram
  /// to avoid bias in the maximum position from the projection
  int iMaxBinX{0}, iMaxBinY{0}, iMaxBinZ{0};
  getRecHits2D(iProjTmp).GetMaximumBin(iMaxBinX, iMaxBinY, iMaxBinZ);
  ///Rebin 2D histogram in time bins to avoid cases when a single bin
  ///on the far end of track is dominating the maximum position
  if(std::abs(maxTimeBin-minTimeBin)<minTimeProjLength && iProj!=definitions::projection_type::DIR_TIME){
    TH2D *hTmp = (TH2D*)getRecHits2D(iProjTmp).Clone("hTmp");
    hTmp->Rebin2D(8,1)->GetMaximumBin(iMaxBinX, iMaxBinY, iMaxBinZ);
  }

  int iMaxBin = iMaxBinY;
  if(iProj==definitions::projection_type::DIR_TIME) iMaxBin = iMaxBinX;
  //If track is horizontal use a projection on strip direction to avoid bias in maximum position
  else if(std::abs(maxTimeBin-minTimeBin)<minTimeProjLength &&
          std::abs(maxStrip-minStrip)<minStripProjLength) iMaxBin = hProj->GetMaximumBin();

  double maxPos = hProj->GetBinCenter(iMaxBin);

  int direction = std::abs(maxPos - minStrip)<std::abs(maxPos - maxStrip) ? 1 : -1;

  double length = std::abs(maxStrip - minStrip);
  if(iProj==definitions::projection_type::DIR_TIME && length<minStripProjLength){
    length = getLengthProjectionFromProfile(iProj, auxProj);
  }

  double signedLength = length*direction;

  if(iProj!=definitions::projection_type::DIR_TIME) delete hProj;//FIXME - unify types
  return signedLength;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TVector2 TrackBuilder::get2DBias(definitions::projection_type iProj, int iTrack2DSeed) const{

  if(iTrack2DSeed>=0 and iTrack2DSeed<int(my2DSeeds[iProj].size())){
   return TVector2(my2DSeeds[iProj].at(iTrack2DSeed).getStart().X(),
                   my2DSeeds[iProj].at(iTrack2DSeed).getStart().Y());
  }

  double minTimeBin=0, maxTimeBin=0;
  std::tie(minTimeBin, maxTimeBin) = getProjectionEdges(hTimeProjection, timeDiffusionMargin);

  int iMaxBinX{0}, iMaxBinY{0}, iMaxBinZ{0};
  getRecHits2D(iProj).GetMaximumBin(iMaxBinX, iMaxBinY, iMaxBinZ);
  double maxTimePos = getRecHits2D(iProj).GetXaxis()->GetBinCenter(iMaxBinX);
  //If track is horizontal use a projection on strip direction to avoid bias in maximum position
  if(std::abs(maxTimeBin-minTimeBin)<minTimeProjLength) iMaxBinY = getRecHits2D(iProj).ProjectionY()->GetMaximumBin();
  double maxStripPos = getRecHits2D(iProj).GetYaxis()->GetBinCenter(iMaxBinY);

  return TVector2(maxTimePos, maxStripPos);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::getSignedLengthsAndMaxPos(int iTrack2DSeed) {

 /// Lengths of projection on strip directions
  double longProj = 0.0;
  for(auto iDir : getProjectionsList()){
    lengths[iDir] = getSignedLengthProjection(iDir, iDir, iTrack2DSeed);
    biases2D[iDir] = get2DBias(iDir, iTrack2DSeed);
    if(std::abs(lengths[iDir])>longProj){
      longProj = std::abs(lengths[iDir]);
      long_proj_type = iDir;
    }
  }
  /// Length of time projection
  lengths[definitions::projection_type::DIR_TIME] = getSignedLengthProjection(definitions::projection_type::DIR_TIME, long_proj_type, iTrack2DSeed);
}
/////////////////////////////////////////////////////////
TVector3 TrackBuilder::getBias(int iTrack2DSeed, bool acceptDot){

  TVector2 bias2D, bias2D_mean;
  double biasZ = 0.0;
  bool result{false};
  double weight{0.0}, weightSum{0.0};

  for(auto iDir_1: getProjectionsList()){
    for(auto iDir_2: getProjectionsList()){
      if(iDir_1==iDir_2) continue;
     
        result = myGeometryPtr->GetUVWCrossPointInMM(iDir_1, biases2D[iDir_1].Y(),
                                                      iDir_2, biases2D[iDir_2].Y(),
                                                      bias2D);

        weight = std::abs(lengths[iDir_1]*lengths[iDir_2])*result;
        weightSum += weight;    

        bias2D_mean += bias2D*weight;                     
        biasZ += biases2D[iDir_1].X()*weight;        

    }
   }
   bias2D_mean = bias2D_mean/weightSum;
   biasZ /= weightSum;
  
  TVector3 aBias(bias2D_mean.X(), bias2D_mean.Y(), biasZ);
  return aBias;  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackBuilder::getXYLength(definitions::projection_type dir1, 
                                 definitions::projection_type dir2) const{

  double l1 = lengths.at(dir1);
  double l2 = lengths.at(dir2);
  double cosDir1Dir2 = myGeometryPtr->GetStripPitchVector(dir1)*myGeometryPtr->GetStripPitchVector(dir2);
  double g = 1 - std::pow(cosDir1Dir2,2);
  double lXY = sqrt((std::pow(l1,2) + std::pow(l2,2) - 2*l1*l2*cosDir1Dir2)/g);
  return lXY;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackBuilder::getTangentPhiFromUnsignedLengths() const{

  double phi_U = myGeometryPtr->GetStripPitchVector(definitions::projection_type::DIR_U).Phi();
  double phi_V = myGeometryPtr->GetStripPitchVector(definitions::projection_type::DIR_V).Phi();
  double phi_W = myGeometryPtr->GetStripPitchVector(definitions::projection_type::DIR_W).Phi();
  double phi_longProj = myGeometryPtr->GetStripPitchVector(long_proj_type).Phi();

  double l_U = lengths.at(definitions::projection_type::DIR_U);
  double l_V = lengths.at(definitions::projection_type::DIR_V);
  double l_W = lengths.at(definitions::projection_type::DIR_W);
  double longProjLength = std::abs(lengths.at(long_proj_type));

  if(phi_U<0) phi_U += 2*M_PI;
  if(phi_V<0) phi_V += 2*M_PI;
  if(phi_W<0) phi_W += 2*M_PI;
  if(phi_longProj<0) phi_longProj += 2*M_PI;

  double phi{0}, bestPhi{0};
  double loss{0}, bestLoss{1E6};
  double longProjCos{0};
  double ratio_diff_U{0}, ratio_diff_V{0}, ratio_diff_W{0};

  double ratio_U{0}, ratio_V{0}, ratio_W{0};

  /// FIXME: use minimization instead of stupid loop
  int nSteps = 180;
  for (int iPhi=0;iPhi<nSteps;++iPhi){
    phi = iPhi*M_PI/nSteps;

    ///this variant uses information of only two lengths, the third one  is
    /// used as normalization factor
    longProjCos = std::abs(cos(phi_longProj - phi));
    ratio_diff_U = std::abs(l_U)/longProjLength - std::abs(cos(phi_U-phi))/longProjCos;
    ratio_diff_V = std::abs(l_V)/longProjLength - std::abs(cos(phi_V-phi))/longProjCos;
    ratio_diff_W = std::abs(l_W)/longProjLength - std::abs(cos(phi_W-phi))/longProjCos;
    loss = std::abs(ratio_diff_U) + std::abs(ratio_diff_V) + std::abs(ratio_diff_W);
    
    ///another variant of loss function, that uses information of all three
    ///projections 
    ratio_U = std::abs(cos(phi_U-phi)/l_U);
    ratio_V = std::abs(cos(phi_V-phi)/l_V);
    ratio_W = std::abs(cos(phi_W-phi)/l_W);
    double loss1 = std::abs(ratio_U-ratio_V) +
            std::abs(ratio_U-ratio_W) +
            std::abs(ratio_V-ratio_W);

    //loss1 /= ratio_long;        
/*
    if(std::abs(phi)<0.01){        
    std::cout<<KGRN<<"loss: "<<RST<<loss<<" phi="<<phi
             <<" l_U="<<l_U<<" l_V="<<l_V<<" l_W="<<l_W
             <<" ratio_diff_U="<<ratio_diff_U
             <<" ratio_diff_V="<<ratio_diff_V
             <<" ratio_diff_W="<<ratio_diff_W
             <<std::endl
             <<" loss1: "<<loss1
             <<" ratio_U="<<ratio_U
              <<" ratio_V="<<ratio_V
              <<" ratio_W="<<ratio_W
              <<" ratio_U-ratio_long="<<std::abs(ratio_U-ratio_V)
              <<" ratio_V-ratio_long="<<std::abs(ratio_U-ratio_W)
              <<" ratio_W-ratio_long="<<std::abs(ratio_W-ratio_V)
             <<std::endl;  
    }   
    */
    
  

    loss += 0*loss1;
       
      
    if(loss<bestLoss){
      bestLoss = loss;
      bestPhi = phi;
    }
  }  

  //std::cout<<KGRN<<"bestPhi="<<bestPhi<<" bestLoss="<<bestLoss<<RST<<std::endl;
  return bestPhi;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackBuilder::getTangentPhiFromSignedLengths(definitions::projection_type dir1, 
                                                    definitions::projection_type dir2) const{
   //Solve equations for cos(phi) and sin(phi):
   /// l_dir1 = e_dir1*cos(phi) + e_dir1*sin(phi)
   /// l_dir2 = e_dir2*cos(phi) + e_dir2*sin(phi)
   ////////////////
   double determinant = myGeometryPtr->GetStripPitchVector(dir1).X()*
                       myGeometryPtr->GetStripPitchVector(dir2).Y() -

                       myGeometryPtr->GetStripPitchVector(dir1).Y()*
                       myGeometryPtr->GetStripPitchVector(dir2).X();

   double l1 = lengths.at(dir1);
   double l2 = lengths.at(dir2);                    

  double cosPhi = (l1*myGeometryPtr->GetStripPitchVector(dir2).Y() - 
                   l2*myGeometryPtr->GetStripPitchVector(dir1).Y())/determinant;

  double sinPhi = (l2*myGeometryPtr->GetStripPitchVector(dir1).X() - 
                  l1*myGeometryPtr->GetStripPitchVector(dir2).X())/determinant;                
  
  double phi = atan2(sinPhi, cosPhi);
  return phi;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TVector3 TrackBuilder::getTangent(int iTrack2DSeed, bool guiMode){
    
  ///Mean values of XY lengths and tangent angles
  double weight{0}, weightSum{1};
  double l_XY{0}, l_XY_mean{0};
  double phi{0};
  ROOT::Math::DisplacementVector2D<ROOT::Math::Polar2D<double> > aTmpVector;

   for(auto iDir_1: getProjectionsList()){
    for(auto iDir_2: getProjectionsList()){
      if(iDir_1==iDir_2) continue;

       weight = std::abs(lengths[iDir_1]*lengths[iDir_2]);
       weightSum += weight;

       l_XY = getXYLength(iDir_1, iDir_2);
       l_XY_mean  += l_XY*weight;

       phi = getTangentPhiFromSignedLengths(iDir_1, iDir_2);
       aTmpVector += ROOT::Math::DisplacementVector2D<ROOT::Math::Polar2D<double> >(weight, phi);  
    }
   }
    l_XY_mean /= weightSum;

    double theta = M_PI/2.0 - atan2(lengths[definitions::projection_type::DIR_TIME], l_XY_mean);
    if(guiMode) phi = aTmpVector.Phi();
    else phi = getTangentPhiFromUnsignedLengths();
                                      
  TVector3 aTangent;
  aTangent.SetMagThetaPhi(1, theta, phi);
  return aTangent;                               
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackSegment3D TrackBuilder::buildSegment3D(int iTrack2DSeed, bool guiMode){

  getSignedLengthsAndMaxPos(iTrack2DSeed);

  TVector3 aBias = getBias(iTrack2DSeed);
  TVector3 aTangent = getTangent(iTrack2DSeed, guiMode);

  TrackSegment3D a3DSeed;
  a3DSeed.setGeometry(myGeometryPtr); 
  a3DSeed.setBiasTangent(aBias, aTangent);
  a3DSeed.setRecHits(myRecHits);

  if(guiMode) return a3DSeed;

  ///FIX ME Stupid work around for track direction ambiguity
  double totalCharge = a3DSeed.getIntegratedCharge(a3DSeed.getLength());

  TVector3 aTangent_flipped = aTangent;
  aTangent_flipped.SetTheta(-aTangent_flipped.Theta());
  a3DSeed.setBiasTangent(aBias, aTangent_flipped);
  double totalCharge_flipped = a3DSeed.getIntegratedCharge(a3DSeed.getLength());

  if(totalCharge_flipped<totalCharge){
    a3DSeed.setBiasTangent(aBias, aTangent);
  }
  ///////////////////////////////////////////////////////////

  return a3DSeed;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Track3D TrackBuilder::fitDot(Track3D & aFittedTrack){

  bool acceptDot = true;
  TVector3 aStart = getBias(0, acceptDot);
  aFittedTrack.getSegments().front().setGeometry(myGeometryPtr);  
  aFittedTrack.getSegments().front().setStartEnd(aStart, aStart);
  aFittedTrack.getSegments().front().setRecHits(myRawHits);
  aFittedTrack.getSegments().front().setDiffusion(0);
  aFittedTrack.getSegments().front().setPID(pid_type::DOT);
  aFittedTrack.update();
  return aFittedTrack;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::fitTrack3DInSelectedDir(Track3D & aFittedTrack, definitions::fit_type fitType){

  double chamberRadius = 300; //mm parameter to be moved to configuration

  aFittedTrack.setFitMode(fitType);
  aFittedTrack.extendToChamberRange(chamberRadius);

  double initialLoss = aFittedTrack.getLoss();
  auto fitResult = fitTrackNodesBiasTangent(aFittedTrack, fitType);
  double afterFitLoss = fitResult.MinFcnValue();
   if(initialLoss>1.01*afterFitLoss){
    aFittedTrack.updateAndGetLoss(fitResult.GetParams());
  }  
 aFittedTrack.extendToChamberRange(chamberRadius);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Track3D TrackBuilder::fitTrack3D(const Track3D & aTrackCandidate){

  Track3D aFittedTrack = aTrackCandidate; 
  if(aFittedTrack.getSegments().front().getBias().Mag()<1E-6){
    aFittedTrack = fitDot(aFittedTrack);
    return aFittedTrack;
  }

  fitTrack3DInSelectedDir(aFittedTrack, definitions::fit_type::TANGENT);
  fitTrack3DInSelectedDir(aFittedTrack, definitions::fit_type::BIAS_XY);
  fitTrack3DInSelectedDir(aFittedTrack, definitions::fit_type::BIAS_Z);
  aFittedTrack.shrinkToHits();

  return aFittedTrack;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
ROOT::Fit::FitResult TrackBuilder::fitTrackNodesBiasTangent(const Track3D & aTrack, 
                    definitions::fit_type fitType) const{

  double zRange = 20; //parameter to be moved to configuration
  double xyRange = 20; //parameter to be moved to configuration
  double thetaRange = 0.4; //parameter to be moved to configuration
  double phiRange = 0.4; //parameter to be moved to configuration                    

  Track3D aTrackCandidate = aTrack;
  aTrackCandidate.setFitMode(fitType);
  std::vector<double> params = aTrackCandidate.getSegmentsBiasTangentCoords();
  int nParams = 2;

  ROOT::Math::Functor fcn(&aTrackCandidate, &Track3D::updateAndGetLoss, nParams);
  fitter.SetFCN(fcn, params.data());

  if(fitType==definitions::fit_type::BIAS_Z){
    double biasZ = aTrackCandidate.getSegments().front().getBias().Z();
    fitter.Config().ParSettings(0).SetValue(biasZ);
    fitter.Config().ParSettings(0).SetLimits(biasZ-zRange/2, biasZ+zRange/2);
    fitter.Config().ParSettings(0).SetStepSize(zRange/2);
    fitter.Config().ParSettings(1).Fix();

  }
  else if(fitType==definitions::fit_type::BIAS_XY){
    double biasX = aTrackCandidate.getSegments().front().getBias().X();
    double biasY = aTrackCandidate.getSegments().front().getBias().Y();
    fitter.Config().ParSettings(0).SetValue(biasX);
    fitter.Config().ParSettings(0).SetLimits(biasX-xyRange/2, biasX+xyRange/2);
    fitter.Config().ParSettings(0).SetStepSize(xyRange/2);

    fitter.Config().ParSettings(1).SetValue(biasY);
    fitter.Config().ParSettings(1).SetLimits(biasY-xyRange/2, biasY+xyRange/2);
    fitter.Config().ParSettings(1).SetStepSize(xyRange/2);

    fitter.Config().ParSettings(0).Release();
    fitter.Config().ParSettings(1).Release();
  }
  else if(fitType==definitions::fit_type::TANGENT){
    double theta = aTrackCandidate.getSegments().front().getTangent().Theta();  
    double phi = aTrackCandidate.getSegments().front().getTangent().Phi();
    fitter.Config().ParSettings(0).SetValue(theta);
    fitter.Config().ParSettings(0).SetStepSize(thetaRange/2);
    fitter.Config().ParSettings(0).SetLimits(theta-thetaRange/2, theta+thetaRange/2);

    fitter.Config().ParSettings(1).SetValue(phi);
    fitter.Config().ParSettings(1).SetStepSize(phiRange/2);
    fitter.Config().ParSettings(1).SetLimits(phi-phiRange/2, phi+phiRange/2);

    fitter.Config().ParSettings(0).Release();
    fitter.Config().ParSettings(1).Release();
  }
    
  bool fitStatus = fitter.FitFCN();
  if (!fitStatus) {
    Error(__FUNCTION__, "Track3D Fit failed");
    std::cout<<KRED<<"Track3D Fit failed."<<RST<<std::endl;
      fitter.Result().Print(std::cout);
    }
  return fitter.Result();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Track3D TrackBuilder::fitEventHypothesis(const Track3D & aTrackCandidate){

  if(aTrackCandidate.getLength()<1) return aTrackCandidate;

  const TrackSegment3D & aSegment = aTrackCandidate.getSegments().front(); 
  TH1F hChargeProfile = aSegment.getChargeProfile(); 
  mydEdxFitter.fitHisto(hChargeProfile);

  pid_type eventType = mydEdxFitter.getBestFitEventType();
  bool isReflected = mydEdxFitter.getIsReflected();
  double vertexOffset = mydEdxFitter.getVertexOffset();
  double alphaRange = mydEdxFitter.getAlphaRange();
  double carbonRange = mydEdxFitter.getCarbonRange();

  TVector3 tangent =  aSegment.getTangent();
  TVector3 start =  aSegment.getStart();
  
  if(isReflected){
    tangent *=-1;
    start = aSegment.getEnd();
  }

  TVector3 vertexPos =  start + vertexOffset*tangent;
  TVector3 alphaEnd =  vertexPos + alphaRange*tangent;
  TVector3 carbonEnd =  vertexPos - carbonRange*tangent;

  Track3D aSplitTrackCandidate;
  aSplitTrackCandidate.setChargeProfile(mydEdxFitter.getFittedHisto());
  aSplitTrackCandidate.setHypothesisFitLoss(mydEdxFitter.getLoss());
  TrackSegment3D alphaSegment;
  alphaSegment.setGeometry(myGeometryPtr);  
  alphaSegment.setStartEnd(vertexPos, alphaEnd);
  alphaSegment.setRecHits(myRawHits);
  alphaSegment.setDiffusion(mydEdxFitter.getDiffusion());
  alphaSegment.setPID(pid_type::ALPHA);
  aSplitTrackCandidate.addSegment(alphaSegment);
  
  if(eventType==C12_ALPHA){   
    TrackSegment3D carbonSegment;
    carbonSegment.setGeometry(myGeometryPtr);  
    carbonSegment.setStartEnd(vertexPos, carbonEnd);
    carbonSegment.setRecHits(myRawHits);
    carbonSegment.setDiffusion(mydEdxFitter.getDiffusion());
    carbonSegment.setPID(pid_type::CARBON_12);
    aSplitTrackCandidate.addSegment(carbonSegment);
  }
  return aSplitTrackCandidate;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
