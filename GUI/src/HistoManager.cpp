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
#include "colorText.h"

#include "HistoManager.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
HistoManager::HistoManager() {

  myEvent = 0;
  doAutozoom = false;

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
HistoManager::~HistoManager() { }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr){
  
  myGeometryPtr = aGeometryPtr;
  myTkBuilder.setGeometry(aGeometryPtr);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::setEvent(EventTPC* aEvent){
  if(!aEvent) return;
  myEvent.reset(aEvent);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::setEvent(std::shared_ptr<EventTPC> aEvent){
  if(!aEvent) return;
  myEvent = aEvent;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::reconstruct(){
  myTkBuilder.setEvent(myEvent);
  //myTkBuilder.reconstruct();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::reconstructSegmentsFromMarkers(std::vector<double> * segmentsXY){

  myTkBuilder.getSegment2DCollectionFromGUI(*segmentsXY);  
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
std::shared_ptr<TH1D> HistoManager::getRawTimeProjection(){
  TH1D *h = myEvent->GetTimeProjection();
  h->GetYaxis()->SetTitleOffset(2.0);
  return std::shared_ptr<TH1D>(h);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH1D> HistoManager::getRawStripProjection(int strip_dir){

  return  std::shared_ptr<TH1D>(myEvent->GetStripProjection(strip_dir));
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::getRawStripVsTime(int strip_dir){

  std::shared_ptr<TH2D> aHisto = myEvent->GetStripVsTime(strip_dir);  
  if(doAutozoom) makeAutozoom(aHisto);
  aHisto->GetYaxis()->SetTitleOffset(1.8);
  aHisto->GetZaxis()->SetTitleOffset(1.5);
  return aHisto;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::getRawStripVsTimeInMM(int strip_dir){

  std::shared_ptr<TH2D> aHisto = myEvent->GetStripVsTimeInMM(myTkBuilder.getCluster(), strip_dir);
  aHisto->GetYaxis()->SetTitleOffset(1.8);
  aHisto->GetZaxis()->SetTitleOffset(1.5);
  if(doAutozoom) makeAutozoom(aHisto);
  return aHisto;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::getFilteredStripVsTime(int strip_dir){

  std::shared_ptr<TH2D> aHisto = myEvent->GetStripVsTime(myTkBuilder.getCluster(), strip_dir);
  aHisto->GetYaxis()->SetTitleOffset(1.8);
  aHisto->GetZaxis()->SetTitleOffset(1.5);
  if(doAutozoom) makeAutozoom(aHisto);
  return aHisto;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::getRecHitStripVsTime(int strip_dir){

  TH2D *h = (TH2D*)myTkBuilder.getRecHits2D(strip_dir).Clone();
  std::shared_ptr<TH2D> aHisto(h);

  if(doAutozoom) makeAutozoom(aHisto);
  aHisto->GetYaxis()->SetTitleOffset(1.8);
  aHisto->GetZaxis()->SetTitleOffset(1.5);

  return aHisto;//FIX ME avoid object copying
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
TH2D* HistoManager::get2DReconstruction(int strip_dir){

  double radius = 2.0;
  int rebin_space=EVENTTPC_DEFAULT_STRIP_REBIN;
  int rebin_time=EVENTTPC_DEFAULT_TIME_REBIN; 
  int method=EVENTTPC_DEFAULT_RECO_METHOD;
  std::vector<TH2D*> h2DVector = myEvent->Get2D(myTkBuilder.getCluster(),  radius, rebin_space, rebin_time, method);
  if(!h2DVector.size()) return 0;
  int index = 0;
  
  if(strip_dir==DIR_XY) index = 0;
  if(strip_dir==DIR_XZ) index = 1;
  if(strip_dir==DIR_YZ) index = 2;
   
  return h2DVector[index];
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
const TH2D & HistoManager::getHoughAccumulator(int strip_dir, int iPeak){

  return myTkBuilder.getHoughtTransform(strip_dir);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawTrack3D(TVirtualPad *aPad){

  aPad->cd();

  int rebin_space=EVENTTPC_DEFAULT_STRIP_REBIN;
  int rebin_time=EVENTTPC_DEFAULT_TIME_REBIN; 
  TH3D *h3DFrame = myEvent->Get3DFrame(rebin_space, rebin_time);
  h3DFrame->GetXaxis()->SetTitleOffset(2.0);
  h3DFrame->GetYaxis()->SetTitleOffset(2.0);
  h3DFrame->GetZaxis()->SetTitleOffset(2.0);
  h3DFrame->Draw();
  
  const Track3D & aTrack3D = myTkBuilder.getTrack3D(0);
  const TrackSegment3DCollection & trackSegments = aTrack3D.getSegments();
  if(!trackSegments.size()) return;
  
  int iColor = 2;
  std::vector<double> xVec, yVec, zVec;
   for(auto aSegment: trackSegments){
     
     double totalCharge =  aSegment.getIntegratedCharge(aSegment.getLength());
     std::cout<<KBLU<<"segment properties: "<<RST<<std::endl;
     std::cout<<"\t charge: "<<totalCharge<<std::endl;
     std::cout<<"direction: ";
     aSegment.getTangent().Print();
     
     TPolyLine3D aPolyLine;
     aPolyLine.SetLineWidth(2);
     aPolyLine.SetLineColor(iColor++);

     aPolyLine.SetPoint(0,
			aSegment.getStart().X(),
			aSegment.getStart().Y(),
			aSegment.getStart().Z());			
     aPolyLine.SetPoint(1,
			aSegment.getEnd().X(),
			aSegment.getEnd().Y(),
			aSegment.getEnd().Z());
     aPolyLine.DrawClone();
     xVec.push_back(aSegment.getStart().X());
     xVec.push_back(aSegment.getEnd().X());
     yVec.push_back(aSegment.getStart().Y());
     yVec.push_back(aSegment.getEnd().Y());
     zVec.push_back(aSegment.getStart().Z());
     zVec.push_back(aSegment.getEnd().Z());
   }
   double min = *std::min_element(xVec.begin(), xVec.end());
   double max = *std::max_element(xVec.begin(), xVec.end());
   h3DFrame->GetXaxis()->SetRangeUser(min, max);
   min = *std::min_element(yVec.begin(), yVec.end());
   max = *std::max_element(yVec.begin(), yVec.end());
   h3DFrame->GetYaxis()->SetRangeUser(min, max);
   min = *std::min_element(zVec.begin(), zVec.end());
   max = *std::max_element(zVec.begin(), zVec.end());
   h3DFrame->GetZaxis()->SetRangeUser(min, max);
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
void HistoManager::drawTrack2DSeed(int strip_dir, TVirtualPad *aPad){

  const TrackSegment2D & aSegment2D = myTkBuilder.getSegment2D(strip_dir);
  const TVector3 & start = aSegment2D.getStart();
  const TVector3 & end = aSegment2D.getEnd();

  TLine aSegment2DLine;
  aSegment2DLine.SetLineWidth(3);
  aSegment2DLine.SetLineColor(2);
  aPad->cd();
  aSegment2DLine.DrawLine(start.X(), start.Y(),  end.X(),  end.Y());
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawTrack3DProjectionTimeStrip(int strip_dir, TVirtualPad *aPad,  bool zoomIn){

  aPad->cd();
  const Track3D & aTrack3D = myTkBuilder.getTrack3D(0);

  int iSegment = 0;
  TLine aSegment2DLine;
  aSegment2DLine.SetLineWidth(3);
  double minX = 999.0, minY = 999.0;
  double maxX = -999.0, maxY = -999.0;
  double tmp = 0.0;

  for(const auto & aItem: aTrack3D.getSegments()){
    const TrackSegment2D & aSegment2DProjection = aItem.get2DProjection(strip_dir, 0, aItem.getLength());
    const TVector3 & start = aSegment2DProjection.getStart();
    const TVector3 & end = aSegment2DProjection.getEnd();

    aSegment2DLine.SetLineColor(2+iSegment);
    aSegment2DLine.DrawLine(start.X(), start.Y(),  end.X(),  end.Y());	
    ++iSegment;

    tmp = std::min(start.Y(), end.Y());
    minY = std::min(minY, tmp);

    tmp = std::max(start.Y(), end.Y());
    maxY = std::max(maxY, tmp);

    tmp = std::min(start.X(), end.X());
    minX = std::min(minX, tmp);

    tmp = std::max(start.X(), end.X());
    maxX = std::max(maxX, tmp);   
  }
  minX -=5;
  minY -=5;
  
  double delta = std::max( std::abs(maxX - minX),
			   std::abs(maxY - minY));
  maxX = minX + delta;
  maxY = minY + delta;

  if(!zoomIn) return;
  if(aPad->GetListOfPrimitives()->GetSize()){
    TH2D *hFrame = (TH2D*)aPad->GetListOfPrimitives()->At(0);
     if(hFrame){
      hFrame->GetXaxis()->SetRangeUser(minX, maxX);
      hFrame->GetYaxis()->SetRangeUser(minY, maxY);
    }
  }
  else{
    std::cout<<KRED<<"No frame histogram drawn!."<<RST<<std::endl;
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawChargeAlongTrack3D(TVirtualPad *aPad){

  const Track3D & aTrack3D = myTkBuilder.getTrack3D(0);

  aPad->cd();
  TGraph aGr = aTrack3D.getChargeProfile();
  //TGraph aGr = aTrack3D.getHitDistanceProfile();
  aGr.SetTitle("Charge distribution along track.;d[track length];charge[arbitrary units]");
  aGr.SetLineWidth(2);
  aGr.SetLineColor(2);
  aGr.GetYaxis()->SetTitleOffset(1.5);
  aGr.DrawClone("AL");
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::makeAutozoom(std::shared_ptr<TH2D>& aHisto){

  if(!aHisto.get()) return;  
  for(int iAxis=1;iAxis<3;++iAxis){
    double threshold = 0.8*aHisto->GetMaximum();  
    int lowBin = aHisto->FindFirstBinAbove(threshold, iAxis) - 30;
    int highBin = aHisto->FindLastBinAbove(threshold, iAxis) + 30;
    if(iAxis==1) aHisto->GetXaxis()->SetRange(lowBin, highBin);
    else if(iAxis==2) aHisto->GetYaxis()->SetRange(lowBin, highBin);
  }  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::openOutputStream(const std::string & fileName){

  std::size_t last_dot_position = fileName.find_last_of(".");
  std::size_t last_slash_position = fileName.find_last_of("//");
  std::string recoFileName = "Reco_"+fileName.substr(last_slash_position+1,
						     last_dot_position-last_slash_position-1)+".root";
  std::cout<<KBLU<<"recoFileName: "<<RST<<recoFileName<<std::endl;
  myTkBuilder.openOutputStream(recoFileName);  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::writeSegments(){

  myTkBuilder.fillOutputStream();
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

