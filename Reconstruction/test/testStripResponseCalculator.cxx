//////////////////////////
//
// root
// root [0] .L testStripResponseCalculator.cxx
// root [1] generateResponse();
// root [2] plotStripResponse(0.5);
// root [3] plotTimeResponse(0.5);
// root [4] testResponse1(0.5);
// root [5] testResponse2();
//
//////////////////////////

#ifndef __ROOTLOGON__
R__ADD_INCLUDE_PATH(../../DataFormats/include)
R__ADD_INCLUDE_PATH(../../Reconstruction/include)
R__ADD_LIBRARY_PATH(../lib)
#endif

//////////////////////////
//////////////////////////
void generateResponse() {
  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libDataFormats.so);
  }
  if (!gROOT->GetClass("StripResponseCalculator")){
    R__LOAD_LIBRARY(libReconstruction.so);
  }
  //  gSystem->Load("../lib/libDataFormats.so");
  //  gSystem->Load("../lib/libReconstruction.so");
  auto geo=std::make_shared<GeometryTPC>("geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat", false);
  geo->SetTH2PolyPartition(3*200,2*200); // higher TH2Poly granularity speeds up initialization of XY response histograms!  
  std::vector<double> sigma{0.5, 1, 2, 5};
  for(auto & v : sigma) {
    auto nstrips=4;
    auto ncells=20;
    auto sigmaXY=v;
    auto sigmaZ=v;
    auto calc=new StripResponseCalculator(geo, nstrips, ncells, sigmaXY, sigmaZ, true);
    calc->saveResponseHistograms(Form("myResponse_%dx%d_%gmm.root", nstrips, ncells, v));
  }
}

//////////////////////////
//////////////////////////
void plotStripResponse(double sigma=0.5) {
  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libDataFormats.so);
  }
  if (!gROOT->GetClass("StripResponseCalculator")){
    R__LOAD_LIBRARY(libReconstruction.so);
  }
  //  gSystem->Load("../lib/libDataFormats.so");
  //  gSystem->Load("../lib/libReconstruction.so");
  auto nstrips=4;
  auto ncells=20;
  auto sigmaXY=sigma;
  auto sigmaZ=sigma;
  auto geo=std::make_shared<GeometryTPC>("geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat", false);
  auto calc=new StripResponseCalculator(geo, nstrips, ncells, sigmaXY, sigmaZ, true, Form("myResponse_%dx%d_%gmm.root", nstrips, ncells, sigma));
  auto c=new TCanvas("c","c",5*300, 2.5*300);
  auto pad=0;
  c->Divide(5,3);
  for(auto dir=0; dir<3; dir++) {
    for(auto delta=-2; delta<=2; delta++) {
      c->cd(pad+1);
      gPad->SetRightMargin(0.2);
      gPad->SetLogz(false);
      auto h=calc->getStripResponseHistogram(dir,delta);
      h->SetStats(false);
      h->SetTitleOffset(1.8, "Z");
      h->DrawClone("CONT4Z");
      pad++;
    }
  }
  c->Print(Form("plot_responseXY_%gmm.pdf", sigma));
  c->Print(Form("plot_responseXY_%gmm.C", sigma));
}

//////////////////////////
//////////////////////////
void plotTimeResponse(double sigma=0.5) {
  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libDataFormats.so);
  }
  if (!gROOT->GetClass("StripResponseCalculator")){
    R__LOAD_LIBRARY(libReconstruction.so);
  }
  //  gSystem->Load("../lib/libDataFormats.so");
  //  gSystem->Load("../lib/libReconstruction.so");
  auto nstrips=4;
  auto ncells=20;
  auto sigmaXY=sigma;
  auto sigmaZ=sigma;
  auto geo=std::make_shared<GeometryTPC>("geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat", false);
  auto calc=new StripResponseCalculator(geo, nstrips, ncells, sigmaXY, sigmaZ, true, Form("myResponse_%dx%d_%gmm.root", nstrips, ncells, sigma));
  auto c=new TCanvas("c","c",3*300, 2.5*300);
  auto pad=0;
  std::vector<int> cells{-ncells, -ncells+1, -ncells+2, -1, 0, 1, ncells-2, ncells-1, ncells};
  c->Divide(3,3);
  for(auto & cell : cells) {
    c->cd(pad+1);
    gPad->SetLeftMargin(0.2);
    gPad->SetRightMargin(0.05);
    gPad->SetLogy(false);
    auto h=calc->getTimeResponseHistogram(cell);
    h->SetStats(false);
    h->SetFillColor(kBlue);
    h->SetFillStyle(3001);
    h->SetLineColor(kBlue);
    h->SetLineWidth(2);
    h->SetTitleOffset(1.8, "Y");
    h->DrawClone("HISTO");
    pad++;
  }
  c->Print(Form("plot2_responseZ_%gmm.pdf", sigma));
  c->Print(Form("plot2_responseZ_%gmm.C", sigma));

  // alternative representation
  c->Clear();
  c->Divide(1,1);
  c->cd(pad+1);
  gPad->SetLeftMargin(0.15);
  gPad->SetRightMargin(0.05);
  gPad->SetLogy(false);
  TH1D* first_hist=NULL;
  auto ymin=1e30;
  auto ymax=-1e30;
  for(auto cell=-ncells; cell<=ncells; cell++) {
    auto h=calc->getTimeResponseHistogram(cell);
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
  c->Print(Form("plot2_responseZ_%gmm.pdf", sigma));
  c->Print(Form("plot2_responseZ_%gmm.C", sigma));
}

//////////////////////////
//////////////////////////
void addConstantToTH2D(TH2D* h, double c) {
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
    R__LOAD_LIBRARY(libDataFormats.so);
  }
  if (!gROOT->GetClass("StripResponseCalculator")){
    R__LOAD_LIBRARY(libReconstruction.so);
  }
  //  gSystem->Load("../lib/libDataFormats.so");
  //  gSystem->Load("../lib/libReconstruction.so");
  auto nstrips=4;
  auto ncells=20;
  auto sigmaXY=sigma;
  auto sigmaZ=sigma;
  auto geo=std::make_shared<GeometryTPC>("geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat", false);
  geo->SetTH2PolyPartition(3*20,2*20); // higher TH2Poly granularity speeds up finding reference nodes
  auto calc=new StripResponseCalculator(geo, nstrips, ncells, sigmaXY, sigmaZ, true, Form("myResponse_%dx%d_%gmm.root", nstrips, ncells, sigma));

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
  auto pos=TVector3(50.0, 10.0, -10.0);
  auto charge=1000;
  auto length=50.0; // [mm]
  auto npoints=(int)(length/geo->GetStripPitch());
  auto unit_vec=TVector3(1,1,1).Unit();
  for(auto ipoint=0; ipoint<npoints; ipoint++) {
    calc->fillProjections(pos+unit_vec*(ipoint*length/npoints), (ipoint+1)*charge/npoints);
  }
  //  calc->fillProjections(pos, charge);

  // plot results
  auto c=new TCanvas("c","c",3*500, 500);
  auto pad=0;
  c->Divide(3,1);
  for(auto & h : histosInMM) {
    c->cd(pad+1);
    gPad->SetRightMargin(0.2);
    gPad->SetLogz(false);
    h->SetMinimum(minval); // for log scale
    h->SetStats(false);
    h->SetTitleOffset(1.5, "X");
    h->SetTitleOffset(1.5, "Y");
    h->SetTitleOffset(1.8, "Z");
    h->DrawClone("COLZ");
    pad++;
  }
  c->Print(Form("plot_UVW_%gmm.pdf", sigma));
  c->Print(Form("plot_UVW_%gmm.C", sigma));  
}

//////////////////////////
//////////////////////////
void testResponse2() {
  if (!gROOT->GetClass("GeometryTPC")){
    R__LOAD_LIBRARY(libDataFormats.so);
  }
  if (!gROOT->GetClass("StripResponseCalculator")){
    R__LOAD_LIBRARY(libReconstruction.so);
  }
  //  gSystem->Load("../lib/libDataFormats.so");
  //  gSystem->Load("../lib/libReconstruction.so");
  auto nstrips=4;
  auto ncells=20;
  auto geo=std::make_shared<GeometryTPC>("geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat", false);
  geo->SetTH2PolyPartition(3*20,2*20); // higher TH2Poly granularity speeds up finding reference nodes
  std::vector<double> sigma{0.5, 1, 2};
  std::vector<StripResponseCalculator*> calc(sigma.size());
  for(auto i=0; i<calc.size(); i++) { 
    auto sigmaXY=sigma[i];
    auto sigmaZ=sigma[i];
    calc[i]=new StripResponseCalculator(geo, nstrips, ncells, sigmaXY, sigmaZ, true, Form("myResponse_%dx%d_%gmm.root", nstrips, ncells, sigma[i]));
  }

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
  for(auto i=0; i<calc.size(); i++) {
    calc[i]->setUVWprojectionsRaw(histosRaw);
    calc[i]->setUVWprojectionsInMM(histosInMM);
  }
  
  // fill UZ/VZ/WZ histograms - simulate triangular shape of dE/dx
  for(auto i=0; i<calc.size(); i++) {
    auto pos0=TVector3(50.0, 50.0, -30.0);
    auto pos=pos0+TVector3(0, -50, 0)*i;
    auto charge=10000;
    auto length=50.0; // [mm]
    auto npoints=(int)(length/geo->GetStripPitch());
    auto unit_vec=TVector3(1,1,1).Unit();
    for(auto ipoint=0; ipoint<npoints; ipoint++) {
      calc[i]->fillProjections(pos+unit_vec*(ipoint*length/npoints), (ipoint+1)*charge/npoints);
    }
  }

  // fill UZ/VZ/WZ histograms - simulate point charge
  for(auto i=0; i<calc.size(); i++) {
    auto pos0=TVector3(60.0, 60.0, 40.0);
    auto pos=pos0+TVector3(0, -50, 0)*i;
    auto charge=10000;
     calc[i]->fillProjections(pos, charge);
  }

  // plot results
  auto c=new TCanvas("c","c",3*500, 500);
  auto pad=0;
  c->Divide(3,1);
  for(auto & h : histosInMM) {
    c->cd(pad+1);
    gPad->SetRightMargin(0.2);
    gPad->SetLogz(true);
    h->SetMinimum(minval); // for log scale
    h->SetStats(false);
    h->SetTitleOffset(1.5, "X");
    h->SetTitleOffset(1.5, "Y");
    h->SetTitleOffset(1.8, "Z");
    h->DrawClone("COLZ");
    pad++;
  }
  c->Print("plot_UVW_mix.pdf");
  c->Print("plot_UVW_mix.C");
}

