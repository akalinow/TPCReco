#include <cstdlib>
#include <iostream>

#include <TGFrame.h>
#include <TGLabel.h>
#include <TGFontDialog.h>

#include <EntryDialog.h>
#include <MainFrame.h>
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EntryDialog::EntryDialog(const TGWindow * p, MainFrame * aFrame)
 : TGCompositeFrame(p, 10, 10, kVerticalFrame), theMainFrame(aFrame){

   SetCleanup(kDeepCleanup);

   datasetInfoFrame = new TGHorizontalFrame(this, 300, 100);
   fileInfoFrame = new TGHorizontalFrame(this, 300, 100);
   
   TGLayoutHints *aLayoutHints = new TGLayoutHints(kLHintsTop | kLHintsLeft  |
						 kLHintsShrinkX|kLHintsShrinkY |
						 kLHintsFillX|kLHintsFillY, 2, 2, 2, 2);
   
   AddFrame(fileInfoFrame, new TGLayoutHints(kLHintsFillX, 2, 2, 2, 2));
   AddFrame(datasetInfoFrame, aLayoutHints);
   
   TGGroupFrame *totalEventsFrame = new TGGroupFrame(datasetInfoFrame, "Events in the file:");
   TGGroupFrame *currentEventFrame = new TGGroupFrame(datasetInfoFrame, "Current event:");
   TGGroupFrame *fileNameFrame = new TGGroupFrame(fileInfoFrame, "Processing file:");
   
   totalEventsLabel = new TGLabel(totalEventsFrame, "No input.");
   currentEventLabel = new TGLabel(currentEventFrame, "No input.");
   fileNameLabel = new TGLabel(fileNameFrame, "No input.");

   aLayoutHints = new TGLayoutHints(kLHintsTop | kLHintsLeft, 5, 5, 5, 5);
   totalEventsFrame->AddFrame(totalEventsLabel, aLayoutHints);
   currentEventFrame->AddFrame(currentEventLabel, aLayoutHints);
   fileNameFrame->AddFrame(fileNameLabel, aLayoutHints);

   aLayoutHints = new TGLayoutHints(kLHintsTop | kLHintsLeft, 2, 2, 1, 1);
   datasetInfoFrame->AddFrame(totalEventsFrame, aLayoutHints);
   datasetInfoFrame->AddFrame(currentEventFrame, aLayoutHints);
   fileInfoFrame->AddFrame(fileNameFrame, aLayoutHints);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
EntryDialog::~EntryDialog(){

  delete datasetInfoFrame;
  delete totalEventsLabel;
  delete currentEventLabel;

  delete fileInfoFrame;
  delete fileNameLabel;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EntryDialog::initialize(){ }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EntryDialog::updateEventNumbers(unsigned int nTotalEvents,
                                     unsigned int iCurrentEvent){
 
  totalEventsLabel->SetText(Form("%u", nTotalEvents));
  currentEventLabel->SetText(Form("%u",iCurrentEvent));
  
  datasetInfoFrame->Layout();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void EntryDialog::updateFileName(const std::string & fileName){

  std::string fileNameWithBreaks = fileName;
  unsigned int lineLength = 40;
  for(unsigned int iPos=lineLength;iPos<lineLength*5;iPos+=lineLength)
  if(fileName.size()>iPos){
    fileNameWithBreaks.insert(iPos,"\n");
  }
  fileNameLabel->SetText(fileNameWithBreaks.c_str());  
  fileInfoFrame->Layout();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

bool EntryDialog::ProcessMessage(int64_t msg, int64_t parm1, int64_t /*parm2*/){
   switch (GET_MSG(msg)) {
   case kC_COMMAND:
      {
         switch (GET_SUBMSG(msg)) {
         case kCM_BUTTON:
            {
               switch (parm1) {
                  // exit button
               case 1:
                  {
                     std::cout<<"EntryDialog::ProcessMessage(): msg: "<<msg<<std::endl;
                     break;
                  }
                  // set button
               case 2:
                  {
                     break;
                  }
               }
               break;
            }
         }
         break;
      }
   }
   return kTRUE;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
