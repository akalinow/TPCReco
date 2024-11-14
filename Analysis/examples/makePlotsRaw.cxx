//////////////////////////
//
// root
// root [0] .L makePlotsRaw.cxx
// root [1] makePlotsRaw("RawSignalTree.root");
// root [1] makePlotsRaw("RawSignalTree.root", true, 400, 300e3); // with max_charge (horiz.) and total_charge (vert.) markers
//
// Macro for looking at raw signals properties
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

#include <TFile.h>
#include <TTree.h>
#include <TLatex.h>
#include <TString.h>
#include <TLine.h>
#include <TColor.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TCanvas.h>
#include <TStyle.h>
#include <TLegend.h>
#include <TTreeIndex.h>

#include "TPCReco/RawSignal_tree_dataFormat.h"

TFile *open_TFile(std::string &file_name){
  TFile * tree_file = TFile::Open(file_name.c_str(), "read");
  if(tree_file == 0){
    std::cerr << "ERROR: Cannot open file: " << file_name << std::endl;
  }
  return tree_file;
}

TTree *get_tree(TFile *tree_file, TString key){
  TTree *tree = (TTree*)tree_file->Get(key);
  if(tree == 0){
    std::cerr << "ERROR: TTree pointer is NULL" << std::endl;
  } else {
    std::cout << "TTree '" << key << "' opened" << std::endl;
  }
  return tree;
}

void set_global_style(){
  gStyle->SetHistLineWidth(2);
  //  gStyle->SetOptStat("neuoi");
  gStyle->SetOptStat("e"); // kisourmen
  //  gStyle->SetOptStat(0);
  gStyle->SetPalette(kRainBow);
}

void set_hist_style(TH1D* &hist, std::string xaxis, std::string yaxis, Int_t color, std::string title="DUMMY", bool fill=false){
  hist->GetYaxis()->SetTitleOffset(1.45);
  hist->GetXaxis()->SetTitleOffset(1.25);
  hist->GetXaxis()->SetTitle(xaxis.c_str());
  hist->GetYaxis()->SetTitle(yaxis.c_str());
  hist->SetLineColor(color);
  if(title!="DUMMY") {
    std::cout << "Old title:" << hist->GetTitle() << std::endl;
    hist->SetTitle(title.c_str());
    std::cout << "New title:" << hist->GetTitle() << std::endl;
  }
  if(fill){
    hist->SetFillColor(kGray);
  }
}

void set_hist2D_style(TH2D* &hist, std::string xaxis, std::string yaxis, std::string zaxis, Int_t color, std::string title="DUMMY"){
  hist->GetYaxis()->SetTitleOffset(1.45);
  hist->GetXaxis()->SetTitleOffset(1.25);
  hist->GetXaxis()->SetTitle(xaxis.c_str());
  hist->GetYaxis()->SetTitle(yaxis.c_str());
  hist->GetZaxis()->SetTitle(zaxis.c_str());
  hist->SetLineColor(color);
  hist->SetMarkerColor(color);
  if(title!="DUMMY") {
    std::cout << "Old title:" << hist->GetTitle() << std::endl;
    hist->SetTitle(title.c_str());
    std::cout << "New title:" << hist->GetTitle() << std::endl;
  }
}

int makePlotsRaw(std::string raw_tree_file_name,
		 bool marker_flag=false, float marker_maxCharge=400, float marker_totalCharge=300e3){

  TFile *raw_tree_file = open_TFile(raw_tree_file_name);
  TTree *raw_signal_tree = get_tree(raw_tree_file, "RawSignal");
  auto raw = new Event_rawsignal;
  raw_signal_tree->SetBranchAddress("data", &raw);
  raw_tree_file->cd();
  raw_signal_tree->BuildIndex("runId", "eventId");

  TFile *file = new TFile("Example_raw_signals.root", "RECREATE");
  TCanvas *window = new TCanvas("window", "", 800, 600);
  set_global_style();
  TH1D *totalCharge_all = new TH1D("totalCharge_all", "", 100,-10e5, 4e6);
  TH1D *maxCharge_all = new TH1D("maxCharge_all", "", 100, 0, 6000);
  TH2D *totalCharge_vs_maxCharge = new TH2D("totalCharge_vs_maxCharge", "", 100, -10e5, 4e6, 100, 0, 6e3);
  TH2D *totalCharge_vs_maxCharge_CUT = new TH2D("totalCharge_vs_maxCharge_CUT", "", 100, -10e5, 4e6, 100, 0, 6e3);
  TGraph *line = NULL;
  window->Print("Example_raw_signals.pdf[");

  raw_signal_tree->Draw("totalCharge>>totalCharge_all");
  set_hist_style(totalCharge_all, "Integrated charge [ADC units]", "Events/bin", kBlack, "Integrated charge distribution");
  totalCharge_all->Draw();
  if(marker_flag) {
    line=new TGraph();
    line->SetPoint(line->GetN(), marker_totalCharge, totalCharge_all->GetMinimum());
    line->SetPoint(line->GetN(), marker_totalCharge, totalCharge_all->GetMaximum());
    line->SetLineColor(kMagenta);
    line->SetLineStyle(kDashed);
    line->Draw("L SAME");
  }
  window->Print("Example_raw_signals.pdf");
  file->cd(); window->Write();

  raw_signal_tree->Draw("maxCharge>>maxCharge_all");
  set_hist_style(maxCharge_all, "Maximal charge [ADC units]", "Events/bin", kBlack, "Maximal charge distribution");
  maxCharge_all->Draw();
  if(marker_flag) {
    line=new TGraph();
    line->SetPoint(line->GetN(), marker_maxCharge, maxCharge_all->GetMinimum());
    line->SetPoint(line->GetN(), marker_maxCharge, maxCharge_all->GetMaximum());
    line->SetLineColor(kMagenta);
    line->SetLineStyle(kDashed);
    line->Draw("L SAME");
  }
  window->Print("Example_raw_signals.pdf");
  file->cd(); window->Write();

  raw_signal_tree->Draw("maxCharge:totalCharge>>totalCharge_vs_maxCharge");
  set_hist2D_style(totalCharge_vs_maxCharge, "Integrated charge [ADC units]", "Maximal charge [ADC units]", "Events/bin", 0, "Maximal charge vs Integrated charge");
  gPad->SetRightMargin(0.15);
  totalCharge_vs_maxCharge->Draw("colz");
  if(marker_flag) {
    line=new TGraph();
    line->SetPoint(line->GetN(), marker_totalCharge, totalCharge_vs_maxCharge->GetYaxis()->GetXmin());
    line->SetPoint(line->GetN(), marker_totalCharge, totalCharge_vs_maxCharge->GetYaxis()->GetXmax());
    line->SetLineColor(kMagenta);
    line->SetLineStyle(kDashed);
    line->Draw("L SAME");
    line=new TGraph();
    line->SetPoint(line->GetN(), totalCharge_vs_maxCharge->GetXaxis()->GetXmin(), marker_maxCharge); 
    line->SetPoint(line->GetN(), totalCharge_vs_maxCharge->GetXaxis()->GetXmax(), marker_maxCharge);
    line->SetLineColor(kMagenta);
    line->SetLineStyle(kDashed);
    line->Draw("L SAME");
  }
  window->Print("Example_raw_signals.pdf");
  file->cd(); window->Write();

  // same after zooming into upper-right quadrant
  if(marker_flag) {
    raw_signal_tree->Draw("maxCharge:totalCharge>>totalCharge_vs_maxCharge_CUT",Form("maxCharge>%le && totalCharge>%le", marker_maxCharge, marker_totalCharge));
    set_hist2D_style(totalCharge_vs_maxCharge_CUT, "Integrated charge [ADC units]", "Maximal charge [ADC units]", "Events/bin", 0, "Maximal charge vs Integrated charge (with cut)");
    gPad->SetRightMargin(0.15);
    totalCharge_vs_maxCharge_CUT->Draw("colz");
    line=new TGraph();
    line->SetPoint(line->GetN(), marker_totalCharge, totalCharge_vs_maxCharge->GetYaxis()->GetXmin());
    line->SetPoint(line->GetN(), marker_totalCharge, totalCharge_vs_maxCharge->GetYaxis()->GetXmax());
    line->SetLineColor(kMagenta);
    line->SetLineStyle(kDashed);
    line->Draw("L SAME");
    line=new TGraph();
    line->SetPoint(line->GetN(), totalCharge_vs_maxCharge->GetXaxis()->GetXmin(), marker_maxCharge); 
    line->SetPoint(line->GetN(), totalCharge_vs_maxCharge->GetXaxis()->GetXmax(), marker_maxCharge);
    line->SetLineColor(kMagenta);
    line->SetLineStyle(kDashed);
    line->Draw("L SAME");
    //    totalCharge_vs_maxCharge_CUT->GetXaxis()->SetRangeUser(marker_totalCharge, totalCharge_vs_maxCharge->GetXaxis()->GetXmax());
    //    totalCharge_vs_maxCharge_CUT->GetYaxis()->SetRangeUser(marker_maxCharge, totalCharge_vs_maxCharge->GetYaxis()->GetXmax());
    //    totalCharge_vs_maxCharge_CUT->Draw("colz");
  }
  window->Print("Example_raw_signals.pdf");
  file->cd(); window->Write();

  window->Print("Example_raw_signals.pdf]");
  file->Close();
  return 0;
}
