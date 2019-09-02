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
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::getCartesianProjection(int aDir){

  return myEvent->GetStripVsTimeInMM(myTkBuilder.getCluster(), aDir);
  
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

  return myEvent->GetStripVsTime(myTkBuilder.getCluster(), aDir);
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
  h3DReco = myEvent->Get3D(myTkBuilder.getCluster(),  radius, rebin_space, rebin_time, method);
  return h3DReco;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TH2D & HistoManager::getHoughAccumulator(int aDir, int iPeak){

  return myTkBuilder.getHoughtTransform(aDir);

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TLine HistoManager::get2DLine(int aDir, int iTrack){

  //const TrackSegment2D & aTrack2DProjection = myTkBuilder.getSegment2D(aDir, iTrack);

  //const TrackSegment3D & aTrack3D = myTkBuilder.getSegment3DSeed();
  const TrackSegment3D & aTrack3D = myTkBuilder.getSegment3DFitted(iTrack);
  const TrackSegment2D & aTrack2DProjection = aTrack3D.get2DProjection(aDir);
  
  const TVector3 & start = aTrack2DProjection.getStart();
  const TVector3 & end = aTrack2DProjection.getEnd();

  if(aDir==DIR_U){

    aTrack3D.getStart().Print();
    //aTrack3D.getBias().Print();
    aTrack3D.getEnd().Print();
    
    start.Print();
    end.Print();
  }

  double xBegin = start.X();
  double yBegin = start.Y();

  double xEnd =  end.X();
  double yEnd =  end.Y();
  
  TLine aTrackLine(xBegin, yBegin, xEnd, yEnd);
  aTrackLine.SetLineColor(2+iTrack);
  aTrackLine.SetLineWidth(2);

  return aTrackLine;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
