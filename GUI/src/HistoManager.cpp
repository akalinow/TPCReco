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
#include "MakeUniqueName.h"
#include "GeometryTPC.h"
#include "EventTPC.h"
#include "RunIdParser.h"
#include "colorText.h"

#include "HistoManager.h"

#include "CoordinateConverter.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
HistoManager::HistoManager() {

  myEventPtr = 0;
  myRecoOutput.setEventInfo(myEventInfo);
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
void HistoManager::setPressure(double aPressure){
  
  myTkBuilder.setPressure(aPressure);
  myRangeCalculator.setGasPressure(aPressure);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::setConfig(const boost::property_tree::ptree &aConfig){
  
  myConfig = aConfig;

  if(myConfig.find("recoClusterEnable")==myConfig.not_found()) myConfig.put("recoClusterEnable", true);
  if(myConfig.find("recoClusterThreshold")==myConfig.not_found()) myConfig.put("recoClusterThreshold", 35.0);
  if(myConfig.find("recoClusterDeltaStrips")==myConfig.not_found()) myConfig.put("recoClusterDeltaStrips", 2);
  if(myConfig.find("recoClusterDeltaTimeCells")==myConfig.not_found()) myConfig.put("recoClusterDeltaTimeCells", 5);

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::setEvent(EventTPC* aEvent){

  setEvent(std::shared_ptr<EventTPC>(aEvent));

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::setEvent(std::shared_ptr<EventTPC> aEvent){
  
  if(!aEvent) return;
  myEventPtr = aEvent;
  myEventPtr->setHitFilterConfig(filter_type::threshold, myConfig);
  myTkBuilder.setEvent(myEventPtr);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::reconstruct(){
  if(myEventPtr->GetEventInfo().GetPedestalSubtracted()) myTkBuilder.reconstruct();
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
  
  for(int strip_dir=projection_type::DIR_U;strip_dir<=projection_type::DIR_W;++strip_dir){
    TVirtualPad *aPad = aCanvas->GetPad(padNumberOffset+strip_dir+1);
    if(!aPad) return;
    aPad->cd();
    aCanvas->Modified();
    aCanvas->Update();
    auto projType = get2DProjectionType(strip_dir);
    aPad->SetFrameFillColor(kAzure-6);
    get2DProjection(projType, filter_type::none, scale_type::raw)->DrawCopy("colz");
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
    get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::raw)->DrawCopy("hist");
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
  
  //reco disabled for clicking campaign reconstruct();
  filter_type filterType = filter_type::threshold;
  if(!myConfig.get<bool>("recoClusterEnable")) filterType = filter_type::none;

   for(int strip_dir=projection_type::DIR_U;strip_dir<=projection_type::DIR_W;++strip_dir){
     TVirtualPad *aPad = aCanvas->GetPad(padNumberOffset+strip_dir+1);
     if(!aPad) return;
     aPad->cd();
     aCanvas->Modified();
     aCanvas->Update();

     auto projType = get2DProjectionType(strip_dir);     
     auto histo2D = get2DProjection(projType, filterType, scale_type::mm);
     aPad->SetFrameFillColor(kAzure-6);
     /*
     if(myConfig.get<bool>("recoClusterEnable")){
       histo2D->SetMinimum(0.0);
       histo2D->DrawCopy("colz");
       if(aPad->GetLogz()) histo2D->SetMinimum(1.0);
       hPlotBackground->Draw("col same");
       aPad->RedrawAxis();
       histo2D->DrawCopy("colz same");
       drawTrack3DProjectionTimeStrip(strip_dir, aPad, false);	    
     }
     else */histo2D->DrawCopy("colz");
   }
   int strip_dir=3;
   TVirtualPad *aPad = aCanvas->GetPad(padNumberOffset+strip_dir+1);
   if(!aPad) return;
   aPad->cd();
   aCanvas->Modified();
   aCanvas->Update();
   /*   if(myConfig.get<bool>("recoClusterEnable")) drawChargeAlongTrack3D(aPad);
	else  */get1DProjection(projection_type::DIR_TIME, filterType, scale_type::mm)->DrawCopy("hist");

   aCanvas->Modified();
   aCanvas->Update(); 
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawRecoFromMarkers(TCanvas *aCanvas, std::vector<double> * segmentsXY){

  reconstructSegmentsFromMarkers(segmentsXY);
  
  for(int strip_dir=projection_type::DIR_U;strip_dir<=projection_type::DIR_W;++strip_dir){
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
  filter_type filterType = filter_type::threshold;
  if(!myConfig.get<bool>("recoClusterEnable")) filterType = filter_type::none;

   for(int strip_dir=projection_type::DIR_U;strip_dir<=projection_type::DIR_W;++strip_dir){
     TVirtualPad *aPad = aCanvas->GetPad(padNumberOffset+strip_dir+1);
     if(!aPad) return;
     aPad->cd();
     aCanvas->Modified();
     aCanvas->Update();

     auto projType = get2DProjectionType(strip_dir);     
     auto histo2D = get2DProjection(projType, filterType, scale_type::mm);
     if(myConfig.get<bool>("recoClusterEnable")){
       histo2D->SetMinimum(0.0);
       histo2D->DrawCopy("colz");
       if(aPad->GetLogz()) histo2D->SetMinimum(1.0);
       hPlotBackground->Draw("col same");
       aPad->RedrawAxis();
       histo2D->DrawCopy("colz same");
       drawTrack3DProjectionTimeStrip(strip_dir, aPad, false);	    
     }
     else histo2D->DrawCopy("colz");
   }
   int strip_dir=3;
   TVirtualPad *aPad = aCanvas->GetPad(padNumberOffset+strip_dir+1);
   if(!aPad) return;
   aPad->cd();
   aCanvas->Modified();
   aCanvas->Update();
   if(myConfig.get<bool>("recoClusterEnable")) drawChargeAlongTrack3D(aPad);
   else  get1DProjection(projection_type::DIR_TIME, filterType, scale_type::mm)->DrawCopy("hist");

   aCanvas->Modified();
   aCanvas->Update();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::clearCanvas(TCanvas *aCanvas, bool isLogScaleOn){

  if(!aCanvas) return; 
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
  if(grDetLayoutAll) { delete grDetLayoutAll;} 
  if(grDetLayoutVeto) { delete grDetLayoutVeto; }
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
  std::tie(strip_min, strip_max)=myGeometryPtr->rangeStripDirInMM(projection_type::DIR_U);
  for(int idir=projection_type::DIR_V; idir<=projection_type::DIR_W; ++idir) {
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
std::shared_ptr<TH1D> HistoManager::get1DProjection(projection_type projType,
						    filter_type filterType,
						    scale_type scaleType){
  
  auto aHisto = myEventPtr->get1DProjection(projType, filterType, scaleType);
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
std::shared_ptr<TH2D> HistoManager::get2DProjection(projection_type projType,
						    filter_type filterType,
						    scale_type scaleType){

  auto aHisto = myEventPtr->get2DProjection(projType, filterType, scaleType);
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
const TH2D & HistoManager::getHoughAccumulator(int strip_dir, int iPeak){

  return myTkBuilder.getHoughtTransform(strip_dir);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawTrack3D(TVirtualPad *aPad){

  aPad->cd();
  int rebin_space=EVENTTPC_DEFAULT_STRIP_REBIN;
  int rebin_time=EVENTTPC_DEFAULT_TIME_REBIN; 
  TH3D *h3DFrame = myGeometryPtr->Get3DFrame(rebin_space, rebin_time);
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
  if(aTrack3D.getLength()<1) return;
  
  TH1F hFrame("hFrame",";d [mm];charge [arb. units]",2,-20, 20+aTrack3D.getLength());
  hFrame.GetYaxis()->SetTitleOffset(2.0);
  hFrame.SetMinimum(0.0);

  TH1F hChargeProfile = aTrack3D.getChargeProfile();
  //hChargeProfile = aTrack3D.getSegments().front().getChargeProfile();//TEST
  hChargeProfile.SetLineWidth(2);
  hChargeProfile.SetLineColor(2);
  hChargeProfile.SetMarkerColor(2);
  hChargeProfile.SetMarkerSize(1.0);
  hChargeProfile.SetMarkerStyle(20);

  hFrame.SetMaximum(1.2*hChargeProfile.GetMaximum());  
  hFrame.DrawCopy();
  hChargeProfile.DrawCopy("same HIST P");
  /*
  TH1F hChargeProfile1 = aTrack3D.getSegments().back().getChargeProfile();
  hChargeProfile1.Scale(scale);
  hChargeProfile1.SetLineWidth(2);
  hChargeProfile1.SetLineColor(3);
  hChargeProfile1.SetMarkerColor(3);
  hChargeProfile1.SetMarkerSize(1.0);
  hChargeProfile1.SetMarkerStyle(20);
  hChargeProfile1.DrawCopy("same HIST P");
  return;
  */
  TLegend *aLegend = new TLegend(0.7, 0.75, 0.95,0.95);
  fObjClones.push_back(aLegend);
  
  TF1 dEdx = myTkBuilder.getdEdx();
  if(!dEdx.GetNpar()) return;
  double carbonScale = dEdx.GetParameter("carbonScale");
  
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

  double alphaRange = aTrack3D.getSegments().front().getLength();
  double carbonRange = 0.0;
  if( aTrack3D.getSegments().size()==2){
    carbonRange = aTrack3D.getSegments().back().getLength();
  }
  double alphaEnergy = myRangeCalculator.getIonEnergyMeV(pid_type::ALPHA,alphaRange);   
  double carbonEnergy = myRangeCalculator.getIonEnergyMeV(pid_type::CARBON_12,carbonRange);  
  TLatex aLatex;
  double x = 0.1;
  double y = 0.91;
  aLatex.DrawLatexNDC(x,y,TString::Format("Total length [mm]: %3.0f",aTrack3D.getLength()));
  y = 0.95;
  aLatex.DrawLatexNDC(x,y,TString::Format("Total E [MeV]:        %2.1f",alphaEnergy+carbonEnergy));
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
  //  myRunParser.reset(new RunIdParser(fileName)); // HACK by MC
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::writeRecoData(unsigned long eventType){

  myEventInfo = myEventPtr->GetEventInfo(); 
  myEventInfo.SetEventType(eventType);				   
  myRecoOutput.setRecTrack(myTkBuilder.getTrack3D(0));
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
  Long64_t currentEventTime = myEventPtr->GetEventInfo().GetEventTimestamp()*1E-8;//[s]
  Long64_t currentEventNumber = myEventPtr->GetEventInfo().GetEventId();  
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
/*
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// Dot-like events useful for neutron flux monitoring
/////////////////////////////////////////////////////////
void HistoManager::initializeDotFinder(unsigned int hitThr,
				       unsigned int totalChargeThr,
				       double matchRadiusInMM,
				       const std::string & filePath) {
  myDotFinder.openOutputStream(filePath);
  myDotFinder.setCuts(hitThr, totalChargeThr, matchRadiusInMM);
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
*/
