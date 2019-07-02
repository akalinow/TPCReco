#include <cstdlib>
#include <iostream>

#include "TVector3.h"

#include "GeometryTPC.h"
#include "EventTPC.h"

#include "TrackBuilder.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackBuilder::TrackBuilder() {

  myEvent = 0;

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
 
  if(!myAccumulators.size()){

    int nRhoBins = 100;//FIX ME move to configuarable
    int nPhiBins = 100;//FIX ME move to configuarable
     
    for(int iDir = 0; iDir<3;++iDir){
      std::shared_ptr<TH2D> hProjection = myEvent->GetStripVsTime(iDir);
      double maxX = hProjection->GetXaxis()->GetXmax();
      double maxY = hProjection->GetYaxis()->GetXmax();
      double rho = sqrt( maxX*maxX + maxY*maxY);
      hName = "hAccumulator_"+std::to_string(iDir);
      hTitle = "Hough accumulator for direction: "+std::to_string(iDir)+";#theta;#rho";
      TH2D hAccumulator(hName.c_str(), hTitle.c_str(), nPhiBins, -M_PI, M_PI, nRhoBins, 0, rho);   
      myAccumulators.push_back(hAccumulator);
    }
  } 
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::reconstruct(){

  myTrack2DProjections.clear();
  for(int iDir=0;iDir<3;++iDir){
    fillHoughAccumulator(iDir);
    myTrack2DProjections.push_back(findTrack2D(iDir, 0));
  }

  myTrack3DSeed = buildTrack3D();
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TH2D & TrackBuilder::getHoughtTransform(int iDir) const{

  return myAccumulators[iDir];
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const Track3D & TrackBuilder::getTrack2D(int iDir) const{

  return myTrack2DProjections[iDir];
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const Track3D & TrackBuilder::getTrack3D() const{

  return myTrack3DSeed;
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackBuilder::fillHoughAccumulator(int iDir){

  std::shared_ptr<TH2D> hProjection = myEvent->GetStripVsTime(iDir);
  myAccumulators[iDir].Reset();
  
  double theta = 0.0, rho = 0.0;
  int chargeTreshold = 10;//FIX ME move to configuarable
  for(int iBinX=0;iBinX<hProjection->GetNbinsX();++iBinX){
    for(int iBinY=0;iBinY<hProjection->GetNbinsY();++iBinY){      
      int charge = hProjection->GetBinContent(iBinX, iBinY);
      for(int iBinTheta=1;iBinTheta<myAccumulators[iDir].GetNbinsX();++iBinTheta){
	theta = myAccumulators[iDir].GetXaxis()->GetBinCenter(iBinTheta);
	rho = iBinX*cos(theta) + iBinY*sin(theta);
	if(charge<chargeTreshold) continue;
	myAccumulators[iDir].Fill(theta, rho, charge);
      }
    }
  }  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Track3D TrackBuilder::findTrack2D(int iDir, int iPeak) const{
  
  int iBinX = 0, iBinY = 0, iBinZ=0;
  const TH2D & hAccumulator = myAccumulators[iDir];

  myAccumulators[iDir].GetMaximumBin(iBinX, iBinY, iBinZ);
  /*
  for(int aPeak=0;aPeak<iPeak;++aPeak){
    hAccumulator.SetBinContent(iBinX, iBinY, 0,0);
    hAccumulator.GetMaximumBin(iBinX, iBinY, iBinZ);
    }*/

  TVector3 aTangent, aBias;
  double theta = hAccumulator.GetXaxis()->GetBinCenter(iBinX);
  double rho = hAccumulator.GetYaxis()->GetBinCenter(iBinY);
  double aX = rho*cos(theta);
  double aY = rho*sin(theta);
  aBias.SetXYZ(aX, aY, 0.0);

  aX = -rho*sin(theta);
  aY = rho*cos(theta);
  double norm = sqrt(aX*aX + aY*aY);
  aX /= norm;
  aY /= norm;
  aTangent.SetXYZ(aX, aY, 0.0);
  
  ///Set tangent direction along time arrow
  if(aTangent.X()<0) aTangent *= -1;
  //FIX ME: aTangent.X()!=0 !!!
  aTangent *= 1.0/aTangent.X();
  
  ///Move seed bias so lambda=0 correspond to t=0
  ///so biases from different projections can be compared
  ///FIX ME: seedTangent.X()==0 !!!
  double lambda = -aBias.X()/aTangent.X();
  aBias += lambda*aTangent;

  Track3D a2DSeed(aTangent, aBias, 500);

  return a2DSeed;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Track3D TrackBuilder::buildTrack3D() const{

  myTrack2DProjections[0].getTangent();

  double bZ = myTrack2DProjections[0].getBias().X();  
  double bX = (myTrack2DProjections[0].getBias().Y() + stripOffset[0])*cos(phiPitchDirection[0]);
  double bY_fromV = (myTrack2DProjections[1].getBias().Y()+stripOffset[1] -bX*cos(phiPitchDirection[1]))/sin(phiPitchDirection[1]);
  double bY_fromW = (myTrack2DProjections[2].getBias().Y()+stripOffset[2] -bX*cos(phiPitchDirection[2]))/sin(phiPitchDirection[2]);
  double bY = (bY_fromV + bY_fromW)/2.0;
  TVector3 aBias(bX, bY, bZ);

  double tZ = myTrack2DProjections[0].getTangent().X();
  double tX = myTrack2DProjections[0].getTangent().Y()*cos(phiPitchDirection[0]);
  double tY_fromV = (myTrack2DProjections[1].getTangent().Y() - tX*cos(phiPitchDirection[1]))/sin(phiPitchDirection[1]);
  double tY_fromW = (myTrack2DProjections[2].getTangent().Y() - tX*cos(phiPitchDirection[2]))/sin(phiPitchDirection[2]);
  double tY = (tY_fromV + tY_fromW)/2.0;
  TVector3 aTangent(tX, tY, tZ);

  Track3D a3DSeed(aTangent, aBias, 500);

  return a3DSeed;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
