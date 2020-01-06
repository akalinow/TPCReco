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
  double radiusCut = 2.0;//FIXME put into configuration
  
#ifndef _cpp17_
  std::vector<double> charges;
  charges.resize(aRecHits.size());
  std::transform(aRecHits.begin(), aRecHits.end(), charges.begin(), [&](const auto& aHit) {
      TVector3 aPoint;
      auto x = aHit().posTime;
      auto y = aHit().posWire;
      aPoint.SetXYZ(x, y, 0.0);
      auto distance = getPointTransverseDistance(aPoint);
      return (distance < radiusCut && distance > 0 ? aHit().charge : 0.0);
  });
  return std::accumulate(charges.begin(), charges.end(), 0.0);
#else
  return std::transform_reduce( std::execution::par, aRecHits.begin(), aRecHits.end(), 0.0, std::plus<>(), [&](const auto& aHit) {
      TVector3 aPoint;
      auto x = aHit().posTime;
      auto y = aHit().posWire;
      aPoint.SetXYZ(x, y, 0.0);
      auto distance = getPointTransverseDistance(aPoint);
      return (distance < radiusCut && distance > 0 ? aHit().charge : 0.0);
});
#endif // !_cpp17_
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment2D::getIntegratedHitDistance(double lambdaCut, const Hit2DCollection & aRecHits) const{

  //FIXME divide by total segment charge, not total charge up to lambda
#ifndef _cpp17_
  std::vector<double> distances;
  distances.resize(aRecHits.size());
  std::transform(aRecHits.begin(), aRecHits.end(), distances.begin(), [&](const auto& aHit) {
      TVector3 aPoint;
    auto x = aHit().posTime;
    auto y = aHit().posWire;
    aPoint.SetXYZ(x, y, 0.0);    
    auto distance = getPointTransverseDistance(aPoint);
    return (distance > 0 ? distance*aHit().charge : 0.0);
  });
  return std::accumulate(distances.begin(), distances.end(), 0.0);
#else
    return std::transform_reduce(aRecHits.begin(), aRecHits.end(), 0.0, std::plus<>(), [&](const auto& aHit) {
        TVector3 aPoint;
        auto x = aHit().posTime;
        auto y = aHit().posWire;
        aPoint.SetXYZ(x, y, 0.0);
        auto distance = getPointTransverseDistance(aPoint);
        return (distance > 0 ? distance * aHit().charge : 0.0);
    });
#endif
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
struct sum_elems { //temporary solution
    double
        chi2 = 0.0,
        charge = 0.0;
    int
        pointCount = 0;
};

auto operator+(sum_elems& elems1, sum_elems& elems2) {
    return sum_elems{
        elems1.chi2 + elems2.chi2,
        elems1.charge + elems2.charge,
        elems1.pointCount + elems2.pointCount
    };
}

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

  double chi2 = 0.0;
  double chargeSum = 0.0;
  int pointCount = 0;

#ifndef _cpp17_
  std::vector<sum_elems> elems_struct_vec;
  elems_struct_vec.resize(aRecHits.size());
  std::transform(aRecHits.begin(), aRecHits.end(), elems_struct_vec.begin(), [&](const auto& aHit) {
      TVector3 aPoint;
      sum_elems elems;
      auto x = aHit().posTime;
      auto y = aHit().posWire;
      aPoint.SetXYZ(x, y, 0.0);
      auto distance = getPointTransverseDistance(aPoint);
      if (distance < 0) return elems;
      elems.charge = aHit().charge;
      elems.pointCount = 1;
      elems.chi2 = std::pow(distance, 2) * elems.charge;
      return elems;
  });
  auto sums = std::accumulate(elems_struct_vec.begin(), elems_struct_vec.end(), sum_elems());
#else
  auto sums = std::transform_reduce(std::execution::par, aRecHits.begin(), aRecHits.end(), sum_elems(), std::plus<>(), [&](const auto& aHit) {
      TVector3 aPoint;
      sum_elems elems;
      auto x = aHit().posTime;
      auto y = aHit().posWire;
      aPoint.SetXYZ(x, y, 0.0);
      auto distance = getPointTransverseDistance(aPoint);
      if (distance < 0) return elems;
      elems.charge = aHit().charge;
      elems.pointCount = 1;
      elems.chi2 = std::pow(distance, 2) * elems.charge;
      return elems;
  });
#endif // !_cpp17_
  pointCount = sums.pointCount;
  chargeSum = sums.charge;
  if(pointCount == 0) return dummyChi2;
  chi2 = sums.chi2 / chargeSum;
  return chi2;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

