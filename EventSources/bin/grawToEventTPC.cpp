#include "TPCReco/grawToEventTPC.h"

#include "TPCReco/ConfigManager.h"


int main(int argc, char *argv[]) {

  ConfigManager cm;
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);

  convertGRAWFile(myConfig);
  return 0;
}
