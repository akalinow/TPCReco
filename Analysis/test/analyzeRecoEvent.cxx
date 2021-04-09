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

  std::string fileName = "Reco_EventTPC_2018-06-19T15:13:33.941_0008.root";
  Track3D *aTrack = loadRecoEvent(fileName);
  fileName = "/home/akalinow/scratch/data/neutrons/geometry_mini_eTPC_2018-06-19T10:35:30.853.dat";
  std::shared_ptr<GeometryTPC> aGeometry = loadGeometry(fileName);
  std::cout<<*aTrack<<std::endl;

  TCanvas aCanvas("aCanvas","",500,500);
  TrackSegment3D aSegment = aTrack->getSegments().front();
  aSegment.setGeometry(aGeometry);
  int strip_dir = 0;
  double startLambda = 0;
  double endLambda = aSegment.getLength();
  TrackSegment2D aStripProjection = aSegment.get2DProjection(strip_dir, startLambda, endLambda);
  std::cout<<aStripProjection<<std::endl;
}
//////////////////////////
//////////////////////////
