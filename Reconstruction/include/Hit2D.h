#ifndef _Hit2D_H_
#define _Hit2D_H_

#include <vector>
#include <memory>

#include "CommonDefinitions.h"

class Hit2D {

public:

  Hit2D(double aPosTime, double aPosUVW, double aCharge) : posUVW(aPosUVW), posTime(aPosTime), charge(aCharge) { }

  double getPosUVW() const { return posUVW;}

  double getPosTime() const { return posTime;}

  double getCharge() const {return charge;}
  
private:
  double posUVW, posTime;
  double charge;

};

typedef std::vector<Hit2D> Hit2DCollection;

#endif

