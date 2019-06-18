#include <iostream>
#include <cstdlib>
#include <string>

#include "UtilsTPC.h"
#include "GeometryTPC.h"
#include "UVWprojector.h"
#include "EventTPC.h"

#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"

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

  TCanvas *c1 = new TCanvas("c1","c1", 900, 800);

  aEvent->SetGeoPtr(myGeometryPtr);
  
  aEvent->GetStripVsTime(DIR_U)->Draw("colz");
  c1->Print("results/UZProjection.png");

  aEvent->GetStripVsTime(DIR_V)->Draw("colz");
  c1->Print("results/VZProjection.png");
  

  aEvent->GetStripVsTime(DIR_W)->Draw("colz");
  c1->Print("results/WZProjection.png");  
  
  return 0;
}
