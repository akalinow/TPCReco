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

  TH1D *hEnergy = new TH1D("hEnergy", ";E(#alpha + ^{12}C) [MeV]; Number of events",50,4,9);  
  TH1D *hLength = new TH1D("hLength", "Track length; d[mm]; Number of events", 200,0,200);
  TH1D *hLengthCut0 = (TH1D*)hLength->Clone("hLengthCut0");
  TH1D *hLengthCut1 = (TH1D*)hLength->Clone("hLengthCut1");
  TH1D *hAlphaRange = (TH1D*)hLength->Clone("hAlphaRange");
  TH1D *hCarbonRange = (TH1D*)hLength->Clone("hCarbonRange");
  hLengthCut0->SetStats(kFALSE);
  hLengthCut1->SetStats(kFALSE);
  hAlphaRange->SetStats(kFALSE);
  hCarbonRange->SetStats(kFALSE);
    
  TH3D *hPosXYZ = new TH3D("hPosXYZ", "Vertex position; x [mm]; y[mm]; z[mm]",
				 50, -200, 200,
				 400, -200, 200,
				 50, -200, 200);
  
  TH3D *hPosXYZCut1 = (TH3D*)hPosXYZ->Clone("hPosXYZCut1");
  hPosXYZCut1->SetStats(kFALSE);

  TH1D *hChi2 = new TH1D("hChi2","Track loss function;loss function;Number of events",50,-5,20);
  TH1D *hChi2Cut0 = (TH1D*)hChi2->Clone("hChi2Cut0");
  TH1D *hChi2Cut1 = (TH1D*)hChi2->Clone("hChi2Cut1");
  hChi2Cut0->SetStats(kFALSE);
  hChi2Cut1->SetStats(kFALSE);

  TH1D *hPhi = new TH1D("hPhi","Track azimuthal angle wrt. beam line;#varphi;Number of events",50,-M_PI,M_PI);

  //TCut qualityCut = "chi2<10 && charge>100 && length>0";
  //TCut cut0 = "cosTheta>0.9 && abs(x0-127)<25 && abs(y0-76)<25"&&qualityCut;
  //TCut cut1 = "!(abs(x0-127)<25 && abs(y0-76)<25)"&&qualityCut;

  std::string qualityCut = "chi2<10 && charge>1000 && length>50 && eventType==3";
  std::string fidutialCut = "abs(xAlphaEnd)<100 && abs(yAlphaEnd)<100 && abs(xCarbonEnd)<100 && abs(yCarbonEnd)<100"; 
  std::string cut0 = qualityCut;
  std::string cut1 = "abs(yVtx)<10 && zVtx<-50"+std::string("&&")+qualityCut;
  cut1 += std::string("&&")+fidutialCut;
  std::string cut2 = "length>110"+std::string("&&")+cut1;

  trackTree->Draw("zVtx:yVtx:xVtx>>hPosXYZCut1",cut0.c_str(), "goff");
  
  trackTree->Draw("charge:length>>hChargeVsLength",qualityCut.c_str(), "goff");
  trackTree->Draw("length>>hLengthCut0",cut0.c_str(), "goff");
  trackTree->Draw("length>>hLengthCut1",cut1.c_str(), "goff");
  trackTree->Draw("alphaRange>>hAlphaRange",cut1.c_str(), "goff");
  trackTree->Draw("carbonRange>>hCarbonRange",cut1.c_str(), "goff");
  trackTree->Draw("chi2>>hChi2Cut0",cut1.c_str(), "goff");
  trackTree->Draw("chi2>>hChi2Cut1",cut2.c_str(), "goff");
  trackTree->Draw("phi>>hPhiCut1",cut1.c_str(), "goff");
  ///////////////////////////////////////////////////
  TLegend *aLeg = new TLegend(0.1, 0.1, 0.5, 0.3);
  TCanvas *aCanvas = new TCanvas("aCanvas","",700,700);
  aCanvas->Divide(2,2);

  ////////////////////////////////
  aCanvas->cd(1);
  TH1 *hProj = hPosXYZCut1->Project3D("yx");
  hProj->GetYaxis()->SetRangeUser(-20,20);
  hProj->DrawCopy("colz");

  ////////////////////////////////
  aCanvas->cd(2);
  
  hProj = hPosXYZCut1->Project3D("y");
  hProj->GetXaxis()->SetRangeUser(-20,20);
  hProj->DrawCopy("");
  
  ////////////////////////////////
  aCanvas->cd(3);
  hProj = hPosXYZCut1->Project3D("x");
  hProj->GetXaxis()->SetRangeUser(-200,200);
  hProj->DrawCopy("");

  ////////////////////////////////
  aCanvas->cd(4);
  hProj = hPosXYZCut1->Project3D("z");
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
  hLengthCut0->SetLineColor(1);
  hLengthCut0->SetLineStyle(2);
  hLengthCut0->SetLineWidth(3);
  hLengthCut0->SetMaximum(1.5*hLengthCut0->GetMaximum());
  hLengthCut0->Draw("");

  hLengthCut1->SetLineColor(2);
  hLengthCut1->SetLineStyle(2);
  hLengthCut1->SetLineWidth(3);
  hLengthCut1->SetMaximum(1.5*hLengthCut1->GetMaximum());
  hLengthCut1->Draw("same");

  aLeg = new TLegend(0.45, 0.8, 0.9, 0.9);
  aLeg->AddEntry(hLengthCut0, "NO cut on vtx.","l");
  aLeg->AddEntry(hLengthCut1, "|y|<10 && z<-60 && end_{X,Y}<100","l");
  aLeg->Draw();
  ////////////////////////////////


  aCanvas->cd(2);
  hAlphaRange->SetTitle("#alpha track length");
  hAlphaRange->SetLineColor(1);
  hAlphaRange->SetLineStyle(2);
  hAlphaRange->SetLineWidth(3);
  hAlphaRange->SetMaximum(1.5*hAlphaRange->GetMaximum());
  hAlphaRange->Draw("");
  ////////////////////////////////
  
  aCanvas->cd(3);
  hCarbonRange->SetTitle("C track length");
  hCarbonRange->SetLineColor(1);
  hCarbonRange->SetLineStyle(2);
  hCarbonRange->SetLineWidth(3);
  hCarbonRange->SetMaximum(1.5*hCarbonRange->GetMaximum());
  hCarbonRange->GetXaxis()->SetRangeUser(0,20);
  hCarbonRange->Draw("");
  ////////////////////////////////
  aCanvas->cd(4);

  hPhi->Draw();
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
  aCanvas->Print("Plots_set1.png");
  ////////////////
  ////////////////
}


