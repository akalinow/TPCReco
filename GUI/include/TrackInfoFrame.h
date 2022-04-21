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

  void updateFileName(const std::string & fileName);

  void updateModeLabel(const std::string & aMode);

private:

  MainFrame *theMainFrame;

  TGHorizontalFrame* trackInfoFrame;
  TGHorizontalFrame* modeInfoFrame;
  
  TGLabel *fileNameLabel;
  TGLabel *modeLabel;

  unsigned int fileNameLineLength = 50;

};

#endif
