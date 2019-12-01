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

   virtual bool ProcessMessage(int64_t msg, int64_t parm1, int64_t);

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
