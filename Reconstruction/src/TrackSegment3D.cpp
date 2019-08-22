#include "TrackSegment3D.h"

#include <iostream>

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::setBiasTangent(const TVector3 & aBias, const TVector3 & aTangent){

  myBias = aBias;
  myTangent = aTangent;

  double lambda = 100;
  myStart = myBias;
  myEvent = myStart + lambda*myTangent;
  
  initialize();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::setStartEnd(const TVector3 & aStart, const TVector3 & aEnd){

  myStart = aStart;
  myEvent = aEnd;

  myTangent = (myEnd - myStart).Unit();
  myBias = myStart;

  initialize();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void initialize(){
  
    double lambda = -myBias.X()/myTangent.X();
    myBiasAtX0 = myBias + lambda*myTangent;

    lambda = -myBias.Y()/myTangent.Y();
    myBiasAtY0 = myBias + lambda*myTangent;

    lambda = -myBias.Z()/myTangent.Z();
    myBiasAtZ0 = myBias + lambda*myTangent;

    myLenght = (myEnd - myStart).Mag();

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TVector3 TrackSegment3D::getPointOn2DProjection(double lambda, int strip_dir) const{

  TVector3 stripPitchDirection(cos(phiPitchDirection[strip_dir]),
			       sin(phiPitchDirection[strip_dir]), 0);

  const TVector3 & bias = getBias();
  const TVector3 & tangent = getTangent();
  TVector3 aPointOnLine = bias + lambda*tangent;

  double projectionTime = aPointOnLine.Z();
  double projectionWire = aPointOnLine*stripPitchDirection;

  return TVector3(projectionTime, projectionWire, 0.0);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackSegment2D TrackSegment3D::get2DProjection(int strip_dir) const{
  
  TVector3 startPoint = getPointOn2DProjection(0, iDir);
  TVector3 endPoint = getPointOn2DProjection(100, iDir);

  const TVector3 & aBias = startPoint;
  const TVector3 & aTangent = (endPoint - startPoint).Unit();

  TrackSegment2D a2DProjection(strip_dir);
  a2DProjection.setBiasTangent(aBias, aTangent);

  return a2DProjection;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::get2DProjectionRecHitChi2(const TH2D & hRecHits) const {

  if(getProjection()==DIR_3D) return -999.0;
  
  TVector3 aPoint, d;
  double chi2 = 0.0;
  double maxCharge = 0;
  int pointCount = 0;

  const TVector3 & bias = getBias();
  const TVector3 & tangent = getTangent();

  double lambda = 0.0;
  double x = 0.0, y = 0.0;
  double charge = 0.0;
  for(int iBinX=1;iBinX<hRecHits.GetNbinsX();++iBinX){
    for(int iBinY=1;iBinY<hRecHits.GetNbinsY();++iBinY){
      x = hRecHits.GetXaxis()->GetBinCenter(iBinX);
      y = hRecHits.GetYaxis()->GetBinCenter(iBinY);
      charge = hRecHits.GetBinContent(iBinX, iBinY);
      aPoint.SetXYZ(x, y, 0.0);
      lambda = (aPoint - bias)*tangent/tangent.Mag2();      
      d = aPoint - bias - lambda*tangent;
      chi2 += d.Mag2()*charge;
      ++pointCount;
      if(charge>maxCharge) maxCharge = charge;
    }
  }
  return chi2/pointCount;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::getRecHitChi2() const {

  double chi2 = 0.0;
  double projectionChi2 = 0.0;
  for(int strip_dir=DIR_U;strip_dir<=DIR_W;++strip_dir){
    const TrackSegment3D & aTrack2DProjection = get2DProjection(strip_dir);
    const TH2D & aRecHits = myRecHits[strip_dir];
    projectionChi2 = aTrack2DProjection.get2DProjectionRecHitChi2(aRecHits);    
    chi2 += projectionChi2;
  }
  return chi2;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::operator() (const double *par) {

  double tangentTheta = par[0];
  double tangentPhi = par[1];

  double biasComponentA = par[2];
  double biasComponentB = par[3];

  TVector3 perpPlaneBaseUnitA, perpPlaneBaseUnitB;
    
  perpPlaneBaseUnitA.SetMagThetaPhi(1.0, M_PI/2.0 + tangentTheta, tangentPhi);
  perpPlaneBaseUnitB.SetMagThetaPhi(1.0, M_PI/2.0, M_PI/2.0 + tangentPhi);
  
  myBiasAtStart = perpPlaneBaseUnitA*biasComponentA + perpPlaneBaseUnitB*biasComponentB;

  //FIX ME notiation mess.
  myTangentUnit.SetMagThetaPhi(1.0, tangentTheta, tangentPhi);

  return getRecHitChi2();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
