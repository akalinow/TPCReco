//////////////////////////
//
// root
// root [0] .L makePlotsReco_mm_scale_bug.cxx
// root [1] makePlotsReco_mm_scale_bug("Trees.root", "geometry.dat")
//
// Macro for evaluating various effects of "millimiter scaling bug" caused by
// wrong U/V/W bin sizes in [mm] in 2D histograms produced by GeometryTPC class.
//
// NOTE: This macro is valid only for RECO files produced with releases <= higs_06 (including higs_06.1).
//       The histogram scaling bug is now corrected in the master branch as of Dec 2022.
//
//////////////////////////
//////////////////////////

#ifndef __ROOTLOGON__
R__ADD_INCLUDE_PATH(../../Analysis/include)
R__ADD_INCLUDE_PATH(../../DataFormats/include)
R__ADD_INCLUDE_PATH(../../Utilities/include)
R__ADD_LIBRARY_PATH(../lib)
#endif

#include <iostream>
#include <string>
#include <vector>
#include <tuple>
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
#include "TVector2.h"
#include "TVector3.h"
#include "TPaveStats.h"

#include "GeometryTPC.h"
#include "HIGS_trees_dataFormat.h"

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
  gStyle->SetOptStat(1001101); // show: integral, mean, rms, name
  //  gStyle->SetOptStat("rmni");
  //  gStyle->SetOptStat("neuoi");
  //  gStyle->SetOptStat(0);
  gStyle->SetPalette(kRainBow);
}

void set_hist_style(TH1D* &hist, std::string xaxis, std::string yaxis, Int_t color, std::string title="", bool fill=false){
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
/////////////////////////
/////////////////////////
bool check_bin_compatibility(TH1 *h1, TH1 *h2){
  // x-axis (1D and 2D histograms)
  std::cout << __FUNCTION__ << ": dimension = " << h1->GetDimension() << " " << h2->GetDimension() << std::endl;
  if(h1->GetDimension()!=h2->GetDimension()) return false;
  std::cout << __FUNCTION__ << ": nbins = " << h1->GetNbinsX() << " " << h2->GetNbinsX() << std::endl;
  if(h1->GetNbinsX()!=h2->GetNbinsX()) return false;
  std::cout << __FUNCTION__ << ": xmin = " << h1->GetXaxis()->GetXmin() << " " << h2->GetXaxis()->GetXmin()<< std::endl;
  if(h1->GetXaxis()->GetXmin()!=h2->GetXaxis()->GetXmin()) return false;
  std::cout << __FUNCTION__ << ": xmax = " << h1->GetXaxis()->GetXmax() << " " << h2->GetXaxis()->GetXmax() << std::endl;
  if(h1->GetXaxis()->GetXmax()!=h2->GetXaxis()->GetXmax()) return false;
  // y-axis (2D histograms)
  if(h1->GetDimension()>=2) {
    if(h1->GetNbinsY()!=h2->GetNbinsY()) return false;
    if(h1->GetYaxis()->GetXmin()!=h2->GetYaxis()->GetXmin()) return false;
    if(h1->GetYaxis()->GetXmax()!=h2->GetYaxis()->GetXmax()) return false;
  }
  return true;
}
/////////////////////////
/////////////////////////
void draw_chi2_by_bin(TH1D *h1, TH1D *h2, Int_t color=kBlue+2, const char *xtitle=NULL){
  if(!check_bin_compatibility(h1, h2) || h1->GetDimension()!=1) {
    std::cout << "ERROR: Bins of input histograms are not compatible!" << std::endl;
    return;
  }
  auto hnew=new TH1D("h_chi2",
		Form("%s vs %s;%s;#chi^2 / bin", h1->GetName(), h2->GetName(), h1->GetXaxis()->GetTitle()),
		h1->GetNbinsX(), h1->GetXaxis()->GetXmin(), h1->GetXaxis()->GetXmax());
  if(xtitle) hnew->GetXaxis()->SetTitle(xtitle);

  for(auto ibin=1; ibin<=h1->GetNbinsX(); ibin++) {
    if(h1->GetBinError(ibin)==0 && h2->GetBinError(ibin)==0) continue; // skip empty bins
    auto chi2=pow(h1->GetBinContent(ibin) - h2->GetBinContent(ibin), 2)/
      ( pow(h1->GetBinError(ibin), 2) + pow(h2->GetBinError(ibin), 2) );
    hnew->Fill( h1->GetBinCenter(ibin), chi2 );
  }
  hnew->SetLineColor(color);
  hnew->Draw("HIST");
  hnew->SetDirectory(0); // prevent storing this temporary histogram to a ROOT file
}
/////////////////////////
/////////////////////////
void draw_difference_by_bin(TH1D *h1, TH1D *h2, Int_t color=kBlue+2, const char *xtitle=NULL){
  if(!check_bin_compatibility(h1, h2) || h1->GetDimension()!=1) {
    std::cout << "ERROR: Bins of input histograms are not compatible!" << std::endl;
    return;
  }
  auto hnew=new TH1D("h_diff",
		Form("%s - %s;%s;Difference / bin", h1->GetName(), h2->GetName(), h1->GetXaxis()->GetTitle()),
		h1->GetNbinsX(), h1->GetXaxis()->GetXmin(), h1->GetXaxis()->GetXmax());
  hnew->Sumw2(false);
  if(xtitle) hnew->GetXaxis()->SetTitle(xtitle);
  hnew->SetStats(0);
  for(auto ibin=1; ibin<=h1->GetNbinsX(); ibin++) {
    if(h1->GetBinError(ibin)==0 && h2->GetBinError(ibin)==0) continue; // skip empty bins
    auto diff=h1->GetBinContent(ibin) - h2->GetBinContent(ibin);
    auto err=sqrt( pow(h1->GetBinError(ibin), 2) + pow(h2->GetBinError(ibin), 2) );
    hnew->SetBinContent(ibin, diff);
    hnew->SetBinError(ibin, err);
  }
  hnew->SetLineColor(color);
  hnew->Draw("HIST");
  hnew->SetDirectory(0); // prevent storing this temporary histogram to a ROOT file
}
/////////////////////////
/////////////////////////
void overlay_histograms(TH1D *h1, TH1D *h2, Int_t color1=kRed, Int_t color2=kBlue+2, const char *xtitle=NULL){
  auto hnew1=(TH1D*)h1->DrawClone("HIST");
  hnew1->SetLineColor(color1);
  auto hnew2=(TH1D*)h2->DrawClone("SAMES HIST");
  hnew2->SetLineColor(color2);
  double ymin=std::min(hnew1->GetMinimum(), hnew2->GetMinimum());
  double ymax=std::max(hnew1->GetMaximum(), hnew2->GetMaximum());
  hnew1->SetMinimum(ymin);
  hnew1->SetMaximum(ymax + (ymax-ymin)*0.05 );
  if(xtitle) hnew1->GetXaxis()->SetTitle(xtitle);
  gPad->Update();
  auto stats1 = (TPaveStats*)hnew1->GetListOfFunctions()->FindObject("stats");
  auto stats2 = (TPaveStats*)hnew2->GetListOfFunctions()->FindObject("stats")->Clone("stats2");
  stats2->SetY1NDC( stats1->GetY1NDC() - 0.2 );
  stats2->SetY2NDC( stats1->GetY2NDC() - 0.2 );
  stats1->SetTextColor(color1);
  stats2->SetTextColor(color2);
  stats1->Draw();
  stats2->Draw();
  hnew1->SetDirectory(0); // prevent storing this temporary histogram to a ROOT file
  hnew2->SetDirectory(0); // prevent storing this temporary histogram to a ROOT file
}
/////////////////////////
/////////////////////////
std::shared_ptr<GeometryTPC> loadGeometry(const std::string fileName){
  return std::make_shared<GeometryTPC>(fileName.c_str(), false);
}
/////////////////////////
/////////////////////////
std::vector<TVector3> get_correct_pos_list(std::shared_ptr<GeometryTPC> geo, TVector3 &point){

  std::vector<TVector3> list;

  double x_bad=point.X(); // wrong X-coordinate (affected by scaling bug)
  double y_bad=point.Y(); // wrong Y-coordinate (affected by scaling bug)

  // convert (X,Y) pair to (U,V,W) triplet
  double uvw_pos_bad[3];
  for(int idir=0; idir<3; idir++) {
    bool err=false;
    uvw_pos_bad[idir]=geo->Cartesian2posUVW(x_bad, y_bad, idir, err); // wrong UVW-coordinate (affected by scaling bug)
  }

  // compute corrected (U,V,W) triplet
  double uvw_pos_corr[3];
  double pitch=geo->GetStripPitch();
  double uvw_range_bad_min, uvw_range_bad_max;
  double uvw_range_corr_min, uvw_range_corr_max;
  for(int idir=0; idir<3; idir++) {
    // std::tie(uvw_range_bad_min, uvw_range_bad_max) = geo->rangeStripDirInMM(idir); // does not work in interactive mode

    // first & last strip (merged by sections)
    bool err=false;
    uvw_range_bad_min=geo->Strip2posUVW(idir, geo->GetDirMinStripMerged(idir), err);
    uvw_range_bad_max=geo->Strip2posUVW(idir, geo->GetDirMaxStripMerged(idir), err);
    if(uvw_range_bad_min>uvw_range_bad_max) {
      auto val=uvw_range_bad_max;
      uvw_range_bad_max=uvw_range_bad_min;
      uvw_range_bad_min=val;
    }

    uvw_range_corr_min=uvw_range_bad_min-0.5*pitch; // corrected 2D histogram range in [mm]
    uvw_range_corr_max=uvw_range_bad_max+0.5*pitch; // corrected 2D hsitogram range in [mm]
    uvw_pos_corr[idir]=uvw_range_corr_min+
      (uvw_range_corr_max-uvw_range_corr_min)*
      (uvw_pos_bad[idir]-uvw_range_bad_min)/(uvw_range_bad_max-uvw_range_bad_min); // correct UVW position in [mm]
  }

  // convert corrected (U,V), (V,W), (W,U) pairs to new, corrected (X,Y) pairs
  for(int idir1=0; idir1<3; idir1++) {
    TVector2 corrected_point(0,0);
    int idir2=(idir1+1)%3;
    bool err=geo->GetUVWCrossPointInMM(idir1, uvw_pos_corr[idir1], idir2, uvw_pos_corr[idir2], corrected_point);
    list.push_back(TVector3( corrected_point.X(), corrected_point.Y(), point.Z() )); // correct candidate position
  }

  return list;
}

int makePlotsReco_mm_scale_bug(std::string reco_tree_file_name, std::string geometryName){

  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }

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

  auto myGeometry=loadGeometry(geometryName.c_str());

  TFile *out_file = new TFile("Debug_histograms.root", "RECREATE");
  if(!out_file) {
    std::cout << "ERROR: Cannot create output ROOT file!" << std::endl;
    return -1;
  }

  TCanvas *window = new TCanvas("window", "", 800, 600);
  set_global_style();
  const double bin_width=0.2; // [mm]
  const int max_diff=5; // [mm]
  const int nbins=2*(int)(max_diff/bin_width)+1; // odd number

  // all
  TH1D *deltaX_all = new TH1D("deltaX_all", "All events;(Original-Smeared) X_{DET} difference [mm];Points / bin (normalized)",
			      nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width);
  TH1D *deltaY_all = new TH1D("deltaY_all", "All events;(Original-Smeared) Y_{DET} difference [mm];Points / bin (normalized)",
			      nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width);
  TH2D *deltaXY_all = new TH2D("deltaXY_all", "All events;(Original-Smeared) X_{DET} difference [mm];(Original-Smeared) Y_{DET} difference [mm];Points / bin (normalized)",
			       nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width,
			       nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width);
  TH1D *length_corr_all = new TH1D("length_corr_all", "All events;Smeared track length [mm];Tracks / bin (normalized)",
					   120, 0.0, 120.0);
  TH1D *length_bad_all = new TH1D("length_bad_all", "All events;Original track length [mm];Tracks / bin",
					   120, 0.0, 120.0);
  TH1D *deltaLength_all = new TH1D("deltaLength_all", "All events;(Original-Smeared) track length difference [mm];Tracks / bin (normalized)",
				   nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width);
  TH2D *deltaLengthVsLength_all = new TH2D("deltaLengthVsLength_all", "All events;Original track length [mm];(Original-Smeared) track length difference [mm];Tracks / bin (normalized)",
					   200, 0.0, 200.0,
					   nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width);

  // 1-prong
  TH1D *deltaX_1prong = new TH1D("deltaX_1prong", "1-prong events;(Original-Smeared) X_{DET} difference [mm];Points / bin (normalized)",
				 nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width);
  TH1D *deltaY_1prong = new TH1D("deltaY_1prong", "1-prong events;(Original-Smeared) Y_{DET} difference [mm];Points / bin (normalized)",
				 nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width);
  TH2D *deltaXY_1prong = new TH2D("deltaXY_1prong", "1-prong events;(Original-Smeared) X_{DET} difference [mm];(Original-Smeared) Y_{DET} difference [mm];Points / bin (normalized)",
				  nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width,
				  nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width);
  TH1D *length_corr_1prong = new TH1D("length_corr_1prong", "1-prong events;Smeared track length [mm];Tracks / bin (normalized)",
					   120, 0.0, 120.0);
  TH1D *length_bad_1prong = new TH1D("length_bad_1prong", "1-prong events;Original track length [mm];Tracks / bin",
					   120, 0.0, 120.0);
  TH1D *deltaLength_1prong = new TH1D("deltaLength_1prong", "1-prong events;(Original-Smeared) track length difference [mm];Tracks / bin (normalized)",
				      nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width);
  TH2D *deltaLengthVsLength_1prong = new TH2D("deltaLengthVsLength_1prong", "1-prong events;Original track length [mm];(Original-Smeared) track length difference [mm];Tracks / bin (normalized)",
					   200, 0.0, 200.0,
					   nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width);

  // 2-prong
  TH1D *deltaX_2prong = new TH1D("deltaX_2prong", "2-prong events;(Original-Smeared) X_{DET} difference [mm];Points / bin (normalized)",
				 nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width);
  TH1D *deltaY_2prong = new TH1D("deltaY_2prong", "2-prong events;(Original-Smeared) Y_{DET} difference [mm];Points / bin (normalized)",
				 nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width);
  TH2D *deltaXY_2prong = new TH2D("deltaXY_2prong", "2-prong events;(Original-Smeared) X_{DET} difference [mm];(Original-Smeared) Y_{DET} difference [mm];Points / bin (normalized)",
				  nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width,
				  nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width);
  TH1D *alphaLength_corr_2prong = new TH1D("alphaLength_corr_2prong", "2-prong events;Smeared #alpha track length [mm];Tracks / bin (normalized)",
					   120, 0.0, 120.0);
  TH1D *alphaLength_bad_2prong = new TH1D("alphaLength_bad_2prong", "2-prong events;Original #alpha track length [mm];Tracks / bin",
					   120, 0.0, 120.0);
  TH1D *carbonLength_corr_2prong = new TH1D("carbonLength_corr_2prong", "2-prong events;Smeared carbon track length [mm];Tracks / bin (normalized)",
					    50, 0.0, 50.0);
  TH1D *carbonLength_bad_2prong = new TH1D("carbonLength_bad_2prong", "2-prong events;Original carbon track length [mm];Tracks / bin",
					    50, 0.0, 50.0);
  TH1D *sumLength_corr_2prong = new TH1D("sumLength_corr_2prong", "2-prong events;Smeared #alpha+C track length sum [mm];Events / bin (normalized)",
					 120, 0.0, 120.0);
  TH1D *sumLength_bad_2prong = new TH1D("sumLength_bad_2prong", "2-prong events;Original #alpha+C track length sum [mm];Events / bin",
					 120, 0.0, 120.0);
  TH1D *deltaSumLength_2prong = new TH1D("deltaSumLength_2prong", "2-prong events;(Original-Smeared) #alpha+C track length sum difference [mm];Events / bin (normalized)",
				      nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width);
  TH1D *length_corr_2prong = new TH1D("length_corr_2prong", "2-prong events;Smeared track length [mm];Tracks / bin (normalized)",
					   120, 0.0, 120.0);
  TH1D *length_bad_2prong = new TH1D("length_bad_2prong", "2-prong events;Original track length [mm];Tracks / bin",
					   120, 0.0, 120.0);
  TH1D *deltaLength_2prong = new TH1D("deltaLength_2prong", "2-prong events;(Original-Smeared) track length difference [mm];Tracks / bin (normalized)",
				      nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width);
  TH2D *deltaLengthVsLength_2prong = new TH2D("deltaLengthVsLength_2prong", "2-prong events;Original track length [mm];(Original-Smeared) track length difference [mm];Tracks / bin (normalized)",
					   200, 0.0, 200.0,
					   nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width);

  // 3-prong
  TH1D *deltaX_3prong = new TH1D("deltaX_3prong", "3-prong events;(Original-Smeared) X_{DET} difference [mm];Points / bin (normalized)",
				 nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width);
  TH1D *deltaY_3prong = new TH1D("deltaY_3prong", "3-prong events;(Original-Smeared) Y_{DET} difference [mm];Points / bin (normalized)",
				 nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width);
  TH2D *deltaXY_3prong = new TH2D("deltaXY_3prong", "3-prong events;(Original-Smeared) X_{DET} difference [mm];(Original-Smeared) Y_{DET} difference [mm];Points / bin (normalized)",
				  nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width,
				  nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width);
  TH1D *alpha1Length_corr_3prong = new TH1D("alpha1Length_corr_3prong", "3-prong events;Smeared #alpha_{1} track length [mm];Tracks / bin (normalized)",
					   120, 0.0, 120.0);
  TH1D *alpha1Length_bad_3prong = new TH1D("alpha1Length_bad_3prong", "3-prong events;Original #alpha_{1} track length [mm];Tracks / bin",
					   120, 0.0, 120.0);
  TH1D *alpha2Length_corr_3prong = new TH1D("alpha2Length_corr_3prong", "3-prong events;Smeared #alpha_{2} track length [mm];Tracks / bin (normalized)",
					   120, 0.0, 120.0);
  TH1D *alpha2Length_bad_3prong = new TH1D("alpha2Length_bad_3prong", "3-prong events;Original #alpha_{2} track length [mm];Tracks / bin",
					   120, 0.0, 120.0);
  TH1D *alpha3Length_corr_3prong = new TH1D("alpha3Length_corr_3prong", "3-prong events;Smeared #alpha_{3} track length [mm];Tracks / bin (normalized)",
					   120, 0.0, 120.0);
  TH1D *alpha3Length_bad_3prong = new TH1D("alpha3Length_bad_3prong", "3-prong events;Original #alpha_{3} track length [mm];Tracks / bin",
					   120, 0.0, 120.0);
  TH1D *sumLength_corr_3prong = new TH1D("sumLength_corr_3prong", "3-prong events;Smeared #alpha_{1}+#alpha_{2}+#alpha_{3} track length sum [mm];Events / bin (normalized)",
					 120, 0.0, 120.0);
  TH1D *sumLength_bad_3prong = new TH1D("sumLength_bad_3prong", "3-prong events;Original #alpha_{1}+#alpha_{2}+#alpha_{3} track length sum [mm];Events / bin",
					 120, 0.0, 120.0);
  TH1D *deltaSumLength_3prong = new TH1D("deltaSumLength_3prong", "3-prong events;(Original-Smeared) #alpha_{1}+#alpha_{2}+#alpha_{3} track length sum difference [mm];Events / bin (normalized)",
				      nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width);
  TH1D *length_corr_3prong = new TH1D("length_corr_3prong", "3-prong events;Smeared track length [mm];Tracks / bin (normalized)",
					   120, 0.0, 120.0);
  TH1D *length_bad_3prong = new TH1D("length_bad_3prong", "3-prong events;Original track length [mm];Tracks / bin",
					   120, 0.0, 120.0);
  TH1D *deltaLength_3prong = new TH1D("deltaLength_3prong", "3-prong events;(Original-Smeared) track length difference [mm];Tracks / bin (normalized)",
				      nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width);
  TH2D *deltaLengthVsLength_3prong = new TH2D("deltaLengthVsLength_3prong", "3-prong events;Original track length [mm];(Original-Smeared) track length difference [mm];Tracks / bin (normalized)",
					   200, 0.0, 200.0,
					   nbins, -0.5*nbins*bin_width, 0.5*nbins*bin_width);

  const double combinatorics_factor_point = 3;
  const double combinatorics_factor_length = 3*3;
  const double combinatorics_factor_1prong = combinatorics_factor_length;
  const double combinatorics_factor_2prong = 3*3*3;
  const double combinatorics_factor_3prong = 3*3*3*3;

  // loop over 1-prong tree
  for(auto i=0; i<reco_tree_1prong->GetEntries(); i++){
    reco_tree_1prong->GetEntry(i);

    std::vector<TVector3> vertexList = get_correct_pos_list(myGeometry, event1->vertexPos);
    std::vector<TVector3> endList = get_correct_pos_list(myGeometry, event1->endPos);

    for(auto &pos : vertexList) {
      auto diff = event1->vertexPos - pos; // [mm]
      auto w = 1./combinatorics_factor_point; // weight
      deltaX_all->Fill( diff.X(), w );
      deltaX_1prong->Fill( diff.X(), w );
      deltaY_all->Fill( diff.Y(), w );
      deltaY_1prong->Fill( diff.Y(), w );
      deltaXY_all->Fill( diff.X(), diff.Y(), w );
      deltaXY_1prong->Fill( diff.X(), diff.Y(), w );
    }
    for(auto &pos1 : endList) {
      auto diff = event1->endPos - pos1; // [mm]
      auto w = 1./combinatorics_factor_point; // weight
      deltaX_all->Fill( diff.X(), w );
      deltaX_1prong->Fill( diff.X(), w );
      deltaY_all->Fill( diff.Y(), w );
      deltaY_1prong->Fill( diff.Y(), w );
      deltaXY_all->Fill( diff.X(), diff.Y(), w );
      deltaXY_1prong->Fill( diff.X(), diff.Y(), w );
    }

    length_bad_all->Fill( event1->length );
    length_bad_1prong->Fill( event1->length );
    for(auto &pos : vertexList) {
      for(auto &pos1 : endList) {
	auto len = (pos1-pos).Mag(); // [mm]
	auto diff = event1->length - len; // [mm]
	auto w = 1./combinatorics_factor_1prong; // weight
	deltaLength_all->Fill( diff, w );
	deltaLength_1prong->Fill( diff, w );
	deltaLengthVsLength_all->Fill( event1->length, diff, w );
	deltaLengthVsLength_1prong->Fill( event1->length, diff, w );
	length_corr_all->Fill( len, w );
	length_corr_1prong->Fill( len, w );
      }
    }
  }

  // loop over 2-prong tree
  for(auto i=0; i<reco_tree_2prong->GetEntries(); i++){
    reco_tree_2prong->GetEntry(i);

    std::vector<TVector3> vertexList = get_correct_pos_list(myGeometry, event2->vertexPos);
    std::vector<TVector3> alphaEndList = get_correct_pos_list(myGeometry, event2->alpha_endPos);
    std::vector<TVector3> carbonEndList = get_correct_pos_list(myGeometry, event2->carbon_endPos);

    for(auto &pos : vertexList) {
      auto diff = event2->vertexPos - pos; // [mm]
      auto w = 1./combinatorics_factor_point; // weight
      deltaX_all->Fill( diff.X(), w );
      deltaX_2prong->Fill( diff.X(), w );
      deltaY_all->Fill( diff.Y(), w );
      deltaY_2prong->Fill( diff.Y(), w );
      deltaXY_all->Fill( diff.X(), diff.Y(), w );
      deltaXY_2prong->Fill( diff.X(), diff.Y(), w );
    }
    for(auto &pos1 : alphaEndList) {
      auto diff = event2->alpha_endPos - pos1; // [mm]
      auto w = 1./combinatorics_factor_point; // weight
      deltaX_all->Fill( diff.X(), w );
      deltaX_2prong->Fill( diff.X(), w );
      deltaY_all->Fill( diff.Y(), w );
      deltaY_2prong->Fill( diff.Y(), w );
      deltaXY_all->Fill( diff.X(), diff.Y(), w );
      deltaXY_2prong->Fill( diff.X(), diff.Y(), w );
    }
    for(auto &pos2 : carbonEndList) {
      auto diff = event2->carbon_endPos - pos2; // [mm]
      auto w = 1./combinatorics_factor_point; // weight
      deltaX_all->Fill( diff.X(), w );
      deltaX_2prong->Fill( diff.X(), w );
      deltaY_all->Fill( diff.Y(), w );
      deltaY_2prong->Fill( diff.Y(), w );
      deltaXY_all->Fill( diff.X(), diff.Y(), w );
      deltaXY_2prong->Fill( diff.X(), diff.Y(), w );
    }

    length_bad_all->Fill( event2->alpha_length );
    length_bad_all->Fill( event2->carbon_length );
    length_bad_2prong->Fill( event2->alpha_length );
    length_bad_2prong->Fill( event2->carbon_length );
    alphaLength_bad_2prong->Fill( event2->alpha_length );
    carbonLength_bad_2prong->Fill( event2->carbon_length );
    sumLength_bad_2prong->Fill( event2->alpha_length + event2->carbon_length );
    for(auto &pos : vertexList) {
      for(auto &pos1 : alphaEndList) {
	auto len = (pos1-pos).Mag(); // [mm]
	auto diff = event2->alpha_length - len; // [mm]
	auto w = 1./combinatorics_factor_length; // weight
	deltaLength_all->Fill( diff, w );
	deltaLength_2prong->Fill( diff, w );
	deltaLengthVsLength_all->Fill( event2->alpha_length, diff, w );
	deltaLengthVsLength_2prong->Fill( event2->alpha_length, diff, w );
	alphaLength_corr_2prong->Fill( len, w );
	length_corr_all->Fill( len, w );
	length_corr_2prong->Fill( len, w );
      }
      for(auto &pos2 : carbonEndList) {
	auto len = (pos2-pos).Mag(); // [mm]
	auto diff = event2->carbon_length - len; // [mm]
	auto w = 1./combinatorics_factor_length; // weight
	deltaLength_all->Fill( diff, w );
	deltaLength_2prong->Fill( diff, w );
	deltaLengthVsLength_all->Fill( event2->carbon_length, diff, w );
	deltaLengthVsLength_2prong->Fill( event2->carbon_length, diff, w );
	carbonLength_corr_2prong->Fill( len, w );
	length_corr_all->Fill( len, w );
	length_corr_2prong->Fill( len, w );
      }
      for(auto &pos1 : alphaEndList) {
	for(auto &pos2 : carbonEndList) {
	  auto lenSum=(pos1-pos).Mag()+(pos2-pos).Mag(); // [mm]
	  auto diffSum=(event2->alpha_length + event2->carbon_length) - lenSum; // [mm]
	  auto w = 1./combinatorics_factor_2prong; // weight
	  deltaSumLength_2prong->Fill( diffSum, w );
	  sumLength_corr_2prong->Fill( lenSum, w );
	}
      }
    }
  }

  // loop over 3-prong tree
  for(auto i=0; i<reco_tree_3prong->GetEntries(); i++){
    reco_tree_3prong->GetEntry(i);

    std::vector<TVector3> vertexList = get_correct_pos_list(myGeometry, event3->vertexPos);
    std::vector<TVector3> alpha1EndList = get_correct_pos_list(myGeometry, event3->alpha1_endPos);
    std::vector<TVector3> alpha2EndList = get_correct_pos_list(myGeometry, event3->alpha2_endPos);
    std::vector<TVector3> alpha3EndList = get_correct_pos_list(myGeometry, event3->alpha3_endPos);

    for(auto &pos : vertexList) {
      auto diff = event3->vertexPos - pos; // [mm]
      auto w = 1./combinatorics_factor_point; // weight
      deltaX_all->Fill( diff.X(), w );
      deltaX_3prong->Fill( diff.X(), w );
      deltaY_all->Fill( diff.Y(), w );
      deltaY_3prong->Fill( diff.Y(), w );
      deltaXY_all->Fill( diff.X(), diff.Y(), w );
      deltaXY_3prong->Fill( diff.X(), diff.Y(), w );
    }
    for(auto &pos1 : alpha1EndList) {
      auto diff = event3->alpha1_endPos - pos1; // [mm]
      auto w = 1./combinatorics_factor_point; // weight
      deltaX_all->Fill( diff.X(), w );
      deltaX_3prong->Fill( diff.X(), w );
      deltaY_all->Fill( diff.Y(), w );
      deltaY_3prong->Fill( diff.Y(), w );
      deltaXY_all->Fill( diff.X(), diff.Y(), w );
      deltaXY_3prong->Fill( diff.X(), diff.Y(), w );
    }
    for(auto &pos2 : alpha2EndList) {
      auto diff = event3->alpha2_endPos - pos2; // [mm]
      auto w = 1./combinatorics_factor_point; // weight
      deltaX_all->Fill( diff.X(), w );
      deltaX_3prong->Fill( diff.X(), w );
      deltaY_all->Fill( diff.Y(), w );
      deltaY_3prong->Fill( diff.Y(), w );
      deltaXY_all->Fill( diff.X(), diff.Y(), w );
      deltaXY_3prong->Fill( diff.X(), diff.Y(), w );
    }
    for(auto &pos3 : alpha3EndList) {
      auto diff = event3->alpha3_endPos - pos3; // [mm]
      auto w = 1./combinatorics_factor_point; // weight
      deltaX_all->Fill( diff.X(), w );
      deltaX_3prong->Fill( diff.X(), w );
      deltaY_all->Fill( diff.Y(), w );
      deltaY_3prong->Fill( diff.Y(), w );
      deltaXY_all->Fill( diff.X(), diff.Y(), w );
      deltaXY_3prong->Fill( diff.X(), diff.Y(), w );
    }

    length_bad_all->Fill( event3->alpha1_length );
    length_bad_all->Fill( event3->alpha2_length );
    length_bad_all->Fill( event3->alpha3_length );
    length_bad_3prong->Fill( event3->alpha1_length );
    length_bad_3prong->Fill( event3->alpha2_length );
    length_bad_3prong->Fill( event3->alpha3_length );
    alpha1Length_bad_3prong->Fill( event3->alpha1_length );
    alpha2Length_bad_3prong->Fill( event3->alpha2_length );
    alpha3Length_bad_3prong->Fill( event3->alpha3_length );
    sumLength_bad_3prong->Fill( event3->alpha1_length + event3->alpha2_length + event3->alpha3_length );
    for(auto &pos : vertexList) {
      for(auto &pos1 : alpha1EndList) {
	auto len = (pos1-pos).Mag(); // [mm]
	auto diff = event3->alpha1_length - len; // [mm]
	auto w = 1./combinatorics_factor_length; // weight
	deltaLength_all->Fill( diff, w );
	deltaLength_3prong->Fill( diff, w );
	deltaLengthVsLength_all->Fill( event3->alpha1_length, diff, w );
	deltaLengthVsLength_3prong->Fill( event3->alpha1_length, diff, w );
	alpha1Length_corr_3prong->Fill( len, w );
	length_corr_all->Fill( len, w );
	length_corr_3prong->Fill( len, w );
      }
      for(auto &pos2 : alpha2EndList) {
	auto len = (pos2-pos).Mag(); // [mm]
	auto diff = event3->alpha2_length - len; // [mm]
	auto w = 1./combinatorics_factor_length; // weight
	deltaLength_all->Fill( diff, w );
	deltaLength_3prong->Fill( diff, w );
	deltaLengthVsLength_all->Fill( event3->alpha2_length, diff, w );
	deltaLengthVsLength_3prong->Fill( event3->alpha2_length, diff, w );
	alpha2Length_corr_3prong->Fill( len, w );
	length_corr_all->Fill( len, w );
	length_corr_3prong->Fill( len, w );
      }
      for(auto &pos3 : alpha3EndList) {
	auto len = (pos3-pos).Mag();
	auto diff = event3->alpha3_length - len;
	auto w = 1./combinatorics_factor_length; // weight
	deltaLength_all->Fill( diff, w );
	deltaLength_3prong->Fill( diff, w );
	deltaLengthVsLength_all->Fill( event3->alpha3_length, diff, w );
	deltaLengthVsLength_3prong->Fill( event3->alpha3_length, diff, w );
	alpha3Length_corr_3prong->Fill( len, w );
	length_corr_all->Fill( len, w );
	length_corr_3prong->Fill( len, w );
      }

      for(auto &pos1 : alpha1EndList) {
	for(auto &pos2 : alpha2EndList) {
	  for(auto &pos3 : alpha3EndList) {
	    auto lenSum=(pos3-pos).Mag()+(pos2-pos).Mag()+(pos1-pos).Mag(); // [mm]
	    auto diffSum=(event3->alpha1_length + event3->alpha2_length + event3->alpha3_length) - lenSum; // [mm]
	    auto w = 1./combinatorics_factor_3prong; // weight
	    deltaSumLength_3prong->Fill( diffSum, w );
	    sumLength_corr_3prong->Fill( lenSum, w );
	  }
	}
      }
    }
  }

  // set permanent drawing option to be stored in a ROOT file
  deltaX_all->SetOption("HIST");
  deltaX_all->SetOption("HIST");
  deltaY_all->SetOption("HIST");
  deltaXY_all->SetOption("BOX");
  length_corr_all->SetOption("HIST");
  length_bad_all->SetOption("HIST");
  deltaLength_all->SetOption("HIST");
  deltaLengthVsLength_all->SetOption("BOX");
  deltaX_1prong->SetOption("HIST");
  deltaY_1prong->SetOption("HIST");
  deltaXY_1prong->SetOption("BOX");
  length_corr_1prong->SetOption("HIST");
  length_bad_1prong->SetOption("HIST");
  deltaLength_1prong->SetOption("HIST");
  deltaLengthVsLength_1prong->SetOption("BOX");
  deltaX_2prong->SetOption("HIST");
  deltaY_2prong->SetOption("HIST");
  deltaXY_2prong->SetOption("BOX");
  alphaLength_corr_2prong->SetOption("HIST");
  alphaLength_bad_2prong->SetOption("HIST");
  carbonLength_corr_2prong->SetOption("HIST");
  carbonLength_bad_2prong->SetOption("HIST");
  sumLength_corr_2prong->SetOption("HIST");
  sumLength_bad_2prong->SetOption("HIST");
  deltaSumLength_2prong->SetOption("HIST");
  deltaLength_2prong->SetOption("HIST");
  deltaLengthVsLength_2prong->SetOption("BOX");
  deltaX_3prong->SetOption("HIST");
  deltaY_3prong->SetOption("HIST");
  deltaXY_3prong->SetOption("BOX");
  alpha1Length_corr_3prong->SetOption("HIST");
  alpha1Length_bad_3prong->SetOption("HIST");
  alpha2Length_corr_3prong->SetOption("HIST");
  alpha2Length_bad_3prong->SetOption("HIST");
  alpha3Length_corr_3prong->SetOption("HIST");
  alpha3Length_bad_3prong->SetOption("HIST");
  sumLength_corr_3prong->SetOption("HIST");
  sumLength_bad_3prong->SetOption("HIST");
  deltaSumLength_3prong->SetOption("HIST");
  deltaLength_3prong->SetOption("HIST");
  deltaLengthVsLength_3prong->SetOption("BOX");

  // plot all histograms
  window->cd();
  window->Print("Debug_plots.pdf[");
  deltaX_all->Draw();                 window->Print("Debug_plots.pdf");
  deltaY_all->Draw();                 window->Print("Debug_plots.pdf");
  deltaXY_all->Draw();                window->Print("Debug_plots.pdf");
  //  length_corr_all->Draw();            window->Print("Debug_plots.pdf");
  //  length_bad_all->Draw();             window->Print("Debug_plots.pdf");
  overlay_histograms(length_corr_all, length_bad_all, kRed, kBlue+2, "Track length [mm]");
  window->Print("Debug_plots.pdf");
  draw_difference_by_bin(length_corr_all, length_bad_all, kRed, "Track length [mm]");
  window->Print("Debug_plots.pdf");
  deltaLength_all->Draw();            window->Print("Debug_plots.pdf");
  deltaLengthVsLength_all->Draw();    window->Print("Debug_plots.pdf");
  deltaX_1prong->Draw();              window->Print("Debug_plots.pdf");
  deltaY_1prong->Draw();              window->Print("Debug_plots.pdf");
  deltaXY_1prong->Draw();             window->Print("Debug_plots.pdf");
  //  length_corr_1prong->Draw();         window->Print("Debug_plots.pdf");
  //  length_bad_1prong->Draw();          window->Print("Debug_plots.pdf");
  overlay_histograms(length_corr_1prong, length_bad_1prong, kRed, kBlue+2, "Track length [mm]");
  window->Print("Debug_plots.pdf");
  draw_difference_by_bin(length_corr_1prong, length_bad_1prong, kRed, "Track length [mm]");
  window->Print("Debug_plots.pdf");
  deltaLength_1prong->Draw();         window->Print("Debug_plots.pdf");
  deltaLengthVsLength_1prong->Draw(); window->Print("Debug_plots.pdf");
  deltaX_2prong->Draw();              window->Print("Debug_plots.pdf");
  deltaY_2prong->Draw();              window->Print("Debug_plots.pdf");
  deltaXY_2prong->Draw();             window->Print("Debug_plots.pdf");
  //  alphaLength_corr_2prong->Draw();    window->Print("Debug_plots.pdf");
  //  alphaLength_bad_2prong->Draw();     window->Print("Debug_plots.pdf");
  overlay_histograms(length_corr_2prong, length_bad_2prong, kRed, kBlue+2, "Track length [mm]");
  window->Print("Debug_plots.pdf");
  draw_difference_by_bin(length_corr_2prong, length_bad_2prong, kRed, "Track length [mm]");
  window->Print("Debug_plots.pdf");
  overlay_histograms(alphaLength_corr_2prong, alphaLength_bad_2prong, kRed, kBlue+2, "#alpha track length [mm]");
  window->Print("Debug_plots.pdf");
  draw_difference_by_bin(alphaLength_corr_2prong, alphaLength_bad_2prong, kRed, "#alpha track length [mm]");
  window->Print("Debug_plots.pdf");
  //  carbonLength_corr_2prong->Draw();   window->Print("Debug_plots.pdf");
  //  carbonLength_bad_2prong->Draw();    window->Print("Debug_plots.pdf");
  overlay_histograms(carbonLength_corr_2prong, carbonLength_bad_2prong, kRed, kBlue+2, "Carbon track length [mm]");
  window->Print("Debug_plots.pdf");
  draw_difference_by_bin(carbonLength_corr_2prong, carbonLength_bad_2prong, kRed, "Carbon track length [mm]");
  window->Print("Debug_plots.pdf");
  //  sumLength_corr_2prong->Draw();      window->Print("Debug_plots.pdf");
  //  sumLength_bad_2prong->Draw();       window->Print("Debug_plots.pdf");
  overlay_histograms(sumLength_corr_2prong, sumLength_bad_2prong, kRed, kBlue+2, "#alpha+C track length sum [mm]");
  window->Print("Debug_plots.pdf");
  draw_difference_by_bin(sumLength_corr_2prong, sumLength_bad_2prong, kRed, "#alpha+C track length sum [mm]");
  window->Print("Debug_plots.pdf");
  deltaSumLength_2prong->Draw();      window->Print("Debug_plots.pdf");
  deltaLength_2prong->Draw();         window->Print("Debug_plots.pdf");
  deltaLengthVsLength_2prong->Draw(); window->Print("Debug_plots.pdf");
  deltaX_3prong->Draw();              window->Print("Debug_plots.pdf");
  deltaY_3prong->Draw();              window->Print("Debug_plots.pdf");
  deltaXY_3prong->Draw();             window->Print("Debug_plots.pdf");
  //  alpha1Length_corr_3prong->Draw();   window->Print("Debug_plots.pdf");
  //  alpha1Length_bad_3prong->Draw();    window->Print("Debug_plots.pdf");
  overlay_histograms(length_corr_3prong, length_bad_3prong, kRed, kBlue+2, "Track length [mm]");
  window->Print("Debug_plots.pdf");
  draw_difference_by_bin(length_corr_3prong, length_bad_3prong, kRed, "Track length [mm]");
  window->Print("Debug_plots.pdf");
  overlay_histograms(alpha1Length_corr_3prong, alpha1Length_bad_3prong, kRed, kBlue+2, "#alpha_{1} track length [mm]");
  window->Print("Debug_plots.pdf");
  draw_difference_by_bin(alpha1Length_corr_3prong, alpha1Length_bad_3prong, kRed, "#alpha_{1} track length [mm]");
  window->Print("Debug_plots.pdf");
  //  alpha2Length_corr_3prong->Draw();   window->Print("Debug_plots.pdf");
  //  alpha2Length_bad_3prong->Draw();    window->Print("Debug_plots.pdf");
  overlay_histograms(alpha2Length_corr_3prong, alpha2Length_bad_3prong, kRed, kBlue+2, "#alpha_{2} track length [mm]");
  window->Print("Debug_plots.pdf");
  draw_difference_by_bin(alpha2Length_corr_3prong, alpha2Length_bad_3prong, kRed, "#alpha_{2} track length [mm]");
  window->Print("Debug_plots.pdf");
  //  alpha3Length_corr_3prong->Draw();   window->Print("Debug_plots.pdf");
  //  alpha3Length_bad_3prong->Draw();    window->Print("Debug_plots.pdf");
  overlay_histograms(alpha3Length_corr_3prong, alpha3Length_bad_3prong, kRed, kBlue+2, "#alpha_{3} track length [mm]");
  window->Print("Debug_plots.pdf");
  draw_difference_by_bin(alpha3Length_corr_3prong, alpha3Length_bad_3prong, kRed, "#alpha_{3} track length [mm]");
  window->Print("Debug_plots.pdf");
  //  sumLength_corr_3prong->Draw();      window->Print("Debug_plots.pdf");
  //  sumLength_bad_3prong->Draw();       window->Print("Debug_plots.pdf");
  overlay_histograms(sumLength_corr_3prong, sumLength_bad_3prong, kRed, kBlue+2, "#alpha_{1}+#alpha_{2}+#alpha_{3} track length sum [mm]");
  window->Print("Debug_plots.pdf");
  draw_difference_by_bin(sumLength_corr_3prong, sumLength_bad_3prong, kRed, "#alpha_{1}+#alpha_{2}+#alpha_{3} track length sum [mm]");
  window->Print("Debug_plots.pdf");
  deltaSumLength_3prong->Draw();      window->Print("Debug_plots.pdf");
  deltaLength_3prong->Draw();         window->Print("Debug_plots.pdf");
  deltaLengthVsLength_3prong->Draw(); window->Print("Debug_plots.pdf");
  window->Print("Debug_plots.pdf]");

  // write all histograms
  out_file->cd();
  out_file->Write();
  out_file->Close();

  std::cout << std::endl
	    << "NOTE: This macro is valid only for RECO files produced with releases <= higs_06 (including higs_06.1)." << std::endl
	    << "      The histogram scaling bug is now corrected in the master branch as of Dec 2022." << std::endl << std::endl;

  return 0;
}
