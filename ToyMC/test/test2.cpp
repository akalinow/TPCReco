#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

#include "UtilsTPC.h"
#include "GeometryTPC.h"
#include "UVWprojector.h"
#include "EventTPC.h"

#include "TApplication.h"
#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TBrowser.h"
#include "TStyle.h"

#define CANVAS_PAD_NX 3
#define CANVAS_PAD_NY 1
#define CANVAS_TITLE_FRACY 0.2
#define CANVAS_PAD_SIZEX 100
#define CANVAS_PAD_SIZEY 300

TCanvas* getCanvas() {

 TCanvas *canvas=new TCanvas("c_raw", "On-line raw data preview", 800, 700);

 // set draw style
 gROOT->SetStyle("Plain");   
 gStyle->SetTitleYOffset(1.3);
 gStyle->SetTitleXOffset(1.3);
 gStyle->SetLineWidth(1);
 gStyle->SetOptStat(0);
 gStyle->SetOptTitle(1);

 gStyle->SetPalette(56);     // yellow-to-dark red

 canvas->Divide(1,2);
 TPad *pad1 = (TPad*)canvas->GetPad(1);
 TPad *pad2 = (TPad*)canvas->GetPad(2);
 pad1->SetPad(0.01,0.29,0.99,0.99);
 pad2->SetPad(0.01,0.01,0.99,0.29);
 pad2->Divide(3,1);
 pad2->SetBorderMode(1);
 
 return canvas; 
}

int main(int argc, char *argv[]) {

  TApplication app("app", &argc, argv);

   ///Initialize projections
  std::string geomFileName = "resources/geometry_mini_eTPC.dat";
  std::shared_ptr<GeometryTPC> myGeometryPtr = std::make_shared<GeometryTPC>(geomFileName.c_str());
  
  std::string dataFileName = "resources/EventTPC_1.root";
  TFile aFile(dataFileName.c_str(),"READ");
  TTree *aTree = (TTree*)aFile.Get("TPCData");
  unsigned int nEvents = aTree->GetEntries();
  std::cout<<"Number of events: "<<nEvents<<std::endl;

  EventTPC *aEvent = 0;
  aTree->SetBranchAddress("Event", &aEvent);

  TCanvas *canvas = getCanvas();

  for(unsigned int iEvent=0;iEvent<nEvents;++iEvent){
    std::cout<<"Reading event: "<<iEvent<<std::endl;
    aEvent->SetGeoPtr(0);
    aTree->GetEntry(iEvent);
    aEvent->SetGeoPtr(myGeometryPtr);

    double eventMaxCharge = aEvent->GetMaxCharge();

    std::cout<<"eventMaxCharge: "<<eventMaxCharge<<std::endl;
    if(eventMaxCharge<200){
      std::cout<<"Empty event. Skipping."<<std::endl;
      continue;
    }

    canvas->cd(1);
    canvas->GetPad(1)->Clear();
    canvas->GetPad(1)->Update();

    canvas->cd(2);
    gPad->cd(1);
    aEvent->GetStripVsTime(DIR_U)->Draw("colz");
    canvas->cd(2);
    gPad->cd(2);
    aEvent->GetStripVsTime(DIR_V)->Draw("colz");
    canvas->cd(2);
    gPad->cd(3);
    aEvent->GetStripVsTime(DIR_W)->Draw("colz");
            
    double chargeThreshold = 0.2*eventMaxCharge;
    int delta_timecells = 1;
    int delta_strips = 1;
    double radius = 10.0;
    int rebin_space=EVENTTPC_DEFAULT_STRIP_REBIN;
    int rebin_time=EVENTTPC_DEFAULT_TIME_REBIN; 
    int method=EVENTTPC_DEFAULT_RECO_METHOD;

    SigClusterTPC aCluster = aEvent->GetOneCluster(chargeThreshold, delta_strips, delta_timecells);

    canvas->cd(1);
    TH3F *h3D = aEvent->Get3D(aCluster,  radius, rebin_space, rebin_time, method);
    if(h3D) h3D->Draw("box2z");
    canvas->Update();
  
    app.Run(kTRUE);
  }

 
  return 0;
}
