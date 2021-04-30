#ifndef FileInfoFrame_H
#define FileInfoFrame_H

#include <string>

#include <TGFrame.h>
#include <TGLayout.h>
#include <TGLabel.h>

class MainFrame;

class FileInfoFrame : public TGCompositeFrame {

public:
   FileInfoFrame(const TGWindow *p, MainFrame *aFrame);

   void initialize();

   virtual ~FileInfoFrame();

   virtual Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t);

  void updateFileName(const std::string & fileName);

  void updateEventNumbers(unsigned int nTotalEvents,
			  unsigned int iCurrentEvent,
			  unsigned int iCurrentEntry);

  void updateModeLabel(const std::string & aMode);

private:

  MainFrame *theMainFrame;

  TGHorizontalFrame *datasetInfoFrame;
  TGHorizontalFrame* fileInfoFrame;
  TGHorizontalFrame* modeInfoFrame;
  
  TGLabel *totalEventsLabel;
  TGLabel *currentEventLabel;
  TGLabel *currentEntryLabel;
  TGLabel *fileNameLabel;
  TGLabel *modeLabel;

  unsigned int fileNameLineLength = 36;

};

#endif
