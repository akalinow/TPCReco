/*
Usage:

[user1@d21ee95b36ff resources]$ root
root [0] .L makeMCPlots.cpp
root [1] makeMCPlots("TrackTree_MCTest.root")

*/
void makeMCPlots(std::string fileName){

  gStyle->SetOptFit(11);
  gStyle->SetOptStat(0);

  TFile *aFile = new TFile(fileName.c_str());
  TTree *trackTree = (TTree*)aFile->Get("trackTree");


  trackTree->Draw("(alphaRangeReco-alphaRangeGen)/alphaRangeGen:alphaRangeGen>>hRangeResVsRangeGen(21,0, 70,  41,-0.5,0.5)","eventTypeGen==1","goff");
  trackTree->Draw("(alphaRangeReco-alphaRangeGen)/alphaRangeGen:cosThetaGen>>hRangeResVsCosTheta(21,-1, 1,  41,-0.5,0.5)","eventTypeGen==1","goff");
  trackTree->Draw("(cosThetaReco-cosThetaGen)/cosThetaGen:cosThetaGen>>hCosThetaResVsCosTheta(21,-1, 1,  41,-0.5,0.5)","eventTypeGen==1","goff");
  trackTree->Draw("(phiReco-phiGen)/phiGen:cosThetaGen>>hPhiResVsCosTheta(21,-0.5, 0.5,  41,-0.5,0.5)","eventTypeGen==1","goff");
  trackTree->Draw("(chargeReco*1E-5-chargeGen)/chargeGen:cosThetaGen>>hChargeResVsCosTheta(21,-1, 1,  41,-0.5,0.5)","eventTypeGen==1","goff");
  ///////////////////////////////////////////////////
  TLegend *aLeg = new TLegend(0.1, 0.1, 0.5, 0.3);
  TCanvas *aCanvas = new TCanvas("aCanvas","",700,700);
  aCanvas->Divide(2,2);


  ////////////////////////////////
  aCanvas->cd(1);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);

  TH2F *hRangeResVsCosTheta = (TH2F*)gDirectory->Get("hRangeResVsCosTheta");
  hRangeResVsCosTheta->SetTitle("");
  hRangeResVsCosTheta->SetXTitle("cos(#theta^{GEN})");
  hRangeResVsCosTheta->SetYTitle("#frac{d^{RECO} - d^{GEN}}{d^{RECO}}");
  hRangeResVsCosTheta->SetZTitle("Number of events");
  hRangeResVsCosTheta->Draw("colz");
  ////////////////////////////////
  aCanvas->cd(2);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);

  TH2F *hRangeResVsRangeGen = (TH2F*)gDirectory->Get("hRangeResVsRangeGen");
  hRangeResVsRangeGen->SetTitle("");
  hRangeResVsRangeGen->SetXTitle("d^{GEN} [mm]");
  hRangeResVsRangeGen->SetYTitle("#frac{d^{RECO} - d^{GEN}}{d^{RECO}}");
  hRangeResVsRangeGen->SetZTitle("Number of events");
  hRangeResVsRangeGen->Draw("colz");

  TH2F *hCosThetaResVsCosTheta = (TH2F*)gDirectory->Get("hCosThetaResVsCosTheta");
  hCosThetaResVsCosTheta->SetTitle("");
  hCosThetaResVsCosTheta->SetXTitle("cos(#theta^{GEN})");
  hCosThetaResVsCosTheta->SetYTitle("#frac{cos(#theta^{RECO}) - cos(#theta^{GEN})}{cos(#theta^{GEN})}");
  hCosThetaResVsCosTheta->SetZTitle("Number of events");
  //hCosThetaResVsCosTheta->Draw("colz");
  ////////////////////////////////
  aCanvas->cd(3);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);

  TH2F *hPhiResVsCosTheta = (TH2F*)gDirectory->Get("hPhiResVsCosTheta");
  hPhiResVsCosTheta->SetTitle("");
  hPhiResVsCosTheta->SetXTitle("cos(#theta^{GEN})");
  hPhiResVsCosTheta->SetYTitle("#frac{#varphi^{RECO} - #varphi^{GEN}}{#varphi^{RECO}}");
  hPhiResVsCosTheta->SetZTitle("Number of events");
  hPhiResVsCosTheta->Draw("colz");
  ////////////////////////////////
  aCanvas->cd(4);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);

  TH2F *hChargeResVsCosTheta = (TH2F*)gDirectory->Get("hChargeResVsCosTheta");
  hChargeResVsCosTheta->SetTitle("");
  hChargeResVsCosTheta->SetXTitle("cos(#theta^{GEN})");
  hChargeResVsCosTheta->SetYTitle("#frac{charge^{RECO} - charge^{GEN}}{charge^{RECO}}");
  hChargeResVsCosTheta->SetZTitle("Number of events");
  hChargeResVsCosTheta->Draw("colz");
  ////////////////////////////////
  aCanvas->Print("MCPlots_set0.png");
  ////////////////////////////////
  return;
  ////////////////////////////////
  aCanvas->Clear();
  aCanvas->Divide(2,2);

  ////////////////////////////////
  aCanvas->cd(1);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);

  hRangeResVsCosTheta->FitSlicesY();
  TH2F *hRangeResVsCosTheta_mean = (TH2F*)gDirectory->Get("hRangeResVsCosTheta_1");
  TH2F *hRangeResVsCosTheta_sigma = (TH2F*)gDirectory->Get("hRangeResVsCosTheta_2");
  //hRangeResVsCosTheta_mean->Draw();
  hRangeResVsCosTheta_sigma->Draw();
 
}


