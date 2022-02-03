#include "TrackSegment3D.h"
#include "GeometryTPC.h"
#include "colorText.h"

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
void TrackSegment3D::setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr){
  
  myGeometryPtr = aGeometryPtr;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<GeometryTPC> TrackSegment3D::getGeometry() const{
  return myGeometryPtr;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::setBiasTangent(const TVector3 & aBias, const TVector3 & aTangent){

  myBias = aBias;
  myTangent = aTangent.Unit();

  double lambda = 1500;
  myStart = myBias-lambda*myTangent;   
  myEnd = myBias+lambda*myTangent;  
  initialize();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::setStartEnd(const TVector3 & aStart, const TVector3 & aEnd){

  myStart = aStart;
  myEnd = aEnd;

  myTangent = (myEnd - myStart).Unit();
  myBias = myStart + 0.5*(myEnd - myStart);
  myBias = myBias - myTangent.Dot(myBias)*myTangent;//TEST
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
void TrackSegment3D::setBiasTangent(const double *par){
  /*
  TVector3 delta_bias = getTangent().Orthogonal().Unit();
  double length = par[0];
  //double phi = par[1];
  //delta_bias.Rotate(phi, par[1]*getTangent().Unit());
  delta_bias *=length;
  */
  TVector3 bias = getBias();// + delta_bias;
  TVector3 tangent;
  tangent.SetMagThetaPhi(1.0, par[2], par[3]);  
  setBiasTangent(bias, tangent);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::setRecHits(const std::vector<TH2D> & aRecHits){

  myRecHits.clear();
  myRecHits.resize(3);
  
  double x=-999.0, y=-999.0, charge=-999.0;
  for(int strip_dir=DIR_U;strip_dir<=DIR_W;++strip_dir){
    const TH2D & hRecHits = aRecHits[strip_dir];
    for(int iBinX=1;iBinX<hRecHits.GetNbinsX();++iBinX){
      for(int iBinY=1;iBinY<hRecHits.GetNbinsY();++iBinY){
	charge = hRecHits.GetBinContent(iBinX, iBinY);
	x = hRecHits.GetXaxis()->GetBinCenter(iBinX);
	y = hRecHits.GetYaxis()->GetBinCenter(iBinY);
	if(charge>0.0) myRecHits.at(strip_dir).push_back(Hit2D(x, y, charge));
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
std::vector<double> TrackSegment3D::getBiasTangentCoords() const{

  std::vector<double> coordinates(4);
  double *data = coordinates.data();

  data[0] = 0.0;
  data[1] = 0.0;
  data[2] = getTangent().Theta();
  data[3] = getTangent().Phi();

  return coordinates;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::getLambdaAtZ(double z) const {
  return (z - myStart.Z())/myTangent.Z();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TVector3 TrackSegment3D::getPointOn2DProjection(double lambda,
						const TVector3 & stripPitchDirection) const{

  const TVector3 & start = getStart();
  const TVector3 & tangent = getTangent();
  TVector3 aPointOnLine = start + lambda*tangent;
  double projectionTime = aPointOnLine.Z();
  double projectionStrip = aPointOnLine*stripPitchDirection;

  return TVector3(projectionTime, projectionStrip, 0.0);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackSegment2D TrackSegment3D::get2DProjection(int strip_dir, double lambdaStart, double lambdaEnd)const{

  TrackSegment2D a2DProjection(strip_dir);
  if(!myGeometryPtr){
    std::cout<<__FUNCTION__<<KRED<<" No valid geometry pointer!"<<std::endl;
    return a2DProjection;
  }
  const TVector3 & stripPitchDirection = myGeometryPtr->GetStripPitchVector3D(strip_dir);
  TVector3 start = getPointOn2DProjection(lambdaStart, stripPitchDirection);
  TVector3 end = getPointOn2DProjection(lambdaEnd, stripPitchDirection);
  a2DProjection.setStartEnd(start, end);

  return a2DProjection;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TGraph TrackSegment3D::getChargeProfile() const{

  TGraph chargeProfile;
  Double_t x = 0, y = 0;
  double radiusCut = 2.0;
  double timeProjection = getTangent().Unit().Z();

  chargeProfile.SetPoint(chargeProfile.GetN(),0.0, 0.0);
  for(int strip_dir=DIR_U;strip_dir<=DIR_W;++strip_dir){
    TrackSegment2D aTrack2DProjection = get2DProjection(strip_dir, 0, getLength());
    const TVector3 & stripPitchDirection = myGeometryPtr->GetStripPitchVector3D(strip_dir);
    const Hit2DCollection & aRecHits = myRecHits.at(strip_dir);
    const TGraph & chargeProfileProjection = aTrack2DProjection.getChargeProfile(aRecHits, radiusCut);
    double dirProjection = std::abs(stripPitchDirection.Unit().Dot(getTangent().Unit()));
    double cosPhiProjectionAngle = sqrt(dirProjection*dirProjection +
					timeProjection*timeProjection);
    std::cout<<KBLU<<"cosPhiProjectionAngle: "<<RST
	     <<" strip_dir: "<<strip_dir<<" "
	     <<cosPhiProjectionAngle
      	     <<" 3D segment length: "<<getLength()
	     <<" 2D segment length: "<<aTrack2DProjection.getLength()
	     <<std::endl;
    for (Int_t i = 0 ; i < chargeProfileProjection.GetN(); i++) {
      chargeProfileProjection.GetPoint(i, x, y);
      chargeProfile.SetPoint(chargeProfile.GetN(), x/cosPhiProjectionAngle, y);
    }
    break;
  }
  chargeProfile.SetPoint(chargeProfile.GetN(), getLength(), 0.0);
  chargeProfile.Sort();
  return chargeProfile;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::getIntegratedCharge(double lambdaCut) const{

  if(lambdaCut<0) return 0;

  double charge = 0.0;
  for(int strip_dir=DIR_U;strip_dir<=DIR_W;++strip_dir){
    TrackSegment2D aTrack2DProjection = get2DProjection(strip_dir, 0, lambdaCut);
    const Hit2DCollection & aRecHits = myRecHits.at(strip_dir);
    charge += aTrack2DProjection.getIntegratedCharge(lambdaCut, aRecHits);    
  } 
  return charge;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::getRecHitChi2(int iProjection) const{

  double chi2 = 0.0;
  if(iProjection<DIR_U || iProjection>DIR_W){   
    std::for_each(myProjectionsChi2.begin(), myProjectionsChi2.end(), [&](auto aItem){chi2 += aItem;});
  }
  else{
    chi2 += myProjectionsChi2[iProjection];
  }
  return chi2;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::calculateRecHitChi2(){

  for(int strip_dir=DIR_U;strip_dir<=DIR_W;++strip_dir){
    TrackSegment2D aTrack2DProjection = get2DProjection(strip_dir, 0, getLength());
    const Hit2DCollection & aRecHits = myRecHits.at(strip_dir);
    myProjectionsChi2[strip_dir] = aTrack2DProjection.getRecHitChi2(aRecHits);
  }  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::operator() (const double *par) {

  TVector3 start(par[0], par[1], par[2]);
  TVector3 end(par[3], par[4], par[5]);  
  setStartEnd(start, end);
  return getRecHitChi2();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::ostream & operator << (std::ostream &out, const TrackSegment3D &aSegment){

  const TVector3 & start = aSegment.getStart();
  const TVector3 & end = aSegment.getEnd();

  const TVector3 & bias = aSegment.getBias();
  const TVector3 & tangent = aSegment.getTangent();
  
  out<<"("<<start.X()<<", "<<start.Y()<<", "<<start.Z()<<")"
       <<" -> "
     <<"("<<end.X()<<", "<<end.Y()<<", "<<end.Z()<<") "
     <<std::endl
     <<"\t\t chi2: "<<aSegment.getRecHitChi2()<<""
     <<" charge [arb. u.]: "<<aSegment.getIntegratedCharge(aSegment.getLength())
     <<" length [mm]: "<<aSegment.getLength()
     <<std::endl
     <<"\t bias (X,Y,Z): "
     <<"("<<bias.X()<<", "<<bias.Y()<<", "<<bias.Z()<<")"
     <<"\t tangent (R, Theta, Phi): "
     <<"("<<tangent.Mag()<<", "<<tangent.Theta()<<", "<<tangent.Phi()<<")";
    
  return out;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
