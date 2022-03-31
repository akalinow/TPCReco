#include <cstdlib>
#include <iostream>

#include <TGResourcePool.h>
#include <TGFrame.h>
#include <TGLabel.h>
#include <TGTextEntry.h>
#include <TGNumberEntry.h>
#include <TGCanvas.h>
#include <TGTableLayout.h>
#include <TGFontDialog.h>
#include <TFrame.h>

#include "colorText.h"
#include "MarkersManager.h"
#include "MainFrame.h"
#include "HistoManager.h"
#include "CommonDefinitions.h"

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
MarkersManager::MarkersManager(const TGWindow * p, MainFrame * aFrame)
 : TGCompositeFrame(p, 10, 10, kVerticalFrame), fParentFrame(aFrame){

   SetCleanup(kDeepCleanup);

   int nRows = 2;
   int nColumns = 3;
   fHeaderFrame = new TGGroupFrame(this, "Segment creation");
   TGTableLayout* tlo = new TGTableLayout(fHeaderFrame, nRows, nColumns, 1);
   fHeaderFrame->SetLayoutManager(tlo);
   AddFrame(fHeaderFrame, new TGLayoutHints(kLHintsExpandX, 2, 2, 1, 1));
   addButtons();   
   initialize();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MarkersManager::addButtons(){
  
  std::vector<std::string> button_names = {"Clear track", "Add segment", "Fit segments"};
  std::vector<std::string> button_tooltips = {"Remove tracks from images. If \"Fit segments\" will NOT be clicked, \n the track will still be saved to the reco file",
					      "Click to add segments end point. \n All segments share a common vertex - the starting point of the first segment.\n Each point is set by coordinates on two projections",
					      "Calculate 3D orientation from the 2D projections"};
  std::vector<unsigned int> button_id = {M_CLEAR_TRACKS, M_ADD_SEGMENT, M_FIT_SEGMENT};

  ULong_t aColor = TColor::RGB2Pixel(255, 255, 26);
  UInt_t attach_left=0;
  UInt_t attach_right=attach_left+1;
  UInt_t attach_top=0;
  UInt_t attach_bottom=attach_top+1;

  for (unsigned int iButton = 0; iButton < button_names.size(); ++iButton) {
    TGTextButton* aButton = new TGTextButton(fHeaderFrame,
					    button_names[iButton].c_str(),
					    button_id[iButton]);
    TGTableLayoutHints *tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
    						      kLHintsFillX | kLHintsFillY);

    fHeaderFrame->AddFrame(aButton,tloh);
    aButton->Connect("Clicked()","MarkersManager",this,"DoButton()");
    if(button_names[iButton]!="Add segment") aButton->SetState(kButtonDisabled);
    if(button_names[iButton]=="Clear track") aButton->SetState(kButtonUp);
    aButton->ChangeBackground(aColor);
    aButton->SetToolTipText(button_tooltips[iButton].c_str());
    myButtons[button_names[iButton]] = aButton;
    ++attach_left;
    ++attach_right;
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
MarkersManager::~MarkersManager(){

  delete fMarkerGCanvas;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MarkersManager::initialize(){

  fMarkersContainer.resize(3);
  fHelperLinesContainer.resize(3);
  fSegmentsContainer.resize(3);

  firstMarker = 0;
  timeMarker = 0;
  acceptPoints = false;
  setEnabled(false);
  fGeometryTPC = NULL;

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MarkersManager::setEnabled(bool enable){

    for(auto item: myButtons){
        if(!enable) item.second->SetState(kButtonDisabled);	
    }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MarkersManager::updateSegments(int strip_dir){

  std::vector<TLine> &aSegmentsContainer = fSegmentsContainer.at(strip_dir);

  if(aSegmentsContainer.size() && !isLastSegmentComplete(strip_dir)){    
    TLine &aLine = aSegmentsContainer.back();
    double x = fMarkersContainer.at(strip_dir)->GetX();
    double y = fMarkersContainer.at(strip_dir)->GetY();
    aLine.SetX2(x);
    aLine.SetY2(y);
  }
  else{
    double x1 = fMarkersContainer.at(strip_dir)->GetX();
    double y1 = fMarkersContainer.at(strip_dir)->GetY();
    double x2 = x1;
    double y2 = y1;
    if(aSegmentsContainer.size()){
      x1 = aSegmentsContainer.front().GetX1();
      y1 = aSegmentsContainer.front().GetY1();
      x2 = fMarkersContainer.at(strip_dir)->GetX();
      y2 = fMarkersContainer.at(strip_dir)->GetY();
    }
    TLine aLine(x1, y1, x2, y2);
    aLine.SetLineWidth(3);
    aLine.SetLineStyle(2);
    aLine.SetLineColor(2+aSegmentsContainer.size());
    aSegmentsContainer.push_back(aLine);
  }

  std::string padName = "Histograms_"+std::to_string(strip_dir+1);
  TPad *aPad = (TPad*)gROOT->FindObject(padName.c_str());
  if(!aPad) return;
  aPad->cd();
  for(auto &item:aSegmentsContainer){
    item.Draw();
    TMarker aMarker(item.GetX1(), item.GetY1(), 21);
    aMarker.SetMarkerColor(item.GetLineColor());
    aMarker.DrawMarker(item.GetX1(), item.GetY1());
    aMarker.DrawMarker(item.GetX2(), item.GetY2());
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MarkersManager::reset(){

  resetMarkers(true);
  resetSegments();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MarkersManager::resetSegments(){

  std::for_each(fSegmentsContainer.begin(), fSegmentsContainer.end(),
		[](std::vector<TLine> &item){item.clear();});
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MarkersManager::setPadsEditable(bool isEditable){

  for(int strip_dir=DIR_U;strip_dir<=DIR_W;++strip_dir){
    std::string padName = "Histograms_"+std::to_string(strip_dir+1);
    TPad *aPad = (TPad*)gROOT->FindObject(padName.c_str());
    if(!aPad) continue;
    aPad->SetEditable(isEditable);
  }  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MarkersManager::resetMarkers(bool force){

  std::for_each(fMarkersContainer.begin(), fMarkersContainer.end(),
		[](TMarker *&item){if(item){delete item; item = 0;}});
  firstMarker = 0;
  timeMarker = 0;
  clearHelperLines();

  for(auto aObj : fObjClones) delete aObj;
  fObjClones.clear();

  int strip_dir = DIR_U;//FIX ME
  if(force || isLastSegmentComplete(strip_dir)){
    acceptPoints = false;
    if(myButtons.find("Add segment")!=myButtons.end() &&
       myButtons.find("Fit segments")!=myButtons.end() &&
       myButtons.find("Clear track")!=myButtons.end()){
      myButtons.find("Add segment")->second->SetState(kButtonUp);
      myButtons.find("Clear track")->second->SetState(kButtonUp);
      if(!force) myButtons.find("Fit segments")->second->SetState(kButtonUp);
      else myButtons.find("Fit segments")->second->SetState(kButtonDisabled);
    }
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MarkersManager::clearHelperLines(){

std::for_each(fHelperLinesContainer.begin(), fHelperLinesContainer.end(),
		[](TLine *&item){if(item){delete item; item = 0;}});  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MarkersManager::addMarkerFrame(int iMarker){

  TGHorizontalFrame *aHorizontalFrame = new TGHorizontalFrame(fMarkerGCanvas->GetContainer(), 200, 30);
  TGLayoutHints *aLayoutHints = new TGLayoutHints(kLHintsLeft, 2, 2, 2, 2);
  TGCompositeFrame *aCompositeFrame = (TGCompositeFrame*)fMarkerGCanvas->GetContainer();
  aCompositeFrame->AddFrame(aHorizontalFrame, aLayoutHints);

  float value = 1.0;
  TGNumberEntry *aNumberEntry = new TGNumberEntry(aHorizontalFrame, value, 5, 0,
						  TGNumberFormat::EStyle::kNESRealTwo);
  aNumberEntry->Connect("ValueSet(Long_t)","MainFrame",fParentFrame,"ProcessMessage(Long_t)");
  aNumberEntry->Associate(this);
  aHorizontalFrame->AddFrame(aNumberEntry, aLayoutHints);

  aNumberEntry = new TGNumberEntry(aHorizontalFrame, value, 5, 0,
				   TGNumberFormat::EStyle::kNESRealTwo);
  aNumberEntry->Connect("ValueSet(Long_t)","MainFrame",fParentFrame,"ProcessMessage(Long_t)");
  aNumberEntry->Associate(this);
  aHorizontalFrame->AddFrame(aNumberEntry, aLayoutHints);

  aNumberEntry = new TGNumberEntry(aHorizontalFrame, value, 5, 0,
				   TGNumberFormat::EStyle::kNESRealTwo);
  aNumberEntry->Connect("ValueSet(Long_t)","MainFrame",fParentFrame,"ProcessMessage(Long_t)");
  aNumberEntry->Associate(this);
  aHorizontalFrame->AddFrame(aNumberEntry, aLayoutHints);

  aNumberEntry = new TGNumberEntry(aHorizontalFrame, value, 5, 0,
				   TGNumberFormat::EStyle::kNESRealTwo);
  aNumberEntry->Connect("ValueSet(Long_t)","MainFrame",fParentFrame,"ProcessMessage(Long_t)");
  aNumberEntry->Associate(this);
  aHorizontalFrame->AddFrame(aNumberEntry, aLayoutHints);
 
  fMarkerGCanvas->Layout();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MarkersManager::processClickCoordinates(int strip_dir, float x, float y){
  /*
  if(strip_dir==3){
    if(!timeMarker){
      timeMarker = new TMarker(x, y, 1);
      drawFixedTimeLines(DIR_U, x);
      drawFixedTimeLines(DIR_W, x);
      drawFixedTimeLines(DIR_W, x);
    }
  }
  */
  if(strip_dir<DIR_U || strip_dir>=(int)fMarkersContainer.size() || fMarkersContainer.at(strip_dir)) return;
  //if(timeMarker){ x = timeMarker->GetX(); }
  if(firstMarker){ x = firstMarker->GetX(); }
  

  int iMarkerColor = 2;
  int iMarkerStyle = 8;
  int iMarkerSize = 1;
  TMarker *aMarker = new TMarker(x, y, iMarkerStyle);
  aMarker->SetMarkerColor(iMarkerColor);
  aMarker->SetMarkerSize(iMarkerSize);
  aMarker->Draw();
  fMarkersContainer.at(strip_dir) = aMarker;
  if(!firstMarker){
    firstMarker = aMarker;
    drawFixedTimeLines(strip_dir, x);
  }
  else{
    int missingMarkerDir = findMissingMarkerDir();
    y = getMissingYCoordinate(missingMarkerDir);
    aMarker = new TMarker(x, y, iMarkerStyle);
    aMarker->SetMarkerColor(iMarkerColor);
    aMarker->SetMarkerSize(iMarkerSize);
    fMarkersContainer.at(missingMarkerDir) = aMarker;    
    std::string padName = "Histograms_"+std::to_string(missingMarkerDir+1);
    TPad *aPad = (TPad*)gROOT->FindObject(padName.c_str());
    aPad->cd();
    aMarker->Draw();
    updateSegments(DIR_U);
    updateSegments(DIR_V);
    updateSegments(DIR_W);
    resetMarkers();
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MarkersManager::drawFixedTimeLines(int strip_dir, double time){

  clearHelperLines();
  int aColor = 1;
  TLine aLine(time, 0, time, 0);
  aLine.SetLineColor(aColor);
  aLine.SetLineWidth(2);
  
  for(int strip_dir=DIR_U;strip_dir<=DIR_W;++strip_dir){
    std::string padName = "Histograms_"+std::to_string(strip_dir+1);
    TPad *aPad = (TPad*)gROOT->FindObject(padName.c_str());
    if(!aPad) continue;
    aPad->cd();
    TFrame *hFrame = (TFrame*)aPad->GetListOfPrimitives()->At(0);
    if(!hFrame) continue;
    double minY = hFrame->GetY1();
    double maxY = hFrame->GetY2();
    fHelperLinesContainer[strip_dir] = aLine.DrawLine(time, minY, time, maxY);
  }  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int MarkersManager::findMissingMarkerDir(){

 for(int strip_dir=DIR_U;strip_dir<=DIR_W;++strip_dir){
    TMarker *item = fMarkersContainer.at(strip_dir);
    if(!item) return strip_dir;
    }
    return -1;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double MarkersManager::getMissingYCoordinate(unsigned int missingMarkerDir){

  bool err_flag=false;
  assert(fGeometryTPC); // DEBUG - TPC geometry should be defined at this point

  // find crossing point of 2 lines defined by 2 UVW coordinates (from 2 TMarkers)
  int dir[2] = { -1, -1 };
  double pos[2];
  int counter=0;
  for(unsigned int strip_dir=DIR_U;strip_dir<=DIR_W;++strip_dir){
    TMarker *item = fMarkersContainer.at(strip_dir);
    if(!item || strip_dir==missingMarkerDir) continue;
    dir[counter] = strip_dir;    // UVW direction index
    pos[counter] = item->GetY(); // UVW coordinate [mm]
    counter++;
  }
  if(counter!=2) {
    /////// DEBUG
    std::cout << "getMissingYCoordinate: 3rd_dir=" << missingMarkerDir << ": ERROR: Number of TMarkers <2" << std::endl;
    /////// DEBUG
    err_flag=true;
  }
  TVector2 point;

  if(!err_flag && fGeometryTPC->GetUVWCrossPointInMM(dir[0], pos[0], dir[1], pos[1], point)) {
    /////// DEBUG
    //    std::cout << "getMissingYCoordinate: 3rd_dir=" << missingMarkerDir << ":"
    //	      << " 1st_dir=" << dir[0] << ", 1st_coord[mm]=" << pos[0]
    //	      << ", 2nd_dir=" << dir[1] << ", 2nd_coord[mm]=" << pos[1]
    //	      << " Cartesian intersection point (X[mm],Y[mm])=("
    //	      << point.X() << ", " << point.Y() << ")" << std::endl;
    /////// DEBUG
  } else {
    /////// DEBUG
    std::cout << "getMissingYCoordinate: 3rd_dir=" << missingMarkerDir << ": ERROR: Cannot calculate intersection point from 2 UVW coordinates" << std::endl;
    /////// DEBUG
    err_flag=true;
  }

  if(!err_flag) {
    double strip_posUVW = fGeometryTPC->Cartesian2posUVW(point, missingMarkerDir, err_flag); // in [mm]
    if(!err_flag) {
      /////// DEBUG
      //      std::cout << "getMissingYCoordinate: 3rd_dir=" << missingMarkerDir << ": 3rd_coord[mm]=" << strip_posUVW << std::endl;
      /////// DEBUG
      return strip_posUVW;
    }
  }
  /////// DEBUG
  //std::cout << "getMissingYCoordinate: 3rd_dir=" << missingMarkerDir << ": ERROR: Cannot calculate 3rd UVW coordinate" << std::endl;
  /////// DEBUG
  return 0;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MarkersManager::HandleMarkerPosition(Int_t event, Int_t x, Int_t y, TObject *sel){

  if(!acceptPoints) return;
  TVirtualPad *aCurrentPad = gPad->GetSelectedPad();
  if(!aCurrentPad) return;
  std::string padName = std::string(aCurrentPad->GetName());
  if(event == kButton1 && padName.find("Histograms_")!=std::string::npos){
    aCurrentPad->cd();
    int strip_dir = stoi(padName.substr(11,1))-1;
    float localX = aCurrentPad->AbsPixeltoX(x);
    float localY = aCurrentPad->AbsPixeltoY(y);    
    processClickCoordinates(strip_dir, localX, localY);
    aCurrentPad->Update();
  }
  return;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MarkersManager::repackSegmentsData(){

  fSegmentsXY.clear();
  int nSegments = fSegmentsContainer.at(DIR_U).size();
  for(int iSegment=0;iSegment<nSegments;++iSegment){
    for(auto & strip_segments: fSegmentsContainer){
      TLine &aLine = strip_segments.at(iSegment);
      fSegmentsXY.push_back(aLine.GetX1());
      fSegmentsXY.push_back(aLine.GetY1());
      fSegmentsXY.push_back(aLine.GetX2());
      fSegmentsXY.push_back(aLine.GetY2());    
    }
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MarkersManager::sendSegmentsData(std::vector<double> *segmentsXY){
  
  Long_t args[1];
  args[0] = (Long_t)segmentsXY;
  
  Emit("sendSegmentsData(std::vector<double> *)", args);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Bool_t MarkersManager::HandleButton(Long_t id){
   switch (id) {
   case M_CLEAR_TRACKS:
    {
      Emit("HandleButton(Long_t)", id); 
    }
   break; 
   case M_ADD_VERTEX:
    {
      acceptPoints = true;
    }
   break; 
   case M_ADD_SEGMENT:
    {
      acceptPoints = true;
      if(myButtons.find("Add segment")!=myButtons.end() &&
	 myButtons.find("Fit segments")!=myButtons.end()){
	myButtons.find("Add segment")->second->SetState(kButtonDisabled);
	myButtons.find("Fit segments")->second->SetState(kButtonDisabled);
      }
    }
    break;
   case M_FIT_SEGMENT:
     {
       repackSegmentsData();
       sendSegmentsData(&fSegmentsXY);
       Emit("HandleButton(Int_t)", id);
       reset();
     }     
     break;
   }
   return kTRUE;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MarkersManager::DoButton(){
 TGButton* button = (TGButton*)gTQSender;
   UInt_t button_id = button->WidgetId();
   HandleButton(button_id);
 }
////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool MarkersManager::isLastSegmentComplete(int strip_dir){

  std::vector<TLine> &aSegmentsContainer = fSegmentsContainer.at(strip_dir);

  return !aSegmentsContainer.size() ||
    (std::abs(aSegmentsContainer.back().GetX1() - aSegmentsContainer.back().GetX2())>1E-3 &&
     std::abs(aSegmentsContainer.back().GetY1() - aSegmentsContainer.back().GetY2())>1E-3);
}
////////////////////////////////////////////////////////
///////////////////////////////////////////////////////// Added by MC - 19 Aug 2021
void MarkersManager::setGeometry(std::shared_ptr<GeometryTPC>geo){
    fGeometryTPC=geo;
}
////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
