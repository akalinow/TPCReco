#ifndef SelectionBox_h
#define SelectionBox_h

#include <TGClient.h>
#include <TGButton.h>
#include <TGListBox.h>
#include <TList.h>
#include <RQ_OBJECT.h>

class SelectionBox {

  RQ_OBJECT("SelectionBox")

private:

   std::unique_ptr<TGTransientFrame> fMain;
   std::unique_ptr<TGHorizontalFrame> fFrame;
   std::unique_ptr<TGListBox> fListBox;
   std::unique_ptr<TList> fSelected;

public:

   SelectionBox(const TGWindow *p, TGWindow *main, UInt_t w, UInt_t h,
                UInt_t options = kVerticalFrame);
   virtual ~SelectionBox();

   void Initialize(const std::vector<std::string> &);

   void DoExit();
   void DoSelect(Long_t msg = 0);
   void HandleButtons();

   ClassDef(SelectionBox, 0)

};
#endif
