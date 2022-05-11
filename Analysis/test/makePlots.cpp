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

  TH1D *hEnergy = new TH1D("hEnergy", "#alpha + ^{12}C energy;E(#alpha + ^{12}C) [MeV]; Number of events",60,3,9);
  TH1D *hEnergyCut2 = (TH1D*)hEnergy->Clone("hEnergyCut2");
  TH1D *hEnergyCut3 = (TH1D*)hEnergy->Clone("hEnergyCut3");

  TH2D *hEnergyVsCosTheta = new TH2D("hEnergyVsCosTheta",				     
				     ";cos(#theta_{BEAM}); #alpha energy [MeV];",
				     30,-1,1,
				     100, 2, 5);
  TH2D* hEnergyVsCosThetaCut2 = (TH2D*)hEnergyVsCosTheta->Clone("hEnergyVsCosThetaCut2");
  TH2D* hEnergyVsCosThetaCut3 = (TH2D*)hEnergyVsCosTheta->Clone("hEnergyVsCosThetaCut3");

  TH2D *hLength2D = new TH2D("hLength2D", "Track length; d[mm]; d [mm]", 400,0,200, 400,0,200);
  TH2D *hLength2DCut1 = (TH2D*)hLength2D->Clone("hLength2DCut1");
  
  TH1D *hLength = new TH1D("hLength", "Track length; d[mm]; Number of events", 400,0,200);
  hLength->SetStats(kFALSE);
  TH1D *hLengthCut0 = (TH1D*)hLength->Clone("hLengthCut0");
  TH1D *hLengthCut1 = (TH1D*)hLength->Clone("hLengthCut1");
  
  TH1D *hAlphaRangeCut1 = (TH1D*)hLength->Clone("hAlphaRangeCut1");
  TH1D *hAlphaRangeCut2 = (TH1D*)hLength->Clone("hAlphaRangeCut2");
  TH1D *hAlphaRangeCut3 = (TH1D*)hLength->Clone("hAlphaRangeCut3");
  TH1D *hAlphaRangeCut4 = (TH1D*)hLength->Clone("hAlphaRangeCut4");
  TH1D *hAlphaRangeCut5 = (TH1D*)hLength->Clone("hAlphaRangeCut5");
  
  TH1D *hCarbonRangeCut1 = (TH1D*)hLength->Clone("hCarbonRangeCut1");

  TH3D *hVtxXYZ = new TH3D("hVtxXYZ", "Vertex position; x [mm]; y[mm]; z[mm]",
				 50, -200, 200,
				 400, -200, 200,
				 50, -200, 200);
  hVtxXYZ->SetStats(kFALSE);
  
  TH3D *hVtxXYZCut1 = (TH3D*)hVtxXYZ->Clone("hVtxXYZCut1");
  TH3D *hVtxYCorrCut1 = (TH3D*)hVtxXYZ->Clone("hVtxYCorrCut1");
  TH3D *hAlphaEndXYZCut1 = (TH3D*)hVtxXYZ->Clone("hAlphaEndXYZCut1");

  TH1D *hChi2 = new TH1D("hChi2","Track loss function;loss function;Number of events",60,-10,20);
  hChi2->SetStats(kFALSE);
  TH1D *hChi2Cut0 = (TH1D*)hChi2->Clone("hChi2Cut0");
  TH1D *hChi2Cut1 = (TH1D*)hChi2->Clone("hChi2Cut1");

  TH2D *hPhiVsZ = new TH2D("hPhiVsZ",";#varphi_{BEAM};zVtx [mm]",20,-M_PI,M_PI, 60, -60, 60);
  
  TH1D *hPhi = new TH1D("hPhi","Track azimuthal angle wrt. beam line;#varphi_{BEAM};Number of events",20,-M_PI,M_PI);
  TH1D *hPhiRaw = (TH1D*)hPhi->Clone("hPhiRaw");
  TH1D *hPhiCorr = (TH1D*)hPhi->Clone("hPhiCorr");
  
  TH1D *hCosTheta = new TH1D("hCosTheta","Track polar angle wrt. beam line;cos(#theta_{BEAM});Number of events",20,-1,1);
  hCosTheta->SetStats(kFALSE);
  TH1D *hCosThetaCut1 = (TH1D*)hCosTheta->Clone("hCosThetaCut1");
  

  std::string qualityCut = "chi2<10 && charge>1000 && length>50 && eventType==3 && hypothesisChi2<5";
  std::string fiducialXYCut = "abs(xAlphaEnd)<160 && abs(yAlphaEnd)<800 && abs(xCarbonEnd)<160 && abs(yCarbonEnd)<800";
  std::string fiducialZCut = "abs(zCarbonEnd - zAlphaEnd)<180";
  std::string vertexCut = "abs(yVtx+2)<6";
  
  std::string cut0 = qualityCut;

  std::string cut1 = "1";
  cut1 += std::string("&&")+qualityCut;
  cut1 += std::string("&&")+vertexCut;
  cut1 += std::string("&&")+fiducialXYCut;
  cut1 += std::string("&&")+fiducialZCut;

  std::string cut2 = cut1;
  std::string lengthCut = "length<97";
  cut2 += std::string("&&")+lengthCut;

  std::string cut3 = cut1;
  lengthCut = "length>97";
  cut3 += std::string("&&")+lengthCut;

  std::string cut4 = cut1;
  lengthCut = "carbonRange<18";
  cut4 += std::string("&&")+lengthCut;

  std::string cut5 = cut1;
  lengthCut = "carbonRange>18";
  cut5 += std::string("&&")+lengthCut;
  
  trackTree->Draw("zVtx:yVtx:xVtx>>hVtxXYZCut1",cut0.c_str(), "goff");
  
  trackTree->Draw("charge:length>>hChargeVsLength",qualityCut.c_str(), "goff");
  trackTree->Draw("length>>hLengthCut0",cut0.c_str(), "goff");
  trackTree->Draw("length>>hLengthCut1",cut1.c_str(), "goff");
  trackTree->Draw("alphaRange>>hAlphaRangeCut1",cut1.c_str(), "goff");
  trackTree->Draw("alphaRange>>hAlphaRangeCut2",cut2.c_str(), "goff");
  trackTree->Draw("alphaRange>>hAlphaRangeCut3",cut3.c_str(), "goff");
  trackTree->Draw("alphaRange>>hAlphaRangeCut4",cut4.c_str(), "goff");
  trackTree->Draw("alphaRange>>hAlphaRangeCut5",cut5.c_str(), "goff");
  
  trackTree->Draw("carbonRange>>hCarbonRangeCut1",cut1.c_str(), "goff");

  trackTree->Draw("carbonRange:alphaRange>>hLength2DCut1",cut1.c_str(), "goff");

  trackTree->Draw("alphaEnergy+carbonEnergy>>hEnergyCut2",cut2.c_str(), "goff");
  trackTree->Draw("alphaEnergy+carbonEnergy>>hEnergyCut3",cut3.c_str(), "goff");
  trackTree->Draw("alphaEnergy:cosTheta>>hEnergyVsCosThetaCut2",cut2.c_str(), "goff");
  trackTree->Draw("alphaEnergy:cosTheta>>hEnergyVsCosThetaCut3",cut3.c_str(), "goff");
  
  trackTree->Draw("phi>>hPhiRaw",cut1.c_str(), "goff");
  trackTree->Draw("cosTheta>>hCosThetaCut1",cut1.c_str(), "goff");  
  trackTree->Draw("yAlphaEnd-yVtx:zAlphaEnd-zVtx:xAlphaEnd-xVtx>>hAlphaEndXYZCut1",cut1.c_str(), "goff");
  
  trackTree->Draw("hypothesisChi2>>hChi2Cut0",cut0.c_str(), "goff");
  trackTree->Draw("hypothesisChi2>>hChi2Cut1",cut1.c_str(), "goff");
  //trackTree->Scan("eventId:carbonRange",cut3.c_str());
  ///////////////////////////////////////////////////
  TLegend *aLeg = new TLegend(0.1, 0.1, 0.5, 0.3);
  TCanvas *aCanvas = new TCanvas("aCanvas","",700,700);
  aCanvas->Divide(2,2);


  ////////////////////////////////
  aCanvas->cd(1);
  gPad->SetLeftMargin(0.12);
  gPad->SetRightMargin(0.12);
  TH1 *hProj = hVtxXYZCut1->Project3D("yx");
  hProj->GetYaxis()->SetRangeUser(-20,20);
  hProj->DrawCopy("colz");

  ////////////////////////////////
  aCanvas->cd(2);
  gPad->SetLeftMargin(0.12);
  gPad->SetRightMargin(0.12);

  TH1 *hProfile = ((TH2F*)hProj)->ProfileX("hProfile");
  hProfile->SetTitle("Vertex Y position vs X");
  hProfile->SetYTitle("y [mm]");
  hProfile->GetYaxis()->SetRangeUser(-3,1);
  hProfile->Draw();
  TFitResultPtr aFitPtr = hProfile->Fit("pol1","RS","",-150,150);
  double vtxCorr_p0 = aFitPtr->GetParams()[0];
  double vtxCorr_p1 = aFitPtr->GetParams()[1];
  std::stringstream aStringStream;
  aStringStream<<"("<<vtxCorr_p0<<"+"<<vtxCorr_p1<<"*xVtx)";
  double beamAngle = atan(vtxCorr_p1);
  std::string yCorrString = aStringStream.str();
  std::string columns = "0:yVtx-"+yCorrString+":0>>hVtxYCorrCut1";  
  trackTree->Draw(columns.c_str(),cut0.c_str(), "goff");
  columns = "atan2(zAlphaEnd-zVtx,-(xAlphaEnd-xVtx)*sin(0.00315905) + (yAlphaEnd-yVtx)*cos(0.00315905))";
  columns +=">>hPhiCorr";
  trackTree->Draw(columns.c_str(),cut1.c_str(), "goff");
  ////////////////////////////////
  aCanvas->cd(3);
  gPad->SetLeftMargin(0.12);
  gPad->SetRightMargin(0.12);
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
  aCanvas->cd(4);
  gPad->SetLeftMargin(0.12);
  gPad->SetRightMargin(0.12);
  hProj = hVtxXYZCut1->Project3D("x");
  hProj->GetXaxis()->SetRangeUser(-200,200);
  hProj->DrawCopy("");
  ////////////////////////////////
 
  ////////////////  
  aCanvas->Print("Plots_set0.png");
  ////////////////
  //return;
  ////////////////
  aCanvas->Clear();
  aCanvas->Divide(2,2);

  ////////////////////////////////
  aCanvas->cd(1);
  gPad->SetLeftMargin(0.12);
  gPad->SetRightMargin(0.12);
  
  hLengthCut0->SetTitle("#alpha + ^{12}C track length");
  hLengthCut0->SetLineColor(1);
  hLengthCut0->SetLineStyle(2);
  hLengthCut0->SetLineWidth(3);
  hLengthCut0->SetMaximum(100*hLengthCut0->GetMaximum());
  hLengthCut0->GetXaxis()->SetRangeUser(50,200);
  hLengthCut0->Draw("");

  hLengthCut1->SetTitle("#alpha + ^{12}C track length");
  hLengthCut1->SetLineColor(4);
  hLengthCut1->SetLineStyle(2);
  hLengthCut1->SetLineWidth(3); 
  hLengthCut1->SetStats(kTRUE);
  hLengthCut1->SetMaximum(100*hLengthCut1->GetMaximum());
  hLengthCut1->Draw("same");
  hLengthCut0->Draw("same");
  hLengthCut1->Draw();
  gPad->SetLogy();

  aLeg = new TLegend(0.1, 0.63, 0.6, 0.9);
  aLeg->AddEntry(hLengthCut1, "#splitline{|yVtx-yBeam|<6}{#splitline{|tk. end_{X}|<160}{|C_{Z} - #alpha_{Z}|<180}}","l");
  aLeg->Draw();
  ////////////////////////////////  
  aCanvas->cd(2);
  gPad->SetLeftMargin(0.12);
  gPad->SetRightMargin(0.12);
  
  hAlphaRangeCut1->SetTitle("#alpha track length");
  hAlphaRangeCut1->SetLineColor(4);
  hAlphaRangeCut1->SetLineWidth(3);
  hAlphaRangeCut1->SetStats(kTRUE);
  hAlphaRangeCut1->SetMaximum(1.5*hAlphaRangeCut1->GetMaximum());
  hAlphaRangeCut1->GetXaxis()->SetRangeUser(50,110);
  hAlphaRangeCut1->Draw("");
  ////////////////////////////////
  
  aCanvas->cd(3);
  gPad->SetLeftMargin(0.12);
  gPad->SetRightMargin(0.12);
  
  hCarbonRangeCut1->SetTitle("^{12}C track length");
  hCarbonRangeCut1->SetLineColor(4);
  hCarbonRangeCut1->SetLineWidth(3);
  hCarbonRangeCut1->SetMaximum(1.5*hCarbonRangeCut1->GetMaximum());
  hCarbonRangeCut1->GetXaxis()->SetRangeUser(0,25);
  hCarbonRangeCut1->Draw("");
  ////////////////////////////////
  aCanvas->cd(4);
  gPad->SetLeftMargin(0.12);
  gPad->SetRightMargin(0.12);

  hEnergyCut2->SetLineWidth(2);
  hEnergyCut2->SetLineColor(4);
  hEnergyCut2->Scale(1.0/hEnergyCut2->Integral());
  hEnergyCut2->SetMaximum(0.4);
  hEnergyCut2->Fit("gaus","");
  hEnergyCut2->Draw("");
  gPad->Update();

  hEnergyCut3->SetLineWidth(2);
  hEnergyCut3->SetLineColor(2);
  hEnergyCut3->Scale(1.0/hEnergyCut3->Integral());
  hEnergyCut3->SetMaximum(0.25);
  hEnergyCut3->Fit("gaus","");
  hEnergyCut3->Draw("");
  gPad->Update();
  TPaveStats *st2 = (TPaveStats*)hEnergyCut3->FindObject("stats");
  st2->SetY1NDC(0.55);
  st2->SetY2NDC(0.7);
  
  hEnergyCut2->Draw("same");

  aLeg = new TLegend(0.1, 0.7, 0.6, 0.9);
  aLeg->AddEntry(hEnergyCut2, "#alpha+C length<100 mm","l");
  aLeg->AddEntry(hEnergyCut3, "#alpha+C length>100 mm","l");
  aLeg->Draw();
  ////////////////
  aCanvas->Print("Plots_set1.png");
  ////////////////
  //return;
  ////////////////
  aCanvas->Clear();
  aCanvas->Divide(2,2);

  ////////////////////////////////
  aCanvas->cd(1);
  gPad->SetLeftMargin(0.12);
  gPad->SetRightMargin(0.12);

  hPhiRaw->SetLineColor(4);
  hPhiRaw->SetLineStyle(2);
  hPhiRaw->SetLineWidth(2);
  hPhiRaw->SetMaximum(1.5*hPhiRaw->GetMaximum());
  hPhiRaw->SetMinimum(0.0);
  hPhiRaw->Draw();
  ////////////////////////////////
  aCanvas->cd(2);
  gPad->SetLeftMargin(0.12);
  gPad->SetRightMargin(0.12);

  hCosThetaCut1->SetLineWidth(2);
  hCosThetaCut1->SetLineColor(4);
  hCosThetaCut1->Draw();
  
  ////////////////////////////////
  aCanvas->cd(3);
  gPad->SetLeftMargin(0.12);
  gPad->SetRightMargin(0.12);
    

  hProj = hAlphaEndXYZCut1->Project3D("zy");
  hProj->SetTitle("#alpha track endpoint wrt. vertex");
  hProj->GetXaxis()->SetRangeUser(-100,100);
  hProj->GetYaxis()->SetRangeUser(-100,100);
  hProj->SetXTitle("Y_{DETECTOR}");
  hProj->SetYTitle("Z_{DETECTOR}");
  hProj->DrawCopy("colz");

  ////////////////////////////////
  aCanvas->cd(3);
  gPad->SetLeftMargin(0.12);
  gPad->SetRightMargin(0.12);

  ////////////////
  aCanvas->Print("Plots_set2.png");
  ////////////////
  //return;
  ////////////////
  aCanvas->Clear();
  aCanvas->Divide(2,2);

  ////////////////////////////////
  aCanvas->cd(1);
  gPad->SetLeftMargin(0.12);
  gPad->SetRightMargin(0.12);

  hEnergyVsCosThetaCut2->GetYaxis()->SetRangeUser(2.5,3.5);
  hEnergyVsCosThetaCut3->SetTitle("#alpha+C length<100 mm");
  hEnergyVsCosThetaCut2->Draw("colz");

  ////////////////////////////////
  aCanvas->cd(2);
  gPad->SetLeftMargin(0.12);
  gPad->SetRightMargin(0.12);

  hEnergyVsCosThetaCut3->GetYaxis()->SetRangeUser(3.5,4.5);
  hEnergyVsCosThetaCut3->SetTitle("#alpha+C length>100 mm");
  hEnergyVsCosThetaCut3->Draw("colz");

  ////////////////////////////////
  aCanvas->cd(3);
  gPad->SetLeftMargin(0.12);
  gPad->SetRightMargin(0.12);

  hProfile = hEnergyVsCosThetaCut2->ProfileX();
  hProfile->SetYTitle("#alpha energy [MeV]");
  hProfile->Fit("pol1");
  hProfile->Draw();

  ////////////////////////////////
  aCanvas->cd(4);
  gPad->SetLeftMargin(0.12);
  gPad->SetRightMargin(0.12);

  hProfile = hEnergyVsCosThetaCut3->ProfileX();
  hProfile->SetYTitle("#alpha energy [MeV]");
  hProfile->Fit("pol1");
  hProfile->Draw();
  
  aCanvas->Print("Plots_set3.png");
  ////////////////
  ////////////////
  aCanvas->Clear();
  aCanvas->Divide(2,2);

  ////////////////////////////////
  aCanvas->cd(1);
  gPad->SetLeftMargin(0.12);
  gPad->SetRightMargin(0.12);

  ////////////////////////////////  
  aCanvas->cd(1);
  gPad->SetLeftMargin(0.12);
  gPad->SetRightMargin(0.12);
  
  hAlphaRangeCut4->SetTitle("#alpha track length");
  hAlphaRangeCut4->SetLineColor(4);
  hAlphaRangeCut4->SetLineWidth(3);
  hAlphaRangeCut4->SetStats(kTRUE);
  hAlphaRangeCut4->SetMaximum(1.5*hAlphaRangeCut4->GetMaximum());
  hAlphaRangeCut4->GetXaxis()->SetRangeUser(50,80);
  hAlphaRangeCut4->DrawNormalized("");

  hAlphaRangeCut5->SetLineColor(8);
  hAlphaRangeCut5->SetLineWidth(3);
  hAlphaRangeCut5->SetLineStyle(3);
  hAlphaRangeCut5->DrawNormalized("same");

  aLeg = new TLegend(0.5, 0.75, 0.85, 0.9);
  aLeg->AddEntry(hAlphaRangeCut4, "^{12}C range<18 mm","l");
  aLeg->AddEntry(hAlphaRangeCut5, "^{12}C range>18 mm","l");
  aLeg->Draw();
  ////////////////////////////////  
  aCanvas->cd(2);
  gPad->SetLeftMargin(0.12);
  gPad->SetRightMargin(0.12);

  hLength2DCut1->GetXaxis()->SetRangeUser(50,110);
  hLength2DCut1->GetYaxis()->SetRangeUser(5,20);
  hLength2DCut1->SetXTitle("#alpha range [mm]");
  hLength2DCut1->SetYTitle("^{12}C range [mm]");
  hLength2DCut1->Draw("colz");


  aCanvas->Print("Plots_set4.png");
}


