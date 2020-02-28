#include <cstdlib>
#include <iostream>
#include <vector>

#include <TROOT.h>
#include <TApplication.h>
#include <MainFrame.h>
#include <TH1F.h>

#include <boost/property_tree/json_parser.hpp>


int main(int argc, char **argv){


  boost::property_tree::ptree root;
  if(argc<1){
    std::cout<<" Usage: tpcGUI config.json"<<std::endl;
    return 0;
  }
  else {
    std::cout<<"Using configFileName = "<<argv[1]<<std::endl;
    boost::property_tree::read_json(argv[1], root);
  }
  ROOT::EnableThreadSafety();
  TApplication theApp("App", &argc, argv);
  MainFrame mainWindow(gClient->GetRoot(),0, 0,root);
  theApp.Run();

  return 0;
}
