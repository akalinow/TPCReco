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

#include "TPCReco/colorText.h"
#include "TPCReco/ConfigManager.h"


int main(int argc, char** argv) {

	ConfigManager cm;
	boost::property_tree::ptree tree = cm.getConfig(argc,argv);

	ROOT::EnableThreadSafety();
	TApplication theApp("App", &argc, argv);
	MainFrame mainWindow(gClient->GetRoot(), 0, 0, tree);
	theApp.Run();

	return 0;
}
