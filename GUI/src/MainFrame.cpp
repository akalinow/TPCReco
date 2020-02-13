#include <cstdlib>
#include <iostream>
#include <fstream>

#include <TApplication.h>
#include <MainFrame.h>
#include <SelectionBox.h>

#include <TStyle.h>
#include <TFrame.h>
#include <TVirtualX.h>
#include <TImage.h>

#include <TH2D.h>
#include <TH3D.h>
#include <TLatex.h>
#include <TProfile.h>

#ifdef WITH_GET
#include "EventSourceGRAW.h"
#endif
#include "EventSourceROOT.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
MainFrame::MainFrame(const TGWindow *p, UInt_t w, UInt_t h,  const boost::property_tree::ptree &aConfig)
      : TGMainFrame(p, w, h){

  myConfig = aConfig;
  std::string dataFileName = myConfig.get<std::string>("dataFile");
  std::string geometryFileName = myConfig.get<std::string>("geometryFile");

  if(dataFileName.find(".root")!=std::string::npos) myEventSource = std::make_shared<EventSourceROOT>();
  #ifdef WITH_GET
  else if(dataFileName.find(".graw")!=std::string::npos) myEventSource = std::make_shared<EventSourceGRAW>(geometryFileName);
  #endif
  else{
    std::cerr<<"Input source not know. Exiting."<<std::endl;
    exit(0);
  }
  
  myEventSource->loadGeometry(geometryFileName); 
  myEventSource->loadDataFile(dataFileName);
  myEventSource->loadFileEntry(0);
  myHistoManager.setGeometry(myEventSource->getGeometry());

  fSelectionBox = 0;
  fArrow = 0;
  fLine = 0;

  SetCleanup(kDeepCleanup);
  SetWMPosition(500,0);
  SetWMSize(1200,800);
  
  AddTopMenu();
  SetTheFrame();
  AddHistoCanvas();
  AddButtons();
  AddGoToEventDialog(4);
  AddNumbersDialog();
  AddLogos();

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

  int nRows = 12, nColumns = 12;
  fFrame = new TGCompositeFrame(this,400,400,kSunkenFrame);
  TGTableLayout* tlo = new TGTableLayout(fFrame, nRows, nColumns, 1);
  fFrame->SetLayoutManager(tlo);
  fFrameLayout = new TGLayoutHints(kLHintsTop|kLHintsLeft|
				   kLHintsExpandX|kLHintsExpandY);
  AddFrame(fFrame,fFrameLayout);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::AddHistoCanvas(){

    // The Canvas
   TRootEmbeddedCanvas* embeddedCanvas = new TRootEmbeddedCanvas("Histograms",fFrame,1000,1000);
   UInt_t attach_left=0, attach_right=8;
   UInt_t attach_top=0,  attach_bottom=12;
   fTCanvasLayout = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
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

  std::vector<std::string> button_names = {"Next event", "Previous event",  "Exit"};
  std::vector<std::string> tooltips =     {"Load the next event.",
					   "Load the previous event.",
					   "Close the application"};
  std::vector<unsigned int> button_id = {M_NEXT_EVENT, M_PREVIOUS_EVENT, M_FILE_EXIT};

  UInt_t attach_left=8, attach_right=9;
  for (unsigned int iButton = 0; iButton < button_names.size(); ++iButton) {
    TGTextButton* button = new TGTextButton(fFrame,
					    button_names[iButton].c_str(),
					    button_id[iButton]);

    UInt_t attach_top=iButton,  attach_bottom=iButton+1;
    TGTableLayoutHints *tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
						      kLHintsFillX | kLHintsFillY);

    fFrame->AddFrame(button,tloh);
    button->Connect("Clicked()","MainFrame",this,"DoButton()");
    button->SetToolTipText(tooltips[iButton].c_str());
   }
 }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::AddGoToEventDialog(int attach_top){

  fGframe = new TGGroupFrame(this, "Go to event");
  fEventIdEntry = new TGNumberEntryField(fGframe, M_GOTO_EVENT, 0,
					 TGNumberFormat::kNESInteger,
					 TGNumberFormat::kNEANonNegative,
					 TGNumberFormat::kNELLimitMinMax,
					 0, myEventSource->numberOfEvents());
  fEventIdEntry->Connect("ReturnPressed()", "MainFrame", this, "DoButton()");
  fEventIdEntry->SetToolTipText("Jump to given event id.");  

  UInt_t attach_left=8, attach_right=9;
  UInt_t attach_bottom=attach_top+1;
  TGTableLayoutHints *tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
						    kLHintsFillX | kLHintsFillY,
						    0, 0, 5, 2);  
  fFrame->AddFrame(fGframe, tloh);
  fGframe->AddFrame(fEventIdEntry, new TGLayoutHints(kLHintsExpandX, 0, 0, 0, 0));
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::AddNumbersDialog(){

  fEntryDialog = new EntryDialog(fFrame, this);

  UInt_t attach_left=9, attach_right=12;
  UInt_t attach_top=0,  attach_bottom=3;
  TGTableLayoutHints *tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
						    kLHintsShrinkX|kLHintsShrinkY|
						    kLHintsFillX|kLHintsFillY);
  fEntryDialog->initialize();  
  fFrame->AddFrame(fEntryDialog, tloh);

 }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::AddLogos(){

  std::string filePath = myConfig.get<std::string>("resourcesPath")+"/FUW_znak.png";
  TImage *img = TImage::Open(filePath.c_str());
  if(!img) return;
  double ratio = img->GetWidth()/img->GetHeight();
  double height = 80;
  double width = ratio*height;
  img->Scale(width, height);
  ///FIXME clean up the ipic at the application closure.
  const TGPicture *ipic=(TGPicture *)gClient->GetPicturePool()->GetPicture("FUW_znak", img->GetPixmap(), img->GetMask());
  delete img;
  TGIcon *icon = new TGIcon(fFrame, ipic, width, height);

  UInt_t attach_left=9, attach_right=10;
  UInt_t attach_top=10,  attach_bottom=12;
  TGTableLayoutHints *tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom);
  fFrame->AddFrame(icon, tloh);

  filePath = myConfig.get<std::string>("resourcesPath")+"/ELITEPC_znak.png";
  img = TImage::Open(filePath.c_str());
  if(!img) return;
  ratio = img->GetWidth()/img->GetHeight();
  height = 100;
  width = ratio*height;
  img->Scale(width, height);
  ///FIXME clean up the ipic at the application closure.
  ipic=(TGPicture *)gClient->GetPicturePool()->GetPicture("FUW_znak", img->GetPixmap(), img->GetMask());
  delete img;
  icon = new TGIcon(fFrame, ipic, width, height);

  attach_left=11, attach_right=12;
  tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom, 0, 0, 0, -20);
  fFrame->AddFrame(icon, tloh);
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

  myHistoManager.setEvent(myEventSource->getCurrentEvent());
  fEntryDialog->updateEventNumbers(myEventSource->numberOfEvents(),
				   myEventSource->currentEventNumber());
  
  fCanvas->Clear();
  fCanvas->cd();
  fCanvas->Divide(3,3);
  fCanvas->cd(1);

  for(int strip_dir=0;strip_dir<3;++strip_dir){
    myHistoManager.getHoughAccumulator(strip_dir);
  }

  for(int strip_dir=0;strip_dir<3;++strip_dir){
    ///First row
    TVirtualPad *aPad = fCanvas->cd(strip_dir+1);
    myHistoManager.getCartesianProjection(strip_dir)->DrawClone("colz");
    ///Second row
    aPad = fCanvas->cd(strip_dir+1+3);
    myHistoManager.getRecHitStripVsTime(strip_dir)->DrawClone("colz");
    myHistoManager.getRecHitStripVsTime(strip_dir)->SaveAs(TString::Format("RecHits_%d.root", strip_dir));
    myHistoManager.getCartesianProjection(strip_dir)->SaveAs(TString::Format("RawHits_%d.root", strip_dir));
    myHistoManager.drawTrack3DProjectionTimeStrip(strip_dir, aPad);
    //myHistoManager.drawTrack2DSeed(strip_dir, aPad);
    
    ///Third row.
    aPad = fCanvas->cd(strip_dir+1+3+3);
    myHistoManager.getHoughAccumulator(strip_dir).DrawClone("colz");
    myHistoManager.getHoughAccumulator(strip_dir).SaveAs(TString::Format("HoughAccumulator_%d.root", strip_dir));
    //myHistoManager.drawChargeAlongTrack3D(aPad);
  }  
  fCanvas->Update();    //TEST
  return;//TEST
  
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
      myEventSource->loadDataFile(fileName);
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
     myEventSource->getNextEvent();      
      Update();
    }
    break;
  case M_PREVIOUS_EVENT:
    {
      myEventSource->getPreviousEvent();
      Update();
    }
    break;
  case M_GOTO_EVENT:
    {
      int eventId = fEventIdEntry->GetIntNumber();
      std::cout<<"M_GOTO_EVENT eventId: "<<eventId<<std::endl;
      myEventSource->loadEventId(eventId);
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

