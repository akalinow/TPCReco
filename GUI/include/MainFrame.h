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
#include "FileInfoFrame.h"
#include "SelectionBox.h"
#include "MarkersManager.h"
#include "RunConditionsDialog.h"

#include "EventSourceBase.h"
#include "HistoManager.h"
#include "DirectoryWatch.h"

#include <boost/property_tree/json_parser.hpp>

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
class MainFrame : public TGMainFrame{

  RQ_OBJECT("MainFrame")

public:
  
  MainFrame(const TGWindow *p, UInt_t w, UInt_t h, const boost::property_tree::ptree &aConfig);
  virtual ~MainFrame();
  virtual void CloseWindow();

  virtual Bool_t ProcessMessage();
  virtual Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t);
  virtual Bool_t ProcessMessage(Long_t msg);
  virtual Bool_t ProcessMessage(const char *);

  void processSegmentData(std::vector<double> *segmentsXY);
  void updateRunConditions(std::vector<double> *runParams);

  void HandleMenu(Int_t);

  void DoButton();

private:
  
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
  int AddFileInfoFrame(int attach);
  int AddRunConditionsDialog(int attach);

  void AddLogos();

  void SetCursorTheme();

  void ClearCanvases();
  void Update();
  unsigned long UpdateEventLog();

  boost::property_tree::ptree myConfig;
  int myWorkMode{0};
  bool isLogScaleOn{false}, isRecoModeOn{false};
  bool isRateDisplayOn{false};

  std::shared_ptr<EventSourceBase> myEventSource;
  HistoManager myHistoManager;

  DirectoryWatch myDirWatch;
  std::thread fileWatchThread;
  std::mutex myMutex;

  TGCompositeFrame *fFrame{0};
  TRootEmbeddedCanvas *embeddedCanvas{0};
  TCanvas *fHistosCanvas{0};
  TCanvas *fRawHistosCanvas{0};
  TCanvas *fTechHistosCanvas{0};
  TGMenuBar *fMenuBar{0};
  TGPopupMenu *fMenuFile{0}, *fMenuHelp{0};

  TGLayoutHints *fFrameLayout{0};
  TGTableLayoutHints *fTCanvasLayout{0};

  TGTransientFrame *fLegendMain{0};

  TGNumberEntryField *fEventIdEntry{0}, *fFileEntryEntry{0};
  TGGroupFrame *fGframe{0};
  TGButtonGroup *eventTypeButtonGroup{0};

  MarkersManager *fMarkersManager{0};
  FileInfoFrame *fFileInfoFrame{0};
  SelectionBox *fSelectionBox{0};
  RunConditionsDialog *fRunConditionsDialog{0};

  ClassDef(MainFrame, 0);
};
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
#endif
