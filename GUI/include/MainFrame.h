#ifndef MainFrame_H
#define MainFrame_H

#include <TGDockableFrame.h>
#include <TGFrame.h>
#include <TGCanvas.h>
#include <TGLayout.h>
#include <TGTableLayout.h>
#include <TGMenu.h>
#include <TRootEmbeddedCanvas.h>
#include <TGFileDialog.h>
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

#include "DataManager.h"
#include "HistoManager.h"

enum ETestCommandIdentifiers {
   M_FILE_OPEN,
   M_FILE_SAVE,
   M_FILE_SAVEAS,
   M_FILE_PRINT,
   M_FILE_PRINTSETUP,
   M_FILE_EXIT,

   M_HELP_CONTENTS,
   M_HELP_SEARCH,
   M_HELP_ABOUT,

   M_NEXT_EVENT,
   M_PREVIOUS_EVENT,
   M_GOTO_EVENT,

};
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
class MainFrame : public TGMainFrame {

  RQ_OBJECT("MainFrame")

public:
   MainFrame(const TGWindow *p, uint32_t w, uint32_t h);
   virtual ~MainFrame();
   virtual void CloseWindow();
   virtual bool ProcessMessage(int64_t msg, int64_t parm1, int64_t);

///Process message from EntryDialog
    virtual bool ProcessMessage(int64_t msg);

   void HandleEmbeddedCanvas(int32_t event, int32_t x, int32_t y, TObject *sel);

   void HandleMenu(int32_t);

   void DoButton();

private:

  DataManager myDataManager;
  HistoManager myHistoManager;
  
  void AddTopMenu();
  void SetTheFrame();
  void AddHistoCanvas();
  void AddButtons();
  void AddNumbersDialog();
  void AddGoToEventDialog(int attach_left);
  void AddLogos();

   void SetCursorTheme();

  void Update();

   TGCompositeFrame   *fFrame;
   TGCanvas           *fCanvasWindow;
   TCanvas            *fCanvas;

   TGMenuBar          *fMenuBar;
   TGPopupMenu        *fMenuFile, *fMenuHelp;

   TGLayoutHints      *fFrameLayout;
   TGTableLayoutHints *fTCanvasLayout;

   TGTransientFrame *fLegendMain;

  TGNumberEntryField *fEventIdEntry;
  TGGroupFrame        *fGframe;
  
  EntryDialog *fEntryDialog;
  SelectionBox *fSelectionBox;
  

   TArrow *fArrow;
   TLine *fLine;

};
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
#endif
