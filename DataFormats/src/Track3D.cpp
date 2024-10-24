#include <iostream>
#include <algorithm>

#include "TPCReco/Track3D.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/colorText.h"

#include <Fit/Fitter.h>
#include <Math/Functor.h>
#include <Math/GenVector/VectorUtil.h>

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Track3D::Track3D(){ };
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void Track3D::addSegment(const TrackSegment3D & aSegment3D){

  mySegments.push_back(aSegment3D);
  update();

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void Track3D::update(){

  myLenght = 0.0;
  for(auto &aSegment: mySegments){
    myLenght += aSegment.getLength();
  }
  updateChi2();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<double> Track3D::getSegmentsStartEndXYZ() const{

  std::vector<double> coordinates;
  if(!mySegments.size()) return coordinates;

  std::vector<double> segmentCoordinates = mySegments.front().getStartEndXYZ();
  coordinates.insert(coordinates.end(), segmentCoordinates.begin(), segmentCoordinates.begin()+3);
  
  for(auto &aSegment: mySegments){
    std::vector<double> segmentCoordinates = aSegment.getStartEndXYZ();
    coordinates.insert(coordinates.end(), segmentCoordinates.begin()+3, segmentCoordinates.begin()+6);
  }

  return coordinates;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<double> Track3D::getSegmentsBiasTangentCoords() const{

  std::vector<double> coordinates;
  if(!mySegments.size()) return coordinates;

  for(auto &aSegment: mySegments){
    std::vector<double> segmentCoordinates = aSegment.getBiasTangentCoords();
    coordinates.insert(coordinates.end(), segmentCoordinates.begin(), segmentCoordinates.end());
  }

  return coordinates;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double Track3D::getIntegratedCharge(double lambda) const{
  double charge = 0.0;
  double pastSegmentsLambda = 0.0;
  double segmentLambda = 0.0;
  for(const auto & aSegment: mySegments){
    segmentLambda = lambda - pastSegmentsLambda;
    if(segmentLambda>aSegment.getLength()) segmentLambda = aSegment.getLength();
    charge += aSegment.getIntegratedCharge(segmentLambda);
    pastSegmentsLambda += aSegment.getLength();
    if(segmentLambda<aSegment.getLength()) break;
  }
  return charge;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void Track3D::enableProjectionForChi2(int iProjection){

  iProjectionForChi2 = iProjection;
  updateChi2();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double Track3D::getChi2() const{

  double chi2 = 0.0;
  chi2 += getSegmentsChi2();
  chi2 += getNodesChi2();

  return chi2;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double Track3D::getSegmentsChi2() const{

  double chi2 = 0.0;
  std::for_each(segmentChi2.begin(), segmentChi2.end(), [&](auto aItem){chi2 += aItem;});
  return chi2;

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double Track3D::getNodesChi2() const{

  double chi2 = 0.0;
  std::for_each(nodeHitsChi2.begin(), nodeHitsChi2.end(), [&](auto aItem){chi2 += aItem;});
  std::for_each(nodeAngleChi2.begin(), nodeAngleChi2.end(), [&](auto aItem){chi2 += aItem;});
  return chi2;

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void Track3D::updateNodesChi2(int strip_dir){

  if(!mySegments.size()) return;

  TVector3 aPoint;
  double charge = 0.0;
  double sum = 0.0;
  double nodeChi2 = 0.0;
  unsigned int iNode = 0;

  for(auto aSegment = mySegments.begin(); aSegment!=(mySegments.end()-1);++aSegment){

    sum = 0.0;
    nodeChi2 = 0.0;

    TVector3 node3D = aSegment->getEnd();
    TVector3 formerTangent3D = aSegment->getTangent();
    TVector3 latterTangent3D = (aSegment+1)->getTangent();
    TVector3 stripPitchDirection = aSegment->getGeometry()->GetStripPitchVector3D(strip_dir);

    TVector3 node2D(node3D.Z(), node3D*stripPitchDirection, 0.0);
    TVector3 formerTransverse2D(-formerTangent3D*stripPitchDirection, formerTangent3D.Z(), 0.0);
    TVector3 latterTransverse2D(-latterTangent3D*stripPitchDirection, latterTangent3D.Z(), 0.0);
    double deltaPhi = ROOT::Math::VectorUtil::DeltaPhi(formerTransverse2D, latterTransverse2D);
    nodeAngleChi2[iNode] = 0.1*(std::pow(deltaPhi,2));//FIX me optimize the coefficient value
    if(ROOT::Math::VectorUtil::DeltaPhi(formerTransverse2D, latterTransverse2D)<0) std::swap(formerTransverse2D, latterTransverse2D);

    for(const auto aHit:mySegments.front().getRecHits().at(strip_dir)){
      charge = aHit.getCharge();
      aPoint.SetXYZ(aHit.getPosTime(), aHit.getPosStrip(), 0.0);
      aPoint -= node2D;
      double deltaHit = ROOT::Math::VectorUtil::Angle(formerTransverse2D, aPoint);
      double nodeOpeningAngle = ROOT::Math::VectorUtil::DeltaPhi(formerTransverse2D, latterTransverse2D);
      bool belongsToNode = deltaHit < nodeOpeningAngle && deltaHit>0.0;
      if(belongsToNode){
	nodeChi2 += aPoint.Mag2()*charge;
	sum += charge;
      }
    }
    if(sum>0.0) nodeHitsChi2[iNode] += nodeChi2/sum;
    ++iNode;
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void Track3D::updateChi2(){

  segmentChi2.clear();
  for(auto aItem: mySegments){
    segmentChi2.push_back(aItem.getRecHitChi2(iProjectionForChi2));
  }

  /* Nodes not active yet
  nodeHitsChi2.clear();
  if(mySegments.size()) nodeHitsChi2.resize(mySegments.size()-1);

  nodeAngleChi2.clear();
  if(mySegments.size()) nodeAngleChi2.resize(mySegments.size()-1);

  for(int strip_dir=definitions::projection_type::DIR_U;strip_dir<=definitions::projection_type::DIR_W;++strip_dir){
    updateNodesChi2(strip_dir);
  }
  */
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double Track3D::getNodeHitsChi2(unsigned int iNode) const{

  if(iNode>=nodeHitsChi2.size()) return 0.0;
  else return nodeHitsChi2.at(iNode);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double Track3D::getNodeAngleChi2(unsigned int iNode) const{

  if(iNode>=nodeAngleChi2.size()) return 0.0;
  else return nodeAngleChi2.at(iNode);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void Track3D::splitWorseChi2Segment(double lenghtFraction){

  update();
  if(!segmentChi2.size()) return;
  if(lenghtFraction<0 || lenghtFraction>1.0) lenghtFraction = 0.5;

  double maxChi2 = 0.0;
  unsigned int worseChi2Segment = 0;
  for(unsigned int iSegment=0;iSegment<mySegments.size();++iSegment){
    if(segmentChi2[iSegment]>maxChi2){
      maxChi2 = segmentChi2[iSegment];
      worseChi2Segment = iSegment;
    }
  }
  splitSegment(worseChi2Segment, lenghtFraction);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void Track3D::splitSegment(unsigned int iSegment, double lenghtFraction){

  TrackSegment3D & aSegment = mySegments.at(iSegment);
  TVector3 aStep = aSegment.getLength()*aSegment.getTangent();
  aSegment.setStartEnd(aSegment.getStart(), aSegment.getStart() + lenghtFraction*aStep);

  TrackSegment3D aNewSegment = mySegments.at(iSegment);
  aNewSegment.setStartEnd(aSegment.getEnd(), aSegment.getEnd() + (1.0-lenghtFraction)*aStep);
  mySegments.insert(mySegments.begin()+iSegment+1, aNewSegment);
  update();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void Track3D::extendToChamberRange(const std::tuple<double, double, double, double> & xyRange,
				   const std::tuple<double, double> & zRange){

  if(getLength()<1) return;
  bool extended = extendToZRange(std::get<0>(zRange),std::get<1>(zRange));

  if(!extended){
    extendToXYRange(std::get<0>(xyRange), std::get<1>(xyRange),
		    std::get<2>(xyRange), std::get<3>(xyRange));
  }
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool Track3D::extendToZRange(double zMin, double zMax){

  if(!mySegments.size()) return false;

  TrackSegment3D & aFirstSegment = mySegments.front();
  TrackSegment3D & aLastSegment = mySegments.back();

  if(std::abs(aFirstSegment.getTangent().Z())<1E-3 ||
     std::abs(aLastSegment.getTangent().Z())<1E-3) return false;

  double lambda =  aFirstSegment.getLambdaAtZ(zMin);
  TVector3 aStart = aFirstSegment.getStart() + lambda*aFirstSegment.getTangent();
  TVector3 aEnd = aFirstSegment.getEnd();
  aFirstSegment.setStartEnd(aStart, aEnd);

  lambda =  aLastSegment.getLambdaAtZ(zMax);
  aStart = aLastSegment.getStart();
  aEnd = aStart + lambda*aLastSegment.getTangent();
  aLastSegment.setStartEnd(aStart, aEnd);
  update();
  return true;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void Track3D::extendToXYRange(double xMin, double xMax,
			      double yMin, double yMax){

  if(!mySegments.size()) return;
  double lambda =  0;
  TrackSegment3D & aFirstSegment = mySegments.front();
  TrackSegment3D & aLastSegment = mySegments.back();

  TVector3 aStart = aFirstSegment.getStart();
  TVector3 aEnd = aLastSegment.getEnd();
  double delta = 0.0;

  aStart = aFirstSegment.getStart();
  aEnd = aLastSegment.getEnd();
  delta = std::abs(aStart.X() - xMin);
  if(std::abs(aEnd.X() - xMin)<delta){
    lambda = aLastSegment.getLambdaAtX(xMin);
    aEnd =  aLastSegment.getStart() + lambda*aLastSegment.getTangent();
    aLastSegment.setStartEnd(aLastSegment.getStart(), aEnd);
  }
  else{
    lambda = aFirstSegment.getLambdaAtX(xMin);
    aStart =  aFirstSegment.getStart() + lambda*aFirstSegment.getTangent();
    aFirstSegment.setStartEnd(aStart, aFirstSegment.getEnd());
  }
  aStart = aFirstSegment.getStart();
  aEnd = aLastSegment.getEnd();
  delta = std::abs(aStart.X() - xMax);
  if(std::abs(aEnd.X() - xMax)<delta){
    lambda = aLastSegment.getLambdaAtX(xMax);
    aEnd =  aLastSegment.getStart() + lambda*aLastSegment.getTangent();
    aLastSegment.setStartEnd(aLastSegment.getStart(), aEnd);
  }
  else{
    lambda = aFirstSegment.getLambdaAtX(xMax);
    aStart =  aFirstSegment.getStart() + lambda*aFirstSegment.getTangent();
    aFirstSegment.setStartEnd(aStart, aFirstSegment.getEnd());
  }
  update();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void Track3D::shrinkToHits(){

  if(getLength()<1.0) return;

  TrackSegment3D & aFirstSegment = mySegments.front();
  TrackSegment3D & aLastSegment = mySegments.back();

  TH1F hChargeProfileStart = aFirstSegment.getChargeProfile();
  TH1F hChargeProfileEnd = aLastSegment.getChargeProfile();
  double chargeCut = 0.0;
  
  int startBin = 0;
  int endBin = 1E6;
  int binTmp = 0;
  chargeCut = 0.05*hChargeProfileStart.GetMaximum();
  binTmp = hChargeProfileStart.FindFirstBinAbove(chargeCut);
  if(binTmp>startBin) startBin = binTmp;
    ///
  chargeCut = 0.05*hChargeProfileEnd.GetMaximum();
  binTmp = hChargeProfileEnd.FindLastBinAbove(chargeCut);
  if(binTmp<endBin) endBin = binTmp;

  if(endBin-startBin<2){
    startBin-=1;
    endBin+=1;
  }

  double lambdaStart = hChargeProfileStart.GetBinCenter(startBin);
  double lambdaEnd = hChargeProfileEnd.GetBinCenter(endBin);

  TVector3 aStart = aFirstSegment.getStart() + getSegmentLambda(lambdaStart, 0)*aFirstSegment.getTangent();
  TVector3 aEnd = aLastSegment.getStart() + getSegmentLambda(lambdaEnd, mySegments.size()-1)*aLastSegment.getTangent();

  aFirstSegment.setStartEnd(aStart, aFirstSegment.getEnd());
  aLastSegment.setStartEnd(aLastSegment.getStart(), aEnd);
  update();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double Track3D::getSegmentLambda(double lambda, unsigned int iSegment) const{

  double segmentLambda = lambda;
  for(unsigned int index=0;index<iSegment;++index){
    segmentLambda -= mySegments[index].getLength();
  }
  if(segmentLambda<0) segmentLambda = 0.0;
  return segmentLambda;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void Track3D::removeEmptySegments(){

  auto modifiedEnd = std::remove_if(mySegments.begin(), mySegments.end(), [](auto aItem){
      std::vector<double> segmentParameters = aItem.getStartEndXYZ();
      double segmentChi2 = aItem(segmentParameters.data());
      return segmentChi2<1E-5 || segmentChi2>999;
    });

  unsigned int newSize = modifiedEnd - mySegments.begin();
  mySegments.resize(newSize);
  update();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double Track3D::chi2FromNodesList(const double *par){
  
  for(unsigned int iSegment=0;iSegment<mySegments.size();++iSegment){
    if(myFitMode==FIT_START_STOP){

      double segmentParameters[6];

      segmentParameters[0]  = par[0];
      segmentParameters[1]  = par[1];
      segmentParameters[2]  = par[2];

      segmentParameters[3]  = par[3*iSegment+3];
      segmentParameters[4]  = par[3*iSegment+4];
      segmentParameters[5]  = par[3*iSegment+5];
      
      mySegments.at(iSegment).setStartEnd(segmentParameters);
    }
    if(myFitMode==FIT_BIAS_TANGENT){
      const double *segmentParameters = par+3*iSegment;
      mySegments.at(iSegment).setBiasTangent(segmentParameters);
    }
  }

  update();
  return getChi2();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
double Track3D::chi2FromSplitPoint(const double *par){

  double splitPoint = 0.5;
  if(par[0]>0.0 && par[0]<1.0) splitPoint = par[0];
  Track3D aTestTrack = *this;
  aTestTrack.splitWorseChi2Segment(splitPoint);
  return aTestTrack.getChi2();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::ostream & operator << (std::ostream &out, const Track3D &aTrack){

  out << "Number of segments: "<<aTrack.getSegments().size()<<std::endl;
  if(!aTrack.getSegments().size()) return out;

  out<<"\t Segments:"<<std::endl;
  out<<KBLU<<"-----------------------------------"<<RST<<std::endl;
  for(auto aSegment: aTrack.getSegments()){
    out<<"\t \t"<<aSegment<<std::endl;
    out<<KBLU<<"-----------------------------------"<<RST<<std::endl;
  }

  auto length = aTrack.getLength();
  auto charge = aTrack.getIntegratedCharge(length);
  auto chargeFromProfile = aTrack.getChargeProfile().Integral("width");
  out<<"\t Total track length [mm]: "<<length<<std::endl;
  out<<"\t Total track charge [arb. units]: "<<charge<<std::endl;
  out<<"\t Total track charge  "<<std::endl;
  out<<"\t from 3D profile [arb. units]: "<<aTrack.getChargeProfile().Integral("width")<<std::endl;
  out<<"\t Hit fit loss func.: "<<aTrack.getChi2()<<std::endl;
  out<<"\t dE/dx fit loss func./charge^2: "<<aTrack.getHypothesisFitChi2()/std::pow(chargeFromProfile,2)<<std::endl;
  out<<"\t       fitted diffusion [mm]: "<<aTrack.getSegments().front().getDiffusion()<<std::endl;
  out<<KBLU<<"-----------------------------------"<<RST<<std::endl;
  return out;
}
