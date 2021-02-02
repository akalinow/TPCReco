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

#include <MarkersManager.h>
#include <EntryDialog.h>
#include <MainFrame.h>
#include <HistoManager.h>
#include <ScrollFrame.h>

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
MarkersManager::MarkersManager(const TGWindow * p, MainFrame * aFrame)
 : TGCompositeFrame(p, 10, 10, kVerticalFrame), fParentFrame(aFrame){

   SetCleanup(kDeepCleanup);

   fTopFrame = new TGVerticalFrame(this, 300, 300);
   TGLayoutHints aLayoutHints(kLHintsTop | kLHintsLeft | kLHintsExpandY |
			      kLHintsShrinkX|kLHintsShrinkY |
			      kLHintsFillX|kLHintsFillY, 2, 2, 2, 2);
   AddFrame(fTopFrame, &aLayoutHints);
   /*
   TGGroupFrame *aHeaderFrame = new TGGroupFrame(fTopFrame, "Marker selection");
   fTopFrame->AddFrame(aHeaderFrame, new TGLayoutHints(kLHintsExpandX, 2, 2, 1, 1));
   TGLabel *aLabel = new TGLabel(aHeaderFrame,"U");
   aHeaderFrame->AddFrame(aLabel, new TGLayoutHints(kLHintsLeft, 2, 2, 1, 1));

   aLabel = new TGLabel(aHeaderFrame,"V");
   aHeaderFrame->AddFrame(aLabel, new TGLayoutHints(kLHintsLeft, 2, 2, 1, 1));

   aLabel = new TGLabel(aHeaderFrame,"W");
   aHeaderFrame->AddFrame(aLabel, new TGLayoutHints(kLHintsLeft, 2, 2, 1, 1));
   */
   fMarkerGCanvas = new TGCanvas(fTopFrame, 300, 300);
   TGCompositeFrame *aMarkerContainer = new TGCompositeFrame(fMarkerGCanvas->GetViewPort(), kVerticalFrame);
   fMarkerGCanvas->SetContainer(aMarkerContainer);
   fTopFrame->AddFrame(fMarkerGCanvas, new TGLayoutHints(kLHintsExpandX, 2, 2, 1, 1));
   fTopFrame->Layout();

   
   for(int iMarker=0;iMarker<4;++iMarker){
     addMarkerFrame(iMarker);
   }
  

   initialize();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
MarkersManager::~MarkersManager(){

  delete fMarkerGCanvas;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MarkersManager::initialize(){

  firstMarker = 0;
  secondMarker = 0;
  currentLine = 0;

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MarkersManager::reset(){

  if(firstMarker) delete firstMarker;
  if(secondMarker) delete secondMarker;
  if(currentLine) delete currentLine;
  firstMarker = 0;
  secondMarker = 0;
  currentLine = 0;
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
void MarkersManager::HandleMarkerPosition(Int_t event, Int_t x, Int_t y, TObject *sel){

  TObject *select = gPad->GetSelected();
    std::string objName = "";
    if(select) objName = std::string(select->GetName());
    
    if(event == kButton1 && objName.find("vs_time")!=std::string::npos){      
      std::cout<<"select->GetName(): "<<objName<<std::endl;
      TH2F *aHisto = (TH2F*)gROOT->FindObject(objName.c_str());
      double minY = 0.0;
      double maxY = 70.0;
      if(aHisto){
	aHisto->Print();
	minY = aHisto->GetYaxis()->GetXmin();
	maxY = aHisto->GetYaxis()->GetXmax();
      }
      TVirtualPad *aCurrentPad = gPad->GetSelectedPad();
      aCurrentPad->cd();
      
      float localX = aCurrentPad->AbsPixeltoX(x);
      float localY = aCurrentPad->AbsPixeltoY(y);
      std::cout<<"localX: "<<localX<<std::endl;
      std::cout<<"localY: "<<localY<<std::endl;

      if(firstMarker && !secondMarker && std::abs(localX-firstMarker->GetX())<10){
	  localX = firstMarker->GetX();
	  if(currentLine){
	    delete currentLine;
	    currentLine = 0;
	  }
      }
      else if(firstMarker && secondMarker){
	return;
      }
      else if(firstMarker){
	return;
      }

      
      int aMarkerColor = 2;
      TMarker *aMarker = new TMarker(localX, localY, 8);
      aMarker->SetMarkerColor(aMarkerColor);
      aMarker->SetMarkerSize(1);
      aMarker->Draw();
      if(!firstMarker) {
	firstMarker = aMarker;
	addMarkerFrame(0);
      }
      else if(!secondMarker) secondMarker = aMarker;

      if(!secondMarker){
	currentLine = new TLine(localX, minY, localX, maxY);
	currentLine->SetLineColor(aMarkerColor);
	currentLine->SetLineWidth(2);
	
	TPad *aPad_1 = (TPad*)gROOT->FindObject("Histograms_1");
	aPad_1->cd();
	currentLine->Draw();
	
	TPad *aPad_2 = (TPad*)gROOT->FindObject("Histograms_2");
	aPad_2->cd();
	currentLine->Draw();

	TPad *aPad_3 = (TPad*)gROOT->FindObject("Histograms_3");
	aPad_3->cd();
	currentLine->Draw();
      }
    }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Bool_t MarkersManager::ProcessMessage(Long_t msg, Long_t parm1, Long_t /*parm2*/){
   switch (GET_MSG(msg)) {
   case kC_COMMAND:
      {
         switch (GET_SUBMSG(msg)) {
         case kCM_BUTTON:
            {
               switch (parm1) {
                  // exit button
               case 1:
                  {
                     std::cout<<"MarkersManager::ProcessMessage(): msg: "<<msg<<std::endl;
                     break;
                  }
                  // set button
               case 2:
                  {
                     break;
                  }
               }
               break;
            }
         }
         break;
      }
   }
   return kTRUE;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
