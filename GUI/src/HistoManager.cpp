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
#include "TVirtualViewer3D.h"
#include "TF1.h"
#include "TLegend.h"
#include "TText.h"
#include "TPaletteAxis.h"
#include "TLatex.h"
#include "TLorentzVector.h"

#include "CommonDefinitions.h"
#include "IonRangeCalculator.h"
#include "MakeUniqueName.h"
#include "GeometryTPC.h"
#include "EventTPC.h"
#include "RunIdParser.h"
#include "colorText.h"

#include "HistoManager.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
HistoManager::HistoManager() {

  myEventPtr = 0;
  myEventInfo = std::make_shared<eventraw::EventInfo>();
  myRecoOutput.setEventInfo(myEventInfo);
  mydEdxFitter.setPressure(190);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
HistoManager::~HistoManager() { }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr){
  
  myGeometryPtr = aGeometryPtr;
  myTkBuilder.setGeometry(aGeometryPtr);
  setDetLayout();
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::setRecoClusterParameters(bool recoClusterEnable, double recoClusterThreshold, int recoClusterDeltaStrips, int recoClusterDeltaTimeCells) {

  this->recoClusterEnable = recoClusterEnable;
  this->recoClusterThreshold = recoClusterThreshold;
  this->recoClusterDeltaStrips = recoClusterDeltaStrips;
  this->recoClusterDeltaTimeCells = recoClusterDeltaTimeCells;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::setEvent(EventTPC* aEvent){
  if(!aEvent) return;
  myEventPtr.reset(aEvent);
  myTkBuilder.setEvent(myEventPtr);
  myEventInfo->set(myEventPtr);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::setEvent(std::shared_ptr<EventTPC> aEvent){
  if(!aEvent) return;
  myEventPtr = aEvent;
<<<<<<< HEAD
  myTkBuilder.setEvent(myEventPtr);
=======
  if(aEvent->GetPedestalSubstracted()){
    myTkBuilder.setEvent(myEventPtr, this->getRecoClusterThreshold(), this->getRecoClusterDeltaStrips(), this->getRecoClusterDeltaTimeCells());
  }
>>>>>>> c41b7bbcd6c95aefca0d75f1041ebc1a2d994fc6
  myEventInfo->set(myEventPtr);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::reconstruct(){
  myTkBuilder.reconstruct();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::reconstructSegmentsFromMarkers(std::vector<double> * segmentsXY){

  myTkBuilder.getSegment2DCollectionFromGUI(*segmentsXY);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawRawHistos(TCanvas *aCanvas, bool isRateDisplayOn){

  if(!aCanvas) return;
  int padNumberOffset = 0;
  if(std::string(aCanvas->GetName())=="fRawHistosCanvas") padNumberOffset = 100;
  
  for(int strip_dir=DIR_U;strip_dir<=DIR_W;++strip_dir){
    TVirtualPad *aPad = aCanvas->GetPad(padNumberOffset+strip_dir+1);
    if(!aPad) return;
    aPad->cd();
    aCanvas->Modified();
    aCanvas->Update();
    getRawStripVsTime(strip_dir)->DrawCopy("colz");
    aPad->RedrawAxis();
  }

  int strip_dir=3;
  TVirtualPad *aPad = aCanvas->GetPad(padNumberOffset+strip_dir+1);
  if(!aPad) return;
  aPad->cd();
  aCanvas->Modified();
  aCanvas->Update();
  if(isRateDisplayOn){
    fObjClones.push_back(getEventRateGraph()->DrawClone("AP"));
  } else{
    getRawTimeProjection()->DrawCopy("hist");
  }
  aCanvas->Modified();
  aCanvas->Update();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawTechnicalHistos(TCanvas *aCanvas, int nAgetChips){

  if(!aCanvas) return;
  int padNumberOffset = 0;
  if(std::string(aCanvas->GetName())=="fTechHistosCanvas") padNumberOffset = 200;
  
  auto cobo_id=0;
  for( int aget_id = 0; aget_id <nAgetChips; ++aget_id ){
    TVirtualPad *aPad = aCanvas->GetPad(padNumberOffset+aget_id+1);
    if(!aPad) return;
    aPad->cd();
    aCanvas->Modified();
    aCanvas->Update();
    getChannels(cobo_id, aget_id)->DrawCopy("colz");
    aCanvas->Modified();
    aCanvas->Update();
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawRecoHistos(TCanvas *aCanvas){

  if(!aCanvas) return;
  int padNumberOffset = 0;
  if(std::string(aCanvas->GetName())=="Histograms") padNumberOffset = 0;
  
  reconstruct();

   for(int strip_dir=DIR_U;strip_dir<=DIR_W;++strip_dir){
     TVirtualPad *aPad = aCanvas->GetPad(padNumberOffset+strip_dir+1);
     if(!aPad) return;
     aPad->cd();
     aCanvas->Modified();
     aCanvas->Update();

<<<<<<< HEAD
     auto h1 = getClusterStripVsTimeInMM(strip_dir)->DrawCopy("colz");
     if(aPad->GetLogz()) h1->SetMinimum(1.0);
     hPlotBackground->Draw("col same");
     aPad->RedrawAxis();  // repaints both axes & fixes X-axis zooming issue
     h1->Draw("same colz");
     drawTrack3DProjectionTimeStrip(strip_dir, aPad, false);
=======
     if(getRecoClusterEnable()) {
       auto h1 = getClusterStripVsTimeInMM(strip_dir)->DrawCopy("colz");
       if(aPad->GetLogz()) h1->SetMinimum(1.0);
       hPlotBackground->Draw("col same");
       aPad->RedrawAxis();  // repaints both axes & fixes X-axis zooming issue
       h1->Draw("same colz");

       drawTrack3DProjectionTimeStrip(strip_dir, aPad, false);

     } else {
       getRawStripVsTimeInMM(strip_dir)->DrawCopy("colz");
     }
     //getRecHitStripVsTime(strip_dir)->DrawCopy("box same");
     //getRawStripVsTimeInMM(strip_dir)->DrawCopy("colz");
     //     drawTrack3DProjectionTimeStrip(strip_dir, aPad, false);
     //if(strip_dir==DIR_W) getClusterTimeProjectionInMM()->DrawCopy("hist");
     //else getRawStripVsTimeInMM(strip_dir)->DrawCopy("colz");
>>>>>>> c41b7bbcd6c95aefca0d75f1041ebc1a2d994fc6
  }
   int strip_dir=3;
   TVirtualPad *aPad = aCanvas->GetPad(padNumberOffset+strip_dir+1);
   if(!aPad) return;
   aPad->cd();
   aCanvas->Modified();
   aCanvas->Update();
<<<<<<< HEAD
   getClusterTimeProjectionInMM()->DrawCopy("hist");
=======
   //drawTrack3DProjectionXY(aPad);
   //drawChargeAlongTrack3D(aPad);
   //myHistoManager.drawTrack3D(aPad);
   if(getRecoClusterEnable()) {
     getClusterTimeProjectionInMM()->DrawCopy("hist");
   } else {
     getRawTimeProjectionInMM()->DrawCopy("hist");
   }
   //getRecHitTimeProjection()->DrawCopy("hist same");
>>>>>>> c41b7bbcd6c95aefca0d75f1041ebc1a2d994fc6
   aCanvas->Modified();
   aCanvas->Update();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawRecoFromMarkers(TCanvas *aCanvas, std::vector<double> * segmentsXY){

  reconstructSegmentsFromMarkers(segmentsXY);
  for(int strip_dir=DIR_U;strip_dir<=DIR_W;++strip_dir){
    TVirtualPad *aPad = aCanvas->cd(strip_dir+1);
    aCanvas->Modified();
    aCanvas->Update();
    drawTrack3DProjectionTimeStrip(strip_dir, aPad, false);
  }
   int strip_dir=3;
   TVirtualPad *aPad = aCanvas->GetPad(strip_dir+1);
   if(!aPad) return;
   aPad->cd();
   aCanvas->Modified();
   aCanvas->Update();
   drawTrack3DProjectionXY(aPad);
   aCanvas->Modified();
   aCanvas->Update();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawDevelHistos(TCanvas *aCanvas){

  if(!aCanvas) return;
  int padNumberOffset = 0;
  if(std::string(aCanvas->GetName())=="Histograms") padNumberOffset = 0;
  reconstruct();

   for(int strip_dir=DIR_U;strip_dir<=DIR_W;++strip_dir){
     TVirtualPad *aPad = aCanvas->GetPad(padNumberOffset+strip_dir+1);
     if(!aPad) return;
     aPad->cd();
     aCanvas->Modified();
     aCanvas->Update();
     
     auto h1 = getClusterStripVsTimeInMM(strip_dir)->DrawCopy("colz");
     if(aPad->GetLogz()) h1->SetMinimum(1.0);
     hPlotBackground->Draw("col same");
     aPad->RedrawAxis();  // repaints both axes & fixes X-axis zooming issue
     h1->Draw("same colz");
     
     //getRecHitStripVsTime(strip_dir)->DrawCopy("box same");     
     //getRawStripVsTimeInMM(strip_dir)->DrawCopy("colz");
     drawTrack3DProjectionTimeStrip(strip_dir, aPad, false);
     //drawTrack2DSeed(strip_dir, aPad);
     //if(strip_dir==DIR_W) getClusterTimeProjectionInMM()->DrawCopy("hist");
  }
   int strip_dir=3;
   TVirtualPad *aPad = aCanvas->GetPad(padNumberOffset+strip_dir+1);
   if(!aPad) return;
   aPad->cd();
   aCanvas->Modified();
   aCanvas->Update();
   //drawTrack3DProjectionXY(aPad);
   drawChargeAlongTrack3D(aPad);
   //myHistoManager.drawTrack3D(aPad);
   //getClusterTimeProjectionInMM()->DrawCopy("hist");
   //getRecHitTimeProjection()->DrawCopy("hist same");
   aCanvas->Modified();
   aCanvas->Update();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::clearCanvas(TCanvas *aCanvas, bool isLogScaleOn){

  if(!aCanvas) return;
  /* TEST
  clearTracks();  
  for(auto aObj : fObjClones){
    if(aObj) delete aObj;
  }
  fObjClones.clear();
  */
  TList *aList = aCanvas->GetListOfPrimitives();
  TText aMessage(0.0, 0.0,"Waiting for data.");
  for(auto obj: *aList){
    TPad *aPad = (TPad*)(obj);
    if(!aPad) continue;
    aPad->Clear();
    aPad->cd();
    fObjClones.push_back(aMessage.DrawTextNDC(0.3, 0.5,"Waiting for data."));
    aPad->SetLogz(isLogScaleOn);
  }
  aCanvas->Modified();  
  aCanvas->Update();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::clearTracks(){
  
  for(auto aObj : fTrackLines){
    if(aObj) aObj->Delete();
  }
  fTrackLines.clear();
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::setDetLayout(){
  if(!myGeometryPtr) {
    return;
  }
  if(grDetLayoutAll) { delete grDetLayoutAll; grDetLayoutAll=0; } // reset TGraph
  if(grDetLayoutVeto) { delete grDetLayoutVeto; grDetLayoutVeto=0; } // reset TGraph
  grDetLayoutAll = new TGraph(myGeometryPtr->GetActiveAreaConvexHull());
  grDetLayoutVeto = new TGraph(myGeometryPtr->GetActiveAreaConvexHull(grVetoBand));

  // calculates optimal range for special histogram depicting UVW active area
  // - preserves 1:1 aspect ratio
  // - adds +/-5% margin to the longest dimension
  // - bin area = 1 mm x 1 mm
  float xmin, xmax, ymin, ymax;
  std::tie(xmin, xmax, ymin, ymax)=myGeometryPtr->rangeXY();
  float best_width = 1.1*std::max(xmax-xmin, ymax-ymin);
  TH2F hDetLayoutTmp("hDetLayoutTmp",";x [mm];y [mm]",
		     (int)(best_width+0.5), 0.5*(xmin+xmax)-0.5*best_width, 0.5*(xmin+xmax)+0.5*best_width,
		     (int)(best_width+0.5), 0.5*(ymin+ymax)-0.5*best_width, 0.5*(ymin+ymax)+0.5*best_width);
  hDetLayout = (TH2F*)hDetLayoutTmp.Clone("hDetLayout");
  
  // calculates optimal ranges for special background 2D histogram:
  // - UVW STRIP projection range from all directions
  // - DRIFT projection range
  float strip_min, strip_max;
  std::tie(strip_min, strip_max)=myGeometryPtr->rangeStripDirInMM(DIR_U);
  for(int idir=DIR_V; idir<=DIR_W; ++idir) {
    float a, b;
    std::tie(a, b)=myGeometryPtr->rangeStripDirInMM(idir);
    if(a<strip_min) strip_min=a;
    if(b>strip_max) strip_max=b;
  }
  float drift_min, drift_max;
  std::tie(drift_min, drift_max)=myGeometryPtr->rangeZ();
  hPlotBackground = new TH2F("hPlotBackground","",
			     1, drift_min-0.05*(drift_max-drift_min), drift_max+0.05*(drift_max-drift_min),
			     1, strip_min-0.05*(strip_max-strip_min), strip_max+0.05*(strip_max-strip_min));

  // set background color to 25% of the linear full color scale
  const float bkg_min=1.0;
  hPlotBackground->Fill(0.5*(drift_min+drift_max), 0.5*(strip_min+strip_max), bkg_min);
  double contours[] = {0, 11, 20, 30};
  hPlotBackground->SetContour(4, contours);
  hPlotBackground->SetStats(kFALSE);
  
  hDetLayout->SetStats(kFALSE);
  hDetLayout->GetYaxis()->SetTitleOffset(1.4);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawDetLayout(){

  if(!hDetLayout) return;
  hDetLayout->DrawCopy();
  if(grDetLayoutAll) {
    grDetLayoutAll->SetLineColor(kBlack);
    grDetLayoutAll->SetLineWidth(2);
    grDetLayoutAll->SetLineStyle(kSolid);
    grDetLayoutAll->DrawClone("L");
  }
  if(grDetLayoutVeto) {
    grDetLayoutVeto->SetLineColor(kBlack);
    grDetLayoutVeto->SetLineWidth(1);
    grDetLayoutVeto->SetLineStyle(kDashed);
    grDetLayoutVeto->DrawClone("L");
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::setDetLayoutVetoBand(double distance){ // [mm]
  if(distance<0) distance=0;
  if(distance!=grVetoBand) { // update upon distance change
    grVetoBand=distance;
    setDetLayout();
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH1D> HistoManager::getRawTimeProjection(){
  auto aHisto=std::shared_ptr<TH1D>(myEventPtr->GetTimeProjection());
  if(aHisto) {
    if(doAutozoom) makeAutozoom(aHisto.get());
    aHisto->GetXaxis()->SetTitleOffset(1.5);
    aHisto->GetYaxis()->SetTitleOffset(1.6);
    aHisto->SetDrawOption("HIST0");
  }
  return aHisto;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH1D> HistoManager::getRawTimeProjectionInMM(){
  auto aHisto=std::shared_ptr<TH1D>(myEventPtr->GetTimeProjectionInMM());
  if(aHisto) {
    if(doAutozoom) makeAutozoom(aHisto.get());
    aHisto->GetXaxis()->SetTitleOffset(1.5);
    aHisto->GetYaxis()->SetTitleOffset(1.6);
    aHisto->SetDrawOption("HIST0");
  }
  return aHisto;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH1D> HistoManager::getRawTimeProjectionInMM(int strip_dir){
  auto aHisto=std::shared_ptr<TH1D>(myEventPtr->GetTimeProjectionInMM(strip_dir));
  if(aHisto) {
    if(doAutozoom) makeAutozoom(aHisto.get());
    aHisto->GetXaxis()->SetTitleOffset(1.5);
    aHisto->GetYaxis()->SetTitleOffset(1.6);
    aHisto->SetDrawOption("HIST0");
  }
  return aHisto;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH1D> HistoManager::getRawStripProjection(int strip_dir){
  auto aHisto=std::shared_ptr<TH1D>(myEventPtr->GetStripProjection(strip_dir));
  if(aHisto) {
    if(doAutozoom) makeAutozoom(aHisto.get());
    aHisto->GetXaxis()->SetTitleOffset(1.5);
    aHisto->GetYaxis()->SetTitleOffset(1.6);
    aHisto->SetDrawOption("HIST0");
  }
  return aHisto;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH1D> HistoManager::getRawStripProjectionInMM(int strip_dir){
  auto aHisto=std::shared_ptr<TH1D>(myEventPtr->GetStripProjectionInMM(strip_dir));
  if(aHisto) {
    if(doAutozoom) makeAutozoom(aHisto.get());
    aHisto->GetXaxis()->SetTitleOffset(1.5);
    aHisto->GetYaxis()->SetTitleOffset(1.6); 
    aHisto->SetDrawOption("HIST0");
  }
  return aHisto;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::getRawStripVsTime(int strip_dir){
  auto aHisto=std::shared_ptr<TH2D>(myEventPtr->GetStripVsTime(strip_dir));
  if(aHisto) {
    if(doAutozoom) makeAutozoom(aHisto.get());
    aHisto->GetXaxis()->SetTitleOffset(1.5);
    aHisto->GetYaxis()->SetTitleOffset(1.5);
    aHisto->GetZaxis()->SetTitleOffset(1.5);
    aHisto->SetDrawOption("COLZ");
  }
  return aHisto;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::getRawStripVsTimeInMM(int strip_dir){
  auto aHisto=std::shared_ptr<TH2D>(myEventPtr->GetStripVsTimeInMM(strip_dir)); 
  if(aHisto) {
    if(doAutozoom) makeAutozoom(aHisto.get());
    aHisto->GetXaxis()->SetTitleOffset(1.5);
    aHisto->GetYaxis()->SetTitleOffset(1.5);
    aHisto->GetZaxis()->SetTitleOffset(1.5);
    aHisto->SetDrawOption("COLZ");
  }
  return aHisto;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH1D> HistoManager::getClusterTimeProjection(){
  auto aHisto=std::shared_ptr<TH1D>(myEventPtr->GetTimeProjection(myTkBuilder.getCluster()));
  if(aHisto) {
    if(doAutozoom) makeAutozoom(aHisto.get());
    aHisto->GetXaxis()->SetTitleOffset(1.5);
    aHisto->GetYaxis()->SetTitleOffset(1.6);
    aHisto->SetDrawOption("HIST0");
  }
  return aHisto;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH1D> HistoManager::getClusterTimeProjectionInMM(){
  //auto aHisto=std::shared_ptr<TH1D>(myEventPtr->GetTimeProjectionInMM(myTkBuilder.getCluster()));
  auto aHisto=std::shared_ptr<TH1D>((TH1D*)myTkBuilder.getCluster2D(DIR_U).ProjectionX()->Clone("hClusterTimeProjectionInMM"));
  aHisto->SetTitle("Clustered hits time projection. All directions.");
  for(int strip_dir=DIR_U;strip_dir<=DIR_W;++strip_dir){
    TH1D *hTimeProj = myTkBuilder.getCluster2D(strip_dir).ProjectionX();
    aHisto->Add(hTimeProj);
  }

  if(aHisto) {
    if(doAutozoom) makeAutozoom(aHisto.get());
    aHisto->GetXaxis()->SetTitleOffset(1.5);
    aHisto->GetYaxis()->SetTitleOffset(1.6);
    aHisto->SetDrawOption("HIST0");
  }
  return aHisto;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH1D> HistoManager::getClusterTimeProjection(int strip_dir){
  auto aHisto=std::shared_ptr<TH1D>(myEventPtr->GetTimeProjection(myTkBuilder.getCluster(), strip_dir));
  if(aHisto) {
    if(doAutozoom) makeAutozoom(aHisto.get());
    aHisto->GetXaxis()->SetTitleOffset(1.5);
    aHisto->GetYaxis()->SetTitleOffset(1.6);
    aHisto->SetDrawOption("HIST0");
  }
  return aHisto;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH1D> HistoManager::getClusterTimeProjectionInMM(int strip_dir){
  //auto aHisto=std::shared_ptr<TH1D>(myEventPtr->GetTimeProjectionInMM(myTkBuilder.getCluster(), strip_dir));
  auto aHisto=std::shared_ptr<TH1D>((TH1D*)myTkBuilder.getCluster2D(strip_dir).ProjectionX()->Clone("hClusterTimeProjectionInMM"));
  
  if(aHisto) {
    if(doAutozoom) makeAutozoom(aHisto.get());
    aHisto->GetXaxis()->SetTitleOffset(1.5);
    aHisto->GetYaxis()->SetTitleOffset(1.6);
    aHisto->SetDrawOption("HIST0");
  }
  return aHisto;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH1D> HistoManager::getClusterStripProjection(int strip_dir){

  auto aHisto=std::shared_ptr<TH1D>(myEventPtr->GetStripProjection(myTkBuilder.getCluster(), strip_dir));  
  if(aHisto) {
    if(doAutozoom) makeAutozoom(aHisto.get());
    aHisto->GetXaxis()->SetTitleOffset(1.5);
    aHisto->GetYaxis()->SetTitleOffset(1.6);
    aHisto->SetDrawOption("HIST0");
  }
  return aHisto;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH1D> HistoManager::getClusterStripProjectionInMM(int strip_dir){
  //auto aHisto=std::shared_ptr<TH1D>(myEventPtr->GetStripProjectionInMM(myTkBuilder.getCluster(), strip_dir));
  auto aHisto=std::shared_ptr<TH1D>((TH1D*)myTkBuilder.getCluster2D(strip_dir).ProjectionY()->Clone("hClusterStripProjectionInMM"));

  if(aHisto) {
    if(doAutozoom) makeAutozoom(aHisto.get());
    aHisto->GetXaxis()->SetTitleOffset(1.5);
    aHisto->GetYaxis()->SetTitleOffset(1.6);
    aHisto->SetDrawOption("HIST0");
  }
  return aHisto;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::getClusterStripVsTime(int strip_dir){
  auto aHisto=std::shared_ptr<TH2D>(myEventPtr->GetStripVsTime(myTkBuilder.getCluster(), strip_dir));

  if(aHisto) {
    if(doAutozoom) makeAutozoom(aHisto.get());
    aHisto->GetXaxis()->SetTitleOffset(1.5);
    aHisto->GetYaxis()->SetTitleOffset(1.5);
    aHisto->GetZaxis()->SetTitleOffset(1.5);
    aHisto->SetDrawOption("COLZ");
  }
  return aHisto;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::getClusterStripVsTimeInMM(int strip_dir){
  
  //std::shared_ptr<TH2D> aHisto = myEventPtr->GetStripVsTimeInMM(myTkBuilder.getCluster(), strip_dir);
  std::shared_ptr<TH2D> aHisto(new TH2D(myTkBuilder.getCluster2D(strip_dir)));
  if(aHisto) {
    if(doAutozoom) makeAutozoom(aHisto.get());
    aHisto->GetXaxis()->SetTitleOffset(1.5);
    aHisto->GetYaxis()->SetTitleOffset(1.5);
    aHisto->GetZaxis()->SetTitleOffset(1.5);
    aHisto->SetDrawOption("COLZ");
  }
  return aHisto;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool HistoManager::getRecoClusterEnable(){
  return recoClusterEnable;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double HistoManager::getRecoClusterThreshold(){
  return recoClusterThreshold;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int HistoManager::getRecoClusterDeltaStrips(){
  return recoClusterDeltaStrips;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int HistoManager::getRecoClusterDeltaTimeCells(){
  return recoClusterDeltaTimeCells;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::getFilteredStripVsTime(int strip_dir){
  auto aHisto=std::shared_ptr<TH2D>(myEventPtr->GetStripVsTime(myTkBuilder.getCluster(), strip_dir));
  if(aHisto) {
    if(doAutozoom) makeAutozoom(aHisto.get());
    aHisto->GetYaxis()->SetTitleOffset(1.8);
    aHisto->GetZaxis()->SetTitleOffset(1.5);
  }
  return aHisto;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::getChannels(int cobo_id, int asad_id){
  auto aHisto=std::shared_ptr<TH2D>(myEventPtr->GetChannels(cobo_id, asad_id));
  if(aHisto) {
    aHisto->GetXaxis()->SetTitleOffset(1.5);
    aHisto->GetYaxis()->SetTitleOffset(1.5);
    aHisto->GetZaxis()->SetTitleOffset(1.5);
    aHisto->SetDrawOption("COLZ");
  }
  return aHisto;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::getRecHitStripVsTime(int strip_dir){
  TH2D *h = (TH2D*)myTkBuilder.getRecHits2D(strip_dir).Clone("hRecHitStripVsTime");
  std::shared_ptr<TH2D> aHisto(h);
  if(aHisto) {
    if(doAutozoom) makeAutozoom(aHisto.get());
    aHisto->SetMarkerStyle(24);
    aHisto->SetMarkerColorAlpha(kRed, 0.1);
    aHisto->SetFillColorAlpha(kRed, 0.1);
    aHisto->SetLineColorAlpha(kRed, 0.1);
    aHisto->GetYaxis()->SetTitleOffset(1.8);
    aHisto->GetZaxis()->SetTitleOffset(1.5);
  }
  return aHisto;//FIX ME avoid object copying
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH1D> HistoManager::getRecHitTimeProjection(){
  TH1D *h = (TH1D*)myTkBuilder.getRecHitsTimeProjection().Clone("hRecHitTimeProjection");
  auto aHisto=std::shared_ptr<TH1D>(h);
  if(doAutozoom) makeAutozoom(aHisto.get());
  aHisto->SetLineColor(2);
  return aHisto;//FIX ME avoid object copying
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TH3D* HistoManager::get3DReconstruction(){

  double radius = 2.0;
  int rebin_space=EVENTTPC_DEFAULT_STRIP_REBIN;
  int rebin_time=EVENTTPC_DEFAULT_TIME_REBIN; 
  int method=EVENTTPC_DEFAULT_RECO_METHOD;
  h3DReco = myEventPtr->Get3D(myTkBuilder.getCluster(),  radius, rebin_space, rebin_time, method);
  return h3DReco;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TH2D* HistoManager::get2DReconstruction(int strip_dir){

  double radius = 2.0;
  int rebin_space=EVENTTPC_DEFAULT_STRIP_REBIN;
  int rebin_time=EVENTTPC_DEFAULT_TIME_REBIN; 
  int method=EVENTTPC_DEFAULT_RECO_METHOD;
  std::vector<TH2D*> h2DVector = myEventPtr->Get2D(myTkBuilder.getCluster(),  radius, rebin_space, rebin_time, method);
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
  TH3D *h3DFrame = myEventPtr->Get3DFrame(rebin_space, rebin_time);
  h3DFrame->GetXaxis()->SetTitleOffset(2.0);
  h3DFrame->GetYaxis()->SetTitleOffset(2.0);
  h3DFrame->GetZaxis()->SetTitleOffset(2.0);
  h3DFrame->Draw("box");

  TVirtualViewer3D * view3D = aPad->GetViewer3D("pad");
  view3D->BeginScene();
  
  const Track3D & aTrack3D = myTkBuilder.getTrack3D(0);
  const TrackSegment3DCollection & trackSegments = aTrack3D.getSegments();
  if(!trackSegments.size()) return;
  
  int iColor = 2;
  std::vector<double> xVec, yVec, zVec;
   for(auto aSegment: trackSegments){
     
     std::cout<<KBLU<<"segment properties  START -> STOP [chi2], [charge]: "<<RST<<std::endl;
     std::cout<<"\t"<<aSegment<<std::endl;
     
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

     fObjClones.push_back(aPolyLine.DrawClone());

     xVec.push_back(aSegment.getStart().X());
     xVec.push_back(aSegment.getEnd().X());
     yVec.push_back(aSegment.getStart().Y());
     yVec.push_back(aSegment.getEnd().Y());
     zVec.push_back(aSegment.getStart().Z());
     zVec.push_back(aSegment.getEnd().Z());
     /*
   TList outlineList;
   std::vector<double> minCoords, maxCoords;
   double min = *std::min_element(xVec.begin(), xVec.end());
   double max = *std::max_element(xVec.begin(), xVec.end());
   minCoords.push_back(min);
   maxCoords.push_back(max);

   min = *std::min_element(yVec.begin(), yVec.end());
   max = *std::max_element(yVec.begin(), yVec.end());
   minCoords.push_back(min);
   maxCoords.push_back(max);

   
   min = *std::min_element(zVec.begin(), zVec.end());
   max = *std::max_element(zVec.begin(), zVec.end());
   minCoords.push_back(min);
   maxCoords.push_back(max);
   
   aPolyLine.DrawOutlineCube(&outlineList, minCoords.data(), maxCoords.data());
   aPolyLine.DrawCopy();
   view3D->EndScene();
   return;
     */
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

   view3D->EndScene();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawTrack3DProjectionXY(TVirtualPad *aPad){

  aPad->cd();
  aPad->SetLogz(kFALSE);
  drawDetLayout();
  
  const Track3D & aTrack3D = myTkBuilder.getTrack3D(0);

  int iSegment = 0;
  TLine aSegment2DLine;
  aSegment2DLine.SetLineWidth(2);
  for(const auto & aItem: aTrack3D.getSegments()){
    const TVector3 & start = aItem.getStart();
    const TVector3 & end = aItem.getEnd();
    aSegment2DLine.SetLineColor(2 + iSegment);
    aSegment2DLine.SetLineWidth(3);
    fTrackLines.push_back(aSegment2DLine.DrawLine(start.X(), start.Y(),  end.X(),  end.Y()));
    fTrackLines.back()->ResetBit(kCanDelete);
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
  aSegment2DLine.SetLineWidth(4);
  aSegment2DLine.SetLineStyle(8);
  double minX = 999.0, minY = 999.0;
  double maxX = -999.0, maxY = -999.0;
  double tmp = 0.0;

  for(const auto & aItem: aTrack3D.getSegments()){
    const TrackSegment2D & aSegment2DProjection = aItem.get2DProjection(strip_dir, 0, aItem.getLength());
    const TVector3 & start = aSegment2DProjection.getStart();
    const TVector3 & end = aSegment2DProjection.getEnd();

    aSegment2DLine.SetLineColor(2+iSegment);
    fTrackLines.push_back(aSegment2DLine.DrawLine(start.X(), start.Y(),  end.X(),  end.Y()));
    fTrackLines.back()->ResetBit(kCanDelete);
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

  aPad->cd();

  const Track3D & aTrack3D = myTkBuilder.getTrack3D(0);
  if(!aTrack3D.getSegments().size()) return;
  
  TH1F hFrame("hFrame",";d [mm];charge [arb. units]",2,-20, 20+aTrack3D.getLength());
  hFrame.GetYaxis()->SetTitleOffset(2.0);
  hFrame.SetMinimum(0.0);

  TH1F hAlphaChargeProfile = aTrack3D.getSegments().front().getChargeProfile();
  hAlphaChargeProfile.SetLineWidth(2);
  hAlphaChargeProfile.SetLineColor(kBlue-9);
  hAlphaChargeProfile.SetMarkerColor(kBlue-9);
  hAlphaChargeProfile.SetMarkerSize(1.0);
  hAlphaChargeProfile.SetMarkerStyle(20);

  TH1F hCarbonChargeProfile = aTrack3D.getSegments().back().getChargeProfile();
  hCarbonChargeProfile.SetLineWidth(2);
  hCarbonChargeProfile.SetLineColor(2);
  hCarbonChargeProfile.SetMarkerColor(2);
  hCarbonChargeProfile.SetMarkerSize(1.0);
  hCarbonChargeProfile.SetMarkerStyle(20);

  TH1F hChargeProfile = aTrack3D.getChargeProfile();
  hChargeProfile.Scale(1.0/hChargeProfile.GetMaximum());
  hChargeProfile.SetLineWidth(2);
  hChargeProfile.SetLineColor(2);
  hChargeProfile.SetMarkerColor(2);
  hChargeProfile.SetMarkerSize(1.0);
  hChargeProfile.SetMarkerStyle(20);

  //double maxCharge = hAlphaChargeProfile.GetMaximum();
  //maxCharge = std::max(maxCharge, hCarbonChargeProfile.GetMaximum());
  //maxCharge = 1.0;
  //hFrame.SetMaximum(1.1*maxCharge);
  hFrame.DrawCopy();
  hChargeProfile.DrawCopy("same HIST P");
  //hAlphaChargeProfile.DrawCopy("same HIST P");
  //hCarbonChargeProfile.DrawCopy("same HIST P");
  
  TLegend *aLegend = new TLegend(0.7, 0.75, 0.95,0.95);
  fObjClones.push_back(aLegend);
  
  TF1 dEdx = myTkBuilder.getdEdx();
  if(!dEdx.GetNpar()) return;
  double carbonScale = dEdx.GetParameter("carbonScale");
  return;

  dEdx.SetLineColor(kBlack);
  dEdx.SetLineStyle(1);
  dEdx.SetLineWidth(3);
  TObject *aObj = dEdx.DrawCopy("same");
  aLegend->AddEntry(aObj,"^{12}C + #alpha","l");
  fObjClones.push_back(aObj);

  dEdx.SetParameter("carbonScale",0.0);
  dEdx.SetLineColor(kRed);
  dEdx.SetLineStyle(2);
  dEdx.SetLineWidth(2);
  TObject *aObj1 = dEdx.DrawCopy("same");
  aLegend->AddEntry(aObj1,"#alpha","l");
  fObjClones.push_back(aObj1);
  
  dEdx.SetParameter("alphaScale",0.0);
  dEdx.SetParameter("carbonScale",carbonScale);
  dEdx.SetLineColor(kBlue-9);
  dEdx.SetLineStyle(2);
  dEdx.SetLineWidth(2);
  TObject *aObj2  = dEdx.DrawCopy("same");
  aLegend->AddEntry(aObj2,"^{12}C","l");
  fObjClones.push_back(aObj2);
  aLegend->Draw();

  IonRangeCalculator myRangeCalculator(gas_mixture_type::CO2,190.0,293.15);  
  double alphaRange = aTrack3D.getSegments().front().getLength();
  double carbonRange = aTrack3D.getSegments().back().getLength();
  double alphaEnergy = myRangeCalculator.getIonEnergyMeV(pid_type::ALPHA,alphaRange);   
  double carbonEnergy = myRangeCalculator.getIonEnergyMeV(pid_type::CARBON_12,carbonRange);
  
  TLatex aLatex;
  double x = 0.35*aTrack3D.getLength();
  double y = 0.75;
  aLatex.DrawLatex(x,y,TString::Format("Track length: %3.0f mm",aTrack3D.getLength()));
  y = 0.68;
  aLatex.DrawLatex(x,y,TString::Format("Total E: %2.1f MeV",alphaEnergy+carbonEnergy));
  
  std::cout<<"Alpha energy [MeV]: "<<alphaEnergy<<std::endl;
  std::cout<<"Carbon energy [MeV]: "<<carbonEnergy<<std::endl;
  std::cout<<"Total energy [MeV]: "<<alphaEnergy+carbonEnergy<<std::endl;

  std::cout<<"Alpha range [mm]: "<<alphaRange<<std::endl;
  std::cout<<"Carbon range [mm]: "<<carbonRange<<std::endl;
  std::cout<<"Total range [mm]: "<<alphaRange+carbonRange<<std::endl;

  double atomicMassUnit = 931.49410242; //MeV/c^2
  double alphaMass = 4*atomicMassUnit + 2.4249; //A*u + Delta MeV/c2
  double carbon12Mass = 12*atomicMassUnit;
  double carbon13Mass = 13*atomicMassUnit + 3.1250093323;
  double oxygen16Mass = 16*atomicMassUnit - 4.7370022;
  double oxygen18Mass = 18*atomicMassUnit - 0.7828163;

  double energy = alphaEnergy + alphaMass;
  double momentum = sqrt(energy*energy - alphaMass*alphaMass);
  TLorentzVector alphaP4(aTrack3D.getSegments().front().getTangent()*momentum, energy);

  energy = carbonEnergy + carbon12Mass;
  momentum = sqrt(energy*energy - carbon12Mass*carbon12Mass);
  TLorentzVector carbon12P4(aTrack3D.getSegments().back().getTangent()*momentum, energy);

  energy = carbonEnergy + carbon13Mass;
  momentum = sqrt(energy*energy - carbon13Mass*carbon13Mass);
  TLorentzVector carbon13P4(aTrack3D.getSegments().back().getTangent()*momentum, energy);

  std::cout<<"---------------------------------"<<std::endl;
  TLorentzVector totalP4 = alphaP4+carbon12P4;
  totalP4.Print();
  (alphaP4+carbon13P4).Print();
  std::cout<<"oxygen16Mass: "<<oxygen16Mass<<std::endl;
  std::cout<<"oxygen18Mass: "<<oxygen18Mass<<std::endl;
  std::cout<<"hypothesis chi2: "<<aTrack3D.getHypothesisFitChi2()<<std::endl;
  std::cout<<"---------------------------------"<<std::endl;
  
  //Dla tej energi gamm E(12C)+E(alpha) = 5.133 MeV natomiast E(14C)+E(alpha) = 6.06916 MeV.
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::makeAutozoom(TH1 * aHisto){

  if(!aHisto) return;
  int margin = 0;
  for(int iAxis=1;iAxis<=aHisto->GetDimension();++iAxis){
    margin = 10;
    double threshold = 0.1*aHisto->GetMaximum();  
    int lowBin = aHisto->FindFirstBinAbove(threshold, iAxis);
    int highBin = aHisto->FindLastBinAbove(threshold, iAxis);
    margin += (highBin - lowBin)*0.1;
    if(iAxis==1) aHisto->GetXaxis()->SetRange(lowBin-margin, highBin+margin);
    else if(iAxis==2) aHisto->GetYaxis()->SetRange(lowBin-margin, highBin+margin);
  }  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::openOutputStream(const std::string & filePath){

  if(openOutputStreamInitialized) return;

  openOutputStreamInitialized = true;
  std::size_t last_dot_position = filePath.find_last_of(".");
  std::size_t last_slash_position = filePath.find_last_of("//");
  std::string recoFileName = MakeUniqueName("Reco_"+filePath.substr(last_slash_position+1,
						     last_dot_position-last_slash_position-1)+".root");
  myRecoOutput.open(recoFileName);

  std::string fileName = filePath.substr(last_slash_position+1);
  if(fileName.find("CoBo")==std::string::npos){
    fileName = fileName.replace(0,8,"CoBo_ALL_AsAd_ALL");
  }
  RunIdParser runParser(fileName);
  myEventInfo->SetRunId(runParser.runId());
  /*
  std::string fluxFileName = "Flux_"+filePath.substr(last_slash_position+1,
						     last_dot_position-last_slash_position-1)+".root";
  std::cout<<KBLU<<"fluxFileName: "<<RST<<recoFileName<<std::endl;
  myDotFinder.openOutputStream(fluxFileName);
  */
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::writeRecoData(unsigned long eventType){

  myRecoOutput.setRecTrack(myTkBuilder.getTrack3D(0));
  myEventInfo->SetEventType(eventType);				   
  myRecoOutput.setEventInfo(myEventInfo);				   
  myRecoOutput.update();  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::updateEventRateGraph(){

  if(!grEventRate){
    grEventRate = new TGraph();
    grEventRate->SetMarkerColor(4);
    grEventRate->SetMarkerSize(1.0);
    grEventRate->SetMarkerStyle(20);
    grEventRate->GetXaxis()->SetTitle("Time since start of run [s]");
    grEventRate->GetYaxis()->SetTitle("Event rate [Hz]");
    grEventRate->GetYaxis()->SetTitleOffset(1.5);
    grEventRate->GetXaxis()->SetNdivisions(5);
  }
  Long64_t currentEventTime = myEventPtr->GetEventTime()*1E-8;//[s]
  Long64_t currentEventNumber = myEventPtr->GetEventId();  
  if(previousEventTime<0 || previousEventNumber>currentEventNumber){
    previousEventTime = currentEventTime;
    previousEventNumber = currentEventNumber;
  }
  Long64_t deltaTime = currentEventTime - previousEventTime;
  Long64_t deltaEventCount = currentEventNumber - previousEventNumber;
  double rate = double(deltaEventCount)/deltaTime; //[Hz]
  if(deltaTime==0) rate = 0.0;
  previousEventTime = currentEventTime;
  previousEventNumber = currentEventNumber;
  /*
  std::cout<<"currentEventTime: "<<currentEventTime<<" [s]"<<std::endl;
  std::cout<<"currentEventNumber: "<<currentEventNumber<<std::endl;
  std::cout<<"deltaTime: "<<deltaTime<<" [s]"<<std::endl;
  std::cout<<"delta event count: "<<deltaEventCount<<std::endl;
  std::cout<<"rate: "<<rate<<" [Hz]"<<std::endl;
  */
  grEventRate->Expand(grEventRate->GetN()+1);
  grEventRate->SetPoint(grEventRate->GetN(), currentEventTime, rate);
  grEventRate->GetXaxis()->SetTitle("Time since start of run [s]; ");
  grEventRate->GetYaxis()->SetTitle("Event rate [Hz]");
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TGraph* HistoManager::getEventRateGraph(){

  updateEventRateGraph();
  return grEventRate; 
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::resetEventRateGraph(){
 if(grEventRate){
   grEventRate->Set(0);
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// Dot-like events useful for neutron flux monitoring
/////////////////////////////////////////////////////////
void HistoManager::initializeDotFinder(unsigned int hitThr,
				       //				       unsigned int maxStripsPerDir,
				       //				       unsigned int maxTimecellsPerDir,
				       unsigned int totalChargeThr,
				       double matchRadiusInMM,
				       const std::string & filePath) {
  myDotFinder.openOutputStream(filePath);
  myDotFinder.setCuts(hitThr, /* maxStripsPerDir, maxTimecellsPerDir,*/ totalChargeThr, matchRadiusInMM);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// Dot-like events usful for neutron flux monitoring
/////////////////////////////////////////////////////////
void HistoManager::runDotFinder() {
  myDotFinder.setEvent(myEventPtr);
  myDotFinder.reconstruct();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// Dot-like events usful for neutron flux monitoring
/////////////////////////////////////////////////////////
void HistoManager::finalizeDotFinder() {
  myDotFinder.fillOutputStream();
  myDotFinder.closeOutputStream();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

