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
#include <TMath.h>

#include "TPCReco/HIGS_trees_dataFormat.h"
#include "TPCReco/RawSignal_tree_dataFormat.h"

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
  hist->SetMarkerSize(20);
  hist->SetTitle(title.c_str());
}

int makePlotsRawVsReco(std::string reco_tree_file_name, std::string raw_tree_file_name, bool qualityCheck_flag=false){

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

  TFile *file = new TFile("Example_correlation.root", "RECREATE");
  TCanvas *window = new TCanvas("window", "", 800, 600);
  set_global_style();
  TH1D *totalCharge_all = new TH1D("totalCharge_all", "Integrated charge (RAW all);Integrated charge [ADC units]", 100,-10e5, 4e6);
  TH1D *totalCharge_all_clicked = new TH1D("totalCharge_all_clicked", "Integrated charge (RECO all);Integrated charge [ADC units]", 100,-10e5, 4e6);
  TH1D *totalCharge_1prong = new TH1D("totalCharge_1prong", "Integrated charge (RECO 1-prong);Integrated charge [ADC units]", 100,-10e5, 4e6);
  TH1D *totalCharge_2prong = new TH1D("totalCharge_2prong", "Integrated charge (RECO 2-prong);Integrated charge [ADC units]", 100,-10e5, 4e6);
  TH1D *totalCharge_3prong = new TH1D("totalCharge_3prong", "Integrated charge (RECO 3-prong);Integrated charge [ADC units]", 100,-10e5, 4e6);

  TH1D *maxCharge_all = new TH1D("maxCharge_all", "Maximal charge (RAW all);Maximal charge [ADC units]", 100, 0, 4096);
  TH1D *maxCharge_all_clicked = new TH1D("maxCharge_all_clicked", "Maximal charge (RECO all);Maximal charge [ADC units]", 100, 0, 4096);
  TH1D *maxCharge_1prong = new TH1D("maxCharge_1prong", "Maximal charge (RECO 1-prong);Maximal charge [ADC units]", 100, 0, 4096);
  TH1D *maxCharge_2prong = new TH1D("maxCharge_2prong", "Maximal charge (RECO 2-prong);Maximal charge [ADC units]", 100, 0, 4096);
  TH1D *maxCharge_3prong = new TH1D("maxCharge_3prong", "Maximal charge (RECO 3-prong);Maximal charge [ADC units]", 100, 0, 4096);

  TH2D *totalCharge_vs_maxCharge_1prong = new TH2D("totalCharge_vs_maxCharge_1prong", "Integrated vs Maximal charge (RECO 1-prong);Integrated charge [ADC units];Maximal charge [ADC units]", 100, -10e5, 4e6, 100, 0, 4096);
  TH2D *totalCharge_vs_maxCharge_2prong = new TH2D("totalCharge_vs_maxCharge_2prong", "Integrated vs Maximal charge (RECO 2-prong);Integrated charge [ADC units];Maximal charge [ADC units]", 100, -10e5, 4e6, 100, 0, 4096);
  TH2D *totalCharge_vs_maxCharge_3prong = new TH2D("totalCharge_vs_maxCharge_3prong", "Integrated vs Maximal charge (RECO 3-prong);Integrated charge [ADC units];Maximal charge [ADC units]", 100, -10e5, 4e6, 100, 0, 4096);

  TH2D *totalCharge_vs_length_1prong = new TH2D("totalCharge_vs_length_1prong", "Integrated vs track length (RECO 1-prong);Track lenght [mm];Integrated charge [ADC units];Events/bin", 330/2, 0, 330, 100,-10e5, 4e6);
  TH2D *totalCharge_vs_length_2prong = new TH2D("totalCharge_vs_length_2prong", "Integrated vs Sum of track lengths (RECO 2-prong);Sum of track lenghts [mm];Integrated charge [ADC units];Events/bin", 200/2, 0, 200, 100,-10e5, 4e6);
  TH2D *totalCharge_vs_length_3prong = new TH2D("totalCharge_vs_length_3prong", "Integrated vs Sum of track lengths (RECO 3-prong);Sum of track lenghts [mm];Integrated charge [ADC units];Events/bin", 200/2, 0, 200, 100,-10e5, 4e6);

  TH1D *nHits_all = new TH1D("nHits_all", "Number of hits (RAW all);Number of hits", 100, 0, 10e3);
  TH1D *nHits_all_clicked = new TH1D("nHits_all_clicked", "Number of hits (RECO all);Number of hits", 100, 0, 10e3);
  TH1D *nHits_1prong = new TH1D("nHits_1prong", "Number of hits (RECO 1-prong);Number of hits", 100, 0, 10e3);
  TH1D *nHits_2prong = new TH1D("nHits_2prong", "Number of hits (RECO 2-prong);Number of hits", 100, 0, 10e3);
  TH1D *nHits_3prong = new TH1D("nHits_3prong", "Number of hits (RECO 3-prong);Number of hits", 100, 0, 10e3);

  TH1D *pseudoLength_all = new TH1D("pseudoLength_all", "Pseudo length (RAW all);Pseudo length [mm]", 100, 0, 200);
  TH1D *pseudoLength_all_clicked = new TH1D("pseudoLength_all_clicked", "Pseudo length (RECO all);Pseudo length [mm]", 100, 0, 200);
  TH1D *pseudoLength_1prong = new TH1D("pseudoLength_1prong", "Pseudo length (RECO 1-prong);Pseudo length [mm]", 100, 0, 200);
  TH1D *pseudoLength_2prong = new TH1D("pseudoLength_2prong", "Pseudo length (RECO 2-prong);Pseudo lenght [mm]", 100, 0, 200);
  TH1D *pseudoLength_3prong = new TH1D("pseudoLength_3prong", "Pseudo length (RECO 3-prong);Pseudo lenght [mm]", 100, 0, 200);

  TH1D *pseudoDensity_all = new TH1D("pseudoDensity_all", "Pseudo charge density (RAW all);Psuedo charge density [ADC units / mm]", 100, 0, 10e3);
  TH1D *pseudoDensity_all_clicked = new TH1D("pseudoDensity_all_clicked", "Pseudo charge density (RECO all);Pseudo charge density [ADC units  mm]", 100, 0, 10e3);
  TH1D *pseudoDensity_1prong = new TH1D("pseudoDensity_1prong", "Psuedo charge density (RECO 1-prong);Psuedo charge density [ADC units / mm]", 100, 0, 10e3);
  TH1D *pseudoDensity_2prong = new TH1D("pseudoDensity_2prong", "Psuedo charge density (RECO 2-prong);Pseudo charge density [ADC units / mm]", 100, 0, 10e3);
  TH1D *pseudoDensity_3prong = new TH1D("pseudoDensity_3prong", "Psuedo charge density (RECO 3-prong);Pseudo lenght [ADC units / mm]", 100, 0, 10e3);

  TH1D *chargeRatio_all = new TH1D("chargeRatio_all", "Ratio totalCharge/maxCharge (RAW all);Ratio totalCharge/maxCharge", 100, 0, 500);
  TH1D *chargeRatio_all_clicked = new TH1D("chargeRatio_all_clicked", "Ratio totalCharge/maxCharge (RECO all);Ratio totalCharge/maxCharge", 100, 0, 500);
  TH1D *chargeRatio_1prong = new TH1D("chargeRatio_1prong", "Ratio totalCharge/maxCharge (RECO 1-prong);Ratio totalCharge/maxCharge", 100, 0, 500);
  TH1D *chargeRatio_2prong = new TH1D("chargeRatio_2prong", "Ratio totalCharge/maxCharge (RECO 2-prong);Ratio totalCharge/maxCharge", 100, 0, 500);
  TH1D *chargeRatio_3prong = new TH1D("chargeRatio_3prong", "Ratio totalCharge/maxCharge (RECO 3-prong);Ratio totalCharge/maxCharge", 100, 0, 500);

  TH1D *reducedMaxCharge_all = new TH1D("reducedMaxCharge_all", "Ratio maxCharge/nHits (RAW all);Ratio maxCharge/nHits", 100, 0, 5);
  TH1D *reducedMaxCharge_all_clicked = new TH1D("reducedMaxCharge_all_clicked", "Ratio maxCharge/nHits (RECO all);Ratio maxCharge/nHits", 100, 0, 5);
  TH1D *reducedMaxCharge_1prong = new TH1D("reducedMaxCharge_1prong", "Ratio maxCharge/nHits (RECO 1-prong);Ratio maxCharge/nHits", 100, 0, 5);
  TH1D *reducedMaxCharge_2prong = new TH1D("reducedMaxCharge_2prong", "Ratio maxCharge/nHits (RECO 2-prong);Ratio maxCharge/nHits", 100, 0, 5);
  TH1D *reducedMaxCharge_3prong = new TH1D("reducedMaxCharge_3prong", "Ratio maxCharge/nHits (RECO 3-prong);Ratio maxCharge/nHits", 100, 0, 5);

  TH1D *reducedTotalCharge_all = new TH1D("reducedTotalCharge_all", "Ratio totalCharge/nHits (RAW all);Ratio totalCharge/nHits", 100, 0, 150);
  TH1D *reducedTotalCharge_all_clicked = new TH1D("reducedTotalCharge_all_clicked", "Ratio totalCharge/nHits (RECO all);Ratio totalCharge/nHits", 100, 0, 150);
  TH1D *reducedTotalCharge_1prong = new TH1D("reducedTotalCharge_1prong", "Ratio totalCharge/nHits (RECO 1-prong);Ratio totalCharge/nHits", 100, 0, 150);
  TH1D *reducedTotalCharge_2prong = new TH1D("reducedTotalCharge_2prong", "Ratio totalCharge/nHits (RECO 2-prong);Ratio totalCharge/nHits", 100, 0, 150);
  TH1D *reducedTotalCharge_3prong = new TH1D("reducedTotalCharge_3prong", "Ratio totalCharge/nHits (RECO 3-prong);Ratio totalCharge/nHits", 100, 0, 150);

  TH2D *maxCharge_vs_nHits_1prong = new TH2D("maxCharge_vs_nHits_1prong", "Maximal charge vs number of hits (RECO 1-prong);Maximal charge [ADC units];Number of hits;Events/bin", 100, 0, 4096, 100, 0, 10e3);
  TH2D *maxCharge_vs_nHits_2prong = new TH2D("maxCharge_vs_nHits_2prong", "Maximal charge vs number of hits (RECO 2-prong);Maximal charge [ADC units];Number of hits;Events/bin", 100, 0, 4096, 100, 0, 10e3);
  TH2D *maxCharge_vs_nHits_3prong = new TH2D("maxCharge_vs_nHits_3prong", "Maximal charge vs number of hits (RECO 3-prong);Maximal charge [ADC units];Number of hits;Events/bin", 100, 0, 4096, 100, 0, 10e3);

  TH2D *pseudoDensity_vs_nHits_1prong = new TH2D("pseudoDensity_vs_nHits_1prong", "Pseudo charge density vs number of hits (RECO 1-prong);Pseudo charge density [ADC units / mm];Number of hits;Events/bin", 100, 0, 10e3, 100, 0, 10e3);
  TH2D *pseudoDensity_vs_nHits_2prong = new TH2D("pseudoDensity_vs_nHits_2prong", "Pseudo charge density vs number of hits (RECO 2-prong);Pseudo charge density [ADC units / mm];Number of hits;Events/bin", 100, 0, 10e3, 100, 0, 10e3);
  TH2D *pseudoDensity_vs_nHits_3prong = new TH2D("pseudoDensity_vs_nHits_3prong", "Pseudo charge density vs number of hits (RECO 3-prong);Pseudo charge density [ADC units / mm];Number of hits;Events/bin", 100, 0, 10e3, 100, 0, 10e3);

  TH2D *pseudoDensity_vs_chargeRatio_1prong = new TH2D("pseudoDensity_vs_chargeRatio_1prong", "Pseudo charge density vs totalCharge/maxCharge ratio (RECO 1-prong);Pseudo charge density [ADC units / mm];Ratio totalCharge/maxCharge;Events/bin", 100, 0, 10e3, 100, 0, 500);
  TH2D *pseudoDensity_vs_chargeRatio_2prong = new TH2D("pseudoDensity_vs_chargeRatio_2prong", "Pseudo charge density vs totalCharge/maxCharge ratio (RECO 2-prong);Pseudo charge density [ADC units / mm];Ratio totalCharge/maxCharge;Events/bin", 100, 0, 10e3, 100, 0, 500);
  TH2D *pseudoDensity_vs_chargeRatio_3prong = new TH2D("pseudoDensity_vs_chargeRatio_3prong", "Pseudo charge density vs totalCharge/maxCharge ratio (RECO 3-prong);Pseudo charge density [ADC units / mm];Ratio totalCharge/maxCharge;Events/bin", 100, 0, 10e3, 100, 0, 500);

  TH1D *horizontalChargeAsymmetry_all = new TH1D("horizontalChargeAsymmetry_all", "Horizontal charge asymmetry (RAW all);Q^[MAX}_{HALF} / pseudo-length [ADC units / mm]", 100, 0, 10e3);
  TH1D *horizontalChargeAsymmetry_all_clicked = new TH1D("horizontalChargeAsymmetry_all_clicked", "Horizontal charge asymmetry (RECO all);Q^[MAX}_{HALF} / pseudo-length [ADC units / mm]", 100, 0, 10e3);
  TH1D *horizontalChargeAsymmetry_1prong = new TH1D("horizontalChargeAsymmetry_1prong", "Horizontal charge asymmetry (RECO 1-prong);Q^[MAX}_{HALF} / pseudo-length [ADC units / mm]", 100, 0, 10e3);
  TH1D *horizontalChargeAsymmetry_2prong = new TH1D("horizontalChargeAsymmetry_2prong", "Horizontal charge asymmetry (RECO 2-prong);Q^[MAX}_{HALF} / pseudo-length [ADC units / mm]", 100, 0, 10e3);
  TH1D *horizontalChargeAsymmetry_3prong = new TH1D("horizontalChargeAsymmetry_3prong", "Horizontal charge asymmetry (RECO 3-prong);Q^[MAX}_{HALF} / pseudo-length [ADC units / mm]", 100, 0, 10e3);

  TH1D *verticalChargeAsymmetry_all = new TH1D("verticalChargeAsymmetry_all", "Vertical charge asymmetry (RAW all);Q^[MAX}_{HALF} / pseudo-length [ADC units / mm]", 100, 0, 10e3);
  TH1D *verticalChargeAsymmetry_all_clicked = new TH1D("verticalChargeAsymmetry_all_clicked", "Vertical charge asymmetry (RECO all);Q^[MAX}_{HALF} / pseudo-length [ADC units / mm]", 100, 0, 10e3);
  TH1D *verticalChargeAsymmetry_1prong = new TH1D("verticalChargeAsymmetry_1prong", "Vertical charge asymmetry (RECO 1-prong);Q^[MAX}_{HALF} / pseudo-length [ADC units / mm]", 100, 0, 10e3);
  TH1D *verticalChargeAsymmetry_2prong = new TH1D("verticalChargeAsymmetry_2prong", "Vertical charge asymmetry (RECO 2-prong);Q^[MAX}_{HALF} / pseudo-length [ADC units / mm]", 100, 0, 10e3);
  TH1D *verticalChargeAsymmetry_3prong = new TH1D("verticalChargeAsymmetry_3prong", "Vertical charge asymmetry (RECO 3-prong);Q^[MAX}_{HALF} / pseudo-length [ADC units / mm]", 100, 0, 10e3);

  TH2D *horizontalChargeAsymmetry_vs_verticalChargeAsymmetry_1prong =
    new TH2D("horizontalChargeAsymmetry_vs_verticalChargeAsymmetry_1prong", "Horizontal vs Vertical charge asymmetry (RECO 1-prong);"
	     "Vertical Q^{MAX}_{HALF}) / pseudo-length [ADC units / mm];"
	     "Horizontal Q^[MAX}_{HALF} / pseudo-length [ADC units / mm];Events/bin",
	     100, 0, 10e3, 100, 0, 10e3);
  TH2D *horizontalChargeAsymmetry_vs_verticalChargeAsymmetry_2prong =
    new TH2D("horizontalChargeAsymmetry_vs_verticalChargeAsymmetry_2prong", "Horizontal vs Vertical charge asymmetry (RECO 2-prong);"
	     "Vertical Q^{MAX}_{HALF}) / pseudo-length [ADC units / mm];"
	     "Horizontal Q^[MAX}_{HALF} / pseudo-length [ADC units / mm];Events/bin",
	     100, 0, 10e3, 100, 0, 10e3);
  TH2D *horizontalChargeAsymmetry_vs_verticalChargeAsymmetry_3prong =
    new TH2D("horizontalChargeAsymmetry_vs_verticalChargeAsymmetry_3prong", "Horizontal vs Vertical charge asymmetry (RECO 3-prong);"
	     "Vertical Q^{MAX}_{HALF}) / pseudo-length [ADC units / mm];"
	     "Horizontal Q^[MAX}_{HALF} / pseudo-length [ADC units / mm];Events/bin",
	     100, 0, 10e3, 100, 0, 10e3);

  // loop over 1-prong tree
  for(auto i=0; i<reco_tree_1prong->GetEntries(); i++){
    reco_tree_1prong->GetEntry(i);
    if(qualityCheck_flag && (!event1->XYmargin5mm || !event1->Zmargin5mm)) continue; // check basic event quality
    // NOTE: GetEntryNumberWithBestIndex reports wrong index when size of reco_tree_1prong is larger that size of raw_signal_tree!
    // auto rawIndex = GetEntryNumberWithBestIndex(event1->runId, event1->eventId);
    auto rawIndex = raw_signal_tree->GetEntryNumberWithIndex(event1->runId, event1->eventId);
    if(rawIndex>-1){
      raw_signal_tree->GetEntry(rawIndex);
      totalCharge_1prong->Fill(raw->totalCharge);
      maxCharge_1prong->Fill(raw->maxCharge);
      totalCharge_vs_maxCharge_1prong->Fill(raw->totalCharge, raw->maxCharge);
      totalCharge_vs_length_1prong->Fill(event1->length, raw->totalCharge);
      maxCharge_all_clicked->Fill(raw->maxCharge);
      totalCharge_all_clicked->Fill(raw->totalCharge);

      double pseudoLength=
	(sqrt(pow(raw->horizontalWidthPerDir[0],2)+pow(raw->verticalWidthPerDir[0],2))+
	 sqrt(pow(raw->horizontalWidthPerDir[1],2)+pow(raw->verticalWidthPerDir[1],2))+
	 sqrt(pow(raw->horizontalWidthPerDir[2],2)+pow(raw->verticalWidthPerDir[2],2)))/3;
      double pseudoDensity=
	(raw->totalChargePerDir[0]/sqrt(pow(raw->horizontalWidthPerDir[0],2)+pow(raw->verticalWidthPerDir[0],2))+
	 raw->totalChargePerDir[1]/sqrt(pow(raw->horizontalWidthPerDir[1],2)+pow(raw->verticalWidthPerDir[1],2))+
	 raw->totalChargePerDir[2]/sqrt(pow(raw->horizontalWidthPerDir[2],2)+pow(raw->verticalWidthPerDir[2],2)))/3;
      nHits_1prong->Fill(raw->nHits);
      nHits_all_clicked->Fill(raw->nHits);
      pseudoLength_1prong->Fill(pseudoLength);
      pseudoLength_all_clicked->Fill(pseudoLength);
      pseudoDensity_1prong->Fill(pseudoDensity);
      pseudoDensity_all_clicked->Fill(pseudoDensity);
      chargeRatio_1prong->Fill(raw->totalCharge/raw->maxCharge);
      chargeRatio_all_clicked->Fill(raw->totalCharge/raw->maxCharge);
      reducedMaxCharge_1prong->Fill(raw->maxCharge/raw->nHits);
      reducedMaxCharge_all_clicked->Fill(raw->maxCharge/raw->nHits);
      reducedTotalCharge_1prong->Fill(raw->totalCharge/raw->nHits);
      reducedTotalCharge_all_clicked->Fill(raw->totalCharge/raw->nHits);
      maxCharge_vs_nHits_1prong->Fill(raw->maxCharge, raw->nHits);
      pseudoDensity_vs_nHits_1prong->Fill(pseudoDensity, raw->nHits);
      pseudoDensity_vs_chargeRatio_1prong->Fill(pseudoDensity, raw->totalCharge/raw->maxCharge);

      for(auto ihalf=0; ihalf<1; ihalf++) {
	double asymmetryH=(raw->horizontalChargePerDirHalf[0][ihalf]+raw->horizontalChargePerDirHalf[1][ihalf]+raw->horizontalChargePerDirHalf[2][ihalf])/pseudoLength;
	double asymmetryV=(raw->verticalChargePerDirHalf[0][ihalf]+raw->verticalChargePerDirHalf[1][ihalf]+raw->verticalChargePerDirHalf[2][ihalf])/pseudoLength;
	horizontalChargeAsymmetry_1prong->Fill(asymmetryH);
	horizontalChargeAsymmetry_all_clicked->Fill(asymmetryH);
	verticalChargeAsymmetry_1prong->Fill(asymmetryV);
	verticalChargeAsymmetry_all_clicked->Fill(asymmetryV);
	horizontalChargeAsymmetry_vs_verticalChargeAsymmetry_1prong->Fill(asymmetryV, asymmetryH);
      }
    }
  }

  // loop over 2-prong tree
  for(auto i=0; i<reco_tree_2prong->GetEntries(); i++){
    reco_tree_2prong->GetEntry(i);
    if(qualityCheck_flag && (!event2->XYmargin5mm || !event2->Zmargin5mm)) continue; // check basic event quality
    // NOTE: GetEntryNumberWithBestIndex reports wrong index when size of reco_tree_2prong is larger that size of raw_signal_tree!
    // auto rawIndex = GetEntryNumberWithBestIndex(event2->runId, event2->eventId);
    auto rawIndex = raw_signal_tree->GetEntryNumberWithIndex(event2->runId, event2->eventId);
    if(rawIndex>-1){
      raw_signal_tree->GetEntry(rawIndex);
      totalCharge_2prong->Fill(raw->totalCharge);
      maxCharge_2prong->Fill(raw->maxCharge);
      totalCharge_vs_length_2prong->Fill((event2->alpha_length+event2->carbon_length), raw->totalCharge);
      totalCharge_vs_maxCharge_2prong->Fill(raw->totalCharge, raw->maxCharge);
      maxCharge_all_clicked->Fill(raw->maxCharge);
      totalCharge_all_clicked->Fill(raw->totalCharge);

      //// DEBUG
      //      if (raw->maxCharge>200 && raw->maxCharge<300) {
      std::cout << "2-prong: reco_entry=" << i << "/" << reco_tree_2prong->GetEntries()-1
		<<", raw_entry=" << rawIndex << "/" << raw_signal_tree->GetEntries()-1
		<<": run=" << event2->runId << ", evt=" << event2->eventId << ", maxQ=" << raw->maxCharge << ", totQ=" << raw->totalCharge << std::endl;
      //      }
      //// DEBUG

      double pseudoLength=
	(sqrt(pow(raw->horizontalWidthPerDir[0],2)+pow(raw->verticalWidthPerDir[0],2))+
	 sqrt(pow(raw->horizontalWidthPerDir[1],2)+pow(raw->verticalWidthPerDir[1],2))+
	 sqrt(pow(raw->horizontalWidthPerDir[2],2)+pow(raw->verticalWidthPerDir[2],2)))/3;
      double pseudoDensity=
	(raw->totalChargePerDir[0]/sqrt(pow(raw->horizontalWidthPerDir[0],2)+pow(raw->verticalWidthPerDir[0],2))+
	 raw->totalChargePerDir[1]/sqrt(pow(raw->horizontalWidthPerDir[1],2)+pow(raw->verticalWidthPerDir[1],2))+
	 raw->totalChargePerDir[2]/sqrt(pow(raw->horizontalWidthPerDir[2],2)+pow(raw->verticalWidthPerDir[2],2)))/3;
      nHits_2prong->Fill(raw->nHits);
      nHits_all_clicked->Fill(raw->nHits);
      pseudoLength_2prong->Fill(pseudoLength);
      pseudoLength_all_clicked->Fill(pseudoLength);
      pseudoDensity_2prong->Fill(pseudoDensity);
      pseudoDensity_all_clicked->Fill(pseudoDensity);
      chargeRatio_2prong->Fill(raw->totalCharge/raw->maxCharge);
      chargeRatio_all_clicked->Fill(raw->totalCharge/raw->maxCharge);
      reducedMaxCharge_2prong->Fill(raw->maxCharge/raw->nHits);
      reducedMaxCharge_all_clicked->Fill(raw->maxCharge/raw->nHits);
      reducedTotalCharge_2prong->Fill(raw->totalCharge/raw->nHits);
      reducedTotalCharge_all_clicked->Fill(raw->totalCharge/raw->nHits);
      maxCharge_vs_nHits_2prong->Fill(raw->maxCharge, raw->nHits);
      pseudoDensity_vs_nHits_2prong->Fill(pseudoDensity, raw->nHits);
      pseudoDensity_vs_chargeRatio_2prong->Fill(pseudoDensity, raw->totalCharge/raw->maxCharge);

      for(auto ihalf=0; ihalf<1; ihalf++) {
	double asymmetryH=(raw->horizontalChargePerDirHalf[0][ihalf]+raw->horizontalChargePerDirHalf[1][ihalf]+raw->horizontalChargePerDirHalf[2][ihalf])/pseudoLength;
	double asymmetryV=(raw->verticalChargePerDirHalf[0][ihalf]+raw->verticalChargePerDirHalf[1][ihalf]+raw->verticalChargePerDirHalf[2][ihalf])/pseudoLength;
	horizontalChargeAsymmetry_2prong->Fill(asymmetryH);
	horizontalChargeAsymmetry_all_clicked->Fill(asymmetryH);
	verticalChargeAsymmetry_2prong->Fill(asymmetryV);
	verticalChargeAsymmetry_all_clicked->Fill(asymmetryV);
	horizontalChargeAsymmetry_vs_verticalChargeAsymmetry_2prong->Fill(asymmetryV, asymmetryH);
      }
    }
  }

  // loop over 3-prong tree
  for(auto i=0; i<reco_tree_3prong->GetEntries(); i++){
    reco_tree_3prong->GetEntry(i);
    if(qualityCheck_flag && (!event3->XYmargin5mm || !event3->Zmargin5mm)) continue; // check basic event quality
    // NOTE: GetEntryNumberWithBestIndex reports wrong index when size of reco_tree_3prong is larger that size of raw_signal_tree!
    // auto rawIndex = GetEntryNumberWithBestIndex(event3->runId, event3->eventId);
    auto rawIndex = raw_signal_tree->GetEntryNumberWithIndex(event3->runId, event3->eventId);
    if(rawIndex>-1){
      raw_signal_tree->GetEntry(rawIndex);
      totalCharge_3prong->Fill(raw->totalCharge);
      maxCharge_3prong->Fill(raw->maxCharge);
      totalCharge_vs_maxCharge_3prong->Fill(raw->totalCharge, raw->maxCharge);
      totalCharge_vs_length_3prong->Fill((event3->alpha1_length+event3->alpha2_length+event3->alpha3_length), raw->totalCharge);
      maxCharge_all_clicked->Fill(raw->maxCharge);
      totalCharge_all_clicked->Fill(raw->totalCharge);

      double pseudoLength=
	(sqrt(pow(raw->horizontalWidthPerDir[0],2)+pow(raw->verticalWidthPerDir[0],2))+
	 sqrt(pow(raw->horizontalWidthPerDir[1],2)+pow(raw->verticalWidthPerDir[1],2))+
	 sqrt(pow(raw->horizontalWidthPerDir[2],2)+pow(raw->verticalWidthPerDir[2],2)))/3;
      double pseudoDensity=
	(raw->totalChargePerDir[0]/sqrt(pow(raw->horizontalWidthPerDir[0],2)+pow(raw->verticalWidthPerDir[0],2))+
	 raw->totalChargePerDir[1]/sqrt(pow(raw->horizontalWidthPerDir[1],2)+pow(raw->verticalWidthPerDir[1],2))+
	 raw->totalChargePerDir[2]/sqrt(pow(raw->horizontalWidthPerDir[2],2)+pow(raw->verticalWidthPerDir[2],2)))/3;
      nHits_3prong->Fill(raw->nHits);
      nHits_all_clicked->Fill(raw->nHits);
      pseudoLength_3prong->Fill(pseudoLength);
      pseudoLength_all_clicked->Fill(pseudoLength);
      pseudoDensity_3prong->Fill(pseudoDensity);
      pseudoDensity_all_clicked->Fill(pseudoDensity);
      chargeRatio_3prong->Fill(raw->totalCharge/raw->maxCharge);
      chargeRatio_all_clicked->Fill(raw->totalCharge/raw->maxCharge);
      reducedMaxCharge_3prong->Fill(raw->maxCharge/raw->nHits);
      reducedMaxCharge_all_clicked->Fill(raw->maxCharge/raw->nHits);
      reducedTotalCharge_3prong->Fill(raw->totalCharge/raw->nHits);
      reducedTotalCharge_all_clicked->Fill(raw->totalCharge/raw->nHits);
      maxCharge_vs_nHits_3prong->Fill(raw->maxCharge, raw->nHits);
      pseudoDensity_vs_nHits_3prong->Fill(pseudoDensity, raw->nHits);
      pseudoDensity_vs_chargeRatio_3prong->Fill(pseudoDensity, raw->totalCharge/raw->maxCharge);

      for(auto ihalf=0; ihalf<1; ihalf++) {
	double asymmetryH=(raw->horizontalChargePerDirHalf[0][ihalf]+raw->horizontalChargePerDirHalf[1][ihalf]+raw->horizontalChargePerDirHalf[2][ihalf])/pseudoLength;
	double asymmetryV=(raw->verticalChargePerDirHalf[0][ihalf]+raw->verticalChargePerDirHalf[1][ihalf]+raw->verticalChargePerDirHalf[2][ihalf])/pseudoLength;
	horizontalChargeAsymmetry_3prong->Fill(asymmetryH);
	horizontalChargeAsymmetry_all_clicked->Fill(asymmetryH);
	verticalChargeAsymmetry_3prong->Fill(asymmetryV);
	verticalChargeAsymmetry_all_clicked->Fill(asymmetryV);
	horizontalChargeAsymmetry_vs_verticalChargeAsymmetry_3prong->Fill(asymmetryV, asymmetryH);
      }
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
  TLegend *legend = NULL;
  legend = new TLegend(0.7,0.75,0.9,0.9,NULL,"brNDC");
  legend->AddEntry(totalCharge_all, "Raw all", "f");
  legend->AddEntry(totalCharge_all_clicked, "Reco all", "l");
  legend->AddEntry(totalCharge_1prong, "Reco 1-prong", "l");
  legend->AddEntry(totalCharge_2prong, "Reco 2-prong", "l");
  legend->AddEntry(totalCharge_3prong, "Reco 3-prong", "l");
  legend->Draw("same");
  window->Print("Example_correlation.pdf");
  file->cd(); window->Write();

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
  legend = new TLegend(0.7,0.75,0.9,0.9,NULL,"brNDC");
  legend->AddEntry(maxCharge_all, "Raw all", "f");
  legend->AddEntry(maxCharge_all_clicked, "Reco all", "l");
  legend->AddEntry(maxCharge_1prong, "Reco 1-prong", "l");
  legend->AddEntry(maxCharge_2prong, "Reco 2-prong", "l");
  legend->AddEntry(maxCharge_3prong, "Reco 3-prong", "l");
  legend->Draw("same");
  window->Print("Example_correlation.pdf");
  file->cd(); window->Write();

  set_hist2D_style(totalCharge_vs_maxCharge_1prong, "Integrated charge [ADC units]", "Maximal charge [ADC units]", "Events/bin", color_1prong, "Maximal charge vs integrated charge");
  totalCharge_vs_maxCharge_1prong->Draw("box"); // "box"
  set_hist2D_style(totalCharge_vs_maxCharge_2prong, "Integrated charge [ADC units]", "Maximal charge [ADC units]", "Events/bin", color_2prong);
  totalCharge_vs_maxCharge_2prong->Draw("same box"); // "same box"
  set_hist2D_style(totalCharge_vs_maxCharge_3prong, "Integrated charge [ADC units]", "Maximal charge [ADC units]", "Events/bin", color_3prong);
  totalCharge_vs_maxCharge_3prong->Draw("same *"); // "same box"
  //  gPad->SetRightMargin(0.15);
  legend = new TLegend(0.7,0.81,0.9,0.9,NULL,"brNDC");
  legend->AddEntry(totalCharge_1prong, "Reco 1-prong", "l");
  legend->AddEntry(totalCharge_2prong, "Reco 2-prong", "l");
  legend->AddEntry(totalCharge_3prong, "Reco 3-prong", "l");
  legend->Draw("same");
  window->Print("Example_correlation.pdf");
  file->cd(); window->Write();

  set_hist2D_style(totalCharge_vs_length_1prong, "Track length [mm]", "Integrated charge [ADC units]", "Events/bin", color_1prong, "Integrated charge vs track length (Reco 1-prong)");
  totalCharge_vs_length_1prong->Draw("colz");
  gPad->SetRightMargin(0.15);
  window->Print("Example_correlation.pdf");
  file->cd(); window->Write();

  set_hist2D_style(totalCharge_vs_length_2prong, "#alpha+C length [mm]", "Integrated charge [ADC units]", "Events/bin", color_2prong, "Integrated charge vs #alpha+C length (Reco 2-prong)");
  totalCharge_vs_length_2prong->Draw("colz");
  gPad->SetRightMargin(0.15);
  window->Print("Example_correlation.pdf");
  file->cd(); window->Write();

  set_hist2D_style(totalCharge_vs_length_3prong, "#alpha_{1}+#alpha_{2}+#alpha_{3} length [mm]", "Integrated charge [ADC units]", "Events/bin", color_2prong, "Integrated charge vs #alpha_{1}+#alpha_{2}+#alpha_{3} length (Reco 3-prong)");
  totalCharge_vs_length_3prong->Draw("colz");
  gPad->SetRightMargin(0.15);
  window->Print("Example_correlation.pdf");
  file->cd(); window->Write();

  set_hist2D_style(totalCharge_vs_length_1prong, "Track length sum [mm]", "Integrated charge [ADC units]", "Events/bin", color_1prong, "Integrated charge vs track length sum");
  totalCharge_vs_length_1prong->Draw("box"); // "box"
  set_hist2D_style(totalCharge_vs_length_2prong, "Track length sum [mm]", "Integrated charge [ADC units]", "Events/bin", color_2prong);
  totalCharge_vs_length_2prong->Draw("same box"); // "same box"
  set_hist2D_style(totalCharge_vs_length_3prong, "Track length sum [mm]", "Integrated charge [ADC units]", "Events/bin", color_3prong);
  totalCharge_vs_length_3prong->Draw("same box"); // "same box"
  //  gPad->SetRightMargin(0.15);
  legend = new TLegend(0.7,0.81,0.9,0.9,NULL,"brNDC");
  legend->AddEntry(totalCharge_vs_length_1prong, "Reco 1-prong", "l");
  legend->AddEntry(totalCharge_vs_length_2prong, "Reco 2-prong", "l");
  legend->AddEntry(totalCharge_vs_length_3prong, "Reco 3-prong", "l");
  legend->Draw("same");
  window->Print("Example_correlation.pdf");
  file->cd(); window->Write();

  raw_signal_tree->Draw("nHits>>nHits_all");
  set_hist_style(nHits_all, "Number of hits", "Events/bin", 0, "Number of hits distribution", 1);
  nHits_all->Draw();
  set_hist_style(nHits_all_clicked, "Number of hits", "Events/bin", color_all, "Number of hits distribution");
  nHits_all_clicked->Draw("same");
  set_hist_style(nHits_1prong, "Number of hits", "Events/bin", color_1prong);
  nHits_1prong->Draw("same");
  set_hist_style(nHits_2prong, "Number of hits", "Events/bin", color_2prong);
  nHits_2prong->Draw("same");
  set_hist_style(nHits_3prong, "Number of hits", "Events/bin", color_3prong);
  nHits_3prong->Draw("same");
  legend = new TLegend(0.7,0.75,0.9,0.9,NULL,"brNDC");
  legend->AddEntry(nHits_all, "Raw all", "f");
  legend->AddEntry(nHits_all_clicked, "Reco all", "l");
  legend->AddEntry(nHits_1prong, "Reco 1-prong", "l");
  legend->AddEntry(nHits_2prong, "Reco 2-prong", "l");
  legend->AddEntry(nHits_3prong, "Reco 3-prong", "l");
  legend->Draw("same");
  window->Print("Example_correlation.pdf");
  file->cd(); window->Write();

  raw_signal_tree->Draw("(sqrt(pow(horizontalWidthPerDir[0],2)+pow(verticalWidthPerDir[0],2))+"
			"sqrt(pow(horizontalWidthPerDir[1],2)+pow(verticalWidthPerDir[1],2))+"
			"sqrt(pow(horizontalWidthPerDir[2],2)+pow(verticalWidthPerDir[2],2)))/3>>pseudoLength_all");
  set_hist_style(pseudoLength_all, "Pseudo length [mm]", "Events/bin", 0, "Pseudo length distribution", 1);
  pseudoLength_all->Draw();
  set_hist_style(pseudoLength_all_clicked, "Pseudo length [mm]", "Events/bin", color_all, "Pseudo length distribution");
  pseudoLength_all_clicked->Draw("same");
  set_hist_style(pseudoLength_1prong, "Pseudo length [mm]", "Events/bin", color_1prong);
  pseudoLength_1prong->Draw("same");
  set_hist_style(pseudoLength_2prong, "Pseudo length [mm]", "Events/bin", color_2prong);
  pseudoLength_2prong->Draw("same");
  set_hist_style(pseudoLength_3prong, "Pseudo length [mm]", "Events/bin", color_3prong);
  pseudoLength_3prong->Draw("same");
  legend = new TLegend(0.7,0.75,0.9,0.9,NULL,"brNDC");
  legend->AddEntry(pseudoLength_all, "Raw all", "f");
  legend->AddEntry(pseudoLength_all_clicked, "Reco all", "l");
  legend->AddEntry(pseudoLength_1prong, "Reco 1-prong", "l");
  legend->AddEntry(pseudoLength_2prong, "Reco 2-prong", "l");
  legend->AddEntry(pseudoLength_3prong, "Reco 3-prong", "l");
  legend->Draw("same");
  window->Print("Example_correlation.pdf");
  file->cd(); window->Write();

  raw_signal_tree->Draw("(totalChargePerDir[0]/sqrt(pow(horizontalWidthPerDir[0],2)+pow(verticalWidthPerDir[0],2))+"
			"totalChargePerDir[1]/sqrt(pow(horizontalWidthPerDir[1],2)+pow(verticalWidthPerDir[1],2))+"
			"totalChargePerDir[2]/sqrt(pow(horizontalWidthPerDir[2],2)+pow(verticalWidthPerDir[2],2)))/3>>pseudoDensity_all");
  set_hist_style(pseudoDensity_all, "Pseudo charge density [ADC units / mm]", "Events/bin", 0, "Pseudo charge density distribution", 1);
  pseudoDensity_all->Draw();
  set_hist_style(pseudoDensity_all_clicked, "Pseudo charge density [ADC units / mm]", "Events/bin", color_all, "Pseudo charge density distribution");
  pseudoDensity_all_clicked->Draw("same");
  set_hist_style(pseudoDensity_1prong, "Pseudo charge density [ADC units / mm]", "Events/bin", color_1prong);
  pseudoDensity_1prong->Draw("same");
  set_hist_style(pseudoDensity_2prong, "Pseudo charge density [ADC units / mm]", "Events/bin", color_2prong);
  pseudoDensity_2prong->Draw("same");
  set_hist_style(pseudoDensity_3prong, "Pseudo charge density [ADC units / mm]", "Events/bin", color_3prong);
  pseudoDensity_3prong->Draw("same");
  legend = new TLegend(0.7,0.75,0.9,0.9,NULL,"brNDC");
  legend->AddEntry(pseudoDensity_all, "Raw all", "f");
  legend->AddEntry(pseudoDensity_all_clicked, "Reco all", "l");
  legend->AddEntry(pseudoDensity_1prong, "Reco 1-prong", "l");
  legend->AddEntry(pseudoDensity_2prong, "Reco 2-prong", "l");
  legend->AddEntry(pseudoDensity_3prong, "Reco 3-prong", "l");
  legend->Draw("same");
  window->Print("Example_correlation.pdf");
  file->cd(); window->Write();

  raw_signal_tree->Draw("totalCharge/maxCharge>>chargeRatio_all");
  set_hist_style(chargeRatio_all, "Ratio totalCharge/maxCharge", "Events/bin", 0, "Ratio totalCharge/maxCharge distribution", 1);
  chargeRatio_all->Draw();
  set_hist_style(chargeRatio_all_clicked, "Ratio totalCharge/maxCharge", "Events/bin", color_all, "Ratio totalCharge/maxCharge distribution");
  chargeRatio_all_clicked->Draw("same");
  set_hist_style(chargeRatio_1prong, "Ratio totalCharge/maxCharge", "Events/bin", color_1prong);
  chargeRatio_1prong->Draw("same");
  set_hist_style(chargeRatio_2prong, "Ratio totalCharge/maxCharge", "Events/bin", color_2prong);
  chargeRatio_2prong->Draw("same");
  set_hist_style(chargeRatio_3prong, "Ratio totalCharge/maxCharge", "Events/bin", color_3prong);
  chargeRatio_3prong->Draw("same");
  legend = new TLegend(0.7,0.75,0.9,0.9,NULL,"brNDC");
  legend->AddEntry(chargeRatio_all, "Raw all", "f");
  legend->AddEntry(chargeRatio_all_clicked, "Reco all", "l");
  legend->AddEntry(chargeRatio_1prong, "Reco 1-prong", "l");
  legend->AddEntry(chargeRatio_2prong, "Reco 2-prong", "l");
  legend->AddEntry(chargeRatio_3prong, "Reco 3-prong", "l");
  legend->Draw("same");
  window->Print("Example_correlation.pdf");
  file->cd(); window->Write();

  raw_signal_tree->Draw("maxCharge/nHits>>reducedMaxCharge_all");
  set_hist_style(reducedMaxCharge_all, "Ratio maxCharge/nHits", "Events/bin", 0, "Ratio maxCharge/nHits distribution", 1);
  reducedMaxCharge_all->Draw();
  set_hist_style(reducedMaxCharge_all_clicked, "Ratio maxCharge/nHits", "Events/bin", color_all, "Ratio maxCharge/nHits distribution");
  reducedMaxCharge_all_clicked->Draw("same");
  set_hist_style(reducedMaxCharge_1prong, "Ratio maxCharge/nHits", "Events/bin", color_1prong);
  reducedMaxCharge_1prong->Draw("same");
  set_hist_style(reducedMaxCharge_2prong, "Ratio maxCharge/nHits", "Events/bin", color_2prong);
  reducedMaxCharge_2prong->Draw("same");
  set_hist_style(reducedMaxCharge_3prong, "Ratio maxCharge/nHits", "Events/bin", color_3prong);
  reducedMaxCharge_3prong->Draw("same");
  legend = new TLegend(0.7,0.75,0.9,0.9,NULL,"brNDC");
  legend->AddEntry(reducedMaxCharge_all, "Raw all", "f");
  legend->AddEntry(reducedMaxCharge_all_clicked, "Reco all", "l");
  legend->AddEntry(reducedMaxCharge_1prong, "Reco 1-prong", "l");
  legend->AddEntry(reducedMaxCharge_2prong, "Reco 2-prong", "l");
  legend->AddEntry(reducedMaxCharge_3prong, "Reco 3-prong", "l");
  legend->Draw("same");
  window->Print("Example_correlation.pdf");
  file->cd(); window->Write();

  raw_signal_tree->Draw("totalCharge/nHits>>reducedTotalCharge_all");
  set_hist_style(reducedTotalCharge_all, "Ratio totalCharge/nHits", "Events/bin", 0, "Ratio totalCharge/nHits distribution", 1);
  reducedTotalCharge_all->Draw();
  set_hist_style(reducedTotalCharge_all_clicked, "Ratio totalCharge/nHits", "Events/bin", color_all, "Ratio totalCharge/nHits distribution");
  reducedTotalCharge_all_clicked->Draw("same");
  set_hist_style(reducedTotalCharge_1prong, "Ratio totalCharge/nHits", "Events/bin", color_1prong);
  reducedTotalCharge_1prong->Draw("same");
  set_hist_style(reducedTotalCharge_2prong, "Ratio totalCharge/nHits", "Events/bin", color_2prong);
  reducedTotalCharge_2prong->Draw("same");
  set_hist_style(reducedTotalCharge_3prong, "Ratio totalCharge/nHits", "Events/bin", color_3prong);
  reducedTotalCharge_3prong->Draw("same");
  legend = new TLegend(0.7,0.75,0.9,0.9,NULL,"brNDC");
  legend->AddEntry(reducedTotalCharge_all, "Raw all", "f");
  legend->AddEntry(reducedTotalCharge_all_clicked, "Reco all", "l");
  legend->AddEntry(reducedTotalCharge_1prong, "Reco 1-prong", "l");
  legend->AddEntry(reducedTotalCharge_2prong, "Reco 2-prong", "l");
  legend->AddEntry(reducedTotalCharge_3prong, "Reco 3-prong", "l");
  legend->Draw("same");
  window->Print("Example_correlation.pdf");
  file->cd(); window->Write();

  for(auto ihalf=0; ihalf<1; ihalf++) {
    raw_signal_tree->Draw(Form("(horizontalChargePerDirHalf[0][%d]+horizontalChargePerDirHalf[1][%d]+horizontalChargePerDirHalf[2][%d])/"
			       "((sqrt(pow(horizontalWidthPerDir[0],2)+pow(verticalWidthPerDir[0],2))+"
			       "sqrt(pow(horizontalWidthPerDir[1],2)+pow(verticalWidthPerDir[1],2))+"
			       "sqrt(pow(horizontalWidthPerDir[2],2)+pow(verticalWidthPerDir[2],2)))/3)>>horizontalChargeAsymmetry_all",
			       ihalf, ihalf, ihalf));
  }
  set_hist_style(horizontalChargeAsymmetry_all, "Q^{MAX}_{HALF} / pseudo-length [ADC units / mm]", "Events/bin", 0, "Horizontal charge asymmetry distribution", 1);
  horizontalChargeAsymmetry_all->Draw();
  set_hist_style(horizontalChargeAsymmetry_all_clicked, "Q^{MAX}_{HALF} / pseudo-length [ADC units / mm]", "Events/bin", color_all, "Horizontal charge asymmetry distribution");
  horizontalChargeAsymmetry_all_clicked->Draw("same");
  set_hist_style(horizontalChargeAsymmetry_1prong, "Q^{MAX}_{HALF} / pseudo-length [ADC units / mm]", "Events/bin", color_1prong);
  horizontalChargeAsymmetry_1prong->Draw("same");
  set_hist_style(horizontalChargeAsymmetry_2prong, "Q^{MAX}_{HALF} / pseudo-length [ADC units / mm]", "Events/bin", color_2prong);
  horizontalChargeAsymmetry_2prong->Draw("same");
  set_hist_style(horizontalChargeAsymmetry_3prong, "Q^{MAX}_{HALF} / pseudo-length [ADC units / mm]", "Events/bin", color_3prong);
  horizontalChargeAsymmetry_3prong->Draw("same");
  legend = new TLegend(0.7,0.75,0.9,0.9,NULL,"brNDC");
  legend->AddEntry(horizontalChargeAsymmetry_all, "Raw all", "f");
  legend->AddEntry(horizontalChargeAsymmetry_all_clicked, "Reco all", "l");
  legend->AddEntry(horizontalChargeAsymmetry_1prong, "Reco 1-prong", "l");
  legend->AddEntry(horizontalChargeAsymmetry_2prong, "Reco 2-prong", "l");
  legend->AddEntry(horizontalChargeAsymmetry_3prong, "Reco 3-prong", "l");
  legend->Draw("same");
  window->Print("Example_correlation.pdf");
  file->cd(); window->Write();

  for(auto ihalf=0; ihalf<1; ihalf++) {
    raw_signal_tree->Draw(Form("(verticalChargePerDirHalf[0][%d]+verticalChargePerDirHalf[1][%d]+verticalChargePerDirHalf[2][%d])/"
			       "((sqrt(pow(horizontalWidthPerDir[0],2)+pow(verticalWidthPerDir[0],2))+"
			       "sqrt(pow(horizontalWidthPerDir[1],2)+pow(verticalWidthPerDir[1],2))+"
			       "sqrt(pow(horizontalWidthPerDir[2],2)+pow(verticalWidthPerDir[2],2)))/3)>>verticalChargeAsymmetry_all",
			       ihalf, ihalf, ihalf));
  }
  set_hist_style(verticalChargeAsymmetry_all, "Q^{MAX}_{HALF} / pseudo-length [ADC units / mm]", "Events/bin", 0, "Vertical charge asymmetry distribution", 1);
  verticalChargeAsymmetry_all->Draw();
  set_hist_style(verticalChargeAsymmetry_all_clicked, "Q^{MAX}_{HALF} / pseudo-length [ADC units / mm]", "Events/bin", color_all, "Vertical charge asymmetry distribution");
  verticalChargeAsymmetry_all_clicked->Draw("same");
  set_hist_style(verticalChargeAsymmetry_1prong, "Q^{MAX}_{HALF} / pseudo-length [ADC units / mm]", "Events/bin", color_1prong);
  verticalChargeAsymmetry_1prong->Draw("same");
  set_hist_style(verticalChargeAsymmetry_2prong, "Q^{MAX}_{HALF} / pseudo-length [ADC units / mm]", "Events/bin", color_2prong);
  verticalChargeAsymmetry_2prong->Draw("same");
  set_hist_style(verticalChargeAsymmetry_3prong, "Q^{MAX}_{HALF} / pseudo-length [ADC units / mm]", "Events/bin", color_3prong);
  verticalChargeAsymmetry_3prong->Draw("same");
  legend = new TLegend(0.7,0.75,0.9,0.9,NULL,"brNDC");
  legend->AddEntry(verticalChargeAsymmetry_all, "Raw all", "f");
  legend->AddEntry(verticalChargeAsymmetry_all_clicked, "Reco all", "l");
  legend->AddEntry(verticalChargeAsymmetry_1prong, "Reco 1-prong", "l");
  legend->AddEntry(verticalChargeAsymmetry_2prong, "Reco 2-prong", "l");
  legend->AddEntry(verticalChargeAsymmetry_3prong, "Reco 3-prong", "l");
  legend->Draw("same");
  window->Print("Example_correlation.pdf");
  file->cd(); window->Write();

  set_hist2D_style(horizontalChargeAsymmetry_vs_verticalChargeAsymmetry_1prong, "Vertical Q^{MAX}_{HALF} / pseudo-length [ADC units / mm]", "Horizontal Q^{MAX}_{HALF} / pseudo-length [ADC units / mm]", "Events/bin", color_1prong, "Horizontal vs Vertical charge asymmetry");
  horizontalChargeAsymmetry_vs_verticalChargeAsymmetry_1prong->Draw("box"); // "box"
  set_hist2D_style(horizontalChargeAsymmetry_vs_verticalChargeAsymmetry_2prong, "Vertical Q^{MAX}_{HALF} / pseudo-length [ADC units / mm]", "Horizontal Q^{MAX}_{HALF} / pseudo-length [ADC units / mm]", "Events/bin", color_2prong);
  horizontalChargeAsymmetry_vs_verticalChargeAsymmetry_2prong->Draw("same box"); // "same box"
  set_hist2D_style(horizontalChargeAsymmetry_vs_verticalChargeAsymmetry_3prong, "Vertical Q^{MAX}_{HALF} / pseudo-length [ADC units / mm]", "Horizontal Q^{MAX}_{HALF} / pseudo-length [ADC units / mm]", "Events/bin", color_3prong);
  horizontalChargeAsymmetry_vs_verticalChargeAsymmetry_3prong->Draw("same box"); // "same box"
  //  gPad->SetRightMargin(0.15);
  legend = new TLegend(0.7,0.81,0.9,0.9,NULL,"brNDC");
  legend->AddEntry(horizontalChargeAsymmetry_vs_verticalChargeAsymmetry_1prong, "Reco 1-prong", "l");
  legend->AddEntry(horizontalChargeAsymmetry_vs_verticalChargeAsymmetry_2prong, "Reco 2-prong", "l");
  legend->AddEntry(horizontalChargeAsymmetry_vs_verticalChargeAsymmetry_3prong, "Reco 3-prong", "l");
  legend->Draw("same");
  window->Print("Example_correlation.pdf");
  file->cd(); window->Write();

  set_hist2D_style(maxCharge_vs_nHits_1prong, "Maximal charge [ADC units]", "Number of hits", "Events/bin", color_1prong, "Number of hits vs maximal charge");
  maxCharge_vs_nHits_1prong->Draw("box"); // "box"
  set_hist2D_style(maxCharge_vs_nHits_2prong, "Maximal charge [ADC units]", "Number of hits", "Events/bin", color_2prong);
  maxCharge_vs_nHits_2prong->Draw("same box"); // "same box"
  set_hist2D_style(maxCharge_vs_nHits_3prong, "Maximal charge [ADC units]", "Number of hits", "Events/bin", color_3prong);
  maxCharge_vs_nHits_3prong->Draw("same box"); // "same box"
  //  gPad->SetRightMargin(0.15);
  legend = new TLegend(0.7,0.81,0.9,0.9,NULL,"brNDC");
  legend->AddEntry(maxCharge_vs_nHits_1prong, "Reco 1-prong", "l");
  legend->AddEntry(maxCharge_vs_nHits_2prong, "Reco 2-prong", "l");
  legend->AddEntry(maxCharge_vs_nHits_3prong, "Reco 3-prong", "l");
  legend->Draw("same");
  window->Print("Example_correlation.pdf");
  file->cd(); window->Write();

  set_hist2D_style(pseudoDensity_vs_nHits_1prong, "Pseudo charge density [ADC units / mm]", "Number of hits", "Events/bin", color_1prong, "Number of hits vs pseudo charge density");
  pseudoDensity_vs_nHits_1prong->Draw("box"); // "box"
  set_hist2D_style(pseudoDensity_vs_nHits_2prong, "Pseudo charge density [ADC units / mm]", "Number of hits", "Events/bin", color_2prong);
  pseudoDensity_vs_nHits_2prong->Draw("same box"); // "same box"
  set_hist2D_style(pseudoDensity_vs_nHits_3prong, "Pseudo charge density [ADC units / mm]", "Number of hits", "Events/bin", color_3prong);
  pseudoDensity_vs_nHits_3prong->Draw("same *"); // "same box"
  //  gPad->SetRightMargin(0.15);
  legend = new TLegend(0.7,0.81,0.9,0.9,NULL,"brNDC");
  legend->AddEntry(pseudoDensity_vs_nHits_1prong, "Reco 1-prong", "l");
  legend->AddEntry(pseudoDensity_vs_nHits_2prong, "Reco 2-prong", "l");
  legend->AddEntry(pseudoDensity_vs_nHits_3prong, "Reco 3-prong", "l");
  legend->Draw("same");
  window->Print("Example_correlation.pdf");
  file->cd(); window->Write();

  set_hist2D_style(pseudoDensity_vs_chargeRatio_1prong, "Pseudo charge density [ADC units / mm]", "Ratio totalCharge/maxCharge", "Events/bin", color_1prong, "Number of hits vs pseudo charge density");
  pseudoDensity_vs_chargeRatio_1prong->Draw("box"); // "box"
  set_hist2D_style(pseudoDensity_vs_chargeRatio_2prong, "Pseudo charge density [ADC units / mm]", "Ratio totalCharge/maxCharge", "Events/bin", color_2prong);
  pseudoDensity_vs_chargeRatio_2prong->Draw("same box"); // "same box"
  set_hist2D_style(pseudoDensity_vs_chargeRatio_3prong, "Pseudo charge density [ADC units / mm]", "Ratio totalCharge/maxCharge", "Events/bin", color_3prong);
  pseudoDensity_vs_chargeRatio_3prong->Draw("same box"); // "same box"
  //  gPad->SetRightMargin(0.15);
  legend = new TLegend(0.7,0.81,0.9,0.9,NULL,"brNDC");
  legend->AddEntry(pseudoDensity_vs_chargeRatio_1prong, "Reco 1-prong", "l");
  legend->AddEntry(pseudoDensity_vs_chargeRatio_2prong, "Reco 2-prong", "l");
  legend->AddEntry(pseudoDensity_vs_chargeRatio_3prong, "Reco 3-prong", "l");
  legend->Draw("same");
  window->Print("Example_correlation.pdf");
  file->cd(); window->Write();

  window->Print("Example_correlation.pdf]");
  file->Close();
  return 0;
}
