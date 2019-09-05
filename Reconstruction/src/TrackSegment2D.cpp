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
  double dummyChi2 = 1E9;
  double longitudinalChi2 = 0.0;
  double maxCharge = 0;
  int pointCount = 0;

  const TVector3 & bias = getBias();
  const TVector3 & tangent = getTangent();
  const TVector3 & start = getStart();

  if(tangent.Mag()<1E-3){
    tangent.Print();
    return dummyChi2;
  }
  
  double lambda = 0.0;
  double x = 0.0, y = 0.0;
  double charge = 0.0;

  double minLambda = 999.0;
  double maxLambda = -999.0;

  for(const auto aHit:aRecHits){
    x = aHit.getPosTime();
    y = aHit.getPosWire();
    aPoint.SetXYZ(x, y, 0.0);    
    lambda = (aPoint - start)*tangent/tangent.Mag();
    /*
    std::cout<<"x: "<<x<<" y: "<<y<<" charge: "<<charge
	     <<" lambda: "<<lambda
	     <<" length: "<<getLength()
	     <<" transverse chi2: "<<transverseComponent.Mag2()*charge
	     <<" longitudinal chi2: "<<longitudinalChi2
	     <<" pointCount: "<<pointCount
	     <<std::endl;
    */    
    if(lambda<0 || lambda>getLength()) continue;
    transverseComponent = aPoint - bias - lambda*tangent;
      
    if(lambda>0 && lambda<getLength()) longitudinalChi2 = 0.0;
    else longitudinalChi2 = std::pow(lambda, 2);   
    longitudinalChi2 = 0.0;
    
    charge = aHit.getCharge();
    //charge = 1.0;//TEST
    ++pointCount;      
    
    chi2 += longitudinalChi2*charge;      
    chi2 += transverseComponent.Mag2()*charge;
    
    if(charge>maxCharge) maxCharge = charge;
    if(lambda<minLambda) minLambda = lambda;
    if(lambda>maxLambda) maxLambda = lambda;
  }
  if(!pointCount){
    //std::cout<<__FUNCTION__<<" pointCount: "<<pointCount<<std::endl;
    return -1E-10;
  }

  chi2 /= maxCharge;
  chi2 /= pointCount;
  /*
  std::cout<<" minLambda/length: "<<minLambda/getLength()
	   <<" maxLambda/length: "<<maxLambda/getLength()
	   <<" length: "<<getLength()
	   <<" pointCount: "<<pointCount
	   <<" maxCharge: "<<maxCharge
	   <<" chi2: "<<chi2
	   <<std::endl;
  */
  return chi2;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

