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
#include <TProfile.h>

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
MainFrame::MainFrame(const TGWindow *p, UInt_t w, UInt_t h)
      : TGMainFrame(p, w, h){

  //TEST ---
  std::string dataFileName = "/scratch_local/akalinow/ELITPC/TPCReco/build/resources/EventTPC_1.root";
  std::string geometryFileName = "/home/akalinow/scratch/ELITPC/TPCReco/build/resources/geometry_mini_eTPC.dat";

  dataFileName = "/scratch_local/akalinow/ELITPC/TPCReco/build/EventTPC_1.root";
  geometryFileName = "/scratch_local/akalinow/ELITPC/TPCReco/build/EventTPC_1.root";

  dataFileName = "/home/akalinow/scratch/ELITPC/data/neutrons/EventTPC_2018-06-19T15:13:33.941.root";
  geometryFileName = "/home/akalinow/scratch/ELITPC/data/neutrons/geometry_mini_eTPC_2018-06-19T15:13:33.941.dat";

  myDataManager.loadGeometry(geometryFileName);  
  myDataManager.loadDataFile(dataFileName);
  myDataManager.loadEventId(10);
  //myDataManager.loadTreeEntry(0);
  myHistoManager.setGeometry(myDataManager.getGeometry());
  ////////////////////

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
  AddGoToEventDialog(4);
  AddNumbersDialog();

  MapSubwindows();
  Resize();
  MapWindow();
  SetWindowName("TPC GUI");

  fEntryDialog->updateFileName(dataFileName);
  
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
   TGTableLayout* tlo = new TGTableLayout(fFrame, 12, 12, 1);
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
   fTCanvasLayout = new TGTableLayoutHints(0, 8, 0, 12,
                                 kLHintsExpandX|kLHintsExpandY |
                                 kLHintsShrinkX|kLHintsShrinkY |
                                 kLHintsFillX|kLHintsFillY);
   fFrame->AddFrame(embeddedCanvas, fTCanvasLayout);

   fCanvas = embeddedCanvas->GetCanvas();
   fCanvas->MoveOpaque(kFALSE);
   gStyle->SetOptStat(0);
   gStyle->SetPalette(55);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::AddButtons(){

  int iColumn = 8;
  int iRow = 0;

  std::vector<std::string> button_names = {"Next event", "Previous event",  "Exit"};

  std::vector<std::string> tooltips =     {"Load the next event.",
					   "Load the previous event.",
					   "Close the application"
					   };

  std::vector<unsigned int> button_id = {M_NEXT_EVENT, M_PREVIOUS_EVENT, M_FILE_EXIT};

  for (unsigned int iButton = 0; iButton < button_names.size(); ++iButton) {
    TGTextButton* button = new TGTextButton(fFrame,
					    button_names[iButton].c_str(),
					    button_id[iButton]);

    iRow = iButton;
    TGTableLayoutHints *tloh = new TGTableLayoutHints(iColumn, iColumn+1, iRow, iRow+1,
						     kLHintsFillX | kLHintsFillY);

    fFrame->AddFrame(button,tloh);
    button->Connect("Clicked()","MainFrame",this,"DoButton()");
    button->SetToolTipText(tooltips[iButton].c_str());
   }
 }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::AddGoToEventDialog(int iRow){

  fGframe = new TGGroupFrame(this, "Go to event");
  fEventIdEntry = new TGNumberEntryField(fGframe, M_GOTO_EVENT, 0,
					 TGNumberFormat::kNESInteger,
					 TGNumberFormat::kNEANonNegative,
					 TGNumberFormat::kNELLimitMinMax,
					 0, myDataManager.numberOfEvents());
  fEventIdEntry->Connect("ReturnPressed()", "MainFrame", this, "DoButton()");
  fEventIdEntry->SetToolTipText("Jump to given event id.");  

  int iColumn = 8;
  TGTableLayoutHints *tloh = new TGTableLayoutHints(iColumn, iColumn+1, iRow, iRow+1,
						    kLHintsFillX | kLHintsFillY,
						    0, 0, 5, 2);  
  fFrame->AddFrame(fGframe, tloh);
  fGframe->AddFrame(fEventIdEntry, new TGLayoutHints(kLHintsExpandX, 0, 0, 0, 0));
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::AddNumbersDialog(){

  fEntryDialog = new EntryDialog(fFrame, this);
  TGTableLayoutHints *tloh = new TGTableLayoutHints(9,12,0,4,
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
  fCanvas->cd();
  fCanvas->Divide(3,3);
  fCanvas->cd(1);

  for(int strip_dir=0;strip_dir<3;++strip_dir){
    myHistoManager.getHoughAccumulator(strip_dir);
  }

  //myHistoManager.getRawStripVsTime(DIR_U)->SaveAs("histo.root");
  //myHistoManager.getCartesianProjection(DIR_U)->SaveAs("histo.root");
  
  for(int strip_dir=0;strip_dir<3;++strip_dir){
    ///First row
    TVirtualPad *aPad = fCanvas->cd(strip_dir+1);
    myHistoManager.getCartesianProjection(strip_dir)->DrawClone("colz");
    ///Second row
    aPad = fCanvas->cd(strip_dir+1+3);
    myHistoManager.getRecHitStripVsTime(strip_dir)->DrawClone("colz");
    myHistoManager.drawTrack3DProjectionTimeStrip(strip_dir, aPad);
    ///Third row.
    aPad = fCanvas->cd(strip_dir+1+3+3);
    myHistoManager.getHoughAccumulator(strip_dir).DrawClone("colz");
  }
  fCanvas->Update();    
  return;
  
  //Third row again.
  TVirtualPad *aPad = fCanvas->cd(7);

  TH3D *h3DReco =  myHistoManager.get3DReconstruction();
  if(h3DReco){
    aPad->Clear();
    h3DReco->DrawClone("box2z");
    myHistoManager.drawTrack3D(aPad);
  }
  else {
    aPad->Clear();
    TText aMessage(0.2, 0.5,"Calculating 3D scence");
    aMessage.DrawText(0.2, 0.5, "3D scene not available.");
  }

  aPad = fCanvas->cd(8);
  TH2D *h2D =   myHistoManager.get2DReconstruction(DIR_XY);
  if(h2D) h2D->Draw("colz");
  else myHistoManager.getDetectorLayout()->Draw("colz 0"); 
  myHistoManager.drawTrack3DProjectionXY(aPad);

  aPad = fCanvas->cd(9);
  myHistoManager.drawChargeAlongTrack3D(aPad);
  
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
  case M_GOTO_EVENT:
    {
      int eventId = fEventIdEntry->GetIntNumber();
      std::cout<<"M_GOTO_EVENT eventId: "<<eventId<<std::endl;
      myDataManager.loadEventId(eventId);
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

