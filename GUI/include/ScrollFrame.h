#ifndef ScrollFrame_H
#define ScrollFrame_H

#include <root/include/RQ_OBJECT.h>
#include <root/include/TGFrame.h>
#include <root/include/TGCanvas.h>


class ScrollFrame {

RQ_OBJECT("ScrollFrame")

private:
   TGCompositeFrame *fFrame;
   TGCanvas         *fCanvas;

public:
   ScrollFrame(const TGWindow *p);
   virtual ~ScrollFrame() { delete fFrame; }

   TGFrame *GetFrame() const { return fFrame; }

   void SetCanvas(TGCanvas *canvas) { fCanvas = canvas; }
   void HandleMouseWheel(Event_t *event);
};

#endif
