#include "TPCReco/TrackSegment2D.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/colorText.h"

#include <iostream>

TrackSegment2D::TrackSegment2D(int strip_dir, std::shared_ptr<GeometryTPC> aGeometryPtr){

  myStripDir = strip_dir;
  myGeometryPtr = aGeometryPtr;
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment2D::setBiasTangent(const TVector3 & aBias, const TVector3 & aTangent){

  myBias = aBias;
  myTangent = aTangent.Unit();

  double lambda = 100;
  myStart = myBias-lambda*myTangent;
  myEnd = myBias+lambda*myTangent;
  initialize();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment2D::setStartEnd(const TVector3 & aStart, const TVector3 & aEnd){

  myStart = aStart;
  myEnd = aEnd;

  myTangent = (myEnd - myStart).Unit();
  myBias = (myStart + myEnd)*0.5;
  initialize();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment2D::initialize(){

  myTangent = myTangent.Unit();
  myLenght = (myEnd - myStart).Mag();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TVector3 TrackSegment2D::getMinBias() const{

  double lambda = myBias.Dot(myTangent);
  return myBias - lambda*myTangent;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TVector3 TrackSegment2D::getBiasAtT(double time) const{

if(std::abs(myTangent.X())<1E-3)
  {
    return getMinBias();
  }
 else{
   double lambda = (time-myBias.X())/myTangent.X();
   return myBias + lambda*myTangent;
 }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TGraphErrors TrackSegment2D::getChargeProfile(const Hit2DCollection & aRecHits, double radiusCut){

  if(getLength()<1) return TGraphErrors(0);

  double x = 0.0, y = 0.0;
  double ex = 0.0;
  double distance = 0.0;
  double lambda = 0.0;
  TVector3 aPoint;

  TVector3 cellDiagonal(myGeometryPtr->GetTimeBinWidth(), myGeometryPtr->GetStripPitch(), 0);
  TVector3 cellDiagonal1(myGeometryPtr->GetTimeBinWidth(), -myGeometryPtr->GetStripPitch(), 0);
  double cellProjectionOnSegment2D = std::max(cellDiagonal.Dot(getTangent().Unit()),
					      cellDiagonal1.Dot(getTangent().Unit()));  
  double binWidth = std::abs(cellProjectionOnSegment2D)/getLength();
  TGraphErrors grChargeProfile2D(0);

  for(const auto aHit:aRecHits){
    x = aHit.getPosTime();
    y = aHit.getPosStrip();
    aPoint.SetXYZ(x, y, 0.0);
    std::tie(lambda, distance) = getPointLambdaAndDistance(aPoint);
    lambda /= getLength();
    if(distance<radiusCut){
      grChargeProfile2D.SetPoint(grChargeProfile2D.GetN(),lambda, aHit.getCharge()/binWidth);
      grChargeProfile2D.SetPointError(grChargeProfile2D.GetN()-1,binWidth/2.0,0.0);
    }
  }
  grChargeProfile2D.Sort();
  
  grChargeProfile2D.GetPoint(grChargeProfile2D.GetN()-1,x,y);
  ex = grChargeProfile2D.GetErrorX(grChargeProfile2D.GetN()-1);
  grChargeProfile2D.SetPoint(grChargeProfile2D.GetN(),x+ex,0.0);

  grChargeProfile2D.GetPoint(0,x,y);
  ex = grChargeProfile2D.GetErrorX(0);
  grChargeProfile2D.SetPoint(grChargeProfile2D.GetN(),x-ex,0.0);
  grChargeProfile2D.Sort();
  return grChargeProfile2D;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment2D::getIntegratedCharge(double lambdaCut, const Hit2DCollection & aRecHits) const{

  double x = 0.0, y = 0.0;
  double totalCharge = 0.0;
  double radiusCut = 4.0;//FIXME put into configuration. Value 4.0 got by looking at the plots.
  double distance = 0.0;
  double lambda = 0.0;
  TVector3 aPoint;
  
  for(const auto aHit:aRecHits){
    x = aHit.getPosTime();
    y = aHit.getPosStrip();
    aPoint.SetXYZ(x, y, 0.0);    
    std::tie(lambda, distance) = getPointLambdaAndDistance(aPoint);
    if(lambda>0 && lambda<getLength() && distance>0 && distance<radiusCut) totalCharge += aHit.getCharge();
  }
  return totalCharge;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::tuple<double,double> TrackSegment2D::getPointLambdaAndDistance(const TVector3 & aPoint) const{

  const TVector3 & tangent = getTangent();
  const TVector3 & start = getStart();

  TVector3 delta = aPoint - start;
  double lambda = delta*tangent.Unit();
  TVector3 transverseComponent = delta - lambda*tangent;
  return std::make_tuple(lambda, transverseComponent.Mag());
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment2D::getRecHitChi2(const Hit2DCollection & aRecHits) const {

  if(!aRecHits.size()) return 0.0;
  double dummyChi2 = 999.0;

  if(getTangent().Mag()<1E-3){
    std::cout<<__FUNCTION__<<KRED<< " TrackSegment2D has null tangent "<<RST
	     <<" for direction: "<<getStripDir()<<std::endl;
    std::cout<<KGRN<<"Start point: "<<RST;
    getStart().Print();
    std::cout<<KGRN<<"End point: "<<RST;
    getEnd().Print();
    return dummyChi2;
  }

  TVector3 aPoint;
  double chi2 = 0.0;
  double chargeSum = 0.0;
  double totalChargeSum = 0.0;
  double distance = 0.0;
  double lambda = 0.0;
  int pointCount = 0;
  
  double x = 0.0, y = 0.0;
  double charge = 0.0;
  double maxDistance = 10.0;

  for(const auto aHit:aRecHits){
    x = aHit.getPosTime();
    y = aHit.getPosStrip();
    charge = aHit.getCharge();
    aPoint.SetXYZ(x, y, 0.0);
    std::tie(lambda,distance) = getPointLambdaAndDistance(aPoint);
    totalChargeSum += charge;
    if(distance>maxDistance) continue;
    ++pointCount;
    chi2 += std::pow(distance, 2)*charge;
    chargeSum +=charge;
  }
  if(!pointCount) return dummyChi2;

  chi2 /= chargeSum;
  chargeSum /= totalChargeSum;

  if(!pointCount) return 0.0;
  return chi2;// - 3*chargeSum;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::ostream & operator << (std::ostream &out, const TrackSegment2D &aSegment){

  const TVector3 & start = aSegment.getStart();
  const TVector3 & end = aSegment.getEnd();
  
  out<<"direction: "<<aSegment.getStripDir()
     <<" ("<<start.X()<<", "<<start.Y()<<")"
     <<" -> "
     <<"("<<end.X()<<", "<<end.Y()<<") "<<std::endl
     <<"\t N Hough accumulator hits: "
     <<"["<<aSegment.getNAccumulatorHits()<<"]";
  return out;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
