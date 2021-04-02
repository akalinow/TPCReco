#ifndef MarkersManager_H
#define MarkersManager_H

#include <string>

#include <TMarker.h>
#include <TLine.h>

#include <TGFrame.h>

#include "GUI_commons.h"

class MainFrame;
class TGCanvas;


class MarkersManager : public TGCompositeFrame {

public:
  MarkersManager(const TGWindow *p, MainFrame *aFrame);
  virtual ~MarkersManager();

  void initialize();  
  void DoButton();  
  void HandleMarkerPosition(Int_t,Int_t,Int_t,TObject*);
  
private:

  void addMarkerFrame(int iMarker);
  void processClickCoordinates(int iDir, float x, float y);
  void drawFixedTimeLines(int iDir, double time);
  int findMissingMarkerDir();
  double getMissingYCoordinate(unsigned int missingMarkerDir);

  void resetMarkers();
  void clearHelperLines();
  void updateSegments(int strip_dir);
  bool isLastSegmentComplete(int strip_dir);
  
  Bool_t HandleButton(Int_t id);

  MainFrame *fParentFrame;
  TGVerticalFrame *fTopFrame;
  TGCanvas *fMarkerGCanvas;

  TGTextButton* addSegmentButton;

  TMarker *firstMarker;
  std::vector<TMarker*> fMarkersContainer;
  std::vector<TLine*> fHelperLinesContainer;
  std::vector<std::vector<TLine>> fSegmentsContainer;
  
  bool acceptPoints;
};

#endif
