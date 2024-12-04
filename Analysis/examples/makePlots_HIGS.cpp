/*
Usage:

$ root -l
root [0] .L makePlots_HIGS.cpp
root [1] makePlots_HIGS("Histos.root", 8.66);

*/
//#undef __ROOTLOGON__
//#ifndef __ROOTLOGON__
R__ADD_INCLUDE_PATH(../../DataFormats/include)
R__ADD_INCLUDE_PATH(../../Utilities/include)
R__ADD_LIBRARY_PATH(../lib)
//#endif

#include <tuple>
#include <algorithm>
#include "TPCReco/GeometryTPC.h"

#define BEAM_TILT_FIT_XMIN   -150.0 // [mm]
#define BEAM_TILT_FIT_XMAX    150.0 // [mm]
#define BEAM_TILT_FIT_ENABLE  true
#define TCUT_COSTHETA_VS_LENGTH_ENABLE true
#define CUT2D_ECMS_ENABLE true

void makePlots_HIGS(std::string fileNameHistos, float energyMeV, std::string cutInfo="") {

  bool plot_2prong=true;
  bool plot_3prong=false;
  bool plot_fill=true; // false;
  int plot_fill_style=3345;  // 45 deg
  int plot_fill_color=kAzure+2; // kGray+2;
  if(!plot_fill) {
    plot_fill_style=0;
    plot_fill_color=0;
  }

  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  GeometryTPC *geo=new GeometryTPC("geometry_ELITPC.dat", false);
  if(!geo) exit(-1);
  double xmin, xmax, ymin, ymax;
  std::tie(xmin, xmax, ymin, ymax)=geo->rangeXY();
  TGraph *area1=new TGraph(geo->GetActiveAreaConvexHull(0));
  TGraph *area2=new TGraph(geo->GetActiveAreaConvexHull(5.0));
  area1->GetXaxis()->SetLimits(xmin-0.025*(xmax-xmin), xmax+0.025*(xmax-xmin));
  area1->GetYaxis()->SetLimits(ymin-0.100*(ymax-ymin), ymax+0.100*(ymax-ymin));
  area1->SetLineColor(kBlack);
  area1->SetLineWidth(2);
  area1->SetLineStyle(kSolid);
  area2->SetLineColor(kRed);
  area2->SetLineWidth(2);
  area2->SetLineStyle(kSolid);

  TFile *f = new TFile(fileNameHistos.c_str(), "OLD");
  if(!f) exit(-1);
  
  gStyle->SetOptFit(111); // PCEV : prob=disabled / chi2,errors,variables=enabled
  gStyle->SetOptFit(111); // PCEV : prob=disabled / chi2,errors,variables=enabled
  gStyle->SetOptStat(10); // KISOURMEN : entries=enabled, name=disabled
  gStyle->SetPadLeftMargin(0.125);
  gStyle->SetPadBottomMargin(0.125);
  gStyle->SetTitleOffset(1.6, "X");
  gStyle->SetTitleOffset(1.7, "Y");

  TCanvas *c=new TCanvas("c","c",800,600);
  gPad->SetLogy(false);

  std::string prefix=Form("figures_%.3fMeV", energyMeV);
  c->Print(((string)(prefix)+".pdf[").c_str());
  TFile *out = new TFile(((string)(prefix)+".root").c_str(), "RECREATE");
  if(!out) exit(-1);
  gStyle->Write("MY_STYLE"); // NOTE: In output ROOT file with TCanvas -> if needed click this OBJECT to display fit results instead of usual stat box

  TH1D *h1;
  TH2D *h2;
  TH1D *h1_xcheck; // HIGS analysis reaction ID 2D cut x-check
  TH2D *h2_xcheck; // HIGS analysis reaction ID 2D cut x-check
  TProfile *hp;
  TPaveStats *st;
  auto statX1=0.75;
  auto statY1=0.825;
  auto statX2=statX1-0.1;
  auto statY2=statY1-0.05;
  auto statX3=statX1-0.04;
  auto statY3=statY1;
  auto statX4=0.65;
  auto statY4=0.25;
  auto statX5=0.625;
  auto statY5=0.20;
  auto statX6=statX1-0.1;
  auto statY6=statY1;
  auto lineH=0.075;
  auto lineW=0.15;
  auto statX7=0.9-1.5*lineW;
  auto statY7=0.9-2.5*lineH;
  std::string energyInfo=Form("E_{#gamma}=%g MeV : ", energyMeV);//Form("E_{#gamma}=%.3f MeV : ", energyMeV);
  
  ///// control plot: number of tracks
  h1=(TH1D*)f->Get("h_ntracks")->Clone(); // copy for modifications of the same histogram
  c->SetName(Form("c_%s",h1->GetName()));
  h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
  gStyle->SetOptStat(10); // KISOURMEN : show entries only
  h1->SetStats(true);
  h1->Draw("HIST");
  h1->Draw("TEXT00 SAME");
  h1->UseCurrentStyle();
  h1->SetLineWidth(2);
  h1->SetFillColor(plot_fill_color);
  h1->SetFillStyle(plot_fill_style);
  h1->GetXaxis()->SetNdivisions(5);
  c->Update();
  st = (TPaveStats *)c->GetPrimitive("stats");
  if(st) {
    st->SetX1NDC(statX1); st->SetX2NDC(statX1+lineW); st->SetY1NDC(statY1); st->SetY2NDC(statY1+lineH);
  }
  c->Update();
  c->Modified();
  c->Print(((string)(prefix)+".pdf").c_str());
  c->Write();

  //// ALL categories, any track endpoint X vs Y
  c->Clear();
  h2=(TH2D*)f->Get("h_all_endXY")->Clone(); // copy for modifications of the same histogram
  c->SetName(Form("c_%s",h2->GetName()));
  h2->SetTitle((energyInfo+"Fully contained tracks").c_str());
  h2->SetXTitle("Track endpoint X_{DET} [mm]");
  h2->SetYTitle("Track endpoint Y_{DET} [mm]");
  gStyle->SetOptStat(10); // KISOURMEN : show entries only
  h2->Rebin2D(5,5);
  h2->UseCurrentStyle();
  area1->GetXaxis()->SetTitle(h2->GetXaxis()->GetTitle());
  area1->GetYaxis()->SetTitle(h2->GetYaxis()->GetTitle());
  area1->SetTitle(h2->GetTitle());
  area1->Draw("AL"); // UVW perimeter
  h2->Draw("COLZ SAMES"); // draw stat box
  area2->Draw("L SAME"); // UVW perimeter with 5mm veto band
  h2->SetStats(true);
  gPad->SetLeftMargin(0.1);
  gPad->SetRightMargin(0.14);
  area1->GetXaxis()->SetTitleOffset(1.6);
  area1->GetYaxis()->SetTitleOffset(1.4);
  h2->SetTitleOffset(1.2, "Z");
  gPad->Update();
  st = (TPaveStats *)c->GetPrimitive("stats");
  if(st) {
    st->SetFillColor(kWhite);
    st->SetFillStyle(1001); // solid
    st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
    st->Draw();
  }
  c->Update();
  c->Modified();
  c->Print(((string)(prefix)+".pdf").c_str());
  c->Write();

  //// 2-prong, alpha endpoint X vs Y
  if(plot_2prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_alpha_endXY")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(5,5);
    h2->UseCurrentStyle();
    area1->GetXaxis()->SetTitle(h2->GetXaxis()->GetTitle());
    area1->GetYaxis()->SetTitle(h2->GetYaxis()->GetTitle());
    area1->SetTitle(h2->GetTitle());
    area1->Draw("AL"); // UVW perimeter
    h2->Draw("COLZ SAMES"); // draw stat box
    area2->Draw("L SAME"); // UVW perimeter with 5mm veto band
    h2->SetStats(true);
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    area1->GetXaxis()->SetTitleOffset(1.6);
    area1->GetYaxis()->SetTitleOffset(1.4);
    h2->SetTitleOffset(1.2, "Z");
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetFillColor(kWhite);
      st->SetFillStyle(1001); // solid
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
      st->Draw();
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, carbon endpoint X vs Y
  if(plot_2prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_carbon_endXY")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(5,5);
    h2->UseCurrentStyle();
    area1->GetXaxis()->SetTitle(h2->GetXaxis()->GetTitle());
    area1->GetYaxis()->SetTitle(h2->GetYaxis()->GetTitle());
    area1->SetTitle(h2->GetTitle());
    area1->Draw("AL"); // UVW perimeter
    h2->Draw("COLZ SAMES"); // draw stat box
    area2->Draw("L SAME"); // UVW perimeter with 5mm veto band
    h2->SetStats(true);
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    area1->GetXaxis()->SetTitleOffset(1.6);
    area1->GetYaxis()->SetTitleOffset(1.4);
    h2->SetTitleOffset(1.2, "Z");
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetFillColor(kWhite);
      st->SetFillStyle(1001); // solid
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
      st->Draw();
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, vertex X vs Y (ALL) 
  if(plot_2prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_vertexXY")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(3,3);
    h2->UseCurrentStyle();
    area1->GetXaxis()->SetTitle(h2->GetXaxis()->GetTitle());
    area1->GetYaxis()->SetTitle(h2->GetYaxis()->GetTitle());
    area1->SetTitle(h2->GetTitle());
    area1->Draw("AL"); // UVW perimeter
    h2->Draw("COLZ SAMES"); // draw stat box
    area2->Draw("L SAME"); // UVW perimeter with 5mm veto band
    h2->SetStats(true);
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    area1->GetXaxis()->SetTitleOffset(1.6);
    area1->GetYaxis()->SetTitleOffset(1.4);
    h2->SetTitleOffset(1.2, "Z");
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetFillColor(kWhite);
      st->SetFillStyle(1001); // solid
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
      st->Draw();
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, vertex X vs Y, zoomed Y=[-15mm, 15mm]
  if(plot_2prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_vertexXY")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_zoom_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(1110); // KISOURMEN : show entries + mean + rms
    h2->Rebin2D(5,1);
    h2->UseCurrentStyle();
    h2->SetStats(true);
    h2->Draw("COLZ");
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.2, "Z");
    h2->GetYaxis()->SetRangeUser(-15.0, 15.0);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3-lineH*2); st->SetY2NDC(statY3+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, vertex X vs Y, zoomed Y=[-15mm, 15mm] with fitted beam tilt and offset
  if(plot_2prong && BEAM_TILT_FIT_ENABLE) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_vertexXY")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_fit_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(1110); // KISOURMEN : show entries + mean + rms
    h2->Rebin2D(5,1);
    h2->UseCurrentStyle();
    h2->SetStats(true);
    h2->Draw("COLZ");
    TF1 *tiltFunc=new TF1("tiltFunc", "[0]+[1]*x", BEAM_TILT_FIT_XMIN, BEAM_TILT_FIT_XMAX);//h2->GetXaxis()->GetXmin()+20, h2->GetXaxis()->GetXmax()-20);
    tiltFunc->SetParNames("Offset", "Slope");
    h2->Fit(tiltFunc, "R");
    h2->Draw("SAME");
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.2, "Z");
    h2->GetYaxis()->SetRangeUser(-15.0, 15.0);
    gStyle->SetOptFit(true);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX3-lineW*0.5); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3-lineH*3); st->SetY2NDC(statY3+lineH);
      //      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3-lineH*2); st->SetY2NDC(statY3+lineH);
    }
    gROOT->ForceStyle();
    c->Update();
    c->Modified();
    c->Write();
    c->Print(((string)(prefix)+".pdf").c_str());
  }

  //// 2-prong, profile histogram: vertex X vs average vertex Y with fitted beam tilt & offset (alternatie version)
  if(plot_2prong && BEAM_TILT_FIT_ENABLE) {
    c->Clear();
    hp=(TProfile*)f->Get("h_2prong_vertexXY_prof")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_fit_%s",hp->GetName()));
    hp->SetTitle((energyInfo+hp->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    hp->Rebin(1);
    hp->Draw();
    hp->UseCurrentStyle();
    hp->SetStats(true);
    hp->SetLineWidth(2);
    TF1 *tiltFunc=new TF1("tiltFunc", "[0]+[1]*x", BEAM_TILT_FIT_XMIN, BEAM_TILT_FIT_XMAX);//hp->GetXaxis()->GetXmin()+20, hp->GetXaxis()->GetXmax()-20);
    tiltFunc->SetParNames("Offset", "Slope");
    hp->Fit(tiltFunc, "R");
    hp->Draw("E1 SAME");
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX5); st->SetX2NDC(statX5+lineW*1.5); st->SetY1NDC(statY5); st->SetY2NDC(statY5+lineH*2.5);
    }
    //  if(st) {
    //    st->SetX1NDC(statX4); st->SetX2NDC(statX4+lineW); st->SetY1NDC(statY4-lineH); st->SetY2NDC(statY4+lineH);
    //  }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, vertex X distribution (along beam)
  if(plot_2prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_2prong_vertexX")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h1->GetName()));
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(1110); // KISOURMEN : show entries + mean + rms
    h1->Rebin(5);
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX4); st->SetX2NDC(statX4+lineW); st->SetY1NDC(statY4-lineH); st->SetY2NDC(statY4+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, vertex Y distribution, zoomed Y=[-15mm, 15mm], no correction for beam tilt
  if(plot_2prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_2prong_vertexY")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h1->GetName()));
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(1110); // KISOURMEN : show entries + mean + rms
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    h1->GetXaxis()->SetRangeUser(-15.0, 15.0);
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX1); st->SetX2NDC(statX1+lineW); st->SetY1NDC(statY1-lineH); st->SetY2NDC(statY1+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, vertex XBEAM distribution, zoomed XBEAM=[-15mm, 15mm], corrected for beam tilt
  if(plot_2prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_2prong_vertexXBEAM")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h1->GetName()));
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(1110); // KISOURMEN : show entries + mean + rms
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    h1->GetXaxis()->SetRangeUser(-15.0, 15.0);
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX1); st->SetX2NDC(statX1+lineW); st->SetY1NDC(statY1-lineH); st->SetY2NDC(statY1+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, Sum of alpha + carbon track lengths, zoomed X=[20mm, 100mm], log Y scale @ 8.66 MeV
  //// 2-prong, Sum of alpha + carbon track lengths, zoomed X=[30mm, 120mm], log Y scale @ 9.845 MeV
  //// 2-prong, Sum of alpha + carbon track lengths, zoomed X=[30mm, 130mm], log Y scale @ 11.5 MeV
  //// 2-prong, Sum of alpha + carbon track lengths, zoomed X=[30mm, 150mm], log Y scale @ 12.3 MeV
  if(plot_2prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_2prong_lenSum")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_log_%s",h1->GetName()));
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    h1->GetXaxis()->SetRangeUser(20.0, 100.0); // valid for 8.66 MeV
    //    h1->GetXaxis()->SetRangeUser(30.0, 120.0); // valid for 9.845 MeV
    //    h1->GetXaxis()->SetRangeUser(30.0, 130.0); // valid for 11.5 MeV
    //    h1->GetXaxis()->SetRangeUser(30.0, 140.0); // valid for 11.9 MeV
    //    h1->GetXaxis()->SetRangeUser(30.0, 150.0); // valid for 12.3 MeV
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->SetLogy(true);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX1); st->SetX2NDC(statX1+lineW); st->SetY1NDC(statY1); st->SetY2NDC(statY1+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, Sum of alpha + carbon track lengths, zoomed X=[20mm, 100mm], linear Y scale @ 8.66 MeV
  //// 2-prong, Sum of alpha + carbon track lengths, zoomed X=[30mm, 120mm], linear Y scale @ 9.845 MeV
  //// 2-prong, Sum of alpha + carbon track lengths, zoomed X=[30mm, 130mm], linear Y scale @ 11.5 MeV
  //// 2-prong, Sum of alpha + carbon track lengths, zoomed X=[30mm, 150mm], linear Y scale @ 12.3 MeV
  if(plot_2prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_2prong_lenSum")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h1->GetName()));
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    h1->GetXaxis()->SetRangeUser(20.0, 100.0); // valid for 8.66 MeV
    //    h1->GetXaxis()->SetRangeUser(30.0, 120.0); // valid for 9.845 MeV
    //    h1->GetXaxis()->SetRangeUser(30.0, 130.0); // valid for 11.5 MeV
    //    h1->GetXaxis()->SetRangeUser(30.0, 140.0); // valid for 11.9 MeV
    //    h1->GetXaxis()->SetRangeUser(30.0, 150.0); // valid for 12.3 MeV
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->SetLogy(false);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX1); st->SetX2NDC(statX1+lineW); st->SetY1NDC(statY1); st->SetY2NDC(statY1+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, Alpha track length, zoomed X=[10mm, 80mm], log Y scale @ 8.66 MeV
  //// 2-prong, Alpha track length, zoomed X=[20mm, 100mm], log Y scale @ 9.845 MeV
  //// 2-prong, Alpha track length, zoomed X=[30mm, 130mm], log Y scale @ 11.5 MeV
  //// 2-prong, Alpha track length, zoomed X=[30mm, 150mm], log Y scale @ 12.3 MeV
  if(plot_2prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_2prong_alpha_len")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_log_%s",h1->GetName()));
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    h1->GetXaxis()->SetRangeUser(10.0, 80.0); // valid for 8.66 MeV
    //    h1->GetXaxis()->SetRangeUser(20.0, 100.0); // valid for 9.845 MeV
    //    h1->GetXaxis()->SetRangeUser(30.0, 130.0); // valid for 11.5 MeV
    //    h1->GetXaxis()->SetRangeUser(30.0, 130.0); // valid for 11.9 MeV after O16 selection
    //    h1->GetXaxis()->SetRangeUser(30.0, 150.0); // valid for 12.3 MeV
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->SetLogy(true);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX1); st->SetX2NDC(statX1+lineW); st->SetY1NDC(statY1); st->SetY2NDC(statY1+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 16O : 2-prong, Alpha track length, zoomed X=[10mm, 80mm], linear Y scale @ 8.66 MeV
  //// 2-prong, Alpha track length, zoomed X=[40mm, 70mm], linear Y scale @ 9.845 MeV
  //// 2-prong, Alpha track length, zoomed X=[50mm, 90mm], linear Y scale @ 11.5 MeV
  //// 2-prong, Alpha track length, zoomed X=[60mm, 90mm], linear Y scale @ 11.9 MeV
  //// 2-prong, Alpha track length, zoomed X=[75mm, 105mm], linear Y scale @ 12.3 MeV
  if(plot_2prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_2prong_alpha_len")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h1->GetName()));
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    h1->GetXaxis()->SetRangeUser(10.0, 80.0); // valid for 8.66 MeV
    //    h1->GetXaxis()->SetRangeUser(30.0, 90.0); // valid for 9.845 MeV
    //    h1->GetXaxis()->SetRangeUser(40.0, 70.0); // valid for 9.845 MeV after O16 selection
    //    h1->GetXaxis()->SetRangeUser(50.0, 90.0); // valid for 11.5 MeV
    //    h1->GetXaxis()->SetRangeUser(60.0, 100.0); // valid for 11.9 MeV after O16 selection
    //    h1->GetXaxis()->SetRangeUser(50.0, 120.0); // valid for 11.9 MeV
    //    h1->GetXaxis()->SetRangeUser(75.0, 105.0); // valid for 12.3 MeV
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->SetLogy(false);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX1); st->SetX2NDC(statX1+lineW); st->SetY1NDC(statY1); st->SetY2NDC(statY1+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, Alpha track length (X) vs Carbon track length (Y), zoomed X=[10mm, 80mm] x Y=[4mm, 18mm] @ 8.66 MeV
  //// 2-prong, Alpha track length (X) vs Carbon track length (Y), zoomed X=[20mm, 120mm] x Y=[4mm, 18mm] @ 9.845 MeV
  //// 2-prong, Alpha track length (X) vs Carbon track length (Y), zoomed X=[20mm, 120mm] x Y=[4mm, 18mm] @ 11.5 MeV
  //// 2-prong, Alpha track length (X) vs Carbon track length (Y), zoomed X=[20mm, 120mm] x Y=[4mm, 18mm] @ 12.3 MeV
  if(plot_2prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_alpha_len_carbon_len")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(1,1);
    h2->UseCurrentStyle();
    h2->SetStats(true);
    h2->Draw("COLZ");
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.2, "Z");
    h2->GetXaxis()->SetRangeUser(10, 80); // valid for 8.66 MeV
    h2->GetYaxis()->SetRangeUser(4, 18); // valid for 8.66 MeV
    //    h2->GetXaxis()->SetRangeUser(35, 95); // valid for 9.845 MeV
    //    h2->GetYaxis()->SetRangeUser(5, 30); // valid for 9.845 MeV
    //    h2->GetXaxis()->SetRangeUser(35, 65); // valid for 9.845 MeV after 16O selection
    //    h2->GetYaxis()->SetRangeUser(6, 18); // valid for 9.845 MeV after 16O selection
    //    h2->GetXaxis()->SetRangeUser(45, 100); // valid for 11.5 MeV
    //    h2->GetYaxis()->SetRangeUser(4, 20); // valid for 11.5 MeV
    //    h2->GetXaxis()->SetRangeUser(50, 120); // valid for 11.9 MeV
    //    h2->GetYaxis()->SetRangeUser(4, 25); // valid for 11.9 MeV
    //    h2->GetXaxis()->SetRangeUser(55, 100); // valid for 11.9 MeV after 16O selection
    //    h2->GetYaxis()->SetRangeUser(5, 20); // valid for 11.9 MeV after 16 selection
    //    h2->GetXaxis()->SetRangeUser(20, 120); // valid for 12.3 MeV
    //    h2->GetYaxis()->SetRangeUser(4, 18); // valid for 12.3 MeV
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, Alpha track length (X) vs Carbon track length (Y), after Alpha vs Carbon E_CMS 2D-cut, zoomed X=[0, 2.5MeV] x Y=[0, 2MeV] @ 8.66 MeV
  if(plot_2prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_alpha_len_carbon_len_CutO16")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(1,1);
    h2->UseCurrentStyle();
    h2->SetStats(true);
    h2->Draw("COLZ");
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.2, "Z");
    h2->GetXaxis()->SetRangeUser(10, 80); // valid for 8.66 MeV
    h2->GetYaxis()->SetRangeUser(4, 18); // valid for 8.66 MeV
    //    h2->GetXaxis()->SetRangeUser(35, 95); // valid for 9.845 MeV
    //    h2->GetYaxis()->SetRangeUser(5, 30); // valid for 9.845 MeV
    //    h2->GetXaxis()->SetRangeUser(35, 65); // valid for 9.845 MeV after 16O selection
    //    h2->GetYaxis()->SetRangeUser(6, 18); // valid for 9.845 MeV after 16O selection
    //    h2->GetXaxis()->SetRangeUser(45, 100); // valid for 11.5 MeV
    //    h2->GetYaxis()->SetRangeUser(4, 20); // valid for 11.5 MeV
    //    h2->GetXaxis()->SetRangeUser(50, 120); // valid for 11.9 MeV
    //    h2->GetYaxis()->SetRangeUser(4, 25); // valid for 11.9 MeV
    //    h2->GetXaxis()->SetRangeUser(55, 100); // valid for 11.9 MeV after 16O selection
    //    h2->GetYaxis()->SetRangeUser(5, 20); // valid for 11.9 MeV after 16 selection
    //    h2->GetXaxis()->SetRangeUser(20, 120); // valid for 12.3 MeV
    //    h2->GetYaxis()->SetRangeUser(4, 18); // valid for 12.3 MeV
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 16O : 2-prong, Alpha kinetic energy in CMS
  if(plot_2prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_2prong_alpha_E_CMS")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h1->GetName()));
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(1110); // KISOURMEN : show entries + mean + rms
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    h1->GetXaxis()->SetRangeUser(0.0, 3.0); // valid for 8.66 MeV
    //   h1->GetXaxis()->SetRangeUser(0.5, 3.0); // valid for 9.845 MeV
    //    h1->GetXaxis()->SetRangeUser(0.5, 2.5); // valid for 9.845 MeV after 16O selection
    //    h1->GetXaxis()->SetRangeUser(2.0, 4.0); // valid for 11.5 MeV after 16O selection
    //    h1->GetXaxis()->SetRangeUser(2.5, 4.5); // valid for 11.9 MeV after 16O selection
    //    h1->GetXaxis()->SetRangeUser(2.5, 5.0); // valid for 11.9 MeV
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->SetLogy(false);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX1); st->SetX2NDC(statX1+lineW); st->SetY1NDC(statY1-lineH); st->SetY2NDC(statY1+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 16O : 2-prong, Carbon kinetic energy in CMS
  if(plot_2prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_2prong_carbon_E_CMS")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h1->GetName()));
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(1110); // KISOURMEN : show entries + mean + rms
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    h1->GetXaxis()->SetRangeUser(0.0, 3.0); // valid for 8.66 MeV
    //    h1->GetXaxis()->SetRangeUser(0.0, 1.5); // valid for 9.845 MeV after 16O selection
    //    h1->GetXaxis()->SetRangeUser(0.0, 2.5); // valid for 11.5 MeV after 16O selection
    //    h1->GetXaxis()->SetRangeUser(0.0, 4.0); // valid for 11.9 MeV
    //    h1->GetXaxis()->SetRangeUser(0.0, 3.0); // valid for 11.9 MeV after 16O selection
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->SetLogy(false);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX1); st->SetX2NDC(statX1+lineW); st->SetY1NDC(statY1-lineH); st->SetY2NDC(statY1+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, Alpha kinetic energy in CMS (X) vs Carbon kinetic energy in CMS (Y), zoomed X=[0, 2.5MeV] x Y=[0, 2MeV] @ 8.66 MeV
  if(plot_2prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_alpha_E_carbon_E_CMS")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(1,1);
    h2->UseCurrentStyle();
    h2->SetStats(true);
    h2->Draw("COLZ");
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.2, "Z");
    h2->GetXaxis()->SetRangeUser(0, 2.5); // valid for 8.66 MeV
    h2->GetYaxis()->SetRangeUser(0, 2.0); // valid for 8.66 MeV
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, Alpha kinetic energy in CMS (X) vs Carbon kinetic energy in CMS (Y), after Alpha vs Carbon E_CMS 2D-cut, zoomed X=[0, 2.5MeV] x Y=[0, 2MeV] @ 8.66 MeV
  if(plot_2prong && CUT2D_ECMS_ENABLE) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_alpha_E_carbon_E_CMS_CutO16")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(1,1);
    h2->UseCurrentStyle();
    h2->SetStats(true);
    h2->Draw("COLZ");
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.2, "Z");
    h2->GetXaxis()->SetRangeUser(0, 2.5); // valid for 8.66 MeV
    h2->GetYaxis()->SetRangeUser(0, 2.0); // valid for 8.66 MeV
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, Alpha kinetic energy in LAB (Y) vs Alpha cos(theta_BEAM) in LAB (X)
  if(plot_2prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_alpha_cosThetaBEAM_E_LAB")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(4,2);
    h2->UseCurrentStyle();
    h2->SetStats(true);
    h2->Draw("COLZ");
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.2, "Z");
    h2->GetYaxis()->SetRangeUser(0.0, 2.5); // valid for 8.66  MeV
    //    h2->GetYaxis()->SetRangeUser(1.0, 3.0); // valid for 9.845 MeV
    //    h2->GetYaxis()->SetRangeUser(1.5, 2.5); // valid for 9.845 MeV after O16 selection
    //    h2->GetYaxis()->SetRangeUser(2.0, 4.0); // valid for 11.5 MeV 
    //    h2->GetYaxis()->SetRangeUser(2.0, 5.0); // valid for 11.9 MeV 
    //    h2->GetYaxis()->SetRangeUser(2.5, 4.5); // valid for 11.9 MeV after O16 selection
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, Alpha kinetic energy in CMS (Y) vs Alpha cos(theta_BEAM) in CMS (X)
  if(plot_2prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_alpha_cosThetaBEAM_E_CMS")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(4,2);
    h2->UseCurrentStyle();
    h2->SetStats(true);
    h2->Draw("COLZ");
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.2, "Z");
    h2->GetYaxis()->SetRangeUser(0.0, 2.5); // valid for 8.66  MeV
    //    h2->GetYaxis()->SetRangeUser(1.0, 3.0); // valid for 9.845 MeV
    //    h2->GetYaxis()->SetRangeUser(1.5, 2.5); // valid for 9.845 MeV after O16 selection
    //    h2->GetYaxis()->SetRangeUser(2.0, 4.0); // valid for 11.5 MeV 
    //    h2->GetYaxis()->SetRangeUser(2.0, 5.0); // valid for 11.9 MeV 
    //    h2->GetYaxis()->SetRangeUser(2.5, 4.5); // valid for 11.9 MeV after O16 selection cuts
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, Alpha kinetic energy in CMS (Y) vs Alpha cos(theta_BEAM) in CMS (X), after Alpha vs Carbon E_CMS 2D-cut
  if(plot_2prong && CUT2D_ECMS_ENABLE) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_alpha_cosThetaBEAM_E_CMS_CutO16")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(4,2);
    h2->UseCurrentStyle();
    h2->SetStats(true);
    h2->Draw("COLZ");
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.2, "Z");
    h2->GetYaxis()->SetRangeUser(0.0, 2.5); // valid for 8.66  MeV
    //    h2->GetYaxis()->SetRangeUser(1.0, 3.0); // valid for 9.845 MeV
    //    h2->GetYaxis()->SetRangeUser(1.5, 2.5); // valid for 9.845 MeV after O16 selection
    //    h2->GetYaxis()->SetRangeUser(2.0, 4.0); // valid for 11.5 MeV 
    //    h2->GetYaxis()->SetRangeUser(2.0, 5.0); // valid for 11.9 MeV 
    //    h2->GetYaxis()->SetRangeUser(2.5, 4.5); // valid for 11.9 MeV after O16 selection cuts
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, Vertex XBEAM (X) vs Alpha cos(theta_BEAM) in CMS (Y), zoomed XBEAM=[-8mm, 8mm]
  if(plot_2prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_vertexXBEAM_alpha_cosThetaBEAM_CMS")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(2,4);
    h2->UseCurrentStyle();
    h2->SetStats(true);
    h2->Draw("COLZ");
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.2, "Z");
    h2->GetXaxis()->SetRangeUser(-8.0, 8.0); // mm
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, Vertex XBEAM (X) vs Alpha kinetic energy in CMS (Y), zoomed XBEAM=[-8mm, 8mm]
  if(plot_2prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_vertexXBEAM_alpha_E_CMS")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(2,2);
    h2->UseCurrentStyle();
    h2->SetStats(true);
    h2->Draw("COLZ");
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.2, "Z");
    h2->GetXaxis()->SetRangeUser(-8.0, 8.0); // mm
    h2->GetYaxis()->SetRangeUser(0.0, 2.5); // MeV, valid for 8.66 MeV
    //    h2->GetYaxis()->SetRangeUser(1.0, 3.0); // MeV, valid for 9.845 MeV
    //    h2->GetYaxis()->SetRangeUser(1.5, 2.5); // MeV, valid for 9.845 MeV after O16 selection
    //    h2->GetYaxis()->SetRangeUser(2.0, 4.5); // MeV, valid for 11.5 MeV
    //    h2->GetYaxis()->SetRangeUser(2.0, 5.0); // MeV, valid for 11.9 MeV
    //    h2->GetYaxis()->SetRangeUser(2.5, 4.5); // MeV, valid for 11.9 MeV after O16 selection 
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, Vertex XBEAM (X) vs Reconstructed E_gamma in LAB (Y), zoomed XBEAM=[-8mm, 8mm]
  if(plot_2prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_vertexXBEAM_gamma_E_LAB")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(2,2);
    h2->UseCurrentStyle();
    h2->SetStats(true);
    h2->Draw("COLZ");
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.2, "Z");
    h2->GetXaxis()->SetRangeUser(-8.0, 8.0); // mm
    h2->GetYaxis()->SetRangeUser(7.5, 11.5); // MeV, valid for 8.66 MeV
    //    h2->GetYaxis()->SetRangeUser(8.5, 11.0); // MeV, valid for 9.845 MeV
    //    h2->GetYaxis()->SetRangeUser(9.0, 10.5); // MeV, valid for 9.845 MeV after O16 selection
    //    h2->GetYaxis()->SetRangeUser(9.5, 13.0); // MeV, valid for 11.5 MeV
    //    h2->GetYaxis()->SetRangeUser(10.0, 14.0); // MeV, valid for 11.9 MeV
    //    h2->GetYaxis()->SetRangeUser(10.5, 14.0); // MeV, valid for 11.9 MeV after O16 selection
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// define 2D cut to select Oxygen-16 only for next 3 plots
  Double_t theta[5]  = {-1,    1,       1,    -1, -1};
  //  Double_t length[5] = {58, 58+5, 58+5+20, 58+20, 58}; // valid for 11.5MeV
  //  Double_t length[5] = {78, 78+7, 78+7+20, 78+20, 78}; // valid for 12.3MeV
  //  Double_t length[5] = {38, 38+7, 38+7+20, 38+20, 38}; // valid for 9.845 MeV
  Double_t length[5] = {10, 10+3, 10+3+25, 10+25, 10}; // valid for 8.66 MeV
  
  TCutG *mycut=new TCutG("select_O16", 5, theta, length);
  mycut->SetLineColor(kRed);
  mycut->SetLineWidth(3);
  
  //// 2-prong, Alpha track length (X) vs Alpha cos(theta_BEAM_LAB), zoomed Y=[30mm, 120mm]
  if(plot_2prong && TCUT_COSTHETA_VS_LENGTH_ENABLE) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_alpha_cosThetaBEAM_len_LAB")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    //    h2->Rebin2D(2,2);
    h2->UseCurrentStyle();
    h2->SetStats(true);
    h2->Draw("COLZ");
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.2, "Z");
    h2->GetYaxis()->SetRangeUser(0, 80); // valid for 8.66 MeV
    //    h2->GetYaxis()->SetRangeUser(30, 110); // valid for 11.5 MeV
    //    h2->GetYaxis()->SetRangeUser(30, 120); // valid for 11.9 MeV
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
    }
    mycut->Draw("SAME");
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }
  
  //// 2-prong, Alpha track length (X) vs Alpha cos(theta_BEAM_LAB), zoomed Y=[15mm, 40mm], tagged O-16 @ 8.66 MeV
  //// 2-prong, Alpha track length (X) vs Alpha cos(theta_BEAM_LAB), zoomed Y=[30mm, 70mm], tagged O-16 @ 9.845 MeV
  //// 2-prong, Alpha track length (X) vs Alpha cos(theta_BEAM_LAB), zoomed Y=[50mm, 90mm], tagged O-16 @ 11.5 MeV
  //// 2-prong, Alpha track length (X) vs Alpha cos(theta_BEAM_LAB), zoomed Y=[70mm, 110mm], tagged O-16 @ 12.3 MeV
  if(plot_2prong && TCUT_COSTHETA_VS_LENGTH_ENABLE) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_alpha_cosThetaBEAM_len_LAB")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_cut_%s",h1->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    //  gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(2,2);
    h2->UseCurrentStyle();
    h2->SetStats(false);
    h2->Draw("COLZ [select_O16]");
    //    h2->SetTitle(Form("%s, ^{16}O candidates", h2->GetTitle())); // modify the title
    h2->SetTitle(Form("%s, (with #alpha length,cos(#theta_{BEAM})^{LAB} cut)", h2->GetTitle())); // modify the title
    mycut->Draw("SAME");
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.2, "Z");
    //    h2->GetYaxis()->SetRangeUser(60, 100); // valid for 11.9 MeV
    //    h2->GetYaxis()->SetRangeUser(50, 90); // valid for 11.5 MeV
    //    h2->GetYaxis()->SetRangeUser(70, 110); // valid for 12.3 MeV
    //    h2->GetYaxis()->SetRangeUser(30, 70); // valid for 9.845 MeV
    h2->GetYaxis()->SetRangeUser(10, 40); // valid for 8.66 MeV
    gPad->Update();
    //  st = (TPaveStats *)c->GetPrimitive("stats");
    //  if(st) {
    //    st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
    //  }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }
  
  //// 2-prong, Alpha cos(theta_BEAM_LAB), tagged Oxygen-16
  if(plot_2prong && TCUT_COSTHETA_VS_LENGTH_ENABLE) {
    h2=(TH2D*)f->Get("h_2prong_alpha_cosThetaBEAM_len_LAB")->Clone(); // copy for modifications of the same histogram
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    h1=h2->ProjectionX("_px",1, h2->GetNbinsY(),"[select_O16]");
    c->SetName(Form("c_cut_%s",h1->GetName()));
    h1->Rebin(4);
    h1->SetYTitle(h2->GetZaxis()->GetTitle());
    //    h1->SetTitle(Form("%s, ^{16}O candidates", h2->GetTitle())); // modify the title
    h1->SetTitle(Form("%s, (with #alpha length,cos(#theta_{BEAM})^{LAB} cut)", h2->GetTitle())); // modify the title
    h1->Draw("HIST E1");
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    //    h1->GetXaxis()->SetRangeUser(60.0, 80.0);
    h1->GetMinimum(0);
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->SetLogy(false);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX1); st->SetX2NDC(statX1+lineW); st->SetY1NDC(statY1); st->SetY2NDC(statY1+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }
  
  //// 2-prong, Alpha cos(theta_BEAM_CMS)
  if(plot_2prong) {
    h1=(TH1D*)f->Get("h_2prong_alpha_cosThetaBEAM_CMS")->Clone(); // copy for modifications of the same histogram
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    c->SetName(Form("c_cut_%s",h1->GetName()));
    h1->Rebin(4);
    h1->SetYTitle(h2->GetZaxis()->GetTitle());
    h1->Draw("HIST E1");
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    h1->GetMinimum(0);
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->SetLogy(false);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX1); st->SetX2NDC(statX1+lineW); st->SetY1NDC(statY1); st->SetY2NDC(statY1+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, Alpha cos(theta_BEAM_CMS), after Alpha vs Carbon E_CMS 2D-cut
  if(plot_2prong && CUT2D_ECMS_ENABLE) {
    h1=(TH1D*)f->Get("h_2prong_alpha_cosThetaBEAM_CMS_CutO16")->Clone(); // copy for modifications of the same histogram
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    c->SetName(Form("c_cut_%s",h1->GetName()));
    h1->Rebin(4);
    h1->SetYTitle(h2->GetZaxis()->GetTitle());
    h1->Draw("HIST E1");
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    h1->GetMinimum(0);
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->SetLogy(false);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX1); st->SetX2NDC(statX1+lineW); st->SetY1NDC(statY1); st->SetY2NDC(statY1+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, Alpha phi_BEAM_LAB, corrected for beam tilt with fit
  if(plot_2prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_2prong_alpha_phiBEAM_LAB")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h1->GetName()));
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h1->Rebin(5);
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    h1->SetMinimum(0);
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->SetLogy(false);
    TF1 *phiFunc=new TF1("phiFunc", "[0]*(1+[1]*cos(2*(x+[2])))", h1->GetXaxis()->GetXmin(), h1->GetXaxis()->GetXmax());
    phiFunc->SetParNames("A", "f", "#delta");
    h1->Fit(phiFunc);
    h1->Draw("E1 SAME");
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX7); st->SetX2NDC(statX7+lineW*1.5); st->SetY1NDC(statY7); st->SetY2NDC(statY7+lineH*2.5);
    }
    TLatex *tl=new TLatex;
    tl->SetTextSize(0.030);
    tl->SetTextAlign(11); // align at bottom, left
    //    tl->DrawLatexNDC(0.2, 0.275, "Fit :  #font[12]{#frac{dN}{d#phi} = A #[]{1 + f cos 2#(){#phi + #delta}}}");
    tl->DrawLatexNDC(0.15, 0.85, "Fit :  #font[12]{#frac{dN}{d#phi} = A #[]{1 + f cos 2#(){#phi + #delta}}}");
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, Alpha phi_BEAM_CMS, corrected for beam tilt with fit
  if(plot_2prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_2prong_alpha_phiBEAM_CMS")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h1->GetName()));
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h1->Rebin(5);
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    h1->SetMinimum(0);
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->SetLogy(false);
    //    TF1 *phiFunc=new TF1("phiFunc", "[0]*(1+[1]*cos(2*(x+[2])))", h1->GetXaxis()->GetXmin(), h1->GetXaxis()->GetXmax());
    //    phiFunc->SetParNames("A", "f", "#delta");
    //    h1->Fit(phiFunc);
    h1->Draw("E1 SAME");
    gPad->Update();
    //    st = (TPaveStats *)c->GetPrimitive("stats");
    //    if(st) {
    //      st->SetX1NDC(statX7); st->SetX2NDC(statX7+lineW*1.5); st->SetY1NDC(statY7); st->SetY2NDC(statY7+lineH*2.5);
    //    }
    //    TLatex *tl=new TLatex;
    //    tl->SetTextSize(0.030);
    //    tl->SetTextAlign(11); // align at bottom, left
    ////    tl->DrawLatexNDC(0.2, 0.275, "Fit :  #font[12]{#frac{dN}{d#phi} = A #[]{1 + f cos 2#(){#phi + #delta}}}");
    //    tl->DrawLatexNDC(0.15, 0.85, "Fit :  #font[12]{#frac{dN}{d#phi} = A #[]{1 + f cos 2#(){#phi + #delta}}}");
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, Alpha phi_BEAM_CMS, corrected for beam tilt with fit, after Alpha vs Carbon E_CMS 2D-cut
  if(plot_2prong && CUT2D_ECMS_ENABLE) {
    c->Clear();
    h1=(TH1D*)f->Get("h_2prong_alpha_phiBEAM_CMS_CutO16")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h1->GetName()));
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h1->Rebin(5);
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    h1->SetMinimum(0);
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->SetLogy(false);
    //    TF1 *phiFunc=new TF1("phiFunc", "[0]*(1+[1]*cos(2*(x+[2])))", h1->GetXaxis()->GetXmin(), h1->GetXaxis()->GetXmax());
    //    phiFunc->SetParNames("A", "f", "#delta");
    //    h1->Fit(phiFunc);
    h1->Draw("E1 SAME");
    gPad->Update();
    //   st = (TPaveStats *)c->GetPrimitive("stats");
    //    if(st) {
    //      st->SetX1NDC(statX7); st->SetX2NDC(statX7+lineW*1.5); st->SetY1NDC(statY7); st->SetY2NDC(statY7+lineH*2.5);
    //    }
    //    TLatex *tl=new TLatex;
    //    tl->SetTextSize(0.030);
    //    tl->SetTextAlign(11); // align at bottom, left
    ////    tl->DrawLatexNDC(0.2, 0.275, "Fit :  #font[12]{#frac{dN}{d#phi} = A #[]{1 + f cos 2#(){#phi + #delta}}}");
    //    tl->DrawLatexNDC(0.15, 0.85, "Fit :  #font[12]{#frac{dN}{d#phi} = A #[]{1 + f cos 2#(){#phi + #delta}}}");
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, (Alpha track enpoint_Y vs vertex_Y) vs (Alpha track enpoint_Z vs vertex_Z)
  if(plot_2prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_alpha_deltaYZ")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(2,2);
    //    h2->Rebin2D(10,10);
    //    h2->Rebin2D(20,20);
    h2->UseCurrentStyle();
    h2->SetStats(true);
    h2->Draw("COLZ");
    gPad->SetLeftMargin(0.200);
    gPad->SetRightMargin(0.200);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.25, "Z");
    h2->SetXTitle("#(){#alpha track end wrt vertex}_{Y_{DET}} [mm]");
    h2->SetYTitle("#(){#alpha track end wrt vertex}_{Z_{DET}} [mm]");
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX6); st->SetX2NDC(statX6+lineW); st->SetY1NDC(statY6); st->SetY2NDC(statY6+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, (Alpha track enpoint_Y vs vertex_Y) vs (Alpha track enpoint_Z vs vertex_Z)
  if(plot_2prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_alpha_deltaYZ")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()+" (rebinned)").c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    //    h2->Rebin2D(2,2);
    h2->Rebin2D(10,10);
    //    h2->Rebin2D(20,20);
    h2->UseCurrentStyle();
    h2->SetStats(true);
    h2->Draw("COLZ");
    gPad->SetLeftMargin(0.200);
    gPad->SetRightMargin(0.200);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.25, "Z");
    h2->SetXTitle("#(){#alpha track end wrt vertex}_{Y_{DET}} [mm]");
    h2->SetYTitle("#(){#alpha track end wrt vertex}_{Z_{DET}} [mm]");
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX6); st->SetX2NDC(statX6+lineW); st->SetY1NDC(statY6); st->SetY2NDC(statY6+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 2-prong, (Alpha track enpoint_Y vs vertex_Y) vs (Alpha track enpoint_Z vs vertex_Z) after 2D elliptic E_CMS cut
  if(plot_2prong && CUT2D_ECMS_ENABLE) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_alpha_deltaYZ_CutO16")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(2,2);
    //    h2->Rebin2D(10,10);
    //    h2->Rebin2D(20,20);
    h2->UseCurrentStyle();
    h2->SetStats(true);
    h2->Draw("COLZ");
    gPad->SetLeftMargin(0.200);
    gPad->SetRightMargin(0.200);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.25, "Z");
    h2->SetXTitle("#(){#alpha track end wrt vertex}_{Y_{DET}} [mm]");
    h2->SetYTitle("#(){#alpha track end wrt vertex}_{Z_{DET}} [mm]");
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX6); st->SetX2NDC(statX6+lineW); st->SetY1NDC(statY6); st->SetY2NDC(statY6+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }
  //////////////////////////////////

  //// 3-prong, alpha1 endpoint X vs Y
  if(plot_3prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_3prong_alpha1_endXY")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(5,5);
    h2->UseCurrentStyle();
    area1->GetXaxis()->SetTitle(h2->GetXaxis()->GetTitle());
    area1->GetYaxis()->SetTitle(h2->GetYaxis()->GetTitle());
    area1->SetTitle(h2->GetTitle());
    area1->Draw("AL"); // UVW perimeter
    h2->Draw("COLZ SAMES"); // draw stat box
    area2->Draw("L SAME"); // UVW perimeter with 5mm veto band
    h2->SetStats(true);
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    area1->GetXaxis()->SetTitleOffset(1.6);
    area1->GetYaxis()->SetTitleOffset(1.4);
    h2->SetTitleOffset(1.2, "Z");
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetFillColor(kWhite);
      st->SetFillStyle(1001); // solid
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
      st->Draw();
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 3-prong, vertex X vs Y (ALL)
  if(plot_3prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_3prong_vertexXY")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(3,3);
    h2->UseCurrentStyle();
    area1->GetXaxis()->SetTitle(h2->GetXaxis()->GetTitle());
    area1->GetYaxis()->SetTitle(h2->GetYaxis()->GetTitle());
    area1->SetTitle(h2->GetTitle());
    area1->Draw("AL"); // UVW perimeter
    h2->Draw("COLZ SAMES"); // draw stat box
    area2->Draw("L SAME"); // UVW perimeter with 5mm veto band
    h2->SetStats(true);
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    area1->GetXaxis()->SetTitleOffset(1.6);
    area1->GetYaxis()->SetTitleOffset(1.4);
    h2->SetTitleOffset(1.2, "Z");
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetFillColor(kWhite);
      st->SetFillStyle(1001); // solid
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
      st->Draw();
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 3-prong, vertex X vs Y, zoomed Y=[-15mm, 15mm]
  if(plot_3prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_3prong_vertexXY")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_zoom_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(1110); // KISOURMEN : show entries + mean + rms
    h2->Rebin2D(5,1);
    h2->UseCurrentStyle();
    h2->SetStats(true);
    h2->Draw("COLZ");
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.2, "Z");
    h2->GetYaxis()->SetRangeUser(-15.0, 15.0);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3-lineH*2); st->SetY2NDC(statY3+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 3-prong, vertex X distribution (along beam)
  if(plot_3prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_3prong_vertexX")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h1->GetName()));
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(1110); // KISOURMEN : show entries + mean + rms
    h1->Rebin(20);
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX1); st->SetX2NDC(statX1+lineW); st->SetY1NDC(statY1-lineH); st->SetY2NDC(statY1+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 3-prong, vertex Y distribution, zoomed Y=[-15mm, 15mm], no correction for beam tilt
  if(plot_3prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_3prong_vertexY")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h1->GetName()));
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(1110); // KISOURMEN : show entries + mean + rms
    h1->Rebin(2);
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    h1->GetXaxis()->SetRangeUser(-15.0, 15.0);
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX1); st->SetX2NDC(statX1+lineW); st->SetY1NDC(statY1-lineH); st->SetY2NDC(statY1+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 3-prong, vertex XBEAM distribution, zoomed XBEAM=[-15mm, 15mm], corrected for beam tilt
  if(plot_3prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_3prong_vertexXBEAM")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h1->GetName()));
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(1110); // KISOURMEN : show entries + mean + rms
    h1->Rebin(2);
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    h1->GetXaxis()->SetRangeUser(-15.0, 15.0);
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX1); st->SetX2NDC(statX1+lineW); st->SetY1NDC(statY1-lineH); st->SetY2NDC(statY1+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 3-prong, Sum of alpha1 + alpha2 + alpha3 track lengths, zoomed X=[20mm, 100mm], log Y scale @ 8.66 MeV 
  //// 3-prong, Sum of alpha1 + alpha2 + alpha3 track lengths, zoomed X=[30mm, 130mm], log Y scale
  if(plot_3prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_3prong_lenSum")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_log_%s",h1->GetName()));
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h1->Rebin(4);
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    //    h1->GetXaxis()->SetRangeUser(30.0, 130.0); // valid for 11.9 MeV
    //    h1->GetXaxis()->SetRangeUser(20.0, 100.0); // valid for 8.66 MeV
    h1->GetXaxis()->SetRangeUser(20.0, 100.0); // valid for 9.845 MeV
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->SetLogy(true);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX1); st->SetX2NDC(statX1+lineW); st->SetY1NDC(statY1); st->SetY2NDC(statY1+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 3-prong, Sum of alpha1 + alpha2 + alpha3 track lengths, zoomed X=[20mm, 100mm], linear Y scale @ 8.66 MeV
  //// 3-prong, Sum of alpha1 + alpha2 + alpha3 track lengths, zoomed X=[30mm, 130mm], linear Y scale
  if(plot_3prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_3prong_lenSum")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h1->GetName()));
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h1->Rebin(4);
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    //    h1->GetXaxis()->SetRangeUser(30.0, 130.0); // valid for 11.9 MeV
    //    h1->GetXaxis()->SetRangeUser(20.0, 100.0); // valid for 8.66 MeV
    h1->GetXaxis()->SetRangeUser(20.0, 100.0); // valid for 9.845 MeV
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->SetLogy(false);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX1); st->SetX2NDC(statX1+lineW); st->SetY1NDC(statY1); st->SetY2NDC(statY1+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 3-prong, Longest alpha track length, zoomed X=[0mm, 80mm], log Y scale @ 8.66 MeV
  //// 3-prong, Longest alpha track length, zoomed X=[30mm, 130mm], log Y scale
  if(plot_3prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_3prong_alpha1_len")->Clone(); // copy for modifications of the same histogram
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    c->SetName(Form("c_log_%s",h1->GetName()));
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h1->Rebin(4);
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    //    h1->GetXaxis()->SetRangeUser(30.0, 130.0); // valid for 11.9 MeV
    //    h1->GetXaxis()->SetRangeUser(0.0, 80.0); // valid for 8.66 MeV
    h1->GetXaxis()->SetRangeUser(0.0, 80.0); // valid for 9.845 MeV
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->SetLogy(true);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX1); st->SetX2NDC(statX1+lineW); st->SetY1NDC(statY1); st->SetY2NDC(statY1+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 3-prong, Longest alpha track length, zoomed X=[0mm, 80mm], linear Y scale @ 8.66 MeV
  //// 3-prong, Longest alpha track length, zoomed X=[30mm, 130mm], linear Y scale
  if(plot_3prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_3prong_alpha1_len")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h1->GetName()));
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h1->Rebin(4);
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    //    h1->GetXaxis()->SetRangeUser(30.0, 130.0); // valid for 11.9 MeV
    //    h1->GetXaxis()->SetRangeUser(0.0, 80.0); // valid for 8.66 MeV
    h1->GetXaxis()->SetRangeUser(0.0, 80.0); // valid for 9.845 MeV
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->SetLogy(false);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX1); st->SetX2NDC(statX1+lineW); st->SetY1NDC(statY1); st->SetY2NDC(statY1+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 3-prong, Longest alpha cos(theta_BEAM_LAB), linear Y scale
  if(plot_3prong) {
    h1=(TH1D*)f->Get("h_3prong_alpha1_cosThetaBEAM_LAB")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h1->GetName()));
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h1->Rebin(2);
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->SetLogy(false);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX1-0.2); st->SetX2NDC(statX1-0.2+lineW); st->SetY1NDC(statY1); st->SetY2NDC(statY1+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 3-prong, Carbon-12 excitation energy above ground state, CMS, linear Y scale
  if(plot_3prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_3prong_excitation_E_CMS")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h1->GetName()));
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    //    h1->Rebin(4);
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    //    h1->GetXaxis()->SetRangeUser(10.0, 14.0); // valid for 11.9 MeV
    //    h1->GetXaxis()->SetRangeUser(9.0, 13.0); // valid for 11.5 MeV
    h1->GetXaxis()->SetRangeUser(7.5, 11.0); // valid for 9.845 MeV 
    //    h1->GetXaxis()->SetRangeUser(6.5, 10.5); // valid for 8.66 MeV
    //    h1->GetXaxis()->SetRangeUser(0.0, 10.0);
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->SetLogy(false);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX1); st->SetX2NDC(statX1+lineW); st->SetY1NDC(statY1); st->SetY2NDC(statY1+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 3-prong, reaction Q-value, CMS, linear Y scale
  if(plot_3prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_3prong_Qvalue_CMS")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h1->GetName()));
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    //    h1->Rebin(4);
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    h1->GetXaxis()->SetRangeUser(0.0, 4.0); // valid for 9.845 MeV
    //    h1->GetXaxis()->SetRangeUser(2.0, 5.0); // valid for 11.5 MeV
    //    h1->GetXaxis()->SetRangeUser(3.0, 7.0); // valid for 11.9 MeV
    gPad->SetLeftMargin(0.125);
    gPad->SetRightMargin(0.1);
    gPad->SetLogy(false);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX1); st->SetX2NDC(statX1+lineW); st->SetY1NDC(statY1); st->SetY2NDC(statY1+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 3-prong, Dalitz plot, invariant mass of each alpha pair (symmetrized)
  if(plot_3prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_3prong_Dalitz1_CMS")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(4,4);
    h2->UseCurrentStyle();
    h2->SetStats(true);
    h2->Draw("COLZ");
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.2, "Z");
    gPad->SetLogy(false);
    gPad->SetLogz(false);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 3-prong, Dalitz plot, chi/kappa variables (symmeterized)
  if(plot_3prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_3prong_Dalitz2_CMS")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(4,4);
    h2->UseCurrentStyle();
    h2->SetStats(true);
    h2->Draw("COLZ");
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.2, "Z");
    gPad->SetLogy(false);
    gPad->SetLogz(false);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 3-prong, Dalitz plot, Excitation energy vs Alpha energy in CMS
  if(plot_3prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_3prong_Dalitz3_CMS")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(4,4);
    h2->UseCurrentStyle();
    h2->SetStats(true);
    h2->Draw("COLZ");
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.2, "Z");
    gPad->SetLogy(false);
    gPad->SetLogz(false);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //// 3-prong, Dalitz plot, Sum of alpha energies vs Alpha energy in CMS
  if(plot_3prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_3prong_Dalitz4_CMS")->Clone(); // copy for modifications of the same histogram
    c->SetName(Form("c_%s",h2->GetName()));
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(4,4);
    h2->UseCurrentStyle();
    h2->SetStats(true);
    h2->Draw("COLZ");
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.2, "Z");
    gPad->SetLogy(false);
    gPad->SetLogz(false);
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
    }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
    c->Write();
  }

  //////////////////////////////////
  f->Close();
  c->Print(((string)(prefix)+".pdf]").c_str());
  out->Close();

  return;
}

//////////////////////////////////
//////////////////////////////////
void makeCombinedPlots_report(std::string fileNameHistosAuto, std::string fileNameHistosManual, float energyMeV, const char *commentHistosAuto="Automatic", const char *commentHistosManual="Manual"){

  const auto rebin=2; //5;
  const auto width=2;

  TFile *f1 = new TFile(fileNameHistosAuto.c_str(), "OLD"); // AUTOMATIC RECO
  if(!f1) exit(-1);
  TFile *f2 = new TFile(fileNameHistosManual.c_str(), "OLD"); // MANUAL RECO
  if(!f2) exit(-1);

  std::string prefix=Form("figures2_%.3fMeV_rebin%d", energyMeV, rebin);
  std::string energyInfo=Form("E_{#gamma}=%g MeV : ", energyMeV);//Form("E_{#gamma}=%.3f MeV : ", energyMeV);

  gStyle->SetOptFit(111); // PCEV : prob=disabled / chi2,errors,variables=enabled
  gStyle->SetOptStat(10); // KISOURMEN : entries=enabled, name=disabled
  gStyle->SetPadLeftMargin(0.125);
  gStyle->SetPadBottomMargin(0.125);
  gStyle->SetTitleOffset(1.6, "X");
  gStyle->SetTitleOffset(1.7, "Y");

  TCanvas *c=new TCanvas("c","c",800,600);
  c->Print(((string)(prefix)+".pdf[").c_str());
  gPad->SetLogy(false);

  TH1D *h1, *h1_range;
  std::vector<TH1D*> stack;
  std::vector<double> integral;
  std::vector<std::string> label;
  std::vector<std::string> label_style;

  //// 2-prong, Sum of alpha + carbon track lengths - automatic RECO
  f1->cd();
  h1=(TH1D*)f1->Get("h_2prong_lenSum")->Clone(); // copy for modifications of the same histogram
  if(!h1) { std::cout << "ERROR: missing 2-prong histogram!" << std::endl;  return; }
  h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
  h1->UseCurrentStyle();
  h1->SetStats(false);
  h1->SetLineWidth(width);
  h1->SetLineColor(kBlue);
  h1->Rebin(rebin);
  stack.push_back(h1);
  label.push_back(Form("%s 2-prong, %.0f evts", commentHistosAuto, h1->GetEntries()));
  //  label.push_back(Form("%s 2-prong, %.1f#times10^{3} evts", commentHistosAuto, h1->GetEntries()*1e-3));
  label_style.push_back("L");

  //// 2-prong, Sum of alpha + carbon track lengths - manual RECO
  f2->cd();
  h1=(TH1D*)f2->Get("h_2prong_lenSum")->Clone(); // copy for modifications of the same histogram
  if(!h1) { std::cout << "ERROR: missing 2-prong histogram!" << std::endl;  return; }
  h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
  h1->UseCurrentStyle();
  h1->SetStats(false);
  h1->SetLineWidth(width);
  h1->SetLineColor(kRed);
  //  h1->SetFillStyle(3004); // 45 deg hatch
  //  h1->SetFillColor(kRed);
  h1->Rebin(rebin);
  stack.push_back(h1);
  label.push_back(Form("%s 2-prong, %.0f evts", commentHistosManual, h1->GetEntries()));
  //  label.push_back(Form("%s 2-prong, %.1f#times10^{3} evts", commentHistosManual, h1->GetEntries()*1e-3));
  label_style.push_back("FL");

  //// 3-prong, Sum of alpha1 + alpha2 + alpha3 track lengths - AUTOMATIC RECO
  f1->cd();
  h1=(TH1D*)f1->Get("h_3prong_lenSum")->Clone(); // copy for modifications of the same histogram
  if(!h1) { std::cout << "ERROR: missing 3-prong histogram!" << std::endl;  return; }
  h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
  h1->UseCurrentStyle();
  h1->SetStats(false);
  h1->SetLineWidth(width);
  h1->SetLineColor(kBlack);
  h1->SetFillStyle(3004); // 45 deg hatch
  h1->SetFillColor(kBlack);
  //  h1->SetLineColor(kMagenta);
  //  h1->SetFillStyle(3144);// double 45 deg hatch
  //  h1->SetFillColor(kMagenta);
  h1->Rebin(rebin);
  stack.push_back(h1);
  label.push_back(Form("%s 3-prong, %.0f evts", commentHistosAuto, h1->GetEntries()));
  label_style.push_back("FL");

  //// 3-prong, Sum of alpha1 + alpha2 + alpha3 track lengths - MANUAL RECO
  f2->cd();
  h1=(TH1D*)f2->Get("h_3prong_lenSum")->Clone(); // copy for modifications of the same histogram
  if(!h1) { std::cout << "ERROR: missing 3-prong histogram!" << std::endl;  return; }
  h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
  h1->UseCurrentStyle();
  h1->SetStats(false);
  h1->SetLineWidth(width);
  h1->SetLineColor(kMagenta);
  h1->SetFillStyle(3144);// double 45 deg hatch
  h1->SetFillColor(kMagenta);
  h1->Rebin(rebin);
  stack.push_back(h1);
  label.push_back(Form("%s 3-prong, %.0f evts", commentHistosManual, h1->GetEntries()));
  label_style.push_back("FL");

  // compute normalization factors
  integral.push_back(1.0);
  integral.push_back(1.0);
  integral.push_back(1.0);
  integral.push_back(1.0);
  //  integral.push_back(stack[1]->GetEntries()/(stack[1]->GetEntries()+stack[2]->GetEntries()+stack[3]->GetEntries()));
  //  integral.push_back(stack[2]->GetEntries()/(stack[1]->GetEntries()+stack[2]->GetEntries()+stack[3]->GetEntries()));
  //  integral.push_back(stack[3]->GetEntries()/(stack[1]->GetEntries()+stack[2]->GetEntries()+stack[3]->GetEntries()));

  // initialize legend
  auto legend = new TLegend(0.59, 0.7, 0.59+0.4, 0.7+0.2);

  // plot stack of normalized histograms with errors, log Y scale
  c->Clear();
  legend->Clear();
  auto first=true;
  auto ymax=0.0;
  for(auto index=0; index<stack.size(); index++) {
    std::string option="HIST";
    if(!first) option="HIST SAME";
    h1=(TH1D*)(stack[index]->DrawNormalized(option.c_str(), integral[index]));
    if(!h1) continue;
    ymax=std::max(ymax, h1->GetMaximum());
    if(first) {
      h1_range=h1;
      h1->GetXaxis()->SetRangeUser(20.0, 130.0);
      h1->SetXTitle("Sum of all track lengths [mm]");
      h1->SetYTitle("Probability [arb.u.]");
      h1->SetTitle((energyInfo+"Comparison of two reconstruction methods").c_str());
      gPad->SetLeftMargin(0.125);
      gPad->SetRightMargin(0.1);
      gPad->SetLogy(true);
      first=false;
    }
    legend->AddEntry(h1, label[index].c_str(), label_style[index].c_str());
  }
  h1_range->SetMaximum(1.05*ymax);
  h1_range->Draw("HIST SAME");
  legend->Draw();
  gPad->Update();
  c->Update();
  c->Modified();
  c->Print(((string)(prefix)+".pdf").c_str());

  // plot stack of normalized histograms with errors, linear Y scale
  c->Clear();
  legend->Clear();
  first=true;
  ymax=0.0;
  for(auto index=0; index<stack.size(); index++) {
    std::string option="HIST";
    if(!first) option="HIST SAME";
    h1=(TH1D*)(stack[index]->DrawNormalized(option.c_str(), integral[index]));
    if(!h1) continue;
    ymax=std::max(ymax, h1->GetMaximum());
    if(first) {
      h1_range=h1;
      h1->GetXaxis()->SetRangeUser(20.0, 130.0);
      h1->SetXTitle("Sum of all track lengths [mm]");
      h1->SetYTitle("Probability [arb.u.]");
      h1->SetTitle("Comparison of two reconstruction methods");
      gPad->SetLeftMargin(0.125);
      gPad->SetRightMargin(0.1);
      gPad->SetLogy(false);
      first=false;
    }
    legend->AddEntry(h1, label[index].c_str(), label_style[index].c_str());
  }
  h1_range->SetMaximum(1.05*ymax);
  h1_range->Draw("HIST SAME");
  legend->Draw();
  gPad->Update();
  c->Update();
  c->Modified();
  c->Print(((string)(prefix)+".pdf").c_str());

  // reset arrays for next plots
  stack.resize(0);
  integral.resize(0);

  //// 2-prong, Alpha track length
  f1->cd();
  h1=(TH1D*)f1->Get("h_2prong_alpha_len")->Clone(); // copy for modifications of the same histogram
  if(!h1) { std::cout << "ERROR: missing 2-prong histogram!" << std::endl;  return; }
  h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
  h1->UseCurrentStyle();
  h1->SetStats(false);
  h1->SetLineWidth(width);
  h1->SetLineColor(kBlue);
  h1->Rebin(rebin);
  stack.push_back(h1);

  //// 2-prong, Alpha track length
  f2->cd();
  h1=(TH1D*)f2->Get("h_2prong_alpha_len")->Clone(); // copy for modifications of the same histogram
  if(!h1) { std::cout << "ERROR: missing 2-prong histogram!" << std::endl;  return; }
  h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
  h1->UseCurrentStyle();
  h1->SetStats(false);
  h1->SetLineWidth(width);
  h1->SetLineColor(kRed);
  //  h1->SetFillStyle(3004); // 45 deg hatch
  //  h1->SetFillColor(kRed);
  h1->Rebin(rebin);
  stack.push_back(h1);

  //// 3-prong, Longest alpha track length
  f1->cd();
  h1=(TH1D*)f1->Get("h_3prong_alpha1_len")->Clone(); // copy for modifications of the same histogram
  if(!h1) { std::cout << "ERROR: missing 3-prong histogram!" << std::endl;  return; }
  h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
  h1->UseCurrentStyle();
  h1->SetStats(false);
  h1->SetLineWidth(width);
  h1->SetLineColor(kBlack);
  h1->SetFillStyle(3004); // 45 deg hatch
  h1->SetFillColor(kBlack);
  //  h1->SetFillStyle(3144);// double 45 deg hatch
  //  h1->SetFillColor(kMagenta);
  h1->Rebin(rebin);
  stack.push_back(h1);

  //// 3-prong, Longest alpha track length
  f2->cd();
  h1=(TH1D*)f2->Get("h_3prong_alpha1_len")->Clone(); // copy for modifications of the same histogram
  if(!h1) { std::cout << "ERROR: missing 3-prong histogram!" << std::endl;  return; }
  h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
  h1->UseCurrentStyle();
  h1->SetStats(false);
  h1->SetLineWidth(width);
  h1->SetLineColor(kMagenta);
  h1->SetFillStyle(3144);// double 45 deg hatch
  h1->SetFillColor(kMagenta);
  h1->Rebin(rebin);
  stack.push_back(h1);

  // compute normalization factors
  integral.push_back(1.0);
  integral.push_back(1.0);
  integral.push_back(1.0);
  integral.push_back(1.0);
  //  integral.push_back(stack[1]->GetEntries()/(stack[1]->GetEntries()+stack[2]->GetEntries()+stack[3]->GetEntries()));
  //  integral.push_back(stack[2]->GetEntries()/(stack[1]->GetEntries()+stack[2]->GetEntries()+stack[3]->GetEntries()));
  //  integral.push_back(stack[3]->GetEntries()/(stack[1]->GetEntries()+stack[2]->GetEntries()+stack[3]->GetEntries()));

  // plot stack of normalized histograms with errors, log Y scale
  c->Clear();
  legend->Clear();
  first=true;
  ymax=0.0;
  for(auto index=0; index<stack.size(); index++) {
    std::string option="HIST";
    if(!first) option="HIST SAME";
    h1=(TH1D*)(stack[index]->DrawNormalized(option.c_str(), integral[index]));
    if(!h1) continue;
    ymax=std::max(ymax, h1->GetMaximum());
    if(first) {
      h1_range=h1;
      h1->GetXaxis()->SetRangeUser(0.0, 100.0);
      h1->SetXTitle("Leading #alpha track length [mm]");
      h1->SetYTitle("Probability [arb.u.]");
      h1->SetTitle("Comparison of two reconstruction methods");
      gPad->SetLeftMargin(0.125);
      gPad->SetRightMargin(0.1);
      gPad->SetLogy(true);
      first=false;
    }
    legend->AddEntry(h1, label[index].c_str(), label_style[index].c_str());
  }
  h1_range->SetMaximum(1.05*ymax);
  h1_range->Draw("HIST SAME");
  legend->Draw();
  gPad->Update();
  c->Update();
  c->Modified();
  c->Print(((string)(prefix)+".pdf").c_str());

  // plot stack of normalized histograms with errors, linear Y scale
  c->Clear();
  legend->Clear();
  first=true;
  ymax=0.0;
  for(auto index=0; index<stack.size(); index++) {
    std::string option="HIST";
    if(!first) option="HIST SAME";
    h1=(TH1D*)(stack[index]->DrawNormalized(option.c_str(), integral[index]));
    if(!h1) continue;
    ymax=std::max(ymax, h1->GetMaximum());
    if(first) {
      h1_range=h1;
      h1->GetXaxis()->SetRangeUser(0.0, 100.0);
      h1->SetXTitle("Leading #alpha track length [mm]");
      h1->SetYTitle("Probability [arb.u.]");
      h1->SetTitle("Comparison of two reconstruction methods");
      gPad->SetLeftMargin(0.125);
      gPad->SetRightMargin(0.1);
      gPad->SetLogy(false);
      first=false;
    }
    legend->AddEntry(h1, label[index].c_str(), label_style[index].c_str());
  }
  h1_range->SetMaximum(1.05*ymax);
  h1_range->Draw("HIST SAME");
  legend->Draw();
  gPad->Update();
  c->Update();
  c->Modified();
  c->Print(((string)(prefix)+".pdf").c_str());

  f1->Close();
  f2->Close();
  c->Print(((string)(prefix)+".pdf]").c_str());

  return;
}
//////////////////////////////////
//////////////////////////////////
