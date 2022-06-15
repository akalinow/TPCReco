#include <iostream>
#include <string>
#include <memory>

#include "GeometryTPC.h"
#include "EventTPC.h"
#include "PEventTPC.h"
#include "EventSourceMultiGRAW.h"

#include "TFile.h"

#include "colorText.h"

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
  for(int i=0;i<100;++i){
    myEventSource->loadFileEntry(i);
  
    std::cout<<myEventPtr->GetEventInfo()<<std::endl;

    myEventPtr->get1DProjection(projection_type::DIR_U, filter_type::none, scale_type::raw)->Print();
    myEventPtr->get1DProjection(projection_type::DIR_V, filter_type::none, scale_type::raw)->Print();
    myEventPtr->get1DProjection(projection_type::DIR_W, filter_type::none, scale_type::raw)->Print();
    std::cout<<std::endl;
    myEventPtr->get1DProjection(projection_type::DIR_U, filter_type::none, scale_type::mm)->Print();
    myEventPtr->get1DProjection(projection_type::DIR_V, filter_type::none, scale_type::mm)->Print();
    myEventPtr->get1DProjection(projection_type::DIR_W, filter_type::none, scale_type::mm)->Print();
    std::cout<<std::endl;
    myEventPtr->get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::mm)->Print();
    myEventPtr->get1DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::mm)->Print();
    myEventPtr->get1DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::mm)->Print();
    myEventPtr->get1DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::mm)->Print();
    std::cout<<std::endl;
    myEventPtr->get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::raw)->Print();
    myEventPtr->get1DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::raw)->Print();
    myEventPtr->get1DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::raw)->Print();
    myEventPtr->get1DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::raw)->Print();
  }

  return 0;
}
/////////////////////////////////////
/////////////////////////////////////
