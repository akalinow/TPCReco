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
#include "ScrollFrame.h"
#include "CommonDefinitions.h"

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
MarkersManager::MarkersManager(const TGWindow * p, MainFrame * aFrame)
 : TGCompositeFrame(p, 10, 10, kVerticalFrame), fParentFrame(aFrame){

   SetCleanup(kDeepCleanup);

   int nRows = 3;
   int nColumns = 3;
   fHeaderFrame = new TGGroupFrame(this, "Segment creation");
   TGTableLayout* tlo = new TGTableLayout(fHeaderFrame, nRows, nColumns, 1);
   fHeaderFrame->SetLayoutManager(tlo);
   AddFrame(fHeaderFrame, new TGLayoutHints(kLHintsExpandX, 2, 2, 1, 1));
   addButtons();
   /*
   addSegmentButton = new TGTextButton(fHeaderFrame,"Add segment", M_ADD_SEGMENT);
   ULong_t aColor = TColor::RGB2Pixel(255, 255, 26);
   addSegmentButton->ChangeBackground(aColor);
   addSegmentButton->Connect("Clicked()","MarkersManager",this,"DoButton()");

   UInt_t attach_left=0, attach_right=1;
   UInt_t attach_top=0,  attach_bottom=1;
   UInt_t padLeft = 0;
   TGTableLayoutHints *aTableLayout = new TGTableLayoutHints(attach_left, attach_right,
							      attach_top, attach_bottom,
							     kLHintsFillX|kLHintsFillY,
							     padLeft);
   fHeaderFrame->AddFrame(addSegmentButton, aTableLayout);
   
   fitButton = new TGTextButton(fHeaderFrame,"Fit segments", M_FIT);
   aColor = TColor::RGB2Pixel(255, 255, 26);
   fitButton->ChangeBackground(aColor);
   fitButton->Connect("Clicked()","MarkersManager",this,"DoButton()");
   fitButton->SetState(kButtonDisabled);
   attach_left=1;
   attach_right=2;   
   padLeft = 5;
   aTableLayout = new TGTableLayoutHints(attach_left, attach_right,
					 attach_top, attach_bottom,
					 kLHintsFillX|kLHintsFillY,
					 padLeft);
   fHeaderFrame->AddFrame(fitButton, aTableLayout);
   */
   initialize();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MarkersManager::addButtons(){

  std::vector<std::string> button_names = {"Add segment", "Fit segments", "Save segments"};
  std::vector<std::string> button_tooltips = {"Click to add segments end points. Each point is set by coordinates on two projections",
					      "Calculate 3D orientation from the 2D projections",
					      "Save the segments to ROOT file"};
  std::vector<unsigned int> button_id = {M_ADD_SEGMENT, M_FIT_SEGMENT,  M_WRITE_SEGMENT};

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
    if(button_names[iButton]=="Save segments") aButton->Connect("Clicked()","MainFrame",fParentFrame,"DoButton()");
    else aButton->Connect("Clicked()","MarkersManager",this,"DoButton()");
    if(button_names[iButton]!="Add segment") aButton->SetState(kButtonDisabled);
    aButton->ChangeBackground(aColor);
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
  acceptPoints = false;
  setEnabled(false);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MarkersManager::setEnabled(bool enable){
  
  //reset();
  if(!enable && myButtons.find("Add segment")!=myButtons.end() &&
     myButtons.find("Fit segments")!=myButtons.end() &&
     myButtons.find("Save segments")!=myButtons.end()){
    myButtons.find("Add segment")->second->SetState(kButtonDisabled);
    myButtons.find("Fit segments")->second->SetState(kButtonDisabled);
    myButtons.find("Save segments")->second->SetState(kButtonDisabled);
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
    double x = fMarkersContainer.at(strip_dir)->GetX();
    double y = fMarkersContainer.at(strip_dir)->GetY();
    TLine aLine(x, y, x, y);
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

  clearHelperLines();

  int strip_dir = DIR_U;//TEST
  if(force || isLastSegmentComplete(strip_dir)){
    acceptPoints = false;
    if(myButtons.find("Add segment")!=myButtons.end() &&
       myButtons.find("Fit segments")!=myButtons.end() &&
       myButtons.find("Save segments")!=myButtons.end()){
      myButtons.find("Add segment")->second->SetState(kButtonUp);
      myButtons.find("Fit segments")->second->SetState(kButtonUp);
      myButtons.find("Save segments")->second->SetState(kButtonDisabled);
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
/*
void MarkersManager::addHeaderFrame(){
TGHorizontalFrame *aHorizontalFrame = new TGHorizontalFrame(fMarkerGCanvas->GetContainer(), 200, 30);
  
}
*/
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
  
  if(strip_dir<DIR_U || strip_dir>=(int)fMarkersContainer.size() || fMarkersContainer.at(strip_dir)) return;  
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

  ///Not implemented yet.
  //#ANGLES: 90.0 -30.0 30.0
  const std::vector<double> phiPitchDirection = {M_PI, -M_PI/6.0 + M_PI/2.0, M_PI/6.0 - M_PI/2.0};

  double x = 0.0, y = 0.0;
  double xDenominator = 0.0, yDenominator = 0.0;
  int sign = 1;
  for(int strip_dir=DIR_U;strip_dir<=DIR_W;++strip_dir){
    TMarker *item = fMarkersContainer.at(strip_dir);
    if(!item) continue;     
    unsigned int next_strip_dir = (strip_dir+1)%3;
    if(next_strip_dir==missingMarkerDir){ next_strip_dir = (next_strip_dir+1)%3;}
    
    x += sign*item->GetY()*sin(phiPitchDirection[next_strip_dir]);					   
    xDenominator += sign*cos(phiPitchDirection[strip_dir])*sin(phiPitchDirection[next_strip_dir]);

    y += sign*item->GetY()*cos(phiPitchDirection[next_strip_dir]);					   
    yDenominator += sign*sin(phiPitchDirection[strip_dir])*cos(phiPitchDirection[next_strip_dir]);
   
    sign *=-1;
  }
  x /= xDenominator;
  y /= yDenominator;
  double strip_value = x*cos(phiPitchDirection[missingMarkerDir]) +
    y*sin(phiPitchDirection[missingMarkerDir]);
  return strip_value; 
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
Bool_t MarkersManager::HandleButton(Int_t id){
   switch (id) {
   case M_ADD_SEGMENT:
    {
      acceptPoints = true;
      if(myButtons.find("Add segment")!=myButtons.end() &&
	 myButtons.find("Fit segments")!=myButtons.end()){
	myButtons.find("Add segment")->second->SetState(kButtonDown);
	myButtons.find("Fit segments")->second->SetState(kButtonDisabled);
      }
    }
    break;
   case M_FIT_SEGMENT:
     {
       repackSegmentsData();
       sendSegmentsData(&fSegmentsXY);
       if(myButtons.find("Save segments")!=myButtons.end()){
	 myButtons.find("Save segments")->second->SetState(kButtonUp);
       }
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
/////////////////////////////////////////////////////////
