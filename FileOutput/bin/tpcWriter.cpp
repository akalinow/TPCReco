#include <cstdlib>
#include <iostream>

#include "TPCReco/ConfigManager.h"
#include "TPCReco/EventSourceFactory.h"
#include "TPCReco/FileOutput.h"

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
int main(int argc, char** argv) {

	ConfigManager cm;
	boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);

	std::shared_ptr<EventSourceBase> myEventSource = EventSourceFactory::makeEventSourceObject(myConfig);

	FileOutput myFileOutput(myConfig);
	
	while(myEventSource->getNextEventLoop()){
		myFileOutput.update(myEventSource); 
	}

	return 0;
}
/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
