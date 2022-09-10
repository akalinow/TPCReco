//////////////////////////
//
// root
// root [0] .L makePlotsRawVsReco.cxx
// root [1] makePlotsRawVsReco("Trees.root", "RawSignalTree.root");
//
// Macro for correlating the raw signals properties with automatic/manual track reconstruction
//
//////////////////////////
//////////////////////////

#ifndef __ROOTLOGON__
R__ADD_INCLUDE_PATH(../../DataFormats/include)
R__ADD_INCLUDE_PATH(../../Analysis/include)
R__ADD_LIBRARY_PATH(../lib)
#endif

#include <iostream>
#include <string>

#include "TFile.h"
#include "TTree.h"
#include "TLatex.h"
#include "TString.h"
#include "TLine.h"
#include "TColor.h"
#include "TH1D.h"
#include "TH2D.h"
#include "TCanvas.h"
#include "TStyle.h"
#include "TLegend.h"
#include "TTreeIndex.h"

#include "HIGS_trees_dataFormat.h"
#include "RawSignal_tree_dataFormat.h"

TFile *open_TFile(std::string &file_name){
  TFile * tree_file = TFile::Open(file_name.c_str(), "read");
  if(!tree_file){
    std::cerr << "ERROR: Cannot open file:" << file_name << std::endl;
  }
  return tree_file;
}

TTree *get_tree(TFile *tree_file, TString key){
  TTree *tree = (TTree*)tree_file->Get(key);
  if(!tree){
    std::cerr << "ERROR: TTree pointer is NULL" << std::endl;
  } else {
    std::cout << "TTree '" << key << "' opened" << std::endl;
  }
  return tree;
}

void set_global_style(){
  gStyle->SetHistLineWidth(2);
  //  gStyle->SetOptStat("neuoi");
  gStyle->SetOptStat(0);
  gStyle->SetPalette(kRainBow);
}

void set_hist_style(TH1D* &hist, std::string xaxis, string yaxis, Int_t color, std::string title="", bool fill=false){
  hist->GetYaxis()->SetTitleOffset(1.45);
  hist->GetXaxis()->SetTitleOffset(1.25);
  hist->GetXaxis()->SetTitle(xaxis.c_str());
  hist->GetYaxis()->SetTitle(yaxis.c_str());
  hist->SetLineColor(color);
  hist->SetTitle(title.c_str());
  if(fill){
    hist->SetFillColor(kGray);
  }
}

void set_hist2D_style(TH2D* &hist, std::string xaxis, std::string yaxis, std::string zaxis, Int_t color, std::string title=""){
  hist->GetYaxis()->SetTitleOffset(1.45);
  hist->GetXaxis()->SetTitleOffset(1.25);
  hist->GetXaxis()->SetTitle(xaxis.c_str());
  hist->GetYaxis()->SetTitle(yaxis.c_str());
  hist->GetZaxis()->SetTitle(yaxis.c_str());
  hist->SetLineColor(color);
  hist->SetMarkerColor(color);
  hist->SetTitle(title.c_str());
}

int makePlotsRawVsReco(std::string reco_tree_file_name, std::string raw_tree_file_name){

  const Int_t color_1prong = kBlue+1;
  const Int_t color_2prong = kRed;
  const Int_t color_3prong = kGreen+2;
  const Int_t color_all = kBlack;

  TFile *reco_tree_file = open_TFile(reco_tree_file_name);
  TTree *reco_tree_1prong = get_tree(reco_tree_file, "Tree_1prong_events");
  TTree *reco_tree_2prong = get_tree(reco_tree_file, "Tree_2prong_events");
  TTree *reco_tree_3prong = get_tree(reco_tree_file, "Tree_3prong_events");
  auto event1 = new Event_1prong;
  auto event2 = new Event_2prong;
  auto event3 = new Event_3prong;
  reco_tree_1prong->SetBranchAddress("data", &event1);
  reco_tree_2prong->SetBranchAddress("data", &event2);
  reco_tree_3prong->SetBranchAddress("data", &event3);
  reco_tree_file->cd();
  reco_tree_1prong->BuildIndex("runId", "eventId");
  reco_tree_2prong->BuildIndex("runId", "eventId");
  reco_tree_3prong->BuildIndex("runId", "eventId");

  TFile *raw_tree_file = open_TFile(raw_tree_file_name);
  TTree *raw_signal_tree = get_tree(raw_tree_file, "RawSignal");
  auto raw = new Event_rawsignal;
  raw_signal_tree->SetBranchAddress("data", &raw);
  raw_tree_file->cd();
  raw_signal_tree->BuildIndex("runId", "eventId");

  TCanvas *window = new TCanvas("window", "", 800, 600);
  set_global_style();
  TH1D *totalCharge_all = new TH1D("totalCharge_all", "Integrated charge (RAW all);Integrated charge [ADC units]", 100,-10e5, 4e6);
  TH1D *totalCharge_all_clicked = new TH1D("totalCharge_all_clicked", "Integrated charge (RECO all);Integrated charge [ADC units]", 100,-10e5, 4e6);
  TH1D *totalCharge_1prong = new TH1D("totalCharge_1prong", "Integrated charge (RECO 1-prong);Integrated charge [ADC units]", 100,-10e5, 4e6);
  TH1D *totalCharge_2prong = new TH1D("totalCharge_2prong", "Integrated charge (RECO 2-prong);Integrated charge [ADC units]", 100,-10e5, 4e6);
  TH1D *totalCharge_3prong = new TH1D("totalCharge_3prong", "Integrated charge (RECO 3-prong);Integrated charge [ADC units]", 100,-10e5, 4e6);

  TH1D *maxCharge_all = new TH1D("maxCharge_all", "Maximal charge (RAW all);Maximal charge [ADC units]", 100, 0, 6000);
  TH1D *maxCharge_all_clicked = new TH1D("maxCharge_all_clicked", "Maximal charge (RECO all);Maximal charge [ADC units]", 100, 0, 6000);
  TH1D *maxCharge_1prong = new TH1D("maxCharge_1prong", "Maximal charge (RECO 1-prong);Maximal charge [ADC units]", 100, 0, 6000);
  TH1D *maxCharge_2prong = new TH1D("maxCharge_2prong", "Maximal charge (RECO 2-prong);Maximal charge [ADC units]", 100, 0, 6000);
  TH1D *maxCharge_3prong = new TH1D("maxCharge_3prong", "Maximal charge (RECO 3-prong);Maximal charge [ADC units]", 100, 0, 6000);

  TH2D *totalCharge_vs_maxCharge_1prong = new TH2D("totalCharge_vs_maxCharge_1prong", "Integrated vs Maximal charge (RECO 1-prong);Integrated charge [ADC units];Maximal charge [ADC units]", 100, -10e5, 4e6, 100, 0, 6e3);
  TH2D *totalCharge_vs_maxCharge_2prong = new TH2D("totalCharge_vs_maxCharge_2prong", "Integrated vs Maximal charge (RECO 2-prong);Integrated charge [ADC units];Maximal charge [ADC units]", 100, -10e5, 4e6, 100, 0, 6e3);
  TH2D *totalCharge_vs_maxCharge_3prong = new TH2D("totalCharge_vs_maxCharge_3prong", "Integrated vs Maximal charge (RECO 3-prong);Integrated charge [ADC units];Maximal charge [ADC units]", 100, -10e5, 4e6, 100, 0, 6e3);

  TH2D *totalCharge_vs_length_2prong = new TH2D("totalCharge_vs_length_2prong", "Integrated vs Sum of track lengths (RECO 2-prong);Sum of track lenghts [mm];Integrated charge [ADC units]", 100, 0, 200, 100,-10e5, 4e6);

  // loop over 1prong tree
  for(auto i=0; i<reco_tree_1prong->GetEntries(); i++){
    reco_tree_1prong->GetEntry(i);
    auto rawIndex = raw_signal_tree->GetEntryNumberWithBestIndex(event1->runId, event1->eventId);
    if(rawIndex>-1){
      raw_signal_tree->GetEntry(rawIndex);
      totalCharge_1prong->Fill(raw->totalCharge);
      maxCharge_1prong->Fill(raw->maxCharge);
      totalCharge_vs_maxCharge_1prong->Fill(raw->totalCharge, raw->maxCharge);
      maxCharge_all_clicked->Fill(raw->maxCharge);
      totalCharge_all_clicked->Fill(raw->totalCharge);
    }
  }

  // loop over 2prong tree
  for(auto i=0; i<reco_tree_2prong->GetEntries(); i++){
    reco_tree_2prong->GetEntry(i);
    auto rawIndex = raw_signal_tree->GetEntryNumberWithBestIndex(event2->runId, event2->eventId);
    if(rawIndex>-1){
      raw_signal_tree->GetEntry(rawIndex);
      totalCharge_2prong->Fill(raw->totalCharge);
      maxCharge_2prong->Fill(raw->maxCharge);
      totalCharge_vs_length_2prong->Fill((event2->alpha_length+event2->carbon_length), raw->totalCharge);
      totalCharge_vs_maxCharge_2prong->Fill(raw->totalCharge, raw->maxCharge);
      maxCharge_all_clicked->Fill(raw->maxCharge);
      totalCharge_all_clicked->Fill(raw->totalCharge);
    }
  }

  // loop over 3prong tree
  for(auto i=0; i<reco_tree_3prong->GetEntries(); i++){
    reco_tree_3prong->GetEntry(i);
    auto rawIndex = raw_signal_tree->GetEntryNumberWithBestIndex(event3->runId, event3->eventId);
    if(rawIndex>-1){
      raw_signal_tree->GetEntry(rawIndex);
      totalCharge_3prong->Fill(raw->totalCharge);
      maxCharge_3prong->Fill(raw->maxCharge);
      totalCharge_vs_maxCharge_3prong->Fill(raw->totalCharge, raw->maxCharge);
      maxCharge_all_clicked->Fill(raw->maxCharge);
      totalCharge_all_clicked->Fill(raw->totalCharge);
    }
  }
  window->Print("Example_correlation.pdf[");

  raw_signal_tree->Draw("totalCharge>>totalCharge_all");
  set_hist_style(totalCharge_all, "Integrated charge [ADC unts]", "Events/bin", 0, "Total charge distribution", 1);
  totalCharge_all->Draw();
  set_hist_style(totalCharge_all_clicked, "Integrated charge [ADC units]", "Events/bin", color_all, "Integrated charge distribution");
  totalCharge_all_clicked->Draw("same");
  set_hist_style(totalCharge_1prong, "Integrated charge [ADC units]", "Events/bin", color_1prong);
  totalCharge_1prong->Draw("same");
  set_hist_style(totalCharge_2prong, "Integrated charge [ADC units]", "Events/bin", color_2prong);
  totalCharge_2prong->Draw("same");
  set_hist_style(totalCharge_3prong, "Integrated charge [ADC units]", "Events/bin", color_3prong);
  totalCharge_3prong->Draw("same");

  TLegend *legend = new TLegend(0.7,0.75,0.9,0.9,NULL,"brNDC");
  legend->AddEntry(totalCharge_all, "Raw all", "f");
  legend->AddEntry(totalCharge_all_clicked, "Reco all", "l");
  legend->AddEntry(totalCharge_1prong, "Reco 1-prong", "l");
  legend->AddEntry(totalCharge_2prong, "Reco 2-prong", "l");
  legend->AddEntry(totalCharge_3prong, "Reco 3-prong", "l");
  legend->Draw("same");
  window->Print("Example_correlation.pdf");

  raw_signal_tree->Draw("maxCharge>>maxCharge_all");
  set_hist_style(maxCharge_all, "Maximal charge [ADC units]", "Events/bin", 0, "Maximal charge distribution", 1);
  maxCharge_all->Draw();
  set_hist_style(maxCharge_all_clicked, "Maximal charge [ADC units]", "Events/bin", color_all, "Maximal charge distribution");
  maxCharge_all_clicked->Draw("same");
  set_hist_style(maxCharge_1prong, "Maximal charge [ADC units]", "Events/bin", color_1prong);
  maxCharge_1prong->Draw("same");
  set_hist_style(maxCharge_2prong, "Maximal charge [ADC units]", "Events/bin", color_2prong);
  maxCharge_2prong->Draw("same");
  set_hist_style(maxCharge_3prong, "Maximal charge [ADC units]", "Events/bin", color_3prong);
  maxCharge_3prong->Draw("same");
  legend->Draw("same");
  window->Print("Example_correlation.pdf");

  set_hist2D_style(totalCharge_vs_maxCharge_1prong, "Integrated charge [ADC units]", "Maximal charge [ADC units]", "Events/bin", color_1prong, "Maximal charge vs integrated charge");
  totalCharge_vs_maxCharge_1prong->Draw("box");
  set_hist2D_style(totalCharge_vs_maxCharge_2prong, "Integrated charge [ADC units]", "Maximal charge [ADC units]", "Events/bin", color_2prong);
  totalCharge_vs_maxCharge_2prong->Draw("same box");
  set_hist2D_style(totalCharge_vs_maxCharge_3prong, "INtegrated charge [ADC units]", "Maximal charge [ADC units]", "Events/bin", color_3prong);
  totalCharge_vs_maxCharge_3prong->Draw("same box");
  //  gPad->SetRightMargin(0.15);
  TLegend *legend2 = new TLegend(0.7,0.81,0.9,0.9,NULL,"brNDC");
  legend2->AddEntry(totalCharge_1prong, "Reco 1-prong", "l");
  legend2->AddEntry(totalCharge_2prong, "Reco 2-prong", "l");
  legend2->AddEntry(totalCharge_3prong, "Reco 3-prong", "l");
  legend2->Draw("same");
  window->Print("Example_correlation.pdf");

  set_hist2D_style(totalCharge_vs_length_2prong, "#alpha+C length [mm]", "Integrated charge [ADC units]", "Events/bin", color_2prong, "Integrated charge vs #alpha+C length");
  totalCharge_vs_length_2prong->Draw("colz");
  gPad->SetRightMargin(0.15);
  window->Print("Example_correlation.pdf");

  window->Print("Example_correlation.pdf]");
  return 0;
}
