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
#include "GeometryTPC.h"

void makePlots_HIGS(std::string fileNameHistos, float energyMeV, std::string cutInfo=""){

  bool plot_2prong=true;
  bool plot_3prong=true;
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
  gStyle->SetOptStat(10); // KISOURMEN : entries=enabled, name=disabled
  gStyle->SetPadLeftMargin(0.125);
  gStyle->SetPadBottomMargin(0.125);
  gStyle->SetTitleOffset(1.6, "X");
  gStyle->SetTitleOffset(1.7, "Y");

  TCanvas *c=new TCanvas("c","c",800,600);

  std::string prefix=Form("figures_%.3fMeV", energyMeV);
  c->Print(((string)(prefix)+".pdf[").c_str());
  gPad->SetLogy(false);

  TH1D *h1;
  TH2D *h2;
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
  std::string energyInfo=Form("E_{#gamma}=%.3f MeV : ", energyMeV);
  
  ///// control plot: number of tracks
  h1=(TH1D*)f->Get("h_ntracks")->Clone(); // copy for modifications of the same histogram
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

  //// ALL categories, any track endpoint X vs Y
  c->Clear();
  h2=(TH2D*)f->Get("h_all_endXY")->Clone(); // copy for modifications of the same histogram
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

  //// 2-prong, alpha endpoint X vs Y
  if(plot_2prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_alpha_endXY")->Clone(); // copy for modifications of the same histogram
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
  }

  //// 2-prong, carbon endpoint X vs Y
  if(plot_2prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_carbon_endXY")->Clone(); // copy for modifications of the same histogram
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
  }

  //// 2-prong, vertex X vs Y (ALL) 
  if(plot_2prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_vertexXY")->Clone(); // copy for modifications of the same histogram
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
  }

  //// 2-prong, vertex X vs Y, zoomed Y=[-15mm, 15mm]
  if(plot_2prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_vertexXY")->Clone(); // copy for modifications of the same histogram
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
  }

  //// 2-prong, profile histogram: vertex X vs average vertex Y
  if(plot_2prong) {
    c->Clear();
    hp=(TProfile*)f->Get("h_2prong_vertexXY_prof")->Clone(); // copy for modifications of the same histogram
    hp->SetTitle((energyInfo+hp->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    hp->Rebin(1);
    hp->Draw();
    hp->UseCurrentStyle();
    hp->SetStats(true);
    hp->SetLineWidth(2);
    TF1 *tiltFunc=new TF1("tiltFunc", "[0]+[1]*x", hp->GetXaxis()->GetXmin()+20, hp->GetXaxis()->GetXmax()-20);
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
  }

  //// 2-prong, vertex X distribution (along beam)
  if(plot_2prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_2prong_vertexX")->Clone(); // copy for modifications of the same histogram
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
  }

  //// 2-prong, vertex Y distribution, zoomed Y=[-15mm, 15mm], no correction for beam tilt
  if(plot_2prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_2prong_vertexY")->Clone(); // copy for modifications of the same histogram
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
  }

  //// 2-prong, Sum of alpha + carbon track lengths, zoomed X=[20mm, 100mm], log Y scale @ 8.66 MeV
  //// 2-prong, Sum of alpha + carbon track lengths, zoomed X=[30mm, 120mm], log Y scale @ 9.845 MeV
  //// 2-prong, Sum of alpha + carbon track lengths, zoomed X=[30mm, 130mm], log Y scale @ 11.5 MeV
  //// 2-prong, Sum of alpha + carbon track lengths, zoomed X=[30mm, 150mm], log Y scale @ 12.3 MeV
  if(plot_2prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_2prong_lenSUM")->Clone(); // copy for modifications of the same histogram
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
  }

  //// 2-prong, Sum of alpha + carbon track lengths, zoomed X=[20mm, 100mm], linear Y scale @ 8.66 MeV
  //// 2-prong, Sum of alpha + carbon track lengths, zoomed X=[30mm, 120mm], linear Y scale @ 9.845 MeV
  //// 2-prong, Sum of alpha + carbon track lengths, zoomed X=[30mm, 130mm], linear Y scale @ 11.5 MeV
  //// 2-prong, Sum of alpha + carbon track lengths, zoomed X=[30mm, 150mm], linear Y scale @ 12.3 MeV
  if(plot_2prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_2prong_lenSUM")->Clone(); // copy for modifications of the same histogram
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
  }

  //// 2-prong, Alpha track length, zoomed X=[10mm, 80mm], log Y scale @ 8.66 MeV
  //// 2-prong, Alpha track length, zoomed X=[20mm, 100mm], log Y scale @ 9.845 MeV
  //// 2-prong, Alpha track length, zoomed X=[30mm, 130mm], log Y scale @ 11.5 MeV
  //// 2-prong, Alpha track length, zoomed X=[30mm, 150mm], log Y scale @ 12.3 MeV
  if(plot_2prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_2prong_alpha_len")->Clone(); // copy for modifications of the same histogram
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
  }

  //// 16O : 2-prong, Alpha track length, zoomed X=[20mm, 40mm], linear Y scale @ 8.66 MeV
  //// 2-prong, Alpha track length, zoomed X=[40mm, 70mm], linear Y scale @ 9.845 MeV
  //// 2-prong, Alpha track length, zoomed X=[60mm, 80mm], linear Y scale @ 11.5 MeV
  //// 2-prong, Alpha track length, zoomed X=[75mm, 105mm], linear Y scale @ 12.3 MeV
  if(plot_2prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_2prong_alpha_len")->Clone(); // copy for modifications of the same histogram
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    h1->GetXaxis()->SetRangeUser(20.0, 40.0); // valid for 8.66 MeV
    //    h1->GetXaxis()->SetRangeUser(40.0, 70.0); // valid for 9.845 MeV
    //    h1->GetXaxis()->SetRangeUser(60.0, 80.0); // valid for 11.5 MeV
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
  }

  //// 2-prong, Alpha track length (X) vs Carbon track length (Y), zoomed X=[10mm, 80mm] x Y=[4mm, 18mm] @ 8.66 MeV
  //// 2-prong, Alpha track length (X) vs Carbon track length (Y), zoomed X=[20mm, 120mm] x Y=[4mm, 18mm] @ 9.845 MeV
  //// 2-prong, Alpha track length (X) vs Carbon track length (Y), zoomed X=[20mm, 120mm] x Y=[4mm, 18mm] @ 11.5 MeV
  //// 2-prong, Alpha track length (X) vs Carbon track length (Y), zoomed X=[20mm, 120mm] x Y=[4mm, 18mm] @ 12.3 MeV
  if(plot_2prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_alpha_len_carbon_len")->Clone(); // copy for modifications of the same histogram
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(2,1);
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
    //    h2->GetXaxis()->SetRangeUser(20, 120); // valid for 11.5 MeV
    //    h2->GetYaxis()->SetRangeUser(4, 18); // valid for 11.5 MeV
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
  }

  //// define 2D cut to select Oxygen-16 only for next 3 plots
  Double_t theta[5]  = {-1,    1,       1,    -1, -1};
  //  Double_t length[5] = {58, 58+5, 58+5+20, 58+20, 58}; // valid for 11.5MeV
  //  Double_t length[5] = {78, 78+7, 78+7+20, 78+20, 78}; // valid for 12.3MeV
  //  Double_t length[5] = {38, 38+7, 38+7+20, 38+20, 38}; // valid for 9.845 MeV
  Double_t length[5] = {15, 15+2, 15+2+20, 15+20, 15}; // valid for 8.66 MeV

  TCutG *mycut=new TCutG("select_O16", 5, theta, length);
  mycut->SetLineColor(kRed);
  mycut->SetLineWidth(3);

  //// 2-prong, Alpha track length (X) vs Alpha cos(theta_BEAM_LAB), zoomed Y=[20mm, 120mm]
  if(plot_2prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_alpha_cosThetaBEAM_len_LAB")->Clone(); // copy for modifications of the same histogram
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
    //    h2->GetYaxis()->SetRangeUser(20, 120);
    h2->GetYaxis()->SetRangeUser(0, 80); // valid for 8.66 MeV
    gPad->Update();
    st = (TPaveStats *)c->GetPrimitive("stats");
    if(st) {
      st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
    }
    mycut->Draw("SAME");
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
  }

  //// 2-prong, Alpha track length (X) vs Alpha cos(theta_BEAM_LAB), zoomed Y=[15mm, 40mm], tagged O-16 @ 8.66 MeV
  //// 2-prong, Alpha track length (X) vs Alpha cos(theta_BEAM_LAB), zoomed Y=[30mm, 70mm], tagged O-16 @ 9.845 MeV
  //// 2-prong, Alpha track length (X) vs Alpha cos(theta_BEAM_LAB), zoomed Y=[50mm, 90mm], tagged O-16 @ 11.5 MeV
  //// 2-prong, Alpha track length (X) vs Alpha cos(theta_BEAM_LAB), zoomed Y=[70mm, 110mm], tagged O-16 @ 12.3 MeV
  if(plot_2prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_alpha_cosThetaBEAM_len_LAB")->Clone(); // copy for modifications of the same histogram
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    //  gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h2->Rebin2D(2,2);
    h2->UseCurrentStyle();
    h2->SetStats(false);
    h2->Draw("COLZ [select_O16]");
    h2->SetTitle(Form("%s, ^{16}O candidates", h2->GetTitle())); // modify the title
    mycut->Draw("SAME");
    gPad->SetLeftMargin(0.1);
    gPad->SetRightMargin(0.14);
    h2->SetTitleOffset(1.6, "X");
    h2->SetTitleOffset(1.4, "Y");
    h2->SetTitleOffset(1.2, "Z");
    //    h2->GetYaxis()->SetRangeUser(50, 90); // valid for 11.5 MeV
    //    h2->GetYaxis()->SetRangeUser(70, 110); // valid for 12.3 MeV
    //    h2->GetYaxis()->SetRangeUser(30, 70); // valid for 9.845 MeV
    h2->GetYaxis()->SetRangeUser(15, 40); // valid for 8.66 MeV
    gPad->Update();
    //  st = (TPaveStats *)c->GetPrimitive("stats");
    //  if(st) {
    //    st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH);
    //  }
    c->Update();
    c->Modified();
    c->Print(((string)(prefix)+".pdf").c_str());
  }

  //// 2-prong, Alpha cos(theta_BEAM_LAB), tagged Oxygen-16
  if(plot_2prong) {
    h2=(TH2D*)f->Get("h_2prong_alpha_cosThetaBEAM_len_LAB")->Clone(); // copy for modifications of the same histogram
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    h1=h2->ProjectionX("_px",1, h2->GetNbinsY(),"[select_O16]");
    h1->Rebin(4);
    h1->SetYTitle(h2->GetZaxis()->GetTitle());
    h1->SetTitle(Form("%s, ^{16}O candidates", h2->GetTitle())); // modify the title
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
  }

  //// 2-prong, Alpha phi_BEAM_LAB, no correction for beam tilt
  if(plot_2prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_2prong_alpha_phiBEAM_LAB")->Clone(); // copy for modifications of the same histogram
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
  }

  //// 2-prong, (Alpha track enpoint_Y vs vertex_Y) vs (Alpha track enpoint_Z vs vertex_Z)
  if(plot_2prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_2prong_alpha_deltaYZ")->Clone(); // copy for modifications of the same histogram
    h2->SetTitle((energyInfo+h2->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    //    h2->Rebin2D(10,10);
    h2->Rebin2D(20,20);
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
  }
  //////////////////////////////////

  //// 3-prong, alpha1 endpoint X vs Y
  if(plot_3prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_3prong_alpha1_endXY")->Clone(); // copy for modifications of the same histogram
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
  }

  //// 3-prong, vertex X vs Y (ALL)
  if(plot_3prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_3prong_vertexXY")->Clone(); // copy for modifications of the same histogram
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
  }

  //// 3-prong, vertex X vs Y, zoomed Y=[-15mm, 15mm]
  if(plot_3prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_3prong_vertexXY")->Clone(); // copy for modifications of the same histogram
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
  }

  //// 3-prong, vertex X distribution (along beam)
  if(plot_3prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_3prong_vertexX")->Clone(); // copy for modifications of the same histogram
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
  }

  //// 3-prong, vertex Y distribution, zoomed Y=[-15mm, 15mm], no correction for beam tilt
  if(plot_3prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_3prong_vertexY")->Clone(); // copy for modifications of the same histogram
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
  }

  //// 3-prong, Sum of alpha1 + alpha2 + alpha3 track lengths, zoomed X=[20mm, 100mm], log Y scale @ 8.66 MeV 
  //// 3-prong, Sum of alpha1 + alpha2 + alpha3 track lengths, zoomed X=[30mm, 130mm], log Y scale
  if(plot_3prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_3prong_lenSUM")->Clone(); // copy for modifications of the same histogram
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h1->Rebin(4);
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    //    h1->GetXaxis()->SetRangeUser(30.0, 130.0);
    h1->GetXaxis()->SetRangeUser(20.0, 100.0); // valid for 8.66 MeV
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
  }

  //// 3-prong, Sum of alpha1 + alpha2 + alpha3 track lengths, zoomed X=[20mm, 100mm], linear Y scale @ 8.66 MeV
  //// 3-prong, Sum of alpha1 + alpha2 + alpha3 track lengths, zoomed X=[30mm, 130mm], linear Y scale
  if(plot_3prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_3prong_lenSUM")->Clone(); // copy for modifications of the same histogram
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h1->Rebin(4);
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    //    h1->GetXaxis()->SetRangeUser(30.0, 130.0);
    h1->GetXaxis()->SetRangeUser(20.0, 100.0); // valid for 8.66 MeV
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
  }

  //// 3-prong, Longest alpha track length, zoomed X=[0mm, 80mm], log Y scale @ 8.66 MeV
  //// 3-prong, Longest alpha track length, zoomed X=[30mm, 130mm], log Y scale
  if(plot_3prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_3prong_alpha1_len")->Clone(); // copy for modifications of the same histogram
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h1->Rebin(4);
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    //    h1->GetXaxis()->SetRangeUser(30.0, 130.0);
    h1->GetXaxis()->SetRangeUser(0.0, 80.0); // valid for 8.66 MeV
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
  }

  //// 3-prong, Longest alpha track length, zoomed X=[0mm, 80mm], linear Y scale @ 8.66 MeV
  //// 3-prong, Longest alpha track length, zoomed X=[30mm, 130mm], linear Y scale
  if(plot_3prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_3prong_alpha1_len")->Clone(); // copy for modifications of the same histogram
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    h1->Rebin(4);
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    //    h1->GetXaxis()->SetRangeUser(30.0, 130.0);
    h1->GetXaxis()->SetRangeUser(0.0, 80.0); // valid for 8.66 MeV
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
  }

  //// 3-prong, Longest alpha cos(theta_BEAM_LAB), linear Y scale
  if(plot_3prong) {
    h1=(TH1D*)f->Get("h_3prong_alpha1_cosThetaBEAM_LAB")->Clone(); // copy for modifications of the same histogram
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
  }

  //// 3-prong, Carbon-12 excitation energy above ground state, CMS, linear Y scale
  if(plot_3prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_3prong_excitation_E_CMS")->Clone(); // copy for modifications of the same histogram
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    //    h1->Rebin(4);
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    h1->GetXaxis()->SetRangeUser(0.0, 10.0);
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

  }

  //// 3-prong, reaction Q-value, CMS, linear Y scale
  if(plot_3prong) {
    c->Clear();
    h1=(TH1D*)f->Get("h_3prong_Qvalue_CMS")->Clone(); // copy for modifications of the same histogram
    h1->SetTitle((energyInfo+h1->GetTitle()).c_str());
    gStyle->SetOptStat(10); // KISOURMEN : show entries only
    //    h1->Rebin(4);
    h1->Draw();
    h1->UseCurrentStyle();
    h1->SetStats(true);
    h1->SetLineWidth(2);
    h1->SetFillColor(plot_fill_color);
    h1->SetFillStyle(plot_fill_style);
    h1->GetXaxis()->SetRangeUser(0.0, 4.0);
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

  }

  //// 3-prong, Dalitz plot, invariant mass of each alpha pair (symmetrized)
  if(plot_3prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_3prong_Dalitz1_CMS")->Clone(); // copy for modifications of the same histogram
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

  }

  //// 3-prong, Dalitz plot, chi/kappa variables (symmeterized)
  if(plot_3prong) {
    c->Clear();
    h2=(TH2D*)f->Get("h_3prong_Dalitz2_CMS")->Clone(); // copy for modifications of the same histogram
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

  }

  //////////////////////////////////
  f->Close();
  c->Print(((string)(prefix)+".pdf]").c_str());

  return;
}

//////////////////////////////////
//////////////////////////////////
void makeCombinedPlots_report(std::string fileNameHistosAuto, std::string fileNameHistosManual, float energyMeV, const char *commentHistosAuto="Automatic", const char *commentHistosManual="Manual"){

  const auto rebin=5;
  const auto width=2;

  TFile *f1 = new TFile(fileNameHistosAuto.c_str(), "OLD"); // AUTOMATIC RECO
  if(!f1) exit(-1);
  TFile *f2 = new TFile(fileNameHistosManual.c_str(), "OLD"); // MANUAL RECO
  if(!f2) exit(-1);

  std::string prefix=Form("figures2_%.3fMeV_rebin%d", energyMeV, rebin);
  std::string energyInfo=Form("E_{#gamma}=%.3f MeV : ", energyMeV);

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
  h1=(TH1D*)f1->Get("h_2prong_lenSUM")->Clone(); // copy for modifications of the same histogram
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
  h1=(TH1D*)f2->Get("h_2prong_lenSUM")->Clone(); // copy for modifications of the same histogram
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
  h1=(TH1D*)f1->Get("h_3prong_lenSUM")->Clone(); // copy for modifications of the same histogram
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
  h1=(TH1D*)f2->Get("h_3prong_lenSUM")->Clone(); // copy for modifications of the same histogram
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
