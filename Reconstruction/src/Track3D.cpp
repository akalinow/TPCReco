#include "Track3D.h"

#include <iostream>

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Track3D::Track3D(const TVector3 & aT, const TVector3 & aB, int aProj){
  
    myTangent = aT;
    myTangentUnit = aT.Unit();
    
    myBias = aB;

    double lambda = -myBias.X()/myTangent.X();
    myBiasAtX0 = myBias + lambda*myTangent;

    lambda = -myBias.Y()/myTangent.Y();
    myBiasAtY0 = myBias + lambda*myTangent;

    lambda = -myBias.Z()/myTangent.Z();
    myBiasAtZ0 = myBias + lambda*myTangent;

    myProjection = aProj;
   
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void Track3D::setStartEnd(double start, double end){

  startTime = start;
  endTime = end;

  double biasTimeComponent = myBias.X();
  double tangentTimeCompopnent = myTangentUnit.X();

  if(myProjection == DIR_3D){
    biasTimeComponent = myBias.Z();
    tangentTimeCompopnent = myTangentUnit.Z();    
  }

  double lambdaStart = (startTime - biasTimeComponent)/tangentTimeCompopnent;
  myBiasAtStart = myBias + lambdaStart*myTangentUnit;

  double lambdaEnd = (endTime - biasTimeComponent)/tangentTimeCompopnent;

  myLenght = lambdaEnd - lambdaStart;

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TVector3 Track3D::getPointOnTrack2DProjection(double lambda, int iDir) const{

  TVector3 stripPitchDirection(cos(phiPitchDirection[iDir]),
			       sin(phiPitchDirection[iDir]), 0);

  const TVector3 & bias = getBiasAtStart();
  const TVector3 & tangent = getTangentUnit();
  TVector3 aPointOnLine = bias + lambda*tangent;

  double projectionX = aPointOnLine.Z();
  double projectionY = aPointOnLine*stripPitchDirection - stripOffset[iDir];

  return TVector3(projectionX, projectionY, 0.0);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Track3D Track3D::get2DProjection(int iDir) const{

  
  TVector3 startPoint = getPointOnTrack2DProjection(0, iDir);
  TVector3 endPoint = getPointOnTrack2DProjection(getLength(), iDir);

  const TVector3 & aBias = startPoint;
  const TVector3 & aTangent = (endPoint - startPoint).Unit();
  Track3D a2DProjection(aTangent, aBias, iDir);

  double startTime = startPoint.X();
  double endTime = endPoint.X();
  a2DProjection.setStartEnd(startTime, endTime);

  return a2DProjection;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double Track3D::get2DProjectionRecHitChi2(std::shared_ptr<TH2D> hRecHits) const {

  if(getProjection()==DIR_3D) return -999.0;
  
  TVector3 aPoint, d;
  double chi2 = 0.0;
  double maxCharge = 0;
  int pointCount = 0;

  const TVector3 & bias = getBiasAtStart();
  const TVector3 & tangent = getTangentUnit();

  double lambda = 0.0;
  double x = 0.0, y = 0.0;
  double charge = 0.0;
  for(int iBinX=1;iBinX<hRecHits->GetNbinsX();++iBinX){
    for(int iBinY=1;iBinY<hRecHits->GetNbinsY();++iBinY){
      x = hRecHits->GetXaxis()->GetBinCenter(iBinX);
      y = hRecHits->GetYaxis()->GetBinCenter(iBinY);
      charge = hRecHits->GetBinContent(iBinX, iBinY);
      if(charge<10) continue;//FIX ME optimize and move to configuration
      aPoint.SetXYZ(x, y, 0.0);
      lambda = (aPoint - bias)*tangent/tangent.Mag2();      
      d = aPoint - bias - lambda*tangent;
      if(d.Mag()>3) continue;//FIX ME optimize and move to configuration
      chi2 += d.Mag2()*charge;
      ++pointCount;
      if(charge>maxCharge) maxCharge = charge;
    }
  }
  return chi2/pointCount;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

