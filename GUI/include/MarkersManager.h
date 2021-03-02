#ifndef MarkersManager_H
#define MarkersManager_H

#include <string>

#include <TMarker.h>
#include <TLine.h>

#include <TGFrame.h>

class MainFrame;
class TGCanvas;


class MarkersManager : public TGCompositeFrame {

public:
  MarkersManager(const TGWindow *p, MainFrame *aFrame);
  
  void initialize();
  void reset();
  
  virtual ~MarkersManager();
  
  virtual Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t);
  
  void HandleMarkerPosition(Int_t,Int_t,Int_t,TObject*);
  
private:

  void addMarkerFrame(int iMarker);
  void processClickCoordinates(int iDir, float x, float y);
  void drawFixedTimeLines(int iDir, double time);
  int findMissingMarkerDir();
  double getMissingYCoordinate(unsigned int missingMarkerDir);

  MainFrame *fParentFrame;
  TGVerticalFrame *fTopFrame;
  TGCanvas *fMarkerGCanvas;

  TLine *aLine;
  TMarker *firstMarker, *secondMarker;
  std::vector<TMarker*> fMarkersContainer;
  int tmp=0;


};

#endif
