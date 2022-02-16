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

  double lambda = 100;
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
  myBias = (myStart + myEnd)*0.5;
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
  
  TVector3 bias(par[0], par[1], par[2]);
  TVector3 tangent;
  tangent.SetMagThetaPhi(1.0, par[3], par[4]);  

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
    for(int iBinX=1;iBinX<=hRecHits.GetNbinsX();++iBinX){
      for(int iBinY=1;iBinY<=hRecHits.GetNbinsY();++iBinY){
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

  std::vector<double> coordinates(5);
  double *data = coordinates.data();

  getBias().GetXYZ(data);
  data[3] = getTangent().Theta();
  data[4] = getTangent().Phi();

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

  TrackSegment2D a2DProjection(strip_dir, myGeometryPtr);
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
double TrackSegment3D::getIntegratedCharge(double lambda) const{
  if(lambda<0) return 0;
  double charge = 0.0;
  for(int strip_dir=DIR_U;strip_dir<=DIR_W;++strip_dir){
    TrackSegment2D aTrack2DProjection = get2DProjection(strip_dir, 0, lambda);
    const Hit2DCollection & aRecHits = myRecHits.at(strip_dir);
    charge += aTrack2DProjection.getIntegratedCharge(lambda, aRecHits);
  } 
  return charge;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TH2F TrackSegment3D::getChargeProfile() const{

  double radiusCut = 2.0;
  double timeProjection = getTangent().Unit().Z();
  
  std::vector<TH1F> projections;
  std::vector<double> cosPhiProjectionAngles;
  double binWidth = 999.0;
  double tmpWidth = 999.0;
  
  for(int strip_dir=DIR_U;strip_dir<=DIR_W;++strip_dir){
    TrackSegment2D aTrack2DProjection = get2DProjection(strip_dir, 0, getLength());
    const TVector3 & stripPitchDirection = myGeometryPtr->GetStripPitchVector3D(strip_dir);
    double dirProjection = std::abs(stripPitchDirection.Unit().Dot(getTangent().Unit()));
    double cosPhiProjectionAngle = sqrt(dirProjection*dirProjection +
					timeProjection*timeProjection);    
    const Hit2DCollection & aRecHits = myRecHits.at(strip_dir);
    projections.push_back(aTrack2DProjection.getChargeProfile(aRecHits, radiusCut));
    cosPhiProjectionAngles.push_back(cosPhiProjectionAngle);
    tmpWidth = projections.back().GetXaxis()->GetBinWidth(1)/cosPhiProjectionAngles.back();
    if(tmpWidth<binWidth) binWidth = tmpWidth;
  }
  int nBins = 2.0/binWidth;
  if(nBins>2000) nBins = 2000;//FIXME
  TH2F hChargeProfile("hChargeProfile",";d [mm];charge/mm",nBins, 0, getLength(), 3, -0.5, 2.5);

  double xLow, xCenter, xHigh;
  int binLow, binCenter, binHigh;
  double charge = 0.0;
  for(int strip_dir=DIR_U;strip_dir<=DIR_W;++strip_dir){
      TH1F & aHisto = projections[strip_dir];
      //aHisto.Print();
      for(int iBin=1;iBin<=aHisto.GetNbinsX();++iBin){
	charge = aHisto.GetBinContent(iBin);
	if(charge<1E-3) continue;
	xLow = aHisto.GetXaxis()->GetBinLowEdge(iBin)*getLength();
	xCenter = aHisto.GetXaxis()->GetBinCenter(iBin)*getLength();
	xHigh = aHisto.GetXaxis()->GetBinUpEdge(iBin)*getLength();
	binLow = hChargeProfile.GetXaxis()->FindBin(xLow);
	binCenter = hChargeProfile.GetXaxis()->FindBin(xCenter);
	binHigh = hChargeProfile.GetXaxis()->FindBin(xHigh);
	hChargeProfile.SetBinContent(binLow, strip_dir+1, charge/(binHigh-binLow+1));
	hChargeProfile.SetBinContent(binCenter, strip_dir+1, charge/(binHigh-binLow+1));
	hChargeProfile.SetBinContent(binHigh, strip_dir+1, charge/(binHigh-binLow+1));
	/*
	for(int iBinTmp=binLow;iBinTmp<=binHigh;++iBinTmp){
	  x = hChargeProfile.GetXaxis()->GetBinCenter(iBinTmp);
	  std::cout<<"x: "<<x<<" charge: "<<charge/(binHigh-binLow+1)<<std::endl;
	  //hChargeProfile.Fill(x, strip_dir, charge/(binHigh-binLow+1));
	  hChargeProfile.SetBinContent(iBinTmp, strip_dir+1, charge/(binHigh-binLow+1));
	}
	*/
      }
    }
  //hChargeProfile.ProjectionX("px",1,1)->Print();
  //hChargeProfile.ProjectionX("px",2,2)->Print();
  //hChargeProfile.ProjectionX("px",3,3)->Print();
  return hChargeProfile;
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
