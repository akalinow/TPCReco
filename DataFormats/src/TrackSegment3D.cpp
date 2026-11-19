#include "TPCReco/TrackSegment3D.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/GainMap.h"
#include "TPCReco/colorText.h"

#include <cmath>
#include <iostream>

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TrackSegment3D::TrackSegment3D(){

  myRecHits.clear();
  myRecHits.resize(3);

  myProjectionsLoss.resize(3);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr){
  
  myGeometryPtr = aGeometryPtr;
  calculateLoss(); // the projections depend on the geometry
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<GeometryTPC> TrackSegment3D::getGeometry() const{
  return myGeometryPtr;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::setBias(const TVector3 & aBias){

   myBias = aBias;
   initialize();

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

  TVector3 bias, tangent;
  bias.SetMagThetaPhi(par[0], myBias.Theta(), myBias.Phi());
  tangent.SetMagThetaPhi(1.0, par[1], par[2]);
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
  calculateLoss();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::initialize(){

  // points where the line crosses the planes X=0, Y=0, Z=0; if the line is
  // (nearly) parallel to a plane (or the tangent is null, e.g. a dot), there is no
  // crossing point and the bias is used (was +-inf or NaN)
  constexpr double minComponent = 1E-3; // crossings within ~1000 x the bias distance
  auto crossing = [this](double biasComponent, double tangentComponent){
    if(std::abs(tangentComponent)<minComponent) return myBias;
    return TVector3(myBias - (biasComponent/tangentComponent)*myTangent);
  };
  myBiasAtX0 = crossing(myBias.X(), myTangent.X());
  myBiasAtY0 = crossing(myBias.Y(), myTangent.Y());
  myBiasAtZ0 = crossing(myBias.Z(), myTangent.Z());

  myLenght = (myEnd - myStart).Mag();

  calculateLoss();
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

  std::vector<double> coordinates(3);
  double *data = coordinates.data();
  data[0] = getBias().Mag();
  data[1] = getTangent().Theta();
  data[2] = getTangent().Phi();

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
TrackSegment2D::ChargeWeight TrackSegment3D::getGainCorrection(double lambdaEnd) const{

  // 2D projections of [0, lambdaEnd] are linear images of the 3D segment: the fraction f
  // along the projection is the fraction along the 3D segment
  std::shared_ptr<const GainMap> aGainMap = myGeometryPtr ? myGeometryPtr->GetGainMap() : nullptr;
  if(!aGainMap || aGainMap->empty()) return TrackSegment2D::ChargeWeight();
  const double x0 = getStart().X();
  const double dx = lambdaEnd*getTangent().X();
  return [aGainMap, x0, dx](double f){ return 1.0/aGainMap->getGain(x0 + f*dx); };
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::getIntegratedCharge(double lambda) const{
  if(lambda<0) return 0;
  double charge = 0.0;
  const TrackSegment2D::ChargeWeight gainCorrection = getGainCorrection(lambda);
  for(int strip_dir=definitions::projection_type::DIR_U;strip_dir<=definitions::projection_type::DIR_W;++strip_dir){
    TrackSegment2D aTrack2DProjection = get2DProjection(strip_dir, 0, lambda);
    const Hit2DCollection & aRecHits = myRecHits.at(strip_dir);
    charge += aTrack2DProjection.getIntegratedCharge(lambda, aRecHits, gainCorrection);
  }
  return charge;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::getMaxCharge() const {

  double maxCharge = 0.0;
  for(int strip_dir=definitions::projection_type::DIR_U;strip_dir<=definitions::projection_type::DIR_W;++strip_dir){
    const Hit2DCollection & aRecHits = myRecHits.at(strip_dir);
    for(const auto & aHit:aRecHits){
      if(aHit.getCharge()>maxCharge) maxCharge = aHit.getCharge();
    }
  }
  return maxCharge;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::getMaxProjWidth() const {

  double maxProjWidth = 0.0;
  for(int strip_dir=definitions::projection_type::DIR_U;strip_dir<=definitions::projection_type::DIR_W;++strip_dir){
    TrackSegment2D aTrack2DProjection = get2DProjection(strip_dir, 0, getLength());
    const Hit2DCollection & aRecHits = myRecHits.at(strip_dir);
    double projWidth = aTrack2DProjection.getLoss(aRecHits, definitions::fit_type::TANGENT);
    // no hit close to the projection: the width is not defined (penalty value)
    if(projWidth>=TrackSegment2D::dummyLoss) continue;
    if(projWidth>maxProjWidth) maxProjWidth = projWidth;
  }
  return maxProjWidth;
}    
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::getLoss(int iProjection) const{

  double loss = 0.0;
  if(iProjection<definitions::projection_type::DIR_U || iProjection>definitions::projection_type::DIR_W){    
    std::for_each(myProjectionsLoss.begin(), myProjectionsLoss.end(), [&](auto aItem){loss += aItem;});
  }
  else{
    loss += myProjectionsLoss[iProjection];
  }
  return loss;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void TrackSegment3D::calculateLoss(){

  for(int strip_dir=definitions::projection_type::DIR_U;strip_dir<=definitions::projection_type::DIR_W;++strip_dir){
    TrackSegment2D aTrack2DProjection = get2DProjection(strip_dir, 0, getLength());
    const Hit2DCollection & aRecHits = myRecHits.at(strip_dir);
    myProjectionsLoss[strip_dir] = aTrack2DProjection.getLoss(aRecHits, myLossType);
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double TrackSegment3D::operator() (const double *par) {

  TVector3 start(par[0], par[1], par[2]);
  TVector3 end(par[3], par[4], par[5]);
  setStartEnd(start, end);
  return getLoss();
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
      // fraction of bin iBin covered by the hit interval [(x-ex)*L, (x+ex)*L]
      if(binLow==binHigh) binCoverage = 2*ex*getLength();
      else if(iBin==binLow) binCoverage = histo.GetXaxis()->GetBinUpEdge(iBin) - (x-ex)*getLength();
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

  // Maximum hit distance from 2D projection
  double radiusCut = 4; //parameter to be put into configuration
  
  // Minimal length of projection to be considered. Short projections introduce noisy floor to the charge profile,
  // which affects dE/dx fit.
  double minProjLength = 30;//parameter to be put into configuration

  int nBins = 1024;
  double minX = -0.2*getLength();
  double maxX = 1.2*getLength();
  // do not attach the histogram to the current directory (e.g. an open output file)
  const bool addDirectoryStatus = TH1::AddDirectoryStatus();
  TH1::AddDirectory(false);
  TH1F hChargeProfile("hChargeProfile",";d [mm];charge/mm",nBins, minX, maxX);
  TH1::AddDirectory(addDirectoryStatus);
  if(getLength()<1) return hChargeProfile;

  const TrackSegment2D::ChargeWeight gainCorrection = getGainCorrection(getLength());
  for(int strip_dir=definitions::projection_type::DIR_U;strip_dir<=definitions::projection_type::DIR_W;++strip_dir){
    TrackSegment2D aTrack2DProjection = get2DProjection(strip_dir, 0, getLength());
    const Hit2DCollection & aRecHits = myRecHits.at(strip_dir);
    TGraphErrors aGraph = aTrack2DProjection.getChargeProfile(aRecHits, radiusCut, gainCorrection);
    double projLength = aTrack2DProjection.getLength();
    //double graphLength = aGraph.GetXaxis()->GetXmax() - aGraph.GetXaxis()->GetXmin();

    double segmentAlongStrip = getTangent().Dot(myGeometryPtr->GetStripPitchVector3D(strip_dir).Unit())/sin(getTangent().Theta());
    ///do not consider short projections unless the track is very long
    ///very long track is a track seed before shrinking to hits range
    /*
    std::cout<<KGRN<<"strip_dir: "<<strip_dir<<" along strip: "<<segmentAlongStrip
            <<" projLength: "<<projLength
            <<" graphLength: "<<graphLength
            <<"projLength*graphLength: "<<projLength*graphLength
            <<RST<<std::endl;
     */
    if(projLength<minProjLength ||   //short projection 
    (getLength()>200 && std::abs(cos(getTangent().Theta()))<0.95 && std::abs(segmentAlongStrip)<0.2)) { //initial track with "infinite" length. Horizontal track do not have well defined phi
      continue;
    }    
    addProjection(hChargeProfile, aGraph);
  }

  int rebinFactor = log(4.0*nBins/(maxX - minX))/log(2); //bin width is around 2 mm
  rebinFactor = std::pow(2, rebinFactor);
  hChargeProfile.Rebin(rebinFactor);
  double scale = 1.0/hChargeProfile.GetBinWidth(1);
  hChargeProfile.Scale(scale); 
  double max = hChargeProfile.GetMaximum();
  if(max<1) max = 1.0;
  hChargeProfile.Scale(1.0/max); 
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
     <<"\t\t loss: "<<aSegment.getLoss()
     <<" max 2D projection width "<<aSegment.getMaxProjWidth()<<std::endl
     <<"\t\t charge [arb. u.]: "<<aSegment.getIntegratedCharge(aSegment.getLength())
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
