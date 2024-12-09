#include <cstdlib>
#include <iostream>
#include <tuple>

#include <TCanvas.h>
#include <TH2D.h>
#include <TH3D.h>
#include <TSpectrum2.h>
#include <TVector3.h>
#include <TPolyLine3D.h>
#include <TView.h>
#include <TVirtualViewer3D.h>
#include <TF1.h>
#include <TLegend.h>
#include <TText.h>
#include <TPaletteAxis.h>
#include <TLatex.h>
#include <TLorentzVector.h>
#include <TMarker.h>

#include "TPCReco/CommonDefinitions.h"
#include "TPCReco/MakeUniqueName.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/EventTPC.h"
#include "TPCReco/RunIdParser.h"
#include "TPCReco/colorText.h"

#include "TPCReco/HistoManager.h"
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
  
  for(int strip_dir=definitions::projection_type::DIR_U;strip_dir<=definitions::projection_type::DIR_W;++strip_dir){
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
    getEventRateGraph()->DrawClone("AP");
  } else{
    get1DProjection(definitions::projection_type::DIR_TIME, filter_type::none, scale_type::raw)->DrawCopy("hist");
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
  if(!myConfig.get<bool>("hitFilter.recoClusterEnable")) filterType = filter_type::none;

   for(int strip_dir=definitions::projection_type::DIR_U;strip_dir<=definitions::projection_type::DIR_W;++strip_dir){
     TVirtualPad *aPad = aCanvas->GetPad(padNumberOffset+strip_dir+1);
     if(!aPad) return;
     aPad->cd();
     aCanvas->Modified();
     aCanvas->Update();

     auto projType = get2DProjectionType(strip_dir);     
     auto histo2D = get2DProjection(projType, filterType, scale_type::mm);
     aPad->SetFrameFillColor(kAzure-6);
     /*
     if(myConfig.get<bool>("hitFilter.recoClusterEnable")){
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
   /*   if(myConfig.get<bool>("hitFilter.recoClusterEnable")) drawChargeAlongTrack3D(aPad);
	else  */get1DProjection(definitions::projection_type::DIR_TIME, filterType, scale_type::mm)->DrawCopy("hist");

   aCanvas->Modified();
   aCanvas->Update(); 
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawRecoFromMarkers(TCanvas *aCanvas, std::vector<double> * segmentsXY){

  reconstructSegmentsFromMarkers(segmentsXY);
  
  for(int strip_dir=definitions::projection_type::DIR_U;strip_dir<=definitions::projection_type::DIR_W;++strip_dir){
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
  //TEST filter_type filterType = filter_type::threshold;
  //TEST if(!myConfig.get<bool>("hitFilter.recoClusterEnable")) filterType = filter_type::none;

   for(int strip_dir=definitions::projection_type::DIR_U;strip_dir<=definitions::projection_type::DIR_W;++strip_dir){
     TVirtualPad *aPad = aCanvas->GetPad(padNumberOffset+strip_dir+1);
     if(!aPad) return;
     aPad->cd();
     aCanvas->Modified();
     aCanvas->Update();
     
     //TEST auto projType = get2DProjectionType(strip_dir);     
     //TEST auto histo2D = get2DProjection(projType, filterType, scale_type::mm);
     auto histo2D = (TH2D*)(&myTkBuilder.getRecHits2D(strip_dir));
     if(doAutozoom) makeAutozoom(histo2D);

     aPad->SetFrameFillColor(kAzure-6);
     
     if(myConfig.get<bool>("hitFilter.recoClusterEnable")){
       histo2D->SetMinimum(0.0);
       histo2D->DrawCopy("colz");
       if(aPad->GetLogz()) histo2D->SetMinimum(1.0);
       hPlotBackground->Draw("col same");
       aPad->RedrawAxis();
       histo2D->DrawCopy("colz same");
     }
     else{
        histo2D->DrawCopy("colz");
     }
     
     drawTrack3DProjectionTimeStrip(strip_dir, aPad, false);
   }
   int strip_dir=3;
   TVirtualPad *aPad = aCanvas->GetPad(padNumberOffset+strip_dir+1);
   if(!aPad) return;
   aPad->cd();
   aCanvas->Modified();
   aCanvas->Update();

   if(myTkBuilder.getTrack3D(0).getSegments().front().getPID()==pid_type::DOT) drawTrack3DProjectionXY(aPad);
   else drawChargeAlongTrack3D(aPad);

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
    aMessage.DrawTextNDC(0.3, 0.5,"Waiting for data.");
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
void HistoManager::clearObjects(){
  
  for(auto aObj : fObjClones){
    if(aObj) aObj->Delete();
  }
  fObjClones.clear();
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
  std::tie(strip_min, strip_max)=myGeometryPtr->rangeStripDirInMM(definitions::projection_type::DIR_U);
  for(int idir=definitions::projection_type::DIR_V; idir<=definitions::projection_type::DIR_W; ++idir) {
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
std::shared_ptr<TH1D> HistoManager::get1DProjection(definitions::projection_type projType,
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
std::shared_ptr<TH2D> HistoManager::get2DProjection(definitions::projection_type projType,
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
    aHisto->SetMarkerColorAlpha(kBlack, 0.1);
    aHisto->SetFillColorAlpha(kBlack, 0.1);
    aHisto->SetLineColorAlpha(kBlack, 0.1);
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
void HistoManager::createWirePlotDriftCage3D(std::unique_ptr<TPad> &aPad) {

    if(!aPad) return;
    aPad->cd();

    // make wire plot of drift cage in 3D
    TView *view=TView::CreateView(1);
    double xmin, xmax, ymin, ymax, zmin, zmax;
    std::tie(xmin, xmax, ymin, ymax, zmin, zmax)=myGeometryPtr->rangeXYZ();

    auto view_span=0.8*std::max(std::max(xmax-xmin, ymax-ymin), zmax-zmin);
    view->SetRange(0.5*(xmax+xmin)-0.5*view_span, 0.5*(ymax+ymin)-0.5*view_span, 0.5*(zmax+zmin)-0.5*view_span,
                   0.5*(xmax+xmin)+0.5*view_span, 0.5*(ymax+ymin)+0.5*view_span, 0.5*(zmax+zmin)+0.5*view_span);
    view->ShowAxis(); 
    // plot active volume's faces
    TGraph gr=myGeometryPtr->GetActiveAreaConvexHull();
    TPolyLine3D l(5*(gr.GetN()-1));
    for(auto iedge=0; iedge<gr.GetN()-1; iedge++) {
        l.SetPoint(iedge*5+0, gr.GetX()[iedge], gr.GetY()[iedge], zmin);
        l.SetPoint(iedge*5+1, gr.GetX()[iedge+1], gr.GetY()[iedge+1], zmin);
        l.SetPoint(iedge*5+2, gr.GetX()[iedge+1], gr.GetY()[iedge+1], zmax);
        l.SetPoint(iedge*5+3, gr.GetX()[iedge], gr.GetY()[iedge], zmax);
        l.SetPoint(iedge*5+4, gr.GetX()[iedge], gr.GetY()[iedge], zmin);
    }
    l.SetLineColor(kBlack);
    l.SetLineWidth(3);
    l.DrawClone();
   
    /// beam line
    TPolyLine3D l_beam(2);
    l_beam.SetPoint(0, xmin-20, 0.0, 0.5*(zmin+zmax));
    l_beam.SetPoint(1, xmax+20, 0.0, 0.5*(zmin+zmax));
    l_beam.SetLineColor(kGreen+2);
    l_beam.SetLineWidth(2);
    l_beam.SetLineStyle(3);
    l_beam.DrawClone();

    aPad->Update();
    aPad->Modified();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawTrack3D(TVirtualPad *aPad){

  aPad->cd();
  
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
   }
    aPad->Update();
    aPad->Modified();
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

    TMarker aMarker(start.X(), start.Y(), 20);
    aMarker.SetMarkerSize(1.0);
    aMarker.SetMarkerStyle(20);

    aMarker.SetMarkerColor(kWhite);
    aMarker.DrawMarker(start.X(), start.Y());
    aMarker.SetMarkerColor(kBlack);
    aMarker.DrawMarker(end.X(), end.Y());

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

    TMarker aMarker(start.X(), start.Y(), 20);
    aMarker.SetMarkerSize(1.0);
    aMarker.SetMarkerStyle(20);

    aMarker.SetMarkerColor(kWhite);
    aMarker.DrawMarker(start.X(), start.Y());
    aMarker.SetMarkerColor(kBlack);
    aMarker.DrawMarker(end.X(), end.Y());

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
  
  TH1F hFrame("hFrame",";d [mm];charge/mm [arb. units]",2,-20, 20+aTrack3D.getLength());
  hFrame.GetYaxis()->SetTitleOffset(2.0);
  hFrame.SetMinimum(0.0);

  TH1F hChargeProfile = aTrack3D.getChargeProfile();
  hChargeProfile.SetLineWidth(2);
  hChargeProfile.SetLineColor(2);
  hChargeProfile.SetMarkerColor(2);
  hChargeProfile.SetMarkerSize(1.0);
  hChargeProfile.SetMarkerStyle(20);

  hFrame.SetMaximum(1.2*hChargeProfile.GetMaximum());  
  hFrame.DrawCopy();
  hChargeProfile.DrawCopy("same HIST P");

  TLegend *aLegend = new TLegend(0.7, 0.75, 0.95,0.95);
  
  TF1 dEdx = myTkBuilder.getdEdx();
  if(!dEdx.GetNpar()) return;
  const double points_per_mm = 100;
  dEdx.SetNpx((dEdx.GetXmax()-dEdx.GetXmin())*points_per_mm);
  double carbonScale = dEdx.GetParameter("carbonScale");
  
  dEdx.SetLineColor(kBlack);
  dEdx.SetLineStyle(1);
  dEdx.SetLineWidth(3);
  TObject *aObj = dEdx.DrawCopy("same");
  aLegend->AddEntry(aObj,"^{12}C + #alpha","l");

  dEdx.SetParameter("carbonScale",0.0);
  dEdx.SetLineColor(kRed);
  dEdx.SetLineStyle(2);
  dEdx.SetLineWidth(2);
  TObject *aObj1 = dEdx.DrawCopy("same");
  if((TF1*)aObj1) ((TF1*)aObj1)->SetName("alpha_model");
  aLegend->AddEntry(aObj1,"#alpha","l");
  
  dEdx.SetParameter("alphaScale",0.0);
  dEdx.SetParameter("carbonScale",carbonScale);
  dEdx.SetLineColor(kBlue-9);
  dEdx.SetLineStyle(2);
  dEdx.SetLineWidth(2);
  TObject *aObj2  = dEdx.DrawCopy("same");
  if((TF1*)aObj2) ((TF1*)aObj2)->SetName("carbon_model");
  aLegend->AddEntry(aObj2,"^{12}C","l");
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
    if(iAxis==1) aHisto->GetXaxis()->SetRange(std::max(lowBin-margin,1) , std::min(highBin+margin, aHisto->GetXaxis()->GetNbins()));
    else if(iAxis==2) aHisto->GetYaxis()->SetRange(std::max(lowBin-margin,1), std::min(highBin+margin, aHisto->GetYaxis()->GetNbins()));
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
