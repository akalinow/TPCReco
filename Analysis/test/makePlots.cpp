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
  TH1D *hLength = new TH1D("hLength", "Track length; d[mm]; Number of events", 400,0,200);
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
  TH3D *hAlphaEndXYZCut1 = (TH3D*)hVtxXYZ->Clone("hAlphaEndXYZCut1");
  hVtxXYZCut1->SetStats(kFALSE);
  hVtxYCorrCut1->SetStats(kFALSE);
  hAlphaEndXYZCut1->SetStats(kFALSE);

  TH1D *hChi2 = new TH1D("hChi2","Track loss function;loss function;Number of events",60,-10,20);
  TH1D *hChi2Cut0 = (TH1D*)hChi2->Clone("hChi2Cut0");
  TH1D *hChi2Cut1 = (TH1D*)hChi2->Clone("hChi2Cut1");
  hChi2Cut0->SetStats(kFALSE);
  hChi2Cut1->SetStats(kFALSE);

  TH2D *hPhiVsZ = new TH2D("hPhiVsZ",";#varphi_{BEAM};zVtx [mm]",20,-M_PI,M_PI, 60, -60, 60);
  
  TH1D *hPhi = new TH1D("hPhi","Track azimuthal angle wrt. beam line;#varphi_{BEAM};Number of events",20,-M_PI,M_PI);
  TH1D *hPhiRaw = (TH1D*)hPhi->Clone("hPhiRaw");
  TH1D *hPhiCorr = (TH1D*)hPhi->Clone("hPhiCorr");
  
  TH1D *hCosTheta = new TH1D("hCosTheta","Track polar angle wrt. beam line;cos(#theta_{BEAM});Number of events",20,-1,1);
  TH1D *hCosThetaCut1 = (TH1D*)hCosTheta->Clone("hCosThetaCut1");
  hCosThetaCut1->SetStats(kFALSE);

  
  std::string qualityCut = "chi2<10 && charge>1000 && length>50 && eventType==3 && hypothesisChi2<5";
  std::string fiducialXYCut = "abs(xAlphaEnd)<160 && abs(yAlphaEnd)<80 && abs(xCarbonEnd)<160 && abs(yCarbonEnd)<80";
  std::string fiducialZCut = "abs(zCarbonEnd - zAlphaEnd)<180";
  std::string vertexCut = "abs(yVtx+2)<6 && alphaRange>85";
  std::string cut0 = qualityCut;
  std::string cut1 = "1";
  cut1 += std::string("&&")+qualityCut;
  cut1 += std::string("&&")+vertexCut;
  cut1 += std::string("&&")+fiducialXYCut;
  cut1 += std::string("&&")+fiducialZCut;

  std::string cut2 = cut1;
  std::string cosThetaCut = "abs(cosTheta)<10.2";
  cut2 += std::string("&&")+cosThetaCut;
  
  trackTree->Draw("zVtx:yVtx:xVtx>>hVtxXYZCut1",cut0.c_str(), "goff");
  
  trackTree->Draw("charge:length>>hChargeVsLength",qualityCut.c_str(), "goff");
  trackTree->Draw("length>>hLengthCut0",cut0.c_str(), "goff");
  trackTree->Draw("length>>hLengthCut1",cut1.c_str(), "goff");
  trackTree->Draw("alphaRange>>hAlphaRange",cut1.c_str(), "goff");
  trackTree->Draw("carbonRange>>hCarbonRange",cut1.c_str(), "goff");
  trackTree->Draw("alphaEnergy+carbonEnergy>>hEnergy",cut1.c_str(), "goff");
  trackTree->Draw("phi>>hPhiRaw",cut1.c_str(), "goff");
  trackTree->Draw("cosTheta>>hCosThetaCut1",cut1.c_str(), "goff");  
  trackTree->Draw("yAlphaEnd-yVtx:zAlphaEnd-zVtx:xAlphaEnd-xVtx>>hAlphaEndXYZCut1",cut2.c_str(), "goff");
  
  trackTree->Draw("hypothesisChi2>>hChi2Cut0",cut0.c_str(), "goff");
  trackTree->Draw("hypothesisChi2>>hChi2Cut1",cut1.c_str(), "goff");
  //trackTree->Scan("eventId:hypothesisChi2",cut1.c_str());
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

  TH1 *hProfile = ((TH2F*)hProj)->ProfileX("hProfile");
  hProfile->SetTitle("Vertex Y position vs X");
  hProfile->SetYTitle("y [mm]");
  hProfile->GetYaxis()->SetRangeUser(-3,-1);
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
  hLengthCut0->SetTitle("#alpha + C tracks length");
  hLengthCut0->SetLineColor(1);
  hLengthCut0->SetLineStyle(2);
  hLengthCut0->SetLineWidth(3);
  hLengthCut0->SetMaximum(100*hLengthCut0->GetMaximum());
  hLengthCut0->GetXaxis()->SetRangeUser(50,200);
  hLengthCut0->Draw("");

  hLengthCut1->SetLineColor(4);
  hLengthCut1->SetLineStyle(2);
  hLengthCut1->SetLineWidth(3);
  //hLengthCut1->Fit("gaus","RS","",90,120);
  //hLengthCut1->Fit("gaus","RS","",60,100);
  //hLengthCut1->Fit("gaus","RS","",90,100);
  hLengthCut1->SetStats(kTRUE);
  hLengthCut1->SetMaximum(100*hLengthCut1->GetMaximum());
  hLengthCut1->Draw("same");
  hLengthCut0->Draw("same");
  hLengthCut1->Draw();
  gPad->SetLogy();

  aLeg = new TLegend(0.1, 0.65, 0.53, 0.9);
  aLeg->AddEntry(hLengthCut0, "NO cut on vtx.","l");
  aLeg->AddEntry(hLengthCut1, "#splitline{|y-2|<6}{#splitline{|end_{X}|<160 &&  |end_{Y}|<80}{|C_{Z} - #alpha_{Z}|<180}}","l");
  aLeg->Draw();
  ////////////////////////////////
  
  aCanvas->cd(2);
  hAlphaRange->SetTitle("#alpha track length");
  hAlphaRange->SetLineColor(4);
  hAlphaRange->SetLineWidth(3);
  //hAlphaRange->Fit("gaus","RS","",80,110);
  hAlphaRange->Fit("gaus","RS","",80,100); 
  hAlphaRange->SetStats(kTRUE);
  hAlphaRange->SetMaximum(1.5*hAlphaRange->GetMaximum());
  hAlphaRange->GetXaxis()->SetRangeUser(50,110);
  hAlphaRange->Draw("");
  ////////////////////////////////
  
  aCanvas->cd(3);
  hCarbonRange->SetTitle("C track length");
  hCarbonRange->SetLineColor(4);
  hCarbonRange->SetLineWidth(3);
  hCarbonRange->SetMaximum(1.5*hCarbonRange->GetMaximum());
  hCarbonRange->GetXaxis()->SetRangeUser(0,25);
  hCarbonRange->Draw("");
  ////////////////////////////////
  aCanvas->cd(4);

  hEnergy->SetLineWidth(2);
  hEnergy->SetLineColor(4);
  hEnergy->Fit("gaus");
  hEnergy->Draw("");

  ////////////////
  aCanvas->Print("Plots_set1.png");
  ////////////////
  return;
  ////////////////
  aCanvas->Clear();
  aCanvas->Divide(2,2);

  ////////////////////////////////
  aCanvas->cd(1);


  hPhiRaw->SetLineColor(1);
  hPhiRaw->SetLineStyle(2);
  hPhiRaw->SetMaximum(1.5*hPhiRaw->GetMaximum());
  hPhiRaw->SetMinimum(0.0);
  hPhiRaw->Draw();
  
  hPhiCorr->SetLineColor(4);
  hPhiCorr->SetLineWidth(2);
  hPhiCorr->Draw("same");
  aLeg = new TLegend(0.43, 0.65, 0.9, 0.9);
  aLeg->AddEntry(hPhiRaw, "#varphi from det. coordinates","l");
  aLeg->AddEntry(hPhiCorr, "#splitline{#varphi corrected for}{the beam direction}","l");
  hPhiRaw->Print();
  for(int iBin=1;iBin<hPhiRaw->GetNbinsX();++iBin){
    std::cout<<hPhiRaw->GetBinCenter(iBin)<<"\t"<<hPhiRaw->GetBinContent(iBin)<<std::endl;
  }
  //aLeg->Draw();
  ////////////////////////////////
  aCanvas->cd(2);

  hCosThetaCut1->SetLineWidth(2);
  hCosThetaCut1->SetLineColor(4);
  hCosThetaCut1->Draw();
  
  ////////////////////////////////
  aCanvas->cd(3);

  hProj = hAlphaEndXYZCut1->Project3D("yz");
  hProj->GetXaxis()->SetRangeUser(-100,100);
  hProj->GetYaxis()->SetRangeUser(-100,100);
  hProj->DrawCopy("colz");


  //return;
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


