#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

#include "UtilsTPC.h"
#include "GeometryTPC.h"
#include "UVWprojector.h"
#include "EventTPC.h"
#include "TrackSegmentTPC.h"

#include "TApplication.h"
#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"
#include "TBrowser.h"
#include "TStyle.h"
#include "TPolyLine3D.h"
#include "TLine.h"
#include "TVector2.h"
#include "TH2Poly.h"

#define CANVAS_PAD_NX 3
#define CANVAS_PAD_NY 1
#define CANVAS_TITLE_FRACY 0.2
#define CANVAS_PAD_SIZEX 100
#define CANVAS_PAD_SIZEY 300


TPolyLine3D getPolyLine(TrackSegment3D & aTrackSegment){

  TPolyLine3D aTrackPolyLine(2);    
  aTrackPolyLine.SetPoint(0,aTrackSegment.GetStartPoint().X(),
			  aTrackSegment.GetStartPoint().Y(),
			  aTrackSegment.GetStartPoint().Z());
  
  aTrackPolyLine.SetPoint(1,aTrackSegment.GetEndPoint().X(),
			  aTrackSegment.GetEndPoint().Y(),
			  aTrackSegment.GetEndPoint().Z());
  
  aTrackPolyLine.SetLineColor(2);
  aTrackPolyLine.SetLineWidth(2);
  std::cout<<"3D poly line: "<<std::endl;
  aTrackSegment.GetStartPoint().Print();
  aTrackSegment.GetEndPoint().Print();

  return aTrackPolyLine;
}

TLine get2DLine(TrackSegment3D & aTrackSegment, int aDir, std::shared_ptr<GeometryTPC> myGeometryPtr){

  TVector3 aStart = aTrackSegment.GetStartPoint();
  TVector3 aEnd = aTrackSegment.GetEndPoint();

  bool err_flag = false;
  int startX = myGeometryPtr->Pos2timecell(aStart.Z(), err_flag);
  int endX = myGeometryPtr->Pos2timecell(aEnd.Z(), err_flag);

  double directionScale = 1.0/myGeometryPtr->GetStripPitch();
  TVector2 referencePoint = myGeometryPtr->GetReferencePoint();
  TVector2 start2D(aStart.X(), aStart.Y());
  TVector2 end2D(aEnd.X(), aEnd.Y());

  start2D -= referencePoint;
  end2D -= referencePoint;

  int offset = 1;
  if(aDir==0){
    start2D += 2*referencePoint;
    end2D += 2*referencePoint;
    offset = 1;
  }
  if(aDir==2){
    start2D += 4*referencePoint;
    end2D += 4*referencePoint;
    offset = 6;
  }
  
  int startY = myGeometryPtr->Cartesian2posUVW(start2D, aDir, err_flag)*directionScale + offset;
  int endY = myGeometryPtr->Cartesian2posUVW(end2D, aDir, err_flag)*directionScale + offset;

  std::cout<<" dir: "<<aDir
	   <<" start (X, Y): "<<startX<<" "<<startY
	   <<" end (X, Y): "<<endX<<" "<<endY
	   <<std::endl;
  TLine aLine(startX, startY, endX, endY);
  aLine.SetLineWidth(2);
  aLine.SetLineColor(2);
	      
  return aLine;
}
///////////////////////////////////////////////
///////////////////////////////////////////////
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

 canvas->Divide(1,3);
 TPad *pad1 = (TPad*)canvas->GetPad(1);
 TPad *pad2 = (TPad*)canvas->GetPad(2);
 TPad *pad3 = (TPad*)canvas->GetPad(3);
 pad1->SetPad(0.01,0.6,0.99,0.99);
 pad2->SetPad(0.01,0.3,0.99,0.6);
 pad3->SetPad(0.01,0.01,0.99,0.3);

 pad2->Divide(3,1);
 pad2->SetBorderMode(1);

 pad3->Divide(3,1);
 pad3->SetBorderMode(1);
 
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

  nEvents = 10;
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
    /*
    std::cout<<"eventMaxCharge: "<<eventMaxCharge<<std::endl;
    if(eventMaxCharge<200){
      std::cout<<"Empty event. Skipping."<<std::endl;
      continue;
      }*/

    canvas->cd(1);
    canvas->GetPad(1)->Clear();
    canvas->GetPad(1)->Update();
            
    double chargeThreshold = 0.2*eventMaxCharge;
    int delta_timecells = 1;
    int delta_strips = 1;
    double radius = 2.0;
    int rebin_space=EVENTTPC_DEFAULT_STRIP_REBIN;
    int rebin_time=EVENTTPC_DEFAULT_TIME_REBIN; 
    int method=EVENTTPC_DEFAULT_RECO_METHOD;
    
    SigClusterTPC aCluster = aEvent->GetOneCluster(chargeThreshold, delta_strips, delta_timecells);
    TrackSegment3D aTrackSegment = aEvent->FindTrack(aCluster);

    canvas->cd(2);
    gPad->cd(1);
    aEvent->GetStripVsTime(aCluster, DIR_U)->Draw("colz");
    TLine aLine_U = get2DLine(aTrackSegment, DIR_U, myGeometryPtr);

    aLine_U.Draw();
    canvas->cd(2);
    gPad->cd(2);
    aEvent->GetStripVsTime(aCluster, DIR_V)->Draw("colz");
    TLine aLine_V = get2DLine(aTrackSegment, DIR_V, myGeometryPtr);
    aLine_V.Draw();
    canvas->cd(2);
    gPad->cd(3);
    aEvent->GetStripVsTime(aCluster, DIR_W)->Draw("colz");
    TLine aLine_W = get2DLine(aTrackSegment, DIR_W, myGeometryPtr);
    aLine_W.Draw();
    gPad->Update();

    canvas->cd(1);
    TH3D *h3D = aEvent->Get3D(aCluster,  radius, rebin_space, rebin_time, method);
    if(h3D){
      TPolyLine3D *aPoly3D = new TPolyLine3D(getPolyLine(aTrackSegment));
      h3D->Draw("box2z");
      aPoly3D->Draw();
      canvas->Update();
    }
    /*
    UVWprojector myUVWProjector(myGeometryPtr.get(), 100, 92, 92);
    myUVWProjector.SetEvent3D(*h3D);  

    canvas->cd(3);
    gPad->cd(1);
    myUVWProjector.GetStripVsTime_TH2D(DIR_U)->Draw("colz");
    canvas->cd(3);
    gPad->cd(2);
    myUVWProjector.GetStripVsTime_TH2D(DIR_V)->Draw("colz");
    canvas->cd(3);
    gPad->cd(3);
    myUVWProjector.GetStripVsTime_TH2D(DIR_W)->Draw("colz");
    */
    app.Run(kTRUE);
  }

 
  return 0;
}
