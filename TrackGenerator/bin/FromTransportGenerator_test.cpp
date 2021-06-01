#include <iostream>
#include <TCanvas.h>
#include "FromTransportGenerator.h"
#include <boost/property_tree/json_parser.hpp>
namespace pt = boost::property_tree;
/*
*This test checks the FromTranposrtGenerator class
*From a set line shape a TPCEvent is created
*In intermediate steps a series of .pdfs is created for consitency checking
*.root tree is created and saved, which can later be used as input for gui application*/
int main(int argc,char *argv[]){
pt::ptree root;

FromTransportGenerator l;
if(argc==2){
  pt::read_json(argv[1], root);
}
else{
  std::cout<< "Usage:\n\tEventGenerator <config.json>" <<std::endl;
  return 1;
}

l.loadGeometry(root.get<std::string>("geometryFile"));
l.loadDataFile(root.get<std::string>("dataFile"));
l.setOutput(root.get<std::string>("outputFile"));
l.setEntry(71);

EventTPC myEvent=l.generateEvent();
l.generateEvents(2);
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


 l.writeOutput();
return 0;
}