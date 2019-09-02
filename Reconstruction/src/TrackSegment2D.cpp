#include "TrackSegment2D.h"

#include <iostream>

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment2D::setBiasTangent(const TVector3 & aBias, const TVector3 & aTangent){

  myBias = aBias;
  myTangent = aTangent.Unit();

  double lambda = 100;
  myStart = myBias;
  myEnd = myStart + lambda*myTangent;
  
  initialize();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment2D::setStartEnd(const TVector3 & aStart, const TVector3 & aEnd){

  myStart = aStart;
  myEnd = aEnd;

  myTangent = (myEnd - myStart).Unit();
  myBias = myStart;

  initialize();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment2D::initialize(){

  //myTangent = myTangent.Unit();
  //if(myTangent.Theta()>M_PI/2.0) myTangent*=-1;
  //myBias = myBias - myBias.Dot(myTangent)*myTangent;
  
  double lambda = -myBias.X()/myTangent.X();
  myBiasAtT0 = myBias + lambda*myTangent;
  
  lambda = -myBias.Y()/myTangent.Y();
  myBiasAtWire0 = myBias + lambda*myTangent;

  ///Set tangent direction along time arrow with unit time component.
  ///So vector components can be compared between projections.
  myTangentWithT1 = myTangent;
  if(myTangentWithT1.X()<0) myTangentWithT1 *= -1;
  if(std::abs(myTangentWithT1.X())>1E-5){
    myTangentWithT1 *= 1.0/myTangentWithT1.X();
  }
  else myTangentWithT1 *=0.0;
  
  myLenght = (myEnd - myStart).Mag();  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment2D::getRecHitChi2(const Hit2DCollection & aRecHits) const {

  TVector3 aPoint;
  TVector3 transverseComponent;
  double chi2 = 0.0;
  double longitudinalChi2 = 0.0;
  double maxCharge = 0;
  int pointCount = 0;

  const TVector3 & bias = getBias();
  const TVector3 & tangent = getTangent();

  const TVector3 & start = getStart();

  double lambda = 0.0;
  double x = 0.0, y = 0.0;
  double charge = 0.0;

  for(const auto aHit:aRecHits){
    x = aHit.getPosTime();
    y = aHit.getPosUVW();
    charge = aHit.getCharge();
    charge = 1.0;//TEST
    ++pointCount;
      
    aPoint.SetXYZ(x, y, 0.0);
    
    lambda = (aPoint - start)*tangent/tangent.Mag();      
    transverseComponent = aPoint - bias - lambda*tangent;
      
    if(lambda>0 && lambda<getLength()) longitudinalChi2 = 0.0;
    else longitudinalChi2 = std::pow(lambda, 2);
    /*
    std::cout<<"x: "<<x<<" y: "<<y<<" charge: "<<charge
	     <<" lambda: "<<lambda
	     <<" length: "<<getLength()
	     <<" transverse chi2: "<<transverseComponent.Mag2()*charge
	     <<" longitudinal chi2: "<<longitudinalChi2
	     <<std::endl;
    */
    longitudinalChi2 = 0.0;
    if(std::abs(lambda)>getLength()) continue;//TEST
    chi2 += longitudinalChi2*charge;      
    chi2 += transverseComponent.Mag2()*charge;
    ++pointCount;
    if(charge>maxCharge) maxCharge = charge;   
  }
  if(!pointCount) return 1E9;
  return chi2/pointCount;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

