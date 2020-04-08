#ifndef MainFrame_H
#define MainFrame_H

#include <thread>
#include <mutex>
#include <string>

#include <TGDockableFrame.h>
#include <TGFrame.h>
#include <TGCanvas.h>
#include <TGLayout.h>
#include <TGTableLayout.h>
#include <TGMenu.h>
#include <TRootEmbeddedCanvas.h>
#include <TGFileDialog.h>
#include <TGFileBrowser.h>
#include <TGNumberEntry.h>
#include <TGIcon.h>
#include <TGPicture.h>

#include <RQ_OBJECT.h>
#include <TCanvas.h>
#include <TObject.h>
#include <TClass.h>
#include <TLine.h>
#include <TArrow.h>

#include "EntryDialog.h"
#include "SelectionBox.h"

#include "EventSourceBase.h"
#include "HistoManager.h"
#include "DirectoryWatch.h"

#include <boost/property_tree/json_parser.hpp>

enum ETestCommandIdentifiers {
	M_FILE_OPEN,
	M_FILE_SAVE,
	M_FILE_SAVEAS,
	M_FILE_PRINT,
	M_FILE_PRINTSETUP,
	M_FILE_EXIT,

	M_DIR_WATCH,

	M_HELP_CONTENTS,
	M_HELP_SEARCH,
	M_HELP_ABOUT,

	M_NEXT_EVENT,
	M_PREVIOUS_EVENT,
	M_GOTO_EVENT,

};

enum Messages {
	M_DATA_FILE_UPDATED

};

enum Modes {
	M_ONLINE_MODE,
	M_OFFLINE_GRAW_MODE,
	M_OFFLINE_ROOT_MODE
};
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
class MainFrame : public TGMainFrame {

	RQ_OBJECT("MainFrame")

public:
	MainFrame(const TGWindow* p, UInt_t w, UInt_t h, const boost::property_tree::ptree& aConfig);
	virtual ~MainFrame();
	virtual void CloseWindow();
	virtual Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t);
	virtual Bool_t ProcessMessage(Long_t msg);
	virtual Bool_t ProcessMessage(const char*);

	void HandleEmbeddedCanvas(Int_t event, Int_t x, Int_t y, TObject* sel);

	void HandleMenu(Int_t);

	void DoButton();

private:

	boost::property_tree::ptree myConfig;
	int myWorkMode = 0;

	std::shared_ptr<EventSourceBase> myEventSource;

	DirectoryWatch myDirWatch;
	std::thread fileWatchThread;
	std::mutex myMutex;

	void InitializeEventSource();
	void InitializeWindows();

	void AddTopMenu();
	void SetTheFrame();
	void AddHistoCanvas();
	void AddButtons();
	void AddNumbersDialog();
	void AddGoToEventDialog(int attach_left);
	void AddLogos();

	void SetCursorTheme();

	void Update();

	TGCompositeFrame* fFrame;
	TRootEmbeddedCanvas* embeddedCanvas;
	TCanvas* fCanvas;

	std::unique_ptr<TGMenuBar> fMenuBar;
	std::unique_ptr<TGPopupMenu> fMenuFile, fMenuHelp;

	TGLayoutHints* fFrameLayout;
	TGTableLayoutHints* fTCanvasLayout;

	TGTransientFrame* fLegendMain;

	TGNumberEntryField* fEventIdEntry;
	std::unique_ptr<TGGroupFrame> fGframe;

	std::unique_ptr<EntryDialog> fEntryDialog;
	SelectionBox* fSelectionBox;


	TArrow* fArrow;
	TLine* fLine;


	ClassDef(MainFrame, 0);
};
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
#endif
