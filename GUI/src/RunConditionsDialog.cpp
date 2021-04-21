#include <cstdlib>
#include <iostream>

#include <TGFrame.h>
#include <TGLabel.h>
#include <TGTextEntry.h>
#include <TGNumberEntry.h>
#include <TGCanvas.h>
#include <TGTableLayout.h>
#include <TGFontDialog.h>
#include <TFrame.h>

#include "colorText.h"
#include "GUI_commons.h"
#include "MarkersManager.h"
#include "MainFrame.h"
#include "HistoManager.h"
#include "ScrollFrame.h"
#include "CommonDefinitions.h"

#include <RunConditionsDialog.h>
#include <MainFrame.h>

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
RunConditionsDialog::RunConditionsDialog(const TGWindow * p, MainFrame * aFrame)
 : TGCompositeFrame(p, 10, 10, kHorizontalFrame), fParentFrame(aFrame){

   SetCleanup(kDeepCleanup);

   int nRows = 3;
   int nColumns = 3;
   fHeaderFrame = new TGGroupFrame(this, "Run conditions manager");
   TGTableLayout* tlo = new TGTableLayout(fHeaderFrame, nRows, nColumns, 1);
   fHeaderFrame->SetLayoutManager(tlo);
   AddFrame(fHeaderFrame, new TGLayoutHints(kLHintsExpandX, 2, 2, 1, 1));
   addButtons();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
RunConditionsDialog::~RunConditionsDialog(){
   // dtor
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void RunConditionsDialog::initialize(const RunConditions & aRunConditions){

  double value = aRunConditions.getDriftVelocity();
  std::map<std::string, TGNumberEntry*>::iterator aItem = myDialogs.find("vdrift");
  if(aItem!=myDialogs.end()) aItem->second->GetNumberEntry()->SetNumber(value);

  value = aRunConditions.getSamplingRate();
  aItem = myDialogs.find("sampling");
  if(aItem!=myDialogs.end()) aItem->second->GetNumberEntry()->SetNumber(value);
 
  value = aRunConditions.getTriggerDelay();
  aItem = myDialogs.find("delay");
  if(aItem!=myDialogs.end()) aItem->second->GetNumberEntry()->SetNumber(value);

  runParams.resize(3);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void RunConditionsDialog::addButtons(){

  UInt_t attach_left=0;
  UInt_t attach_right=attach_left+1;
  UInt_t attach_top=0;
  UInt_t attach_bottom=attach_top+1;

  std::vector<std::string> entry_names = {"vdrift", "sampling", "delay"};
  std::vector<std::string> entry_labels = {"Drift v\n[cm/us]", "Smplg. rate\n[MHz]", "Trg. delay\n[us]"};
  std::vector<unsigned int> button_id = {M_SET_V_DRIFT, M_SET_SAMLING_RATE, M_SET_TRG_DELAY};

  for (unsigned int iEntry = 0; iEntry <entry_names.size(); ++iEntry){

    attach_top=0;
    attach_bottom=attach_top+1;
    TGTableLayoutHints *tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom);
    TGLabel *aLabel = new TGLabel(fHeaderFrame, entry_labels[iEntry].c_str());
    fHeaderFrame->AddFrame(aLabel, tloh);

    attach_top++;
    attach_bottom=attach_top+1;
    tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom);
    TGNumberEntry *aNumberEntry = new TGNumberEntry(fHeaderFrame,0.0,5, button_id[iEntry],
						    TGNumberFormat::EStyle::kNESRealTwo);
    //aNumberEntry->Connect("ValueSet(Long_t)","RunConditionsDialog",this,"setConditions()");
    //aNumberEntry->Connect("ValueSet(Long_t)", "RunConditionsDialog", this, "setConditions()");
    //(aNumberEntry->GetNumberEntry())->Connect("ReturnPressed()", "RunConditionsDialog", this,"setConditions()");
    fHeaderFrame->AddFrame(aNumberEntry, tloh);
    myDialogs[entry_names[iEntry]] = aNumberEntry;    
    ++attach_left;
    ++attach_right;
  }

  attach_left=0;
  attach_right=attach_left+1;
  attach_top=2;
  attach_bottom=attach_top+1;
  ULong_t aColor = TColor::RGB2Pixel(30, 152, 189);
  TGTextButton* aButton = new TGTextButton(fHeaderFrame, "Update conditions", M_SET_CONDITIONS);
  TGTableLayoutHints *tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom);
  fHeaderFrame->AddFrame(aButton,tloh);
  aButton->ChangeBackground(aColor);
  aButton->Connect("Clicked()","RunConditionsDialog",this,"updateRunConditions()");
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void RunConditionsDialog::updateRunConditions(std::vector<double> *dummyArgs){

  Long_t args[1];
  args[0] = (Long_t)&runParams;

  runParams[0] = myDialogs["vdrift"]->GetNumberEntry()->GetNumber();
  runParams[1] = myDialogs["sampling"]->GetNumberEntry()->GetNumber();
  runParams[2] = myDialogs["delay"]->GetNumberEntry()->GetNumber();

  Emit("updateRunConditions(std::vector<double> *)", args);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
