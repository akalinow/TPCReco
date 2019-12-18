 #include <ScrollFrame.h>
#include <iostream>

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
ScrollFrame::ScrollFrame(const TGWindow *p){


   fFrame = new TGCompositeFrame(p, 10, 10, kVerticalFrame);
   fFrame->Connect("ProcessedEvent(Event_t*)", "ScrollFrame", this,
                   "HandleMouseWheel(Event_t*)");
   fCanvas = 0;
   fFrame->SetLayoutManager(new TGTileLayout(fFrame, 8));

   gVirtualX->GrabButton(fFrame->GetId(), kAnyButton, kAnyModifier,
                         kButtonPressMask | kButtonReleaseMask |
                         kPointerMotionMask, kNone, kNone);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void ScrollFrame::HandleMouseWheel(Event_t *event){
   // Handle mouse wheel to scroll.

   if (event->fType != kButtonPress && event->fType != kButtonRelease)
      return;

   int32_t page = 0;
   if (event->fCode == kButton4 || event->fCode == kButton5) {
      if (fCanvas != nullptr) return;
      if (fCanvas->GetContainer()->GetHeight())
         page = int32_t(float(pow(fCanvas->GetViewPort()->GetHeight(), 2) /
                              fCanvas->GetContainer()->GetHeight()));
   }

   if (event->fCode == kButton4) {
      //scroll up
      int32_t newpos = fCanvas->GetVsbPosition() - page;
      if (newpos < 0) newpos = 0;
      fCanvas->SetVsbPosition(newpos);
   }
   if (event->fCode == kButton5) {
      // scroll down
      int32_t newpos = fCanvas->GetVsbPosition() + page;
      fCanvas->SetVsbPosition(newpos);
   }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
