#include "TPCReco/ConfigManager.h"
#include "TPCReco/colorText.h"
#include <TApplication.h>

//////////////////////////
//////////////////////////
int main(int argc, char **argv){

  ConfigManager cm;
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);
  if(cm.isHelpMode()) return 0; // nothing more to do, exit

  auto dumpJsonName = myConfig.get<std::string>("meta.configDumpJson");
  cm.dumpConfig(dumpJsonName);
  std::cout<<std::endl<<KBLU<<"Configuration tree saved to: "<<RST<<dumpJsonName<<std::endl<<std::endl;

  return 0;

}
//////////////////////////
//////////////////////////
