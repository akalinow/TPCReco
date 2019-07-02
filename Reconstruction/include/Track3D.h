#ifndef _Track3D_H_
#define _Track3D_H_

#include <string>
#include <vector>
#include <memory>

#include "TVector3.h"

class Track3D{

public:

  Track3D(){};

  Track3D(const TVector3 & aT, const TVector3 & aB, double aLenght){
    myTangent = aT;
    myBias = aB;
    myLenght = aLenght;
  }

  ~Track3D() {};

  void setTangent(const TVector3 & aT) { myTangent = aT;}

  void setBias(const TVector3 & aB) { myBias = aB;}

  void setLength(double aLenght) { myLenght = aLenght;}

  const TVector3 & getTangent() const { return myTangent;}

  const TVector3 & getBias() const { return myBias;}

  double getLength() const { return myLenght;}

private:

  TVector3 myTangent, myBias;
  double myLenght;
  


};
#endif

