#include <iostream>
#include <thread>
#include <chrono>
#include <memory>

#include "UtilsTPC.h"
#include "GeometryTPC.h"
#include "UVWprojector.h"
#include "EventTPC.h"

#include "TFile.h"
#include "TTree.h"
#include "TCanvas.h"

int main(int argc, char *argv[]) {

  ///Load data
  std::string fileName = "resources/bkg_1e7gammas_8.3MeV__1mm_bins.root";
  std::string hName = "Edep-hist";

  fileName = "resources/signal_4He_12C_gamma_8.3MeV__1mm_bins.root";
  hName = "45";
  TFile file(fileName.c_str(),"R");

  TH3D *hData3D = (TH3D*)file.Get(hName.c_str());  
  if(!hData3D) return 0;

  double convert_to_mm_factor = 10.0;
  TH3D *hData3D_scaled = rescale_TH3D_axes(hData3D, convert_to_mm_factor);
  hData3D->Print();


  ///Initialize projections
  fileName = "resources/geometry_mini_eTPC_rot90deg.dat";
  std::shared_ptr<GeometryTPC> myGeometryPtr = std::make_shared<GeometryTPC>(fileName.c_str());
  UVWprojector myUVWProjector(myGeometryPtr.get(), 100, 25, 25); //  100, 100, 100);

  myGeometryPtr->SetDebug(false);
  myUVWProjector.SetDebug(false);
  myUVWProjector.SetEvent3D(*hData3D_scaled);  
  std::cout << myUVWProjector.GetAreaNpoints() << std::endl;
  std::cout << myGeometryPtr->GetTH2PolyPartitionX() << std::endl;
  std::cout << myGeometryPtr->GetTH2PolyPartitionY() << std::endl;
  std::cout << ", integral=" << myUVWProjector.GetEventIntegral() << std::endl;

  TCanvas *c1 = new TCanvas("c1","c1", 900, 800);
  TH2Poly *tp1 = myUVWProjector.GetStripProfile_TH2Poly();
  if(!tp1) {
    std::cerr << "ERROR: Strip profile TH2Poly is nullptr !!!" << std::endl; 
    return -1;
  }
  tp1->Draw("COLZ");
  c1->Print("UVZProjection.png");
  std::cout << "th2poly: ptr=" << tp1 << ", integral=" << tp1->Integral() << std::endl;

  TH2D *hUZProjection =  myUVWProjector.GetStripVsTime_TH2D(DIR_U);
  TH2D *hVZProjection =  myUVWProjector.GetStripVsTime_TH2D(DIR_V);
  TH2D *hWZProjection =  myUVWProjector.GetStripVsTime_TH2D(DIR_W);

  hUZProjection->Print();
  hVZProjection->Print();
  hWZProjection->Print();

  hUZProjection->Draw("colz");
  c1->Print("UZProjection.png");

  hVZProjection->Draw("colz");
  c1->Print("VZProjection.png");

  hWZProjection->Draw("colz");
  c1->Print("WVProjection.png");

  EventTPC evt;
  evt.SetGeoPtr(myGeometryPtr);

  int strip_number, time_cell;
  double value;
  for(int iBinX=1;iBinX<hUZProjection->GetNbinsX();++iBinX){
    for(int iBinY=1;iBinY<hUZProjection->GetNbinsY();++iBinY){
      time_cell = iBinX;
      strip_number = iBinY;
      value = hUZProjection->GetBinContent(iBinX, iBinY);
      evt.AddValByStrip(DIR_U, strip_number, time_cell, value);

      value = hVZProjection->GetBinContent(iBinX, iBinY);
      evt.AddValByStrip(DIR_V, strip_number, time_cell, value);

      value = hWZProjection->GetBinContent(iBinX, iBinY);
      evt.AddValByStrip(DIR_W, strip_number, time_cell, value);
    }
  }

  evt.GetStripVsTime(DIR_U)->Draw("colz");
  c1->Print("UZProjection_fromEventTPC.png");

  TFile aFile("EventTPC.root","RECREATE");
  TTree aTree("TPCData","");
  EventTPC *persistent_event = &evt;
  evt.SetGeoPtr(0);
  aTree.Branch("Event", &persistent_event);

  for(int i=0;i<1000;++i) aTree.Fill();
  aTree.Write();
  aFile.Close();


  TFile aFile2("EventTPC.root","READ");
  TTree * aTree2 = (TTree*)aFile2.Get("TPCData");
  persistent_event = 0;
  aTree2->SetBranchAddress("Event", &persistent_event);
  aTree2->GetEntry(0);
  persistent_event->SetGeoPtr(myGeometryPtr);
  persistent_event->GetStripVsTime(DIR_U)->Draw("colz");
  c1->Print("UZProjection_fromPersistentEventTPC.png");

  return 0;
}
