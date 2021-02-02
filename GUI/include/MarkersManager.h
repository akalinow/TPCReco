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

  MainFrame *fParentFrame = 0;
  TGVerticalFrame *fTopFrame = 0;
  TGCanvas *fMarkerGCanvas = 0;

  TMarker *firstMarker, *secondMarker;
  TLine *currentLine;

  std::vector<TMarker*> fMarkersContainer;
  int tmp=0;


};

#endif
