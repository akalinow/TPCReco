#include <cstdlib>
#include <iostream>

#include "TCanvas.h"
#include "TH2D.h"
#include "TH3D.h"
#include "TSpectrum2.h"
#include "TVector3.h"

#include "GeometryTPC.h"
#include "EventTPC.h"

#include "HistoManager.h"
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
HistoManager::HistoManager() {

  myEvent = 0;

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
HistoManager::~HistoManager() {

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::setGeometry(std::shared_ptr<GeometryTPC> aGeometryPtr){
  
  myGeometryPtr = aGeometryPtr;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::setEvent(EventTPC* aEvent){

  myEvent = aEvent;
  
  double eventMaxCharge = aEvent->GetMaxCharge();
  double chargeThreshold = 0.1*eventMaxCharge;
  int delta_timecells = 1;
  int delta_strips = 1;

  aCluster = aEvent->GetOneCluster(chargeThreshold, delta_strips, delta_timecells);

  seedBias.resize(3);
  seedTangent.resize(3);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::getRawStripVsTime(int aDir){

  std::shared_ptr<TH2D> hProjection = myEvent->GetStripVsTime(aDir);
  double varianceX = hProjection->GetCovariance(1, 1);
  double varianceY = hProjection->GetCovariance(2, 2);
  double varianceXY = hProjection->GetCovariance(1, 2);

  std::vector<int> nStrips = {72, 92, 92};
  
  std::cout<<" varianceX*12: "<<varianceX*12/450/450
	   <<" varianceY*12: "<<varianceY*12/nStrips[aDir]/nStrips[aDir]
	   <<" varianceXY: "<<varianceXY
	   <<std::endl;
  
  return hProjection;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::getFilteredStripVsTime(int aDir){

  return myEvent->GetStripVsTime(aCluster, aDir);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TH3D* HistoManager::get3DReconstruction(){

  double radius = 2.0;
  int rebin_space=EVENTTPC_DEFAULT_STRIP_REBIN;
  int rebin_time=EVENTTPC_DEFAULT_TIME_REBIN; 
  int method=EVENTTPC_DEFAULT_RECO_METHOD;
  h3DReco = myEvent->Get3D(aCluster,  radius, rebin_space, rebin_time, method);
  return h3DReco;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TH2D* HistoManager::getHoughAccumulator(int aDir, int iPeak){

  std::shared_ptr<TH2D> hProjection = myEvent->GetStripVsTime(aDir);

  double maxX = hProjection->GetXaxis()->GetXmax();
  double maxY = hProjection->GetYaxis()->GetXmax();
  double rho = sqrt(2.0);
  double theta = 0.0;
  TH2D *hAccumulator = new TH2D("hAccumulator","",100,-M_PI,M_PI, 100, 0, rho);
     
    for(int iBinX=0;iBinX<hProjection->GetNbinsX();++iBinX){
      for(int iBinY=0;iBinY<hProjection->GetNbinsY();++iBinY){      
	int charge = hProjection->GetBinContent(iBinX, iBinY);
	if(charge<10) continue;
	for(int iBinTheta=1;iBinTheta<hAccumulator->GetNbinsX();++iBinTheta){
	  theta = hAccumulator->GetXaxis()->GetBinCenter(iBinTheta);
	  rho = iBinX/maxX*cos(theta) + iBinY/maxY*sin(theta);
	  hAccumulator->Fill(theta, rho, std::pow(charge, 1));
	}
      }
    }

    ///Find peaks
    std::cout<<"Looking for peaks in Hough accumulator"<<std::endl;

    int iBinX = 0, iBinY = 0, iBinZ=0;
    hAccumulator->GetMaximumBin(iBinX, iBinY, iBinZ);
    for(int aPeak=0;aPeak<iPeak;++aPeak){
      hAccumulator->SetBinContent(iBinX, iBinY, 0,0);
      hAccumulator->GetMaximumBin(iBinX, iBinY, iBinZ);
    }
    
    std::cout<<"Maximum location: "
	     <<" X: "<<hAccumulator->GetXaxis()->GetBinCenter(iBinX)
	     <<" Y: "<<hAccumulator->GetYaxis()->GetBinCenter(iBinY)
	     <<std::endl;
    double seedTheta = hAccumulator->GetXaxis()->GetBinCenter(iBinX);
    double rhoNormalised = hAccumulator->GetYaxis()->GetBinCenter(iBinY);
    double aX = rhoNormalised*cos(seedTheta)*maxX;
    double aY = rhoNormalised*sin(seedTheta)*maxY;
    seedBias[aDir].SetX(aX);
    seedBias[aDir].SetY(aY);

    aX = -rhoNormalised*sin(seedTheta)*maxX;
    aY = rhoNormalised*cos(seedTheta)*maxY;
    double norm = sqrt(aX*aX + aY*aY);
    seedTangent[aDir].SetX(aX/norm);
    seedTangent[aDir].SetY(aY/norm);
    ///Set tengent direcion along time arrow
    if(seedTangent[aDir].X()<0) seedTangent[aDir] *= -1;
    seedTangent[aDir] /= seedTangent[aDir].X();

    ///Move seed bias so lambda=0 correspond to t=0
    ///so biases from different projections can be compared
    ///FIX ME: seedTangent.X()==0!!
    double lambda = -seedBias[aDir].X()/seedTangent[aDir].X();
    seedBias[aDir] += lambda*seedTangent[aDir];

    std::cout<<__FUNCTION__<<" bias: ";
    seedBias[aDir].Print();
    

    //peakFinder.Search(hAccumulator,2,"col",0.5);
    std::cout<<"Done."<<std::endl;
    return hAccumulator;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TLine HistoManager::getTrackSeed(int aDir){

  double lambda = 0;  
  double x1 = (seedBias[aDir]+lambda*seedTangent[aDir]).X();
  double y1 = (seedBias[aDir]+lambda*seedTangent[aDir]).Y();

  lambda = 250;
  double x2 = (seedBias[aDir]+lambda*seedTangent[aDir]).X();
  double y2 = (seedBias[aDir]+lambda*seedTangent[aDir]).Y();
  
  TLine aTrackLine(x1,y1, x2, y2);
  aTrackLine.SetLineColor(2);
  aTrackLine.SetLineWidth(2);

  std::cout<<"Direction: "<<aDir<<" b: ";
  seedBias[aDir].Print();
  std::cout<<"t: ";
  seedTangent[aDir].Print();

  return aTrackLine;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TLine HistoManager::getLineProjection(int aDir){

  std::vector<int> stripOffset = {-71, 0, -55};
  
  //#### Angles of U/V/W unit vectors wrt X-axis [deg]
  //#ANGLES: 90.0 -30.0 30.0
  std::vector<double> phiPitchDirection = {M_PI, -M_PI/6.0 + M_PI/2.0, M_PI/6.0 - M_PI/2.0};
  TVector3 stripPitchDirection(cos(phiPitchDirection[aDir]),
			       sin(phiPitchDirection[aDir]), 0);


  double bZ = seedBias[0].X();
  double bX = (seedBias[0].Y() + stripOffset[0])*cos(phiPitchDirection[0]);
  double bY = (seedBias[1].Y()+stripOffset[1] -bX*cos(phiPitchDirection[1]))/sin(phiPitchDirection[1]);
  bY += (seedBias[2].Y()+stripOffset[2]-bX*cos(phiPitchDirection[2]))/sin(phiPitchDirection[2]);
  bY /=2.0;

  TVector3 b(bX, bY, bZ);

  double tZ = seedTangent[0].X();
  double tX = -seedTangent[0].Y();
  double tY = (seedTangent[1].Y() - tX*cos(phiPitchDirection[1]))/sin(phiPitchDirection[1]);
  tY += (seedTangent[2].Y() - tX*cos(phiPitchDirection[2]))/sin(phiPitchDirection[2]);
  tY /= 2.0;

  TVector3 t(tX, tY, tZ);
  std::cout<<"3D b: ";
  b.Print();
  std::cout<<"t: ";
  t.Print();
  std::cout<<std::endl;

  double lambda = 0;
  TVector3 aPointOnLine;

  lambda = 0;
  aPointOnLine = b + lambda*t;
  double x1 = aPointOnLine.Z();
  double y1 = aPointOnLine*stripPitchDirection - stripOffset[aDir];

  lambda = 2000;
  aPointOnLine = b + lambda*t;
  double x2 = aPointOnLine.Z();
  double y2 = aPointOnLine*stripPitchDirection - stripOffset[aDir];

  TLine aLineProjection(x1, y1, x2, y2);
  aLineProjection.SetLineColor(4);
  aLineProjection.SetLineWidth(2);

  std::cout<<"direction: "<<aDir
           <<" (x1, y1): "<<x1<<", "<<y1
	   <<" (x2, y2): "<<x2<<", "<<y2
	   <<std::endl;
    

  return aLineProjection;
  
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

