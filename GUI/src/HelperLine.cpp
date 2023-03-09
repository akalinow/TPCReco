#include "TPCReco/HelperLine.h"
#include <TFrame.h>
#include <TPad.h>

HelperLine::HelperLine(TPad *pad) : pad(pad), prototype(1, 0, 10, 0) {
  prototype.SetLineColor(1);
  prototype.SetLineWidth(2);
  prototype.SetBit(kCannotPick);
}

void HelperLine::clear() {
  delete line;
  line = nullptr;
}

template <class Orientation> void HelperLine::draw(Orientation orientation) {
  if (!pad) {
    return;
  }
  pad->cd();
  auto *frame = static_cast<TFrame *>(pad->GetListOfPrimitives()->At(0));
  if (!frame) {
    return;
  }
  drawHelper(frame, orientation);
  pad->Update();
}

void HelperLine::drawHelper(TFrame *frame, HelperLine::Vertical o) {
  double min = frame->GetY1();
  double max = frame->GetY2();
  if (line) {
    line->SetX1(o.x);
    line->SetY1(min);
    line->SetX2(o.x);
    line->SetY2(max);
    line->Draw();
  } else {
    line = prototype.DrawLine(o.x, min, o.x, max);
  }
}

void HelperLine::drawHelper(TFrame *frame, HelperLine::Horizontal o) {
  double min = frame->GetY1();
  double max = frame->GetY2();
  if (line) {
    line->SetX1(min);
    line->SetY1(o.y);
    line->SetX2(max);
    line->SetY2(o.y);
    line->Draw();
  } else {
    line = prototype.DrawLine(min, o.y, max, o.y);
  }
}

template void
HelperLine::draw<HelperLine::Horizontal>(HelperLine::Horizontal param);
template void
HelperLine::draw<HelperLine::Vertical>(HelperLine::Vertical param);