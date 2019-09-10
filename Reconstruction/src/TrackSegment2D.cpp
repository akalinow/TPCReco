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
double TrackSegment2D::getIntegratedCharge(double lambdaCut, const Hit2DCollection & aRecHits) const{

  const TVector3 & bias = getBias();
  const TVector3 & tangent = getTangent();
  const TVector3 & start = getStart();
  TVector3 transverseComponent;

  double lambda = 0.0;
  double x = 0.0, y = 0.0;
  double totalCharge = 0.0;
  double radiusCut = 2.0;//FIXME put into confoguration
  TVector3 aPoint;
  
  for(const auto aHit:aRecHits){
    x = aHit.getPosTime();
    y = aHit.getPosWire();
    aPoint.SetXYZ(x, y, 0.0);    
    lambda = (aPoint - start)*tangent/tangent.Mag();
    if(lambda<0 || lambda>getLength() || lambda>lambdaCut) continue;

    transverseComponent = aPoint - bias - lambda*tangent;
    if(transverseComponent.Mag()<radiusCut) totalCharge += aHit.getCharge();
  }
  return totalCharge;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment2D::getRecHitChi2(const Hit2DCollection & aRecHits) const {

  TVector3 aPoint;
  TVector3 transverseComponent;
  double chi2 = 0.0;
  double longitudinalChi2Component = 0.0;
  double dummyChi2 = 1E9;
  double maxCharge = 0;
  int pointCount = 0;

  const TVector3 & bias = getBias();
  const TVector3 & tangent = getTangent();
  const TVector3 & start = getStart();

  if(tangent.Mag()<1E-3){
    std::cout<<__FUNCTION__<<" Null tangent: ";
    tangent.Print();
    std::cout<<" for direction: "<<getStripDir()<<std::endl;
    getStart().Print();
    getEnd().Print();
    return dummyChi2;
  }
  
  double lambda = 0.0;
  double x = 0.0, y = 0.0;
  double charge = 0.0;

  for(const auto aHit:aRecHits){
    x = aHit.getPosTime();
    y = aHit.getPosWire();
    charge = aHit.getCharge();
    aPoint.SetXYZ(x, y, 0.0);    
    lambda = (aPoint - start)*tangent/tangent.Mag();        
    
    transverseComponent = aPoint - bias - lambda*tangent;
    longitudinalChi2Component = std::min(lambda, getLength()-lambda);
    if(lambda<0 || lambda>getLength()) continue;
    
    if(longitudinalChi2Component>0) longitudinalChi2Component = 0.0;
    longitudinalChi2Component*=longitudinalChi2Component;
    /*
    std::cout<<"lambda: "<<lambda
	     <<" getLength(): "<<getLength()
	     <<" longitudinalChi2Component: "<<longitudinalChi2Component
	     <<std::endl;
    */

    ++pointCount;
    /*
    std::cout<<"x: "<<x<<" y: "<<y<<" charge: "<<charge
	     <<" lambda: "<<lambda
	     <<" length: "<<getLength()
	     <<" transverse chi2: "<<transverseComponent.Mag2()*charge
	     <<" pointCount: "<<pointCount
	     <<" maxCharge: "<<maxCharge
	     <<std::endl;
    */
    chi2 += transverseComponent.Mag2()*charge;
    //chi2 += longitudinalChi2Component*charge;
    
    //if(charge>maxCharge) maxCharge = charge;
    maxCharge +=charge;
  }
  if(!pointCount) return 0.0*dummyChi2;

  chi2 /= maxCharge;
  //chi2 /= pointCount;
  /*
  std::cout<<" pointCount: "<<pointCount
	   <<" maxCharge: "<<maxCharge
	   <<" chi2: "<<chi2
	   <<std::endl;
  */
  return chi2;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

