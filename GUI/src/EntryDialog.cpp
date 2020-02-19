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
   std::string tmp = "No input.";
   tmp.resize(fileNameLineLength,' ');
   fileNameLabel = new TGLabel(fileNameFrame,tmp.c_str());

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
  size_t previousBreakPoint = 0;
  for(size_t iPos=0;iPos<fileNameWithBreaks.size();){
    iPos = fileNameWithBreaks.find("/",iPos+1);
    if(fileNameLineLength-iPos+previousBreakPoint<10){
      fileNameWithBreaks.insert(iPos+1,"\n");
      previousBreakPoint = iPos;
    }
  }
  fileNameLabel->SetText(fileNameWithBreaks.c_str());  
  fileInfoFrame->Layout();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Bool_t EntryDialog::ProcessMessage(Long_t msg, Long_t parm1, Long_t /*parm2*/){
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
