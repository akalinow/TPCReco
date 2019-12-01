#pragma once
#ifndef _Hit2D_H_
#define _Hit2D_H_

#include <vector>
#include <memory>

#include "CommonDefinitions.h"

class Hit2D {

public:

  Hit2D(double aPosTime, double aPosWire, double aCharge) : posWire(aPosWire), posTime(aPosTime), charge(aCharge) { }

  double getPosWire() const { return posWire;}

  double getPosTime() const { return posTime;}

  double getCharge() const {return charge;}
  
private:
  double posWire, posTime;
  double charge;

};

typedef std::vector<Hit2D> Hit2DCollection;

#endif

