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

  TH2F *hChargeVsLength = new TH2F("hChargeVsLength", "Track charge vs length; d[mm];charge [arbitrary units]",100, 0,200, 50,0, 1E5);
  TH2F *hCosThetaVsLength = new TH2F("hCosThetaVsLength", "Track cos(#theta) vs length; d[mm];cos(#theta)",100, 0,200, 10,0, 1);
  TH1D *hLength = new TH1D("hLength", "Track length; d[mm]; Number of events", 200,0,200);
  TH1D *hLengthCut0 = new TH1D("hLengthCut0", "Track length; d[mm]; Number of events", 200,0,200);
  TH1D *hLengthCut0_1 = new TH1D("hLengthCut0_1", "Track length; d[mm]; Number of events", 200,0,200);
  
  TH1D *hEnergy = new TH1D("hEnergy", "Track energy; E [MeV]; Number of events", 80, 2, 6);
  TH1D *hEnergyCut0 = new TH1D("hEnergyCut0", "Track energy; E [MeV]; Number of events", 80, 2, 6);
  TH1D *hEnergyCut0_1 = new TH1D("hEnergyCut0_1", "Track energy; E [MeV]; Number of events", 80, 2, 6);

  
  TH1D *hPhi = new TH1D("hPhi", "Track direction azimuthal angle; #varphi; Number of events", 20, -TMath::Pi(), TMath::Pi());
  TH1D *hCosTheta = new TH1D("hCosTheta", "Track direction cos(#theta); cos(#theta); Number of events", 20, -1, 1);
  TH2D *hPosXY = new TH2D("hPosXY", "Track start position; x [mm]; y[mm]", 50, -150, 150, 50, -150, 150);
  TH3D *hPosXYZCut0_1 = new TH3D("hPosXYZCut0_1", "Track start and stop position; x [mm]; y[mm]; z[mm]",
				 50, -150, 150,
				 50, -150, 150,
				 50, -150, 150);
  
  trackTree->Draw("charge:length>>hChargeVsLength","", "goff");
  trackTree->Draw("cosTheta:length>>hCosThetaVsLength","", "goff");
  trackTree->Draw("length>>hLength","", "goff");

  TCut cut0 = "cosTheta>0.9";
  TCut cut1 = "abs(x0-127)<20 && abs(y0-76)<20";
  
  trackTree->Draw("length>>hLengthCut0",cut0, "goff");
  trackTree->Draw("length>>hLengthCut0_1",cut0&&cut1, "goff");
  trackTree->Draw("energy>>hEnergyCut0",cut0, "goff");
  trackTree->Draw("energy>>hEnergyCut0_1",cut0&&cut1, "goff");
  trackTree->Draw("y0:x0>>hPosXY",cut0, "goff");
  trackTree->Draw("z0:y0:x0>>hPosXYZCut0_1",cut0&&cut1, "goff");

  TCanvas *aCanvas = new TCanvas("aCanvas","",700,700);
  aCanvas->Divide(2,2);

  aCanvas->cd(1);
  hCosThetaVsLength->Draw("colz");

  aCanvas->cd(2);
  hPosXY->Draw("colz");

  aCanvas->cd(3);
  hLength->Draw("");
  hLengthCut0->SetLineColor(2);
  hLengthCut0->SetLineStyle(2);
  hLengthCut0->Draw("same");

  hLengthCut0_1->SetLineColor(4);
  hLengthCut0_1->SetLineStyle(2);
  hLengthCut0_1->SetLineWidth(3);
  hLengthCut0_1->Draw("same");
  
  hLengthCut0->Print();
  TLegend *aLeg = new TLegend(0.35, 0.7, 0.9, 0.9);
  aLeg->AddEntry(hLength, "all tracks","l");
  aLeg->AddEntry(hLengthCut0, "cos(#theta)>0.9","l");
  aLeg->AddEntry(hLengthCut0_1, "cos(#theta)>0.9, |x-127|<20, |y-76|<20","l");
  aLeg->Draw();


  aCanvas->cd(4);
  hEnergyCut0->SetLineColor(2);
  hEnergyCut0->SetLineStyle(2);
  hEnergyCut0->Draw("");

  hEnergyCut0_1->SetLineColor(4);
  hEnergyCut0_1->SetLineStyle(2);
  hEnergyCut0_1->SetLineWidth(3);  
  hEnergyCut0_1->Draw("same");

  TFitResultPtr fitResult = hEnergyCut0->Fit("gaus", "s", "",5, 5.5);
  hEnergy->Print();

  double ratio = fitResult->Parameter(2)/fitResult->Parameter(1);
  TLatex *aLabel = new TLatex(4.5, hEnergyCut0->GetMaximum()*0.7,TString::Format("#sigma/#mu= %3.2f", ratio));  
  aLabel->Draw();
  
  TLegend *aLeg1 = new TLegend(0.0, 0.7, 0.4, 0.9);
  aLeg1->AddEntry(hEnergyCut0, "cos(#theta)>0.9","l");
  aLeg1->AddEntry(hEnergyCut0_1, "cos(#theta)>0.9, |x-127|<20, |y-76|<20","l");
  //aLeg1->Draw();

  aCanvas->Print("Plots_set0.png");

  ////////////////
  ////////////////
  aCanvas->Clear();
  aCanvas->Divide(2,2);

  aCanvas->cd(1);
  TH1D *hPosX = hPosXY->ProjectionX("hPosX");
  hPosX->SetTitle("Track start X position");
  fitResult = hPosX->Fit("gaus", "s", "",100, 150);
  hPosX->SetMaximum(1.5*hPosX->GetMaximum());
  hPosX->Draw();
  ratio = fitResult->Parameter(2)/fitResult->Parameter(1);
  aLabel->DrawLatex(50, hPosX->GetMaximum()*0.7,TString::Format("#sigma/#mu= %3.2f", ratio));
  TLegend *aLeg2 = new TLegend(0.1, 0.8, 0.6, 0.9);
  aLeg2->AddEntry(hPosX, "cos(#theta)>0.9","l");
  aLeg2->Draw();

  aCanvas->cd(2);
  TH1D *hPosY = hPosXY->ProjectionY("hPosY");
  hPosY->SetTitle("Track start Y position");
  fitResult = hPosY->Fit("gaus", "s", "",50, 100);
  hPosY->SetMaximum(1.5*hPosY->GetMaximum());
  hPosY->Draw();
  ratio = fitResult->Parameter(2)/fitResult->Parameter(1);
  aLabel->DrawLatex(50, hPosY->GetMaximum()*0.7,TString::Format("#sigma/#mu= %3.2f", ratio));  
  aLeg2->Draw();

  aCanvas->cd(3);
  TH1D *hPosZ = hPosXYZCut0_1->ProjectionZ("hPosZ");
  hPosZ->SetTitle("Track start Z position");
  fitResult = hPosZ->Fit("gaus", "s", "",-100, -50);
  hPosZ->SetMaximum(1.5*hPosZ->GetMaximum());
  hPosZ->Draw();
  ratio = fitResult->Parameter(2)/fitResult->Parameter(1);
  aLabel->DrawLatex(50, 20,TString::Format("#sigma/#mu= %3.2f", ratio));

  TLegend *aLeg3 = new TLegend(0.1, 0.8, 0.6, 0.9);
  aLeg3->AddEntry(hPosZ, "cos(#theta)>0.9, |x-127|<20, |y-76|<20","l");
  aLeg3->Draw();

  aCanvas->Print("Track_plots.png");
  aCanvas->Print("Plots_set1.png");

}

