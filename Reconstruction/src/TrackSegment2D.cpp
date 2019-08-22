#include "TrackSegment2D.h"

#include <iostream>

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment2D::setBiasTangent(const TVector3 & aBias, const TVector3 & aTangent){

  myBias = aBias;
  myTangent = aTangent;

  double lambda = 100;
  myStart = myBias;
  myEvent = myStart + lambda*myTangent;
  
  initialize();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment2D::setStartEnd(const TVector3 & aStart, const TVector3 & aEnd){

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
    myBiasAtT0 = myBias + lambda*myTangent;

    lambda = -myBias.Y()/myTangent.Y();
    myBiasAtWire0 = myBias + lambda*myTangent;

    myLenght = (myEnd - myStart).Mag();

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment2D::get2DRecHitChi2(const TH2D & hRecHits) const {

  TVector3 aPoint, d;
  double chi2 = 0.0;
  double maxCharge = 0;
  int pointCount = 0;

  const TVector3 & bias = getBias();
  const TVector3 & tangent = getTangent();
  const TH2D & hRecHits = getRecHits();

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

