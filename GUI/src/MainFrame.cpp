#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iomanip>

#include <TApplication.h>
#include "MainFrame.h"
#include "SelectionBox.h"
#include "MarkersManager.h"
#include "colorText.h"

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
  delete fMarkersManager;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::InitializeWindows(){

  SetCleanup(kDeepCleanup);
  SetWMPosition(500,0);
  SetWMSize(1300,800);
  
  AddTopMenu();
  SetTheFrame();
  
  //Left column
  AddHistoCanvas();
  ///Middle column
  int attach  = 0;
  attach = AddButtons(attach);
  attach = AddGoToEventDialog(attach);
  attach = AddGoToFileEntryDialog(attach);
  attach = AddEventTypeDialog(attach);  
  //Right column
  attach = 0;
  attach = AddNumbersDialog(attach);  
  attach = AddMarkersDialog(attach);
  AddLogos();
  /////////////
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
    if(myConfig.find("updateInterval")!=myConfig.not_found()){
      int updateInterval = myConfig.get<int>("updateInterval");
      myDirWatch.setUpdateInterval(updateInterval);
    }
    myDirWatch.Connect("Message(const char *)", "MainFrame", this, "ProcessMessage(const char *)");
  }
#endif
  else if(!myEventSource){
    std::cerr<<KRED<<"Input source not known. dataFile: "<<RST
	     <<dataFileName<<" Exiting."<<std::endl;
#ifndef WITH_GET
    std::cerr<<KRED<<" or GRAW libriaries not set."<<RST<<std::endl;
    exit(0);
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

  int nRows = 18, nColumns = 12;
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
  gStyle->SetPadLeftMargin(0.15);
  gStyle->SetPadRightMargin(0.15);

  embeddedCanvas = new TRootEmbeddedCanvas("Histograms",fFrame,1000,1000);
  TGTableLayout* aLayout = (TGTableLayout*)fFrame->GetLayoutManager();
  int nRows = aLayout->fNrows;
  int nColumns = aLayout->fNcols;
  
  UInt_t attach_left=0, attach_right=0.7*nColumns;
  UInt_t attach_top=0,  attach_bottom=nRows;
  fTCanvasLayout = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
					  kLHintsFillX|kLHintsFillY);
  fFrame->AddFrame(embeddedCanvas, fTCanvasLayout);

  fCanvas = embeddedCanvas->GetCanvas();
  fCanvas->MoveOpaque(kFALSE);
  fCanvas->Divide(2,2, 0.02, 0.02);
  
  TText aMessage(0.2, 0.5,"Waiting for data.");
  for(int iPad=1;iPad<=4;++iPad){
    fCanvas->cd(iPad);
    aMessage.DrawText(0.2, 0.5,"Waiting for data.");
  }
  fCanvas->Update();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int MainFrame::AddButtons(int attach){

  std::vector<std::string> button_names = {"Next event", "Previous event", "Exit"};
  std::vector<std::string> tooltips =     {"Load the next event.",
					   "Load the previous event.",
					   "Close the application"};
  std::vector<unsigned int> button_id = {M_NEXT_EVENT, M_PREVIOUS_EVENT,  M_FILE_EXIT};

  ULong_t aColor = TColor::RGB2Pixel(195,195,250);

  TGTableLayout* aLayout = (TGTableLayout*)fFrame->GetLayoutManager();
  int nColumns = aLayout->fNcols;
  UInt_t attach_left=0.7*nColumns;
  UInt_t attach_right=attach_left+1;
  
  for (unsigned int iButton = 0; iButton < button_names.size(); ++iButton) {
    TGTextButton* aButton = new TGTextButton(fFrame,
					    button_names[iButton].c_str(),
					    button_id[iButton]);

    UInt_t attach_top=iButton+attach,  attach_bottom=iButton+1;
    TGTableLayoutHints *tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
						      kLHintsFillX | kLHintsFillY);

    fFrame->AddFrame(aButton,tloh);
    aButton->Connect("Clicked()","MainFrame",this,"DoButton()");
    aButton->SetToolTipText(tooltips[iButton].c_str());
    aButton->ChangeBackground(aColor);
   }
  std::string button_name="Set logscale";
  std::string button_tooltip="Sets logscale Z";
  TGCheckButton* button = new TGCheckButton(fFrame,
					    button_name.c_str(),
					   M_TOGGLE_LOGSCALE);
  UInt_t attach_top=attach+button_names.size(),  attach_bottom=button_names.size()+1;
  TGTableLayoutHints *tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
						      kLHintsFillX | kLHintsFillY);
  fFrame->AddFrame(button,tloh);
  button->Connect("Clicked()","MainFrame",this,"DoButton()");
  button->SetToolTipText(button_tooltip.c_str());

  return attach_bottom;
 }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int MainFrame::AddGoToEventDialog(int attach){

  fGframe = new TGGroupFrame(this, "Go to event id.");
  fEventIdEntry = new TGNumberEntryField(fGframe, M_GOTO_EVENT, 0,
					 TGNumberFormat::kNESInteger,
					 TGNumberFormat::kNEANonNegative,
					 TGNumberFormat::kNELNoLimits);
  fEventIdEntry->Connect("ReturnPressed()", "MainFrame", this, "DoButton()");
  fEventIdEntry->SetToolTipText("Jump to given event id.");

  TGTableLayout* aLayout = (TGTableLayout*)fFrame->GetLayoutManager();
  int nColumns = aLayout->fNcols;
  //int nRows = aLayout->fNrows;

  UInt_t attach_left=0.7*nColumns;
  UInt_t attach_right=attach_left+1;
  UInt_t attach_top=attach+1;
  UInt_t attach_bottom=attach_top+1;
  TGTableLayoutHints *tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
						    kLHintsFillX | kLHintsFillY,
						    0, 0, 5, 2);  
  fFrame->AddFrame(fGframe, tloh);
  fGframe->AddFrame(fEventIdEntry, new TGLayoutHints(kLHintsExpandX, 0, 0, 0, 0));

  return attach_bottom;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int MainFrame::AddGoToFileEntryDialog(int attach){

  fGframe = new TGGroupFrame(this, "Go to file entry.");
  fFileEntryEntry = new TGNumberEntryField(fGframe, M_GOTO_ENTRY, 0,
					 TGNumberFormat::kNESInteger,
					 TGNumberFormat::kNEANonNegative,
					 TGNumberFormat::kNELNoLimits);
  fFileEntryEntry->Connect("ReturnPressed()", "MainFrame", this, "DoButton()");
  fFileEntryEntry->SetToolTipText("Jump to given event id.");  

  TGTableLayout* aLayout = (TGTableLayout*)fFrame->GetLayoutManager();
  int nColumns = aLayout->fNcols;
  int nRows = aLayout->fNrows;

  UInt_t attach_left=0.7*nColumns;
  UInt_t attach_right=attach_left+1;
  UInt_t attach_top=attach;
  UInt_t attach_bottom=attach_top+nRows*0.1;
  TGTableLayoutHints *tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
						    kLHintsFillX | kLHintsFillY,
						    0, 0, 5, 2);  
  fFrame->AddFrame(fGframe, tloh);
  fGframe->AddFrame(fFileEntryEntry, new TGLayoutHints(kLHintsExpandX, 0, 0, 0, 0));

  return attach_bottom;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int MainFrame::AddNumbersDialog(int attach){

  fEntryDialog = new EntryDialog(fFrame, this);

  TGTableLayout* aLayout = (TGTableLayout*)fFrame->GetLayoutManager();
  //int nRows = aLayout->fNrows;
  int nColumns = aLayout->fNcols;
  UInt_t attach_left=nColumns*0.7+1;
  UInt_t attach_right=nColumns;
  UInt_t attach_top=attach;
  UInt_t attach_bottom=attach_top+6;
  TGTableLayoutHints *tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
						    kLHintsShrinkX|kLHintsShrinkY|
						    kLHintsFillX|kLHintsFillY);
  fEntryDialog->initialize();
  fFrame->AddFrame(fEntryDialog, tloh);
  return attach_bottom;
 }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int MainFrame::AddEventTypeDialog(int attach){

  eventTypeButtonGroup = new TGButtonGroup(fFrame,
					   7, 1, 1.0, 1.0,
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

  TGTableLayout* aLayout = (TGTableLayout*)fFrame->GetLayoutManager();
  int nColumns = aLayout->fNcols;
  int nRows = aLayout->fNrows;
  
  UInt_t attach_left=nColumns*0.7;
  UInt_t attach_right=attach_left+1;
  UInt_t attach_top=attach+1;
  UInt_t attach_bottom=attach_top + nRows*0.25;
  TGTableLayoutHints *tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
						    kLHintsShrinkX|kLHintsShrinkY|
						    kLHintsFillX|kLHintsFillY);
  fFrame->AddFrame(eventTypeButtonGroup, tloh);
  return attach_bottom;
 }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int MainFrame::AddMarkersDialog(int attach){

  TGTableLayout* aLayout = (TGTableLayout*)fFrame->GetLayoutManager();
  int nColumns = aLayout->fNcols;
  int nRows = aLayout->fNcols;
  UInt_t attach_left=nColumns*0.7+1;
  UInt_t attach_right=nColumns;
  UInt_t attach_top=attach+1;
  UInt_t attach_bottom=attach_top+nRows*0.3;

  fMarkersManager = new MarkersManager(fFrame, this);
  fMarkersManager->Connect("sendSegmentsData(std::vector<double> *)","MainFrame",
			   this,"drawRecoFromMarkers(std::vector<double> *)");
  TGTableLayoutHints *tloh = new TGTableLayoutHints(attach_left, attach_right,
						    attach_top, attach_bottom,
						    kLHintsExpandX|kLHintsExpandY |
						    kLHintsShrinkX|kLHintsShrinkY|
						    kLHintsFillX|kLHintsFillY);
  fFrame->AddFrame(fMarkersManager, tloh);  
  fCanvas->Connect("ProcessedEvent(Int_t,Int_t,Int_t,TObject*)",
		   "MarkersManager", fMarkersManager,
		   "HandleMarkerPosition(Int_t,Int_t,Int_t,TObject*)");
  return attach_bottom; 
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::AddLogos(){

  std::string filePath = myConfig.get<std::string>("resourcesPath")+"/FUW_znak.png";
  TImage *img = TImage::Open(filePath.c_str());
  if(!img->IsValid()) return;
  double ratio = img->GetWidth()/img->GetHeight();
  double height = 80;
  double width = ratio*height;
  img->Scale(width, height);
  ///FIXME clean up the ipic at the application closure.
  const TGPicture *ipic=(TGPicture *)gClient->GetPicturePool()->GetPicture("FUW_znak", img->GetPixmap(), img->GetMask());
  delete img;
  TGIcon *icon = new TGIcon(fFrame, ipic, width, height);

  TGTableLayout* aLayout = (TGTableLayout*)fFrame->GetLayoutManager();
  int nColumns = aLayout->fNcols;
  int nRows = aLayout->fNrows;
  UInt_t attach_left=nColumns*0.7+1;
  UInt_t attach_right=attach_left+1;
  UInt_t attach_top=nRows*0.9;
  UInt_t attach_bottom=nRows;
  TGTableLayoutHints *tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom);
  fFrame->AddFrame(icon, tloh);

  filePath = myConfig.get<std::string>("resourcesPath")+"/ELITEPC_znak.png";
  img = TImage::Open(filePath.c_str());
  if(!img->IsValid()) return;
  ratio = img->GetWidth()/img->GetHeight();
  height = 100;
  width = ratio*height;
  img->Scale(width, height);
  ///FIXME clean up the ipic at the application closure.
  ipic=(TGPicture *)gClient->GetPicturePool()->GetPicture("FUW_znak", img->GetPixmap(), img->GetMask());
  delete img;
  icon = new TGIcon(fFrame, ipic, width, height);

  attach_left=attach_right+1;
  attach_right=nColumns;
  tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom, 0, 0, 0, -20);
  fFrame->AddFrame(icon, tloh);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::HandleEmbeddedCanvas(Int_t event, Int_t x, Int_t y, TObject *sel){

  std::cout<<__FUNCTION__<<std::endl;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::CloseWindow(){ gApplication->Terminate(0); }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::Update(){

  if(!myEventSource->numberOfEvents()) return;
  fEntryDialog->updateFileName(myEventSource->getCurrentPath());
  fEntryDialog->updateEventNumbers(myEventSource->numberOfEvents(),
				   myEventSource->currentEventNumber(),
				   myEventSource->currentEntryNumber());
  myHistoManager.setEvent(myEventSource->getCurrentEvent());
  fMarkersManager->reset();

  //drawRawHistos();
  drawRecoHistos();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::drawRawHistos(){

  for(int strip_dir=DIR_U;strip_dir<=DIR_W;++strip_dir){
    fCanvas->cd(strip_dir+1);
    myHistoManager.getRawStripVsTime(strip_dir)->DrawClone("colz");
    fCanvas->Update();
  }  
  fCanvas->cd(4);
  myHistoManager.getRawTimeProjection()->DrawClone("hist");
  fCanvas->Update();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::drawRecoHistos(){

  myHistoManager.reconstruct();
  
   for(int strip_dir=DIR_U;strip_dir<=DIR_W;++strip_dir){
    TVirtualPad *aPad = fCanvas->cd(strip_dir+1);
    //myHistoManager.getRecHitStripVsTime(strip_dir)->DrawClone("colz");
    myHistoManager.getCartesianProjection(strip_dir)->DrawClone("colz");
    //myHistoManager.getHoughAccumulator(strip_dir).DrawClone("colz");
    //myHistoManager.drawTrack2DSeed(strip_dir, aPad);
    //myHistoManager.drawTrack3DProjectionTimeStrip(strip_dir, aPad, false);
    fCanvas->Update();
    std::cout<<aPad<<std::endl;
  }
  TVirtualPad *aPad = fCanvas->cd(4);
  myHistoManager.drawChargeAlongTrack3D(aPad);
  fCanvas->Update();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::drawRecoFromMarkers(std::vector<double> * segmentsXY){

  myHistoManager.reconstructSegmentsFromMarkers(segmentsXY);
  
  for(int strip_dir=0;strip_dir<3;++strip_dir){
    TVirtualPad *aPad = fCanvas->cd(strip_dir+1);
    myHistoManager.drawTrack3DProjectionTimeStrip(strip_dir, aPad, false);
    fCanvas->Update();
  }
  TCanvas *a3dCanvas = new TCanvas("a3dCanvas","3D Canvas", 500, 500);
  TVirtualPad *aPad = a3dCanvas->cd();
  myHistoManager.drawTrack3D(aPad);
  a3dCanvas->Update();
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

  std::cout<<__FUNCTION__<<" msg: "<<msg<<std::endl;

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

  std::cout<<__FUNCTION__<<" msg: "<<msg<<std::endl;

  myMutex.lock();
  myEventSource->loadDataFile(std::string(msg));
  myEventSource->getLastEvent();
  Update();
  myMutex.unlock();
  return kTRUE;
}
/////////////////////////////////////////////////////////
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
