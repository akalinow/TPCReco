#include <iostream>
#include <cstdlib>
#include <string>
#include <memory>

#include "TPCReco/UtilsTPC.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/UVWprojector.h"
#include "TPCReco/EventTPC.h"

#include <TFile.h>
#include <TTree.h>
#include <TChain.h>
#include <TCanvas.h>
#include <TH1D.h>

int main(int argc, char *argv[]) {

  ///Initialize projections
  std::string geomFileName = "/home/akalinow/data/neutrons/ROOT/geometry_mini_eTPC_2018-06-19T11:18:25.056.dat";
  std::shared_ptr<GeometryTPC> myGeometryPtr = std::make_shared<GeometryTPC>(geomFileName.c_str());
  
  std::string dataFileName = "/home/akalinow/data/neutrons/ROOT/EventTPC_2018-06-19T11:18:25.056*root";

  ///Initialize chain of ROOT files.
  TChain aChain("TPCData");
  aChain.Add(dataFileName.c_str());
  int nEntries = aChain.GetEntries();
  std::cout<<"Total events in the chain: "<<nEntries<<std::endl;

  ///Book histograms. Use smart pointers for automatic memory cleaning.
  std::shared_ptr<TH1D> hTotalCharge(new TH1D("hTotalCharge","Total charge;Charge[arb. units];Entries",100,0,500000));
  hTotalCharge->SetAxisRange(-100,1E8);//Draw under- and overflow bins

  ///Set the address of persistent object.
  EventTPC *aEvent = 0;
  aChain.SetBranchAddress("Event", &aEvent);

  ///Eneble multithreading (a simplest version).
  int nthreads = 4;
  ROOT::EnableImplicitMT(nthreads);

  ///Loop over all events and fill histograms.
  for(int iEntry=0;iEntry<nEntries;++iEntry){
    if(iEntry%1000==0) std::cout<<"iEntry: "<<iEntry<<std::endl;

    aEvent->SetGeoPtr(0);    
    aChain.GetEntry(iEntry);
    aEvent->SetGeoPtr(myGeometryPtr);
    
    hTotalCharge->Fill(aEvent->GetTotalCharge());
  }

  ///Plot histograms.
  TCanvas *c1 = new TCanvas("c1","c1", 900, 800);
  hTotalCharge->Draw("hist");
  c1->Print("hTotalCharge.png");
  
  return 0;
}
