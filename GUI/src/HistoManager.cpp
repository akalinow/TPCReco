#include <cstdlib>
#include <iostream>

#include "TCanvas.h"
#include "TH2D.h"
#include "TH3D.h"
#include "TSpectrum2.h"
#include "TVector3.h"

#include "GeometryTPC.h"
#include "EventTPC.h"

#include "HistoManager.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
HistoManager::HistoManager() {

  myEvent = 0;

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
HistoManager::~HistoManager() {

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr){
  
  myGeometryPtr = aGeometryPtr;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::setEvent(EventTPC* aEvent){

  myEvent = aEvent;

  myTkBuilder.setEvent(aEvent);
  myTkBuilder.reconstruct();
  
  double eventMaxCharge = aEvent->GetMaxCharge();
  double chargeThreshold = 0.1*eventMaxCharge;
  int delta_timecells = 1;
  int delta_strips = 1;

  aCluster = aEvent->GetOneCluster(chargeThreshold, delta_strips, delta_timecells);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::getRawStripVsTime(int aDir){

  std::shared_ptr<TH2D> hProjection = myEvent->GetStripVsTime(aDir);
  double varianceX = hProjection->GetCovariance(1, 1);
  double varianceY = hProjection->GetCovariance(2, 2);
  double varianceXY = hProjection->GetCovariance(1, 2);

  std::vector<int> nStrips = {72, 92, 92};
  
  std::cout<<" varianceX*12: "<<varianceX*12/450/450
	   <<" varianceY*12: "<<varianceY*12/nStrips[aDir]/nStrips[aDir]
	   <<" varianceXY: "<<varianceXY
	   <<std::endl;
  
  return hProjection;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::getFilteredStripVsTime(int aDir){

  return myEvent->GetStripVsTime(aCluster, aDir);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TH3D* HistoManager::get3DReconstruction(){

  double radius = 2.0;
  int rebin_space=EVENTTPC_DEFAULT_STRIP_REBIN;
  int rebin_time=EVENTTPC_DEFAULT_TIME_REBIN; 
  int method=EVENTTPC_DEFAULT_RECO_METHOD;
  h3DReco = myEvent->Get3D(aCluster,  radius, rebin_space, rebin_time, method);
  return h3DReco;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TH2D & HistoManager::getHoughAccumulator(int aDir, int iPeak){

  return myTkBuilder.getHoughtTransform(aDir);

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TLine HistoManager::getTrack2D(int aDir){

  const Track3D & aTrack2DProjection = myTkBuilder.getTrack2D(aDir);
  const TVector3 & bias = aTrack2DProjection.getBias();
  const TVector3 & tangent = aTrack2DProjection.getTangent();
  double lambdaMax = aTrack2DProjection.getLength();

  double xBegin = (bias+tangent).X();
  double yBegin = (bias+tangent).Y();

  double xEnd = (bias+lambdaMax*tangent).X();
  double yEnd = (bias+lambdaMax*tangent).Y();
  
  TLine aTrackLine(xBegin, yBegin, xEnd, yEnd);
  aTrackLine.SetLineColor(2);
  aTrackLine.SetLineWidth(2);

  return aTrackLine;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TLine HistoManager::getTrack3DProjection(int aDir){

  TVector3 stripPitchDirection(cos(phiPitchDirection[aDir]),
			       sin(phiPitchDirection[aDir]), 0);
  
  const Track3D & aTrack3D = myTkBuilder.getTrack3D();
  const TVector3 & bias = aTrack3D.getBias();
  const TVector3 & tangent = aTrack3D.getTangent();
  double lambdaMax = aTrack3D.getLength();
  
  double lambda = 0;
  TVector3 aPointOnLine;

  lambda = 0;
  aPointOnLine = bias + lambda*tangent;
  double xStart = aPointOnLine.Z();
  double yStart = aPointOnLine*stripPitchDirection - stripOffset[aDir];

  lambda = lambdaMax;
  aPointOnLine = bias + lambda*tangent;
  double xEnd = aPointOnLine.Z();
  double yEnd = aPointOnLine*stripPitchDirection - stripOffset[aDir];

  TLine aProjection(xStart, yStart, xEnd, yEnd);
  aProjection.SetLineColor(4);
  aProjection.SetLineWidth(2);

  return aProjection;  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

