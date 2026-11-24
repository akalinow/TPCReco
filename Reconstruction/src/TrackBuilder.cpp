#include <cstdlib>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <array>
#include <stdexcept>

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
#define M_PI (4.0*std::atan(1.0))
#endif // M_PI
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackBuilder::TrackBuilder(boost::property_tree::ptree& myConfig) {

  //  nAccumulatorRhoBins = 50;//FIX ME move to configurable
  //  nAccumulatorPhiBins = 2.0*M_PI/0.025;//FIX ME move to configurable
  //  nAccumulatorPhiBins = 2.0*M_PI/0.1;//FIX ME move to configurable
  nAccumulatorRhoBins = 200; //FIX ME large enough to keep RHO bin size below 0.5*STRIP_PITCH
  nAccumulatorPhiBins = 400; //FIX ME move to configurable

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

  myTemperature = myConfig.get<double>("conditions.temperature", 293.15);
  setPressure(myConfig.get<double>("conditions.pressure"));
  mydEdxFitter.setMaxFitAttempts(myConfig.get<int>("reconstruction.dEdxFitAttempts", 1));
  mydEdxFitter.setMaxLossRatio(myConfig.get<double>("reconstruction.dEdxFitMaxLossRatio", dEdxFitter::defaultMaxLossRatio));
  dEdxFitter::setPhysicsConstraint(myConfig.get<bool>("reconstruction.dEdxPhysicsConstraint", false),
                                   myConfig.get<double>("reconstruction.dEdxCarbonRangeMargin", 0.1));

  myMLTrackBuilder = std::make_shared<TensorflowModel>(myConfig);

  // charge gain per setup: track charge -> MeV, and R = Q/(G_raw E_dep) in the dE/dx fit
  dEdxOrientationByR = myConfig.get<bool>("reconstruction.dEdxOrientationByR", false);
  dEdxOrientationVertexWindow = myConfig.get<double>("reconstruction.dEdxOrientationVertexWindow", 6.0);
  myRecHitBuilder.setCleanClusterParameters(myConfig.get<double>("reconstruction.cleanClusterWindowTimeMM", -1.0),
                                            myConfig.get<double>("reconstruction.cleanClusterWindowStripMM", -1.0),
                                            myConfig.get<double>("reconstruction.cleanClusterThreshold", 0.2),
                                            myConfig.get<double>("reconstruction.cleanClusterEmptyBinThreshold", 1.0));
  const std::string gainTableFile = myConfig.get<std::string>("reconstruction.gainTableFile", "");
  if(!gainTableFile.empty()){
    try {
      myGainTable = std::make_shared<const GainTable>(gainTableFile);
      std::cout<<KBLU<<"Gain table: "<<RST<<gainTableFile<<std::endl;
    } catch (const std::exception & e) {
      std::cout<<KRED<<e.what()<<": no charge to energy conversion"<<RST<<std::endl;
    }
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackBuilder::~TrackBuilder() { }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr){

  myHistoInitialized = false; // forces to recalculate ranges of underlying histograms after changing geometry / run conditions
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
  mydEdxFitter.setGasConditions(myPressure, myTemperature);
  myRangeCalculator.setGasConditions(gas_mixture_type::CO2, myPressure, myTemperature);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool TrackBuilder::getRunGains(GainTable::Entry & gains) const {
  if(!myGainTable || !myEventPtr) return false;
  return myGainTable->find(myEventPtr->GetEventInfo().GetRunId(), myPressure, gains);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackBuilder::getChargeRatio(double charge, double gainRaw, const HypothesisFit & aFit){
  double energy = 0.0;
  if(aFit.alphaRange>0) energy += myRangeCalculator.getIonEnergyMeV(pid_type::ALPHA, aFit.alphaRange);
  if(aFit.carbonRange>0) energy += myRangeCalculator.getIonEnergyMeV(aFit.recoilPID, aFit.carbonRange);
  if(!(energy>0) || !(gainRaw>0)) return 0.0;
  return charge/(gainRaw*energy);
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
  std::cout<<KBLU<<"Before building 3D segment from 2D seeds"<<RST<<std::endl;
  myTrack3DSeed = buildSegment3D();
  
  Track3D aTrackCandidate;
  aTrackCandidate.addSegment(myTrack3DSeed);
  aTrackCandidate = fitTrack3D(aTrackCandidate);

  std::cout<<KBLU<<"Track length before hypothesis fit: "<<RST<<aTrackCandidate.getLength()<<std::endl;

  if(aTrackCandidate.getLength()>minTkLenghtWithHypothesis) aTrackCandidate = fitEventHypothesis(aTrackCandidate);

  ///TEST
  if(myMLTrackBuilder->isValid() && aTrackCandidate.getSegments().size()<0) {
    aTrackCandidate = Track3D();
    TrackSegment3D aSegment;
    aSegment.setGeometry(myGeometryPtr); 
    myMLTrackBuilder->run(myEventPtr);
    aSegment.setStartEnd(myMLTrackBuilder->getVertex(),
                          myMLTrackBuilder->getAlphaEnd());
    aSegment.setRecHits(myRecHits);                     
    aTrackCandidate.addSegment(aSegment);

    aSegment.setStartEnd(myMLTrackBuilder->getVertex(),
                          myMLTrackBuilder->getCarbonEnd());                     
    aTrackCandidate.addSegment(aSegment);    
  }
  ////

  GainTable::Entry gains;
  if(getRunGains(gains)) aTrackCandidate.setChargeGain(gains.gainCal);

  myFittedTrack = aTrackCandidate;

  std::cout<<KBLU<<"Reconstructed track: "<<RST<<std::endl;
  std::cout<<myEventPtr->GetEventInfo()<<std::endl;
  std::cout<<aTrackCandidate<<std::endl;
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
  
  const TAxis *axis = hProj.GetXaxis();
  const int nBins = hProj.GetNbinsX();
  const double threshold = 0.01;

  // only bins 1..nBins: under/overflow bins are not used
  double histoSum = 0.0;
  for(int iBin=1;iBin<=nBins;++iBin) histoSum += hProj.GetBinContent(iBin);
  if(!(histoSum>0.0)){ // empty projection: zero length range
    const double center = 0.5*(axis->GetXmin() + axis->GetXmax());
    return std::make_tuple(center, center);
  }

  int iBinStart = -1, iBinEnd = -1;
  double sum = 0.0;
  for(int iBin=1;iBin<=nBins;++iBin){
    sum += hProj.GetBinContent(iBin);
    if(iBinStart<0 && sum/histoSum>threshold) iBinStart = iBin;
    if(iBinEnd<0 && sum/histoSum>1.0-threshold){
      iBinEnd = iBin;
      break;
    }
  }
  if(iBinStart<0) iBinStart = 1;   // possible only with negative bin contents
  if(iBinEnd<0) iBinEnd = nBins;

  iBinStart = std::max(1, std::min(nBins, iBinStart-binMargin));
  iBinEnd = std::max(1, std::min(nBins, iBinEnd+binMargin));
  double start = axis->GetBinCenter(iBinStart);
  double end = axis->GetBinCenter(iBinEnd);
  if(start>end) start = end = 0.5*(start+end); // a negative margin can not make the range negative
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
const TH2D & TrackBuilder::getHoughTransform(int iDir) const{return myAccumulators[iDir];  }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TrackSegment2D & TrackBuilder::getSegment2D(int iDir, unsigned int iTrack) const{

  if(my2DSeeds[iDir].size()<=iTrack) return dummySegment2D;
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

   // the profile is registered in gDirectory under an event dependent name:
   // owned here, so that it is deleted on every return path
   std::unique_ptr<TH1D> hProfile;
   if(iProj==definitions::projection_type::DIR_TIME) hProfile.reset(getRecHits2D(iProjTmp).ProfileY());
   else hProfile.reset(getRecHits2D(iProjTmp).ProfileX());
   int firstBin = -1;
   int lastBin = -1;
   double value = 0.0;
  
   for(int iBin=1;iBin<=hProfile->GetNbinsX();++iBin){
     hProfile->SetBinError(iBin, 1.0);
     value = hProfile->GetBinContent(iBin);
     if(std::abs(value)>0.0){
       if(firstBin==-1) firstBin = iBin;
       lastBin = iBin;
     }
   }
   // at least two filled bins are needed for a linear fit
   if(firstBin==-1 || lastBin-firstBin<1) return 0.0;
   double minPos = hProfile->GetBinCenter(firstBin);
   double maxPos = hProfile->GetBinCenter(lastBin);

   auto aPol = std::make_unique<TF1>("aFit", "[0]+[1]*x", minPos, maxPos);
   aPol->SetParameter(0, (minPos+maxPos)/2.0);
   aPol->SetParameter(1, 0.0);
   TFitResultPtr aFitResult = hProfile->Fit(aPol.get(),"BRWWSQN"); // N: no graphics, would draw into gPad
   if(!aFitResult.Get() || aFitResult->NPar()<2) return 0.0;
   double slope = aFitResult->Parameter(1);
   double length = (maxPos - minPos)*slope;
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
    // temporary copy, deleted (and removed from gDirectory) at the end of the scope
    std::unique_ptr<TH2D> hTmp((TH2D*)getRecHits2D(iProjTmp).Clone("hTmp"));
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
  if(iProj==definitions::projection_type::DIR_TIME && length<minTimeProjLength){
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
  if(std::abs(maxTimeBin-minTimeBin)<minTimeProjLength){
    // the projection is registered in gDirectory under an event dependent name: delete it
    std::unique_ptr<TH1D> hStripProj(getRecHits2D(iProj).ProjectionY());
    iMaxBinY = hStripProj->GetMaximumBin();
  }
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

  // Mean of the crossing points of the strips at the charge maxima, for all
  // pairs of strip directions. Pairs are weighted with the product of their
  // projection lengths, so that longer projections dominate. For a dot (all
  // projection lengths equal to zero) the weighted mean is undefined:
  //  - acceptDot==false: a zero vector is returned (signals a dot, see fitTrack3D),
  //  - acceptDot==true: the crossing points are averaged with equal weights.
  // A zero vector is also returned if no pair of strips crosses.
  // iTrack2DSeed is not used: biases2D and lengths are already set for the seed
  // by getSignedLengthsAndMaxPos().
  (void)iTrack2DSeed;
  TVector2 bias2D, bias2D_mean;
  double biasZ = 0.0;
  bool result{false};
  double weight{0.0}, weightSum{0.0};
  TVector2 bias2D_unweighted;
  double biasZ_unweighted{0.0};
  int nCrossPoints{0};

  for(auto iDir_1: getProjectionsList()){
    for(auto iDir_2: getProjectionsList()){
      if(iDir_1==iDir_2) continue;

        result = myGeometryPtr->GetUVWCrossPointInMM(iDir_1, biases2D[iDir_1].Y(),
                                                      iDir_2, biases2D[iDir_2].Y(),
                                                      bias2D);
        if(!result) continue;

        weight = std::abs(lengths[iDir_1]*lengths[iDir_2]);
        weightSum += weight;

        bias2D_mean += bias2D*weight;
        biasZ += biases2D[iDir_1].X()*weight;

        bias2D_unweighted += bias2D;
        biasZ_unweighted += biases2D[iDir_1].X();
        ++nCrossPoints;
    }
   }

  if(weightSum>0.0){
    bias2D_mean = bias2D_mean/weightSum;
    biasZ /= weightSum;
  }
  else if(acceptDot && nCrossPoints>0){
    bias2D_mean = bias2D_unweighted/nCrossPoints;
    biasZ = biasZ_unweighted/nCrossPoints;
  }
  else return TVector3(0.0, 0.0, 0.0);

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

  // No projection on strip directions: the azimuthal angle is undefined.
  if(!(longProjLength>0.0)) return 0.0;

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

    // An alternative loss using cos(phi_dir-phi)/l_dir ratios was removed:
    // it was multiplied by 0, but for a zero projection length it is infinite,
    // and 0*inf = NaN made all losses NaN, so phi was always 0.
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
  double weight{0}, weightSum{0};
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
    if(weightSum>0.0) l_XY_mean /= weightSum;
    else l_XY_mean = 0.0;

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

  TrackSegment3D a3DSeed;
  a3DSeed.setGeometry(myGeometryPtr); 
  if(myMLTrackBuilder->isValid()) {
    myMLTrackBuilder->run(myEventPtr);
    a3DSeed.setBiasTangent(myMLTrackBuilder->getBias(),
                           myMLTrackBuilder->getTangent());   
    a3DSeed.setRecHits(myRecHits);                  
    return a3DSeed;
  }

  getSignedLengthsAndMaxPos(iTrack2DSeed);

  TVector3 aBias = getBias(iTrack2DSeed);
  TVector3 aTangent = getTangent(iTrack2DSeed, guiMode); 
  a3DSeed.setBiasTangent(aBias, aTangent);
  a3DSeed.setRecHits(myRecHits);

  if(guiMode) return a3DSeed;

  ///FIX ME Stupid workaround for track direction ambiguity
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
  // second direction fit, with the fitted bias: small rotations about the current direction
  fitTrack3DInSelectedDir(aFittedTrack, definitions::fit_type::TANGENT_ROTATION);
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
  double rotationRange = 0.4; //[rad] full range of the rotation angles, parameter to be moved to configuration

  // the FCN refers to a member, not to a local copy: the returned FitResult
  // keeps a copy of the FCN, which must not point to a destroyed object
  myFitCandidate = aTrack;
  Track3D & aTrackCandidate = myFitCandidate;
  aTrackCandidate.setFitMode(fitType);

  // Two fit parameters of the (only) fitted segment, read by
  // Track3D::updateAndGetLoss as par[0], par[1]: start value, initial step
  // size and allowed range (centred at the start value) for each fit mode.
  struct Parameter { double value, step, range; bool fixed; };
  const TVector3 & bias = aTrackCandidate.getSegments().front().getBias();
  const TVector3 & tangent = aTrackCandidate.getSegments().front().getTangent();
  std::array<Parameter, 2> pars;
  switch(fitType){
  case definitions::fit_type::BIAS_Z:
    pars = {{ {bias.Z(), zRange/2, zRange, false},
              {0.0, 0.0, 0.0, true} }};
    break;
  case definitions::fit_type::BIAS_XY:
    pars = {{ {bias.X(), xyRange/2, xyRange, false},
              {bias.Y(), xyRange/2, xyRange, false} }};
    break;
  case definitions::fit_type::TANGENT:
    pars = {{ {tangent.Theta(), thetaRange/2, thetaRange, false},
              {tangent.Phi(), phiRange/2, phiRange, false} }};
    break;
  case definitions::fit_type::TANGENT_ROTATION: // rotation angles about the current tangent
    pars = {{ {0.0, rotationRange/8, rotationRange, false},
              {0.0, rotationRange/8, rotationRange, false} }};
    break;
  default:
    throw std::invalid_argument("TrackBuilder::fitTrackNodesBiasTangent: unsupported fit type");
  }

  const int nParams = 2;
  double startValues[nParams] = {pars[0].value, pars[1].value};
  ROOT::Math::Functor fcn(&aTrackCandidate, &Track3D::updateAndGetLoss, nParams);
  fitter.SetFCN(fcn, startValues);
  for(int iPar=0;iPar<nParams;++iPar){
    auto & settings = fitter.Config().ParSettings(iPar);
    settings.SetValue(pars[iPar].value);
    if(pars[iPar].fixed){
      settings.Fix();
      continue;
    }
    settings.SetStepSize(pars[iPar].step);
    settings.SetLimits(pars[iPar].value - pars[iPar].range/2, pars[iPar].value + pars[iPar].range/2);
    settings.Release();
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

  // vertex position along the track from the ML model, if available,
  // otherwise no constraint
  bool hasMLVertex = false;
  double mlVertexOffset = 0.0;
  if(myMLTrackBuilder->isValid()) {
    myMLTrackBuilder->run(myEventPtr);
    mlVertexOffset = (myMLTrackBuilder->getCarbonEnd() - myMLTrackBuilder->getVertex()).Mag();
    hasMLVertex = true;
  }
  auto setDefaultVertexConstraint = [&](){
    if(hasMLVertex) mydEdxFitter.setVertexConstraint(mlVertexOffset);
    else mydEdxFitter.clearVertexConstraint();
  };
  setDefaultVertexConstraint();

  double trackWidth = aSegment.getMaxProjWidth();
  double centralD = trackWidth/0.76; //parameter to be moved to configuration
  double minD = centralD*0.5; //parameter to be moved to configuration
  double maxD = centralD*1.5; //parameter to be moved to configuration       
  // no projection with hits close to the track: default diffusion range
  // (a zero range would mean no limits on the diffusion in the fit)
  if(!(trackWidth>0.0)){
    minD = 0.5;
    maxD = 1.5;
  }
  mydEdxFitter.setDiffusionRange(minD, maxD);
  mydEdxFitter.setOrientation(-1);
  mydEdxFitter.fitHisto(hChargeProfile);
  HypothesisFit best = getHypothesisFit();

  // no hypothesis fitted: keep the fitted track instead of a zero length alpha segment
  if(best.eventType==pid_type::UNKNOWN){
    std::cout<<KRED<<"dE/dx fit failed: "<<RST<<"event hypothesis not fitted."<<std::endl;
    return aTrackCandidate;
  }

  // R = Q/(G_raw E_dep) with E_dep from the fitted ranges. The dE/dx fit can end in the wrong
  // alpha-C orientation (vertex next to the alpha end, the alpha Bragg peak taken as the carbon),
  // whose E_dep is too low. Refit the other orientation, with the vertex seeded at the mirrored
  // position (one carbon range from the current alpha end), and keep the fit with |R-1| smaller.
  double chargeRatio = 0.0;
  GainTable::Entry gains;
  if(best.eventType==C12_ALPHA && getRunGains(gains)){
    const double charge = aSegment.getIntegratedCharge(aSegment.getLength());
    chargeRatio = getChargeRatio(charge, gains.gainRaw, best);
    if(dEdxOrientationByR && chargeRatio>0){
      // profile coordinates: x in the reflected profile = length - x in the profile as given
      const double length = aSegment.getLength();
      const double altVertexOffset = length - (best.vertexOffset + best.alphaRange) + best.carbonRange;
      mydEdxFitter.setOrientation(best.isReflected ? 0 : 1);
      mydEdxFitter.setVertexConstraint(altVertexOffset, dEdxOrientationVertexWindow);
      mydEdxFitter.fitHisto(hChargeProfile);
      HypothesisFit alt = getHypothesisFit();
      mydEdxFitter.setOrientation(-1);
      setDefaultVertexConstraint();
      const double altRatio = alt.eventType==C12_ALPHA ? getChargeRatio(charge, gains.gainRaw, alt) : 0.0;
      const bool useAlt = altRatio>0 && std::abs(altRatio-1)<std::abs(chargeRatio-1);
      std::cout<<KBLU<<"dE/dx orientation by R: "<<RST
               <<"R = "<<chargeRatio<<" (loss "<<best.loss<<", alpha "<<best.alphaRange<<" mm, carbon "<<best.carbonRange<<" mm), "
               <<"other orientation R = "<<altRatio<<" (loss "<<alt.loss<<", alpha "<<alt.alphaRange<<" mm, carbon "<<alt.carbonRange<<" mm)"
               <<(useAlt ? " -> other orientation" : " -> kept")<<std::endl;
      if(useAlt){
        best = alt;
        chargeRatio = altRatio;
      }
    }
  }

  const pid_type eventType = best.eventType;
  const double vertexOffset = best.vertexOffset;
  const double alphaRange = best.alphaRange;
  const double carbonRange = best.carbonRange;

  TVector3 tangent =  aSegment.getTangent();
  TVector3 start =  aSegment.getStart();
  
  if(best.isReflected){
    tangent *=-1;
    start = aSegment.getEnd();
  }

  TVector3 vertexPos =  start + vertexOffset*tangent;
  TVector3 alphaEnd =  vertexPos + alphaRange*tangent;
  TVector3 carbonEnd =  vertexPos - carbonRange*tangent;

  Track3D aSplitTrackCandidate;
  aSplitTrackCandidate.setChargeProfile(best.fittedHisto);
  aSplitTrackCandidate.setHypothesisFitLoss(best.loss);
  aSplitTrackCandidate.setChargeRatio(chargeRatio);
  TrackSegment3D alphaSegment;
  alphaSegment.setGeometry(myGeometryPtr);  
  alphaSegment.setStartEnd(vertexPos, alphaEnd);
  alphaSegment.setRecHits(myRawHits);
  alphaSegment.setDiffusion(best.diffusion);
  alphaSegment.setPID(pid_type::ALPHA);
  aSplitTrackCandidate.addSegment(alphaSegment);

  std::cout<<KGRN<<"Fitted event hypothesis: "
           <<eventType
           <<" with dE/dx diffusion: "<<best.diffusion
           <<" mm/sqrt(cm), vertex offset: "<<vertexOffset
           <<" mm, alpha range: "<<alphaRange
           <<" mm, carbon range: "<<carbonRange
           <<", fit loss: "<<best.loss;
  if(eventType==C12_ALPHA){
    std::cout<<", kinematics: "<<(best.kinematics==dEdxFitter::Kinematics::O16 ? "O16" :
                                  best.kinematics==dEdxFitter::Kinematics::O18 ? "O18" : "free");
  }
  std::cout<<RST<<std::endl;
  
  if(eventType==C12_ALPHA){   
    TrackSegment3D carbonSegment;
    carbonSegment.setGeometry(myGeometryPtr);  
    carbonSegment.setStartEnd(vertexPos, carbonEnd);
    carbonSegment.setRecHits(myRawHits);
    carbonSegment.setDiffusion(best.diffusion);
    carbonSegment.setPID(best.recoilPID); // CARBON_12 or CARBON_14 (O18 kinematics)
    aSplitTrackCandidate.addSegment(carbonSegment);
  }
  return aSplitTrackCandidate;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackBuilder::HypothesisFit TrackBuilder::getHypothesisFit() const {
  HypothesisFit aFit;
  aFit.eventType = mydEdxFitter.getBestFitEventType();
  if(aFit.eventType==pid_type::UNKNOWN) return aFit;
  aFit.isReflected = mydEdxFitter.getIsReflected();
  aFit.vertexOffset = mydEdxFitter.getVertexOffset();
  aFit.alphaRange = mydEdxFitter.getAlphaRange();
  aFit.carbonRange = mydEdxFitter.getCarbonRange();
  aFit.loss = mydEdxFitter.getLoss();
  aFit.diffusion = mydEdxFitter.getDiffusion();
  aFit.kinematics = mydEdxFitter.getKinematics();
  aFit.recoilPID = mydEdxFitter.getRecoilPID();
  aFit.fittedHisto = mydEdxFitter.getFittedHisto();
  aFit.fittedHisto.SetDirectory(0);
  return aFit;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
