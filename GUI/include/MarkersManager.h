#ifndef MarkersManager_H
#define MarkersManager_H

#include <string>
#include <map>

#include <TMarker.h>
#include <TLine.h>

#include <TGFrame.h>
#include <RQ_OBJECT.h>

#include "GUI_commons.h"
#include "GeometryTPC.h" // added by MC - 19 Aug 2021

class MainFrame;
class TGCanvas;


class MarkersManager : public TGCompositeFrame {

  RQ_OBJECT("MarkersManager")

public:
  MarkersManager(const TGWindow *p, MainFrame *aFrame);
  virtual ~MarkersManager();

  void setEnabled(bool enable);
  void initialize();
  void reset();
  void DoButton();  
  void HandleMarkerPosition(Int_t,Int_t,Int_t,TObject*);
  void setGeometry(GeometryTPC *geo); // added by MC - 19 Aug 2021
  
private:

  void addMarkerFrame(int iMarker);
  void addButtons();
  void processClickCoordinates(int iDir, float x, float y);
  void drawFixedTimeLines(int iDir, double time);
  int findMissingMarkerDir();
  double getMissingYCoordinate(unsigned int missingMarkerDir);

  void resetMarkers(bool force=false);
  void resetSegments();
  void clearHelperLines();
  void updateSegments(int strip_dir);
  bool isLastSegmentComplete(int strip_dir);

  void setPadsEditable(bool isEditable);

  void repackSegmentsData();
  void sendSegmentsData(std::vector<double> *segmentsXY);
  
  Bool_t HandleButton(Int_t id);

  MainFrame *fParentFrame;
  TGGroupFrame *fHeaderFrame;
  TGCanvas *fMarkerGCanvas;
  GeometryTPC *fGeometryTPC; // added by MC - 19 Aug 2021

  std::map<std::string, TGTextButton*> myButtons;

  TMarker *firstMarker;
  std::vector<TMarker*> fMarkersContainer;
  std::vector<TLine*> fHelperLinesContainer;
  std::vector<std::vector<TLine>> fSegmentsContainer;
  std::vector<double> fSegmentsXY;
  
  bool acceptPoints;
};

#endif
