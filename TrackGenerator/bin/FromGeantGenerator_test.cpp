#include <string>
#include <iostream>
#include <TCanvas.h>
#include "FromGeantGenerator.h"
#include <TTree.h>
#include <TFile.h>
#include <TMath.h>
#include <boost/property_tree/json_parser.hpp>
namespace pt = boost::property_tree;
/*
*This tests checks the FromGeantGenerator class
*From a set line shape a TPCEvent is created
*In intermediate steps a series of .pdfs is created for consitency checking
*.root tree is created and saved, which can later be used as input for gui application*/
int main(int argc,char *argv[]){
pt::ptree root;

FromGeantGenerator l;
if(argc==2){
  pt::read_json(argv[1], root);
}
else{
  std::cout<< "Usage:\n\tEventGenerator <config.json>" <<std::endl;
  return 1;
}

l.loadGeometry(root.get<std::string>("geometryFile"));
l.loadDataFile(root.get<std::string>("dataFile"));
l.setTrackSpace(root.get<int>("NbinsX"),root.get<double>("xmin"),root.get<double>("xmax"),
                root.get<int>("NbinsY"),root.get<double>("ymin"),root.get<double>("ymax"),
                root.get<int>("NbinsZ"),root.get<double>("zmin"),root.get<double>("zmax")); 
l.setEntry(1);
EventTPC myEvent=l.generateEvent();

TCanvas *c1 = new TCanvas("c1","c", 2, 78, 500, 500);
//track check            
l.getTrack().Draw("colz");
std::string str =std::to_string(0)+".pdf";
c1->Print(str.c_str());
//projections checks
for (auto i: l.getProjections()){
        i->Draw("COLZ");
        std::string str =std::to_string(1)+".pdf";
        c1->Print(str.c_str());
}
      
//event check
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
  myEvent.SetGeoPtr(0);
  aTree.Fill();
  aTree.Write();
  aFile.Close();
return 0;
}