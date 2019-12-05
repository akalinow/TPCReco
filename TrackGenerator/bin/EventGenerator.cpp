#include <string>
#include <iostream>
#include <TCanvas.h>
#include "LineGenerator.h"
#include <TTree.h>
#include <TFile.h>
#include <TMath.h>
/*
*This tests checks the LineGenerator class
*From a set line shape a TPCEvent is created
*In intermediate steps a series of .pdfs is created for consitency checking
*.root tree is created and saved, which can later be used as input for gui application*/
int main(int argc,char *argv[]){
LineGenerator l;
if(argc==2){
  l.loadGeometry(argv[1]);
}
else{
  std::cout<< "Usage:\n\tEventGenerator <geometry.dat>" <<std::endl;
  return 1;
}
TCanvas *c1 = new TCanvas("c1","c", 2, 78, 500, 500);
l.setTrackCounts(10000);

l.setTrackLength(60);
l.setTrackSigma(0);
l.setTrackTheta(TMath::Pi()/3);
l.setTrackPhi(TMath::Pi()*0);
l.setTrackOrigin(-70,0,-50);

EventTPC myEvent=l.generateLineEvent();
for (auto i: l.getProjections()){
        i->Draw("COLZ");
        std::string str =std::to_string(1)+".pdf";
        c1->Print(str.c_str());
}
        //l.getTrack().Project3D("zx")->Draw("colz");
        l.getTrack().Draw("colz");
        std::string str =std::to_string(0)+".pdf";
        c1->Print(str.c_str());
for (int dir=0;dir!=3;++dir){
        auto h2=myEvent.GetStripVsTime(dir);
        h2->Draw("COLZ");
        std::string str =std::to_string(dir)+std::to_string(dir)+".pdf";
        c1->Print(str.c_str());
}


  std::string rootFileName = "EventTPC_MC.root";
  TFile aFile(rootFileName.c_str(),"RECREATE");
  TTree aTree("TPCData","");
  EventTPC *persistent_event = &myEvent;
  aTree.Branch("Event", &persistent_event);
  myEvent.SetEventId(1);
  myEvent.SetGeoPtr(0);
  aTree.Fill();
  aTree.Write();
  aFile.Close();
return 0;
}