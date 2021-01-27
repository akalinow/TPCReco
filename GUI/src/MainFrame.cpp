#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iomanip>

#include <TApplication.h>
#include <MainFrame.h>
#include <SelectionBox.h>

#include <TSystem.h>
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

#include "TGButtonGroup.h"
#include "TGButton.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
MainFrame::MainFrame(const TGWindow *p, UInt_t w, UInt_t h,  const boost::property_tree::ptree &aConfig)
      : TGMainFrame(p, w, h){

  myConfig = aConfig;
 
  fSelectionBox = 0;
  fArrow = 0;
  fLine = 0;

  InitializeWindows();
  InitializeEventSource();

  std::string modeLabel = "NONE";
  if(myWorkMode==M_ONLINE_MODE){
    modeLabel = "ONLINE";
  }
  else if(myWorkMode==M_OFFLINE_ROOT_MODE){
    modeLabel = "OFFLINE from ROOT";
  }
  else if(myWorkMode==M_OFFLINE_GRAW_MODE){
    modeLabel = "OFFLINE from GRAW";
  }
  fEntryDialog->updateModeLabel(modeLabel);
  Update();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
MainFrame::~MainFrame(){

  fileWatchThread.join();
  // Delete all created widgets.
  delete fMenuFile;
  delete fMenuHelp;
  delete fMenuBar;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::InitializeWindows(){

  SetCleanup(kDeepCleanup);
  SetWMPosition(500,0);
  SetWMSize(1200,800);
  
  AddTopMenu();
  SetTheFrame();
  AddHistoCanvas();
  AddButtons();
  AddGoToEventDialog(5);
  AddGoToFileEntryDialog(6);
  AddNumbersDialog();
  AddEventTypeDialog();
  AddLogos();

  MapSubwindows();
  Resize();
  MapWindow();
  SetWindowName("TPC GUI");
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::InitializeEventSource(){

  std::string dataFileName = myConfig.get<std::string>("dataFile");
  std::string geometryFileName = myConfig.get<std::string>("geometryFile");

  if(dataFileName.empty() || geometryFileName.empty()){
    std::cerr<<"No data or geometry file path provided."<<std::endl;
    return;
  }

  if(dataFileName.find(".root")!=std::string::npos){
    myWorkMode = M_OFFLINE_ROOT_MODE;
    myEventSource = std::make_shared<EventSourceROOT>();
    myEventSource->loadGeometry(geometryFileName); 
  }
#ifdef WITH_GET
  else if(dataFileName.find(".graw")!=std::string::npos){
    myWorkMode = M_OFFLINE_GRAW_MODE;
    myEventSource = std::make_shared<EventSourceGRAW>(geometryFileName);
  }
  else if(dataFileName.back()=='/'){
    myWorkMode = M_ONLINE_MODE;
    myEventSource = std::make_shared<EventSourceGRAW>(geometryFileName);
    fileWatchThread = std::thread(&DirectoryWatch::watch, &myDirWatch, dataFileName);
    myDirWatch.Connect("Message(const char *)", "MainFrame", this, "ProcessMessage(const char *)");
  }
#endif
  else if(!myEventSource){
    std::cerr<<"Input source not known. dataFile: "
	     <<dataFileName<<" Exiting."<<std::endl;
#ifndef WITH_GET
    std::cerr<<"GRAW libriaries not set."<<std::endl;
#endif    
    return;
  }

  if(myWorkMode!=M_ONLINE_MODE){
    myEventSource->loadDataFile(dataFileName);
    myEventSource->loadFileEntry(0);
  }
  myHistoManager.setGeometry(myEventSource->getGeometry());
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

  gStyle->SetOptStat(0);
  gStyle->SetPalette(57);
 // gStyle->SetOptLogz();

  embeddedCanvas = new TRootEmbeddedCanvas("Histograms",fFrame,1000,1000);
  UInt_t attach_left=0, attach_right=8;
  UInt_t attach_top=0,  attach_bottom=12;
  fTCanvasLayout = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
					  kLHintsFillX|kLHintsFillY);
  fFrame->AddFrame(embeddedCanvas, fTCanvasLayout);

  fCanvas = embeddedCanvas->GetCanvas();
  fCanvas->MoveOpaque(kFALSE);
  fCanvas->Divide(2,2);
  TText aMessage(0.2, 0.5,"Waiting for data.");
  for(int iPad=1;iPad<=4;++iPad){
    fCanvas->cd(iPad);
    aMessage.DrawText(0.2, 0.5,"Waiting for data.");
  }
  fCanvas->Update();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::AddButtons(){

  std::vector<std::string> button_names = {"Next event", "Previous event", "Exit"};
  std::vector<std::string> tooltips =     {"Load the next event.",
					   "Load the previous event.",
					   "Close the application"};
  std::vector<unsigned int> button_id = {M_NEXT_EVENT, M_PREVIOUS_EVENT,  M_FILE_EXIT};

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
  std::string button_name="Set logscale";
  std::string button_tooltip="Sets logscale Z";
  TGCheckButton* button = new TGCheckButton(fFrame,
					    button_name.c_str(),
					   M_TOGGLE_LOGSCALE);
  UInt_t attach_top=button_names.size(),  attach_bottom=button_names.size()+1;
  TGTableLayoutHints *tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
						      kLHintsFillX | kLHintsFillY);
  fFrame->AddFrame(button,tloh);
  button->Connect("Clicked()","MainFrame",this,"DoButton()");
  button->SetToolTipText(button_tooltip.c_str());

 }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::AddGoToEventDialog(int attach_top){

  fGframe = new TGGroupFrame(this, "Go to event id.");
  fEventIdEntry = new TGNumberEntryField(fGframe, M_GOTO_EVENT, 0,
					 TGNumberFormat::kNESInteger,
					 TGNumberFormat::kNEANonNegative,
					 TGNumberFormat::kNELNoLimits);
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
void MainFrame::AddGoToFileEntryDialog(int attach_top){

  fGframe = new TGGroupFrame(this, "Go to file entry.");
  fFileEntryEntry = new TGNumberEntryField(fGframe, M_GOTO_ENTRY, 0,
					 TGNumberFormat::kNESInteger,
					 TGNumberFormat::kNEANonNegative,
					 TGNumberFormat::kNELNoLimits);
  fFileEntryEntry->Connect("ReturnPressed()", "MainFrame", this, "DoButton()");
  fFileEntryEntry->SetToolTipText("Jump to given event id.");  

  UInt_t attach_left=8, attach_right=9;
  UInt_t attach_bottom=attach_top+1;
  TGTableLayoutHints *tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
						    kLHintsFillX | kLHintsFillY,
						    0, 0, 5, 2);  
  fFrame->AddFrame(fGframe, tloh);
  fGframe->AddFrame(fFileEntryEntry, new TGLayoutHints(kLHintsExpandX, 0, 0, 0, 0));
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
void MainFrame::AddEventTypeDialog(){

  eventTypeButtonGroup = new TGButtonGroup(fFrame,
					   2, 4, 0.05, 0.05,
					   "Event type");
  eventTypeButtonGroup->SetExclusive(kTRUE);
  std::vector<TGCheckButton*> buttonsContainer;
  buttonsContainer.push_back(new TGCheckButton(eventTypeButtonGroup, new TGHotString("Empty")));
  buttonsContainer.push_back(new TGCheckButton(eventTypeButtonGroup, new TGHotString("Noise")));
  buttonsContainer.push_back(new TGCheckButton(eventTypeButtonGroup, new TGHotString("Dot")));
  buttonsContainer.push_back(new TGCheckButton(eventTypeButtonGroup, new TGHotString("1 track")));
  buttonsContainer.push_back(new TGCheckButton(eventTypeButtonGroup, new TGHotString("2 tracks")));
  buttonsContainer.push_back(new TGCheckButton(eventTypeButtonGroup, new TGHotString("3 tracks")));
  buttonsContainer.push_back(new TGCheckButton(eventTypeButtonGroup, new TGHotString("Other")));
  buttonsContainer.front()->SetState(kButtonDown);
  
  UInt_t attach_left=9, attach_right=12;
  UInt_t attach_top=4,  attach_bottom=5;
  TGTableLayoutHints *tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
						    kLHintsShrinkX|kLHintsShrinkY|
						    kLHintsFillX|kLHintsFillY);
  fFrame->AddFrame(eventTypeButtonGroup, tloh);
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

  if(!myEventSource->numberOfEvents()) return;
  fEntryDialog->updateFileName(myEventSource->getCurrentPath());
  fEntryDialog->updateEventNumbers(myEventSource->numberOfEvents(),
				   myEventSource->currentEventNumber(),
				   myEventSource->currentEntryNumber());
  myHistoManager.setEvent(myEventSource->getCurrentEvent());
 // for(int strip_dir=0;strip_dir<3;++strip_dir){
 //   myHistoManager.getHoughAccumulator(strip_dir);
 // }

  for(int strip_dir=0;strip_dir<3;++strip_dir){
    ///First row
    fCanvas->cd(strip_dir+1);
      myHistoManager.getRawStripVsTime(strip_dir)->DrawClone("colz");
  //  myHistoManager.getCartesianProjection(strip_dir)->DrawClone("colz");
    ///Second row

   // myHistoManager.getRecHitStripVsTime(strip_dir)->DrawClone("colz");
   // myHistoManager.getRecHitStripVsTime(strip_dir)->SaveAs(TString::Format("RecHits_%d.root", strip_dir));
   // myHistoManager.getCartesianProjection(strip_dir)->SaveAs(TString::Format("RawHits_%d.root", strip_dir));
   // myHistoManager.drawTrack3DProjectionTimeStrip(strip_dir, aPad);
    //myHistoManager.drawTrack2DSeed(strip_dir, aPad);
    
    ///Third row.
  //  aPad = fCanvas->cd(strip_dir+1+3+3);
  //  myHistoManager.getHoughAccumulator(strip_dir).DrawClone("colz");
  //  myHistoManager.getHoughAccumulator(strip_dir).SaveAs(TString::Format("HoughAccumulator_%d.root", strip_dir));
    //myHistoManager.drawChargeAlongTrack3D(aPad);
   // aPad->GetName();
  }  
      fCanvas->cd(4);
      myHistoManager.getRawTimeProjection()->DrawClone("hist");
  fCanvas->Update();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::UpdateEventLog(){

  int index =  myEventSource->getCurrentPath().find_last_of("/");
  int pathLength =  myEventSource->getCurrentPath().size();
  std::string logFileName = myEventSource->getCurrentPath().substr(index+1, pathLength);
  logFileName += ".log";
  std::fstream out(logFileName);

  ///Log header
  if(!out.is_open()){
    out.open(logFileName, std::ofstream::app);    
    for(int iButton=1;iButton<=eventTypeButtonGroup->GetCount();++iButton){
      TGTextButton *aButton = (TGTextButton*)eventTypeButtonGroup->GetButton(iButton);
      if(!aButton){
	std::cerr<<__FUNCTION__<<" Coversion to TGTextButton failed!"<<std::endl;
	continue;
      }
      out<<iButton<<" - "<<aButton->GetString()<<std::endl;
    }
    out<<"Event Id \t entry number \t Event type"<<std::endl;
  }
  /////
  out.close();
  out.open(logFileName, std::ofstream::app);
  
  if(!eventTypeButtonGroup){
    std::cerr<<"eventTypeButtonGroup not initialised!";
    return;
  }
  for(int iButton=1;iButton<=eventTypeButtonGroup->GetCount();++iButton){
    if(eventTypeButtonGroup->GetButton(iButton)->IsOn()){
      out<<myEventSource->currentEventNumber()<<" \t\t "
	 <<myEventSource->currentEntryNumber()<<" \t\t "
	 <<iButton<<std::endl;
      break;
    }
  }
  out.close();
  eventTypeButtonGroup->SetButton(1, kTRUE);  
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

  std::cout<<__FUNCTION__<<std::endl;

  switch (msg) {
  case M_DATA_FILE_UPDATED:
    {
    }
    break;
  }
    return kTRUE;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Bool_t MainFrame::ProcessMessage(const char * msg){

  myMutex.lock();
  myEventSource->loadDataFile(std::string(msg));
  myEventSource->getLastEvent();
  Update();
  myMutex.unlock();
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
			     //"GRAW files",    "*.graw",
			     //"All files",     "*",
			     0,               0};
  
  switch (id) {
  case M_FILE_OPEN:
    {
      TGFileInfo fi;
      fi.fFileTypes = filetypes;
      fi.fIniDir    = StrDup(".");
      new TGFileDialog(gClient->GetRoot(), this, kFDOpen, &fi);
      std::string fileName;
      if(fi.fFilename) fileName.append(fi.fFilename);
      else return;
      myEventSource->loadDataFile(fileName);
      myEventSource->loadFileEntry(0);
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
      UpdateEventLog();
      myEventSource->getNextEvent();
      Update();
    }
    break;
  case M_PREVIOUS_EVENT:
    {
      UpdateEventLog();
      myEventSource->getPreviousEvent();
      Update();
    }
    break;
  case M_TOGGLE_LOGSCALE:
    {
      isLogScaleOn=!isLogScaleOn;
      for(int iPad=1;iPad<=3;++iPad){
        fCanvas->cd(iPad)->SetLogz(isLogScaleOn);
      }
      fCanvas->Update();
    }
    break;
  case M_GOTO_EVENT:
    {
      int eventId = fEventIdEntry->GetIntNumber();
      myEventSource->loadEventId(eventId);
      Update();
    }
    break;
  case M_GOTO_ENTRY:
    {
      int fileEntry = fFileEntryEntry->GetIntNumber();
      myEventSource->loadFileEntry(fileEntry);
      Update();
    }
    break;

  case M_DIR_WATCH:
    {
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
