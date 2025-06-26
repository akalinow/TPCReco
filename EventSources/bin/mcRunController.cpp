#include <cstdlib>
#include <iostream>
#include <vector>
#include <ctime>

#include <TStopwatch.h>

#include <boost/program_options.hpp>

#include "TPCReco/EventSourceFactory.h"

#include "TPCReco/ConfigManager.h"
#include "TPCReco/colorText.h"
/////////////////////////////////////
/////////////////////////////////////
int processEvents(boost::property_tree::ptree & aConfig) {
		  
  std::shared_ptr<EventSourceBase> eventSource = EventSourceFactory::makeEventSourceObject(aConfig);
  auto myEventSource = std::dynamic_pointer_cast<EventSourceMC>(eventSource);
  if(!myEventSource){
    std::cout<<KRED<<"Wrong event source type!"<<RST<<std::endl;
    exit(1);
  }

  //Event loop
  int nEntries = aConfig.get<int>("input.readNEvents");
  if(nEntries<0 ) nEntries = 0;

  for(int iEntry=0;iEntry<nEntries;++iEntry){
    if(nEntries>10 && iEntry%(nEntries/10)==0){
      std::cout<<KBLU<<"Processed: "<<int(100*(double)iEntry/nEntries)<<" % events"<<RST<<std::endl;
    }
    myEventSource->loadFileEntry(iEntry);
  }   

  return nEntries;
}
/////////////////////////////////////
/////////////////////////////////////
int main(int argc, char **argv){

  TStopwatch aStopwatch;
  aStopwatch.Start();

  ConfigManager cm;
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);
 
  int nEntriesProcessed = processEvents(myConfig);
 
  aStopwatch.Stop();
  std::cout<<KBLU<<"Real time:       "<<RST<<aStopwatch.RealTime()<<" s"<<std::endl;
  std::cout<<KBLU<<"CPU time:        "<<RST<<aStopwatch.CpuTime()<<" s"<<std::endl;
  std::cout<<KBLU<<"Processing rate: "<<RST<<nEntriesProcessed/aStopwatch.RealTime()<< " ev/s"<<std::endl;

  return 0;
}


