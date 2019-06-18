#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

#include "UtilsTPC.h"
#include "GeometryTPC.h"
#include "UVWprojector.h"
#include "EventTPC.h"

#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TBrowser.h"

int main(int argc, char *argv[]) {

   ///Initialize projections
  std::string geomFileName = "resources/geometry_mini_eTPC.dat";
  std::shared_ptr<GeometryTPC> myGeometryPtr = std::make_shared<GeometryTPC>(geomFileName.c_str());
  
  std::string dataFileName = "resources/EventTPC_1.root";
  TFile aFile(dataFileName.c_str(),"READ");
  TTree *aTree = (TTree*)aFile.Get("TPCData");

  EventTPC *aEvent = 0;
  aTree->SetBranchAddress("Event", &aEvent);
  aTree->GetEntry(0);

  aEvent->SetGeoPtr(myGeometryPtr);

  double eventMaxCharge = aEvent->GetMaxCharge();
  double chargeThreshold = 0.01*eventMaxCharge;
  int delta_timecells = 1;
  int delta_strips = 1;
  double radius = 1.0;
  int rebin_space=EVENTTPC_DEFAULT_STRIP_REBIN;
  int rebin_time=EVENTTPC_DEFAULT_TIME_REBIN; 
  int method=EVENTTPC_DEFAULT_RECO_METHOD;
    
  SigClusterTPC aCluster = aEvent->GetOneCluster(chargeThreshold, delta_strips, delta_timecells);
  TH3F *aCluster_h3D = aEvent->Get3D(aCluster,  radius, rebin_space, rebin_time, method);

  //TCanvas *c1 = new TCanvas("c1","c1", 900, 800);
  //aCluster_h3D->Draw();
  TBrowser* b = new TBrowser("clusterBrowser");
  if(b && aCluster_h3D) std::cout<<"AAA"<<std::endl;

  return 0;
}
