/*
Usage:

[user1@d21ee95b36ff resources]$ root
root [0] .L makeMCPlots.cpp
root [1] makeMCPlots("TrackTree_MCTest.root")

*/
void makeMCPlots(std::string fileName){

  gStyle->SetOptFit(11);
  gStyle->SetOptStat(0);
  gStyle->SetPaintTextFormat("4.2f");

  TFile *aFile = new TFile(fileName.c_str());
  TTree *trackTree = (TTree*)aFile->Get("trackTree");

  TCut fiducialCut = "abs(vtxGenX+alphaRangeGen*sqrt(1-cosThetaGen*cosThetaGen)*cos(phiGen))<140";
      fiducialCut += "abs(vtxGenY+alphaRangeGen*sqrt(1-cosThetaGen*cosThetaGen)*sin(phiGen))<90";
      fiducialCut += "abs(vtxGenZ+alphaRangeGen*cosThetaGen)<120/2";
      //fiducialCut += "abs(phiGen)<3.0";

  TCut goodReco = "eventTypeReco==3 && abs(alphaRangeReco-alphaRangeGen)/alphaRangeGen<0.05";

  trackTree->Draw("(0.99*alphaRangeReco-alphaRangeGen)/alphaRangeGen:alphaRangeGen>>hRangeResVsRangeGen(21,40, 120,  81,-0.5,0.5)",fiducialCut,"goff");
  trackTree->Draw("(0.99*alphaRangeReco-alphaRangeGen)/alphaRangeGen:cosThetaGen>>hRangeResVsCosTheta(21,-1, 1,  81,-0.5,0.5)",fiducialCut,"goff");
  trackTree->Draw("(cosThetaReco-cosThetaGen):cosThetaGen>>hCosThetaResVsCosTheta(21,-1, 1,  161,-1,1)",fiducialCut,"goff");
  trackTree->Draw("(phiReco-phiGen):cosThetaGen>>hPhiResVsCosTheta(21,-1, 1,  81,-0.5,0.5)",fiducialCut,"goff");
  trackTree->Draw("(0.8*chargeReco-chargeGen)/chargeGen:cosThetaGen>>hChargeResVsCosTheta(21,-1, 1,  81,-1,1)",fiducialCut,"goff");

  trackTree->Draw("vtxGenY:vtxGenX>>hGenVtxAll(21,-160, 160, 21,-12,12)",fiducialCut,"goff");
  trackTree->Draw("vtxGenY:vtxGenX>>hGenVtxReco(21,-160, 160, 21,-12,12)",fiducialCut&&goodReco,"goff");

  trackTree->Draw("phiGen:cosThetaGen>>hGenDirAll(21,-1, 1, 41,-3.16,3.16)",fiducialCut,"goff");
  trackTree->Draw("phiGen:cosThetaGen>>hGenDirReco(21,-1, 1, 41,-3.16,3.16)",fiducialCut&&goodReco,"goff");

  trackTree->Draw("(vtxRecoX-vtxGenX):vtxGenX>>hVtxXRes(21,-160,160, 41,-3,3)",fiducialCut,"goff");
  trackTree->Draw("(vtxRecoY-vtxGenY):vtxGenY>>hVtxYRes(21,-12,12, 41,-3,3)",fiducialCut,"goff");
  trackTree->Draw("(vtxRecoZ-vtxGenZ):vtxGenZ>>hVtxZRes(21,-12,12, 41,-3,3)",fiducialCut,"goff");

  trackTree->Draw("(0.99*alphaRangeReco-alphaRangeGen)/alphaRangeGen:lineFitLoss>>hLengthResVsLineLoss(21,0, 20, 41,-1.5,1.5)",fiducialCut,"goff");
  trackTree->Draw("(0.99*alphaRangeReco-alphaRangeGen)/alphaRangeGen:dEdxFitLoss/chargeReco*1E8>>hLengthResVsDedxLoss(21,0, 20, 41,-1.5,1.5)",fiducialCut,"goff");
  trackTree->Draw("(0.99*alphaRangeReco-alphaRangeGen)/alphaRangeGen:dEdxFitSigma>>hLengthResVsDedxSigma(21,0, 5, 41,-1.5,1.5)",fiducialCut,"goff");

  trackTree->Draw("(vtxRecoY + alphaRangeReco*sqrt(1-cosThetaReco*cosThetaReco)*cos(phiReco)):(vtxRecoX + alphaRangeReco*sqrt(1-cosThetaReco*cosThetaReco)*sin(phiReco))>>hRecoAlphaEndpoint(21,-160,160, 21,-160,160)",fiducialCut,"goff");
  
  trackTree->Draw("eventTypeReco>>hEventTypeReco(5,-0.5,4.5)",fiducialCut,"goff");
  ///////////////////////////////////////////////////
  TLegend *aLeg = new TLegend(0.1, 0.1, 0.5, 0.3);
  TCanvas *aCanvas = new TCanvas("aCanvas","",700,700);
  aCanvas->Divide(2,2);

  /////////////////////////////////////
  ////////////////////////////////////
  aCanvas->cd(1);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);
 
  TH2F *hRangeResVsCosTheta = (TH2F*)gDirectory->Get("hRangeResVsCosTheta");
  hRangeResVsCosTheta->SetTitle("");
  hRangeResVsCosTheta->SetXTitle("cos(#theta^{GEN})");
  hRangeResVsCosTheta->SetYTitle("(d^{RECO} - d^{GEN})/d^{GEN}");
  hRangeResVsCosTheta->SetZTitle("Number of events");
  hRangeResVsCosTheta->GetYaxis()->SetTitleOffset(1.6);
  hRangeResVsCosTheta->GetZaxis()->SetTitleOffset(1.5);
  hRangeResVsCosTheta->Draw("colz");
  ////////////////////////////////
  aCanvas->cd(2);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);

  TH2F *hRangeResVsRangeGen = (TH2F*)gDirectory->Get("hRangeResVsRangeGen");
  hRangeResVsRangeGen->SetTitle("");
  hRangeResVsRangeGen->SetXTitle("d^{GEN} [mm]");
  hRangeResVsRangeGen->SetYTitle("(d^{RECO} - d^{GEN})/d^{RECO}");
  hRangeResVsRangeGen->SetZTitle("Number of events");
  hRangeResVsRangeGen->GetYaxis()->SetTitleOffset(1.6);
  hRangeResVsRangeGen->GetZaxis()->SetTitleOffset(1.5);
  hRangeResVsRangeGen->Draw("colz");

  ////////////////////////////////
  aCanvas->cd(3);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);

  TH2F *hCosThetaResVsCosTheta = (TH2F*)gDirectory->Get("hCosThetaResVsCosTheta");
  hCosThetaResVsCosTheta->SetTitle("");
  hCosThetaResVsCosTheta->SetXTitle("cos(#theta^{GEN})");
  hCosThetaResVsCosTheta->SetYTitle("cos(#theta^{RECO}) - cos(#theta^{GEN})");
  hCosThetaResVsCosTheta->SetZTitle("Number of events");
  hCosThetaResVsCosTheta->GetYaxis()->SetTitleOffset(1.6);
  hCosThetaResVsCosTheta->GetZaxis()->SetTitleOffset(1.5);
  hCosThetaResVsCosTheta->Draw("colz");
  ////////////////////////////////
  aCanvas->cd(4);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);

  TH2F *hPhiResVsCosTheta = (TH2F*)gDirectory->Get("hPhiResVsCosTheta");
  hPhiResVsCosTheta->SetTitle("");
  hPhiResVsCosTheta->SetXTitle("cos(#theta^{GEN})");
  hPhiResVsCosTheta->SetYTitle("#varphi^{RECO} - #varphi^{GEN}");
  hPhiResVsCosTheta->SetZTitle("Number of events");
  hPhiResVsCosTheta->GetYaxis()->SetTitleOffset(1.6);
  hPhiResVsCosTheta->GetZaxis()->SetTitleOffset(1.5);
  hPhiResVsCosTheta->Draw("colz");
  ////////////////////////////////
  aCanvas->cd(4);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);

  ////////////////////////////////
  TH2F *hGenVtxAll = (TH2F*)gDirectory->Get("hGenVtxAll");
  TH2F *hGenVtxReco = (TH2F*)gDirectory->Get("hGenVtxReco");
  hGenVtxReco->Divide(hGenVtxAll);

  hGenVtxReco->SetTitle("");
  hGenVtxReco->SetXTitle("x^{GEN} [mm]");
  hGenVtxReco->SetYTitle("y^{GEN} [mm]");
  hGenVtxReco->SetZTitle("Efficiency");
  hGenVtxReco->GetYaxis()->SetTitleOffset(1.6);
  hGenVtxReco->GetZaxis()->SetTitleOffset(1.5);
  hGenVtxReco->SetMinimum(0.0);
  hGenVtxReco->SetMaximum(1.0);
  hGenVtxReco->Draw("colz");
  ////////////////////////////////
  TH2F *hGenDirAll = (TH2F*)gDirectory->Get("hGenDirAll");
  TH2F *hGenDirReco = (TH2F*)gDirectory->Get("hGenDirReco")->Clone();
  hGenDirReco->Divide(hGenDirAll);
  hGenDirReco->SetTitle("");
  hGenDirReco->SetXTitle("cos(#theta^{GEN})");
  hGenDirReco->SetYTitle("#varphi^{GEN} [rad]");
  hGenDirReco->SetZTitle("Efficiency");
  hGenDirReco->GetZaxis()->SetTitleOffset(1.5);
  hGenDirReco->SetMinimum(0.0);
  hGenDirReco->SetMaximum(1.0);
  hGenDirReco->Draw("colz");
  //hGenDirAll->Draw("same text");

  aCanvas->Print("MCPlots_set0.png");
  ////////////////////////////////
  //return;
  ////////////////////////////////
  ////////////////////////////////
  aCanvas->Clear();
  aCanvas->Divide(2,2);

  ////////////////////////////////
  aCanvas->cd(1);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);
  gPad->SetBottomMargin(0.15);
  gPad->SetGrid(1,1);

  ////////////////////////////////
  trackTree->Draw("vtxGenY:vtxGenX>>hGenVtxReco(21,-160, 160, 21,-12,12)",fiducialCut&&goodReco,"goff");

  TH1D *hGenVtxAll_1D = hGenVtxAll->ProjectionX();
  TH1D *hGenVtxReco_1D = ((TH2F*)gDirectory->Get("hGenVtxReco"))->ProjectionX();
  hGenVtxReco_1D->Divide(hGenVtxAll_1D);

  hGenVtxReco_1D->SetTitle("");
  hGenVtxReco_1D->SetXTitle("x^{GEN} [mm]");
  hGenVtxReco_1D->SetYTitle("Efficiency");
  hGenVtxReco_1D->GetYaxis()->SetTitleOffset(1.6);
  hGenVtxReco_1D->SetMinimum(0.0);
  hGenVtxReco_1D->SetMaximum(1.1);
  hGenVtxReco_1D->Draw("");
  ////////////////////////////////
  aCanvas->cd(2);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);
  gPad->SetBottomMargin(0.15);
  gPad->SetGrid(1,1);

  trackTree->Draw("vtxGenY:vtxGenX>>hGenVtxReco(21,-160, 160, 21,-12,12)",fiducialCut&&goodReco,"goff");
  hGenVtxAll_1D = hGenVtxAll->ProjectionY();
  hGenVtxReco_1D = ((TH2F*)gDirectory->Get("hGenVtxReco"))->ProjectionY();
  hGenVtxReco_1D->Divide(hGenVtxAll_1D);

  hGenVtxReco_1D->SetTitle("");
  hGenVtxReco_1D->SetXTitle("y^{GEN} [mm]");
  hGenVtxReco_1D->SetYTitle("Efficiency");
  hGenVtxReco_1D->GetYaxis()->SetTitleOffset(1.6);
  hGenVtxReco_1D->SetMinimum(0.0);
  hGenVtxReco_1D->SetMaximum(1.1);
  hGenVtxReco_1D->Draw("");
  ////////////////////////////////
  aCanvas->cd(3);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);
  gPad->SetBottomMargin(0.15);
  gPad->SetGrid(1,1);

  trackTree->Draw("phiGen:cosThetaGen>>hGenDirReco(21,-1, 1, 41,-3.16,3.16)",fiducialCut&&goodReco,"goff");
  TH1D *hGenDirAll_1D = hGenDirAll->ProjectionX();
  TH1D *hGenDirReco_1D = ((TH2F*)gDirectory->Get("hGenDirReco"))->ProjectionX();
  hGenDirReco_1D->Divide(hGenDirAll_1D);
  hGenDirReco_1D->SetTitle("");
  hGenDirReco_1D->SetXTitle("cos(#theta^{GEN})");
  hGenDirReco_1D->SetYTitle("Efficiency");
  hGenDirReco_1D->GetYaxis()->SetTitleOffset(1.6);
  hGenDirReco_1D->SetMinimum(0.0);
  hGenDirReco_1D->SetMaximum(1.1);
  hGenDirReco_1D->Draw("");

  ////////////////////////////////
  aCanvas->cd(4);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);
  gPad->SetBottomMargin(0.15);
  gPad->SetGrid(1,1);

  trackTree->Draw("phiGen:cosThetaGen>>hGenDirReco(21,-1, 1, 41,-3.16,3.16)",fiducialCut&&goodReco,"goff");
  hGenDirAll_1D = hGenDirAll->ProjectionY();
  hGenDirReco_1D = ((TH2F*)gDirectory->Get("hGenDirReco"))->ProjectionY();
  hGenDirReco_1D->Divide(hGenDirAll_1D);
  hGenDirReco_1D->SetTitle("");
  hGenDirReco_1D->SetXTitle("#varphi^{GEN}");
  hGenDirReco_1D->SetYTitle("Efficiency");
  hGenDirReco_1D->GetYaxis()->SetTitleOffset(1.6);
  hGenDirReco_1D->SetMinimum(0.0);
  hGenDirReco_1D->SetMaximum(1.1);
  hGenDirReco_1D->Draw("");

  aCanvas->Print("MCPlots_set11.png");

  ////////////////////////////////
  //return;
  ////////////////////////////////
  aCanvas->Clear();
  aCanvas->Divide(2,2);

  ////////////////////////////////
  aCanvas->cd(1);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);
  gPad->SetBottomMargin(0.15);
  gPad->SetGrid(1,1);

  TH1D *hProjectionY;
  hProjectionY = hRangeResVsCosTheta->ProjectionY();
  hProjectionY->GetXaxis()->SetTitleOffset(1.5);
  hProjectionY->GetXaxis()->SetRangeUser(-0.1, 0.1);
  hProjectionY->Draw();
  ////////////////////////////////
  aCanvas->cd(2);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);
  gPad->SetBottomMargin(0.15);
  gPad->SetGrid(1,1);

  hProjectionY = hCosThetaResVsCosTheta->ProjectionY();
  hProjectionY->GetXaxis()->SetTitleOffset(1.5);
  hProjectionY->GetXaxis()->SetRangeUser(-0.1, 0.1);
  hProjectionY->Draw();
  ////////////////////////////////
  aCanvas->cd(3);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);
  gPad->SetBottomMargin(0.15);
  gPad->SetGrid(1,1);

  hProjectionY = hPhiResVsCosTheta->ProjectionY();
  hProjectionY->GetXaxis()->SetTitleOffset(1.5);
  hProjectionY->GetXaxis()->SetRangeUser(-0.1, 0.1);
  hProjectionY->Draw();
  ////////////////////////////////
  aCanvas->cd(4);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);
  gPad->SetBottomMargin(0.15);
  gPad->SetGrid(1,1);

  TH2F *hChargeResVsCosTheta = (TH2F*)gDirectory->Get("hChargeResVsCosTheta");
  hChargeResVsCosTheta->SetTitle("");
  hChargeResVsCosTheta->SetXTitle("cos(#theta^{GEN})");
  hChargeResVsCosTheta->SetYTitle("(charge^{RECO} - charge^{GEN})/charge^{GEN}");
  hChargeResVsCosTheta->SetZTitle("Number of events");
  hChargeResVsCosTheta->GetYaxis()->SetTitleOffset(1.6);
  hChargeResVsCosTheta->GetZaxis()->SetTitleOffset(1.5);

  hProjectionY = hChargeResVsCosTheta->ProjectionY();
  hProjectionY->GetXaxis()->SetTitleOffset(1.5);
  hProjectionY->GetXaxis()->SetRangeUser(-0.1, 0.1);
  hProjectionY->Draw();

  aCanvas->Print("MCPlots_set1.png");
  ////////////////////////////////
  //return;
  ////////////////////////////////
  aCanvas->Clear();
  aCanvas->Divide(2,2);
  ///////////////////////////////////////

  aCanvas->cd(1);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);
  gPad->SetBottomMargin(0.15);
  gPad->SetGrid(1,1);

  TH2F *hVtxXRes = (TH2F*)gDirectory->Get("hVtxXRes");
  hVtxXRes->SetTitle("");
  hVtxXRes->SetXTitle("x^{GEN} [mm]");
  hVtxXRes->SetYTitle("x^{RECO} - x^{GEN} [mm]");
  hVtxXRes->GetXaxis()->SetTitleOffset(1.5);
  hVtxXRes->GetYaxis()->SetTitleOffset(1.5);
  hVtxXRes->Draw("colz");
  ///////////////////////////////////////
  aCanvas->cd(2);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);
  gPad->SetBottomMargin(0.15);
  gPad->SetGrid(1,1);

  TH2F *hVtxYRes = (TH2F*)gDirectory->Get("hVtxYRes");
  hVtxYRes->SetTitle("");
  hVtxYRes->SetXTitle("y^{GEN} [mm]");
  hVtxYRes->SetYTitle("y^{RECO} - y^{GEN}[mm]");
  hVtxYRes->GetXaxis()->SetTitleOffset(1.5);
  hVtxYRes->GetYaxis()->SetTitleOffset(1.5);
  hVtxYRes->Draw("colz");
  ///////////////////////////////////////
  aCanvas->cd(3);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);
  gPad->SetBottomMargin(0.15);
  gPad->SetGrid(1,1);
  
  TH2F *hVtxZRes = (TH2F*)gDirectory->Get("hVtxZRes");
  hVtxZRes->SetTitle("");
  hVtxZRes->SetXTitle("z^{GEN} [mm]");
  hVtxZRes->SetYTitle("z^{RECO} - z^{GEN}[mm]");
  hVtxZRes->GetXaxis()->SetTitleOffset(1.5);
  hVtxZRes->GetYaxis()->SetTitleOffset(1.5);
  hVtxZRes->Draw("colz");
  ///////////////////////////////////////
  aCanvas->cd(4);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);
  gPad->SetBottomMargin(0.15);
  gPad->SetGrid(1,1);

  TH1D *hVtxZRes1D = hVtxZRes->ProjectionY();
  hVtxZRes1D->GetXaxis()->SetTitleOffset(1.5);
  hVtxZRes1D->GetYaxis()->SetTitleOffset(1.5);
  hVtxZRes1D->GetXaxis()->SetRangeUser(-5, 5);
  hVtxZRes1D->SetLineColor(kBlue);
  hVtxZRes1D->SetLineWidth(2);
  hVtxZRes1D->SetMaximum(1.2*hVtxZRes1D->GetMaximum());
  hVtxZRes1D->Draw();

  TH1D *hVtxXRes1D = hVtxXRes->ProjectionY();
  hVtxXRes1D->GetXaxis()->SetTitleOffset(1.5);
  hVtxXRes1D->GetYaxis()->SetTitleOffset(1.5);
  hVtxXRes1D->SetLineColor(kBlack);
  hVtxXRes1D->SetLineWidth(2);
  hVtxXRes1D->Draw("same");

  TH1D *hVtxYRes1D = hVtxYRes->ProjectionY();
  hVtxYRes1D->GetXaxis()->SetTitleOffset(1.5);
  hVtxYRes1D->GetYaxis()->SetTitleOffset(1.5);
  hVtxYRes1D->SetLineColor(kRed);
  hVtxYRes1D->SetLineWidth(2);
  hVtxYRes1D->Draw("same");

  TLegend *aLeg1 = new TLegend(0.7, 0.9, 0.99, 0.7);
  aLeg1->AddEntry(hVtxXRes1D, "x^{RECO} - x^{GEN}", "l");
  aLeg1->AddEntry(hVtxYRes1D, "y^{RECO} - y^{GEN}", "l");
  aLeg1->AddEntry(hVtxZRes1D, "z^{RECO} - z^{GEN}", "l");
  aLeg1->Draw();

  aCanvas->Print("MCPlots_set2.png");
  ///////////////////////////////////////
  //return;
  ///////////////////////////////////////
  aCanvas->Clear();
  aCanvas->Divide(2,2);
  ///////////////////////////////////////
  aCanvas->cd(1);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);
  gPad->SetBottomMargin(0.15);
  gPad->SetGrid(1,1);

  TH2F *hLengthResVsLineLoss = (TH2F*)gDirectory->Get("hLengthResVsLineLoss");
  hLengthResVsLineLoss->SetTitle("");
  hLengthResVsLineLoss->SetXTitle("lineFitLoss");
  hLengthResVsLineLoss->SetYTitle("(d^{RECO} - d^{GEN})/d^{GEN}");
  hLengthResVsLineLoss->GetXaxis()->SetTitleOffset(1.5);
  hLengthResVsLineLoss->GetYaxis()->SetTitleOffset(1.5);
  hLengthResVsLineLoss->Draw("colz");
  ///////////////////////////////////////
  aCanvas->cd(2);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);
  gPad->SetBottomMargin(0.15);
  gPad->SetGrid(1,1);

  TH2F *hLengthResVsDedxLoss = (TH2F*)gDirectory->Get("hLengthResVsDedxLoss");
  hLengthResVsDedxLoss->SetTitle("");
  hLengthResVsDedxLoss->SetXTitle("dEdxFitLoss/chargeReco");
  hLengthResVsDedxLoss->SetYTitle("(d^{RECO} - d^{GEN})/d^{GEN}");
  hLengthResVsDedxLoss->GetXaxis()->SetTitleOffset(1.5);
  hLengthResVsDedxLoss->GetYaxis()->SetTitleOffset(1.5);
  hLengthResVsDedxLoss->Draw("colz");
  ///////////////////////////////////////
  aCanvas->cd(3);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);
  gPad->SetBottomMargin(0.15);
  gPad->SetGrid(1,1);

  TH2F *hLengthResVsDedxSigma = (TH2F*)gDirectory->Get("hLengthResVsDedxSigma");
  hLengthResVsDedxSigma->SetTitle("");
  hLengthResVsDedxSigma->SetXTitle("dEdxFitSigma");
  hLengthResVsDedxSigma->SetYTitle("(d^{RECO} - d^{GEN})/d^{GEN}");
  hLengthResVsDedxSigma->GetXaxis()->SetTitleOffset(1.5);
  hLengthResVsDedxSigma->GetYaxis()->SetTitleOffset(1.5);
  hLengthResVsDedxSigma->Draw("colz");
  ///////////////////////////////////////
  aCanvas->cd(4);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);
  gPad->SetBottomMargin(0.15);
  gPad->SetGrid(1,1);

  gStyle->SetTextSize(2.0); 
  TH1F *hEventTypeReco = (TH1F*)gDirectory->Get("hEventTypeReco");
  hEventTypeReco->Scale(1.0/hEventTypeReco->Integral());
  hEventTypeReco->SetTitle("");
  hEventTypeReco->SetXTitle("Event type");
  hEventTypeReco->SetYTitle("Number of events");
  hEventTypeReco->GetXaxis()->SetTitleOffset(1.5);
  hEventTypeReco->GetYaxis()->SetTitleOffset(2.1);
  hEventTypeReco->SetMinimum(0.0);
  hEventTypeReco->SetMaximum(1.1);
  hEventTypeReco->Draw("hist text");

  ///////////////////////////////////////
  aCanvas->Print("MCPlots_set3.png");

  ///////////////////////////////////////
  aCanvas->Clear();
  aCanvas->Divide(2,2);
  ///////////////////////////////////////
  aCanvas->cd(1);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);
  gPad->SetBottomMargin(0.15);
  gPad->SetGrid(1,1);

  hChargeResVsCosTheta->Draw("colz");
  ///////////////////////////////////////
  aCanvas->cd(2);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.15);
  gPad->SetBottomMargin(0.15);
  gPad->SetGrid(1,1);

  TH2F *hRecoAlphaEndpoint = (TH2F*)gDirectory->Get("hRecoAlphaEndpoint");
  hRecoAlphaEndpoint->SetTitle("#alpha track endpoint wrt. vertex");
  //hRecoAlphaEndpoint->GetXaxis()->SetRangeUser(-120,120);
  //hRecoAlphaEndpoint->GetYaxis()->SetRangeUser(-120,120);
  hRecoAlphaEndpoint->SetXTitle("Y_{DETECTOR}");
  hRecoAlphaEndpoint->SetYTitle("Z_{DETECTOR}");
  //hRecoAlphaEndpoint->GetXaxis()->SetTitleOffset(1.5);
  hRecoAlphaEndpoint->GetYaxis()->SetTitleOffset(1.5);
  hRecoAlphaEndpoint->DrawCopy("colz");

  ///////////////////////////////////////
  aCanvas->Print("MCPlots_set4.png");
  
}


