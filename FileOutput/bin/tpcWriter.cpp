#include <cstdlib>
#include <iostream>

#include "TPCReco/ConfigManager.h"
#include "TPCReco/EventSourceFactory.h"
#include "TPCReco/FileOutput.h"
#include "TPCReco/TrackBuilder.h"
#include "TPCReco/colorText.h"

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
int main(int argc, char** argv) {

	ConfigManager cm;
	boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);

	std::shared_ptr<EventSourceBase> myEventSource = EventSourceFactory::makeEventSourceObject(myConfig);

	double pressure = myConfig.get<double>("conditions.pressure"); 
  	double temperature = myConfig.get<double>("conditions.temperature");

	TrackBuilder myTkBuilder;
	myTkBuilder.setGeometry(myEventSource->getGeometry());
	myTkBuilder.setPressure(pressure);
	FileOutput myFileOutput(myConfig);
	//Event loop
	int nEntries = myConfig.get<int>("input.readNEvents");
	if(nEntries<0) nEntries = 0;

	for(int iEntry=0;iEntry<nEntries;++iEntry){
		if(nEntries>10 && iEntry%(nEntries/10)==0){
		  std::cout<<KBLU<<"Processed: "<<int(100*(double)iEntry/nEntries)<<" % events"<<RST<<std::endl;
		}
		myEventSource->loadFileEntry(iEntry);

		if(myEventSource->getEventFilter().isEnabled() &&
		!myEventSource->getEventFilter().pass(*myEventSource)) continue; // skip this event

		myTkBuilder.setEvent(myEventSource->getCurrentEvent());
		myTkBuilder.reconstruct();
		myEventSource->setRecoEvent(myTkBuilder.getTrack3D(0));

		///check filter again, when the reco event is available
		if(myEventSource->getEventFilter().isEnabled() &&
		!myEventSource->getEventFilter().pass(*myEventSource)) continue; 

		myFileOutput.update(myEventSource); 
	}

	return 0;
}
