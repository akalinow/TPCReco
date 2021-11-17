
#include "TFile.h"

#include "HistoManager.h"
#include "EventSourceROOT.h"

#include <boost/property_tree/json_parser.hpp>

int main(int argc, char *argv[]) {

  for(int chunkId=0;chunkId<2;++chunkId){
    std::string suffix = "_"+std::to_string(chunkId);
    std::string dataFileName = "/scratch/akalinow/ELITPC/data/Krakow_2021/ROOT/EventTPC_2021-09-10T12:08:25.751"+suffix+".root";
    std::string geometryFileName = "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_80mbar_25MHz.dat";
    std::string rootFileName = "ProjectionHistos_2021-09-10T12:08:25.751"+suffix+".root";

    //dataFileName = "/scratch/akalinow/ELITPC/data/IFJ_Neutrons_20210910/ROOT/EventTPC_2021-09-10T12:08:25.751"+suffix+".root";
    //geometryFileName = "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_80mbar_50MHz.dat";
    //rootFileName = "ProjectionHistos_2021-09-10T12:08:25.751"+suffix+".root";

    //dataFileName = "/scratch/akalinow/ELITPC/data/IFJ_VdG_20210630/20210616_extTrg_CO2_250mbar_DT1470ET/EventTPC_2021-06-16T17:46:28.582"+suffix+".root";
    //geometryFileName = "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat";
    //rootFileName = "ProjectionHistos_2021-06-16T17:46:28.582"+suffix+".root";

    dataFileName = "/scratch/akalinow/ELITPC/data/IFJ_VdG_20210630/20210622_extTrg_CO2_250mbar_DT1470ET/CoBo_ALL_AsAd_ALL_2021-06-22T12:01:56.568"+suffix+".graw";
    geometryFileName = "/scratch/akalinow/ELITPC/TPCReco/resources/geometry_ELITPC_250mbar_12.5MHz.dat";
    rootFileName = "ProjectionHistos_2021-06-22T12:01:56.568"+suffix+".root";
    
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
  }

  return 0;
}
