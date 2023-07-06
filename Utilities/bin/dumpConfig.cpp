#include "TPCReco/ConfigManager.h"
#include "TPCReco/colorText.h"

//////////////////////////
//////////////////////////
int main(int argc, char **argv){

  ConfigManager cm;
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);
  double energy = myConfig.get<double>("beamParameters.energy");
  std::cout<<KBLU<<"Beam energy is: "<<RST<<energy<<std::endl;
  cm.dumpConfig(myConfig.get<std::string>("meta.configDumpJson"));

  return 0;

}
//////////////////////////
//////////////////////////
