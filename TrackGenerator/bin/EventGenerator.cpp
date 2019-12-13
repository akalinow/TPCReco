#include <iostream>
#include "FromTransportGenerator.h"
#include <boost/property_tree/json_parser.hpp>
namespace pt = boost::property_tree;
/*

*/
int main(int argc,char *argv[]){
pt::ptree root;

if(argc==2){
  pt::read_json(argv[1], root);
}
else{
  std::cout<< "Usage:\n\tEventGenerator <config.json>" <<std::endl;
  return 1;
}
FromTransportGenerator l;
l.loadGeometry(root.get<std::string>("geometryFile"));
l.loadDataFile(root.get<std::string>("dataFile"));
l.setOutput(root.get<std::string>("outputFile"));
l.generateEvents(3);
l.writeOutput();
l.closeOutput();
return 0;
}