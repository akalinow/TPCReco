#ifndef _Track3D_H_
#define _Track3D_H_

#include <string>
#include <vector>
#include <memory>

#include "TVector3.h"
#include "CommonDefinitions.h"

class Track3D{

public:

  Track3D(){};

  Track3D(const TVector3 & aT, const TVector3 & aB, double aLenght, int aProj=DIR_U);

  ~Track3D() {};

  void setTangent(const TVector3 & aT) { myTangent = aT;}

  void setBias(const TVector3 & aB) { myBias = aB;}

  void setProjection(int aProj) { myProjection = aProj;}

  void setLength(double aLenght) { myLenght = aLenght;}

  //Tangential vector with X=1.0
  const TVector3 & getTangent() const { return myTangent;}

  ///Unit tangential vector along track.
  const TVector3 & getTangentUnit() const { return myTangentUnit;}

  ///Bias vector perpendicular to tangent.
  const TVector3 & getBias() const { return myBias;}

  ///Bias vector with X=0.
  const TVector3 & getBiasAtX0() const { return myBiasAtX0;}

  ///Bias vector with Y=0.
  const TVector3 & getBiasAtY0() const { return myBiasAtY0;}

  ///Bias vector with Z=0.
  const TVector3 & getBiasAtZ0() const { return myBiasAtZ0;}

  double getLength() const { return myLenght;}

private:

  int myProjection;

  TVector3 myTangent, myTangentUnit;
  
  TVector3 myBias, myBiasAtX0, myBiasAtY0, myBiasAtZ0;
  
  double myLenght;
  
};


typedef std::vector<Track3D> TrackCollection;

#endif

