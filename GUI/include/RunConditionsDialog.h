#ifndef RunConditionsDialog_H
#define RunConditionsDialog_H

#include <TGFrame.h>
#include <TGLayout.h>
#include <TGLabel.h>
#include <TGNumberEntry.h>
#include <TGButton.h>

#include "RunConditions.h"

class RunConditionsDialog : public TGCompositeFrame {

  RQ_OBJECT("RunConditionsDialog")

public:
   RunConditionsDialog(const TGWindow *p, MainFrame *aFrame);

   virtual ~RunConditionsDialog();

  void initialize(const RunConditions & aRunConditions);

  void updateRunConditions(std::vector<double> *dummyArgs=0);
  
private:

  void addButtons();

  std::map<std::string, TGNumberEntry*> myDialogs;
  std::vector<double> runParams;
  MainFrame *fParentFrame;
  TGGroupFrame *fHeaderFrame;
  
};

#endif
