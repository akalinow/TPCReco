#ifndef TrackInfoFrame_H
#define TrackInfoFrame_H

#include <string>

#include <TGFrame.h>
#include <TGLayout.h>
#include <TGLabel.h>

class MainFrame;

class TrackInfoFrame : public TGCompositeFrame {

public:
   TrackInfoFrame(const TGWindow *p, MainFrame *aFrame);

   void initialize();

   virtual ~TrackInfoFrame();

   virtual Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t);

  void updateTrackInfo(const std::string & trackInfo);

private:

  MainFrame *theMainFrame;

  TGHorizontalFrame* trackInfoFrame;
  TGHorizontalFrame* modeInfoFrame;
  
  TGLabel *fileNameLabel;

  unsigned int fileNameLineLength = 50;

};

#endif