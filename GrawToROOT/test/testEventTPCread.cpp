#include <iostream>
#include <cstdlib>
#include <string>
#include <memory>

#include "TPCReco/UtilsTPC.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/UVWprojector.h"
#include "TPCReco/EventTPC.h"

#include <TFile.h"
#include <TTree.h"
#include <TChain.h"
#include <TCanvas.h"
#include <TH1D.h"

int main(int argc, char *argv[]) {

  ///Initialize projections
  std::string geomFileName = "resources/geometry_mini_eTPC.dat";
  std::shared_ptr<GeometryTPC> myGeometryPtr = std::make_shared<GeometryTPC>(geomFileName.c_str());
  
  std::string dataFileName = "resources/EventTPC_2018-06-19T11:18:25.056.root";
  TFile aFile(dataFileName.c_str(),"READ");
  TTree *aTree = (TTree*)aFile.Get("TPCData");

  EventTPC *aEvent = 0;

  std::shared_ptr<TH1D> hTotalCharge(new TH1D("hTotalCharge","Total charge;Charge[arb. units];Entries",100,0,500000));
  hTotalCharge->SetAxisRange(-100,1E8);//Draw under- and overflow bins

  aTree->SetBranchAddress("Event", &aEvent);

  int nEntries = aTree->GetEntries();
  std::cout<<"Entries in file: "<<dataFileName<<" "<<nEntries<<std::endl;
  
  for(int iEntry=0;iEntry<nEntries;++iEntry){
    if(iEntry%1000==0) std::cout<<"iEntry: "<<iEntry<<std::endl;

    aEvent->SetGeoPtr(0);    
    aTree->GetEntry(iEntry);
    aEvent->SetGeoPtr(myGeometryPtr);
    
    hTotalCharge->Fill(aEvent->GetTotalCharge());
  }

  TCanvas *c1 = new TCanvas("c1","c1", 900, 800);
  hTotalCharge->Draw("hist");
  c1->Print("hTotalCharge.png");
  
  return 0;
}
