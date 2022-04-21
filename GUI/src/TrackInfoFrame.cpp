#include <cstdlib>
#include <iostream>

#include <TGFrame.h>
#include <TGLabel.h>
#include <TGFontDialog.h>

#include <TrackInfoFrame.h>
#include <MainFrame.h>
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackInfoFrame::TrackInfoFrame(const TGWindow * p, MainFrame * aFrame)
 : TGCompositeFrame(p, 10, 10, kVerticalFrame), theMainFrame(aFrame){

   SetCleanup(kDeepCleanup);
   
   trackInfoFrame = new TGHorizontalFrame(this, 300, 50);

   TGLayoutHints *aLayoutHints = new TGLayoutHints(kLHintsTop | kLHintsLeft  |
						 kLHintsShrinkX|kLHintsShrinkY |
						 kLHintsFillX|kLHintsFillY, 2, 2, 2, 2);

   AddFrame(trackInfoFrame, new TGLayoutHints(kLHintsExpandX, 2, 2, 2, 2));

   TGGroupFrame *fileNameFrame = new TGGroupFrame(trackInfoFrame, "kekapepega:");

   std::string tmp = "No input.";
   tmp.resize(fileNameLineLength,' ');
   fileNameLabel = new TGLabel(fileNameFrame,tmp.c_str());
   fileNameLabel->SetWrapLength(280);

   aLayoutHints = new TGLayoutHints(kLHintsExpandX, 5, 5, 5, 5);
   fileNameFrame->AddFrame(fileNameLabel, aLayoutHints);

   aLayoutHints = new TGLayoutHints(kLHintsTop | kLHintsLeft | kLHintsFillX, 2, 2, -5, -5);
   trackInfoFrame->AddFrame(fileNameFrame, aLayoutHints);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackInfoFrame::~TrackInfoFrame(){

  delete trackInfoFrame;
  delete fileNameLabel;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackInfoFrame::initialize(){ }
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackInfoFrame::updateFileName(const std::string & fileName){

  std::string fileNameWithBreaks = fileName;
  size_t previousBreakPoint = 0;
  for(size_t iPos=0;iPos<fileNameWithBreaks.size();){
    iPos = fileNameWithBreaks.find("/",iPos+1);
    bool longPartFromStart = fileNameLineLength-iPos+previousBreakPoint<10;
    if(longPartFromStart){
      fileNameWithBreaks.insert(iPos+1,"\n");
      previousBreakPoint = iPos;
    }
  }

  size_t iPos = fileNameWithBreaks.find_last_of("/");
  bool longLineToEnd = fileNameWithBreaks.length() - iPos>fileNameLineLength;
  if(longLineToEnd){
    fileNameWithBreaks.insert(iPos+1,"\n");
  }
  
  fileNameLabel->SetText(fileNameWithBreaks.c_str());  
  trackInfoFrame->Layout();
  Resize();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackInfoFrame::updateModeLabel(const std::string& aMode) {

    modeLabel->SetText(aMode.c_str());
    modeInfoFrame->Layout();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Bool_t TrackInfoFrame::ProcessMessage(Long_t msg, Long_t parm1, Long_t /*parm2*/){
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
                     std::cout<<"TrackInfoFrame::ProcessMessage(): msg: "<<msg<<std::endl;
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
