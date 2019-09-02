#include "TrackSegment3D.h"

#include <iostream>

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::setBiasTangent(const TVector3 & aBias, const TVector3 & aTangent){

  myBias = aBias;
  myTangent = aTangent.Unit();

  double lambda = 10;
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
void TrackSegment3D::setRecHits(const std::vector<TH2D> & aRecHits){

  myRecHits.clear();
  
  double x=-999.0, y=-999.0, charge=-999.0;
  for(int strip_dir=DIR_U;strip_dir<=DIR_W;++strip_dir){
    const TH2D & hRecHits = aRecHits[strip_dir];
    myRecHits.push_back(Hit2DCollection{});
    for(int iBinX=1;iBinX<hRecHits.GetNbinsX();++iBinX){
      for(int iBinY=1;iBinY<hRecHits.GetNbinsY();++iBinY){
	charge = hRecHits.GetBinContent(iBinX, iBinY);
	x = hRecHits.GetXaxis()->GetBinCenter(iBinX);
	y = hRecHits.GetYaxis()->GetBinCenter(iBinY);
	if(charge>0.0) myRecHits.back().push_back(Hit2D(x, y, charge));
      }
    }  
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::initialize(){

  //myTangent = myTangent.Unit();
  //if(myTangent.Theta()>M_PI/2.0) myTangent*=-1;
  //myBias = myBias - myBias.Dot(myTangent)*myTangent;
  
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
  
  TVector3 start = getPointOn2DProjection(0, strip_dir);
  TVector3 end = getPointOn2DProjection(getLength(), strip_dir);

  TrackSegment2D a2DProjection(strip_dir);
  a2DProjection.setStartEnd(start, end);

  return a2DProjection;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::getRecHitChi2() const {

  double chi2 = 0.0;
  //#pragma omp parallel for reduction(+: chi2)
  for(int strip_dir=DIR_U;strip_dir<=DIR_W;++strip_dir){
    //if(strip_dir!=DIR_V) continue;//TEST
    TrackSegment2D aTrack2DProjection = get2DProjection(strip_dir);
    const Hit2DCollection & aRecHits = myRecHits[strip_dir];
    chi2 += aTrack2DProjection.getRecHitChi2(aRecHits);    
  }
  
  return chi2;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::operator() (const double *par) {

  //TVector3 start(par[0], par[1], par[2]);
  TVector3 start = getStart(); 
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
    
  perpPlaneBaseUnitA.SetMagThetaPhi(1.0, M_PI/2.0 + tangentTheta, tangentPhi);
  perpPlaneBaseUnitB.SetMagThetaPhi(1.0, M_PI/2.0, M_PI/2.0 + tangentPhi);
  
  TVector3 aBias = perpPlaneBaseUnitA*biasComponentA + perpPlaneBaseUnitB*biasComponentB;
  TVector3 aTangent;
  aTangent.SetMagThetaPhi(1.0, tangentTheta, tangentPhi);
  setBiasTangent(aBias, aTangent);
  
  return getRecHitChi2();
  */
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
