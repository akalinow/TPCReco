#include <cstdlib>
#include <iostream>
#include <fstream>

#include <TApplication.h>
#include <MainFrame.h>
#include <SelectionBox.h>

#include <TStyle.h>
#include <TFrame.h>
#include <TVirtualX.h>

#include <TH2D.h>
#include <TH3D.h>
#include <TLatex.h>

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
MainFrame::MainFrame(const TGWindow *p, UInt_t w, UInt_t h)
      : TGMainFrame(p, w, h){

  fSelectionBox = 0;
  fArrow = 0;
  fLine = 0;

  SetCleanup(kDeepCleanup);
  SetWMPosition(500,0);
  SetWMSize(1200,700);
  
  AddTopMenu();
  SetTheFrame();
  AddHistoCanvas();
  AddButtons();
  AddNumbersDialog();

  MapSubwindows();
  Resize();
  MapWindow();
  SetWindowName("TPC GUI");

  //TEST ---
  std::string fileName = "/scratch_local/akalinow/ELITPC/TPCReco/build/resources/EventTPC_1.root";
  myDataManager.loadGeometry("/home/akalinow/scratch/ELITPC/TPCReco/build/resources/geometry_mini_eTPC.dat");
  myDataManager.loadDataFile(fileName);
  fEntryDialog->updateFileName(fileName);
  myDataManager.loadEvent(0);

  fCanvas->Clear();
  fCanvas->Divide(3,3);
  TText aMessage(0.2, 0.5,"Waiting for data.");
  for(int iPad=1;iPad<=9;++iPad){
    fCanvas->cd(iPad);
    aMessage.DrawText(0.2, 0.5,"Waiting for data.");
  }
  //////
 }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
MainFrame::~MainFrame(){
   // Delete all created widgets.
   delete fMenuFile;
   delete fMenuHelp;
   delete fMenuBar;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::AddTopMenu(){

  TGLayoutHints * menuBarItemLayout = new TGLayoutHints(kLHintsTop | kLHintsLeft, 0, 4, 0, 0);
  TGLayoutHints * menuBarHelpLayout = new TGLayoutHints(kLHintsTop | kLHintsRight);

  fMenuFile = new TGPopupMenu(fClient->GetRoot());
  fMenuFile->AddEntry("&Open...", M_FILE_OPEN);
  fMenuFile->AddEntry("S&ave as...", M_FILE_SAVEAS);
  fMenuFile->AddSeparator();
  fMenuFile->AddEntry("E&xit", M_FILE_EXIT);
  fMenuFile->Connect("Activated(Int_t)", "MainFrame", this,"HandleMenu(Int_t)");

  fMenuHelp = new TGPopupMenu(fClient->GetRoot());
  fMenuHelp->AddEntry("&Contents", M_HELP_CONTENTS);
  fMenuHelp->AddEntry("&Search...", M_HELP_SEARCH);
  fMenuHelp->AddSeparator();
  fMenuHelp->AddEntry("&About", M_HELP_ABOUT);
  fMenuHelp->Connect("Activated(Int_t)", "MainFrame", this,"HandleMenu(Int_t)");

  fMenuBar = new TGMenuBar(this, 1, 1, kHorizontalFrame);
  fMenuBar->AddPopup("&File", fMenuFile, menuBarItemLayout);
  fMenuBar->AddPopup("&Help", fMenuHelp, menuBarHelpLayout);
  AddFrame(fMenuBar, new TGLayoutHints(kLHintsExpandX, 0, 0, 1, 0));

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::SetTheFrame(){

   fFrame = new TGCompositeFrame(this,400,400,kSunkenFrame);
   TGTableLayout* tlo = new TGTableLayout(fFrame, 6, 12, 1);
   fFrame->SetLayoutManager(tlo);
   fFrameLayout = new TGLayoutHints(kLHintsTop|kLHintsLeft|
                                          kLHintsExpandX|kLHintsExpandY);
   AddFrame(fFrame,fFrameLayout);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::AddHistoCanvas(){

    // The Canvas
   TRootEmbeddedCanvas* embeddedCanvas = new TRootEmbeddedCanvas("Histograms",fFrame,700,700);
   fTCanvasLayout = new TGTableLayoutHints(0,8,0,6,
                                 kLHintsExpandX|kLHintsExpandY |
                                 kLHintsShrinkX|kLHintsShrinkY |
                                 kLHintsFillX|kLHintsFillY);
   fFrame->AddFrame(embeddedCanvas, fTCanvasLayout);

   fCanvas = embeddedCanvas->GetCanvas();
   fCanvas->MoveOpaque(kFALSE);
   gStyle->SetOptStat(0);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::AddButtons(){

  std::vector<std::string> button_names = {"Next event", "Previous event", "Exit"};

  std::vector<std::string> tooltips =     {"Load the next event.",
					   "Load the previous event.",
					   "Close the application"};

  std::vector<unsigned int> button_id = {M_NEXT_EVENT, M_PREVIOUS_EVENT,  M_FILE_EXIT};

  for (unsigned int iButton = 0; iButton < button_names.size(); ++iButton) {
    TGTextButton* button = new TGTextButton(fFrame,
					    button_names[iButton].c_str(),
					    button_id[iButton]);
    
    TGTableLayoutHints *tloh = new TGTableLayoutHints(8,9, iButton, iButton+1,
						      kLHintsExpandX|kLHintsExpandY |
						      kLHintsShrinkX|kLHintsShrinkY |
						      kLHintsFillX|kLHintsFillY);
    fFrame->AddFrame(button,tloh);
    button->Resize(50, button->GetDefaultHeight());
    button->Connect("Clicked()","MainFrame",this,"DoButton()");
    button->SetToolTipText(tooltips[iButton].c_str());
   }
 }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::AddNumbersDialog(){

  fEntryDialog = new EntryDialog(fFrame, this);
  TGTableLayoutHints *tloh = new TGTableLayoutHints(9,12,0,2,
						    kLHintsExpandX|kLHintsExpandY |
						    kLHintsShrinkX|kLHintsShrinkY |
						    kLHintsFillX|kLHintsFillY);
  fEntryDialog->initialize();  
  fFrame->AddFrame(fEntryDialog,tloh);

 }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::CloseWindow(){
   // Got close message for this MainFrame. Terminate the application
   // or returns from the TApplication event loop (depending on the
   // argument specified in TApplication::Run()).

   gApplication->Terminate(0);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::Update(){

  myHistoManager.setEvent(myDataManager.getCurrentEvent());
  fEntryDialog->updateEventNumbers(myDataManager.numberOfEvents(),
				   myDataManager.currentEventNumber());
  
  fCanvas->Clear();
  fCanvas->Divide(3,3);
  fCanvas->cd(1);

  for(int aDir=0;aDir<3;++aDir){
    myHistoManager.getHoughAccumulator(aDir);
  }
  
  for(int aDir=0;aDir<3;++aDir){
    fCanvas->cd(aDir+1);  
    myHistoManager.getRawStripVsTime(aDir)->DrawClone("colz");
    fCanvas->cd(aDir+1+3);  
    myHistoManager.getFilteredStripVsTime(aDir)->DrawClone("colz");
    fCanvas->cd(aDir+1+3+3);  
    myHistoManager.getHoughAccumulator(aDir)->DrawClone("colz");
    fCanvas->cd(aDir+1+3);  
    myHistoManager.getTrackSeed(aDir).DrawClone();

    myHistoManager.getLineProjection(aDir).DrawClone();
    //myHistoManager.getHoughAccumulator(aDir, 1);
    //myHistoManager.getTrackSeed(aDir).DrawClone();
    
  }
  /*
  TVirtualPad *aPad = fCanvas->cd(7);
  TText aMessage(0.2, 0.5,"Calculating 3D scence");
  aMessage.DrawClone();
  aPad->Update();
  TH3D *h3DReco =  myHistoManager.get3DReconstruction();
  if(h3DReco){
    aPad->Clear();
    h3DReco->DrawClone("box2z");
  }
  else {
    aPad->Clear();  
    aMessage.DrawText(0.2, 0.5, "3D scene not available.");
  }
  aPad->Update();
  */  
  fCanvas->Update();  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Bool_t MainFrame::ProcessMessage(Long_t msg, Long_t parm1, Long_t){
   // Handle messages send to the MainFrame object. E.g. all menu button
   // messages.

   return kTRUE;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Bool_t MainFrame::ProcessMessage(Long_t msg){

  std::cout<<__FUNCTION__<<" msg: "<<msg<<std::endl;

   return kTRUE;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::HandleEmbeddedCanvas(Int_t event, Int_t x, Int_t y,
                                      TObject *sel){
  //TObject *select = gPad->GetSelected();

}
////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::HandleMenu(Int_t id){

  const char *filetypes[] = {
			     "ROOT files",    "*.root",
			     "All files",     "*",
			     0,               0
  };

  switch (id) {
  case M_FILE_OPEN:
    {
      TGFileInfo fi;
      fi.fFileTypes = filetypes;
      fi.fIniDir    = StrDup(".");
      new TGFileDialog(gClient->GetRoot(), this, kFDOpen, &fi);
      std::string fileName;
      if(fi.fFilename) fileName.append(fi.fFilename);
      myDataManager.loadDataFile(fileName);
      fEntryDialog->updateFileName(fileName);
      Update();
    }
    break;

  case M_FILE_SAVEAS:
    {
      TGFileInfo fi;
      fi.fFileTypes = filetypes;
      fi.fIniDir    = StrDup(".");
      fi.fFilename   = StrDup("selections.dat");
      new TGFileDialog(gClient->GetRoot(), this, kFDSave, &fi);
      std::string fileName;
      if(fi.fFilename) fileName.append(fi.fFilename);
    }
    break;
  case M_NEXT_EVENT:
    {
     myDataManager.getNextEvent();      
      Update();
    }
    break;
  case M_PREVIOUS_EVENT:
    {
      myDataManager.getPreviousEvent();
      Update();
    }
    break;

  case M_FILE_EXIT:
    CloseWindow();   // terminate theApp no need to use SendCloseMessage()
    break;
  }
}
////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::DoButton(){
 TGButton* button = (TGButton*)gTQSender;
   UInt_t button_id = button->WidgetId();

   HandleMenu(button_id);
 }
////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

