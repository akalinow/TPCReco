#ifndef TPCRECO_GUI_HELPER_LINE_H_
#define TPCRECO_GUI_HELPER_LINE_H_

#include <TLine.h>

class TPad;
class TFrame;

class HelperLine {
public:

  HelperLine(TPad *pad = nullptr);
  void drawVertical(double x) {draw(Vertical{x});}
  void drawHorizontal(double y) {draw(Horizontal{y});}
  void clear();

  inline TLine &getPrototype() noexcept { return prototype; }
  inline void setPad(TPad *pad) noexcept { this->pad = pad; }
  inline TPad *getPad() const noexcept { return pad; }

private:
  struct Horizontal {
    double y;
  };
  struct Vertical {
    double x;
  };
  template <class Orientation> void draw(Orientation orientation);
  void drawHelper(TFrame *frame, Horizontal y);
  void drawHelper(TFrame *frame, Vertical x);

  TPad *pad;
  TLine prototype;
  TLine *line = nullptr;
};

#endif // TPCRECO_GUI_HELPER_LINE_H_