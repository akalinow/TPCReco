#ifndef EntryDialog_H
#define EntryDialog_H

#include <TGFrame.h>
#include <TGLayout.h>
#include <TGLabel.h>

class MainFrame;

class EntryDialog : public TGCompositeFrame {

public:
   EntryDialog(const TGWindow *p, MainFrame *aFrame);

   void initialize();

   virtual ~EntryDialog();

   virtual Bool_t ProcessMessage(Long_t msg, Long_t parm1, Long_t);

  void updateFileName(const std::string & fileName);

  void updateEventNumbers(unsigned int nTotalEvents, unsigned int iCurrentEvent);

private:

  MainFrame *theMainFrame;

  TGHorizontalFrame *datasetInfoFrame;
  TGHorizontalFrame* fileInfoFrame;
  
  TGLabel *totalEventsLabel;
  TGLabel *currentEventLabel;
  TGLabel *fileNameLabel;

};

#endif
