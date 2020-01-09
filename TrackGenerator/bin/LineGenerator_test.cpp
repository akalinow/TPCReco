#include <iostream>
#include <TCanvas.h>
#include "LineGenerator.h"
#include <TMath.h>
#include <boost/property_tree/json_parser.hpp>
#include <boost/foreach.hpp>
namespace pt = boost::property_tree;

template <typename T>
T jsonGet(const pt::ptree::value_type& child,std::string str){
  boost::optional<T> v=child.second.get_optional<T>(str);
  return v.is_initialized() ? v.get() : static_cast<T>(0);
  }

/*
*This test checks the LineGenerator class
*From a set line shape a TPCEvent is created
*In intermediate steps a series of .pdfs is created for consitency checking
*.root tree is created and saved, which can later be used as input for gui application*/
int main(int argc,char *argv[]){
pt::ptree root;


if(argc==2){
  //l.loadGeometry(argv[1]);
  pt::read_json(argv[1], root);
}
else{
  std::cout<< "Usage:\n\tEventGenerator <config.json>" <<std::endl;
  return 1;
}

LineGenerator l;
//l.setOutput(root.get<std::string>( "outputFile"));
l.loadGeometry(root.get<std::string>("geometryFile"));
BOOST_FOREACH(const pt::ptree::value_type& child,root.get_child("lines")) {
  auto x0 =jsonGet<double>(child,"x0");
  auto y0 =jsonGet<double>(child,"y0");
  auto z0 =jsonGet<double>(child,"z0");
  auto length =jsonGet<double>(child,"length");
  auto theta =jsonGet<double>(child,"theta");
  auto phi =jsonGet<double>(child,"phi");
  auto randomLength =jsonGet<bool>(child,"lengthRandom");
  auto randomTheta =jsonGet<bool>(child,"thetaRandom");
  auto randomPhi =jsonGet<bool>(child,"phiRandom");
  l.addLine(length,theta,phi,TVector3(x0,y0,z0),randomTheta,randomPhi,randomLength);
}

//l.addLine(60,0,0,TVector3(0,0,0),false,false,false);
TCanvas *c1 = new TCanvas("c1","c", 2, 78, 500, 500);
l.setTrackCounts(root.get<double>("MCcounts"));
l.setTrackSigma(root.get<double>("sigma"));
l.setTrackSpace(root.get<int>("NbinsX"),root.get<double>("xmin"),root.get<double>("xmax"),
                root.get<int>("NbinsY"),root.get<double>("ymin"),root.get<double>("ymax"),
                root.get<int>("NbinsZ"),root.get<double>("zmin"),root.get<double>("zmax"));
EventTPC myEvent=l.generateEvent();
//l.closeOutput();
  myEvent.SetEventId(1);
  //std::cout<<l.getProjections().size()<<std::endl;

  
for (auto i: l.getProjections()){
        //i->Print();
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
  myEvent.SetGeoPtr(0);
  aTree.Fill();
  aTree.Write();
  aFile.Close();
return 0;
}