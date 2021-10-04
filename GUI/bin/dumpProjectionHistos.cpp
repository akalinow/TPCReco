
#include "TFile.h"

#include "HistoManager.h"
#include "EventSourceROOT.h"

#include <boost/property_tree/json_parser.hpp>

int main(int argc, char *argv[]) {

  std::string dataFileName = "/data/akalinow/data/neutrons/ROOT/EventTPC_2018-06-19T15:13:33.941_0008.root";
  std::string geometryFileName = "/home/akalinow/scratch/data/neutrons/geometry_mini_eTPC_2018-06-19T10:35:30.853.dat";
  std::string rootFileName = "ProjectionHistos_2018-06-19T15:13:33.941_0008.root";

  std::shared_ptr<EventSourceBase> myEventSource;
  if(dataFileName.find(".root")!=std::string::npos){
    myEventSource = std::make_shared<EventSourceROOT>();
    myEventSource->loadGeometry(geometryFileName);
    myEventSource->loadDataFile(dataFileName);
    std::cout<<"File with "<<myEventSource->numberOfEntries()<<" frames loaded."<<std::endl;
  }
  else{
    std::cout<<"Wrong input file: "<<dataFileName<<std::endl;
    return -1;
  }
  HistoManager myHistoManager;
  myHistoManager.setGeometry(myEventSource->getGeometry());

  TFile aFile(rootFileName.c_str(),"RECREATE");

  for(unsigned int iEntry=0;iEntry<myEventSource->numberOfEntries();++iEntry){
    myEventSource->loadFileEntry(iEntry);
    std::cout<<"EventID: "<<myEventSource->currentEventNumber()<<std::endl;
    myHistoManager.setEvent(myEventSource->getCurrentEvent());
    for(int strip_dir=0;strip_dir<3;++strip_dir){
      myHistoManager.getRawStripVsTime(strip_dir)->Write();
    }
  }

  aFile.Close();

  return 0;
}
