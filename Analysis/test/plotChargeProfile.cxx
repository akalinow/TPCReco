//////////////////////////
//
// root
// root [0] .L plotChargeProfile
// root [1] plotChargeProfile("Trees.root");
//
//
//////////////////////////
//////////////////////////

#ifndef __ROOTLOGON__
R__ADD_INCLUDE_PATH(../../DataFormats/include)
R__ADD_INCLUDE_PATH(../../Analysis/include)
R__ADD_LIBRARY_PATH(../lib)
#endif

#include "TFile.h"
#include "TTree.h"
#include "TLatex.h"
#include "TString.h"

#include "HIGS_trees_dataFormat.h"

void plotChargeProfile(const char *fname) {

  if (!gROOT->GetClass("TrackSegment3D")){
     R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
 
  TFile *f = TFile::Open(fname, "OLD");
  if(!f) {
    std::cout << "Cannot open ROOT file: " << fname << std::endl;
    return;
  }
  f->ls();

  auto *Tree_1prong_events = static_cast<TTree*>(f->Get("Tree_1prong_events"));
  auto *Tree_2prong_events = (TTree*)(f->Get("Tree_2prong_events"));
  auto *Tree_3prong_events = (TTree*)(f->Get("Tree_3prong_events"));

  std::cout << Tree_1prong_events << std::endl;
  std::cout << Tree_2prong_events << std::endl;
  std::cout << Tree_3prong_events << std::endl;
  if(!Tree_1prong_events || !Tree_2prong_events || !Tree_3prong_events) {
    std::cout << "Some of TTree pointers are empty" << std::endl;
    return;
  }
  
  auto event2=new Event_2prong;
  auto event1=new Event_1prong;
  auto event3=new Event_3prong;  
  Tree_1prong_events->SetBranchAddress("data", &event1);
  Tree_2prong_events->SetBranchAddress("data", &event2);
  Tree_3prong_events->SetBranchAddress("data", &event3);

  TCanvas *c=new TCanvas("c","alpha+C", 1200, 600);
  TLatex tl;

  c->Print("chargeProfile.pdf[");
  for(auto i=0; i<Tree_2prong_events->GetEntries(); i++) {
    c->Clear();
    c->Divide(2,1);
    TH1F *h = NULL;
    //    if(event2->alpha_length<1.0) continue;
    Tree_2prong_events->GetEntry(i);
    std::cout << event2->eventId << std::endl;
    c->cd(1);
    h=(TH1F*)(event2->carbon_chargeProfile.DrawClone());
    h->SetName("hCarbon");
    //    tl.DrawTextNDC(0.1, 0.9, Form("run %ld / evt %u", event2->runId, event2->eventId));
    gPad->Update();
    c->cd(2);
    h=(TH1F*)(event2->alpha_chargeProfile.DrawClone());
    h->SetName("hAlpha");
    //    tl.DrawTextNDC(0.1, 0.9, Form("run %ld / evt %u", event2->runId, event2->eventId));
    //    usleep(1000000);
    gPad->Update();
    c->Print("chargeProfile.pdf");
  }
  c->Print("chargeProfile.pdf]");
  
}
