#include "TrackSegment3D.h"

#include <iostream>

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackSegment3D::TrackSegment3D(){

  myRecHits.clear();
  myRecHits.resize(3);

  myProjectionsChi2.resize(3);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::setBiasTangent(const TVector3 & aBias, const TVector3 & aTangent){

  myBias = aBias;
  myTangent = aTangent.Unit();

  double lambda = 30;//FIXME what value shouldbe here?
  myStart = myBias;
  myEnd = myStart + lambda*myTangent;
  
  initialize();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::setStartEnd(const TVector3 & aStart, const TVector3 & aEnd){

  myStart = aStart;
  myEnd = aEnd;

  myTangent = (myEnd - myStart).Unit();
  myBias = myStart;

  initialize();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::setStartEnd(const double *par){

  TVector3 start(par[0], par[1], par[2]);
  TVector3 end(par[3], par[4], par[5]);  
  setStartEnd(start, end);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::setRecHits(const std::vector<TH2D> & aRecHits){

  myRecHits.clear();
  myRecHits.resize(3);
  
  double x=-999.0, y=-999.0, charge=-999.0;
  for(auto&& strip_dir : proj_vec_UVW){
    const TH2D & hRecHits = aRecHits[int(strip_dir)];
    for(int iBinX=1;iBinX<hRecHits.GetNbinsX();++iBinX){
      for(int iBinY=1;iBinY<hRecHits.GetNbinsY();++iBinY){
	charge = hRecHits.GetBinContent(iBinX, iBinY);
	x = hRecHits.GetXaxis()->GetBinCenter(iBinX);
	y = hRecHits.GetYaxis()->GetBinCenter(iBinY);
	if(charge>0.0) myRecHits.at(int(strip_dir)).push_back(Hit2D(x, y, charge));
      }
    }  
  }

  calculateRecHitChi2();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::initialize(){

  double lambda = -myBias.X()/myTangent.X();
  myBiasAtX0 = myBias + lambda*myTangent;

  lambda = -myBias.Y()/myTangent.Y();
  myBiasAtY0 = myBias + lambda*myTangent;

  lambda = -myBias.Z()/myTangent.Z();
  myBiasAtZ0 = myBias + lambda*myTangent;

  myLenght = (myEnd - myStart).Mag();

  calculateRecHitChi2();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<double> TrackSegment3D::getStartEndXYZ() const{

  std::vector<double> coordinates(6);
  double *data = coordinates.data();

  getStart().GetXYZ(data);
  getEnd().GetXYZ(data+3);

  return coordinates;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TVector3 TrackSegment3D::getPointOn2DProjection(double lambda, projection strip_dir) const{

  TVector3 stripPitchDirection(cos(phiPitchDirection[int(strip_dir)]),
			       sin(phiPitchDirection[int(strip_dir)]), 0);

  const TVector3 & start = getStart();
  const TVector3 & tangent = getTangent();
  TVector3 aPointOnLine = start + lambda*tangent;
  double projectionTime = aPointOnLine.Z();
  double projectionWire = aPointOnLine*stripPitchDirection;

  return TVector3(projectionTime, projectionWire, 0.0);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackSegment2D TrackSegment3D::get2DProjection(projection strip_dir, double lambdaStart, double lambdaEnd) const{
  
  TVector3 start = getPointOn2DProjection(lambdaStart, strip_dir);
  TVector3 end = getPointOn2DProjection(lambdaEnd, strip_dir);

  TrackSegment2D a2DProjection(strip_dir);
  a2DProjection.setStartEnd(start, end);

  return a2DProjection;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::getIntegratedHitDistance(double lambdaCut) const{

  double sum = 0.0;
  for(auto&& strip_dir : proj_vec_UVW){
    TrackSegment2D aTrack2DProjection = get2DProjection(strip_dir, 0, lambdaCut);
    const Hit2DCollection & aRecHits = myRecHits.at(int(strip_dir));
    sum += aTrack2DProjection.getIntegratedHitDistance(lambdaCut, aRecHits);    
  } 
  return sum;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::getIntegratedCharge(double lambdaCut) const{

  double charge = 0.0;
  for (auto&& strip_dir : proj_vec_UVW) {
    TrackSegment2D aTrack2DProjection = get2DProjection(strip_dir, 0, lambdaCut);
    const Hit2DCollection & aRecHits = myRecHits.at(int(strip_dir));
    charge += aTrack2DProjection.getIntegratedCharge(lambdaCut, aRecHits);    
  } 
  return charge;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::getRecHitChi2() const{

  double chi2 = 0.0;
  std::for_each(myProjectionsChi2.begin(), myProjectionsChi2.end(), [&](auto aItem){chi2 += aItem;});
  return chi2;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::calculateRecHitChi2(){

  //#pragma omp parallel for
  for(auto&& strip_dir : proj_vec_UVW){
    TrackSegment2D aTrack2DProjection = get2DProjection(strip_dir, 0, getLength());
    const Hit2DCollection & aRecHits = myRecHits.at(int(strip_dir));
    myProjectionsChi2[int(strip_dir)] = aTrack2DProjection.getRecHitChi2(aRecHits);
  }  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::operator() (const double *par) {

  TVector3 start(par[0], par[1], par[2]);
  TVector3 end(par[3], par[4], par[5]);  
  setStartEnd(start, end);
  return getRecHitChi2();
  ///////////////////////////
  
  /*
  double tangentTheta = par[0];
  double tangentPhi = par[1];

  double biasComponentA = par[2];
  double biasComponentB = par[3];

  TVector3 perpPlaneBaseUnitA, perpPlaneBaseUnitB;
    
  perpPlaneBaseUnitA.SetMagThetaPhi(1.0, pi/2.0 + tangentTheta, tangentPhi);
  perpPlaneBaseUnitB.SetMagThetaPhi(1.0, pi/2.0, pi/2.0 + tangentPhi);
  
  TVector3 aBias = perpPlaneBaseUnitA*biasComponentA + perpPlaneBaseUnitB*biasComponentB;
  TVector3 aTangent;
  aTangent.SetMagThetaPhi(1.0, tangentTheta, tangentPhi);
  setBiasTangent(aBias, aTangent);
  
  return getRecHitChi2();
  */
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::ostream & operator << (std::ostream &out, const TrackSegment3D &aSegment){

  const TVector3 & start = aSegment.getStart();
  const TVector3 & end = aSegment.getEnd();
  
  out<<"("<<start.X()<<", "<<start.Y()<<", "<<start.Z()<<")"
       <<" -> "
     <<"("<<end.X()<<", "<<end.Y()<<", "<<end.Z()<<") "
     <<"["<<aSegment.getRecHitChi2()<<"]";

  return out;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
