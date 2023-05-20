//////////////////////////
//
// root
// root [0] .L makePlots_trackDiffusion.cxx
// root [1] makePlotsDiffusion("TreeDiffusion.root");
//
// Macro for drawing various plots out of tree with reconstructed track diffusion properties
//
//////////////////////////
//////////////////////////

#ifndef __ROOTLOGON__
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

#include "TPCReco/TrackDiffusion_tree_dataFormat.h"

TFile *open_TFile(std::string &file_name, std::string mode="read"){
  TFile * tree_file = TFile::Open(file_name.c_str(), mode.c_str());
  if(tree_file == 0){
    std::cerr << "ERROR: Cannot open file: " << file_name << " in " << mode << " mode" << std::endl;
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
  gStyle->SetOptStat("neuoi");
  gStyle->SetOptStat(1);
  gStyle->SetPalette(kRainBow);
  gStyle->SetPadRightMargin(0.15);
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

void set_hist_range(TH1D* &hist, const double xmin, const double xmax){
  hist->GetXaxis()->SetRangeUser(xmin, xmax);
}

void set_hist2D_range(TH2D* &hist, const double xmin, const double xmax, const double ymin, const double ymax){
  hist->GetXaxis()->SetRangeUser(xmin, xmax);
  hist->GetYaxis()->SetRangeUser(ymin, ymax);
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

int makePlotsDiffusion(std::string diffusion_tree_file_name){

  TFile *diffusion_tree_file = open_TFile(diffusion_tree_file_name);
  TTree *diffusion_tree = get_tree(diffusion_tree_file, "TrackDiffusion");
  auto raw = new Event_rawdiffusion;
  diffusion_tree->SetBranchAddress("data", &raw);
  diffusion_tree_file->cd();
  diffusion_tree->BuildIndex("runId", "eventId");

  std::string output_file_name_prefix="Example_diffusion";
  std::string output_file_PDF=output_file_name_prefix+".pdf";
  std::string output_file_ROOT=output_file_name_prefix+".root";
  TFile *output = open_TFile(output_file_ROOT, "RECREATE");
  output->cd();
  set_global_style();
  TCanvas *window = new TCanvas("window", "", 800, 600);
  window->UseCurrentStyle();

  const double meanRange=5.0; // mm
  const double meanDisplayMax=2.0; // mm
  const double meanDisplayMin=-meanDisplayMax; // mm
  const double sigmaRange=5.0; // mm 
  const double sigmaDisplayMin=0.2; // mm
  const double sigmaDisplayMax=1.8; // mm
  TH2D *h_phiDET_vs_cosThetaDET = new TH2D("h_phiDET_vs_cosThetaDET", "", 20, -TMath::Pi(), TMath::Pi(), 20, -1.0, 1.0);
  TH2D *h_absPhiDET_vs_absCosThetaDET = new TH2D("h_absPhiDET_vs_absCosThetaDET", "", 20, 0.0, TMath::Pi(), 20, 0.0, 1.0);
  TH1D *h_meanAll = new TH1D("h_meanAll", "", 200, -meanRange, meanRange);
  TH1D *h_sigmaAll = new TH1D("h_sigmaAll", "", 200, 0.0, sigmaRange);
  TH1D *h_sigmaVertUp = new TH1D("h_sigmaVertUp", "", 200, 0.0, sigmaRange);
  TH1D *h_sigmaVertDown = new TH1D("h_sigmaVertDown", "", 200, 0.0, sigmaRange);
  TH1D *h_sigmaVert = new TH1D("h_sigmaVert", "", 200, 0.0, sigmaRange);
  TH2D *h_meanAll_vs_sigmaAll = new TH2D("h_meanAll_vs_sigmaAll", "", 100, -meanRange, meanRange, 100, 0.0, sigmaRange);
  TH1D *h_meanSum = (TH1D*)h_meanAll->Clone("h_meanSum");
  TH1D *h_sigmaSum = (TH1D*)h_sigmaAll->Clone("h_sigmaSum");
  TH1D *h_sigmaHorizSum = new TH1D("h_sigmaHorizSum", "", 200, 0.0, sigmaRange);
  TH1D *h_meanPerDir[3] = {NULL, NULL, NULL};
  TH1D *h_sigmaPerDir[3] = {NULL, NULL, NULL};
  TH1D *h_sigmaHorizPerDir[3] = {NULL, NULL, NULL};
  TH2D *h_phiDET_vs_sigmaPerDir[3] = {NULL, NULL, NULL};
  TH2D *h_cosThetaDET_vs_sigmaPerDir[3] = {NULL, NULL, NULL};
  const std::string dirname[3] = {"U", "V", "W"};
  for(auto dir=0; dir<3; dir++) {
    h_meanPerDir[dir] = new TH1D(Form("h_meanPerDir%d", dir), "", 200, -meanRange, meanRange);
    h_sigmaPerDir[dir] = new TH1D(Form("h_sigmaPerDir%d", dir), "", 200, 0.0, sigmaRange);
    h_sigmaHorizPerDir[dir] = new TH1D(Form("h_sigmaHorizPerDir%d", dir), "", 200, 0.0, sigmaRange);
    h_phiDET_vs_sigmaPerDir[dir] = new TH2D(Form("h_phiDET_vs_sigmaPerDir%d", dir), "", 50, -TMath::Pi(), TMath::Pi(), 50, 0.0, sigmaRange);
    h_cosThetaDET_vs_sigmaPerDir[dir] = new TH2D(Form("h_cosThetaDET_vs_sigmaPerDir%d", dir), "", 50, -1.0, 1.0, 50, 0.0, sigmaRange);
  }

  TCut cut2prong = "ntracks==2";
  TCut cutAll = cut2prong && "flagAll";
  TCut cut[3] = { cut2prong && "flagPerDir[0]", cut2prong && "flagPerDir[1]", cut2prong && "flagPerDir[2]" };
  TCut cutHorizontal[3] = { cut[0] && "fabs(cosThetaDET)<cos((90-45)/180.*TMath::Pi())" &&
			    "fabs(fmod(phiDET/TMath::Pi()*180,180))<5",      // horizontal U-direction (+/- 5 deg)
  			    cut[1] && "fabs(cosThetaDET)<cos((90-45)/180.*TMath::Pi())" &&
			    "fabs(fmod(phiDET/TMath::Pi()*180-60,180))<5",   // horizontal V-direction (+/- 5 deg)
  			    cut[2] && "fabs(cosThetaDET)<cos((90-45)/180.*TMath::Pi())" &&
			    "fabs(fmod(phiDET/TMath::Pi()*180+60,180))<5" }; // horizontal W-direction (+/- 5 deg)
  TCut cutDown = cutAll && "cosThetaDET>cos(10/180.*TMath::Pi())";            // vertical DOWN (+/- 10 deg)
  TCut cutUp = cutAll && "cosThetaDET<cos((180-10)/180.*TMath::Pi())";        // vertical UP (+/- 10 deg)

  window->Print((output_file_PDF+"[").c_str());

  diffusion_tree->Draw("cosThetaDET:phiDET>>h_phiDET_vs_cosThetaDET", cutAll, "goff");
  set_hist2D_style(h_phiDET_vs_cosThetaDET, "#phi_{DET} [rad]", "cos(#theta_{DET})", "Tracks / bin", kBlack,  "Leading track (2-prong) - all projections");
  h_phiDET_vs_cosThetaDET->Draw("COLZ");
  window->Print(output_file_PDF.c_str());
  window->SetName(h_phiDET_vs_cosThetaDET->GetName());
  window->Write();

  diffusion_tree->Draw("fabs(cosThetaDET):fabs(phiDET)>>h_absPhiDET_vs_absCosThetaDET", cutAll, "goff");
  set_hist2D_style(h_absPhiDET_vs_absCosThetaDET, "|#phi_{DET}| [rad]", "|cos(#theta_{DET})|", "Tracks / bin", kBlack,  "Leading track (2-prong) - all projections");
  h_absPhiDET_vs_absCosThetaDET->Draw("COLZ");
  window->Print(output_file_PDF.c_str());
  window->SetName(h_absPhiDET_vs_absCosThetaDET->GetName());
  window->Write();

  for(auto dir=0; dir<3; dir++) {
    diffusion_tree->Draw(Form("meanPerDir[%d]>>h_meanPerDir%d", dir, dir), cut[dir], "goff");
    set_hist_style(h_meanPerDir[dir], "Mean hit distance w.r.t. RECO axis [mm]", "Tracks / bin", kBlack, Form("Leading track (2-prong) - %sZ projection", dirname[dir].c_str()));
    h_meanPerDir[dir]->Draw();
    set_hist_range(h_meanPerDir[dir], meanDisplayMin, meanDisplayMax);
    window->Print(output_file_PDF.c_str());
    window->SetName(h_meanPerDir[dir]->GetName());
    window->Write();

    diffusion_tree->Draw(Form("sigmaPerDir[%d]>>h_sigmaPerDir%d", dir, dir), cut[dir], "goff"); 
    set_hist_style(h_sigmaPerDir[dir], "RMS of hit distance w.r.t. RECO axis [mm]", "Tracks / bin", kBlack, Form("Leading track (2-prong) - %sZ projection", dirname[dir].c_str()));
    h_sigmaPerDir[dir]->Draw();
    set_hist_range(h_sigmaPerDir[dir], sigmaDisplayMin, sigmaDisplayMax);
    window->Print(output_file_PDF.c_str());
    window->SetName(h_sigmaPerDir[dir]->GetName());
    window->Write();

    h_meanSum->Add(h_meanPerDir[dir]);
    h_sigmaSum->Add(h_sigmaPerDir[dir]);
    
    diffusion_tree->Draw(Form("sigmaPerDir[%d]:phiDET>>h_phiDET_vs_sigmaPerDir%d", dir, dir), cut[dir], "goff"); 
    set_hist2D_style(h_phiDET_vs_sigmaPerDir[dir], "#phi_{DET} [rad]", "RMS of hit distance w.r.t. RECO axis [mm]", "Tracks / bin", kBlack, Form("Leading track (2-prong) - %sZ projection", dirname[dir].c_str()));
    h_phiDET_vs_sigmaPerDir[dir]->Draw("COLZ");
    set_hist2D_range(h_phiDET_vs_sigmaPerDir[dir], -TMath::Pi(), TMath::Pi(), sigmaDisplayMin, sigmaDisplayMax);
    window->Print(output_file_PDF.c_str());
    window->SetName(h_phiDET_vs_sigmaPerDir[dir]->GetName());
    window->Write();

    diffusion_tree->Draw(Form("sigmaPerDir[%d]:cosThetaDET>>h_cosThetaDET_vs_sigmaPerDir%d", dir, dir), cut[dir], "goff"); 
    set_hist2D_style(h_cosThetaDET_vs_sigmaPerDir[dir], "cos(#theta_{DET})", "RMS of hit distance w.r.t. RECO axis [mm]", "Tracks / bin", kBlack, Form("Leading track (2-prong) - %sZ projection", dirname[dir].c_str()));
    h_cosThetaDET_vs_sigmaPerDir[dir]->Draw("COLZ");
    set_hist2D_range(h_cosThetaDET_vs_sigmaPerDir[dir], -1.0, 1.0, sigmaDisplayMin, sigmaDisplayMax);
    window->Print(output_file_PDF.c_str());
    window->SetName(h_cosThetaDET_vs_sigmaPerDir[dir]->GetName());
    window->Write();    
  }
  
  set_hist_style(h_meanSum, "Mean hit distance w.r.t. RECO axis [mm]", "Track projections / bin", kBlack, "Leading track (2-prong) - sum of 3 projections");
  h_meanSum->Draw();
  set_hist_range(h_meanSum, meanDisplayMin, meanDisplayMax);
  window->Print(output_file_PDF.c_str());
  window->SetName(h_meanSum->GetName());
  window->Write();

  set_hist_style(h_sigmaSum, "RMS of hit distance w.r.t. RECO axis [mm]", "Track projections / bin", kBlack, "Leading track (2-prong) - sum of 3 projections");
  h_sigmaSum->Draw();
  set_hist_range(h_sigmaSum, sigmaDisplayMin, sigmaDisplayMax);
  window->Print(output_file_PDF.c_str());
  window->SetName(h_sigmaSum->GetName());
  window->Write();
  
  diffusion_tree->Draw("meanAll>>h_meanAll", cutAll, "goff");
  set_hist_style(h_meanAll, "Mean hit distance w.r.t. RECO axis [mm]", "Tracks / bin", kBlack, "Leading track (2-prong) - all projections");
  h_meanAll->Draw();
  set_hist_range(h_meanAll, meanDisplayMin, meanDisplayMax);
  window->Print(output_file_PDF.c_str());
  window->SetName(h_meanAll->GetName());
  window->Write();

  diffusion_tree->Draw("sigmaAll>>h_sigmaAll", cutAll, "goff");
  set_hist_style(h_sigmaAll, "RMS of distance w.r.t. RECO axis [mm]", "Tracks / bin", kBlack, "Leading track (2-prong) - all projections");
  h_sigmaAll->Draw();
  set_hist_range(h_sigmaAll, sigmaDisplayMin, sigmaDisplayMax);
  window->Print(output_file_PDF.c_str());
  window->SetName(h_sigmaAll->GetName());
  window->Write();

  diffusion_tree->Draw("sigmaAll:meanAll>>h_meanAll_vs_sigmaAll", cutAll, "goff");
  set_hist2D_style(h_meanAll_vs_sigmaAll, "Mean hit distance w.r.t. RECO axis [mm]", "RMS of distance w.r.t. RECO axis [mm]", "Tracks / bin", kBlack, "Leading track (2-prong) - all projections");
  h_meanAll_vs_sigmaAll->Draw("COLZ");
  set_hist2D_range(h_meanAll_vs_sigmaAll, meanDisplayMin, meanDisplayMax, sigmaDisplayMin, sigmaDisplayMax);
  window->Print(output_file_PDF.c_str());
  window->SetName(h_meanAll_vs_sigmaAll->GetName());
  window->Write();

  for(auto dir=0; dir<3; dir++) {
    diffusion_tree->Draw(Form("sigmaPerDir[%d]>>h_sigmaHorizPerDir%d", dir, dir), cutHorizontal[dir], "goff");
    set_hist_style(h_sigmaHorizPerDir[dir], "RMS of distance w.r.t. RECO axis [mm]", Form("Horizontal-%s track projections / bin", dirname[dir].c_str()), kBlack, Form("Leading track (2-prong) - %sZ projection", dirname[dir].c_str()));
    h_sigmaHorizPerDir[dir]->Draw();
    set_hist_range(h_sigmaHorizPerDir[dir], sigmaDisplayMin, sigmaDisplayMax);
    window->Print(output_file_PDF.c_str());
    window->SetName(h_sigmaHorizPerDir[dir]->GetName());
    window->Write();

    h_sigmaHorizSum->Add(h_sigmaHorizPerDir[dir]);
  }

  set_hist_style(h_sigmaHorizSum, "RMS of hit distance w.r.t. RECO axis [mm]", "Horizontal-U/V/W track projections / bin", kBlack, "Leading track (2-prong) - sum of 3 projections");
  h_sigmaHorizSum->Draw();
  set_hist_range(h_sigmaHorizSum, sigmaDisplayMin, sigmaDisplayMax);
  window->Print(output_file_PDF.c_str());
  window->SetName(h_sigmaHorizSum->GetName());
  window->Write();

  diffusion_tree->Draw("sigmaAll>>h_sigmaVertUp", cutUp, "goff");
  set_hist_style(h_sigmaVertUp, "RMS of distance w.r.t. RECO axis [mm]", "Vertical-UP tracks / bin", kBlack, "Leading track (2-prong) - all projections");
  h_sigmaVertUp->Draw();
  set_hist_range(h_sigmaVertUp, sigmaDisplayMin, sigmaDisplayMax);
  window->Print(output_file_PDF.c_str());
  window->SetName(h_sigmaVertUp->GetName());
  window->Write();

  diffusion_tree->Draw("sigmaAll>>h_sigmaVertDown", cutDown, "goff");
  set_hist_style(h_sigmaVertDown, "RMS of distance w.r.t. RECO axis [mm]", "Vertical-DOWN tracks / bin", kBlack, "Leading track (2-prong) - all projections");
  h_sigmaVertDown->Draw();
  set_hist_range(h_sigmaVertDown, sigmaDisplayMin, sigmaDisplayMax);
  window->Print(output_file_PDF.c_str());
  window->SetName(h_sigmaVertDown->GetName());
  window->Write();

  diffusion_tree->Draw("sigmaAll>>h_sigmaVert", cutDown || cutUp, "goff");
  set_hist_style(h_sigmaVert, "RMS of distance w.r.t. RECO axis [mm]", "Vertical-UP/DOWN tracks / bin", kBlack, "Leading track (2-prong) - all projections");
  h_sigmaVert->Draw();
  set_hist_range(h_sigmaVert, sigmaDisplayMin, sigmaDisplayMax);
  window->Print(output_file_PDF.c_str());
  window->SetName(h_sigmaVert->GetName());
  window->Write();

  window->Print((output_file_PDF+"]").c_str());
  return 0;
}
