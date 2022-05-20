/*
Usage:

$ root -l
root [0] .L makePlots_report.cpp
root [1] makePlots_report("Histos.root", 11.5);

*/
void makePlots_report(std::string fileNameHistos, float energyMeV, std::string cutInfo=""){

  TFile *aFile = new TFile(fileNameHistos.c_str(), "OLD");
  if(!aFile) exit(-1);

  gStyle->SetOptFit(111); // PCEV : prob=disabled / chi2,errors,variables=enabled
  gStyle->SetOptStat(10); // KISOURMEN : entries=enabled, name=disabled
  gStyle->SetPadLeftMargin(0.125);
  gStyle->SetPadBottomMargin(0.125);
  gStyle->SetTitleOffset(1.6, "X");
  gStyle->SetTitleOffset(1.7, "Y");
  
  TCanvas *c=new TCanvas("c","c",800,600);

  std::string prefix=Form("figures_%.1fMeV", energyMeV);
  c->Print(((string)(prefix)+".pdf[").c_str());
  gPad->SetLogy(false);
  auto *f=TFile::Open("Histos.root");
  f->cd();

  TH1D *h1;
  TH2D *h2;
  TProfile *hp;
  TPaveStats *st;
  auto statX=0.75;
  auto statY=0.825;
  auto statX2=0.65;
  auto statY2=0.25;
  auto statX3=0.625;
  auto statY3=0.20;
  auto statX4=statX-0.1;
  auto statY4=statY;
  auto lineH=0.075;
  auto lineW=0.15;
  
  ///// control plot: number of tracks
  h1=(TH1D*)f->Get("h_ntracks");
  gStyle->SetOptStat(10); // KISOURMEN : show entries only
  h1->SetStats(true);
  h1->Draw("HIST");
  h1->Draw("TEXT00 SAME");
  h1->UseCurrentStyle();
  //  h1->SetLineColor(kBlack);
  h1->SetLineWidth(2);
  h1->GetXaxis()->SetNdivisions(5);
  c->Update();
  st = (TPaveStats *)c->GetPrimitive("stats");
  st->SetX1NDC(statX); st->SetX2NDC(statX+lineW); st->SetY1NDC(statY); st->SetY2NDC(statY+lineH);
  c->Update();
  c->Modified();
  c->Print(((string)(prefix)+".pdf").c_str());

  //// 2-prong, vertex X vs Y (ALL) 
  c->Clear();
  h2=(TH2D*)f->Get("h_2prong_vertexXY")->Clone(); // copy for modifications of the same histogram
  //  h2=(TH2D*)f->Get("h_2prong_vertexXY");
  gStyle->SetOptStat(1110); // KISOURMEN : show entries + mean + rms
  h2->Rebin2D(3,3);
  h2->UseCurrentStyle();
  h2->SetStats(true);
  h2->Draw("COLZ");
  gPad->SetLeftMargin(0.1);
  gPad->SetRightMargin(0.14);
  h2->SetTitleOffset(1.6, "X");
  h2->SetTitleOffset(1.4, "Y");
  h2->SetTitleOffset(1.2, "Z");
  gPad->Update();
  st = (TPaveStats *)c->GetPrimitive("stats");
  st->SetX1NDC(statX-0.04); st->SetX2NDC(statX-0.04+lineW); st->SetY1NDC(statY-lineH*2); st->SetY2NDC(statY+lineH);
  c->Update();
  c->Modified();
  c->Print(((string)(prefix)+".pdf").c_str());

  //// 2-prong, vertex X vs Y, zoomed Y=[-15mm, 15mm]
  c->Clear();
  h2=(TH2D*)f->Get("h_2prong_vertexXY")->Clone(); // copy for modifications of the same histogram
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
  st->SetX1NDC(statX-0.04); st->SetX2NDC(statX-0.04+lineW); st->SetY1NDC(statY-lineH*2); st->SetY2NDC(statY+lineH);
  c->Update();
  c->Modified();
  c->Print(((string)(prefix)+".pdf").c_str());

  //// 2-prong, vertex X distribution (along beam)
  c->Clear();
  h1=(TH1D*)f->Get("h_2prong_vertexX");
  gStyle->SetOptStat(1110); // KISOURMEN : show entries + mean + rms
  h1->Rebin(5);
  h1->Draw();
  h1->UseCurrentStyle();
  h1->SetStats(true);
  h1->SetLineWidth(2);
  gPad->SetLeftMargin(0.125);
  gPad->SetRightMargin(0.1);
  gPad->Update();
  st = (TPaveStats *)c->GetPrimitive("stats");
  st->SetX1NDC(statX2); st->SetX2NDC(statX2+lineW); st->SetY1NDC(statY2-lineH); st->SetY2NDC(statY2+lineH);
  c->Update();
  c->Modified();
  c->Print(((string)(prefix)+".pdf").c_str());

  //// 2-prong, vertex Y distribution, zoomed Y=[-15mm, 15mm], no correction for beam tilt
  c->Clear();
  h1=(TH1D*)f->Get("h_2prong_vertexY");
  gStyle->SetOptStat(1110); // KISOURMEN : show entries + mean + rms
  h1->Draw();
  h1->UseCurrentStyle();
  h1->SetStats(true);
  h1->SetLineWidth(2);
  h1->GetXaxis()->SetRangeUser(-15.0, 15.0);
  gPad->SetLeftMargin(0.125);
  gPad->SetRightMargin(0.1);
  gPad->Update();
  st = (TPaveStats *)c->GetPrimitive("stats");
  st->SetX1NDC(statX); st->SetX2NDC(statX+lineW); st->SetY1NDC(statY-lineH); st->SetY2NDC(statY+lineH);
  c->Update();
  c->Modified();
  c->Print(((string)(prefix)+".pdf").c_str());

  //// 2-prong, Sum of alpha + C track lengths, zoomed X=[30mm, 130mm], log Y scale
  c->Clear();
  h1=(TH1D*)f->Get("h_2prong_alpha_len");
  gStyle->SetOptStat(10); // KISOURMEN : show entries only
  h1->Draw();
  h1->UseCurrentStyle();
  h1->SetStats(true);
  h1->SetLineWidth(2);
  h1->GetXaxis()->SetRangeUser(30.0, 130.0);
  gPad->SetLeftMargin(0.125);
  gPad->SetRightMargin(0.1);
  gPad->SetLogy(true);
  gPad->Update();
  st = (TPaveStats *)c->GetPrimitive("stats");
  st->SetX1NDC(statX); st->SetX2NDC(statX+lineW); st->SetY1NDC(statY); st->SetY2NDC(statY+lineH);
  c->Update();
  c->Modified();
  c->Print(((string)(prefix)+".pdf").c_str());

  //// 2-prong, Sum of alpha + C track lengths, zoomed X=[60mm, 80mm], linear Y scale
  c->Clear();
  h1=(TH1D*)f->Get("h_2prong_alpha_len");
  gStyle->SetOptStat(10); // KISOURMEN : show entries only
  h1->Draw();
  h1->UseCurrentStyle();
  h1->SetStats(true);
  h1->SetLineWidth(2);
  h1->GetXaxis()->SetRangeUser(60.0, 80.0);
  gPad->SetLeftMargin(0.125);
  gPad->SetRightMargin(0.1);
  gPad->SetLogy(false);
  gPad->Update();
  st = (TPaveStats *)c->GetPrimitive("stats");
  st->SetX1NDC(statX); st->SetX2NDC(statX+lineW); st->SetY1NDC(statY); st->SetY2NDC(statY+lineH);
  c->Update();
  c->Modified();
  c->Print(((string)(prefix)+".pdf").c_str());

  //// 2-prong, Alpha track length (X) vs Carbon track length (Y), zoomed X=[20mm, 120mm] x Y=[4mm, 18mm]
  c->Clear();
  h2=(TH2D*)f->Get("h_2prong_alpha_len_carbon_len")->Clone(); // copy for modifications of the same histogram
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
  h2->GetXaxis()->SetRangeUser(20, 120);
  h2->GetYaxis()->SetRangeUser(4, 18);
  gPad->Update();
  st = (TPaveStats *)c->GetPrimitive("stats");
  st->SetX1NDC(statX-0.04); st->SetX2NDC(statX-0.04+lineW); st->SetY1NDC(statY); st->SetY2NDC(statY+lineH);
  c->Update();
  c->Modified();
  c->Print(((string)(prefix)+".pdf").c_str());

  //// define 2D cut to select Oxygen-16 only for next 3 plots
  Double_t theta[5]  = {-1,    1,       1,    -1, -1};
  Double_t length[5] = {58, 58+5, 58+5+20, 58+20, 58};
  TCutG *mycut=new TCutG("select_O16", 5, theta, length);
  mycut->SetLineColor(kRed);
  mycut->SetLineWidth(3);
 
  //// 2-prong, Alpha track length (X) vs Alpha cos(theta_BEAM_LAB), zoomed Y=[20mm, 120mm]
  c->Clear();
  h2=(TH2D*)f->Get("h_2prong_alpha_cosThetaBEAM_len_LAB")->Clone(); // copy for modifications of the same histogram
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
  h2->GetYaxis()->SetRangeUser(20, 120);
  gPad->Update();
  st = (TPaveStats *)c->GetPrimitive("stats");
  st->SetX1NDC(statX-0.04); st->SetX2NDC(statX-0.04+lineW); st->SetY1NDC(statY); st->SetY2NDC(statY+lineH);
  mycut->Draw("SAME");
  c->Update();
  c->Modified();
  c->Print(((string)(prefix)+".pdf").c_str());

  //// 2-prong, Alpha track length (X) vs Alpha cos(theta_BEAM_LAB), zoomed Y=[50mm, 90mm], tagged O-16
  c->Clear();
  h2=(TH2D*)f->Get("h_2prong_alpha_cosThetaBEAM_len_LAB")->Clone(); // copy for modifications of the same histogram
  //  gStyle->SetOptStat(10); // KISOURMEN : show entries only
  h2->Rebin2D(2,2);
  h2->UseCurrentStyle();
  h2->SetStats(false);
  h2->Draw("COLZ [select_O16]");
  h2->SetTitle(Form("%s - after ^{16}O cut", h2->GetTitle())); // modify the title
  mycut->Draw("SAME");
  gPad->SetLeftMargin(0.1);
  gPad->SetRightMargin(0.14);
  h2->SetTitleOffset(1.6, "X");
  h2->SetTitleOffset(1.4, "Y");
  h2->SetTitleOffset(1.2, "Z");
  h2->GetYaxis()->SetRangeUser(50, 90);
  gPad->Update();
  //  st = (TPaveStats *)c->GetPrimitive("stats");
  //  st->SetX1NDC(statX-0.04); st->SetX2NDC(statX-0.04+lineW); st->SetY1NDC(statY); st->SetY2NDC(statY+lineH);
  c->Update();
  c->Modified();
  c->Print(((string)(prefix)+".pdf").c_str());

  //// 2-prong, Alpha cos(theta_BEAM_LAB), tagged Oxygen-16
  h2=(TH2D*)f->Get("h_2prong_alpha_cosThetaBEAM_len_LAB");
  h1=h2->ProjectionX("_px",1, h2->GetNbinsY(),"[select_O16]");
  h1->Rebin(4);
  h1->SetYTitle(h2->GetZaxis()->GetTitle());
  h1->SetTitle(Form("%s - after ^{16}O cut", h2->GetTitle())); // modify the title
  h1->Draw("HIST E1");
  h1->UseCurrentStyle();
  h1->SetStats(true);
  h1->SetLineWidth(2);
  h1->GetXaxis()->SetRangeUser(60.0, 80.0);
  h1->GetMinimum(0);
  gPad->SetLeftMargin(0.125);
  gPad->SetRightMargin(0.1);
  gPad->SetLogy(false);
  gPad->Update();
  st = (TPaveStats *)c->GetPrimitive("stats");
  st->SetX1NDC(statX); st->SetX2NDC(statX+lineW); st->SetY1NDC(statY); st->SetY2NDC(statY+lineH);
  c->Update();
  c->Modified();
  c->Print(((string)(prefix)+".pdf").c_str());

  //// 2-prong, Alpha phi_BEAM_LAB, no correction for beam tilt
  c->Clear();
  h1=(TH1D*)f->Get("h_2prong_alpha_phiBEAM_LAB");
  gStyle->SetOptStat(10); // KISOURMEN : show entries only
  h1->Rebin(5);
  h1->Draw();
  h1->UseCurrentStyle();
  h1->SetStats(true);
  h1->SetLineWidth(2);
  h1->SetMinimum(0);
  gPad->SetLeftMargin(0.125);
  gPad->SetRightMargin(0.1);
  gPad->SetLogy(false);
  TF1 *myfun=new TF1("myfun", "[0]*(1+[1]*cos(2*(x+[2])))", h1->GetXaxis()->GetXmin(), h1->GetXaxis()->GetXmax());
  myfun->SetParNames("A", "f", "#delta");
  h1->Fit(myfun);
  h1->Draw("E1 SAME");
  gPad->Update();
  st = (TPaveStats *)c->GetPrimitive("stats");
  st->SetX1NDC(statX3); st->SetX2NDC(statX3+lineW*1.5); st->SetY1NDC(statY3); st->SetY2NDC(statY3+lineH*2.5);
  TLatex *tl=new TLatex;
  tl->SetTextSize(0.030);
  tl->SetTextAlign(11); // align at bottom, left
  tl->DrawLatexNDC(0.2, 0.275, "Fit :  #font[12]{#frac{dN}{d#phi} = A #[]{1 + f cos 2#(){#phi + #delta}}}");
  c->Update();
  c->Modified();
  c->Print(((string)(prefix)+".pdf").c_str());
  
  //// 2-prong, (Alpha track enpoint_Y vs vertex_Y) vs (Alpha track enpoint_Z vs vertex_Z)
  c->Clear();
  h2=(TH2D*)f->Get("h_2prong_alpha_deltaYZ")->Clone(); // copy for modifications of the same histogram
  gStyle->SetOptStat(10); // KISOURMEN : show entries only
  h2->Rebin2D(10,10);
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
  st->SetX1NDC(statX4); st->SetX2NDC(statX4+lineW); st->SetY1NDC(statY4); st->SetY2NDC(statY4+lineH);
  c->Update();
  c->Modified();
  c->Print(((string)(prefix)+".pdf").c_str());
  
  //////////////////////////////////
  f->Close();
  c->Print(((string)(prefix)+".pdf]").c_str());

  return;
}
