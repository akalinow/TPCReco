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
#include <TGButton.h>
#include <RQ_OBJECT.h>
#include <TCanvas.h>
#include <TObject.h>
#include <TClass.h>
#include <TLine.h>
#include <TArrow.h>

#include "GUI_commons.h"
#include "EntryDialog.h"
#include "SelectionBox.h"
#include "MarkersManager.h"

#include "EventSourceBase.h"
#include "HistoManager.h"
#include "DirectoryWatch.h"

#include <boost/property_tree/json_parser.hpp>


/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
class MainFrame : public TGMainFrame {

  RQ_OBJECT("MainFrame")

  public:
  MainFrame(const TGWindow *p, UInt_t w, UInt_t h, const boost::property_tree::ptree &aConfig);
  virtual ~MainFrame();
  virtual void CloseWindow();
  virtual Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t);
  virtual Bool_t ProcessMessage(Long_t msg);
  virtual Bool_t ProcessMessage(const char *);

  void drawRecoFromMarkers(std::vector<double> * segmentsXY);

  void HandleEmbeddedCanvas(Int_t event, Int_t x, Int_t y, TObject *sel);

  void HandleMenu(Int_t);

  void DoButton();

private:

  boost::property_tree::ptree myConfig;
  int myWorkMode = 0;
  bool isLogScaleOn{false};
  std::shared_ptr<EventSourceBase> myEventSource;
  HistoManager myHistoManager;

  DirectoryWatch myDirWatch;
  std::thread fileWatchThread;
  std::mutex myMutex; 

  void InitializeEventSource();
  void InitializeWindows();
  
  void AddTopMenu();
  void SetTheFrame();
  void AddHistoCanvas();
  
  int AddButtons(int attach);
  int AddGoToFileEntryDialog(int attach);
  int AddGoToEventDialog(int attach);
  int AddEventTypeDialog(int attach);
  int AddMarkersDialog(int attach);
  int AddNumbersDialog(int attach);
  
  void AddLogos();

  void SetCursorTheme();

  void drawRawHistos();
  void drawRecoHistos();

  void Update();
  void UpdateEventLog();

  TGCompositeFrame   *fFrame;
  TRootEmbeddedCanvas *embeddedCanvas;
  TCanvas            *fCanvas;

  TGMenuBar          *fMenuBar;
  TGPopupMenu        *fMenuFile, *fMenuHelp;

  TGLayoutHints      *fFrameLayout;
  TGTableLayoutHints *fTCanvasLayout;

  TGTransientFrame *fLegendMain;

  TGNumberEntryField *fEventIdEntry, *fFileEntryEntry;
  TGGroupFrame        *fGframe;
  TGButtonGroup *eventTypeButtonGroup;

  MarkersManager *fMarkersManager;  
  EntryDialog *fEntryDialog;
  SelectionBox *fSelectionBox;

  TArrow *fArrow;
  TLine *fLine;

  ClassDef(MainFrame, 0); 
};
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
#endif
