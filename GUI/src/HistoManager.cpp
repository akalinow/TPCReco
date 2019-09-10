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
TH2Poly * HistoManager::getDetectorLayout() const{

  if(!myGeometryPtr) return 0;


  TH2Poly* aPtr = (TH2Poly*)myGeometryPtr->GetTH2Poly()->Clone();
  int nBins = aPtr->GetNumberOfBins(); 
  for(int iBin=1;iBin<nBins;iBin+=nBins/50){
    aPtr->SetBinContent(iBin, 1.0);
  }  
  return aPtr;
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
void HistoManager::drawTrack3DProjectionXY(TVirtualPad *aPad){

  aPad->cd();
  const Track3D & aTrack3D = myTkBuilder.getTrack3D(0);

  int iSegment = 0;
  TLine aSegment2DLine;
  aSegment2DLine.SetLineWidth(2);
  for(const auto & aItem: aTrack3D.getSegments()){
    const TVector3 & start = aItem.getStart();
    const TVector3 & end = aItem.getEnd();
    aSegment2DLine.SetLineColor(2+iSegment);
    aSegment2DLine.DrawLine(start.X(), start.Y(),  end.X(),  end.Y());	
    ++iSegment;
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawTrack3DProjectionTimeStrip(int strip_dir, TVirtualPad *aPad){

  aPad->cd();
  const Track3D & aTrack3D = myTkBuilder.getTrack3D(0);

  int iSegment = 0;
  TLine aSegment2DLine;
  aSegment2DLine.SetLineWidth(2);
  for(const auto & aItem: aTrack3D.getSegments()){
    const TrackSegment2D & aSegment2DProjection = aItem.get2DProjection(strip_dir, 0, aItem.getLength());
    const TVector3 & start = aSegment2DProjection.getStart();
    const TVector3 & end = aSegment2DProjection.getEnd();
    aSegment2DLine.SetLineColor(2+iSegment);
    aSegment2DLine.DrawLine(start.X(), start.Y(),  end.X(),  end.Y());	
    ++iSegment;
  }
  

  /*
  if(aDir==DIR_U){

    aTrack3D.getStart().Print();
    //aTrack3D.getBias().Print();
    aTrack3D.getEnd().Print();
    
    start.Print();
    end.Print();
    }*/
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawChargeAlongTrack3D(TVirtualPad *aPad){

  const Track3D & aTrack3D = myTkBuilder.getTrack3D(0);

  aPad->cd();
  TGraph aGr = aTrack3D.getChargeProfile();
  aGr.SetTitle("Charge distribution along track.;d[track length];charge[arbitrary units]");
  aGr.SetLineWidth(2);
  aGr.SetLineColor(2);
  aGr.DrawClone("AL");
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
