#ifndef _Hit2D_H_
#define _Hit2D_H_

#include <vector>
#include <memory>

#include "TPCReco/CommonDefinitions.h"

class Hit2D {

public:

  Hit2D() : posStrip(-999), posTime(-999), charge(-999) { }
  
  Hit2D(double aPosTime, double aPosStrip, double aCharge) : posStrip(aPosStrip), posTime(aPosTime), charge(aCharge) { }

  double getPosStrip() const { return posStrip;}

  double getPosTime() const { return posTime;}

  double getCharge() const {return charge;}
  
private:
  double posStrip, posTime;
  double charge;

};

typedef std::vector<Hit2D> Hit2DCollection;

std::ostream & operator << (std::ostream &out, const Hit2D &aHit);

#endif

