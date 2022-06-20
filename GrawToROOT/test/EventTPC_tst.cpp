#include <iostream>
#include <string>
#include <memory>

#include "GeometryTPC.h"
#include "EventTPC.h"
#include "PEventTPC.h"
#include "EventSourceMultiGRAW.h"

#include "TFile.h"

#include "colorText.h"

void testHits(std::shared_ptr<EventTPC> aEventPtr, filter_type filterType){
  
  std::cout<<KBLU<<"1D projection on strips: U, V, W [raw]"<<RST<<std::endl;
  aEventPtr->get1DProjection(projection_type::DIR_U, filterType, scale_type::raw)->Print();
  aEventPtr->get1DProjection(projection_type::DIR_V, filterType, scale_type::raw)->Print();
  aEventPtr->get1DProjection(projection_type::DIR_W, filterType, scale_type::raw)->Print();
  std::cout<<KBLU<<"1D projection on strips U, V, W [mm]"<<RST<<std::endl;
  aEventPtr->get1DProjection(projection_type::DIR_U, filterType, scale_type::mm)->Print();
  aEventPtr->get1DProjection(projection_type::DIR_V, filterType, scale_type::mm)->Print();
  aEventPtr->get1DProjection(projection_type::DIR_W, filterType, scale_type::mm)->Print();   
  std::cout<<KBLU<<"1D projection on time : global, U, V, W [raw]"<<RST<<std::endl;
  aEventPtr->get1DProjection(projection_type::DIR_TIME, filterType, scale_type::raw)->Print();
  aEventPtr->get1DProjection(projection_type::DIR_TIME_U, filterType, scale_type::raw)->Print();
  aEventPtr->get1DProjection(projection_type::DIR_TIME_V, filterType, scale_type::raw)->Print();
  aEventPtr->get1DProjection(projection_type::DIR_TIME_W, filterType, scale_type::raw)->Print();
  std::cout<<KBLU<<"1D projection on time : global, U, V, W [mm]"<<RST<<std::endl;
  aEventPtr->get1DProjection(projection_type::DIR_TIME, filterType, scale_type::mm)->Print();
  aEventPtr->get1DProjection(projection_type::DIR_TIME_U, filterType, scale_type::mm)->Print();
  aEventPtr->get1DProjection(projection_type::DIR_TIME_V, filterType, scale_type::mm)->Print();
  aEventPtr->get1DProjection(projection_type::DIR_TIME_W, filterType, scale_type::mm)->Print();
  std::cout<<KBLU<<"2D projection time vs strips: U, V, W [raw]"<<RST<<std::endl;
  aEventPtr->get2DProjection(projection_type::DIR_TIME_U, filterType, scale_type::raw)->Print();
  aEventPtr->get2DProjection(projection_type::DIR_TIME_V, filterType, scale_type::raw)->Print();
  aEventPtr->get2DProjection(projection_type::DIR_TIME_W, filterType, scale_type::raw)->Print();
  std::cout<<KBLU<<"2D projection on time : global, U, V, W [mm]"<<RST<<std::endl;
  aEventPtr->get2DProjection(projection_type::DIR_TIME_U, filterType, scale_type::mm)->Print();
  aEventPtr->get2DProjection(projection_type::DIR_TIME_V, filterType, scale_type::mm)->Print();
  aEventPtr->get2DProjection(projection_type::DIR_TIME_W, filterType, scale_type::mm)->Print();

  if(filterType==filter_type::none){
    std::cout<<KBLU<<"2D projection on time cells vs AGET channels"<<RST<<std::endl;
    aEventPtr->GetChannels(0,0)->Print();
    aEventPtr->GetChannels_raw(0,0)->Print();
  }
}
/////////////////////////////////////
/////////////////////////////////////
int main(int argc, char *argv[]) {

  std::string geometryFileName = "geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat";
  std::string dataFileName = "/scratch_cmsse/akalinow/ELITPC/data/HIgS_2022/20220412_extTrg_CO2_190mbar_DT1470ET/11.5MeV/GRAW/CoBo0_AsAd0_2022-04-12T08:03:44.531_0000.graw,/scratch_cmsse/akalinow/ELITPC/data/HIgS_2022/20220412_extTrg_CO2_190mbar_DT1470ET/11.5MeV/GRAW/CoBo0_AsAd1_2022-04-12T08:03:44.533_0000.graw,/scratch_cmsse/akalinow/ELITPC/data/HIgS_2022/20220412_extTrg_CO2_190mbar_DT1470ET/11.5MeV/GRAW/CoBo0_AsAd2_2022-04-12T08:03:44.536_0000.graw,/scratch_cmsse/akalinow/ELITPC/data/HIgS_2022/20220412_extTrg_CO2_190mbar_DT1470ET/11.5MeV/GRAW/CoBo0_AsAd3_2022-04-12T08:03:44.540_0000.graw";
  std::string referenceDataFileName = "";

  std::shared_ptr<EventSourceBase> myEventSource = std::make_shared<EventSourceMultiGRAW>(geometryFileName);
  myEventSource->loadDataFile(dataFileName);
  std::cout << "File with " << myEventSource->numberOfEntries() << " frames opened." << std::endl;

  auto myEventPtr = myEventSource->getCurrentEvent();
  for(int i=0;i<1;++i){
    myEventSource->loadFileEntry(i);
  
    std::cout<<myEventPtr->GetEventInfo()<<std::endl;
    testHits(myEventPtr, filter_type::none);
    //testHits(myEventPtr, filter_type::threshold);    
  }

  return 0;
}
/////////////////////////////////////
/////////////////////////////////////
