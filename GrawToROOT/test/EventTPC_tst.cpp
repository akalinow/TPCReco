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

int testHits(std::shared_ptr<EventTPC> aEventPtr, filter_type filterType, std::vector<double> comparision){
  
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
  std::vector<bool> error_list_bool;
  std::vector<std::string> error_list = {"charge", "charge_DIR_U", "charge_DIR_U_strip1", "charge_DIR_U_sec1_strip_58", "charge_time_cell128", "charge_DIR_U_time_cell128", "charge_DIR_U_sec1_time_cell28", "max_charge", "max_charge_DIR_U", "max_charge_DIR_U_strip1", "max_charge_DIR_U_sec1_strip58", "max_charge_time", "max_charge_channel", "max_charge_time_DIR_U", "max_charge_strip_DIR_U", "min_time", "max_time", "min_time_DIR_U", "max_time_DIR_U", "min_strip_DIR_U", "max_strip_DIR_U", "multiplicity_total", "multiplicity_DIR_U", "multiplicity_DIR_V", "multiplicity_DIR_W", "multiplicity_DIR_U_0", "multiplicity_DIR_V_0", "multiplicity_DIR_W_0", "Nhits_total", "Nhits_DIR_U", "Nhits_DIR_V", "Nhits_DIR_W", "Nhits_DIR_U_0", "Nhits_DIR_U_0_70", "NhitsMerged_DIR_U_70"};
  std::cout << std::boolalpha;
  double epsilon = 1e-5;
  bool charge = (aEventPtr->GetTotalCharge(-1, -1, -1, -1, filterType) - comparision[0]) < epsilon;                             
  bool charge_DIR_U = (aEventPtr->GetTotalCharge(DIR_U, -1, -1, -1, filterType) - comparision[1]) < epsilon;                    
  bool charge_DIR_U_strip1 = (aEventPtr->GetTotalCharge(DIR_U, -1, 1, -1, filterType) - comparision[2]) < epsilon;              
  bool charge_DIR_U_sec1_strip_58 = (aEventPtr->GetTotalCharge(DIR_U, 1, 58, -1, filterType) - comparision[3]) < epsilon;       
  bool charge_time_cell128 = (aEventPtr->GetTotalCharge(-1, -1, -1, 128, filterType) - comparision[4]) < epsilon;               
  bool charge_DIR_U_time_cell128 = (aEventPtr->GetTotalCharge(DIR_U, -1, -1, 128, filterType) - comparision[5]) < epsilon;      
  bool charge_DIR_U_sec1_time_cell28 = (aEventPtr->GetTotalCharge(DIR_U, 1,  -1, 128, filterType) - comparision[6]) < epsilon;  
  error_list_bool.push_back(charge);
  error_list_bool.push_back(charge_DIR_U);
  error_list_bool.push_back(charge_DIR_U_strip1);
  error_list_bool.push_back(charge_DIR_U_sec1_strip_58);
  error_list_bool.push_back(charge_time_cell128);
  error_list_bool.push_back(charge_DIR_U_time_cell128);
  error_list_bool.push_back(charge_DIR_U_sec1_time_cell28);

  bool max_charge = (aEventPtr->GetMaxCharge(-1,-1,-1,filterType) - comparision[7]) < epsilon;                          
  bool max_charge_DIR_U = (aEventPtr->GetMaxCharge(DIR_U,-1,-1,filterType) - comparision[8]) < epsilon;                 
  bool max_charge_DIR_U_strip1 = (aEventPtr->GetMaxCharge(DIR_U, -1, 1, filterType) - comparision[9]) < epsilon;        
  bool max_charge_DIR_U_sec1_strip58 = (aEventPtr->GetMaxCharge(DIR_U, 1, 58,filterType) - comparision[10]) < epsilon;  
  error_list_bool.push_back(max_charge);
  error_list_bool.push_back(max_charge_DIR_U);
  error_list_bool.push_back(max_charge_DIR_U_strip1);
  error_list_bool.push_back(max_charge_DIR_U_sec1_strip58);

  int maxTime = 0, maxStrip = 0;
  std::tie(maxTime, maxStrip) = aEventPtr->GetMaxChargePos(-1,filterType);
  bool max_charge_time = maxTime - comparision[11] == 0;  
  bool max_charge_channel = 0 - comparision[12] == 0;     
  error_list_bool.push_back(max_charge_time);
  error_list_bool.push_back(max_charge_channel);
  std::tie(maxTime, maxStrip) = aEventPtr->GetMaxChargePos(DIR_U, filterType);
  bool max_charge_time_DIR_U = maxTime - comparision[13] == 0;  
  bool max_charge_strip_DIR_U = maxStrip - comparision[14] == 0;
  error_list_bool.push_back(max_charge_time_DIR_U);
  error_list_bool.push_back(max_charge_strip_DIR_U);

  int minTime=0, minStrip=0;
  std::tie(minTime, maxTime, minStrip, maxStrip) = aEventPtr->GetSignalRange(-1, filterType);
  bool min_time = minTime - comparision[15] == 0;
  bool max_time = maxTime - comparision[16] == 0;
  error_list_bool.push_back(min_time);
  error_list_bool.push_back(max_time);

  std::tie(minTime, maxTime, minStrip, maxStrip) = aEventPtr->GetSignalRange(DIR_U, filterType);
  bool min_time_DIR_U = minTime - comparision[17] == 0;    
  bool max_time_DIR_U = maxTime - comparision[18] == 0;    
  bool min_strip_DIR_U = minStrip - comparision[19] == 0;  
  bool max_strip_DIR_U = maxStrip - comparision[20] == 0;
  error_list_bool.push_back(min_time_DIR_U);
  error_list_bool.push_back(max_time_DIR_U);
  error_list_bool.push_back(min_strip_DIR_U);
  error_list_bool.push_back(max_strip_DIR_U);
  
  bool multiplicity_total = aEventPtr->GetMultiplicity(false, -1, -1, -1, filterType) - comparision[21] == 0;     
  bool multiplicity_DIR_U = aEventPtr->GetMultiplicity(false, DIR_U, -1, -1, filterType) - comparision[22] == 0;  
  bool multiplicity_DIR_V = aEventPtr->GetMultiplicity(false, DIR_V, -1, -1, filterType) - comparision[23] == 0;  
  bool multiplicity_DIR_W = aEventPtr->GetMultiplicity(false, DIR_W, -1, -1, filterType) - comparision[24] == 0;
  error_list_bool.push_back(multiplicity_total);
  error_list_bool.push_back(multiplicity_DIR_U);
  error_list_bool.push_back(multiplicity_DIR_V);
  error_list_bool.push_back(multiplicity_DIR_W);
  
  bool multiplicity_DIR_U_0 = aEventPtr->GetMultiplicity(false, DIR_U, 0, -1, filterType) - comparision[25] == 0;
  bool multiplicity_DIR_V_0 = aEventPtr->GetMultiplicity(false, DIR_V, 0, -1, filterType) - comparision[26] == 0;
  bool multiplicity_DIR_W_0 = aEventPtr->GetMultiplicity(false, DIR_W, 0, -1, filterType) - comparision[27] == 0;
  error_list_bool.push_back(multiplicity_DIR_U_0);
  error_list_bool.push_back(multiplicity_DIR_V_0);
  error_list_bool.push_back(multiplicity_DIR_W_0);

  bool Nhits_total = aEventPtr->GetMultiplicity(true, -1, -1, -1, filterType) - comparision[28] == 0;             
  bool Nhits_DIR_U = aEventPtr->GetMultiplicity(true, DIR_U, -1, -1, filterType) - comparision[29] == 0;          
  bool Nhits_DIR_V = aEventPtr->GetMultiplicity(true, DIR_V, -1, -1, filterType) - comparision[30] == 0;          
  bool Nhits_DIR_W = aEventPtr->GetMultiplicity(true, DIR_W, -1, -1, filterType) - comparision[31] == 0;          
  bool Nhits_DIR_U_0 = aEventPtr->GetMultiplicity(true, DIR_U, 0, -1, filterType) - comparision[32] == 0;         
  bool Nhits_DIR_U_0_70 = aEventPtr->GetMultiplicity(true, DIR_U, 0, 70, filterType) - comparision[33] == 0;      
  bool NhitsMerged_DIR_U_70 = aEventPtr->GetMultiplicity(true, DIR_U, -1, 70, filterType) - comparision[34] == 0; 
  error_list_bool.push_back(Nhits_total);
  error_list_bool.push_back(Nhits_DIR_U);
  error_list_bool.push_back(Nhits_DIR_V);
  error_list_bool.push_back(Nhits_DIR_W);
  error_list_bool.push_back(Nhits_DIR_U_0);
  error_list_bool.push_back(Nhits_DIR_U_0_70);
  error_list_bool.push_back(NhitsMerged_DIR_U_70);

  int check = error_list_bool.size();
  for(std::vector<bool>::iterator it = error_list_bool.begin(); it != error_list_bool.end(); ++it)
    check -= *it;
  
  if(check > 0){
    for(std::vector<std::string>::size_type i = 0; i != error_list.size(); i++) {
      if(error_list_bool[i]==0){
        std::cout<<KRED<<error_list[i]<<RST<<std::endl;
      }
    }
  }
  
  return check;
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

  std::vector<double> comparison_none = { 520098.542683 , 155908.768293 , 1407.329268293 , 367.042682927 , 2890.25609756 , 950.109756098 , -79.378 , 1735.09146341 , 1392.21341463 , 23.7256 , 22.2561 , 58 , 0 ,  58 , 65 , 2 , 500 , 3 , 501 , 1 , 132 , 1018 , 131 , 224 , 225 , 0 , 74 , 74 , 290917 , 65868  , 112275  , 112774  , 0 , 0 , 499};

  std::vector<double> comparison_threshold = { 794935.22561 , 226979.29878 , 0 , 0 , 3425.77439024 , 1032.90243902 , 0 , 1732.16463415 , 1385.27439024 , 0 , 0 , 58 , 0 , 58 , 65 , 33 , 251 , 37 , 250 , 46 , 81 , 112 , 25 , 43 , 29 , 0 , 28 , 15 , 5874 , 1747 , 2266 , 1861 , 0 , 0 , 104};
  int check=0;
  auto myEventPtr = myEventSource->getCurrentEvent();
  for(int i=89;i<90;++i){
    myEventSource->loadFileEntry(i);
  
    std::cout<<myEventPtr->GetEventInfo()<<std::endl;
    check += testHits(myEventPtr, filter_type::none , comparison_none);
    check += testHits(myEventPtr, filter_type::threshold , comparison_threshold);
  }
  //std::cout<<"check"<<a+b<<std::endl;
  ///This part to be moved to GeometryTPC_tst.cpp
  //std::cout<<KBLU<<"Strip direction has reversed strip numbering wrt. cartesian coordinates: "<<RST<<std::endl;
  //std::cout<<KBLU<<"U: "<<RST<<myEventPtr->GetGeoPtr()->IsStripDirReversed(projection_type::DIR_U)<<std::endl;
  //std::cout<<KBLU<<"V: "<<RST<<myEventPtr->GetGeoPtr()->IsStripDirReversed(projection_type::DIR_V)<<std::endl;
  //std::cout<<KBLU<<"W: "<<RST<<myEventPtr->GetGeoPtr()->IsStripDirReversed(projection_type::DIR_W)<<std::endl;
  /////

  if(check>0){return -1;}
  return 0;
}
/////////////////////////////////////
/////////////////////////////////////
