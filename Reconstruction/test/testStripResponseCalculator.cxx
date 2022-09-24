//////////////////////////
//
// root
// root [0] .L ../test/testStripResponseCalculator.cxx
// root [1] generateResponseAll();
// root [2] plotStripResponse(1.0);
// root [3] plotTimeResponse(1.0);
// root [4] testResponse1(1.0);
// root [5] testResponse2();
// root [6] testResponse3();
//
//////////////////////////

#ifndef __ROOTLOGON__
R__ADD_INCLUDE_PATH(../../DataFormats/include)
R__ADD_INCLUDE_PATH(../../Reconstruction/include)
R__ADD_LIBRARY_PATH(../lib)
#endif

//////////////////////////
//////////////////////////
void generateResponse(double sigma=0.5) { // generates histograms and saves them to a separate ROOT file
  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("StripResponseCalculator")){
    R__LOAD_LIBRARY(libTPCReconstruction.so);
  }
  //  gSystem->Load("../lib/libDataFormats.so");
  //  gSystem->Load("../lib/libReconstruction.so");
  auto geo=std::make_shared<GeometryTPC>("geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat", false);
  //  geo->SetTH2PolyPartition(3*20,2*20); // higher TH2Poly granularity speeds up finding reference nodes
  geo->SetTH2PolyPartition(3*200,2*200); // higher TH2Poly granularity speeds up initialization of XY response histograms!
  auto nstrips=6;
  auto ncells=30;
  auto npads=nstrips;
  auto sigmaXY=sigma;
  auto sigmaZ=sigma;
  auto npoints=10000000; // 10M points
  auto calc=new StripResponseCalculator(geo, nstrips, ncells, npads, sigmaXY, sigmaZ);
  calc->setDebug(true);
  calc->initializeStripResponse(npoints); // overwrite default value
  calc->saveHistograms(Form("myResponse_%dx%dx%d_%gmm.root", nstrips, ncells, npads, sigma));
}

//////////////////////////
//////////////////////////
void generateResponseAll() {
  std::vector<double> sigma{0.5, 1, 2};
  for(auto & s : sigma) {
    generateResponse(s); // generates histograms and saves them to a separate ROOT file
  }
}

//////////////////////////
//////////////////////////
void plotStripResponse(double sigma=0.5) {
  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("StripResponseCalculator")){
    R__LOAD_LIBRARY(libTPCReconstruction.so);
  }
  //  gSystem->Load("../lib/libDataFormats.so");
  //  gSystem->Load("../lib/libReconstruction.so");
  auto nstrips=6;
  auto ncells=30;
  auto npads=nstrips;
  auto sigmaXY=sigma;
  auto sigmaZ=sigma;
  auto geo=std::make_shared<GeometryTPC>("geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat", false);
  auto calc=new StripResponseCalculator(geo, nstrips, ncells, npads, sigmaXY, sigmaZ, Form("myResponse_%dx%dx%d_%gmm.root", nstrips, ncells, npads, sigma), true);
  auto c=new TCanvas("c","c",5*300, 2.5*300);
  auto pad=0;
  c->Divide(5,3);
  for(auto dir=0; dir<3; dir++) {
    for(auto delta=-2; delta<=2; delta++) {
      pad++;
      c->cd(pad);
      gPad->SetRightMargin(0.2);
      gPad->SetLogz(false);
      auto h=calc->getStripResponseHistogram(dir,delta);
      if(!h) continue;
      h->SetStats(false);
      h->SetTitleOffset(1.8, "Z");
      h->DrawClone("COLZ"); // ("CONT4Z");
    }
  }
  c->Print(TString(Form("plot_responseXY_%gmm", sigma)).ReplaceAll(".","_")+".png");
  c->Print(TString(Form("plot_responseXY_%gmm", sigma)).ReplaceAll(".","_")+".pdf");
  c->Print(TString(Form("plot_responseXY_%gmm", sigma)).ReplaceAll(".","_")+".root");
}

//////////////////////////
//////////////////////////
void plotTimeResponse(double sigma=0.5) {
  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("StripResponseCalculator")){
    R__LOAD_LIBRARY(libTPCReconstruction.so);
  }
  //  gSystem->Load("../lib/libDataFormats.so");
  //  gSystem->Load("../lib/libReconstruction.so");
  auto nstrips=6;
  auto ncells=30;
  auto npads=nstrips;
  auto sigmaXY=sigma;
  auto sigmaZ=sigma;
  auto geo=std::make_shared<GeometryTPC>("geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat", false);
  auto calc=new StripResponseCalculator(geo, nstrips, ncells, npads, sigmaXY, sigmaZ, Form("myResponse_%dx%dx%d_%gmm.root", nstrips, ncells, npads, sigma), true);
  auto c=new TCanvas("c","c",3*300, 2.5*300);
  auto pad=0;
  std::vector<int> cells{-ncells, -ncells+1, -ncells+2, -1, 0, 1, ncells-2, ncells-1, ncells};
  c->Divide(3,3);
  for(auto & cell : cells) {
    pad++;
    c->cd(pad);
    gPad->SetLeftMargin(0.2);
    gPad->SetRightMargin(0.05);
    gPad->SetLogy(false);
    auto h=calc->getTimeResponseHistogram(cell);
    if(!h) continue;
    h->SetStats(false);
    h->SetFillColor(kBlue);
    h->SetFillStyle(3001);
    h->SetLineColor(kBlue);
    h->SetLineWidth(2);
    h->SetTitleOffset(1.8, "Y");
    h->DrawClone("HISTO");
  }
  c->Print(TString(Form("plot_responseZ_%gmm", sigma)).ReplaceAll(".","_")+".pdf");
  c->Print(TString(Form("plot_responseZ_%gmm", sigma)).ReplaceAll(".","_")+".png");
  c->Print(TString(Form("plot_responseZ_%gmm", sigma)).ReplaceAll(".","_")+".root");

  // alternative representation
  c->Clear();
  c->Divide(1,1);
  c->cd(1);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.05);
  gPad->SetLogy(false);
  TH1D* first_hist=NULL;
  auto ymin=1e30;
  auto ymax=-1e30;
  for(auto cell=-ncells; cell<=ncells; cell++) {
    auto h=calc->getTimeResponseHistogram(cell);
    if(!h) continue;
    ymin=std::min(h->GetMinimum(), ymin);
    ymax=std::max(h->GetMaximum(), ymax);
    h->SetStats(false);
    if(cell<0) {
      h->SetLineColor(kBlue);
      h->SetFillStyle(0);
    } else if(cell>0) {
      h->SetLineColor(kRed+2);
      h->SetFillStyle(0);
    } else {
      h->SetFillColor(kGreen+2);
      h->SetLineColor(kGreen+2);
      h->SetFillStyle(3001);
    }
    h->SetLineWidth(2);
    h->SetTitleOffset(2, "Y");
    if(!first_hist) {
      first_hist=(TH1D*)h->DrawClone("HISTO");
    } else h->DrawClone("HISTO SAME");
  }
  first_hist->SetMinimum(ymin*0.95);
  first_hist->SetMaximum(ymax*1.05);
  first_hist->SetTitle(Form("Vertical response for #sigma_{Z}=%g mm", sigmaZ));
  first_hist->SetTitleOffset(1.8, "Y");
  c->Update();
  c->Modified();
  c->Print(TString(Form("plot2_responseZ_%gmm", sigma)).ReplaceAll(".","_")+".pdf");
  c->Print(TString(Form("plot2_responseZ_%gmm", sigma)).ReplaceAll(".","_")+".png");
  c->Print(TString(Form("plot2_responseZ_%gmm", sigma)).ReplaceAll(".","_")+".root");
}

//////////////////////////
//////////////////////////
void addConstantToTH2D(TH2D* h, double c) {
  if(!h) return;
  for(auto ibin1=1; ibin1<=h->GetNbinsX(); ibin1++) {
    for(auto ibin2=1; ibin2<=h->GetNbinsY(); ibin2++) {
      h->SetBinContent(ibin1, ibin2, h->GetBinContent(ibin1, ibin2) + c);
    }
  }
}


//////////////////////////
//////////////////////////
void testResponse1(double sigma=0.5) {
  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("StripResponseCalculator")){
    R__LOAD_LIBRARY(libTPCReconstruction.so);
  }
  //  gSystem->Load("../lib/libDataFormats.so");
  //  gSystem->Load("../lib/libReconstruction.so");
  auto nstrips=6;
  auto ncells=30;
  auto npads=nstrips;
  auto sigmaXY=sigma;
  auto sigmaZ=sigma;
  auto geo=std::make_shared<GeometryTPC>("geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat", false);
  geo->SetTH2PolyPartition(3*20,2*20); // higher TH2Poly granularity speeds up finding reference nodes
  auto calc=new StripResponseCalculator(geo, nstrips, ncells, npads, sigmaXY, sigmaZ, Form("myResponse_%dx%dx%d_%gmm.root", nstrips, ncells, npads, sigma));

  // create dummy event to get empty UZ/VZ/WZ projection histograms
  auto event=new EventTPC(); // empty event
  event->SetGeoPtr(geo);
  std::vector<std::shared_ptr<TH2D> > histosRaw(3);
  std::vector<std::shared_ptr<TH2D> > histosInMM(3);
  const auto minval=1;
  for(auto strip_dir=0; strip_dir<3; strip_dir++) {
    histosInMM[strip_dir]=event->GetStripVsTimeInMM(strip_dir);
    histosRaw[strip_dir]=event->GetStripVsTime(strip_dir);
    addConstantToTH2D(histosRaw[strip_dir].get(), minval);
    addConstantToTH2D(histosInMM[strip_dir].get(), minval);
  }

  // associate UZ/VZ/WZ histograms with reponse calculator
  calc->setUVWprojectionsRaw(histosRaw);
  calc->setUVWprojectionsInMM(histosInMM);

  // fill UZ/VZ/WZ histograms
  auto pos=TVector3(50.0, 0.0, -10.0); // 10.0, -10.0);
  auto charge=1000;
  auto length=50.0; // [mm]
  auto npoints=(int)(2*length/geo->GetStripPitch());
  auto unit_vec=TVector3(1,1,1).Unit();
  //  for(auto ipoint=0; ipoint<npoints; ipoint++) {
  //    calc->addCharge(pos+unit_vec*(ipoint*length/npoints), (ipoint+1)*charge/npoints);
  //  }
  calc->addCharge(pos, charge);

  // plot results
  auto c=new TCanvas("c","c",3*500, 500);
  auto pad=0;
  c->Divide(3,1);
  for(auto & h : histosInMM) {
    pad++;
    c->cd(pad);
    gPad->SetRightMargin(0.2);
    gPad->SetLogz(false);
    h->SetMinimum(minval); // for log scale
    h->SetStats(false);
    h->SetTitleOffset(1.5, "X");
    h->SetTitleOffset(1.5, "Y");
    h->SetTitleOffset(1.8, "Z");
    h->DrawClone("COLZ");
  }
  c->Print(TString(Form("plot_UVW_%gmm", sigma)).ReplaceAll(".","_")+".pdf");
  c->Print(TString(Form("plot_UVW_%gmm", sigma)).ReplaceAll(".","_")+".png");
  c->Print(TString(Form("plot_UVW_%gmm", sigma)).ReplaceAll(".","_")+".root");
}

//////////////////////////
//////////////////////////
void testResponse2() {
  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("StripResponseCalculator")){
    R__LOAD_LIBRARY(libTPCReconstruction.so);
  }
  //  gSystem->Load("../lib/libDataFormats.so");
  //  gSystem->Load("../lib/libReconstruction.so");
  auto nstrips=6;
  auto ncells=30;
  auto npads=nstrips;
  auto geo=std::make_shared<GeometryTPC>("geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat", false);
  geo->SetTH2PolyPartition(3*20,2*20); // higher TH2Poly granularity speeds up finding reference nodes
  std::vector<double> sigma{0.5, 1, 2};
  std::vector<StripResponseCalculator*> calc(sigma.size());
  for(auto i=0; i<calc.size(); i++) { 
    auto sigmaXY=sigma[i];
    auto sigmaZ=sigma[i];
    calc[i]=new StripResponseCalculator(geo, nstrips, ncells, npads, sigmaXY, sigmaZ, Form("myResponse_%dx%dx%d_%gmm.root", nstrips, ncells, npads, sigma[i]));
  }

  TStopwatch t;
  t.Start();

  // create dummy event to get empty UZ/VZ/WZ projection histograms
  auto event=std::make_shared<EventTPC>(); // empty event
  event->SetGeoPtr(geo);
  std::vector<std::shared_ptr<TH2D> > histosRaw(3);
  std::vector<std::shared_ptr<TH2D> > histosInMM(3);
  const auto minval=1;
  for(auto strip_dir=0; strip_dir<3; strip_dir++) {
    histosInMM[strip_dir]=event->GetStripVsTimeInMM(strip_dir);
    histosRaw[strip_dir]=event->GetStripVsTime(strip_dir);
    addConstantToTH2D(histosRaw[strip_dir].get(), minval);
    addConstantToTH2D(histosInMM[strip_dir].get(), minval);
  }

  // associate UZ/VZ/WZ histograms (merged strip sections) with reponse calculator
  for(auto i=0; i<calc.size(); i++) {
    calc[i]->setUVWprojectionsRaw(histosRaw);
    calc[i]->setUVWprojectionsInMM(histosInMM);
  }
  
  // fill UZ/VZ/WZ histograms (merged strip sections) - simulate triangular shape of dE/dx
  for(auto i=0; i<calc.size(); i++) {
    auto pos0=TVector3(50.0, 50.0, -30.0);
    auto pos=pos0+TVector3(0, -50, 0)*i;
    auto charge=10000;
    auto length=50.0; // [mm]
    auto npoints=(int)(2*length/geo->GetStripPitch());
    auto unit_vec=TVector3(1,1,1).Unit();
    for(auto ipoint=0; ipoint<npoints; ipoint++) {
      calc[i]->addCharge(pos+unit_vec*(ipoint*length/npoints), (ipoint+1)*charge/npoints);
    }
  }

  // fill UZ/VZ/WZ histograms (merged strip sections) - simulate point charge
  for(auto i=0; i<calc.size(); i++) {
    auto pos0=TVector3(60.0, 60.0, 40.0);
    auto pos=pos0+TVector3(0, -50, 0)*i;
    auto charge=10000;
     calc[i]->addCharge(pos, charge);
  }

  t.Stop();
  t.Print();

  // plot results
  auto c=new TCanvas("c","c",3*500, 500);
  auto pad=0;
  c->Divide(3,1);
  for(auto & h : histosInMM) {
    pad++;
    c->cd(pad);
    gPad->SetRightMargin(0.2);
    gPad->SetLogz(true);
    h->SetMinimum(minval); // for log scale
    h->SetStats(false);
    h->SetTitleOffset(1.5, "X");
    h->SetTitleOffset(1.5, "Y");
    h->SetTitleOffset(1.8, "Z");
    h->DrawClone("COLZ");
  }
  c->Print("plot_UVW_mix.pdf");
  c->Print("plot_UVW_mix.png");
  c->Print("plot_UVW_mix.root");
}

//////////////////////////
//////////////////////////
void testResponse3() {
  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libTPCDataFormats.so);
  }
  if (!gROOT->GetClass("StripResponseCalculator")){
    R__LOAD_LIBRARY(libTPCReconstruction.so);
  }
  //  gSystem->Load("../lib/libDataFormats.so");
  //  gSystem->Load("../lib/libReconstruction.so");
  auto nstrips=6;
  auto ncells=30;
  auto npads=nstrips;
  auto geo=std::make_shared<GeometryTPC>("geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat", false);
  geo->SetTH2PolyPartition(3*20,2*20); // higher TH2Poly granularity speeds up finding reference nodes
  std::vector<double> sigma{0.5, 1, 2};
  std::vector<StripResponseCalculator*> calc(sigma.size());
  for(auto i=0; i<calc.size(); i++) {
    auto sigmaXY=sigma[i];
    auto sigmaZ=sigma[i];
    calc[i]=new StripResponseCalculator(geo, nstrips, ncells, npads, sigmaXY, sigmaZ, Form("myResponse_%dx%dx%d_%gmm.root", nstrips, ncells, npads, sigma[i]));
  }

  TStopwatch t;
  t.Start();

  // create in memory 1000 dummy events for algorithm's speed test
  std::shared_ptr<EventTPC> event;
  for(auto ievent=0; ievent<1000; ievent++) {

    if(ievent%100==0) std::cout << "event=" << ievent << std::endl;

    // create empty event
    event=std::make_shared<EventTPC>(); // empty event
    event->SetGeoPtr(geo);
    for(auto i=0; i<calc.size(); i++) {
      calc[i]->setEventTPC(event);
    }

    // fill EventTPC - simulate triangular shape of dE/dx
    for(auto i=0; i<calc.size(); i++) {
      auto pos0=TVector3(50.0, 50.0, -30.0);
      auto pos=pos0+TVector3(0, -50, 0)*i;
      auto charge=10000;
      auto length=50.0; // [mm]
      auto npoints=100; // (int)(2*length/geo->GetStripPitch());
      auto unit_vec=TVector3(1,1,1).Unit();
      for(auto ipoint=0; ipoint<npoints; ipoint++) {
	calc[i]->addCharge(pos+unit_vec*(ipoint*length/npoints), (ipoint+1)*charge/npoints);
      }
    }
  }

  t.Stop();
  t.Print();

  // get UZ/VZ/WZ histograms (merged strip sections) from EventTPC
  std::vector<std::shared_ptr<TH2D> > histosRaw(3);
  std::vector<std::shared_ptr<TH2D> > histosInMM(3);
  const auto minval=1;
  for(auto strip_dir=0; strip_dir<3; strip_dir++) {
    histosInMM[strip_dir]=event->GetStripVsTimeInMM(strip_dir);
    histosRaw[strip_dir]=event->GetStripVsTime(strip_dir);
    addConstantToTH2D(histosRaw[strip_dir].get(), minval);
    addConstantToTH2D(histosInMM[strip_dir].get(), minval);
  }

  // plot results for last event
  auto c=new TCanvas("c","c",3*500, 500);
  auto pad=0;
  c->Divide(3,1);
  for(auto & h : histosInMM) {
    pad++;
    c->cd(pad);
    gPad->SetRightMargin(0.2);
    gPad->SetLogz(true);
    h->SetMinimum(minval); // for log scale
    h->SetStats(false);
    h->SetTitleOffset(1.5, "X");
    h->SetTitleOffset(1.5, "Y");
    h->SetTitleOffset(1.8, "Z");
    h->DrawClone("COLZ");
  }
  c->Print("plot_UVW_mix2.pdf");
  c->Print("plot_UVW_mix2.png");
  c->Print("plot_UVW_mix2.root");
}
