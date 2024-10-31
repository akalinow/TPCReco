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

#include "TPCReco/GeometryTPC.h"

#include "TPCReco/TrackBuilder.h"
#include "TPCReco/colorText.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
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
  fitter.Config().MinimizerOptions().SetMinimizerAlgorithm("Scan");
  
  ///An offset used for filling the Hough transformation.
  ///to avoid having very small rho parameters, as
  ///orignally many tracks traverse close to STRIP_POS[mm]=0, TIME_POS[mm]=0
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
    fillHoughAccumulator(iDir);
    my2DSeeds[iDir] = findSegment2DCollection(iDir);    
  }
  myZRange = getProjectionEdges(hTimeProjection);
  myTrack3DSeed = buildSegment3D();
  if(myTrack3DSeed.getLength()<1) return;
  
  Track3D aTrackCandidate;
  aTrackCandidate.addSegment(myTrack3DSeed);
  aTrackCandidate = fitTrack3D(aTrackCandidate);
  aTrackCandidate = fitEventHypothesis(aTrackCandidate);
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
  
  int iBinStart = -1;
  int iBinEnd = -1;
  double histoSum = hProj.Integral();
  double sum = 0.0;
  double threshold = 0.01;

  for(auto iBin=0;iBin<hProj.GetNbinsX();++iBin){
    sum += hProj.GetBinContent(iBin); 
    if(sum/histoSum>threshold && iBinStart<0) iBinStart = iBin;
    else if(sum/histoSum>1.0-threshold && iBinEnd<0) {
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
const TH2D & TrackBuilder::getCluster2D(int iDir) const{

  return myRawHits[iDir];  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TH2D & TrackBuilder::getRecHits2D(int iDir) const{

  return myRecHits[iDir];  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TH1D & TrackBuilder::getRecHitsTimeProjection() const{

  return hTimeProjection;
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
    TrackSegment3D a3DSeed = buildSegment3D(iSegment);
    double startTime = my2DSeeds.at(definitions::projection_type::DIR_U).at(iSegment).getStart().X();    
    double endTime = my2DSeeds.at(definitions::projection_type::DIR_U).at(iSegment).getEnd().X();
    double lambdaStartTime = a3DSeed.getLambdaAtZ(startTime);
    double lambdaEndTime = a3DSeed.getLambdaAtZ(endTime);
    TVector3 start =  a3DSeed.getStart() + lambdaStartTime*a3DSeed.getTangent();
    TVector3 end =  a3DSeed.getStart() + lambdaEndTime*a3DSeed.getTangent();
    a3DSeed.setStartEnd(start, end);
    a3DSeed.setRecHits(myRawHits);
    aTrackCandidate.addSegment(a3DSeed);
  }
  std::cout<<KBLU<<"Hand clicked track: "<<RST<<std::endl;
  std::cout<<aTrackCandidate<<std::endl;
  
  myFittedTrack = aTrackCandidate;
  myFittedTrack.setHypothesisFitLoss(0);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::normaliseTangents(TVector3 & tangent_U, TVector3 & tangent_V, TVector3 & tangent_W) const{

  //Normalise tangents along the common axis - the time axis
  double epsilon = 1E-2;
  bool hasTimeComponent = (std::abs(tangent_U.Unit().X()*tangent_V.Unit().X())>epsilon) + 
                          (std::abs(tangent_U.Unit().X()*tangent_W.Unit().X())>epsilon) +
                          (std::abs(tangent_V.Unit().X()*tangent_W.Unit().X())>epsilon);

  if(hasTimeComponent){
    tangent_U *= 1.0/tangent_U.X();
    tangent_V *= 1.0/tangent_V.X();
    tangent_W *= 1.0/tangent_W.X();
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackBuilder::getXYTangentPhiFromProjsTangents(definitions::projection_type dir1, definitions::projection_type dir2,
                                                      double l1, double l2) const{


   //Solve equations for cos(phi) and sin(phi):
   /// l_dir1 = e_dir1*cos(phi) + e_dir1*sin(phi)
   /// l_dir2 = e_dir2*cos(phi) + e_dir2*sin(phi)
   ////////////////
   double determinant = myGeometryPtr->GetStripPitchVector(dir1).X()*
                       myGeometryPtr->GetStripPitchVector(dir2).Y() -

                       myGeometryPtr->GetStripPitchVector(dir1).Y()*
                       myGeometryPtr->GetStripPitchVector(dir2).X();

  double cosPhi = (l1*myGeometryPtr->GetStripPitchVector(dir2).Y() - 
                   l2*myGeometryPtr->GetStripPitchVector(dir1).Y())/determinant;

  double sinPhi = (l2*myGeometryPtr->GetStripPitchVector(dir1).X() - 
                  l1*myGeometryPtr->GetStripPitchVector(dir2).X())/determinant;                
  
  double phi = atan2(sinPhi, cosPhi);
  return phi;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackBuilder::getSignedLengthProjection(definitions::projection_type iProj,
                                               definitions::projection_type auxProj) const{

   const TH1D* hProj;
   double diffusionMargin = 2.0;
   definitions::projection_type iProjTmp = iProj;
  if(iProj==definitions::projection_type::DIR_TIME){
    hProj = &hTimeProjection;
    diffusionMargin = 4.0;
    iProjTmp = auxProj;
  }
  else hProj = getRecHits2D(iProj).ProjectionY("hProj");

  /// Have to get maximum bin from full 2D histogram
  /// to avoid bias in the maximum position from the projection
  int iMaxBinX{0}, iMaxBinY{0}, iMaxBinZ{0};
  getRecHits2D(iProjTmp).GetMaximumBin(iMaxBinX, iMaxBinY, iMaxBinZ);
  int iMaxBin = iMaxBinY;
  if(iProj==definitions::projection_type::DIR_TIME) iMaxBin = iMaxBinX;
  double maxPos = hProj->GetBinCenter(iMaxBin);

  double minStrip=0, maxStrip=0;
  std::tie(minStrip, maxStrip) = getProjectionEdges(*hProj, diffusionMargin);
  int direction = std::abs(maxPos - minStrip)<std::abs(maxPos - maxStrip) ? 1 : -1;
  double l = (maxStrip - minStrip)*direction;

  if(iProj!=definitions::projection_type::DIR_TIME) delete hProj;//FIXME - unify types
  return l;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TVector3 TrackBuilder::getBias(int iTrack2DSeed) const{

  const TrackSegment2D & segmentU = my2DSeeds[definitions::projection_type::DIR_U][iTrack2DSeed];
  const TrackSegment2D & segmentV = my2DSeeds[definitions::projection_type::DIR_V][iTrack2DSeed];
  const TrackSegment2D & segmentW = my2DSeeds[definitions::projection_type::DIR_W][iTrack2DSeed];

  int nHits_U = segmentU.getNAccumulatorHits();
  int nHits_V = segmentV.getNAccumulatorHits();
  int nHits_W = segmentW.getNAccumulatorHits();

  double bias_time = (segmentU.getMinBias().X()*(nHits_U>0) +
		                  segmentV.getMinBias().X()*(nHits_V>0) +
		                  segmentW.getMinBias().X()*(nHits_W>0))/((nHits_U>0) + (nHits_V>0) + (nHits_W>0));

  TVector3 bias_U = segmentU.getBiasAtT(bias_time);
  TVector3 bias_V = segmentV.getBiasAtT(bias_time);
  TVector3 bias_W = segmentW.getBiasAtT(bias_time);
 
  TVector2 biasXY_fromUV;
  bool res1=myGeometryPtr->GetUVWCrossPointInMM(segmentU.getStripDir(), bias_U.Y(),
						segmentV.getStripDir(), bias_V.Y(), biasXY_fromUV);

  TVector2 biasXY_fromVW;
  bool res2=myGeometryPtr->GetUVWCrossPointInMM(segmentV.getStripDir(), bias_V.Y(),
						segmentW.getStripDir(), bias_W.Y(), biasXY_fromVW);

  TVector2 biasXY_fromWU;
  bool res3=myGeometryPtr->GetUVWCrossPointInMM(segmentW.getStripDir(), bias_W.Y(),
						segmentU.getStripDir(), bias_U.Y(), biasXY_fromWU);
  assert(res1|res2|res3);

  int weight_fromUV = res1*(nHits_U + nHits_V)*(nHits_U>0)*(nHits_W>0);
  int weight_fromVW = res2*(nHits_V + nHits_W)*(nHits_V>0)*(nHits_W>0);
  int weight_fromWU = res3*(nHits_W + nHits_U)*(nHits_W>0)*(nHits_U>0);
  int weight_sum = weight_fromUV+ weight_fromVW+weight_fromWU;
  const TVector2 biasXY=(weight_fromUV*biasXY_fromUV + weight_fromVW*biasXY_fromVW + weight_fromWU*biasXY_fromWU)/weight_sum;

  int weight_fromU = res1*nHits_U;
  int weight_fromV = res2*nHits_V;
  int weight_fromW = res3*nHits_W;
  weight_sum = weight_fromU+ weight_fromV+weight_fromW;

  double biasZ = (bias_U.X()*weight_fromU +  bias_V.X()*weight_fromV +  bias_W.X()*weight_fromW)/weight_sum;  
  TVector3 aBias( biasXY.X(), biasXY.Y(), biasZ);

  return aBias;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackBuilder::getXYLength(definitions::projection_type dir1, 
                                 definitions::projection_type dir2,
                                 double l1, double l2) const{

  double cosDir1Dir2 = myGeometryPtr->GetStripPitchVector(dir1)*myGeometryPtr->GetStripPitchVector(dir2);
  double g = 1 - std::pow(cosDir1Dir2,2);
  double lXY = sqrt((std::pow(l1,2) + std::pow(l2,2) - 2*l1*l2*cosDir1Dir2)/g);
  return lXY;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TVector3 TrackBuilder::getTangent(int iTrack2DSeed) const{

    double minStripProjLength = 20;
    double minTimeProjLength = 13;
    double epsilon = 1E-2;

  const TrackSegment2D & segmentU = my2DSeeds[definitions::projection_type::DIR_U][iTrack2DSeed];
  const TrackSegment2D & segmentV = my2DSeeds[definitions::projection_type::DIR_V][iTrack2DSeed];
  const TrackSegment2D & segmentW = my2DSeeds[definitions::projection_type::DIR_W][iTrack2DSeed];

  int nHits_U = segmentU.getNAccumulatorHits();
  int nHits_V = segmentV.getNAccumulatorHits();
  int nHits_W = segmentW.getNAccumulatorHits();

  if(nHits_U*nHits_V*nHits_W==0) return TVector3(0,0,0);

  TVector3 tangent_U = segmentU.getTangent();
  TVector3 tangent_V = segmentV.getTangent();
  TVector3 tangent_W = segmentW.getTangent();

  /// Normalise tangents along the common axis - the time axis (if possible)
  normaliseTangents(tangent_U, tangent_V, tangent_W);
    
  /// Lengths of projection on strip directions
  double l_U = getSignedLengthProjection(definitions::projection_type::DIR_U);
  double l_V = getSignedLengthProjection(definitions::projection_type::DIR_V);
  double l_W = getSignedLengthProjection(definitions::projection_type::DIR_W);
  
  definitions::projection_type auxProj = definitions::projection_type::DIR_U;
  if(std::abs(l_V)>std::abs(l_U)) auxProj = definitions::projection_type::DIR_V;
  else if(std::abs(l_W)>std::abs(l_U)) auxProj = definitions::projection_type::DIR_W;
  double l_T = getSignedLengthProjection(definitions::projection_type::DIR_TIME, auxProj);
  l_T *= std::abs(l_T)>minTimeProjLength;

  if(std::abs(l_T)<minTimeProjLength) minStripProjLength = 25;;

  double weight_fromUV = std::abs(l_U*l_V)*(std::abs(l_U)>minStripProjLength)*(std::abs(l_V)>minStripProjLength);
  double weight_fromVW = std::abs(l_V*l_W)*(std::abs(l_V)>minStripProjLength)*(std::abs(l_W)>minStripProjLength);
  double weight_fromWU = std::abs(l_W*l_U)*(std::abs(l_W)>minStripProjLength)*(std::abs(l_U)>minStripProjLength);
  double weightSum = weight_fromUV + weight_fromVW + weight_fromWU;
  if(weightSum<epsilon) weightSum = 1;

  /// Lengths of projection on XY plane calculated fro pairs of projections
  double lXY_UV = getXYLength(definitions::projection_type::DIR_U, definitions::projection_type::DIR_V, l_U, l_V);
  double lXY_VW = getXYLength(definitions::projection_type::DIR_V, definitions::projection_type::DIR_W, l_V, l_W);
  double lXY_WU = getXYLength(definitions::projection_type::DIR_W, definitions::projection_type::DIR_U, l_W, l_U);
  double l_XY = (lXY_UV*weight_fromUV + lXY_VW*weight_fromVW + lXY_WU*weight_fromWU)/weightSum;

  double theta = M_PI/2.0 - atan2(l_T, l_XY);

  double phiUV = getXYTangentPhiFromProjsTangents(definitions::projection_type::DIR_U, definitions::projection_type::DIR_V, l_U, l_V);
  double phiVW = getXYTangentPhiFromProjsTangents(definitions::projection_type::DIR_V, definitions::projection_type::DIR_W, l_V, l_W);
  double phiWU = getXYTangentPhiFromProjsTangents(definitions::projection_type::DIR_W, definitions::projection_type::DIR_U, l_W, l_U);
  double phi = (phiUV*weight_fromUV + phiVW*weight_fromVW + phiWU*weight_fromWU)/weightSum;
  if(phi<0) phi += 2*M_PI;
  else if(phi>2*M_PI) phi -= 2*M_PI;

  /*
  std::cout<<"lXY_UV: "<<lXY_UV<<" lXY_VW: "<<lXY_VW<<" lXY_WU: "<<lXY_WU<<" l_XY: "<<l_XY<<std::endl;
  std::cout<<"nHits_U: "<<nHits_U<<" nHits_V: "<<nHits_V<<" nHits_W: "<<nHits_W<<std::endl;
  std::cout<<"l_U: "<<l_U<<" l_V: "<<l_V<<" l_W: "<<l_W<<" l_T: "<<l_T<<" l_XY: "<<l_XY<<std::endl;
  std::cout<<"weight_fromUV: "<<weight_fromUV<<" weight_fromVW: "<<weight_fromVW<<" weight_fromWU: "<<weight_fromWU<<std::endl;
  std::cout<<"phiUV: "<<phiUV<<" phiVW: "<<phiVW<<" phiWU: "<<phiWU<<" phi: "<<phi<<std::endl;
  std::cout<<"theta: "<<theta<<" phi: "<<phi<<" deg: "<<phi/M_PI*180<<std::endl;
  */
  TVector3 aTangent;
  aTangent.SetMagThetaPhi(1.0, theta, phi);
  return aTangent;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackSegment3D TrackBuilder::buildSegment3D(int iTrack2DSeed) const{

  TVector3 aBias = getBias(iTrack2DSeed);
  TVector3 aTangent = getTangent(iTrack2DSeed);

  TrackSegment3D a3DSeed;
  a3DSeed.setGeometry(myGeometryPtr); 
  a3DSeed.setBiasTangent(aBias, aTangent);
  a3DSeed.setRecHits(myRecHits);
  return a3DSeed;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Track3D TrackBuilder::fitTrack3D(const Track3D & aTrackCandidate){

  Track3D aFittedTrack = aTrackCandidate; 
  aFittedTrack.setFitMode(definitions::fit_type::TANGENT_BIAS);
  aFittedTrack.getSegments().front().setRecHits(myRawHits);
  double chamberRadius = 150; //mm
  aFittedTrack.extendToChamberRange(chamberRadius);

  std::cout<<KBLU<<"Pre-fit: "<<RST<<std::endl; 
  std::cout<<aFittedTrack<<std::endl;
  double initialLoss = aFittedTrack.getLoss();
  
  auto fitResult = fitTrackNodesBiasTangent(aFittedTrack, definitions::fit_type::BIAS);
  aFittedTrack.updateAndGetLoss(fitResult.GetParams());

  fitResult = fitTrackNodesBiasTangent(aFittedTrack, definitions::fit_type::TANGENT);
  aFittedTrack.updateAndGetLoss(fitResult.GetParams());

  fitResult = fitTrackNodesBiasTangent(aFittedTrack, definitions::fit_type::TANGENT_BIAS);
  double finalLoss = fitResult.MinFcnValue();
  if(initialLoss>finalLoss){
    aFittedTrack.updateAndGetLoss(fitResult.GetParams());
  }  
  aFittedTrack.extendToChamberRange(chamberRadius);
  aFittedTrack.shrinkToHits();
  
  std::cout<<KBLU<<"Post-fit: "<<RST<<std::endl;
  std::cout<<aFittedTrack<<std::endl;
  return aFittedTrack;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
ROOT::Fit::FitResult TrackBuilder::fitTrackNodesBiasTangent(const Track3D & aTrack, 
                    definitions::fit_type fitType) const{

  Track3D aTrackCandidate = aTrack;
  aTrackCandidate.setFitMode(fitType);
  std::vector<double> params = aTrackCandidate.getSegmentsBiasTangentCoords();
  int nParams = params.size();

  ROOT::Math::Functor fcn(&aTrackCandidate, &Track3D::updateAndGetLoss, nParams);
  fitter.SetFCN(fcn, params.data());

  for (int iPar = 0; iPar < nParams; ++iPar){
    fitter.Config().ParSettings(iPar).SetValue(params[iPar]);
    if(iPar<1){//bias coordinates
      fitter.Config().ParSettings(iPar).SetStepSize(0.1);
      fitter.Config().ParSettings(iPar).SetLimits(0.01,300);
    }
    if(iPar==1){ //tangent polar angle
      fitter.Config().ParSettings(iPar).SetStepSize(0.05);
      fitter.Config().ParSettings(iPar).SetLimits(params[1]-0.2, params[1]+0.2);
    }
    if(iPar==2){ //tangent azimuthal angle 
      fitter.Config().ParSettings(iPar).SetStepSize(0.05);
      fitter.Config().ParSettings(iPar).SetLimits(params[2]-0.2, params[2]+0.2);
    }
  }
  for(int iIter=0;iIter<1;++iIter){
    if(iIter%2==0){
      fitter.Config().ParSettings(0).Fix();
      fitter.Config().ParSettings(1).Release();
      fitter.Config().ParSettings(2).Release();
    }
    else if(iIter%2==1){
      fitter.Config().ParSettings(0).Fix();
      fitter.Config().ParSettings(1).Release();
      fitter.Config().ParSettings(2).Release();
    }
    else if(iIter%2==2){
      fitter.Config().ParSettings(0).Release();
      fitter.Config().ParSettings(1).Fix();
      fitter.Config().ParSettings(2).Fix();
    }
    else if(iIter%2==3){
      fitter.Config().ParSettings(0).Release();
      fitter.Config().ParSettings(1).Release();
      fitter.Config().ParSettings(2).Release();
    }
    
    bool fitStatus = fitter.FitFCN();
    if (!fitStatus) {
      Error(__FUNCTION__, "Track3D Fit failed");
      std::cout<<KRED<<"Track3D Fit failed."<<RST
	       <<" iteration "<<iIter
	       <<RST<<std::endl;
      fitter.Result().Print(std::cout);
    }
    else if(iIter==0){ break;}
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

  std::cout<<KBLU<<"Post-split: "<<RST<<std::endl;
  std::cout<<aSplitTrackCandidate<<std::endl;
  return aSplitTrackCandidate;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
