//////////////////////////
//
// This macro makes pretty-looking histograms out of manually-saved TCanvas during interactive tpcGUI session.
//
// NOTE: For automatic reconstruction of individually-selected events modify GUI/src/HistoManager.cpp
// and enable drawDevelHisots() to execute dE/dx fitter in RECO mode (this step is required for plotBraggFit).
//
// Instructions:
// 1. Start tpcGUI with relevant GRAW file(s).
//    Enable or disable clustering depending on your plot needs.
// 2. Pick your event of interest.
// 3. Enable or disable AUTOZOOM mode.
// 4. For UVW raw signals in (CHANNEL) x (TIME_CELL) domain save the entire TCanvas (TCanvas::SaveAs) in ROOT format.
//    Suggested file name format is: runYYYYMMDDhhmmss_evtNNNN_raw.root
// 5. For UVW raw signals in MM x MM enable RECO mode first and then save the entire TCanvas (TCanvas::SaveAs) in ROOT format.
//    Keep in mind that clustering settings will apply. In development mode a dE/dx fit should be visible in the 4th TPad. 
//    Suggested file name formats are: runYYYYMMDDhhmmss_evtNNNN_rawMM.root
//                                     runYYYYMMDDhhmmss_evtNNNN_clusteredMM.root
// 6. Quit tpcGUI.
// 7. Run ROOT macro as follows:
// root
// root [0] .L makePlots_eventFromTPCgui.cxx
// root [1] plotRawSignals("craw.root", 20220413172327, 5114); // 3-prong
// root [2] plotRawSignalsInMM("crawMM.root", 20220413172327, 5114, 70, 70, -10, 28, -25, -45); // 3-prong evt=5114, 70mm x 70mm zoom cenetered at U/V/W/Z[mm]=-10/28/-25/-45
// root [3] plotRawSignalsInMM("crawMM.root", 20220413172327, 243, 90, 90, 30, 80, -110, -40); // 2-prong evt=243, 90mm x 90mm zoom cenetered at U/V/W/Z[mm]=30/80/-110/-40
// root [4] plotBraggFit("cfit.root", 20220413172327, 243, 94.3, 5.9); // fit of 2-prong evt=243, fitted length=94.3 mm, fitted total energy=5.9 MeV
//
// NOTE: The resulting plots will be saved as:
//       run20220413172327_evt5114_raw_zoom_linear_UVW.root
//       run20220413172327_evt5114_rawMM_zoom_linear_UVW.root
//       run20220413172327_evt243_rawMM_zoom_linear_UVW.root
//       run20220413172327_evt243_fit.root
//
// #ifndef __ROOTLOGON__
// R__ADD_INCLUDE_PATH(../../Analysis/include)
// R__ADD_INCLUDE_PATH(../../DataFormats/include)
// R__ADD_INCLUDE_PATH(../../Utilities/include)
// R__ADD_LIBRARY_PATH(../lib)
// #endif

#include <iostream>
#include <string>
#include <vector>
#include <string>
#include <TFile.h>
#include <TCanvas.h>
#include <TPad.h>
#include <TH1D.h>
#include <TH2D.h>
#include <TStyle.h>
#include <TAxis.h>
#include <TGaxis.h>
#include <TLatex.h>
#include <TString.h>
#include <TLine.h>
#include <TColor.h>
#include <TLegend.h>
#include <TPaveStats.h>

// #include "TPCReco/GeometryTPC.h"
// #include "TPCReco/HIGS_trees_dataFormat.h"

void plotRawSignals(const char *tcanvas_root="c.root",
		    long run=20220413172327,
		    long evt=5114,
		    double zoom_sx=512, // time cells [0-511]
		    double zoom_sy=134, // strips
		    double center_u=134/2, // strips
		    double center_v=233/2, // strips
		    double center_w=233/2, // strips
		    double center_t=512/2) { // time cells [0-511]
  gStyle->SetOptTitle(0); // no title
  gStyle->SetOptStat(0); // no stat box
  gStyle->SetLabelOffset(0.02,"XY");
  gStyle->SetLabelSize(0.04,"XYZ");
  gStyle->SetLabelSize(0.035,"Z");
  gStyle->SetTitleSize(0.04,"XYZ");
  gStyle->SetTitleOffset(1.35,"X");
  gStyle->SetTitleOffset(1.6,"Y");
  gStyle->SetTitleOffset(1.33,"Z");
  gStyle->SetTitleFont(62,"XYZ");
  //  gStyle->SetTickLength(0.0, "XY");
  TFile *f = TFile::Open(tcanvas_root, "OLD");
  if(!f) {
    std::cout << "Cannot open input ROOT file: " << tcanvas_root << "!" << std::endl;
    return;
  }
  auto Histograms=(TCanvas*)(f->Get("Histograms"));
  if(!Histograms) {
    std::cout << "Cannot find TCanvas with name 'Histograms'!" << std::endl;
    return;
  }
  Histograms->ls();
  Histograms->Draw();
  auto c = new TCanvas("c", "c", 700*3, 600); // 800*3, 600); // 900*3, 600);
  c->Clear();
  c->SetLeftMargin(0.01);c->SetRightMargin(0.01);
  c->SetBottomMargin(0.01);c->SetTopMargin(0.01);
  c->Divide(3,1,0,0);
  auto ipad=0;
  c->cd(++ipad);
  TH2D *h2;
  h2 = (TH2D*)(Histograms->FindObject(Form("hraw_U_vs_time_evt%ld_copy", evt)))->DrawClone("COLZ");
  h2->UseCurrentStyle();
  gPad->SetLeftMargin(Histograms->GetPad(1)->GetLeftMargin()-1*0.025);
  gPad->SetRightMargin(Histograms->GetPad(1)->GetRightMargin()+0.01);
  gPad->SetTopMargin(Histograms->GetPad(1)->GetTopMargin()-0.090);
  gPad->SetBottomMargin(Histograms->GetPad(1)->GetBottomMargin());
  gPad->Update();
  gPad->Modified();
  std::cout << ipad << ": margin x=[" << gPad->GetLeftMargin() << "," << gPad->GetRightMargin() << "] y=[" << gPad->GetBottomMargin() << "," << gPad->GetTopMargin() << "]" << std::endl;
  h2->SetTitle(Form("Event %ld: UT projection;Time cell;U-strip number;",evt)); // ";Charge [arb.u.]"); // skip Z-axis title
  h2->SetTitle(""); // skip title for publication plot
  h2->GetXaxis()->SetRangeUser(center_t-zoom_sx/2, center_t+zoom_sx/2); h2->GetYaxis()->SetRangeUser(center_u-zoom_sy/2, center_u+zoom_sy/2);
  //  h2->GetXaxis()->SetNdivisions(210);
  //  h2->GetYaxis()->SetNdivisions(210);
  h2->GetZaxis()->UnZoom();
  TPaletteAxis *palette = (TPaletteAxis*) h2->GetListOfFunctions()->FindObject("palette");
  palette->SetY2NDC(1.0-gPad->GetTopMargin());
  gPad->Update();
  gPad->Modified();
  std::cout << ipad << ": U range x=[" << gPad->GetUxmin() << "," << gPad->GetUxmax() << "] y=[" << gPad->GetUymin() << "," << gPad->GetUymax() << "]" << std::endl;
  std::cout << ipad << ": A range x=[" << h2->GetXaxis()->GetXmin() << "," << h2->GetXaxis()->GetXmax() << "] y=[" << h2->GetYaxis()->GetXmin() << "," << h2->GetYaxis()->GetXmax() << "]" << std::endl;
  //  TGaxis *axis;
  //  axis = new TGaxis(gPad->GetUxmin(),gPad->GetUymin(),gPad->GetUxmax(),gPad->GetUymin(),gPad->GetUxmin(),gPad->GetUxmax(),h2->GetXaxis()->GetNdivisions(),"SUB-");
  //  axis->SetTickSize(0.02);
  //  axis->Draw();
  //  axis = new TGaxis(gPad->GetUxmin(),gPad->GetUymin(),gPad->GetUxmin(),gPad->GetUymax(),gPad->GetUymin(),gPad->GetUymax(),h2->GetYaxis()->GetNdivisions(),"SUB+");
  //  axis->SetTickSize(0.02*(h2->GetXaxis()->GetXmax()-h2->GetXaxis()->GetXmin())/(h2->GetYaxis()->GetXmax()-h2->GetYaxis()->GetXmin())); // forces same tick length as for X axis
  //  axis->Draw();
  c->cd(++ipad);
  h2 = (TH2D*)(Histograms->FindObject(Form("hraw_V_vs_time_evt%ld_copy", evt)))->DrawClone("COLZ");
  h2->UseCurrentStyle();
  gPad->SetLeftMargin(Histograms->GetPad(2)->GetLeftMargin()-1*0.025);
  gPad->SetRightMargin(Histograms->GetPad(2)->GetRightMargin()+0.01);
  gPad->SetTopMargin(Histograms->GetPad(2)->GetTopMargin()-0.090);
  gPad->SetBottomMargin(Histograms->GetPad(2)->GetBottomMargin());
  gPad->Update();
  gPad->Modified();
  std::cout << ipad << ": margin x=[" << gPad->GetLeftMargin() << "," << gPad->GetRightMargin() << "] y=[" << gPad->GetBottomMargin() << "," << gPad->GetTopMargin() << "]" << std::endl;
  h2->SetTitle(Form("Event %ld: VT projection;Time cell;V-strip number;",evt)); // ";Charge [arb.u.]"); // skip Z-axis title
  h2->SetTitle(""); // skip title for publication plot
  h2->GetXaxis()->SetRangeUser(center_t-zoom_sx/2, center_t+zoom_sx/2); h2->GetYaxis()->SetRangeUser(center_v-zoom_sy/2, center_v+zoom_sy/2);
  //  h2->GetXaxis()->SetNdivisions(210);
  //  h2->GetYaxis()->SetNdivisions(210);
  palette = (TPaletteAxis*) h2->GetListOfFunctions()->FindObject("palette");
  palette->SetY2NDC(1.0-gPad->GetTopMargin());
  gPad->Update();
  gPad->Modified();
  std::cout << ipad << ": U range x=[" << gPad->GetUxmin() << "," << gPad->GetUxmax() << "] y=[" << gPad->GetUymin() << "," << gPad->GetUymax() << "]" << std::endl;
  std::cout << ipad << ": A range x=[" << h2->GetXaxis()->GetXmin() << "," << h2->GetXaxis()->GetXmax() << "] y=[" << h2->GetYaxis()->GetXmin() << "," << h2->GetYaxis()->GetXmax() << "]" << std::endl;
  //  axis = new TGaxis(gPad->GetUxmin(),gPad->GetUymin(),gPad->GetUxmax(),gPad->GetUymin(),gPad->GetUxmin(),gPad->GetUxmax(),h2->GetXaxis()->GetNdivisions(),"SUB-");
  //  axis->SetTickSize(0.02);
  //  axis->Draw();
  //  axis = new TGaxis(gPad->GetUxmin(),gPad->GetUymin(),gPad->GetUxmin(),gPad->GetUymax(),gPad->GetUymin(),gPad->GetUymax(),h2->GetYaxis()->GetNdivisions(),"SUB+");
  //  axis->SetTickSize(0.02*(h2->GetXaxis()->GetXmax()-h2->GetXaxis()->GetXmin())/(h2->GetYaxis()->GetXmax()-h2->GetYaxis()->GetXmin())); // forces same tick length as for X axis
  //  axis->Draw();
  c->cd(++ipad);
  h2 = (TH2D*)(Histograms->FindObject(Form("hraw_W_vs_time_evt%ld_copy", evt)))->DrawClone("COLZ");
  h2->UseCurrentStyle();
  gPad->SetLeftMargin(Histograms->GetPad(3)->GetLeftMargin()-1*0.025);
  gPad->SetRightMargin(Histograms->GetPad(3)->GetRightMargin()+0.01);
  gPad->SetTopMargin(Histograms->GetPad(3)->GetTopMargin()-0.090);
  gPad->SetBottomMargin(Histograms->GetPad(3)->GetBottomMargin());
  gPad->Update();
  gPad->Modified();
  std::cout << ipad << ": margin x=[" << gPad->GetLeftMargin() << "," << gPad->GetRightMargin() << "] y=[" << gPad->GetBottomMargin() << "," << gPad->GetTopMargin() << "]" << std::endl;
  h2->SetTitle(Form("Event %ld: WT projection;Time cell;W-strip number;Charge [arb.u.]",evt));
  h2->SetTitle(""); // skip title for publication plot
  h2->GetXaxis()->SetRangeUser(center_t-zoom_sx/2, center_t+zoom_sx/2); h2->GetYaxis()->SetRangeUser(center_w-zoom_sy/2, center_w+zoom_sy/2);
  //  h2->GetXaxis()->SetNdivisions(210);
  //  h2->GetYaxis()->SetNdivisions(210);
  palette = (TPaletteAxis*) h2->GetListOfFunctions()->FindObject("palette");
  palette->SetY2NDC(1.0-gPad->GetTopMargin());
  gPad->Update();
  gPad->Modified();
  std::cout << ipad << ": U range x=[" << gPad->GetUxmin() << "," << gPad->GetUxmax() << "] y=[" << gPad->GetUymin() << "," << gPad->GetUymax() << "]" << std::endl;
  std::cout << ipad << ": A range x=[" << h2->GetXaxis()->GetXmin() << "," << h2->GetXaxis()->GetXmax() << "] y=[" << h2->GetYaxis()->GetXmin() << "," << h2->GetYaxis()->GetXmax() << "]" << std::endl;
  //  axis = new TGaxis(gPad->GetUxmin(),gPad->GetUymin(),gPad->GetUxmax(),gPad->GetUymin(),gPad->GetUxmin(),gPad->GetUxmax(),h2->GetXaxis()->GetNdivisions(),"SUB-");
  //  axis->SetTickSize(0.02);
  //  axis->Draw();
  //  axis = new TGaxis(gPad->GetUxmin(),gPad->GetUymin(),gPad->GetUxmin(),gPad->GetUymax(),gPad->GetUymin(),gPad->GetUymax(),h2->GetYaxis()->GetNdivisions(),"SUB+");
  //  axis->SetTickSize(0.02*(h2->GetXaxis()->GetXmax()-h2->GetXaxis()->GetXmin())/(h2->GetYaxis()->GetXmax()-h2->GetYaxis()->GetXmin())); // forces same tick length as for X axis
  //  axis->Draw();
  c->Print(Form("run%ld_evt%ld_raw_zoom_linear_UVW.root",run,evt));
  c->Print(Form("run%ld_evt%ld_raw_zoom_linear_UVW.pdf",run,evt));
  f->Close();
}

void plotRawSignalsMM(const char *tcanvas_root="c.root",
		      long run=20220413172327,
		      long evt=5114,  
		      double zoom_sx=70.0, // mm
		      double zoom_sy=70.0, // mm
		      double center_u=-10.0, // mm
		      double center_v=28.0, // mm
		      double center_w=-25.0, // mm
		      double center_t=-45.0) { // mm
  gStyle->SetOptTitle(0); // no title
  gStyle->SetOptStat(0); // no stat box
  gStyle->SetLabelOffset(0.02,"XY");
  gStyle->SetLabelSize(0.04,"XYZ");
  gStyle->SetLabelSize(0.035,"Z");
  gStyle->SetTitleSize(0.04,"XYZ");
  gStyle->SetTitleOffset(1.35,"X");
  gStyle->SetTitleOffset(1.6,"Y");
  gStyle->SetTitleOffset(1.33,"Z");
  gStyle->SetTitleFont(62,"XYZ");
  gStyle->SetTickLength(0.0, "XY");
  TFile *f = TFile::Open(tcanvas_root, "OLD");
  if(!f) {
    std::cout << "Cannot open input ROOT file: " << tcanvas_root << "!" << std::endl;
    return;
  }
  auto Histograms=(TCanvas*)(f->Get("Histograms"));
  if(!Histograms) {
    std::cout << "Cannot find TCanvas with name 'Histograms'!" << std::endl;
    return;
  }
  Histograms->ls();
  Histograms->Draw();
  auto c = new TCanvas("c", "c", 700*3, 600); // 800*3, 600); // 900*3, 600);
  c->Clear();
  c->SetLeftMargin(0.01);c->SetRightMargin(0.01);
  c->SetBottomMargin(0.01);c->SetTopMargin(0.01);
  c->Divide(3,1,0,0);
  auto ipad=0;
  c->cd(++ipad);
  TH2D *h2;
  h2 = (TH2D*)(Histograms->FindObject(Form("hraw_U_vs_time_mm_evt%ld_copy", evt)))->DrawClone("COLZ");
  h2->UseCurrentStyle();
  gPad->SetLeftMargin(Histograms->GetPad(1)->GetLeftMargin()-1*0.025);
  gPad->SetRightMargin(Histograms->GetPad(1)->GetRightMargin()+0.01);
  gPad->SetTopMargin(Histograms->GetPad(1)->GetTopMargin()-0.090);
  gPad->SetBottomMargin(Histograms->GetPad(1)->GetBottomMargin());
  gPad->Update();
  gPad->Modified();
  std::cout << ipad << ": margin x=[" << gPad->GetLeftMargin() << "," << gPad->GetRightMargin() << "] y=[" << gPad->GetBottomMargin() << "," << gPad->GetTopMargin() << "]" << std::endl;
  h2->SetTitle(Form("Event %ld: UZ projection;Z [mm];U [mm];",evt)); // ";Charge [arb.u.]"); // skip Z-axis title
  h2->SetTitle(""); // skip title for publication plot
  h2->GetXaxis()->SetRangeUser(center_t-zoom_sx/2, center_t+zoom_sx/2); h2->GetYaxis()->SetRangeUser(center_u-zoom_sy/2, center_u+zoom_sy/2);
  h2->GetXaxis()->SetNdivisions(210);
  h2->GetYaxis()->SetNdivisions(210);
  h2->GetZaxis()->UnZoom();
  TPaletteAxis *palette = (TPaletteAxis*) h2->GetListOfFunctions()->FindObject("palette");
  palette->SetY2NDC(1.0-gPad->GetTopMargin());
  gPad->Update();
  gPad->Modified();
  std::cout << ipad << ": U range x=[" << gPad->GetUxmin() << "," << gPad->GetUxmax() << "] y=[" << gPad->GetUymin() << "," << gPad->GetUymax() << "]" << std::endl;
  std::cout << ipad << ": A range x=[" << h2->GetXaxis()->GetXmin() << "," << h2->GetXaxis()->GetXmax() << "] y=[" << h2->GetYaxis()->GetXmin() << "," << h2->GetYaxis()->GetXmax() << "]" << std::endl;
  TGaxis *axis;
  axis = new TGaxis(gPad->GetUxmin(),gPad->GetUymin(),gPad->GetUxmax(),gPad->GetUymin(),gPad->GetUxmin(),gPad->GetUxmax(),h2->GetXaxis()->GetNdivisions(),"SUB-");
  axis->SetTickSize(0.02);
  axis->Draw();
  axis = new TGaxis(gPad->GetUxmin(),gPad->GetUymin(),gPad->GetUxmin(),gPad->GetUymax(),gPad->GetUymin(),gPad->GetUymax(),h2->GetYaxis()->GetNdivisions(),"SUB+");
  axis->SetTickSize(0.02*(h2->GetXaxis()->GetXmax()-h2->GetXaxis()->GetXmin())/(h2->GetYaxis()->GetXmax()-h2->GetYaxis()->GetXmin())); // forces same tick length as for X axis
  axis->Draw();
  c->cd(++ipad);
  h2 = (TH2D*)(Histograms->FindObject(Form("hraw_V_vs_time_mm_evt%ld_copy", evt)))->DrawClone("COLZ");
  h2->UseCurrentStyle();
  gPad->SetLeftMargin(Histograms->GetPad(2)->GetLeftMargin()-1*0.025);
  gPad->SetRightMargin(Histograms->GetPad(2)->GetRightMargin()+0.01);
  gPad->SetTopMargin(Histograms->GetPad(2)->GetTopMargin()-0.090);
  gPad->SetBottomMargin(Histograms->GetPad(2)->GetBottomMargin());
  gPad->Update();
  gPad->Modified();
  std::cout << ipad << ": margin x=[" << gPad->GetLeftMargin() << "," << gPad->GetRightMargin() << "] y=[" << gPad->GetBottomMargin() << "," << gPad->GetTopMargin() << "]" << std::endl;
  h2->SetTitle(Form("Event %ld: VZ projection;Z [mm];V [mm];",evt)); // ";Charge [arb.u.]"); // skip Z-axis title
  h2->SetTitle(""); // skip title for publication plot
  h2->GetXaxis()->SetRangeUser(center_t-zoom_sx/2, center_t+zoom_sx/2); h2->GetYaxis()->SetRangeUser(center_v-zoom_sy/2, center_v+zoom_sy/2);
  h2->GetXaxis()->SetNdivisions(210);
  h2->GetYaxis()->SetNdivisions(210);
  palette = (TPaletteAxis*) h2->GetListOfFunctions()->FindObject("palette");
  palette->SetY2NDC(1.0-gPad->GetTopMargin());
  gPad->Update();
  gPad->Modified();
  std::cout << ipad << ": U range x=[" << gPad->GetUxmin() << "," << gPad->GetUxmax() << "] y=[" << gPad->GetUymin() << "," << gPad->GetUymax() << "]" << std::endl;
  std::cout << ipad << ": A range x=[" << h2->GetXaxis()->GetXmin() << "," << h2->GetXaxis()->GetXmax() << "] y=[" << h2->GetYaxis()->GetXmin() << "," << h2->GetYaxis()->GetXmax() << "]" << std::endl;
  axis = new TGaxis(gPad->GetUxmin(),gPad->GetUymin(),gPad->GetUxmax(),gPad->GetUymin(),gPad->GetUxmin(),gPad->GetUxmax(),h2->GetXaxis()->GetNdivisions(),"SUB-");
  axis->SetTickSize(0.02);
  axis->Draw();
  axis = new TGaxis(gPad->GetUxmin(),gPad->GetUymin(),gPad->GetUxmin(),gPad->GetUymax(),gPad->GetUymin(),gPad->GetUymax(),h2->GetYaxis()->GetNdivisions(),"SUB+");
  axis->SetTickSize(0.02*(h2->GetXaxis()->GetXmax()-h2->GetXaxis()->GetXmin())/(h2->GetYaxis()->GetXmax()-h2->GetYaxis()->GetXmin())); // forces same tick length as for X axis
  axis->Draw();
  c->cd(++ipad);
  h2 = (TH2D*)(Histograms->FindObject(Form("hraw_W_vs_time_mm_evt%ld_copy", evt)))->DrawClone("COLZ");
  h2->UseCurrentStyle();
  gPad->SetLeftMargin(Histograms->GetPad(3)->GetLeftMargin()-1*0.025);
  gPad->SetRightMargin(Histograms->GetPad(3)->GetRightMargin()+0.01);
  gPad->SetTopMargin(Histograms->GetPad(3)->GetTopMargin()-0.090);
  gPad->SetBottomMargin(Histograms->GetPad(3)->GetBottomMargin());
  gPad->Update();
  gPad->Modified();
  std::cout << ipad << ": margin x=[" << gPad->GetLeftMargin() << "," << gPad->GetRightMargin() << "] y=[" << gPad->GetBottomMargin() << "," << gPad->GetTopMargin() << "]" << std::endl;
  h2->SetTitle(Form("Event %ld: WZ projection;Z [mm];W [mm];Charge [arb.u.]",evt));
  h2->SetTitle(""); // skip title for publication plot
  h2->GetXaxis()->SetRangeUser(center_t-zoom_sx/2, center_t+zoom_sx/2); h2->GetYaxis()->SetRangeUser(center_w-zoom_sy/2, center_w+zoom_sy/2);
  h2->GetXaxis()->SetNdivisions(210);
  h2->GetYaxis()->SetNdivisions(210);
  palette = (TPaletteAxis*) h2->GetListOfFunctions()->FindObject("palette");
  palette->SetY2NDC(1.0-gPad->GetTopMargin());
  gPad->Update();
  gPad->Modified();
  std::cout << ipad << ": U range x=[" << gPad->GetUxmin() << "," << gPad->GetUxmax() << "] y=[" << gPad->GetUymin() << "," << gPad->GetUymax() << "]" << std::endl;
  std::cout << ipad << ": A range x=[" << h2->GetXaxis()->GetXmin() << "," << h2->GetXaxis()->GetXmax() << "] y=[" << h2->GetYaxis()->GetXmin() << "," << h2->GetYaxis()->GetXmax() << "]" << std::endl;
  axis = new TGaxis(gPad->GetUxmin(),gPad->GetUymin(),gPad->GetUxmax(),gPad->GetUymin(),gPad->GetUxmin(),gPad->GetUxmax(),h2->GetXaxis()->GetNdivisions(),"SUB-");
  axis->SetTickSize(0.02);
  axis->Draw();
  axis = new TGaxis(gPad->GetUxmin(),gPad->GetUymin(),gPad->GetUxmin(),gPad->GetUymax(),gPad->GetUymin(),gPad->GetUymax(),h2->GetYaxis()->GetNdivisions(),"SUB+");
  axis->SetTickSize(0.02*(h2->GetXaxis()->GetXmax()-h2->GetXaxis()->GetXmin())/(h2->GetYaxis()->GetXmax()-h2->GetYaxis()->GetXmin())); // forces same tick length as for X axis
  axis->Draw();
  c->Print(Form("run%ld_evt%ld_rawMM_zoom_linear_UVW.root",run,evt));
  c->Print(Form("run%ld_evt%ld_rawMM_zoom_linear_UVW.pdf",run,evt));
  f->Close();
}

void plotBraggFit(const char *tcanvas_root="c.root",
		  long run=20220413172327,
		  long evt=243,
		  double total_length=0.0,   // mm <-- to be extracted from fit log file
		  double total_energy=0.0) { // MeV <-- to be extracted from fit log file
  gStyle->SetOptTitle(0); // no title
  gStyle->SetOptStat(0); // no stat box
  gStyle->SetFrameLineWidth(2); // thick frame
  TFile *f = TFile::Open(tcanvas_root, "OLD");
  f->cd();
  f->ls();
  if(!f) {
    std::cout << "Cannot open input ROOT file: " << tcanvas_root << "!" << std::endl;
    return;
  }
  TCanvas *Histograms=(TCanvas*)(f->Get("Histograms"));
  if(!Histograms) {
    std::cout << "Cannot find TCanvas with name 'Histograms'!" << std::endl;
    return;
  }
  Histograms->ls();
  Histograms->DrawClone();
  TF1 *fac, *fa, *fc;
  fac = (TF1*)(Histograms->FindObject("carbon_alpha_model"));
  fa = (TF1*)(Histograms->FindObject("alpha_model"));
  fc = (TF1*)(Histograms->FindObject("carbon_model"));
  TH1F *h1;
  h1 = (TH1F*)(Histograms->FindObject("hChargeProfile_copy"))->DrawClone("HIST");
  h1->SetDrawOption("HIST");
  h1->SetOption("HIST");
  h1->SetLineColor(kBlack);
  h1->SetLineWidth(4);
  auto c = new TCanvas("c", "c", 800, 600);
  c->cd();
  c->Clear();
  gPad->SetLeftMargin(Histograms->GetPad(4)->GetLeftMargin()-0.025);
  gPad->SetRightMargin(Histograms->GetPad(4)->GetRightMargin()-0.125);
  gPad->SetTopMargin(Histograms->GetPad(4)->GetTopMargin()-0.090);
  gPad->SetBottomMargin(Histograms->GetPad(4)->GetBottomMargin());
  h1->SetTitle("Projection of charge along track;d [mm];Charge [arb.u.]"); // skip title for publication plot
  h1->SetTitle(""); // skip title for publication plot
  h1->Draw();
  h1->GetXaxis()->SetTitleOffset(h1->GetXaxis()->GetTitleOffset()+0.4);
  h1->GetYaxis()->SetTitleOffset(h1->GetYaxis()->GetTitleOffset()+0.9);
  fc->SetLineWidth(3); // carbon
  fc->SetLineStyle(2);
  fc->SetLineColor(kBlue);
  fc->Draw("SAME");
  fa->SetLineWidth(3); // alpha
  fa->SetLineStyle(2);
  fa->SetLineColor(kGreen+2);
  fa->Draw("SAME");
  fac->SetLineWidth(3); // carbon + alpha
  fac->SetLineStyle(1);
  fac->SetLineColor(kRed);
  fac->Draw("SAME");
  h1->Draw("SAME HIST");
  auto textH=0.035, lineH=textH+0.025;
  auto x=0.65, y=0.65;
  TLatex aLatex;
  aLatex.SetTextSize(textH);
  aLatex.SetTextFont(42);
  if(total_length>0) aLatex.DrawLatexNDC(x, y,       Form("Total length: %g mm",  total_length));
  if(total_energy>0) aLatex.DrawLatexNDC(x, y-lineH, Form("Total energy: %g MeV", total_energy));
  TLegend *tl = new TLegend(0.6, 0.95-lineH*4, 0.95, 0.95);
  tl->AddEntry(h1,"Data", "FL");
  tl->AddEntry(fac, "#alpha+^{12}C hypothesis fit", "L");
  tl->AddEntry(fa, "#alpha contribution", "L");
  tl->AddEntry(fc, "^{12}C contribution", "L");
  tl->SetTextSize(textH);
  tl->Draw();
  gPad->Update();
  gPad->Modified();
  c->Print(Form("run%ld_evt%ld_fit.root",run,evt));
  f->Close();
}


