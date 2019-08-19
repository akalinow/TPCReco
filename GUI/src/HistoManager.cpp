#include <cstdlib>
#include <iostream>
#include <tuple>

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
std::shared_ptr<TH2D> HistoManager::getRecHitStripVsTime(int aDir){

  return std::shared_ptr<TH2D>(new TH2D(myTkBuilder.getRecHits2D(aDir)));//FIX ME avoid object copying

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
TLine HistoManager::getTrack2D(int aDir, int iTrack){

  const Track3D & aTrack2DProjection = myTkBuilder.getTrack2D(aDir, iTrack);
  const TVector3 & bias = aTrack2DProjection.getBiasAtX0();
  const TVector3 & tangent = aTrack2DProjection.getTangent();

  double startTime = aTrack2DProjection.getStartTime();
  double endTime = aTrack2DProjection.getEndTime();

  double lambda = startTime;
  double xBegin = (bias+lambda*tangent).X();
  double yBegin = (bias+lambda*tangent).Y();

  lambda = endTime;
  double xEnd = (bias+lambda*tangent).X();
  double yEnd = (bias+lambda*tangent).Y();
  
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
  const TVector3 & bias = aTrack3D.getBiasAtZ0();
  const TVector3 & tangent = aTrack3D.getTangent();

  double startTime = 0.0, endTime = 300.0;
  
  double lambda = startTime;
  TVector3 aPointOnLine;

  lambda = startTime;
  aPointOnLine = bias + lambda*tangent;
  double xStart = aPointOnLine.Z();
  double yStart = aPointOnLine*stripPitchDirection - stripOffset[aDir];

  lambda = endTime;
  aPointOnLine = bias + lambda*tangent;
  double xEnd = aPointOnLine.Z();
  double yEnd = aPointOnLine*stripPitchDirection - stripOffset[aDir];

  TLine aProjection(xStart, yStart, xEnd, yEnd);
  aProjection.SetLineColor(4);
  aProjection.SetLineWidth(2);

  std::cout<<" aDir: "<<aDir
	   <<" xStart: "<<xStart
	   <<" xEnd: "<<xEnd
	   <<" yStart: "<<yStart
	   <<" yEnd: "<<yEnd
	   <<std::endl;

  return aProjection;  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TH1D HistoManager::getChargeAlong2DTrack(int aDir){

  //std::shared_ptr<TH2D> hProjection = myEvent->GetStripVsTime(aDir);
  std::shared_ptr<TH2D> hProjection = getRecHitStripVsTime(aDir);
  const Track3D & aTrack2DProjection = myTkBuilder.getTrack2D(aDir);
  const TVector3 & bias = aTrack2DProjection.getBiasAtX0();
  const TVector3 & tangent = aTrack2DProjection.getTangent();

  TH1D hCharge("hCharge","Charge along track segment [arb. units]",20,
	       aTrack2DProjection.getStartTime(),
	       aTrack2DProjection.getEndTime());

  double x=0, y=0;
  double charge = 0.0;
  double lambda = 0.0;
  double value = 0.0;
  int sign = 0.0;
  TVector3 aPoint;
  TVector3 d;
  
  for(int iBinX=1;iBinX<hProjection->GetNbinsX();++iBinX){
    for(int iBinY=1;iBinY<hProjection->GetNbinsY();++iBinY){
      x = hProjection->GetXaxis()->GetBinCenter(iBinX);
      y = hProjection->GetYaxis()->GetBinCenter(iBinY);
      charge = hProjection->GetBinContent(iBinX, iBinY);
      if(charge<100) continue;
      aPoint.SetXYZ(x, y, 0.0);
      lambda = (aPoint - bias)*tangent/tangent.Mag2();      
      d = aPoint - bias - lambda*tangent;
      if(d.Mag()>5) continue;
      sign = -1 + 2*(tangent.Cross(d).Z()>0);
      //value = sign*sign/(d.Mag() + 0.001);
      //value = charge*d.Mag()*sign*sign;
      value = charge*sign*sign;
      //value = d.Mag();
      hCharge.Fill(lambda, value);
    }
  }

  //hCharge.Smooth();
  return hCharge;
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
