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

  TH1D *hEnergy = new TH1D("hEnergy", "#alpha + ^{12}C energy;E(#alpha + ^{12}C) [MeV]; Number of events",50,4,9);  
  TH1D *hLength = new TH1D("hLength", "Track length; d[mm]; Number of events", 200,0,200);
  TH1D *hLengthCut0 = (TH1D*)hLength->Clone("hLengthCut0");
  TH1D *hLengthCut1 = (TH1D*)hLength->Clone("hLengthCut1");

  TH1D *hAlphaRange = (TH1D*)hLength->Clone("hAlphaRange");
  TH1D *hCarbonRange = (TH1D*)hLength->Clone("hCarbonRange");
  hLengthCut0->SetStats(kFALSE);
  hLengthCut1->SetStats(kFALSE);
  hAlphaRange->SetStats(kFALSE);
  hCarbonRange->SetStats(kFALSE);
    
  TH3D *hVtxXYZ = new TH3D("hVtxXYZ", "Vertex position; x [mm]; y[mm]; z[mm]",
				 50, -200, 200,
				 400, -200, 200,
				 50, -200, 200);
  
  TH3D *hVtxXYZCut1 = (TH3D*)hVtxXYZ->Clone("hVtxXYZCut1");
  TH3D *hVtxYCorrCut1 = (TH3D*)hVtxXYZ->Clone("hVtxYCorrCut1");
  hVtxXYZCut1->SetStats(kFALSE);
  hVtxYCorrCut1->SetStats(kFALSE);

  TH1D *hChi2 = new TH1D("hChi2","Track loss function;loss function;Number of events",50,-5,20);
  TH1D *hChi2Cut0 = (TH1D*)hChi2->Clone("hChi2Cut0");
  TH1D *hChi2Cut1 = (TH1D*)hChi2->Clone("hChi2Cut1");
  hChi2Cut0->SetStats(kFALSE);
  hChi2Cut1->SetStats(kFALSE);

  TH1D *hPhi = new TH1D("hPhi","Track azimuthal angle wrt. beam line;#varphi;Number of events",20,-M_PI,M_PI);
  TH1D *hPhiRaw = (TH1D*)hPhi->Clone("hPhiRaw");
  TH1D *hPhiCorr = (TH1D*)hPhi->Clone("hPhiCorr");
  
  TH1D *hCosTheta = new TH1D("hCosTheta","Track polar angle wrt. beam line;cos(#theta);Number of events",20,-1,1);
  TH1D *hCosThetaCut1 = (TH1D*)hCosTheta->Clone("hCosThetaCut1");
  hCosThetaCut1->SetStats(kFALSE);

  
  std::string qualityCut = "chi2<10 && charge>1000 && length>50 && eventType==3";
  std::string fiducialXYCut = "abs(xAlphaEnd)<160 && abs(yAlphaEnd)<100 && abs(xCarbonEnd)<160 && abs(yCarbonEnd)<100";
  std::string fiducialZCut = "abs(zAlphaEnd-zVtx)<100";
  std::string vertexCut = "abs(yVtx-2)<10";
  std::string cut0 = qualityCut;
  std::string cut1 = "1";
  cut1 += std::string("&&")+qualityCut;
  cut1 += std::string("&&")+vertexCut;
  cut1 += std::string("&&")+fiducialXYCut;
  cut1 += std::string("&&")+fiducialZCut;
  
  trackTree->Draw("zVtx:yVtx:xVtx>>hVtxXYZCut1",cut0.c_str(), "goff");
  trackTree->Draw("0:yVtx-(-2.042+0.004248*xVtx):0>>hVtxYCorrCut1",cut0.c_str(), "goff");
  
  trackTree->Draw("charge:length>>hChargeVsLength",qualityCut.c_str(), "goff");
  trackTree->Draw("length>>hLengthCut0",cut0.c_str(), "goff");
  trackTree->Draw("length>>hLengthCut1",cut1.c_str(), "goff");
  trackTree->Draw("alphaRange>>hAlphaRange",cut1.c_str(), "goff");
  trackTree->Draw("carbonRange>>hCarbonRange",cut1.c_str(), "goff");
  trackTree->Draw("alphaEnergy+carbonEnergy>>hEnergy",cut1.c_str(), "goff");

  trackTree->Draw("phi>>hPhiRaw",cut1.c_str(), "goff");
  trackTree->Draw("atan2(yAlphaEnd-(yVtx-(-2.042+0.004248*xVtx)),zAlphaEnd-zVtx)>>hPhiCorr",cut1.c_str(), "goff");
  
  trackTree->Draw("cosTheta>>hCosThetaCut1",cut1.c_str(), "goff");
  ///////////////////////////////////////////////////
  TLegend *aLeg = new TLegend(0.1, 0.1, 0.5, 0.3);
  TCanvas *aCanvas = new TCanvas("aCanvas","",700,700);
  aCanvas->Divide(2,2);

  ////////////////////////////////
  aCanvas->cd(1);
  TH1 *hProj = hVtxXYZCut1->Project3D("yx");
  hProj->GetYaxis()->SetRangeUser(-20,20);
  hProj->DrawCopy("colz");

  ////////////////////////////////
  aCanvas->cd(2);
  
  hProj = hVtxXYZCut1->Project3D("y");
  hProj->SetLineWidth(2);
  hProj->GetXaxis()->SetRangeUser(-20,30);

  TH1 *hProjYCorr = hVtxYCorrCut1->Project3D("y");
  hProjYCorr->SetLineStyle(2);
  hProjYCorr->SetLineColor(2);
  hProjYCorr->SetLineWidth(2);
      
  hProj->DrawCopy("");
  hProjYCorr->DrawCopy("same");

  aLeg = new TLegend(0.5, 0.8, 0.9, 0.9);
  aLeg->AddEntry(hProj, "raw vtx. Y","l");
  aLeg->AddEntry(hProjYCorr, "corrected vtx. Y","l");
  aLeg->Draw();
  ////////////////////////////////
  aCanvas->cd(3);
  hProj = hVtxXYZCut1->Project3D("x");
  hProj->GetXaxis()->SetRangeUser(-200,200);
  hProj->DrawCopy("");

  ////////////////////////////////
  aCanvas->cd(4);
  hProj = hVtxXYZCut1->Project3D("z");
  hProj->GetXaxis()->SetRangeUser(-100,100);
  hProj->Draw("");
 
  ////////////////  
  aCanvas->Print("Plots_set0.png");
  ////////////////
  //return;
  ////////////////
  aCanvas->Clear();
  aCanvas->Divide(2,2);

  ////////////////////////////////
  aCanvas->cd(1);
  hLengthCut0->SetTitle("#alpha + C tracks length");
  hLengthCut0->SetLineColor(1);
  hLengthCut0->SetLineStyle(2);
  hLengthCut0->SetLineWidth(3);
  hLengthCut0->SetMaximum(1.5*hLengthCut0->GetMaximum());
  hLengthCut0->GetXaxis()->SetRangeUser(50,200);
  hLengthCut0->Draw("");

  hLengthCut1->SetLineColor(2);
  hLengthCut1->SetLineStyle(2);
  hLengthCut1->SetLineWidth(3);
  hLengthCut1->SetMaximum(1.6*hLengthCut1->GetMaximum());
  hLengthCut1->Draw("same");

  aLeg = new TLegend(0.43, 0.65, 0.9, 0.9);
  aLeg->AddEntry(hLengthCut0, "NO cut on vtx.","l");
  aLeg->AddEntry(hLengthCut1, "#splitline{|y-2|<6 && z<-60}{#splitline{end_{X}<160 &&  end_{Y}<100}{|#alpha_{Z} - VTX_{Z}|<100}}","l");
  aLeg->Draw();
  ////////////////////////////////
  
  aCanvas->cd(2);
  hAlphaRange->SetTitle("#alpha track length");
  hAlphaRange->SetLineColor(4);
  hAlphaRange->SetLineWidth(3);
  hAlphaRange->SetMaximum(1.5*hAlphaRange->GetMaximum());
  hAlphaRange->GetXaxis()->SetRangeUser(80,150);
  hAlphaRange->Draw("");
  ////////////////////////////////
  
  aCanvas->cd(3);
  hCarbonRange->SetTitle("C track length");
  hCarbonRange->SetLineColor(4);
  hCarbonRange->SetLineWidth(3);
  hCarbonRange->SetMaximum(1.5*hCarbonRange->GetMaximum());
  hCarbonRange->GetXaxis()->SetRangeUser(0,20);
  hCarbonRange->Draw("");
  ////////////////////////////////
  aCanvas->cd(4);

  hEnergy->SetLineWidth(2);
  hEnergy->SetLineColor(4);
  hEnergy->Fit("gaus");
  hEnergy->Draw("");

  ////////////////
  aCanvas->Print("Plots_set1.png");
  //return;
  ////////////////
  aCanvas->Clear();
  aCanvas->Divide(2,2);

  ////////////////////////////////
  aCanvas->cd(1);


  hPhiRaw->SetLineColor(1);
  hPhiRaw->SetLineStyle(2);
  hPhiRaw->SetMaximum(1.5*hPhiRaw->GetMaximum());
  hPhiRaw->Draw();
  
  hPhiCorr->SetLineColor(4);
  hPhiCorr->Draw("same");
  aLeg = new TLegend(0.43, 0.65, 0.9, 0.9);
  aLeg->AddEntry(hPhiRaw, "#varphi from det. coordinates","l");
  aLeg->AddEntry(hPhiCorr, "#splitline{#varphi corrected for}{the beam direction}","l");
  aLeg->Draw();
  ////////////////////////////////
  aCanvas->cd(2);

  hCosThetaCut1->SetLineWidth(2);
  hCosThetaCut1->SetLineColor(4);
  hCosThetaCut1->Draw();
  

  /*
  hChi2Cut1->SetLineColor(2);
  hChi2Cut1->SetMaximum(1.2*hChi2Cut1->GetMaximum());
  hChi2Cut1->DrawNormalized();
  hChi2Cut0->DrawNormalized("same");
  aLeg = new TLegend(0.1, 0.8, 0.6, 0.9);
  aLeg->AddEntry(hChi2Cut0, "all tracks","l");
  aLeg->AddEntry(hChi2Cut1, "length>110","l");
  aLeg->Draw();
  */
  aCanvas->Print("Plots_set2.png");
  ////////////////
  ////////////////
 


}


