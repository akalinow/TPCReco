/*
Usage:

[user1@d21ee95b36ff resources]$ root
root [0] .L makePlots.cpp
root [1] makeCalibrationPlots("TrackAnalysis_2021-06-22T14:11:08.614.root")

*/
void makeCalibrationPlots(std::string fileName){

  gStyle->SetOptFit(11);
  gStyle->SetOptStat(0);

  TFile *aFile = new TFile(fileName.c_str());
  TTree *trackTree = (TTree*)aFile->Get("trackTree");

  TH2F *hChargeVsLength = new TH2F("hChargeVsLength", "Track charge vs length; d[mm];charge [arbitrary units]",200,0,200, 50,0, 1E5);
  TH2F *hCosThetaVsLength = new TH2F("hCosThetaVsLength", "Track cos(#theta) vs length; d[mm];cos(#theta)",201,-1, 200, 11, 0, 1.1);
  
  TH1D *hLength = new TH1D("hLength", "Track length; d[mm]; Number of events", 200,0,200);
  TH1D *hLengthCut0 = (TH1D*)hLength->Clone("hLengthCut0");
  TH1D *hLengthCut1 = (TH1D*)hLength->Clone("hLengthCut1");
  hLengthCut0->SetStats(kFALSE);
  hLengthCut1->SetStats(kFALSE);
    
  TH1D *hPhi = new TH1D("hPhi", "Track direction azimuthal angle; #varphi; Number of events", 20, -TMath::Pi(), TMath::Pi());
  TH1D *hPhiCut0 = (TH1D*)hPhi->Clone("hPhiCut0");
  hPhiCut0->SetStats(kFALSE);

  TH1D *hChi2 = new TH1D("hChi2","Track loss function;loss function;Number of events",50,-5,20);
  TH1D *hChi2Cut0 = (TH1D*)hChi2->Clone("hChi2Cut0");
  TH1D *hChi2Cut1 = (TH1D*)hChi2->Clone("hChi2Cut1");
  hChi2Cut0->SetStats(kFALSE);
  hChi2Cut1->SetStats(kFALSE);

  TH3D *hPosXYZCut0 = new TH3D("hPosXYZCut0", "Track start position; x [mm]; y[mm]; z[mm]",
				 50, -150, 150,
				 50, -150, 150,
				 50, -150, 150);
  
  TH3D *hPosXYZCut1 = (TH3D*)hPosXYZCut0->Clone("hPosXYZCut1");
  hPosXYZCut1->SetStats(kFALSE);

  TCut qualityCut = "chi2>-100 && length>0";
  TCut sourcePositionCut = "abs(x0-127)<25 && abs(y0-76)<25";
  TCut cut0 = "cosTheta<0.5"&&qualityCut;
  TCut cut1 = "cosTheta>0.9"&&qualityCut;
  TCut cut2 = cut1&&qualityCut&&sourcePositionCut;

  trackTree->Draw("charge:length>>hChargeVsLength","", "goff");
  trackTree->Draw("cosTheta:length>>hCosThetaVsLength","", "goff");
  trackTree->Draw("length>>hLength","", "goff");
  trackTree->Draw("phi>>hPhiCut0",cut0, "goff");
  trackTree->Draw("length+horizontalLostLength>>hLengthCut0",cut0, "goff");
  trackTree->Draw("length+verticalLostLength>>hLengthCut1",cut1, "goff");
  trackTree->Draw("length>>hLengthCut1",cut2, "goff");
  trackTree->Draw("z0:y0:x0>>hPosXYZCut0",cut0, "goff");
  trackTree->Draw("z0:y0:x0>>hPosXYZCut1",cut1, "goff");
  trackTree->Draw("chi2>>hChi2Cut0",cut0, "goff");
  trackTree->Draw("chi2>>hChi2Cut1",cut1, "goff");
  //trackTree->Scan("frameId:eventId:chi2","chi2>2");
  ///////////////////////////////////////////////////
  TCanvas *aCanvas = new TCanvas("aCanvas","",700,700);
  aCanvas->Divide(2,2);

  aCanvas->cd(1);
  hCosThetaVsLength->Draw("colz");

  aCanvas->cd(2);
  hPosXYZCut1->Project3D("xy")->Draw("colz");
  TLegend *aLeg = new TLegend(0.1, 0.1, 0.5, 0.3);
  aLeg->AddEntry(hLengthCut0, "cos(#theta)>0.9","l");
  aLeg->Draw();


  aCanvas->cd(3);
  hLengthCut0->SetTitle("Track length +  73.4/cos(#alpha)");
  hLengthCut0->SetLineColor(4);
  hLengthCut0->SetLineStyle(2);
  hLengthCut0->SetLineWidth(3);
  hLengthCut0->SetMaximum(1.5*hLengthCut0->GetMaximum());
  hLengthCut0->Draw("same");

  TFitResultPtr fitResult = hLengthCut0->Fit("gaus", "s", "",80, 120);
  double mu = fitResult->Parameter(1);
  double sigma = fitResult->Parameter(2);
  TLatex *aLabel = new TLatex(100, hLengthCut0->GetMaximum()*0.7,
			      TString::Format("#mu= %3.2f #sigma= %3.2f", mu, sigma));  
  aLabel->Draw();
  
  aLeg = new TLegend(0.35, 0.75, 0.9, 0.9);
  aLeg->AddEntry(hLengthCut0, "cos(#theta)<0.5","l");
  aLeg->Draw();

  aCanvas->cd(4);
  hLengthCut1->SetTitle("Track length +  6.0/cos(#alpha)");
  hLengthCut1->SetLineColor(4);
  hLengthCut1->SetLineStyle(2);
  hLengthCut1->SetLineWidth(3);
  hLengthCut1->SetMaximum(1.5*hLengthCut1->GetMaximum());
  hLengthCut1->Draw("same");
   
  aLeg = new TLegend(0.35, 0.75, 0.9, 0.9);
  aLeg->AddEntry(hLengthCut1, "cos(#theta)>0.9","l");
  fitResult = hLengthCut1->Fit("gaus", "s", "",80, 100);
  mu = fitResult->Parameter(1);
  sigma = fitResult->Parameter(2);
  aLabel = new TLatex(100, hLengthCut1->GetMaximum()*0.7,
		      TString::Format("#mu= %3.2f #sigma= %3.2f", mu, sigma));  
  aLeg->Draw();
  aLabel->Draw();
  aCanvas->Print("Calibration_plots_set0.png");

  int horizontalTkCount = trackTree->GetEntries(cut0);
  int verticalTkCount = trackTree->GetEntries(cut2);
  double ratio = (double)verticalTkCount/horizontalTkCount;
  double ratioError = ratio*sqrt(1.0/horizontalTkCount +  1.0/verticalTkCount);
  std::cout<<"Vertical Tk count: "<<verticalTkCount<<std::endl;
  std::cout<<"Horizontal Tk count: "<<horizontalTkCount<<std::endl;
  std::cout<<"Ratio ver./hor.: "<<ratio<<" +- "<<ratioError<<std::endl;
  std::cout<<"Expected value (0.25/25):\t"<<0.25/25<<std::endl;
  ////////////////
  //return;
  ////////////////
  aCanvas->Clear();
  aCanvas->Divide(2,2);

  aCanvas->cd(1);
  TH1D *hPosX =   hPosXYZCut1->ProjectionX("hPosX");
  hPosX->SetStats(kFALSE);
  hPosX->SetTitle("Track start X position");
  hPosX->SetMaximum(1.5*hPosX->GetMaximum());
  hPosX->Draw();
  aLeg = new TLegend(0.1, 0.8, 0.6, 0.9);
  aLeg->AddEntry(hPosX, "cos(#theta)>0.9","l");
  aLeg->Draw();
  fitResult = hPosX->Fit("gaus", "s", "",100, 150);
  mu = fitResult->Parameter(1);
  sigma = fitResult->Parameter(2);
  aLabel = new TLatex(-100, hPosX->GetMaximum()*0.6,
		      TString::Format("#mu= %3.2f #sigma= %3.2f", mu, sigma));
  aLabel->Draw();

  aCanvas->cd(2);
  TH1D *hPosY = hPosXYZCut1->ProjectionY("hPosY");
  hPosY->SetStats(kFALSE);
  hPosY->SetTitle("Track start Y position");
  hPosY->SetMaximum(1.5*hPosY->GetMaximum());
  hPosY->Draw();
  aLeg->Draw();
  fitResult = hPosY->Fit("gaus", "s", "",50, 150);
  mu = fitResult->Parameter(1);
  sigma = fitResult->Parameter(2);
  aLabel = new TLatex(-100, hPosY->GetMaximum()*0.6,
		      TString::Format("#mu= %3.2f #sigma= %3.2f", mu, sigma)); 
  aLabel->Draw();

  aCanvas->cd(3);
  TH1D *hPosZ = hPosXYZCut1->ProjectionZ("hPosZ");
  hPosZ->SetStats(kFALSE);
  hPosZ->SetTitle("Track start Z position");
  fitResult = hPosZ->Fit("gaus", "s", "",-100, -50);
  hPosZ->SetMaximum(1.5*hPosZ->GetMaximum());
  hPosZ->Draw();
  aLeg->Draw();
  fitResult = hPosZ->Fit("gaus", "s", "",-100, 0);
  mu = fitResult->Parameter(1);
  sigma = fitResult->Parameter(2);
  aLabel = new TLatex(0, hPosZ->GetMaximum()*0.6,
		      TString::Format("#mu= %3.2f #sigma= %3.2f", mu, sigma));
  aLabel->Draw();

  aCanvas->cd(4);
  hChi2Cut0->DrawNormalized();
  hChi2Cut1->SetLineColor(2);
  hChi2Cut1->DrawNormalized("same");

  aCanvas->Print("Calibration_plots_set1.png");
  //////////////////////////////////////
  //////////////////////////////////////
  aCanvas->Clear();
  aCanvas->Divide(2,2);

  aCanvas->cd(1);
  hPosXYZCut0->Project3D("yx")->Draw("colz");
  aLeg = new TLegend(0.1, 0.8, 0.6, 0.9);
  aLeg->AddEntry(hPosX, "cos(#theta)<0.5","l");
  aLeg->Draw();

  aCanvas->cd(2);
  hPosX = (TH1D*)hPosXYZCut0->Project3D("x");
  fitResult = hPosX->Fit("gaus", "s", "",-100,100);
  hPosX->SetMaximum(1.5*hPosX->GetMaximum());
  hPosX->Draw();
  aLeg->Draw();
  
  aLeg = new TLegend(0.1, 0.8, 0.6, 0.9);
  aLeg->AddEntry(hPosX, "cos(#theta)<0.5","l");
  aLeg->Draw();

  aCanvas->cd(3);
  hPosXYZCut0->Project3D("y")->Draw("colz");
  aLeg = new TLegend(0.1, 0.8, 0.6, 0.9);
  aLeg->AddEntry(hPosX, "cos(#theta)<0.5","l");
  aLeg->Draw();

  aCanvas->Print("Calibration_plots_set2.png");
  //////////////////////////////////////
  //////////////////////////////////////
}

