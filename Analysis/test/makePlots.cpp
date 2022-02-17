/*
Usage:

[user1@d21ee95b36ff resources]$ root
root [0] .L makePlots.cpp
root [1] makePlots("TrackAnalysis_2021-06-22T14:11:08.614.root")

*/
void makePlots(std::string fileName){

  gStyle->SetOptFit(11);
  gStyle->SetOptStat(0);

  TFile *aFile = new TFile(fileName.c_str());
  TTree *trackTree = (TTree*)aFile->Get("trackTree");
  
  TH1D *hLength = new TH1D("hLength", "Track length; d[mm]; Number of events", 200,0,200);
  TH1D *hLengthCut0 = (TH1D*)hLength->Clone("hLengthCut0");
  TH1D *hLengthCut1 = (TH1D*)hLength->Clone("hLengthCut1");
  hLengthCut0->SetStats(kFALSE);
  hLengthCut1->SetStats(kFALSE);
    
  TH3D *hPosXYZ = new TH3D("hPosXYZ", "Track start position; x [mm]; y[mm]; z[mm]",
				 50, -150, 150,
				 50, -150, 150,
				 50, -150, 150);
  
  TH3D *hPosXYZCut1 = (TH3D*)hPosXYZ->Clone("hPosXYZCut1");
  hPosXYZCut1->SetStats(kFALSE);
						

  TCut cut0 = "cosTheta>0.9 && abs(x0-127)<20 && abs(y0-76)<20";
  TCut cut1 = "!(abs(x0-127)<20 && abs(y0-76)<20)";

  trackTree->Draw("charge:length>>hChargeVsLength","", "goff");
  trackTree->Draw("cosTheta:length>>hCosThetaVsLength","", "goff");
  trackTree->Draw("length+verticalLostLength>>hLengthCut0",cut0, "goff");
  trackTree->Draw("length>>hLengthCut1",cut1, "goff");
  trackTree->Draw("z0:y0:x0>>hPosXYZCut1",cut1, "goff");
  trackTree->Draw("z1:y1:x1>>+hPosXYZCut1",cut1, "goff");
  ///////////////////////////////////////////////////
  TLegend *aLeg = new TLegend(0.1, 0.1, 0.5, 0.3);
  TCanvas *aCanvas = new TCanvas("aCanvas","",700,700);
  aCanvas->Divide(2,2);

  ////////////////////////////////
  aCanvas->cd(1);

  hLengthCut0->SetLineColor(4);
  hLengthCut0->SetLineStyle(2);
  hLengthCut0->SetLineWidth(3);
  hLengthCut0->SetMaximum(1.5*hLengthCut0->GetMaximum());
  hLengthCut0->Draw("");

  aLeg = new TLegend(0.45, 0.8, 0.9, 0.9);
  aLeg->AddEntry(hLengthCut0, "vertical #alpha","l");
  aLeg->Draw();
  //aLabel->Draw();
  ////////////////////////////////
  aCanvas->cd(2);

  hLengthCut1->SetLineColor(4);
  hLengthCut1->SetLineStyle(2);
  hLengthCut1->SetLineWidth(3);
  hLengthCut1->SetMaximum(1.5*hLengthCut1->GetMaximum());
  hLengthCut1->Draw("same");

  aLeg = new TLegend(0.45, 0.8, 0.9, 0.9);
  aLeg->AddEntry(hLengthCut1, "signal","l");
  aLeg->Draw();
  ////////////////////////////////
  aCanvas->cd(3);
  hPosXYZCut1->Project3D("xy")->Draw("colz");
  
  aLeg = new TLegend(0.45, 0.8, 0.9, 0.9);
  aLeg->AddEntry(hLengthCut0, "signal start pos.","l");
  //aLabel->Draw();
  ////////////////
  aCanvas->Print("Plots_set0.png");
  ////////////////
  ////////////////
}


