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

int testHits(std::shared_ptr<EventTPC> aEventPtr, filter_type filterType, std::vector<double> comparision1, std::vector<double> comparision2){
  
  std::vector<bool> error_list_bool;
  double epsilon = 1e-5;
  std::cout << std::boolalpha;
  std::string Filter_string;
  std::string Filter;
  if (filterType == filter_type::none) { Filter_string = "raw"; Filter="none";}
  if (filterType == filter_type::threshold) { Filter_string = "Threshold";  Filter="threshold";}

  std::map<std::tuple<projection_type, filter_type, scale_type>, std::vector<std::pair<std::string, std::string>> > Test_GetTitle_1DProjection ={
    {std::make_tuple(projection_type::DIR_V, filterType, scale_type::raw),
    {{"get1DProjection(DIR_V, "+Filter+", raw)->GetTitle()", "Event-89 selected by " + Filter_string + " from V integrated over time"},
    { "get1DProjection(DIR_V, "+Filter+", raw)->GetXaxis()->GetTitle()", "V [strip]"},
    { "get1DProjection(DIR_V, "+Filter+", raw)->GetYaxis()->GetTitle()", "Charge/strip [arb.u.]"}}},
    {std::make_tuple(projection_type::DIR_TIME, filterType, scale_type::raw),
    {{"get1DProjection(DIR_TIME, "+Filter+", raw)->GetTitle()", "Event-89 selected by " + Filter_string + " from time "},
    { "get1DProjection(DIR_TIME, "+Filter+", raw)->GetXaxis()->GetTitle()", "Time [bin]"},
    { "get1DProjection(DIR_TIME, "+Filter+", raw)->GetYaxis()->GetTitle()", "Charge/time bin [arb.u.]"}}}
  };
  for(auto itr=Test_GetTitle_1DProjection.begin(); itr!=Test_GetTitle_1DProjection.end(); itr++){
  std::shared_ptr<TH1D> temp = aEventPtr->get1DProjection(std::get<0>(itr->first) , std::get<1>(itr->first) , std::get<2>(itr->first)); 
  if (bool (std::string(temp->GetTitle()) == itr->second[0].second)) {error_list_bool.push_back(true);} else {
    std::cout << KRED << itr->second[0].first << RST << std::endl; error_list_bool.push_back(false);} 
  if (bool (std::string(temp->GetXaxis()->GetTitle()) == itr->second[1].second)) {error_list_bool.push_back(true);} else {
    std::cout << KRED << itr->second[1].first << RST << std::endl; error_list_bool.push_back(false);} 
  if (bool (std::string(temp->GetYaxis()->GetTitle()) == itr->second[2].second)) {error_list_bool.push_back(true);} else {
    std::cout << KRED << itr->second[2].first << RST << std::endl; error_list_bool.push_back(false);}}

  std::map<std::tuple<projection_type, filter_type, scale_type>, std::vector<std::pair<std::string, std::string>> > Test_GetTitle_2DProjection ={
    {std::make_tuple(projection_type::DIR_TIME_V, filterType, scale_type::raw),
    {{"get2DProjection(DIR_TIME_V, "+Filter+", raw)->GetTitle()", "Event-89 selected by " + Filter_string + " from V"},
    { "get2DProjection(DIR_TIME_V, "+Filter+", raw)->GetXaxis()->GetTitle()", "Time [bin]"},
    { "get2DProjection(DIR_TIME_V, "+Filter+", raw)->GetYaxis()->GetTitle()", "V [strip]"}}},
    {std::make_tuple(projection_type::DIR_TIME_V, filterType, scale_type::mm),
    {{"get2DProjection(DIR_TIME_V, "+Filter+", mm)->GetTitle()", "Event-89 selected by " + Filter_string + " from V"},
    { "get2DProjection(DIR_TIME_V, "+Filter+", mm)->GetXaxis()->GetTitle()", "Time [mm]"},
    { "get2DProjection(DIR_TIME_V, "+Filter+", mm)->GetYaxis()->GetTitle()", "V [mm]"}}}};
  for(auto itr=Test_GetTitle_2DProjection.begin(); itr!=Test_GetTitle_2DProjection.end(); itr++){
  std::shared_ptr<TH2D> temp = aEventPtr->get2DProjection(std::get<0>(itr->first) , std::get<1>(itr->first) , std::get<2>(itr->first));
  if (bool (std::string(temp->GetTitle()) == itr->second[0].second)) {error_list_bool.push_back(true);} else {
    std::cout << KRED << itr->second[0].first << RST << std::endl; error_list_bool.push_back(false);} 
  if (bool (std::string(temp->GetXaxis()->GetTitle()) == itr->second[1].second)) {error_list_bool.push_back(true);} else {
    std::cout << KRED << itr->second[1].first << RST << std::endl; error_list_bool.push_back(false);} 
  if (bool (std::string(temp->GetYaxis()->GetTitle()) == itr->second[2].second)) {error_list_bool.push_back(true);} else {
    std::cout << KRED << itr->second[2].first << RST << std::endl; error_list_bool.push_back(false);}}

  
  std::map<std::tuple<projection_type, filter_type, scale_type>, std::tuple<std::string, std::string, int, double>> Test_1DProjection ={
    {std::make_tuple(projection_type::DIR_U, filterType, scale_type::raw),
    std::make_tuple("get1DProjection(DIR_U, "+Filter+", raw)->GetName()/GetEntries()/GetSumOfWeights()",
    "h" + Filter_string + "_Upro_evt89", comparision1[0], comparision1[1])},
    {std::make_tuple(projection_type::DIR_V, filterType, scale_type::raw),
    std::make_tuple("get1DProjection(DIR_V, "+Filter+", raw)->GetName()/GetEntries()/GetSumOfWeights()",
    "h" + Filter_string + "_Vpro_evt89", comparision1[2], comparision1[3])},
    {std::make_tuple(projection_type::DIR_W, filterType, scale_type::raw),
    std::make_tuple("get1DProjection(DIR_W, "+Filter+", raw)->GetName()/GetEntries()/GetSumOfWeights()",
    "h" + Filter_string + "_Wpro_evt89", comparision1[4], comparision1[5])},
    {std::make_tuple(projection_type::DIR_U, filterType, scale_type::mm),
    std::make_tuple("get1DProjection(DIR_U, "+Filter+", mm)->GetName()/GetEntries()/GetSumOfWeights()",
    "h" + Filter_string + "_Upro_mm_evt89", comparision1[6], comparision1[7])},
    {std::make_tuple(projection_type::DIR_V, filterType, scale_type::mm),
    std::make_tuple("get1DProjection(DIR_V, "+Filter+", mm)->GetName()/GetEntries()/GetSumOfWeights()",
    "h" + Filter_string + "_Vpro_mm_evt89", comparision1[8], comparision1[9])},
    {std::make_tuple(projection_type::DIR_W, filterType, scale_type::mm),
    std::make_tuple("get1DProjection(DIR_W, "+Filter+", mm)->GetName()/GetEntries()/GetSumOfWeights()",
    "h" + Filter_string + "_Wpro_mm_evt89", comparision1[10], comparision1[11])},
    {std::make_tuple(projection_type::DIR_TIME, filterType, scale_type::raw),
    std::make_tuple("get1DProjection(DIR_TIME, "+Filter+", raw)->GetName()/GetEntries()/GetSumOfWeights()",
    "h" + Filter_string + "_timetime_evt89", comparision1[12], comparision1[13])},
    {std::make_tuple(projection_type::DIR_TIME_U, filterType, scale_type::raw),
    std::make_tuple("get1DProjection(DIR_TIME_U, "+Filter+", raw)->GetName()/GetEntries()/GetSumOfWeights()",
    "h" + Filter_string + "_Utime_evt89", comparision1[14], comparision1[15])},
    {std::make_tuple(projection_type::DIR_TIME_V, filterType, scale_type::raw),
    std::make_tuple("get1DProjection(DIR_TIME_V, "+Filter+", raw)->GetName()/GetEntries()/GetSumOfWeights()",
    "h" + Filter_string + "_Vtime_evt89", comparision1[16], comparision1[17])},
    {std::make_tuple(projection_type::DIR_TIME_W, filterType, scale_type::raw),
    std::make_tuple("get1DProjection(DIR_TIME_W, "+Filter+", raw)->GetName()/GetEntries()/GetSumOfWeights()",
    "h" + Filter_string + "_Wtime_evt89", comparision1[18], comparision1[19])},
    {std::make_tuple(projection_type::DIR_TIME, filterType, scale_type::mm),
    std::make_tuple("get1DProjection(DIR_TIME, "+Filter+", mm)->GetName()/GetEntries()/GetSumOfWeights()",
    "h" + Filter_string + "_timetime_mm_evt89", comparision1[20], comparision1[21])},
    {std::make_tuple(projection_type::DIR_TIME_U, filterType, scale_type::mm),
    std::make_tuple("get1DProjection(DIR_TIME_U, "+Filter+", mm)->GetName()/GetEntries()/GetSumOfWeights()",
    "h" + Filter_string + "_Utime_mm_evt89", comparision1[22], comparision1[23])},
    {std::make_tuple(projection_type::DIR_TIME_V, filterType, scale_type::mm),
    std::make_tuple("get1DProjection(DIR_TIME_V, "+Filter+", mm)->GetName()/GetEntries()/GetSumOfWeights()",
    "h" + Filter_string + "_Vtime_mm_evt89", comparision1[24], comparision1[25])},
    {std::make_tuple(projection_type::DIR_TIME_W, filterType, scale_type::mm),
    std::make_tuple("get1DProjection(DIR_TIME_W, "+Filter+", mm)->GetName()/GetEntries()/GetSumOfWeights()",
    "h" + Filter_string + "_Wtime_mm_evt89", comparision1[26], comparision1[27])}
    };
    for(auto itr=Test_1DProjection.begin(); itr!=Test_1DProjection.end(); itr++){
    std::shared_ptr<TH1D> temp = aEventPtr->get1DProjection(std::get<0>(itr->first) , std::get<1>(itr->first) , std::get<2>(itr->first));
    if (bool (std::string(temp->GetName()) == std::get<1>(itr->second) &&
              int(temp->GetEntries()) == std::get<2>(itr->second) &&
              abs(double(temp->GetSumOfWeights()) - std::get<3>(itr->second)) < epsilon)) {error_list_bool.push_back(true);} else {
      std::cout << KRED << std::get<0>(itr->second) << RST << std::endl; error_list_bool.push_back(false);} 
    }
  
  std::map<std::tuple<projection_type, filter_type, scale_type>, std::tuple<std::string, std::string, int, double>> Test_2DProjection ={
    {std::make_tuple(projection_type::DIR_TIME_U, filterType, scale_type::raw),
    std::make_tuple("get2DProjection(DIR_TIME_U, "+Filter+", raw)->GetName()/GetEntries()/GetSumOfWeights()",
    "h" + Filter_string + "_U_vs_time_evt89", comparision1[28], comparision1[29])},
    {std::make_tuple(projection_type::DIR_TIME_V, filterType, scale_type::raw),
    std::make_tuple("get2DProjection(DIR_TIME_V, "+Filter+", raw)->GetName()/GetEntries()/GetSumOfWeights()",
    "h" + Filter_string + "_V_vs_time_evt89", comparision1[30], comparision1[31])},
    {std::make_tuple(projection_type::DIR_TIME_W, filterType, scale_type::raw),
    std::make_tuple("get2DProjection(DIR_TIME_W, "+Filter+", raw)->GetName()/GetEntries()/GetSumOfWeights()",
    "h" + Filter_string + "_W_vs_time_evt89", comparision1[32], comparision1[33])},
    {std::make_tuple(projection_type::DIR_TIME_U, filterType, scale_type::mm),
    std::make_tuple("get2DProjection(DIR_TIME_U, "+Filter+", mm)->GetName()/GetEntries()/GetSumOfWeights()",
    "h" + Filter_string + "_U_vs_time_mm_evt89", comparision1[34], comparision1[35])},
    {std::make_tuple(projection_type::DIR_TIME_V, filterType, scale_type::mm),
    std::make_tuple("get2DProjection(DIR_TIME_V, "+Filter+", mm)->GetName()/GetEntries()/GetSumOfWeights()",
    "h" + Filter_string + "_V_vs_time_mm_evt89", comparision1[36], comparision1[37])},
    {std::make_tuple(projection_type::DIR_TIME_W, filterType, scale_type::mm),
    std::make_tuple("get2DProjection(DIR_TIME_W, "+Filter+", mm)->GetName()/GetEntries()/GetSumOfWeights()",
    "h" + Filter_string + "_W_vs_time_mm_evt89", comparision1[38], comparision1[39])}
    };
    for(auto itr=Test_2DProjection.begin(); itr!=Test_2DProjection.end(); itr++){
    std::shared_ptr<TH2D> temp = aEventPtr->get2DProjection(std::get<0>(itr->first) , std::get<1>(itr->first) , std::get<2>(itr->first));
    if (bool (std::string(temp->GetName()) == std::get<1>(itr->second) &&
              int(temp->GetEntries()) == std::get<2>(itr->second) &&
              abs(double(temp->GetSumOfWeights()) - std::get<3>(itr->second)) < epsilon)) {error_list_bool.push_back(true);} else {
      std::cout << KRED << std::get<0>(itr->second) << RST << std::endl; error_list_bool.push_back(false);} 
    }
  
  std::vector<std::string> error_list = { "Title_raw", "V_Strip", "Charge_strip", "Title_raw_time", "Time_bin", "Charge_time_bin", "Title_raw_V", "Time_bin_V", "V_Strip_", "Title_raw_V_", "Time_mm", "V_Strip_mm", "U_raw", "V_raw", "W_raw", "U_mm", "V_mm", "W_mm", "TTime", "UTime", "VTime", "WTime", "TTime_mm", "UTime_mm", "VTime_mm", "WTime_mm", "U_vs_time", "V_vs_time", "W_vs_time", "U_vs_time_mm", "V_vs_time_mm", "W_vs_time_mm", "charge", "charge_DIR_U", "charge_DIR_U_strip1", "charge_DIR_U_sec1_strip_58", "charge_time_cell128", "charge_DIR_U_time_cell128", "charge_DIR_U_sec1_time_cell28", "max_charge", "max_charge_DIR_U", "max_charge_DIR_U_strip1", "max_charge_DIR_U_sec1_strip58", "max_charge_time", "max_charge_channel", "max_charge_time_DIR_U", "max_charge_strip_DIR_U", "min_time", "max_time", "min_time_DIR_U", "max_time_DIR_U", "min_strip_DIR_U", "max_strip_DIR_U", "multiplicity_total", "multiplicity_DIR_U", "multiplicity_DIR_V", "multiplicity_DIR_W", "multiplicity_DIR_U_0", "multiplicity_DIR_V_0", "multiplicity_DIR_W_0", "Nhits_total", "Nhits_DIR_U", "Nhits_DIR_V", "Nhits_DIR_W", "Nhits_DIR_U_0", "Nhits_DIR_U_0_70", "NhitsMerged_DIR_U_70", "Channels" ,"Channels_raw" };
  
  std::map<std::tuple<double, double, double, double, filter_type>, std::pair<std::string, double>> Test_GetTotalCharge ={
    {std::make_tuple(-1, -1, -1, -1, filterType),       {"total charge: Test_GetTotalCharge(-1, -1, -1, -1, "+Filter+")", comparision2[0]}},
    {std::make_tuple(DIR_U, -1, -1, -1, filterType),    {"total charge DIR_U: Test_GetTotalCharge(DIR_U, -1, -1, -1, "+Filter+")", comparision2[1]}},
    {std::make_tuple(DIR_U, -1, 1, -1, filterType),     {"total charge DIR_U, strip 1: Test_GetTotalCharge(DIR_U, -1, 1, -1, "+Filter+")", comparision2[2]}},
    {std::make_tuple(DIR_U, 1, 58, -1, filterType),     {"total charge DIR_U, sec. 1, strip 58: Test_GetTotalCharge(DIR_U, 1, 58, -1, "+Filter+")", comparision2[3]}},
    {std::make_tuple(-1, -1, -1, 128, filterType),      {"total charge time cell 128: Test_GetTotalCharge(-1, -1, -1, 128, "+Filter+")", comparision2[4]}},
    {std::make_tuple(DIR_U, -1, -1, 128, filterType),   {"total charge DIR_U, time cell 128: Test_GetTotalCharge(DIR_U, -1, -1, 128, "+Filter+")", comparision2[5]}},
    {std::make_tuple(DIR_U, 1,  -1, 128, filterType),   {"total charge DIR_U, sec. 1, time cell 128: Test_GetTotalCharge(DIR_U, 1,  -1, 128, "+Filter+")", comparision2[6]}}
    };
    for(auto itr=Test_GetTotalCharge.begin(); itr!=Test_GetTotalCharge.end(); itr++){
    double temp = aEventPtr->GetTotalCharge(std::get<0>(itr->first), std::get<1>(itr->first), std::get<2>(itr->first), std::get<3>(itr->first), std::get<4>(itr->first));
    if (bool ((temp - itr->second.second) < epsilon)) {error_list_bool.push_back(true);} else {
      std::cout << KRED << itr->second.first << RST << std::endl; error_list_bool.push_back(false);} 
    }

  bool max_charge = (aEventPtr->GetMaxCharge(-1,-1,-1,filterType) - comparision2[7]) < epsilon;                          
  bool max_charge_DIR_U = (aEventPtr->GetMaxCharge(DIR_U,-1,-1,filterType) - comparision2[8]) < epsilon;                 
  bool max_charge_DIR_U_strip1 = (aEventPtr->GetMaxCharge(DIR_U, -1, 1, filterType) - comparision2[9]) < epsilon;        
  bool max_charge_DIR_U_sec1_strip58 = (aEventPtr->GetMaxCharge(DIR_U, 1, 58,filterType) - comparision2[10]) < epsilon;  
  error_list_bool.push_back(max_charge);
  error_list_bool.push_back(max_charge_DIR_U);
  error_list_bool.push_back(max_charge_DIR_U_strip1);
  error_list_bool.push_back(max_charge_DIR_U_sec1_strip58);

  int maxTime = 0, maxStrip = 0;
  std::tie(maxTime, maxStrip) = aEventPtr->GetMaxChargePos(-1,filterType);
  bool max_charge_time = maxTime - comparision2[11] == 0;  
  bool max_charge_channel = 0 - comparision2[12] == 0;     
  error_list_bool.push_back(max_charge_time);
  error_list_bool.push_back(max_charge_channel);
  std::tie(maxTime, maxStrip) = aEventPtr->GetMaxChargePos(DIR_U, filterType);
  bool max_charge_time_DIR_U = maxTime - comparision2[13] == 0;  
  bool max_charge_strip_DIR_U = maxStrip - comparision2[14] == 0;
  error_list_bool.push_back(max_charge_time_DIR_U);
  error_list_bool.push_back(max_charge_strip_DIR_U);

  int minTime=0, minStrip=0;
  std::tie(minTime, maxTime, minStrip, maxStrip) = aEventPtr->GetSignalRange(-1, filterType);
  bool min_time = minTime - comparision2[15] == 0;
  bool max_time = maxTime - comparision2[16] == 0;
  error_list_bool.push_back(min_time);
  error_list_bool.push_back(max_time);

  std::tie(minTime, maxTime, minStrip, maxStrip) = aEventPtr->GetSignalRange(DIR_U, filterType);
  bool min_time_DIR_U = minTime - comparision2[17] == 0;    
  bool max_time_DIR_U = maxTime - comparision2[18] == 0;    
  bool min_strip_DIR_U = minStrip - comparision2[19] == 0;  
  bool max_strip_DIR_U = maxStrip - comparision2[20] == 0;
  error_list_bool.push_back(min_time_DIR_U);
  error_list_bool.push_back(max_time_DIR_U);
  error_list_bool.push_back(min_strip_DIR_U);
  error_list_bool.push_back(max_strip_DIR_U);
  
  bool multiplicity_total = aEventPtr->GetMultiplicity(false, -1, -1, -1, filterType) - comparision2[21] == 0;     
  bool multiplicity_DIR_U = aEventPtr->GetMultiplicity(false, DIR_U, -1, -1, filterType) - comparision2[22] == 0;  
  bool multiplicity_DIR_V = aEventPtr->GetMultiplicity(false, DIR_V, -1, -1, filterType) - comparision2[23] == 0;  
  bool multiplicity_DIR_W = aEventPtr->GetMultiplicity(false, DIR_W, -1, -1, filterType) - comparision2[24] == 0;
  error_list_bool.push_back(multiplicity_total);
  error_list_bool.push_back(multiplicity_DIR_U);
  error_list_bool.push_back(multiplicity_DIR_V);
  error_list_bool.push_back(multiplicity_DIR_W);
  
  bool multiplicity_DIR_U_0 = aEventPtr->GetMultiplicity(false, DIR_U, 0, -1, filterType) - comparision2[25] == 0;
  bool multiplicity_DIR_V_0 = aEventPtr->GetMultiplicity(false, DIR_V, 0, -1, filterType) - comparision2[26] == 0;
  bool multiplicity_DIR_W_0 = aEventPtr->GetMultiplicity(false, DIR_W, 0, -1, filterType) - comparision2[27] == 0;
  error_list_bool.push_back(multiplicity_DIR_U_0);
  error_list_bool.push_back(multiplicity_DIR_V_0);
  error_list_bool.push_back(multiplicity_DIR_W_0);

  bool Nhits_total = aEventPtr->GetMultiplicity(true, -1, -1, -1, filterType) - comparision2[28] == 0;             
  bool Nhits_DIR_U = aEventPtr->GetMultiplicity(true, DIR_U, -1, -1, filterType) - comparision2[29] == 0;          
  bool Nhits_DIR_V = aEventPtr->GetMultiplicity(true, DIR_V, -1, -1, filterType) - comparision2[30] == 0;          
  bool Nhits_DIR_W = aEventPtr->GetMultiplicity(true, DIR_W, -1, -1, filterType) - comparision2[31] == 0;          
  bool Nhits_DIR_U_0 = aEventPtr->GetMultiplicity(true, DIR_U, 0, -1, filterType) - comparision2[32] == 0;         
  bool Nhits_DIR_U_0_70 = aEventPtr->GetMultiplicity(true, DIR_U, 0, 70, filterType) - comparision2[33] == 0;      
  bool NhitsMerged_DIR_U_70 = aEventPtr->GetMultiplicity(true, DIR_U, -1, 70, filterType) - comparision2[34] == 0; 
  error_list_bool.push_back(Nhits_total);
  error_list_bool.push_back(Nhits_DIR_U);
  error_list_bool.push_back(Nhits_DIR_V);
  error_list_bool.push_back(Nhits_DIR_W);
  error_list_bool.push_back(Nhits_DIR_U_0);
  error_list_bool.push_back(Nhits_DIR_U_0_70);
  error_list_bool.push_back(NhitsMerged_DIR_U_70);

  if (filterType == filter_type::none) {
      //std::cout << KBLU << "2D projection on time cells vs AGET channels" << RST << std::endl;
      std::shared_ptr<TH2D> Channels = aEventPtr->GetChannels(0, 0);
      bool Channels_bool = (std::string(Channels->GetName()) == "h" + Filter_string + "_cobo0_asad0_signal_evt89" && int(Channels->GetEntries()) == comparision1[40] && abs(double(Channels->GetSumOfWeights()) - comparision1[41]) < epsilon);
      std::shared_ptr<TH2D> Channels_raw = aEventPtr->GetChannels_raw(0, 0);
      bool Channels_raw_bool = (std::string(Channels_raw->GetName()) == "h" + Filter_string + "_cobo0_asad0_signal_fpn_evt89" && int(Channels_raw->GetEntries()) == comparision1[42] && abs(double(Channels_raw->GetSumOfWeights()) - comparision1[43]) < epsilon);
      error_list_bool.push_back(Channels_bool);
      error_list_bool.push_back(Channels_raw_bool);
  }
  int check = error_list_bool.size();
  for(std::vector<bool>::iterator it = error_list_bool.begin(); it != error_list_bool.end(); ++it) {check -= *it;}
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

  std::vector<double> comparison_none2 = { 520098.542683 , 155908.768293 , 1407.329268293 , 367.042682927 , 2890.25609756 , 950.109756098 , -79.378 , 1735.09146341 , 1392.21341463 , 23.7256 , 22.2561 , 58 , 0 ,  58 , 65 , 2 , 500 , 3 , 501 , 1 , 132 , 1018 , 131 , 224 , 225 , 0 , 74 , 74 , 290917 , 65868  , 112275  , 112774  , 0 , 0 , 499};

  std::vector<double> comparison_threshold2 = { 794935.22561 , 226979.29878 , 0 , 0 , 3425.77439024 , 1032.90243902 , 0 , 1732.16463415 , 1385.27439024 , 0 , 0 , 58 , 0 , 58 , 65 , 33 , 251 , 37 , 250 , 46 , 81 , 112 , 25 , 43 , 29 , 0 , 28 , 15 , 5874 , 1747 , 2266 , 1861 , 0 , 0 , 104};

  std::vector<double> comparison_none1 = { 155908 , 155908.76829307 , 176881, 176881.810976, 188406, 188406.182927, 155908, 155908.768293, 176881, 176882 - 0.189024, 188406, 188406 + 0.182927, 520098, 520098.542683, 155908, 155908.768293, 176881, 176881.81097561, 187307, 187307.96341464,
                                            520098, 520098.542683, 155908, 155908.768293, 176881, 176881.810976, 187307, 187307.9634146, 155908, 155908.768293, 176881, 176881.81097563, 187307, 187307.9634146, 224650, 155908.768293, 176881, 176881.810976, 304271, 187307.9634145, 131072, -5029.57317073, 139264, -5029.57317073 };
  std::vector<double> comparison_threshold1 = { 226979, 226979.29878 ,285676, 285676.20122 ,282279, 282279.72561 ,226979,226979.29878,285676, 285676.20122 ,282279, 282279.72561,794935, 794935.22561,226979, 226979.29878,285676, 285676.20122, 282279, 282279.72561,
                                                 794935,794935.22561, 226979,226979.29878,285676, 285676.20122,282279, 282279.72561,226979, 226979.29878, 285676,285676.20122, 282279,282279.72561,295721,226979.29878,285676, 285676.20122,399243, 282279.72561 };
  int check=0;
  auto myEventPtr = myEventSource->getCurrentEvent();
  for(int i=89;i<90;++i){
    myEventSource->loadFileEntry(i);
  
    std::cout<<myEventPtr->GetEventInfo()<<std::endl;
    check += testHits(myEventPtr, filter_type::none , comparison_none1, comparison_none2);
    check += testHits(myEventPtr, filter_type::threshold , comparison_threshold1, comparison_threshold2);
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
