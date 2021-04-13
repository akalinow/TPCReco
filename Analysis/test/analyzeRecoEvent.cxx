#include "TROOT.h"
#include "../../DataFormats/include/Track3D.h"
#include "../../DataFormats/include/GeometryTPC.h"
//////////////////////////
//////////////////////////
std::shared_ptr<GeometryTPC> loadGeometry(const std::string fileName){
  return std::make_shared<GeometryTPC>(fileName.c_str(), false);
}
//////////////////////////
//////////////////////////
Track3D *loadRecoEvent(const std::string fileName){

  if (!gROOT->GetClass("Track3D")){
    R__LOAD_LIBRARY(/home/akalinow/TPCReco/build/lib/libDataFormats.so);
  }
  
  TFile *aFile = new TFile(fileName.c_str());
  TTree *aTree = (TTree*)aFile->Get("TPCRecoData");
  Track3D *aTrack = new Track3D();
  TBranch *aBranch  = aTree->GetBranch("RecoEvent");
  aBranch->SetAddress(&aTrack);
  aBranch->GetEntry(0); 
  return aTrack;
}
//////////////////////////
//////////////////////////
void plotTrack(){

  ///Data and geometry loading.
  std::string fileName = "Reco_EventTPC_2018-06-19T15:13:33.941_0008.root";
  Track3D *aTrack = loadRecoEvent(fileName);
  fileName = "/home/akalinow/scratch/data/neutrons/geometry_mini_eTPC_2018-06-19T10:35:30.853.dat";
  std::shared_ptr<GeometryTPC> aGeometry = loadGeometry(fileName);
  std::cout<<*aTrack<<std::endl;

  ///Fetching tracks segments from the full track.
  TrackSegment3D aSegment = aTrack->getSegments().front();
  aSegment.setGeometry(aGeometry);

  ///Choose the U direction and get the 2D segment
  int strip_dir = 0;
  double startLambda = 0;
  double endLambda = aSegment.getLength();
  TrackSegment2D aStripProjection = aSegment.get2DProjection(strip_dir, startLambda, endLambda);
  const TVector3 & start = aStripProjection.getStart();
  const TVector3 & end = aStripProjection.getEnd();
  std::cout<<aStripProjection<<std::endl;

  ///Some simple plots for the 2D segment.
  TH2F *hProjection = new TH2F("hProjection",";x [mm]; y[mm]; charge",120, 0, 120, 120, -60, 60);
  hProjection->SetStats(false);
  TH1D *hDistance = new TH1D("hDistance",";hit distance [mm];charge",20,0,100);
  hDistance->SetStats(false);
  TVector3 aPoint;
  double distance = 0.0;
  for(auto aHit: aSegment.getRecHits().at(strip_dir)){
    double x = aHit.getPosTime();
    double y = aHit.getPosWire();
    double charge = aHit.getCharge();
    aPoint.SetXYZ(x,y,0);
    distance = aStripProjection.getPointTransverseDistance(aPoint);
    if(distance>0){
      hProjection->Fill(x,y,charge);
    }
    hDistance->Fill(distance,charge);
  }
  TCanvas *aCanvas = new TCanvas("aCanvas","",700,600);
  TLine *aSegment2DLine = new TLine(start.X(), start.Y(),  end.X(),  end.Y());
  aSegment2DLine->SetLineColor(2);
  aSegment2DLine->SetLineWidth(2);
  aCanvas->Divide(2,1);
  aCanvas->cd(1);
  hProjection->Draw("colz");
  aSegment2DLine->Draw();
  aCanvas->cd(2);
  hDistance->Draw("colz");
}
//////////////////////////
//////////////////////////
