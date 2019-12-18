#include "TrackSegment2D.h"

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

  double x = 0.0, y = 0.0;
  double totalCharge = 0.0;
  double radiusCut = 2.0;//FIXME put into configuration
  double distance = 0.0;
  TVector3 aPoint;
  
  for(const auto aHit:aRecHits){
    x = aHit().posTime;
    y = aHit().posWire;
    aPoint.SetXYZ(x, y, 0.0);    
    distance = getPointTransverseDistance(aPoint);
    if(distance>0 && distance<radiusCut) totalCharge += aHit().charge;
  }
  return totalCharge;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment2D::getIntegratedHitDistance(double lambdaCut, const Hit2DCollection & aRecHits) const{

  double x = 0.0, y = 0.0;
  double totalCharge = 0.0;
  double distance = 0.0;
  double sum = 0.0;
  TVector3 aPoint;
  
  for(const auto aHit:aRecHits){
    x = aHit().posTime;
    y = aHit().posWire;
    aPoint.SetXYZ(x, y, 0.0);    
    distance = getPointTransverseDistance(aPoint);
    if(distance>0){
      sum += distance*aHit().charge;
      totalCharge += aHit().charge;
    }
  }
  //FIXME divide by total segment charge, not total charge up to lambda
  return sum;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment2D::getPointTransverseDistance(const TVector3 & aPoint) const{

  const TVector3 & bias = getBias();
  const TVector3 & tangent = getTangent();
  const TVector3 & start = getStart();

  double lambda = (aPoint - start)*tangent/tangent.Mag();
  if(lambda<0 || lambda>getLength()) return -999;
  TVector3 transverseComponent = aPoint - bias - lambda*tangent;
  return transverseComponent.Mag();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment2D::getRecHitChi2(const Hit2DCollection & aRecHits) const {

  double dummyChi2 = 1E9;

  if(getTangent().Mag()<1E-3){
    std::cout<<__FUNCTION__<<" Null tangent: ";
    getTangent().Print();
    std::cout<<" for direction: "<<getStripDir()<<std::endl;
    getStart().Print();
    getEnd().Print();
    return dummyChi2;
  }

  TVector3 aPoint;
  double chi2 = 0.0;
  double chargeSum = 0.0;
  double distance = 0.0;
  int pointCount = 0;
  
  double x = 0.0, y = 0.0;
  double charge = 0.0;

  for(const auto aHit:aRecHits){
    x = aHit().posTime;
    y = aHit().posWire;
    charge = aHit().charge;
    aPoint.SetXYZ(x, y, 0.0);
    distance = getPointTransverseDistance(aPoint);
    if(distance<0) continue;    
    ++pointCount;    
    chi2 += std::pow(distance, 2)*charge;
    chargeSum +=charge;
  }
  if(!pointCount) return dummyChi2;

  chi2 /= chargeSum;
  return chi2;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

