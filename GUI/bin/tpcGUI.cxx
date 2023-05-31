#include <cstdlib>
#include <iostream>
#include <vector>

#include <TROOT.h>
#include <TApplication.h>
#include <TH1F.h>
#include "TPCReco/MainFrame.h"


#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>

<<<<<<< HEAD
#include "colorText.h"
#include "ConfigManager.h"
=======
#include "TPCReco/colorText.h"
>>>>>>> f354324fc0e2a0130807f8471dda39732124fe4f


int main(int argc, char** argv) {

<<<<<<< HEAD
	ConfigManager cm;
	boost::property_tree::ptree tree = cm.getConfig(argc,argv);

	ROOT::EnableThreadSafety();
	TApplication theApp("App", &argc, argv);
	MainFrame mainWindow(gClient->GetRoot(), 0, 0, tree);
	theApp.Run();

	return 0;
=======
  if (varMap.count("help")) {
    std::cout<<cmdLineOptDesc<<std::endl;
    exit(1);
  }
  return varMap;
}

int main(int argc, char **argv){

  boost::program_options::variables_map varMap = parseCmdLineArgs(argc, argv);
  boost::property_tree::ptree tree;
  if(argc<1){
    std::cout<<" Usage: tpcGUI config.json"<<std::endl;
    return 0;
  }
  else {
    std::cout<<"Using configFileName: "<<argv[1]<<std::endl;
    boost::property_tree::read_json(argv[1], tree);
  }
  if (varMap.count("dataFile")) {
    tree.put("dataFile",varMap["dataFile"].as<std::string>());
  }
  if (varMap.count("removePedestal")) {
    tree.put("removePedestal",varMap["removePedestal"].as<bool>());
  }
  if(varMap.count("singleAsadGrawFile")){
    tree.put("singleAsadGrawFile", varMap[""].as<bool>());
  }
  
  if(varMap.count("recoClusterEnable")){
    tree.put("hitFilter.recoClusterEnable", varMap["recoClusterEnable"].as<bool>());
  }
  if(varMap.count("recoClusterThreshold")){
    tree.put("hitFilter.recoClusterThreshold", varMap["recoClusterThreshold"].as<double>());
  }
  if(varMap.count("recoClusterDeltaStrips")){
    tree.put("hitFilter.recoClusterDeltaStrips", varMap["recoClusterDeltaStrips"].as<int>());
  }
  if(varMap.count("recoClusterDeltaTimeCells")){
    tree.put("hitFilter.recoClusterDeltaTimeCells", varMap["recoClusterDeltaTimeCells"].as<int>());
  }

  ROOT::EnableThreadSafety();
  TApplication theApp("App", &argc, argv);
  MainFrame mainWindow(gClient->GetRoot(),0, 0, tree);
  theApp.Run();

  return 0;
>>>>>>> f354324fc0e2a0130807f8471dda39732124fe4f
}
