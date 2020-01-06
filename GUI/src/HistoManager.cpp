#include <cstdlib>
#include <iostream>
#include <tuple>

#include "TCanvas.h"
#include "TH2D.h"
#include "TH3D.h"
#include "TSpectrum2.h"
#include "TVector3.h"
#include "TPolyLine3D.h"
#include "TView.h"

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
void HistoManager::setEvent(std::shared_ptr<EventTPC> aEvent){

  myEvent = aEvent;

  myTkBuilder.setEvent(aEvent);
  myTkBuilder.reconstruct();
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::getCartesianProjection(projection strip_dir){

  return myEvent->GetStripVsTimeInMM(myTkBuilder.getCluster(), strip_dir);
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TH2Poly * HistoManager::getDetectorLayout() const{

  if(!myGeometryPtr) return nullptr;

  TH2Poly* aPtr = (TH2Poly*)myGeometryPtr->GetTH2Poly()->Clone();
  int nBins = aPtr->GetNumberOfBins(); 
  for(int iBin=1;iBin<nBins;iBin+=nBins/50){
    aPtr->SetBinContent(iBin, 1.0);
  }  
  return aPtr;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::getRawStripVsTime(projection strip_dir){

  std::shared_ptr<TH2D> hProjection = myEvent->GetStripVsTime(strip_dir);
  double varianceX = hProjection->GetCovariance(1, 1);
  double varianceY = hProjection->GetCovariance(2, 2);
  double varianceXY = hProjection->GetCovariance(1, 2);

  std::vector<int> nStrips = {72, 92, 92};
  
  std::cout<<" varianceX*12: "<<varianceX*12/450/450
	   <<" varianceY*12: "<<varianceY*12/nStrips[int(strip_dir)]/nStrips[int(strip_dir)]
	   <<" varianceXY: "<<varianceXY
	   <<std::endl;
  
  return hProjection;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::getRecHitStripVsTime(projection strip_dir){

  return std::make_shared<TH2D>(myTkBuilder.getRecHits2D(int(strip_dir)));//FIX ME avoid object copying

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Reconstr_hist HistoManager::getReconstruction(bool force) {
    if (!reconstruction_done || force) {
        double radius = 2.0;
        int rebin_space = EVENTTPC_DEFAULT_STRIP_REBIN;
        int rebin_time = EVENTTPC_DEFAULT_TIME_REBIN;
        int method = EVENTTPC_DEFAULT_RECO_METHOD;
        reconstruction = myEvent->Get(myTkBuilder.getCluster(), radius, rebin_space, rebin_time, method);
        reconstruction_done = true;
    }
    return reconstruction;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH3D> HistoManager::get3DReconstruction(bool force){
  return getReconstruction(force).second;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::get2DReconstruction(projection strip_dir, bool force) {
  return getReconstruction(force).first[strip_dir];
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TH2D & HistoManager::getHoughAccumulator(projection strip_dir, int iPeak){

  return myTkBuilder.getHoughtTransform(int(strip_dir));

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawTrack3D(TVirtualPad *aPad){

  aPad->cd();
  const Track3D & aTrack3D = myTkBuilder.getTrack3D(0);
  const TrackSegment3DCollection & trackSegments = aTrack3D.getSegments();
  if(trackSegments.size() == 0) return;
  
  TPolyLine3D aPolyLine;
  aPolyLine.SetLineWidth(2);
  aPolyLine.SetLineColor(2);

  aPolyLine.SetPoint(0,
		     trackSegments.front().getStart().X(),
		     trackSegments.front().getStart().Y(),
		     trackSegments.front().getStart().Z());
  
   for(auto& aSegment: trackSegments){
     aPolyLine.SetPoint(aPolyLine.GetLastPoint()+1,
			aSegment.getEnd().X(),
			aSegment.getEnd().Y(),
			aSegment.getEnd().Z());     
   }
   aPolyLine.DrawClone();
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
void HistoManager::drawTrack2DSeed(projection strip_dir, TVirtualPad *aPad){

  const TrackSegment2D & aSegment2D = myTkBuilder.getSegment2D(int(strip_dir));
  const TVector3 & start = aSegment2D.getStart();
  const TVector3 & end = aSegment2D.getEnd();

  TLine aSegment2DLine;
  aSegment2DLine.SetLineWidth(2);
  aSegment2DLine.SetLineColor(2);
  aPad->cd();
  aSegment2DLine.DrawLine(start.X(), start.Y(),  end.X(),  end.Y());	  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawTrack3DProjectionTimeStrip(projection strip_dir, TVirtualPad *aPad){

  aPad->cd();
  const Track3D & aTrack3D = myTkBuilder.getTrack3D(0);

  int iSegment = 0;
  TLine aSegment2DLine;
  aSegment2DLine.SetLineWidth(2);
  double minX = 999.0, minY = 999.0;
  double maxX = -999.0, maxY = -999.0;

  for(const auto & aItem: aTrack3D.getSegments()){
    const TrackSegment2D & aSegment2DProjection = aItem.get2DProjection(strip_dir, 0, aItem.getLength());
    const TVector3 & start = aSegment2DProjection.getStart();
    const TVector3 & end = aSegment2DProjection.getEnd();
    aSegment2DLine.SetLineColor(2+iSegment);
    aSegment2DLine.DrawLine(start.X(), start.Y(),  end.X(),  end.Y());	
    ++iSegment;
    
    minY = std::min({ minY, start.Y(), end.Y() });
    maxY = std::max({ maxY, start.Y(), end.Y() });
    minX = std::min({ minX, start.X(), end.X() });
    maxX = std::max({ maxX, start.X(), end.X() }); 
  }
  minX -=5;
  minY -=5;
  
  double delta = std::max( std::abs(maxX - minX),
			   std::abs(maxY - minY));
  maxX = minX + delta;
  maxY = minY + delta;

  TH2D *hFrame = (TH2D*)aPad->GetListOfPrimitives()->At(0);
  if(hFrame != nullptr){
    hFrame->GetXaxis()->SetRangeUser(minX, maxX);
    hFrame->GetYaxis()->SetRangeUser(minY, maxY);
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawChargeAlongTrack3D(TVirtualPad *aPad) const{

  const Track3D & aTrack3D = myTkBuilder.getTrack3D(0);

  aPad->cd();
  TGraph aGr = aTrack3D.getChargeProfile();
  //TGraph aGr = aTrack3D.getHitDistanceProfile();
  aGr.SetTitle("Charge distribution along track.;d[track length];charge[arbitrary units]");
  aGr.SetLineWidth(2);
  aGr.SetLineColor(2);
  aGr.DrawClone("AL");
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
