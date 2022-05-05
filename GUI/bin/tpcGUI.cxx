#include <cstdlib>
#include <iostream>
#include <vector>

#include <TROOT.h>
#include <TApplication.h>
#include <MainFrame.h>
#include <TH1F.h>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/program_options.hpp>

#include "colorText.h"

boost::program_options::variables_map parseCmdLineArgs(int argc, char **argv){

  boost::program_options::options_description cmdLineOptDesc("Allowed options");
  cmdLineOptDesc.add_options()
    ("help", "produce help message")
    ("dataFile",  boost::program_options::value<std::string>(), "string - path to data file (OFFLINE) or directory (ONLINE). Overrides the value from the config file. In multi-GRAW mode specify several files separated by commas.")
    ("singleAsadGrawFile", boost::program_options::value<bool>(), "bool - Flag enabling multi-GRAW mode. One file stream per each AsAd board.")
    ("removePedestal",  boost::program_options::value<bool>(), "bool - Flag to control pedestal removal. Overrides the value from config file.")
    ("recoClusterEnable",  boost::program_options::value<bool>(), "bool - Flag to enable RECO cluster.")
    ("recoClusterThreshold",  boost::program_options::value<double>(), "double - ADC threshold above pedestal for RECO cluster.")
    ("recoClusterDeltaStrips",  boost::program_options::value<int>(), "int - Envelope in strip units around seed hits for RECO cluster.")
    ("recoClusterDeltaTimeCells",  boost::program_options::value<int>(), "int - Envelope in time cell units around seed hits for RECO cluster.");
    
  boost::program_options::variables_map varMap;        
  boost::program_options::store(boost::program_options::parse_command_line(argc, argv, cmdLineOptDesc), varMap);
  boost::program_options::notify(varMap); 

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
    tree.put("recoClusterEnable", varMap["recoClusterEnable"].as<bool>());
  }
  if(varMap.count("recoClusterThreshold")){
    tree.put("recoClusterThreshold", varMap["recoClusterThreshold"].as<double>());
  }
  if(varMap.count("recoClusterDeltaStrips")){
    tree.put("recoClusterDeltaStrips", varMap["recoClusterDeltaStrips"].as<int>());
  }
  if(varMap.count("recoClusterDeltaTimeCells")){
    tree.put("recoClusterDeltaTimeCells", varMap["recoClusterDeltaTimeCells"].as<int>());
  }
  
  ROOT::EnableThreadSafety();
  TApplication theApp("App", &argc, argv);
  MainFrame mainWindow(gClient->GetRoot(),0, 0, tree);
  theApp.Run();

  return 0;
}
