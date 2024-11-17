#include <cstdlib>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>

#include <TApplication.h>
#include "TPCReco/MainFrame.h"
#include "TPCReco/SelectionBox.h"
#include "TPCReco/MarkersManager.h"
#include "TPCReco/colorText.h"

#include <TSystem.h>
#include <TObjArray.h> 
#include <TObjString.h>
#include <TStyle.h>
#include <TString.h>
#include <TFrame.h>
#include <TVirtualX.h>
#include <TImage.h>

#include <TH2D.h>
#include <TH3D.h>
#include <TLatex.h>
#include <TProfile.h>

#include "TPCReco/EventSourceFactory.h"

#include <TGButtonGroup.h>
#include <TGButton.h>
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
MainFrame::MainFrame(const TGWindow* p, UInt_t w, UInt_t h, const boost::property_tree::ptree& aConfig)
	: TGMainFrame(p, w, h) {

	myConfig = aConfig;

	fSelectionBox = 0;
	InitializeEventSource();
	InitializeWindows();

	std::string modeLabel = "NONE";
	if (myWorkMode == M_ONLINE_GRAW_MODE || myWorkMode == M_ONLINE_NGRAW_MODE) {
		modeLabel = "ONLINE";
	}
	else if (myWorkMode == M_OFFLINE_ROOT_MODE) {
		modeLabel = "OFFLINE from ROOT";
	}
	else if (myWorkMode == M_OFFLINE_MC_MODE) {
		modeLabel = "OFFLINE from Monte Carlo";
	}
	else if (myWorkMode == M_OFFLINE_GRAW_MODE || myWorkMode == M_OFFLINE_NGRAW_MODE) {
		modeLabel = "OFFLINE from GRAW";
	}
	else if (myWorkMode == M_OFFLINE_GEANT4_MODE) {
		modeLabel = "OFFLINE from Geant4";
	}
	fFileInfoFrame->updateModeLabel(modeLabel);
	Update();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
MainFrame::~MainFrame() {

	fileWatchThread.join();
	// Delete all created widgets.
	delete fMenuFile;
	delete fMenuHelp;
	delete fMenuBar;
	delete fMarkersManager;
}
/////////////////////////////////////////////////////////
///////////////////////////////////////////////////////
void MainFrame::InitializeWindows() {

	SetCleanup(kDeepCleanup);
	SetWMPosition(500, 0);
	SetWMSize(1300, 800);

	AddTopMenu();
	SetTheFrame();

	//Left column
	AddHistoCanvas();
	///Middle column
	int attach = 0;
	attach = AddButtons(attach);
	attach = AddGoToEventDialog(attach);
	attach = AddGoToFileEntryDialog(attach);
	attach = AddEventTypeDialog(attach);
	//Right column
	attach = 0;
	attach = AddFileInfoFrame(attach);
	attach = AddMarkersDialog(attach);
	attach = AddRunConditionsDialog(attach);
	AddLogos();
	/////////////
	MapSubwindows();
	Resize();
	MapWindow();
	SetWindowName("TPC GUI");
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::InitializeEventSource() {
	std::string dataFileName = myConfig.get<std::string>("input.dataFile");
	myEventSource = EventSourceFactory::makeEventSourceObject(myConfig);

	event_type eventSourceType = myConfig.get<event_type>("transient.eventType");
	bool onlineFlag = myConfig.get<bool>("transient.onlineFlag");

	if (onlineFlag) {
		fileWatchThread = std::thread(&DirectoryWatch::watch, &myDirWatch, dataFileName);
		int updateInterval = myConfig.get<int>("online.updateInterval");
		myDirWatch.setUpdateInterval(updateInterval);
		myDirWatch.Connect("Message(const char *)", "MainFrame", this, "ProcessMessage(const char *)");
	}

	if (eventSourceType == event_type::EventSourceROOT) {
		myWorkMode = M_OFFLINE_ROOT_MODE;
	}
	else if (eventSourceType == event_type::EventSourceMC) {
		myWorkMode = M_OFFLINE_MC_MODE;
}
	else if (eventSourceType == event_type::EventSourceGRAW) {
		myWorkMode = (onlineFlag ? M_ONLINE_GRAW_MODE : M_OFFLINE_GRAW_MODE);
	}
	else if (eventSourceType == event_type::EventSourceMultiGRAW) {
		myWorkMode = (onlineFlag ? M_ONLINE_NGRAW_MODE : M_OFFLINE_NGRAW_MODE);
	}
	else if (eventSourceType == event_type::EventSourceGeant4) {
		myWorkMode = M_OFFLINE_GEANT4_MODE;
	}
	else {
		std::cerr << "Unknown event source type: " << eventSourceType << std::endl;
		exit(1);
	}
	
	myHistoManager.setConfig(myConfig);
	myHistoManager.setGeometry(myEventSource->getGeometry());

	if (isRecoModeOn) myHistoManager.openOutputStream(dataFileName);
	myEventSource->getEventFilter().setConditions(myConfig);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::AddTopMenu() {

	TGLayoutHints* menuBarItemLayout = new TGLayoutHints(kLHintsTop | kLHintsLeft, 0, 4, 0, 0);
	TGLayoutHints* menuBarHelpLayout = new TGLayoutHints(kLHintsTop | kLHintsRight);

	fMenuFile = new TGPopupMenu(fClient->GetRoot());
	if (myWorkMode != M_ONLINE_GRAW_MODE && myWorkMode != M_ONLINE_NGRAW_MODE) fMenuFile->AddEntry("&Open...", M_FILE_OPEN);
	//fMenuFile->AddEntry("S&ave as...", M_FILE_SAVEAS);
	fMenuFile->AddSeparator();
	fMenuFile->AddEntry("E&xit", M_FILE_EXIT);
	fMenuFile->Connect("Activated(Int_t)", "MainFrame", this, "HandleMenu(Int_t)");

	fMenuHelp = new TGPopupMenu(fClient->GetRoot());
	fMenuHelp->AddEntry("&Contents", M_HELP_CONTENTS);
	fMenuHelp->AddEntry("&Search...", M_HELP_SEARCH);
	fMenuHelp->AddSeparator();
	fMenuHelp->AddEntry("&About", M_HELP_ABOUT);
	fMenuHelp->Connect("Activated(Int_t)", "MainFrame", this, "HandleMenu(Int_t)");

	fMenuBar = new TGMenuBar(this, 1, 1, kHorizontalFrame);
	fMenuBar->AddPopup("&File", fMenuFile, menuBarItemLayout);
	fMenuBar->AddPopup("&Help", fMenuHelp, menuBarHelpLayout);
	AddFrame(fMenuBar, new TGLayoutHints(kLHintsExpandX, 0, 0, 1, 0));
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::SetTheFrame() {

	int nRows = 20, nColumns = 12;
	fFrame = new TGCompositeFrame(this, 400, 400, kSunkenFrame);
	TGTableLayout* tlo = new TGTableLayout(fFrame, nRows, nColumns, 1);
	fFrame->SetLayoutManager(tlo);
	fFrameLayout = new TGLayoutHints(kLHintsTop | kLHintsLeft |
		kLHintsExpandX | kLHintsExpandY);
	AddFrame(fFrame, fFrameLayout);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::AddHistoCanvas() {

	gStyle->SetOptStat(0);
	gStyle->SetPalette(57);
	gStyle->SetPadLeftMargin(0.15);
	gStyle->SetPadRightMargin(0.15);

	/*
	fRawHistosCanvas = new TCanvas("fRawHistosCanvas","Raw Histograms",850,800);
	fRawHistosCanvas->MoveOpaque(kFALSE);
	fRawHistosCanvas->Divide(2,2, 0.02, 0.02);
	TList *aList = fRawHistosCanvas->GetListOfPrimitives();
	for(auto obj: *aList){
	  TPad *aPad = (TPad*)(obj);
	  aPad->SetNumber(100 + aPad->GetNumber());
	}

	fTechHistosCanvas = new TCanvas("fTechHistosCanvas","Diagnostic Histograms",850,800);
	fTechHistosCanvas->MoveOpaque(kFALSE);
	fTechHistosCanvas->Divide(2,2, 0.02, 0.02);
	TList *aList1 = fTechHistosCanvas->GetListOfPrimitives();
	for(auto obj: *aList1){
	  TPad *aPad = (TPad*)(obj);
	  aPad->SetNumber(200 + aPad->GetNumber());
	}
	*/

	embeddedCanvas = new TRootEmbeddedCanvas("embeddedCanvas", fFrame, 1000, 1000);
	TGTableLayout* aLayout = (TGTableLayout*)fFrame->GetLayoutManager();
	int nRows = aLayout->fNrows;
	int nColumns = aLayout->fNcols;

	UInt_t attach_left = 0, attach_right = 0.7 * nColumns;
	UInt_t attach_top = 0, attach_bottom = nRows;
	fTCanvasLayout = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
		kLHintsFillX | kLHintsFillY);
	fFrame->AddFrame(embeddedCanvas, fTCanvasLayout);
	fMainCanvas = embeddedCanvas->GetCanvas();
	fMainCanvas->SetTitle("Reco Histograms");
	fMainCanvas->SetName("Histograms");
	fMainCanvas->MoveOpaque(kFALSE);
	fMainCanvas->Divide(2, 2, 0.02, 0.02);

	ClearCanvases();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int MainFrame::AddButtons(int attach) {

	std::vector<std::string> button_names = { "Next event", "Previous event", "Reset event" , "Exit" };
	std::vector<std::string> button_tooltips = { "Load the next event.",
							"Load the previous event.",
							"Reload the current event, and reset all settings.",
							"Close the application" };
	std::vector<unsigned int> button_id = { M_NEXT_EVENT, M_PREVIOUS_EVENT,  M_RESET_EVENT, M_FILE_EXIT };

	if (myWorkMode == M_ONLINE_GRAW_MODE || myWorkMode == M_ONLINE_NGRAW_MODE) {
		button_names.insert(--button_names.end(), "Reset rate");
		button_tooltips.insert(--button_tooltips.end(), "Clear rate graph");
		button_id.insert(--button_id.end(), M_RESET_RATE);
	}

	ULong_t aColor = TColor::RGB2Pixel(195, 195, 250);

	TGTableLayout* aLayout = (TGTableLayout*)fFrame->GetLayoutManager();
	int nColumns = aLayout->fNcols;
	UInt_t attach_left = 0.7 * nColumns;
	UInt_t attach_right = attach_left + 1;
	UInt_t attach_top = attach;
	UInt_t attach_bottom = 1;

	for (unsigned int iButton = 0; iButton < button_names.size(); ++iButton) {
		TGTextButton* aButton = new TGTextButton(fFrame,
			button_names[iButton].c_str(),
			button_id[iButton]);
		TGTableLayoutHints* tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
			kLHintsFillX | kLHintsFillY);

		fFrame->AddFrame(aButton, tloh);
		aButton->Connect("Clicked()", "MainFrame", this, "DoButton()");
		aButton->SetToolTipText(button_tooltips[iButton].c_str());
		aButton->ChangeBackground(aColor);
		++attach_top;
		++attach_bottom;
	}

	std::vector<std::string> checkbox_names = { "Set Z logscale", "Set auto zoom", "Set manual reco", "Display rate" };
	std::vector<std::string> checkbox_tooltips = { "Enables the logscale on Z axis",
						  "Enables automatic zoom in on region with deposits",
						  "Enables manual track reconstruction mode", "Display event rate plot" };
	std::vector<std::string> checkbox_config = { "zLogScale", "autoZoom", "recoMode", "rate" };
	std::vector<unsigned int> checkbox_id = { M_TOGGLE_LOGSCALE, M_TOGGLE_AUTOZOOM, M_TOGGLE_RECOMODE, M_TOGGLE_RATE };

	auto displayConfig = myConfig.find("display");
	for (unsigned int iCheckbox = 0; iCheckbox < checkbox_names.size(); ++iCheckbox) {
		TGCheckButton* aCheckbox = new TGCheckButton(fFrame,
			checkbox_names[iCheckbox].c_str(),
			checkbox_id[iCheckbox]);
		TGTableLayoutHints* tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
			kLHintsFillX | kLHintsFillY);
		fFrame->AddFrame(aCheckbox, tloh);
		aCheckbox->Connect("Clicked()", "MainFrame", this, "DoButton()");
		aCheckbox->SetToolTipText(checkbox_tooltips[iCheckbox].c_str());
		++attach_top;
		++attach_bottom;
		if (displayConfig != myConfig.not_found() && displayConfig->second.get(checkbox_config[iCheckbox], false)) {
			aCheckbox->SetState(kButtonDown, true);
		}
		if (checkbox_names[iCheckbox] == "Set reco mode" &&
			(myWorkMode == M_ONLINE_GRAW_MODE || myWorkMode == M_ONLINE_NGRAW_MODE)) {
			aCheckbox->SetState(kButtonUp, true);
			aCheckbox->SetState(kButtonDisabled);
		}
	}
	return attach_bottom;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int MainFrame::AddGoToEventDialog(int attach) {

	fGframe = new TGGroupFrame(this, "Go to event id.");
	fEventIdEntry = new TGNumberEntryField(fGframe, M_GOTO_EVENT, 0,
		TGNumberFormat::kNESInteger,
		TGNumberFormat::kNEANonNegative,
		TGNumberFormat::kNELNoLimits);
	fEventIdEntry->Connect("ReturnPressed()", "MainFrame", this, "DoButton()");
	fEventIdEntry->SetToolTipText("Jump to given event id.");

	TGTableLayout* aLayout = (TGTableLayout*)fFrame->GetLayoutManager();
	int nColumns = aLayout->fNcols;
	int nRows = aLayout->fNrows;

	UInt_t attach_left = 0.7 * nColumns;
	UInt_t attach_right = attach_left + 1;
	UInt_t attach_top = attach;
	UInt_t attach_bottom = attach_top + nRows * 0.08;
	TGTableLayoutHints* tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
		kLHintsFillX | kLHintsFillY,
		0, 0, 5, 2);
	fFrame->AddFrame(fGframe, tloh);
	fGframe->AddFrame(fEventIdEntry, new TGLayoutHints(kLHintsExpandX, 0, 0, 0, 0));

	return attach_bottom;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int MainFrame::AddGoToFileEntryDialog(int attach) {

	fGframe = new TGGroupFrame(this, "Go to frame.");
	fFileEntryEntry = new TGNumberEntryField(fGframe, M_GOTO_ENTRY, 0,
		TGNumberFormat::kNESInteger,
		TGNumberFormat::kNEANonNegative,
		TGNumberFormat::kNELNoLimits);
	fFileEntryEntry->Connect("ReturnPressed()", "MainFrame", this, "DoButton()");
	fFileEntryEntry->SetToolTipText("Jump to given event id.");

	TGTableLayout* aLayout = (TGTableLayout*)fFrame->GetLayoutManager();
	int nColumns = aLayout->fNcols;
	int nRows = aLayout->fNrows;

	UInt_t attach_left = 0.7 * nColumns;
	UInt_t attach_right = attach_left + 1;
	UInt_t attach_top = attach;
	UInt_t attach_bottom = attach_top + nRows * 0.08;
	TGTableLayoutHints* tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
		kLHintsFillX | kLHintsFillY,
		0, 0, 5, 2);
	fFrame->AddFrame(fGframe, tloh);
	fGframe->AddFrame(fFileEntryEntry, new TGLayoutHints(kLHintsExpandX, 0, 0, 0, 0));

	return attach_bottom;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int MainFrame::AddFileInfoFrame(int attach) {

	fFileInfoFrame = new FileInfoFrame(fFrame, this);

	TGTableLayout* aLayout = (TGTableLayout*)fFrame->GetLayoutManager();
	//int nRows = aLayout->fNrows;
	int nColumns = aLayout->fNcols;
	UInt_t attach_left = nColumns * 0.7 + 1;
	UInt_t attach_right = nColumns;
	UInt_t attach_top = attach;
	UInt_t attach_bottom = attach_top + 8;
	TGTableLayoutHints* tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
		kLHintsShrinkX | kLHintsShrinkY |
		kLHintsFillX | kLHintsFillY);
	fFileInfoFrame->initialize();
	fFrame->AddFrame(fFileInfoFrame, tloh);
	return attach_bottom;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int MainFrame::AddEventTypeDialog(int attach) {

	eventTypeButtonGroup = new TGButtonGroup(fFrame,
		10, 1, 1.0, 1.0,
		"Event type");
	eventTypeButtonGroup->SetExclusive(kFALSE);
	std::vector<TGCheckButton*> buttonsContainer;
	buttonsContainer.push_back(new TGCheckButton(eventTypeButtonGroup, new TGHotString("Noise")));
	buttonsContainer.push_back(new TGCheckButton(eventTypeButtonGroup, new TGHotString("Multi-vertex")));
	buttonsContainer.push_back(new TGCheckButton(eventTypeButtonGroup, new TGHotString("Fractured track")));
	buttonsContainer.push_back(new TGCheckButton(eventTypeButtonGroup, new TGHotString("Pretty event")));
	buttonsContainer.push_back(new TGCheckButton(eventTypeButtonGroup, new TGHotString("Weird event")));
	buttonsContainer.push_back(new TGCheckButton(eventTypeButtonGroup, new TGHotString("Spare cat. 1")));
	buttonsContainer.push_back(new TGCheckButton(eventTypeButtonGroup, new TGHotString("Spare cat. 2")));
	buttonsContainer.push_back(new TGCheckButton(eventTypeButtonGroup, new TGHotString("Spare cat. 3")));

	TGTableLayout* aLayout = (TGTableLayout*)fFrame->GetLayoutManager();
	int nColumns = aLayout->fNcols;
	int nRows = aLayout->fNrows;

	UInt_t attach_left = nColumns * 0.7;
	UInt_t attach_right = attach_left + 1;
	UInt_t attach_top = attach;
	UInt_t attach_bottom = attach_top + nRows * 0.25;
	TGTableLayoutHints* tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom,
		kLHintsShrinkX | kLHintsShrinkY |
		kLHintsFillX | kLHintsFillY);
	fFrame->AddFrame(eventTypeButtonGroup, tloh);
	return attach_bottom;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int MainFrame::AddMarkersDialog(int attach) {

	TGTableLayout* aLayout = (TGTableLayout*)fFrame->GetLayoutManager();
	int nColumns = aLayout->fNcols;
	int nRows = aLayout->fNcols;
	UInt_t attach_left = nColumns * 0.7 + 1;
	UInt_t attach_right = nColumns;
	UInt_t attach_top = attach;
	UInt_t attach_bottom = attach_top + nRows * 0.2;

	fMarkersManager = new MarkersManager(fFrame, this);
	fMarkersManager->setGeometry(myEventSource->getGeometry());
	fMarkersManager->Connect("sendSegmentsData(std::vector<double> *)", "MainFrame",
		this, "processSegmentData(std::vector<double> *)");
	fMarkersManager->Connect("HandleButton(Long_t)", "MainFrame",
		this, "ProcessMessage(Long_t)");

	TGTableLayoutHints* tloh = new TGTableLayoutHints(attach_left, attach_right,
		attach_top, attach_bottom,
		kLHintsExpandX | kLHintsExpandY |
		kLHintsShrinkX | kLHintsShrinkY |
		kLHintsFillX | kLHintsFillY);
	fFrame->AddFrame(fMarkersManager, tloh);
	fMainCanvas->Connect("ProcessedEvent(Int_t,Int_t,Int_t,TObject*)",
		"MarkersManager", fMarkersManager,
		"HandleMarkerPosition(Int_t,Int_t,Int_t,TObject*)");
	return attach_bottom;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
int MainFrame::AddRunConditionsDialog(int attach) {

	TGTableLayout* aLayout = (TGTableLayout*)fFrame->GetLayoutManager();
	int nColumns = aLayout->fNcols;
	int nRows = aLayout->fNcols;
	UInt_t attach_left = nColumns * 0.7 + 1;
	UInt_t attach_right = nColumns;
	UInt_t attach_top = attach;
	UInt_t attach_bottom = attach_top + nRows * 0.35;

	fRunConditionsDialog = new RunConditionsDialog(fFrame, this);
	fRunConditionsDialog->Connect("updateRunConditions(std::vector<double>*)", "MainFrame",
		this, "updateRunConditions(std::vector<double>*)");
	if (myEventSource && myEventSource->getGeometry()) {
		fRunConditionsDialog->initialize(myEventSource->getGeometry()->getRunConditions());
	}
	else {
		std::cout << KRED << "ERROR " << RST << "Geometry not available for RunConditionsDialog.";
		std::cout << " dialog not added." << _endl_;
		return attach_bottom;
	}
	TGTableLayoutHints* tloh = new TGTableLayoutHints(attach_left, attach_right,
		attach_top, attach_bottom,
		kLHintsExpandX | kLHintsExpandY |
		kLHintsShrinkX | kLHintsShrinkY |
		kLHintsFillX | kLHintsFillY);
	fFrame->AddFrame(fRunConditionsDialog, tloh);
	return attach_bottom;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::AddLogos() {

	std::string filePath = myConfig.get<std::string>("input.resourcesPath") + "/FUW_znak.png";
	TImage* img = TImage::Open(filePath.c_str());
	if (!img->IsValid()) return;

	double ratio = img->GetWidth() / img->GetHeight();
	double height = 80;
	double width = ratio * height;
	img->Scale(width, height);
	///FIXME clean up the ipic at the application closure.
	const TGPicture* ipic = (TGPicture*)gClient->GetPicturePool()->GetPicture("FUW_znak", img->GetPixmap(), img->GetMask());
	delete img;
	TGIcon* icon = new TGIcon(fFrame, ipic, width, height);

	TGTableLayout* aLayout = (TGTableLayout*)fFrame->GetLayoutManager();
	int nColumns = aLayout->fNcols;
	int nRows = aLayout->fNrows;
	UInt_t attach_left = nColumns * 0.7 + 1;
	UInt_t attach_right = attach_left + 1;
	UInt_t attach_top = nRows * 0.9;
	UInt_t attach_bottom = nRows;

	TGTableLayoutHints* tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom);
	fFrame->AddFrame(icon, tloh);

	filePath = myConfig.get<std::string>("input.resourcesPath") + "/ELITEPC_znak.png";
	img = TImage::Open(filePath.c_str());
	if (!img->IsValid()) return;
	ratio = img->GetWidth() / img->GetHeight();
	height = 100;
	width = ratio * height;
	img->Scale(width, height);
	///FIXME clean up the ipic at the application closure.
	ipic = (TGPicture*)gClient->GetPicturePool()->GetPicture("FUW_znak", img->GetPixmap(), img->GetMask());
	delete img;
	icon = new TGIcon(fFrame, ipic, width, height);

	attach_left = attach_right + 1;
	attach_right = nColumns;
	tloh = new TGTableLayoutHints(attach_left, attach_right, attach_top, attach_bottom, 0, 0, 0, -20);
	fFrame->AddFrame(icon, tloh);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::CloseWindow() {
	myHistoManager.~HistoManager();
	gApplication->Terminate(0);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::ClearCanvases() {

	myHistoManager.clearCanvas(fMainCanvas, isLogScaleOn);
	myHistoManager.clearCanvas(fRawHistosCanvas, isLogScaleOn);
	myHistoManager.clearCanvas(fTechHistosCanvas, isLogScaleOn);

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::Update() {

	if (!myEventSource || !myEventSource->numberOfEvents() ||
		!fFileInfoFrame || !fMarkersManager) {
		return;
	}
	fFileInfoFrame->updateFileName(myEventSource->getCurrentPath());
	fFileInfoFrame->updateEventNumbers(myEventSource->numberOfEvents(),
									   myEventSource->currentEventNumber(),
									   myEventSource->currentEntryNumber());
	myHistoManager.setEvent(myEventSource->getCurrentEvent());
	fMarkersManager->reset();
	fMarkersManager->setEnabled(isRecoModeOn);
	ClearCanvases();
	
	if (isRecoModeOn) myHistoManager.drawRecoHistos(fMainCanvas);
	else if(myConfig.get<bool>("display.develMode")) myHistoManager.drawDevelHistos(fMainCanvas);
	else if(myConfig.get<bool>("display.technicalMode")) myHistoManager.drawTechnicalHistos(fMainCanvas, myEventSource->getGeometry()->GetAgetNchips());
	else myHistoManager.drawRawHistos(fMainCanvas, isRateDisplayOn);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::processSegmentData(std::vector<double>* segmentsXY) {

	myHistoManager.drawRecoFromMarkers(fMainCanvas, segmentsXY);

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
unsigned long MainFrame::UpdateEventLog() {

	int index = myEventSource->getCurrentPath().find_last_of("/");
	int pathLength = myEventSource->getCurrentPath().size();
	std::string logFileName = myEventSource->getCurrentPath().substr(index + 1, pathLength);
	logFileName += ".log";
	std::fstream out(logFileName);

	///Log header
	if (!out.is_open()) {
		out.open(logFileName, std::ofstream::app);
		out << "Event type bits:" << _endl_;
		for (int iButton = 1; iButton <= eventTypeButtonGroup->GetCount(); ++iButton) {
			TGTextButton* aButton = (TGTextButton*)eventTypeButtonGroup->GetButton(iButton);
			if (!aButton) {
				std::cerr << __FUNCTION__ << " Coversion to TGTextButton failed!" << _endl_;
				continue;
			}
			out << iButton - 1 << " - " << aButton->GetString() << _endl_;
		}
		out << "\n";
		out << "Event Id \t entry number \t Event type" << _endl_;
	}
	/////
	out.close();
	out.open(logFileName, std::ofstream::app);

	if (!eventTypeButtonGroup) {
		std::cerr << "eventTypeButtonGroup not initialised!";
		return 0;
	}

	out << myEventSource->currentEventNumber() << " \t\t "
		<< myEventSource->currentEntryNumber() << " \t\t ";

	std::bitset<64> eventType;
	for (int iButton = 1; iButton <= eventTypeButtonGroup->GetCount(); ++iButton) {
		eventType.set(iButton - 1, eventTypeButtonGroup->GetButton(iButton)->IsOn());
		eventTypeButtonGroup->GetButton(iButton)->SetOn(kFALSE);
	}
	out << eventType.to_ulong() << _endl_;
	out.close();
	return eventType.to_ulong();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::updateRunConditions(std::vector<double>* runParams) {

	if (!runParams || !myEventSource ||
		!myEventSource->getGeometry() ||
		runParams->size() < 3) return;
	myEventSource->getGeometry()->setDriftVelocity(runParams->at(0));
	myEventSource->getGeometry()->setSamplingRate(runParams->at(1));
	myEventSource->getGeometry()->setTriggerDelay(runParams->at(2));
	std::cout << myEventSource->getGeometry()->getRunConditions() << _endl_;
	if (isRecoModeOn) {
		ClearCanvases();
		Update();
	}
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Bool_t MainFrame::ProcessMessage(Long_t msg, Long_t parm1, Long_t) {

	return kTRUE;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Bool_t MainFrame::ProcessMessage() {

	return kTRUE;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Bool_t MainFrame::ProcessMessage(Long_t msg) {

	switch (msg) {
	case M_CLEAR_TRACKS:
	{
		myHistoManager.clearTracks();
		fMainCanvas->Update();
	}
	break;
	case M_FIT_SEGMENT:
	{
		myHistoManager.clearTracks();
	}
	break;

	case M_DATA_FILE_UPDATED:
	{
	}
	break;
	}
	return kTRUE;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Bool_t MainFrame::ProcessMessage(const char* msg) {

#ifdef DEBUG
	std::cout << __FUNCTION__ << " msg: " << msg << _endl_;
#endif
	myMutex.lock();
	myEventSource->loadDataFile(std::string(msg));
	myEventSource->getLastEvent();
	Update();
	myMutex.unlock();
	return kTRUE;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::HandleMenu(Int_t id) {

	const char* filetypes[] = { "ROOT files",    "*.root",
				   0,               0 };

	if (myWorkMode == M_ONLINE_GRAW_MODE || myWorkMode == M_OFFLINE_NGRAW_MODE) {
		filetypes[0] = "GRAW files";
		filetypes[1] = "*.graw";
	}

	switch (id) {
	case M_FILE_OPEN:
	{
		// initialising with directory of previously opened file
		// this is very naive and risky implementation of extracting dir from path
		// but we don't have std::filesystem c++17 and ROOT 6.08 doesn't have TSystem::GetDirName
		// this will cause problem if we add changing directories in ONLINE mode 
		auto currentFilePath = myEventSource->getCurrentPath();
		auto dirPath = currentFilePath.substr(0, currentFilePath.find_last_of('/'));
		TGFileInfo fi;
		fi.fFileTypes = filetypes;
		fi.fIniDir = StrDup(dirPath.c_str());
		//
		std::string oldDirectory = gSystem->GetWorkingDirectory();
		new TGFileDialog(gClient->GetRoot(), this, kFDOpen, &fi);
		std::string fileName;
		if (fi.fFilename) fileName.append(fi.fFilename);
		else return;
		gSystem->cd(oldDirectory.c_str());
		myEventSource->loadDataFile(fileName);
		myEventSource->loadFileEntry(0);
		Update();
	}
	break;

	case M_FILE_SAVEAS:
	{
		TGFileInfo fi;
		fi.fFileTypes = filetypes;
		fi.fIniDir = StrDup(".");
		fi.fFilename = StrDup("selections.dat");
		new TGFileDialog(gClient->GetRoot(), this, kFDSave, &fi);
		std::string fileName;
		if (fi.fFilename) fileName.append(fi.fFilename);
	}
	break;

	case M_NEXT_EVENT:
	{
		unsigned int eventType = UpdateEventLog();
		if (isRecoModeOn) myHistoManager.writeRecoData(eventType);
		myEventSource->getNextEventLoop();
		Update();
	}
	break;
	case M_PREVIOUS_EVENT:
	{
		UpdateEventLog();
		myEventSource->getPreviousEventLoop();
		Update();
	}
	break;
	case M_RESET_EVENT:
	{
		Update();
	}
	break;
	case M_RESET_RATE:
	{
		myHistoManager.resetEventRateGraph();
		Update();
	}
	break;
	case M_TOGGLE_AUTOZOOM:
	{
		myHistoManager.toggleAutozoom();
		Update();
	}
	break;
	case M_TOGGLE_LOGSCALE:
	{
		isLogScaleOn = !isLogScaleOn;
		Update();
	}
	break;
	case M_TOGGLE_RECOMODE:
	{
		isRecoModeOn = !isRecoModeOn;
		std::string dataFileName = myConfig.get("input.dataFile", "");
		if (isRecoModeOn) myHistoManager.openOutputStream(dataFileName);
		Update();
	}
	break;
	case M_TOGGLE_RATE:
	{
		isRateDisplayOn = !isRateDisplayOn;
		Update();
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
	{
		CloseWindow();   // terminate theApp no need to use SendCloseMessage()
	}
	break;
	}
}
////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void MainFrame::DoButton() {
	TGButton* button = (TGButton*)gTQSender;
	UInt_t button_id = button->WidgetId();
	HandleMenu(button_id);
}
////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
