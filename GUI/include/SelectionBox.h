#ifndef SelectionBox_h
#define SelectionBox_h

#include "root/include/TGClient.h"
#include "root/include/TGButton.h"
#include "root/include/TGListBox.h"
#include "root/include/TList.h"
#include "root/include/RQ_OBJECT.h"

class SelectionBox {

  RQ_OBJECT("SelectionBox")

private:

   TGTransientFrame    *fMain;
   TGHorizontalFrame   *fFrame;
   TGListBox           *fListBox;
   TList               *fSelected;

public:

   SelectionBox(const TGWindow *p, TGWindow *main, uint32_t w, uint32_t h,
                uint32_t options = kVerticalFrame);
   virtual ~SelectionBox();

   const TList* GetSelected() const;

   void Initialize(const std::vector<std::string> &);

   void DoExit();
   void DoSelect(int64_t msg = 0);
   void HandleButtons();

   ClassDef(SelectionBox, 0)

};
#endif
