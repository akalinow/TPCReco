#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <iomanip>

#include "GeometryTPC.h"
#include "EventTPC.h"
#include "PEventTPC.h"
#include "EventSourceMultiGRAW.h"

#include "TFile.h"

#include "colorText.h"

void testHits(std::shared_ptr<EventTPC> aEventPtr, filter_type filterType, std::vector<double> comparision){
  
  std::cout<<KBLU<<"1D projection on strips: U, V, W [raw]"<<RST<<std::endl;
  std::cout<<aEventPtr->get1DProjection(projection_type::DIR_V, filterType, scale_type::raw)->GetTitle()<<std::endl;
  std::cout<<aEventPtr->get1DProjection(projection_type::DIR_V, filterType, scale_type::raw)->GetXaxis()->GetTitle()<<std::endl;
  std::cout<<aEventPtr->get1DProjection(projection_type::DIR_V, filterType, scale_type::raw)->GetYaxis()->GetTitle()<<std::endl;

  std::cout<<aEventPtr->get1DProjection(projection_type::DIR_TIME, filterType, scale_type::raw)->GetTitle()<<std::endl;
  std::cout<<aEventPtr->get1DProjection(projection_type::DIR_TIME, filterType, scale_type::raw)->GetXaxis()->GetTitle()<<std::endl;
  std::cout<<aEventPtr->get1DProjection(projection_type::DIR_TIME, filterType, scale_type::raw)->GetYaxis()->GetTitle()<<std::endl;
  
  std::cout<<aEventPtr->get2DProjection(projection_type::DIR_TIME_V, filterType, scale_type::raw)->GetTitle()<<std::endl;
  std::cout<<aEventPtr->get2DProjection(projection_type::DIR_TIME_V, filterType, scale_type::raw)->GetXaxis()->GetTitle()<<std::endl;
  std::cout<<aEventPtr->get2DProjection(projection_type::DIR_TIME_V, filterType, scale_type::raw)->GetYaxis()->GetTitle()<<std::endl;
  
  std::cout<<aEventPtr->get2DProjection(projection_type::DIR_TIME_V, filterType, scale_type::mm)->GetTitle()<<std::endl;
  std::cout<<aEventPtr->get2DProjection(projection_type::DIR_TIME_V, filterType, scale_type::mm)->GetXaxis()->GetTitle()<<std::endl;
  std::cout<<aEventPtr->get2DProjection(projection_type::DIR_TIME_V, filterType, scale_type::mm)->GetYaxis()->GetTitle()<<std::endl;

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
  std::cout << std::boolalpha;
  double epsilon = 1e-5;
  bool charge = (aEventPtr->GetTotalCharge(-1, -1, -1, -1, filterType) - comparision[0]) < epsilon; std::cout << "charge: " << charge << std::endl;
  bool charge_DIR_U = (aEventPtr->GetTotalCharge(DIR_U, -1, -1, -1, filterType) - comparision[1]) < epsilon; std::cout << "charge_DIR_U: " << charge_DIR_U << std::endl;
  bool charge_DIR_U_strip1 = (aEventPtr->GetTotalCharge(DIR_U, -1, 1, -1, filterType) - comparision[2]) < epsilon; std::cout << "charge_DIR_U_strip1: " << charge_DIR_U_strip1 << std::endl;
  bool charge_DIR_U_sec1_strip_58 = (aEventPtr->GetTotalCharge(DIR_U, 1, 58, -1, filterType) - comparision[3]) < epsilon; std::cout << "charge_DIR_U_sec1_strip_58: " << charge_DIR_U_sec1_strip_58 << std::endl;
  bool charge_time_cell128 = (aEventPtr->GetTotalCharge(-1, -1, -1, 128, filterType) - comparision[4]) < epsilon; std::cout << "charge_time_cell128: " << charge_time_cell128 << std::endl;
  bool charge_DIR_U_time_cell128 = (aEventPtr->GetTotalCharge(DIR_U, -1, -1, 128, filterType) - comparision[5]) < epsilon; std::cout << "charge_DIR_U_time_cell128: " << charge_DIR_U_time_cell128 << std::endl;
  bool charge_DIR_U_sec1_time_cell28 = (aEventPtr->GetTotalCharge(DIR_U, 1,  -1, 128, filterType) - comparision[6]) < epsilon; std::cout << "charge_DIR_U_sec1_time_cell28: " << charge_DIR_U_sec1_time_cell28 << std::endl;

  bool max_charge = (aEventPtr->GetMaxCharge(-1,-1,-1,filterType) - comparision[7]) < epsilon; std::cout << "max_charge: " << max_charge << std::endl;
  bool max_charge_DIR_U = (aEventPtr->GetMaxCharge(DIR_U,-1,-1,filterType) - comparision[8]) < epsilon; std::cout << "max_charge_DIR_U: " << max_charge_DIR_U << std::endl;
  bool max_charge_DIR_U_strip1 = (aEventPtr->GetMaxCharge(DIR_U, -1, 1, filterType) - comparision[9]) < epsilon; std::cout << "max_charge_DIR_U_strip1: " << max_charge_DIR_U_strip1 << std::endl;
  bool max_charge_DIR_U_sec1_strip58 = (aEventPtr->GetMaxCharge(DIR_U, 1, 58,filterType) - comparision[10]) < epsilon; std::cout << "max_charge_DIR_U_sec1_strip58: " << max_charge_DIR_U_sec1_strip58 << std::endl;

  int maxTime = 0, maxStrip = 0;
  std::tie(maxTime, maxStrip) = aEventPtr->GetMaxChargePos(-1,filterType);
  bool max_charge_time = maxTime - comparision[11] == 0; std::cout << "max_charge_time: " << max_charge_time << std::endl;
  bool max_charge_channel = 0 - comparision[12] == 0; std::cout << "max_charge_channel: " << max_charge_channel << std::endl;
  std::tie(maxTime, maxStrip) = aEventPtr->GetMaxChargePos(DIR_U, filterType);
  bool max_charge_time_DIR_U = maxTime - comparision[13] == 0; std::cout << "max_charge_time_DIR_U: " << max_charge_time_DIR_U << std::endl;
  bool max_charge_strip_DIR_U = maxStrip - comparision[14] == 0; std::cout << "max_charge_strip_DIR_U: " << max_charge_strip_DIR_U << std::endl;

  int minTime=0, minStrip=0;
  std::tie(minTime, maxTime, minStrip, maxStrip) = aEventPtr->GetSignalRange(-1, filterType);
  bool min_time = minTime - comparision[15] == 0; std::cout << "min_time: " << min_time << std::endl;
  bool max_time = maxTime - comparision[16] == 0; std::cout << "max_time: " << max_time << std::endl;

  std::tie(minTime, maxTime, minStrip, maxStrip) = aEventPtr->GetSignalRange(DIR_U, filterType);
  bool min_time_DIR_U = minTime - comparision[17] == 0; std::cout << "min_time_DIR_U: " << min_time_DIR_U << std::endl;
  bool max_time_DIR_U = maxTime - comparision[18] == 0; std::cout << "max_time_DIR_U: " << max_time_DIR_U << std::endl;
  bool min_strip_DIR_U = minStrip - comparision[19] == 0; std::cout << "min_strip_DIR_U: " << min_strip_DIR_U << std::endl;
  bool max_strip_DIR_U = maxStrip - comparision[20] == 0; std::cout << "max_strip_DIR_U: " << max_strip_DIR_U << std::endl;
  
  bool multiplicity_total = aEventPtr->GetMultiplicity(false, -1, -1, -1, filterType) - comparision[21] == 0; std::cout << "multiplicity_total: " << multiplicity_total << std::endl;
  bool multiplicity_DIR_U = aEventPtr->GetMultiplicity(false, DIR_U, -1, -1, filterType) - comparision[22] == 0; std::cout << "multiplicity_DIR_U: " << multiplicity_DIR_U << std::endl;
  bool multiplicity_DIR_V = aEventPtr->GetMultiplicity(false, DIR_V, -1, -1, filterType) - comparision[23] == 0; std::cout << "multiplicity_DIR_V: " << multiplicity_DIR_V << std::endl;
  bool multiplicity_DIR_W = aEventPtr->GetMultiplicity(false, DIR_W, -1, -1, filterType) - comparision[24] == 0; std::cout << "multiplicity_DIR_W: " << multiplicity_DIR_W << std::endl;
  
  bool multiplicity_DIR_U_0 = aEventPtr->GetMultiplicity(false, DIR_U, 0, -1, filterType) - comparision[25] == 0; std::cout << "multiplicity_DIR_U_0: " << multiplicity_DIR_U_0 << std::endl;
  bool multiplicity_DIR_V_0 = aEventPtr->GetMultiplicity(false, DIR_V, 0, -1, filterType) - comparision[26] == 0; std::cout << "multiplicity_DIR_V_0: " << multiplicity_DIR_V_0 << std::endl;
  bool multiplicity_DIR_W_0 = aEventPtr->GetMultiplicity(false, DIR_W, 0, -1, filterType) - comparision[27] == 0; std::cout << "multiplicity_DIR_W_0: " << multiplicity_DIR_W_0 << std::endl;

  bool Nhits_total = aEventPtr->GetMultiplicity(true, -1, -1, -1, filterType) - comparision[28] == 0; std::cout << "Nhits_total: " << Nhits_total << std::endl;
  bool Nhits_DIR_U = aEventPtr->GetMultiplicity(true, DIR_U, -1, -1, filterType) - comparision[29] == 0; std::cout << "Nhits_DIR_U: " << Nhits_DIR_U << std::endl;
  bool Nhits_DIR_V = aEventPtr->GetMultiplicity(true, DIR_V, -1, -1, filterType) - comparision[30] == 0; std::cout << "Nhits_DIR_V: " << Nhits_DIR_V << std::endl;
  bool Nhits_DIR_W = aEventPtr->GetMultiplicity(true, DIR_W, -1, -1, filterType) - comparision[31] == 0; std::cout << "Nhits_DIR_W: " << Nhits_DIR_W << std::endl;
  bool Nhits_DIR_U_0 = aEventPtr->GetMultiplicity(true, DIR_U, 0, -1, filterType) - comparision[32] == 0; std::cout << "Nhits_DIR_U_0: " << Nhits_DIR_U_0 << std::endl;
  bool Nhits_DIR_U_0_70 = aEventPtr->GetMultiplicity(true, DIR_U, 0, 70, filterType) - comparision[33] == 0; std::cout << "Nhits_DIR_U_0_70: " << Nhits_DIR_U_0_70 << std::endl;
  bool NhitsMerged_DIR_U_70 = aEventPtr->GetMultiplicity(true, DIR_U, -1, 70, filterType) - comparision[34] == 0; std::cout << "NhitsMerged_DIR_U_70: " << NhitsMerged_DIR_U_70 << std::endl;
}
/////////////////////////////////////
/////////////////////////////////////
int main(int argc, char *argv[]) {

  std::string geometryFileName = "geometry_ELITPC_190mbar_3332Vdrift_25MHz.dat";
  std::string dataFileName = "/scratch/ELITPC/data/HIgS_2022/20220412_extTrg_CO2_190mbar_DT1470ET/11.5MeV/GRAW/CoBo0_AsAd0_2022-04-12T08:03:44.531_0000.graw,/scratch/ELITPC/data/HIgS_2022/20220412_extTrg_CO2_190mbar_DT1470ET/11.5MeV/GRAW/CoBo0_AsAd1_2022-04-12T08:03:44.533_0000.graw,/scratch/ELITPC/data/HIgS_2022/20220412_extTrg_CO2_190mbar_DT1470ET/11.5MeV/GRAW/CoBo0_AsAd2_2022-04-12T08:03:44.536_0000.graw,/scratch/ELITPC/data/HIgS_2022/20220412_extTrg_CO2_190mbar_DT1470ET/11.5MeV/GRAW/CoBo0_AsAd3_2022-04-12T08:03:44.540_0000.graw";
  std::string referenceDataFileName = "";

  std::shared_ptr<EventSourceBase> myEventSource = std::make_shared<EventSourceMultiGRAW>(geometryFileName);
  myEventSource->loadDataFile(dataFileName);
  std::cout << "File with " << myEventSource->numberOfEntries() << " frames opened." << std::endl;

  std::vector<double> comparison_none = { 520098.542683 , 155908.768293 , 1407.329268293 , 367.042682927 , 2890.25609756 , 950.109756098 , -79.378 , 1735.09146341 , 1392.21341463 , 23.7256 , 22.2561 , 58 , 0 ,  58 , 65 ,
                                        2 , 500 , 3 , 501 , 1 , 132 , 1018 , 131 , 224 , 225 , 0 , 74 , 74 , 290917 , 65868  , 112275  , 112774  , 0 , 0 , 499};

  std::vector<double> comparison_threshold = { 794935.22561 , 226979.29878 , 0 , 0 , 3425.77439024 , 1032.90243902 , 0 , 1732.16463415 , 1385.27439024 , 0 , 0 , 58 , 0 , 58 , 65 , 33 , 251 , 37 , 250 , 46 , 81 , 112 ,
                                             25 , 43 , 29 , 0 , 28 , 15 , 5874 , 1747 , 2266 , 1861 , 0 , 0 , 104};

  auto myEventPtr = myEventSource->getCurrentEvent();
  for(int i=89;i<90;++i){
    myEventSource->loadFileEntry(i);
  
    std::cout<<myEventPtr->GetEventInfo()<<std::endl;
    testHits(myEventPtr, filter_type::none , comparison_none);
    testHits(myEventPtr, filter_type::threshold , comparison_threshold);
  }

  ///This part to be moved to GeometryTPC_tst.cpp
  std::cout<<KBLU<<"Strip direction has reversed strip numbering wrt. cartesian coordinates: "<<RST<<std::endl;
  std::cout<<KBLU<<"U: "<<RST<<myEventPtr->GetGeoPtr()->IsStripDirReversed(projection_type::DIR_U)<<std::endl;
  std::cout<<KBLU<<"V: "<<RST<<myEventPtr->GetGeoPtr()->IsStripDirReversed(projection_type::DIR_V)<<std::endl;
  std::cout<<KBLU<<"W: "<<RST<<myEventPtr->GetGeoPtr()->IsStripDirReversed(projection_type::DIR_W)<<std::endl;
  /////
  
  return 0;
}
/////////////////////////////////////
/////////////////////////////////////
