#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

#include "UtilsTPC.h"
#include "GeometryTPC.h"
#include "UVWprojector.h"
#include "EventTPC.h"

#include "TApplication.h"
#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TBrowser.h"

int main(int argc, char *argv[]) {

  TApplication app("app", &argc, argv);

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
  double chargeThreshold = 0.1*eventMaxCharge;
  int delta_timecells = 1;
  int delta_strips = 1;
  double radius = 10.0;
  int rebin_space=EVENTTPC_DEFAULT_STRIP_REBIN;
  int rebin_time=EVENTTPC_DEFAULT_TIME_REBIN; 
  int method=EVENTTPC_DEFAULT_RECO_METHOD;

  aEvent->GetStripVsTime(DIR_U)->Draw("colz");
  aEvent->GetStripVsTime(DIR_V)->Draw("colz");
  aEvent->GetStripVsTime(DIR_W)->Draw("colz");

  SigClusterTPC aCluster = aEvent->GetOneCluster(chargeThreshold, delta_strips, delta_timecells);
  TH3F *aCluster_h3D = aEvent->Get3D(aCluster,  radius, rebin_space, rebin_time, method);
  
  TBrowser* b = new TBrowser("clusterBrowser");
  if(b && aCluster_h3D)   app.Run(kTRUE);

 
  return 0;
}
