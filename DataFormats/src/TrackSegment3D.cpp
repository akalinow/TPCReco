#include "TPCReco/TrackSegment3D.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/colorText.h"

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

  double lambda = 200;
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
  for(int strip_dir=definitions::projection_type::DIR_U;strip_dir<=definitions::projection_type::DIR_W;++strip_dir){
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
double TrackSegment3D::getLambdaAtX(double x) const {
  return (x - myStart.X())/myTangent.X();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::getLambdaAtY(double y) const {
  return (y - myStart.Y())/myTangent.Y();
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
    std::cout<<__FUNCTION__<<KRED<<" No valid geometry pointer!"<<RST<<std::endl;
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
  for(int strip_dir=definitions::projection_type::DIR_U;strip_dir<=definitions::projection_type::DIR_W;++strip_dir){
    TrackSegment2D aTrack2DProjection = get2DProjection(strip_dir, 0, lambda);
    const Hit2DCollection & aRecHits = myRecHits.at(strip_dir);
    charge += aTrack2DProjection.getIntegratedCharge(lambda, aRecHits);
  }
  return charge;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::getRecHitChi2(int iProjection) const{

  double chi2 = 0.0;
  if(iProjection<definitions::projection_type::DIR_U || iProjection>definitions::projection_type::DIR_W){    
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

  for(int strip_dir=definitions::projection_type::DIR_U;strip_dir<=definitions::projection_type::DIR_W;++strip_dir){
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
void TrackSegment3D::addProjection(TH1F &histo, TGraphErrors &graph) const{

  double x, y, ex;
  double value=0, binCoverage=0.0;
  int binLow, binHigh;
  double binWidth = histo.GetBinWidth(1);
  for(int iPoint=0;iPoint<graph.GetN();++iPoint){
    graph.GetPoint(iPoint, x,y);
    ex = graph.GetErrorX(iPoint);
    binLow = histo.FindBin((x-ex)*getLength());
    binHigh = histo.FindBin((x+ex)*getLength());
    y *= binWidth/getLength();
    for(int iBin=binLow;iBin<=binHigh;++iBin){
      if(iBin==binLow) binCoverage = histo.GetXaxis()->GetBinUpEdge(iBin) - (x-ex)*getLength();
      else if(iBin==binHigh) binCoverage = (x+ex)*getLength() - histo.GetXaxis()->GetBinLowEdge(iBin);
      else binCoverage = histo.GetBinWidth(iBin);
      binCoverage /=histo.GetBinWidth(iBin);
      value = histo.GetBinContent(iBin);
      histo.SetBinContent(iBin, value+y*binCoverage);
    }
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TH1F TrackSegment3D::getChargeProfile() const{

  double radiusCut = 2;
  std::vector<TGraphErrors> projections;
  for(int strip_dir=definitions::projection_type::DIR_U;strip_dir<=definitions::projection_type::DIR_W;++strip_dir){
    TrackSegment2D aTrack2DProjection = get2DProjection(strip_dir, 0, getLength());
    const Hit2DCollection & aRecHits = myRecHits.at(strip_dir);
    projections.push_back(aTrack2DProjection.getChargeProfile(aRecHits, radiusCut));
  }

  int nBins = 1024;
  double minX = -0.2*getLength();
  double maxX = 1.2*getLength();
  TH1F hChargeProfile("hChargeProfile",";d [mm];charge",nBins, minX, maxX);
  hChargeProfile.SetDirectory(0);
  if(getLength()<1) return hChargeProfile;

  int maxPoints = 1;
  for(int strip_dir=definitions::projection_type::DIR_U;strip_dir<=definitions::projection_type::DIR_W;++strip_dir){
    TGraphErrors & aGraph = projections[strip_dir];
    if(aGraph.GetN()>maxPoints) maxPoints = aGraph.GetN();
    /*  
    std::string name = "density_graph_"+std::to_string(strip_dir);
    aGraph.SetName(name.c_str());
    name +=".root";
    aGraph.SaveAs(name.c_str());    
    */
    addProjection(hChargeProfile, aGraph);
  }

  int numberOfBins = std::min(maxPoints,256);
  int rebinFactor = hChargeProfile.GetNbinsX()/numberOfBins;
  while(nBins%rebinFactor && rebinFactor<nBins){
    ++rebinFactor;
  }
  hChargeProfile.Rebin(rebinFactor);
  //  hChargeProfile.Rebin(8);
  double scale = 1.0/hChargeProfile.GetBinWidth(1);
  hChargeProfile.Scale(scale); 
  return hChargeProfile;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::ostream & operator << (std::ostream &out, const TrackSegment3D &aSegment){

  const TVector3 & start = aSegment.getStart();
  const TVector3 & end = aSegment.getEnd();

  const TVector3 & bias = aSegment.getBias();
  const TVector3 & tangent = aSegment.getTangent();

  out<<"Segment:"<<std::endl
     <<"\t\t START -> END: ("<<start.X()<<", "<<start.Y()<<", "<<start.Z()<<")"
     <<" -> "
     <<"("<<end.X()<<", "<<end.Y()<<", "<<end.Z()<<") "
     <<std::endl
     <<"\t\t chi2: "<<aSegment.getRecHitChi2()<<""
     <<" charge [arb. u.]: "<<aSegment.getIntegratedCharge(aSegment.getLength())
     <<" length [mm]: "<<aSegment.getLength()
     <<std::endl
     <<"\t\t bias (X,Y,Z): "
     <<"("<<bias.X()<<", "<<bias.Y()<<", "<<bias.Z()<<")"
     <<std::endl
     <<"\t\t tangent (R, Theta, Phi): "
     <<"("<<tangent.Mag()<<", "<<tangent.Theta()<<", "<<tangent.Phi()<<")";

  return out;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
