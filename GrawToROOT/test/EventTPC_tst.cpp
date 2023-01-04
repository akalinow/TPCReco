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

int testHits(std::shared_ptr<EventTPC> aEventPtr, filter_type filterType, std::vector<double> comparision1, std::vector<double> comparision2, std::map<std::string, double> Test_Reference, std::map<std::string, std::string> Test_Reference_Titles){
  
  std::vector<bool> error_list_bool;
  double epsilon = 1e-5;
  std::cout << std::boolalpha;
  std::string Filter_string;
  std::string Filter;
  if (filterType == filter_type::none) { Filter_string = "raw"; Filter="none";}
  if (filterType == filter_type::threshold) { Filter_string = "Threshold";  Filter="threshold";}

  std::map<std::tuple<projection_type, filter_type, scale_type>, std::vector<std::pair<std::string, std::string>>> Test_GetTitle_1DProjection ={
    {std::make_tuple(projection_type::DIR_V, filterType, scale_type::raw),
    {{"get1DProjection(DIR_V, "+Filter+", raw)->GetTitle()",                "Event-89 selected by "+Filter_string+" from V integrated over time"},
    { "get1DProjection(DIR_V, "+Filter+", raw)->GetXaxis()->GetTitle()",    "V [strip]"},
    { "get1DProjection(DIR_V, "+Filter+", raw)->GetYaxis()->GetTitle()",    "Charge/strip [arb.u.]"}}},
    {std::make_tuple(projection_type::DIR_TIME, filterType, scale_type::raw),
    {{"get1DProjection(DIR_TIME, "+Filter+", raw)->GetTitle()",             "Event-89 selected by "+Filter_string+" from time "},
    { "get1DProjection(DIR_TIME, "+Filter+", raw)->GetXaxis()->GetTitle()", "Time [bin]"},
    { "get1DProjection(DIR_TIME, "+Filter+", raw)->GetYaxis()->GetTitle()", "Charge/time bin [arb.u.]"}}}
  };
  for(auto itr=Test_GetTitle_1DProjection.begin(); itr!=Test_GetTitle_1DProjection.end(); itr++){
    std::shared_ptr<TH1D> temp = aEventPtr->get1DProjection(std::get<0>(itr->first) , std::get<1>(itr->first) , std::get<2>(itr->first)); 
    if (bool (std::string(temp->GetTitle()) == itr->second[0].second)) {error_list_bool.push_back(true);} 
    else {std::cout << KRED << itr->second[0].first << RST << std::endl; error_list_bool.push_back(false);} 
    if (bool (std::string(temp->GetXaxis()->GetTitle()) == itr->second[1].second)) {error_list_bool.push_back(true);} 
    else {std::cout << KRED << itr->second[1].first << RST << std::endl; error_list_bool.push_back(false);} 
    if (bool (std::string(temp->GetYaxis()->GetTitle()) == itr->second[2].second)) {error_list_bool.push_back(true);}
    else {std::cout << KRED << itr->second[2].first << RST << std::endl; error_list_bool.push_back(false);}
  }
  
  std::map<std::tuple<projection_type, filter_type, scale_type>, std::vector<std::pair<std::string, std::string>>> Test_GetTitle_2DProjection ={
    {std::make_tuple(projection_type::DIR_TIME_V, filterType, scale_type::raw),
    {{"get2DProjection(DIR_TIME_V, "+Filter+", raw)->GetTitle()",             "Event-89 selected by "+Filter_string+" from V"},
    { "get2DProjection(DIR_TIME_V, "+Filter+", raw)->GetXaxis()->GetTitle()", "Time [bin]"},
    { "get2DProjection(DIR_TIME_V, "+Filter+", raw)->GetYaxis()->GetTitle()", "V [strip]"}}},
    {std::make_tuple(projection_type::DIR_TIME_V, filterType, scale_type::mm),
    {{"get2DProjection(DIR_TIME_V, "+Filter+", mm)->GetTitle()",              "Event-89 selected by "+Filter_string+" from V"},
    { "get2DProjection(DIR_TIME_V, "+Filter+", mm)->GetXaxis()->GetTitle()",  "Time [mm]"},
    { "get2DProjection(DIR_TIME_V, "+Filter+", mm)->GetYaxis()->GetTitle()",  "V [mm]"}}}
  };
  for(auto itr=Test_GetTitle_2DProjection.begin(); itr!=Test_GetTitle_2DProjection.end(); itr++){
    std::shared_ptr<TH2D> temp = aEventPtr->get2DProjection(std::get<0>(itr->first) , std::get<1>(itr->first) , std::get<2>(itr->first));
    if (bool (std::string(temp->GetTitle()) == itr->second[0].second)) {error_list_bool.push_back(true);}
    else {std::cout << KRED << itr->second[0].first << RST << std::endl; error_list_bool.push_back(false);} 
    if (bool (std::string(temp->GetXaxis()->GetTitle()) == itr->second[1].second)) {error_list_bool.push_back(true);}
    else {std::cout << KRED << itr->second[1].first << RST << std::endl; error_list_bool.push_back(false);} 
    if (bool (std::string(temp->GetYaxis()->GetTitle()) == itr->second[2].second)) {error_list_bool.push_back(true);}
    else {std::cout << KRED << itr->second[2].first << RST << std::endl; error_list_bool.push_back(false);}
  }
  // get1DProjection
  std::map<projection_type, std::string> ProjectionTypes = { {projection_type::DIR_U, "projection_type::DIR_U"},{projection_type::DIR_V , "projection_type::DIR_V"} , {projection_type::DIR_W , "projection_type::DIR_W"} ,{projection_type::DIR_TIME, "projection_type::DIR_TIME"},{projection_type::DIR_TIME_U,"projection_type::DIR_TIME_U"},{projection_type::DIR_TIME_V,"projection_type::DIR_TIME_V"},{projection_type::DIR_TIME_W,"projection_type::DIR_TIME_W"}};
  std::map<filter_type, std::string> FilterTypes = { {filter_type::none,"filter_type::none"} , {filter_type::threshold,"filter_type::threshold"} }; 
  std::map<scale_type, std::string> ScaleTypes = { {scale_type::raw,"scale_type::raw"} , {scale_type::mm,"scale_type::mm"} };
  for (auto projection : ProjectionTypes) {
      for (auto filter : FilterTypes) {
          for (auto scale : ScaleTypes) {
              std::shared_ptr<TH1D> Test = aEventPtr->get1DProjection(std::get<0>(projection), std::get<0>(filter), std::get<0>(scale));
              std::string Test_String = "get1DProjection(" + std::get<1>(projection) + ", " + std::get<1>(filter) + ", " + std::get<1>(scale) + ")";
              if (std::string(Test->GetName()) == Test_Reference_Titles[Test_String + "->GetName()"] && 
                  int(Test->GetEntries()) == Test_Reference[Test_String + "->GetEntries()"] && 
                  abs(double(Test->GetSumOfWeights())) - Test_Reference[Test_String + "->GetSumOfWeights()"] < epsilon) {error_list_bool.push_back(true);}
              else { std::cout << KRED << Test_String << RST << std::endl; error_list_bool.push_back(false); }
          }
      }
  }

  /*std::map<std::tuple<projection_type, filter_type, scale_type>, std::tuple<std::string, std::string, int, double>> Test_1DProjection = {
    {std::make_tuple(projection_type::DIR_U, filterType, scale_type::raw),
     std::make_tuple("get1DProjection(DIR_U, "+Filter+", raw)->GetName()/GetEntries()/GetSumOfWeights()",
                     "h"+Filter_string+"_Upro_evt89",        comparision1[0], comparision1[1])},
    {std::make_tuple(projection_type::DIR_V, filterType, scale_type::raw),
     std::make_tuple("get1DProjection(DIR_V, "+Filter+", raw)->GetName()/GetEntries()/GetSumOfWeights()",
                     "h"+Filter_string+"_Vpro_evt89",        comparision1[2], comparision1[3])},
    {std::make_tuple(projection_type::DIR_W, filterType, scale_type::raw),
     std::make_tuple("get1DProjection(DIR_W, "+Filter+", raw)->GetName()/GetEntries()/GetSumOfWeights()",
                     "h"+Filter_string+"_Wpro_evt89",        comparision1[4], comparision1[5])},
    {std::make_tuple(projection_type::DIR_U, filterType, scale_type::mm),
     std::make_tuple("get1DProjection(DIR_U, "+Filter+", mm)->GetName()/GetEntries()/GetSumOfWeights()",
                     "h"+Filter_string+"_Upro_mm_evt89",     comparision1[6], comparision1[7])},
    {std::make_tuple(projection_type::DIR_V, filterType, scale_type::mm),
     std::make_tuple("get1DProjection(DIR_V, "+Filter+", mm)->GetName()/GetEntries()/GetSumOfWeights()",
                     "h"+Filter_string+"_Vpro_mm_evt89",     comparision1[8], comparision1[9])},
    {std::make_tuple(projection_type::DIR_W, filterType, scale_type::mm),
     std::make_tuple("get1DProjection(DIR_W, "+Filter+", mm)->GetName()/GetEntries()/GetSumOfWeights()",
                     "h"+Filter_string+"_Wpro_mm_evt89",     comparision1[10], comparision1[11])},
    {std::make_tuple(projection_type::DIR_TIME, filterType, scale_type::raw),
     std::make_tuple("get1DProjection(DIR_TIME, "+Filter+", raw)->GetName()/GetEntries()/GetSumOfWeights()",
                     "h"+Filter_string+"_timetime_evt89",    comparision1[12], comparision1[13])},
    {std::make_tuple(projection_type::DIR_TIME_U, filterType, scale_type::raw),
     std::make_tuple("get1DProjection(DIR_TIME_U, "+Filter+", raw)->GetName()/GetEntries()/GetSumOfWeights()",
                     "h"+Filter_string+"_Utime_evt89",       comparision1[14], comparision1[15])},
    {std::make_tuple(projection_type::DIR_TIME_V, filterType, scale_type::raw),
     std::make_tuple("get1DProjection(DIR_TIME_V, "+Filter+", raw)->GetName()/GetEntries()/GetSumOfWeights()",
                     "h"+Filter_string+"_Vtime_evt89",       comparision1[16], comparision1[17])},
    {std::make_tuple(projection_type::DIR_TIME_W, filterType, scale_type::raw),
     std::make_tuple("get1DProjection(DIR_TIME_W, "+Filter+", raw)->GetName()/GetEntries()/GetSumOfWeights()",
                     "h"+Filter_string+"_Wtime_evt89",       comparision1[18], comparision1[19])},
    {std::make_tuple(projection_type::DIR_TIME, filterType, scale_type::mm),
     std::make_tuple("get1DProjection(DIR_TIME, "+Filter+", mm)->GetName()/GetEntries()/GetSumOfWeights()",
                     "h"+Filter_string+"_timetime_mm_evt89", comparision1[20], comparision1[21])},
    {std::make_tuple(projection_type::DIR_TIME_U, filterType, scale_type::mm),
     std::make_tuple("get1DProjection(DIR_TIME_U, "+Filter+", mm)->GetName()/GetEntries()/GetSumOfWeights()",
                     "h"+Filter_string+"_Utime_mm_evt89",    comparision1[22], comparision1[23])},
    {std::make_tuple(projection_type::DIR_TIME_V, filterType, scale_type::mm),
     std::make_tuple("get1DProjection(DIR_TIME_V, "+Filter+", mm)->GetName()/GetEntries()/GetSumOfWeights()",
                     "h"+Filter_string+"_Vtime_mm_evt89",    comparision1[24], comparision1[25])},
    {std::make_tuple(projection_type::DIR_TIME_W, filterType, scale_type::mm),
     std::make_tuple("get1DProjection(DIR_TIME_W, "+Filter+", mm)->GetName()/GetEntries()/GetSumOfWeights()",
                     "h"+Filter_string+"_Wtime_mm_evt89",    comparision1[26], comparision1[27])}
  };
  for(auto itr=Test_1DProjection.begin(); itr!=Test_1DProjection.end(); itr++){
    std::shared_ptr<TH1D> temp = aEventPtr->get1DProjection(std::get<0>(itr->first) , std::get<1>(itr->first) , std::get<2>(itr->first));
    if (bool (std::string(temp->GetName()) == std::get<1>(itr->second) &&
              int(temp->GetEntries()) == std::get<2>(itr->second) &&
              abs(double(temp->GetSumOfWeights()) - std::get<3>(itr->second)) < epsilon)) {error_list_bool.push_back(true);}
    else {std::cout << KRED << std::get<0>(itr->second) << RST << std::endl; error_list_bool.push_back(false);} 
  }*/
  
  std::map<std::tuple<projection_type, filter_type, scale_type>, std::tuple<std::string, std::string, int, double>> Test_2DProjection ={
    {std::make_tuple(projection_type::DIR_TIME_U, filterType, scale_type::raw),
     std::make_tuple("get2DProjection(DIR_TIME_U, "+Filter+", raw)->GetName()/GetEntries()/GetSumOfWeights()",
                     "h"+Filter_string+"_U_vs_time_evt89", comparision1[28], comparision1[29])},
    {std::make_tuple(projection_type::DIR_TIME_V, filterType, scale_type::raw),
     std::make_tuple("get2DProjection(DIR_TIME_V, "+Filter+", raw)->GetName()/GetEntries()/GetSumOfWeights()",
                     "h"+Filter_string+"_V_vs_time_evt89", comparision1[30], comparision1[31])},
    {std::make_tuple(projection_type::DIR_TIME_W, filterType, scale_type::raw),
     std::make_tuple("get2DProjection(DIR_TIME_W, "+Filter+", raw)->GetName()/GetEntries()/GetSumOfWeights()",
                     "h"+Filter_string+"_W_vs_time_evt89", comparision1[32], comparision1[33])},
    {std::make_tuple(projection_type::DIR_TIME_U, filterType, scale_type::mm),
     std::make_tuple("get2DProjection(DIR_TIME_U, "+Filter+", mm)->GetName()/GetEntries()/GetSumOfWeights()",
                     "h"+Filter_string+"_U_vs_time_mm_evt89", comparision1[34], comparision1[35])},
    {std::make_tuple(projection_type::DIR_TIME_V, filterType, scale_type::mm),
     std::make_tuple("get2DProjection(DIR_TIME_V, "+Filter+", mm)->GetName()/GetEntries()/GetSumOfWeights()",
                     "h"+Filter_string+"_V_vs_time_mm_evt89", comparision1[36], comparision1[37])},
    {std::make_tuple(projection_type::DIR_TIME_W, filterType, scale_type::mm),
     std::make_tuple("get2DProjection(DIR_TIME_W, "+Filter+", mm)->GetName()/GetEntries()/GetSumOfWeights()",
                     "h"+Filter_string+"_W_vs_time_mm_evt89", comparision1[38], comparision1[39])}
  };
  for(auto itr=Test_2DProjection.begin(); itr!=Test_2DProjection.end(); itr++){
    std::shared_ptr<TH2D> temp = aEventPtr->get2DProjection(std::get<0>(itr->first) , std::get<1>(itr->first) , std::get<2>(itr->first));
    if (bool (std::string(temp->GetName()) == std::get<1>(itr->second) &&
              int(temp->GetEntries()) == std::get<2>(itr->second) &&
              abs(double(temp->GetSumOfWeights()) - std::get<3>(itr->second)) < epsilon)) {error_list_bool.push_back(true);}
    else {std::cout << KRED << std::get<0>(itr->second) << RST << std::endl; error_list_bool.push_back(false);} 
  }
  
  if (filterType == filter_type::none) {
    std::map<std::tuple<double, double>, std::tuple<std::string, std::string, int, double>> Test_GetChannels ={
      {std::make_tuple(0, 0),
       std::make_tuple("GetChannels(0, 0)->GetName()/GetEntries()/GetSumOfWeights()", "h"+Filter_string+"_cobo0_asad0_signal_evt89", comparision1[40], comparision1[41])}
    };
    for(auto itr=Test_GetChannels.begin(); itr!=Test_GetChannels.end(); itr++){
      std::shared_ptr<TH2D> temp = aEventPtr->GetChannels(std::get<0>(itr->first) , std::get<1>(itr->first));
      if (bool (std::string(temp->GetName()) == std::get<1>(itr->second) &&
                int(temp->GetEntries()) == std::get<2>(itr->second) &&
                abs(double(temp->GetSumOfWeights()) - std::get<3>(itr->second)) < epsilon)) {error_list_bool.push_back(true);}
      else {std::cout << KRED << std::get<0>(itr->second) << RST << std::endl; error_list_bool.push_back(false);}
    } 
      std::map<std::tuple<double, double>, std::tuple<std::string, std::string, int, double>> Test_GetChannels_raw ={
      {std::make_tuple(0, 0),
      std::make_tuple("GetChannels_raw(0, 0)->GetName()/GetEntries()/GetSumOfWeights()", "h"+Filter_string+"_cobo0_asad0_signal_fpn_evt89", comparision1[42], comparision1[43])}
    };
    for(auto itr=Test_GetChannels_raw.begin(); itr!=Test_GetChannels_raw.end(); itr++){
      std::shared_ptr<TH2D> temp = aEventPtr->GetChannels_raw(std::get<0>(itr->first) , std::get<1>(itr->first));
      if (bool (std::string(temp->GetName()) == std::get<1>(itr->second) &&
                int(temp->GetEntries()) == std::get<2>(itr->second) &&
                abs(double(temp->GetSumOfWeights()) - std::get<3>(itr->second)) < epsilon)) {error_list_bool.push_back(true);}
      else {std::cout << KRED << std::get<0>(itr->second) << RST << std::endl; error_list_bool.push_back(false);}
    }
  }

  std::map<std::tuple<double, double, double, double, filter_type>, std::pair<double, std::string>> Test_GetTotalCharge ={
    {std::make_tuple(-1, -1, -1, -1, filterType),     {Test_Reference["GetTotalCharge(-1, -1, -1, -1, filter_type::" + Filter + ")"    ], "total charge: GetTotalCharge(-1, -1, -1, -1, "+Filter+")"}},
    {std::make_tuple(DIR_U, -1, -1, -1, filterType),  {Test_Reference["GetTotalCharge(DIR_U, -1, -1, -1, filter_type::" + Filter + ")" ], "total charge DIR_U: GetTotalCharge(DIR_U, -1, -1, -1, "+Filter+")"}},
    {std::make_tuple(DIR_U, -1, 1, -1, filterType),   {Test_Reference["GetTotalCharge(DIR_U, -1, 1, -1, filter_type::" + Filter + ")"  ], "total charge DIR_U, strip 1: GetTotalCharge(DIR_U, -1, 1, -1, "+Filter+")"}},
    {std::make_tuple(DIR_U, 1, 58, -1, filterType),   {Test_Reference["GetTotalCharge(DIR_U, 1, 58, -1, filter_type::" + Filter + ")"  ], "total charge DIR_U, sec. 1, strip 58: GetTotalCharge(DIR_U, 1, 58, -1, "+Filter+")"}},
    {std::make_tuple(-1, -1, -1, 128, filterType),    {Test_Reference["GetTotalCharge(-1, -1, -1, 128, filter_type::" + Filter + ")"   ], "total charge time cell 128: GetTotalCharge(-1, -1, -1, 128, "+Filter+")"}},
    {std::make_tuple(DIR_U, -1, -1, 128, filterType), {Test_Reference["GetTotalCharge(DIR_U, -1, -1, 128, filter_type::" + Filter + ")"], "total charge DIR_U, time cell 128: GetTotalCharge(DIR_U, -1, -1, 128, "+Filter+")"}},
    {std::make_tuple(DIR_U, 1, -1, 128, filterType),  {Test_Reference["GetTotalCharge(DIR_U, 1,  -1, 128, filter_type::" + Filter + ")"], "total charge DIR_U, sec. 1, time cell 128: GetTotalCharge(DIR_U, 1,  -1, 128, "+Filter+")"}}
  };
  for(auto itr=Test_GetTotalCharge.begin(); itr!=Test_GetTotalCharge.end(); itr++){
    double temp = aEventPtr->GetTotalCharge(std::get<0>(itr->first), std::get<1>(itr->first), std::get<2>(itr->first), std::get<3>(itr->first), std::get<4>(itr->first));
    if (bool ((temp - itr->second.first) < epsilon)) {error_list_bool.push_back(true);}
    else {std::cout << KRED << itr->second.second << RST << std::endl; error_list_bool.push_back(false);} 
  }

  std::map<std::tuple<double, double, double, filter_type>, std::pair<double, std::string>> Test_GetMaxCharge ={
    {std::make_tuple(-1, -1, -1, filterType),    {Test_Reference["GetMaxCharge(-1, -1, -1, filter_type::" + Filter + ")"   ], "max charge: GetMaxCharge(-1, -1, -1, "+Filter+")"}},
    {std::make_tuple(DIR_U, -1, -1, filterType), {Test_Reference["GetMaxCharge(DIR_U, -1, -1, filter_type::" + Filter + ")"], "max charge DIR_U: GetMaxCharge(DIR_U, -1, -1, "+Filter+")"}},
    {std::make_tuple(DIR_U, -1, 1, filterType),  {Test_Reference["GetMaxCharge(DIR_U, -1, 1, filter_type::" + Filter + ")" ], "max charge DIR_U, strip 1: GetMaxCharge(DIR_U, -1, 1, "+Filter+")"}},
    {std::make_tuple(DIR_U, 1, 58, filterType),  {Test_Reference["GetMaxCharge(DIR_U, 1, 58, filter_type::" + Filter + ")" ],"max charge DIR_U, sec. 1, strip 58: GetMaxCharge(DIR_U, 1, 58, "+Filter+")"}}
  };
  for(auto itr=Test_GetMaxCharge.begin(); itr!=Test_GetMaxCharge.end(); itr++){
    double temp = aEventPtr->GetMaxCharge(std::get<0>(itr->first), std::get<1>(itr->first), std::get<2>(itr->first), std::get<3>(itr->first));
    if (bool ((temp - itr->second.first) < epsilon)) {error_list_bool.push_back(true);}
    else {std::cout << KRED << itr->second.second << RST << std::endl; error_list_bool.push_back(false);} 
  }

  std::map<std::tuple<double, filter_type>, std::tuple<double, double, std::string, std::string>> Test_GetMaxChargePos ={
    {std::make_tuple(-1, filterType),    std::make_tuple(comparision2[11], comparision2[12], "max charge time: GetMaxChargePos(-1, "+Filter+")", 
                                                                                             "max charge channel: GetMaxChargePos(-1, "+Filter+")")},
    // something wired in example std::cout<<"max charge channel: "<<0<<std::endl; i changed comparision2[12] from 0 to 65
    {std::make_tuple(DIR_U, filterType), std::make_tuple(comparision2[13], comparision2[14], "max charge time DIR_U: GetMaxChargePos(DIR_U, "+Filter+")",
                                                                                             "max charge strip DIR_U: GetMaxChargePos(DIR_U, "+Filter+")")}
  };
  int maxTime = 0, maxStrip = 0, minTime=0, minStrip=0;
  for(auto itr=Test_GetMaxChargePos.begin(); itr!=Test_GetMaxChargePos.end(); itr++){
    std::tie(maxTime, maxStrip) = aEventPtr->GetMaxChargePos(std::get<0>(itr->first), std::get<1>(itr->first));
    if (bool (maxTime - std::get<0>(itr->second) == 0)) {error_list_bool.push_back(true);}
    else {std::cout << KRED << std::get<2>(itr->second) << RST << std::endl; error_list_bool.push_back(false);}
    if (bool (maxStrip - std::get<1>(itr->second) == 0)) {error_list_bool.push_back(true);}
    else {std::cout << KRED << std::get<3>(itr->second) << RST << std::endl; error_list_bool.push_back(false);} 
  }

  std::map<std::tuple<double, filter_type>, std::tuple<double, double, double, double, std::string, std::string, std::string, std::string>> Test_GetSignalRange ={
    {std::make_tuple(-1, filterType),    std::make_tuple(comparision2[15], comparision2[16], comparision2[17], comparision2[18],
                                                         "min time: GetSignalRange(-1, "+Filter+")", "max time: GetSignalRange(-1, "+Filter+")",
                                                         "min strip: GetSignalRange(-1, "+Filter+")", "max strip: GetSignalRange(-1, "+Filter+")")},
                                                        // added to vectors min/max strip values
    {std::make_tuple(DIR_U, filterType), std::make_tuple(comparision2[19], comparision2[20], comparision2[21], comparision2[22],
                                                         "min time DIR_U: GetSignalRange(DIR_U, "+Filter+")", "max time DIR_U: GetSignalRange(DIR_U, "+Filter+")",
                                                         "min strip DIR_U: GetSignalRange(DIR_U, "+Filter+")", "max strip DIR_U: GetSignalRange(DIR_U, "+Filter+")")}
  };
  for(auto itr=Test_GetSignalRange.begin(); itr!=Test_GetSignalRange.end(); itr++){
    std::tie(minTime, maxTime, minStrip, maxStrip) = aEventPtr->GetSignalRange(std::get<0>(itr->first), std::get<1>(itr->first));
    if (bool (minTime - std::get<0>(itr->second) == 0)) {error_list_bool.push_back(true);}
    else {std::cout << KRED << std::get<4>(itr->second) << RST << std::endl; error_list_bool.push_back(false);}
    if (bool (maxTime - std::get<1>(itr->second) == 0)) {error_list_bool.push_back(true);}
    else {std::cout << KRED << std::get<5>(itr->second) << RST << std::endl; error_list_bool.push_back(false);}
    if (bool (minStrip - std::get<2>(itr->second) == 0)) {error_list_bool.push_back(true);}
    else {std::cout << KRED << std::get<6>(itr->second) << RST << std::endl; error_list_bool.push_back(false);}
    if (bool (maxStrip - std::get<3>(itr->second) == 0)) {error_list_bool.push_back(true);}
    else {std::cout << KRED << std::get<7>(itr->second) << RST << std::endl; error_list_bool.push_back(false);} 
  }

  std::map<std::tuple<bool, double, double, double, filter_type>, std::pair<double, std::string>> Test_GetMultiplicity ={
    {std::make_tuple(false, -1, -1, -1, filterType),    {comparision2[23], "multiplicity(total): GetMultiplicity(false, -1, -1, -1, "+Filter+")" }},
    {std::make_tuple(false, DIR_U, -1, -1, filterType), {comparision2[24], "multiplicity(DIR_U): GetMultiplicity(false, DIR_U, -1, -1, "+Filter+")"}},
    {std::make_tuple(false, DIR_V, -1, -1, filterType), {comparision2[25], "multiplicity(DIR_V): GetMultiplicity(false, DIR_V, -1, -1, "+Filter+")"}},
    {std::make_tuple(false, DIR_W, -1, -1, filterType), {comparision2[26], "multiplicity(DIR_W): GetMultiplicity(false, DIR_W, -1, -1, "+Filter+")"}},
    {std::make_tuple(false, DIR_U, 0, -1, filterType),  {comparision2[27], "multiplicity(DIR_U, 0): GetMultiplicity(false, DIR_U, 0, -1, "+Filter+")"}},
    {std::make_tuple(false, DIR_V, 0, -1, filterType),  {comparision2[28], "multiplicity(DIR_V, 0): GetMultiplicity(false, DIR_V, 0, -1, "+Filter+")"}},
    {std::make_tuple(false, DIR_W, 0, -1, filterType),  {comparision2[29], "multiplicity(DIR_W, 0): GetMultiplicity(false, DIR_W, 0, -1, "+Filter+")"}},
    
    {std::make_tuple(true, -1, -1, -1, filterType),     {comparision2[30], "Nhits(total): GetMultiplicity(true, -1, -1, -1, "+Filter+")"}},
    {std::make_tuple(true, DIR_U, -1, -1, filterType),  {comparision2[31], "Nhits(DIR_U): GetMultiplicity(true, DIR_U, -1, -1, "+Filter+")"}},
    {std::make_tuple(true, DIR_V, -1, -1, filterType),  {comparision2[32], "Nhits(DIR_V): GetMultiplicity(true, DIR_V, -1, -1, "+Filter+")"}},
    {std::make_tuple(true, DIR_W, -1, -1, filterType),  {comparision2[33], "Nhits(DIR_W): GetMultiplicity(true, DIR_W, -1, -1, "+Filter+")"}},
    {std::make_tuple(true, DIR_U, 0, -1, filterType),   {comparision2[34], "Nhits(DIR_U, 0): GetMultiplicity(true, DIR_U, 0, -1, "+Filter+")"}},
    {std::make_tuple(true, DIR_U, 0, 70, filterType),   {comparision2[35], "Nhits(DIR_U, 0, 70): GetMultiplicity(true, DIR_U, 0, 70, "+Filter+")"}},
    {std::make_tuple(true, DIR_U, -1, 70, filterType),  {comparision2[36], "NhitsMerged(DIR_U, 70): GetMultiplicity(true, DIR_U, -1, 70, "+Filter+")"}}
  };
  for(auto itr=Test_GetMultiplicity.begin(); itr!=Test_GetMultiplicity.end(); itr++){
    double temp = aEventPtr->GetMultiplicity(std::get<0>(itr->first), std::get<1>(itr->first), std::get<2>(itr->first), std::get<3>(itr->first), std::get<4>(itr->first));
    if (bool (temp - itr->second.first == 0)) {error_list_bool.push_back(true);}
    else {std::cout << KRED << itr->second.second << RST << std::endl; error_list_bool.push_back(false);} 
  }
  
  int check = error_list_bool.size();
  for(std::vector<bool>::iterator it = error_list_bool.begin(); it != error_list_bool.end(); ++it) {check -= *it;}
  
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

  std::vector<double> comparison_none2 = { 520098.542683 , 155908.768293 , 1407.329268293 , 367.042682927 , 2890.25609756 , 950.109756098 , -79.378 , 1735.09146341 , 1392.21341463 , 23.7256 , 22.2561 , 58 , 65 ,  58 , 65 , 2 , 500 , 1 , 226 , 3 , 501 , 1 , 132 , 1018 , 131 , 224 , 225 , 0 , 74 , 74 , 290917 , 65868  , 112275  , 112774  , 0 , 0 , 499};

  std::vector<double> comparison_threshold2 = { 794935.22561 , 226979.29878 , 0 , 0 , 3425.77439024 , 1032.90243902 , 0 , 1732.16463415 , 1385.27439024 , 0 , 0 , 58 , 65 , 58 , 65 , 33 , 251 , 46 , 216, 37 , 250 , 46 , 81 , 112 , 25 , 43 , 29 , 0 , 28 , 15 , 5874 , 1747 , 2266 , 1861 , 0 , 0 , 104};

  std::vector<double> comparison_none1 = { 155908 , 155908.76829307 , 176881, 176881.810976, 188406, 188406.182927, 155908, 155908.768293, 176881, 176882 - 0.189024, 188406, 188406 + 0.182927, 520098, 520098.542683, 155908, 155908.768293, 176881, 176881.81097561, 187307, 187307.96341464,
                                            520098, 520098.542683, 155908, 155908.768293, 176881, 176881.810976, 187307, 187307.9634146, 155908, 155908.768293, 176881, 176881.81097563, 187307, 187307.9634146, 224650, 155908.768293, 176881, 176881.810976, 304271, 187307.9634145, 131072, -5029.57317073, 139264, -5029.57317073 };
  std::vector<double> comparison_threshold1 = { 226979, 226979.29878 ,285676, 285676.20122 ,282279, 282279.72561 ,226979,226979.29878,285676, 285676.20122 ,282279, 282279.72561,794935, 794935.22561,226979, 226979.29878,285676, 285676.20122, 282279, 282279.72561,
                                                 794935,794935.22561, 226979,226979.29878,285676, 285676.20122,282279, 282279.72561,226979, 226979.29878, 285676,285676.20122, 282279,282279.72561,295721,226979.29878,285676, 285676.20122,399243, 282279.72561 };
  std::map<std::string, std::string> Test_Reference_Titles = {
      {"get1DProjection(projection_type::DIR_V, filter_type::none, scale_type::raw)->GetTitle()"                  , "Event-89 selected by raw from V integrated over time"       } ,
      {"get1DProjection(projection_type::DIR_V, filter_type::threshold, scale_type::raw)->GetTitle()"             , "Event-89 selected by Threshold from V integrated over time" } ,
      {"get1DProjection(projection_type::DIR_V, filter_type::none, scale_type::raw)->GetXaxis()->GetTitle()"      , "V [strip]"                                                  } ,
      {"get1DProjection(projection_type::DIR_V, filter_type::threshold, scale_type::raw)->GetXaxis()->GetTitle()" , "V [strip]"                                                  } ,
      {"get1DProjection(projection_type::DIR_V, filter_type::none, scale_type::raw)->GetYaxis()->GetTitle()"      , "Charge/strip [arb.u.]"                                      } ,
      {"get1DProjection(projection_type::DIR_V, filter_type::threshold, scale_type::raw)->GetYaxis()->GetTitle()" , "Charge/strip [arb.u.]"                                      } ,
      {"get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::raw)->GetTitle()"                  , "Event-89 selected by raw from time "                     } ,
      {"get1DProjection(projection_type::DIR_TIME, filter_type::threshold, scale_type::raw)->GetTitle()"             , "Event-89 selected by Threshold from time "               } ,
      {"get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::raw)->GetXaxis()->GetTitle()"      , "Time [bin]"                                              } ,
      {"get1DProjection(projection_type::DIR_TIME, filter_type::threshold, scale_type::raw)->GetXaxis()->GetTitle()" , "Time [bin]"                                              } ,
      {"get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::raw)->GetYaxis()->GetTitle()"      , "Charge / time bin[arb.u.]"                               } ,
      {"get1DProjection(projection_type::DIR_TIME, filter_type::threshold, scale_type::raw)->GetYaxis()->GetTitle()" , "Charge / time bin[arb.u.]"                               } ,
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::raw)->GetTitle()"                  , "Event-89 selected by raw from V"                       } ,
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::raw)->GetTitle()"             , "Event-89 selected by Threshold from V"                 } ,
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::raw)->GetXaxis()->GetTitle()"      , "Time [bin]"                                            } ,
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::raw)->GetXaxis()->GetTitle()" , "Time [bin]"                                            } ,
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::raw)->GetYaxis()->GetTitle()"      , "V [strip]"                                             } ,
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::raw)->GetYaxis()->GetTitle()" , "V [strip]"                                             } ,
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::mm)->GetTitle()"                   , "Event-89 selected by raw from V"                       } ,
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::mm)->GetTitle()"              , "Event-89 selected by Threshold from V"                 } ,
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::mm)->GetXaxis()->GetTitle()"       , "Time [mm]"                                             } ,
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::mm)->GetXaxis()->GetTitle()"  , "Time [mm]"                                             } ,
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::mm)->GetYaxis()->GetTitle()"       , "V [mm]"                                                } ,
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::mm)->GetYaxis()->GetTitle()"  , "V [mm]"                                                } ,
      
      {"get1DProjection(projection_type::DIR_U, filter_type::none, scale_type::raw)->GetName()"  , "hraw_Upro_evt89" } , {"get1DProjection(projection_type::DIR_U, filter_type::threshold, scale_type::raw)->GetName()" , "hThreshold_Upro_evt89" } ,
      {"get1DProjection(projection_type::DIR_V, filter_type::none, scale_type::raw)->GetName()"  , "hraw_Vpro_evt89" } , {"get1DProjection(projection_type::DIR_V, filter_type::threshold, scale_type::raw)->GetName()" , "hThreshold_Vpro_evt89" } ,
      {"get1DProjection(projection_type::DIR_W, filter_type::none, scale_type::raw)->GetName()"  , "hraw_Wpro_evt89" } , {"get1DProjection(projection_type::DIR_W, filter_type::threshold, scale_type::raw)->GetName()" , "hThreshold_Wpro_evt89" } ,
      {"get1DProjection(projection_type::DIR_U, filter_type::none, scale_type::mm)->GetName()"  , "hraw_Upro_mm_evt89" } , {"get1DProjection(projection_type::DIR_U, filter_type::threshold, scale_type::mm)->GetName()", "hThreshold_Upro_mm_evt89" },
      {"get1DProjection(projection_type::DIR_V, filter_type::none, scale_type::mm)->GetName()"  , "hraw_Vpro_mm_evt89" } , {"get1DProjection(projection_type::DIR_V, filter_type::threshold, scale_type::mm)->GetName()", "hThreshold_Vpro_mm_evt89" },
      {"get1DProjection(projection_type::DIR_W, filter_type::none, scale_type::mm)->GetName()"  , "hraw_Wpro_mm_evt89" } , {"get1DProjection(projection_type::DIR_W, filter_type::threshold, scale_type::mm)->GetName()", "hThreshold_Wpro_mm_evt89" },
      {"get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::raw)->GetName()"  , "hraw_timetime_evt89" }, {"get1DProjection(projection_type::DIR_TIME, filter_type::threshold, scale_type::raw)->GetName()", "hThreshold_timetime_evt89" },
      {"get1DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::raw)->GetName()"  , "hraw_Utime_evt89" } , {"get1DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::raw)->GetName()", "hThreshold_Utime_evt89" },
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::raw)->GetName()"  , "hraw_Vtime_evt89" } , {"get1DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::raw)->GetName()", "hThreshold_Vtime_evt89" },
      {"get1DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::raw)->GetName()"  , "hraw_Wtime_evt89" } , {"get1DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::raw)->GetName()", "hThreshold_Wtime_evt89" },
      {"get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::mm)->GetName()"  , "hraw_timetime_mm_evt89" }, {"get1DProjection(projection_type::DIR_TIME, filter_type::threshold, scale_type::mm)->GetName()", "hThreshold_timetime_mm_evt89" },
      {"get1DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::mm)->GetName()"  , "hraw_Utime_mm_evt89" } , {"get1DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::mm)->GetName()", "hThreshold_Utime_mm_evt89" },
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::mm)->GetName()"  , "hraw_Vtime_mm_evt89" } , {"get1DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::mm)->GetName()", "hThreshold_Vtime_mm_evt89" },
      {"get1DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::mm)->GetName()"  , "hraw_Wtime_mm_evt89" } , {"get1DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::mm)->GetName()", "hThreshold_Wtime_mm_evt89" },
      {"get2DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::raw)->GetName()"  , "hraw_U_vs_time_evt89" } , {"get2DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::raw)->GetName()", "hThreshold_U_vs_time_evt89" },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::raw)->GetName()"  , "hraw_V_vs_time_evt89" } , {"get2DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::raw)->GetName()", "hThreshold_V_vs_time_evt89" },
      {"get2DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::raw)->GetName()"  , "hraw_W_vs_time_evt89" } , {"get2DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::raw)->GetName()", "hThreshold_W_vs_time_evt89" },
      {"get2DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::mm)->GetName()"  , "hraw_U_vs_time_mm_evt89" } , {"get2DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::mm)->GetName()", "hThreshold_U_vs_time_mm_evt89" },
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::mm)->GetName()"  , "hraw_V_vs_time_mm_evt89" } , {"get2DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::mm)->GetName()", "hThreshold_V_vs_time_mm_evt89" },
      {"get2DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::mm)->GetName()"  , "hraw_W_vs_time_mm_evt89" } , {"get2DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::mm)->GetName()", "hThreshold_W_vs_time_mm_evt89" },
      {"GetChannels(0,0)->GetName()" , "hraw_cobo0_asad0_signal_evt89" } , {"GetChannels_raw(0,0)->GetName()" , "hraw_cobo0_asad0_signal_fpn_evt89"}
  };

  std::map<std::string, double> Test_Reference = {
      {"get1DProjection(projection_type::DIR_U, filter_type::none, scale_type::raw)->GetEntries()"           , 155908           } , {"get1DProjection(projection_type::DIR_U, filter_type::threshold, scale_type::raw)->GetEntries()"           , 226979       } , // comparison1[0]
      {"get1DProjection(projection_type::DIR_U, filter_type::none, scale_type::raw)->GetSumOfWeights()"      , 155908.76829307  } , {"get1DProjection(projection_type::DIR_U, filter_type::threshold, scale_type::raw)->GetSumOfWeights()"      , 226979.29878 } , // ...
      {"get1DProjection(projection_type::DIR_V, filter_type::none, scale_type::raw)->GetEntries()"           , 176881           } , {"get1DProjection(projection_type::DIR_V, filter_type::threshold, scale_type::raw)->GetEntries()"           , 285676       } , // ...
      {"get1DProjection(projection_type::DIR_V, filter_type::none, scale_type::raw)->GetSumOfWeights()"      , 176881.810976    } , {"get1DProjection(projection_type::DIR_V, filter_type::threshold, scale_type::raw)->GetSumOfWeights()"      , 285676.20122 } , // ...
      {"get1DProjection(projection_type::DIR_W, filter_type::none, scale_type::raw)->GetEntries()"           , 188406           } , {"get1DProjection(projection_type::DIR_W, filter_type::threshold, scale_type::raw)->GetEntries()"           , 282279       } , // ...
      {"get1DProjection(projection_type::DIR_W, filter_type::none, scale_type::raw)->GetSumOfWeights()"      , 188406.182927    } , {"get1DProjection(projection_type::DIR_W, filter_type::threshold, scale_type::raw)->GetSumOfWeights()"      , 282279.72561 } , // ...
      {"get1DProjection(projection_type::DIR_U, filter_type::none, scale_type::mm)->GetEntries()"            , 155908           } , {"get1DProjection(projection_type::DIR_U, filter_type::threshold, scale_type::mm)->GetEntries()"            , 226979       } , // ...
      {"get1DProjection(projection_type::DIR_U, filter_type::none, scale_type::mm)->GetSumOfWeights()"       , 155908.768293    } , {"get1DProjection(projection_type::DIR_U, filter_type::threshold, scale_type::mm)->GetSumOfWeights()"       , 226979.29878 } , // ...
      {"get1DProjection(projection_type::DIR_V, filter_type::none, scale_type::mm)->GetEntries()"            , 176881           } , {"get1DProjection(projection_type::DIR_V, filter_type::threshold, scale_type::mm)->GetEntries()"            , 285676       } , // ...
      {"get1DProjection(projection_type::DIR_V, filter_type::none, scale_type::mm)->GetSumOfWeights()"       , 176881.810976    } , {"get1DProjection(projection_type::DIR_V, filter_type::threshold, scale_type::mm)->GetSumOfWeights()"       , 285676.20122 } , // ...
      {"get1DProjection(projection_type::DIR_W, filter_type::none, scale_type::mm)->GetEntries()"            , 188406           } , {"get1DProjection(projection_type::DIR_W, filter_type::threshold, scale_type::mm)->GetEntries()"            , 282279       } , // ...
      {"get1DProjection(projection_type::DIR_W, filter_type::none, scale_type::mm)->GetSumOfWeights()"       , 188406.182927    } , {"get1DProjection(projection_type::DIR_W, filter_type::threshold, scale_type::mm)->GetSumOfWeights()"       , 282279.72561 } , // ...
      {"get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::raw)->GetEntries()"        , 520098           } , {"get1DProjection(projection_type::DIR_TIME, filter_type::threshold, scale_type::raw)->GetEntries()"        , 794935       } , // ...
      {"get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::raw)->GetSumOfWeights()"   , 520098.542683    } , {"get1DProjection(projection_type::DIR_TIME, filter_type::threshold, scale_type::raw)->GetSumOfWeights()"   , 794935.22561 } , // ...
      {"get1DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::raw)->GetEntries()"      , 155908           } , {"get1DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::raw)->GetEntries()"      , 226979       } , // ...
      {"get1DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::raw)->GetSumOfWeights()" , 155908.768293    } , {"get1DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::raw)->GetSumOfWeights()" , 226979.29878 } , // ...
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::raw)->GetEntries()"      , 176881           } , {"get1DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::raw)->GetEntries()"      , 285676       } , // ...
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::raw)->GetSumOfWeights()" , 176881.81097561  } , {"get1DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::raw)->GetSumOfWeights()" , 285676.20122 } , // ...
      {"get1DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::raw)->GetEntries()"      , 187307           } , {"get1DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::raw)->GetEntries()"      , 282279       } , // ...
      {"get1DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::raw)->GetSumOfWeights()" , 187307.96341464  } , {"get1DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::raw)->GetSumOfWeights()" , 282279.72561 } , // ...
      {"get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::mm)->GetEntries()"         , 520098           } , {"get1DProjection(projection_type::DIR_TIME, filter_type::threshold, scale_type::mm)->GetEntries()"         , 794935       } , // ...
      {"get1DProjection(projection_type::DIR_TIME, filter_type::none, scale_type::mm)->GetSumOfWeights()"    , 520098.542683    } , {"get1DProjection(projection_type::DIR_TIME, filter_type::threshold, scale_type::mm)->GetSumOfWeights()"    , 794935.22561 } , // ...
      {"get1DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::mm)->GetEntries()"       , 155908           } , {"get1DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::mm)->GetEntries()"       , 226979       } , // ...
      {"get1DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::mm)->GetSumOfWeights()"  , 155908.768293    } , {"get1DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::mm)->GetSumOfWeights()"  , 226979.29878 } , // ...
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::mm)->GetEntries()"       , 176881           } , {"get1DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::mm)->GetEntries()"       , 285676       } , // ...
      {"get1DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::mm)->GetSumOfWeights()"  , 176881.810976    } , {"get1DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::mm)->GetSumOfWeights()"  , 285676.20122 } , // ...
      {"get1DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::mm)->GetEntries()"       , 187307           } , {"get1DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::mm)->GetEntries()"       , 282279       } , // ...
      {"get1DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::mm)->GetSumOfWeights()"  , 187307.9634146   } , {"get1DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::mm)->GetSumOfWeights()"  , 282279.72561 } , // ...
      {"get2DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::raw)->GetEntries()"      , 155908           } , {"get2DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::raw)->GetEntries()"      , 226979       } , // ...
      {"get2DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::raw)->GetSumOfWeights()" , 155908.768293    } , {"get2DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::raw)->GetSumOfWeights()" , 226979.29878 } , // ...
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::raw)->GetEntries()"      , 176881           } , {"get2DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::raw)->GetEntries()"      , 285676       } , // ...
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::raw)->GetSumOfWeights()" , 176881.81097563  } , {"get2DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::raw)->GetSumOfWeights()" , 285676.20122 } , // ...
      {"get2DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::raw)->GetEntries()"      , 187307           } , {"get2DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::raw)->GetEntries()"      , 282279       } , // ...
      {"get2DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::raw)->GetSumOfWeights()" , 187307.9634146   } , {"get2DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::raw)->GetSumOfWeights()" , 282279.72561 } , // ...
      {"get2DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::mm)->GetEntries()"       , 224650           } , {"get2DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::mm)->GetEntries()"       , 295721       } , // ...
      {"get2DProjection(projection_type::DIR_TIME_U, filter_type::none, scale_type::mm)->GetSumOfWeights()"  , 155908.768293    } , {"get2DProjection(projection_type::DIR_TIME_U, filter_type::threshold, scale_type::mm)->GetSumOfWeights()"  , 226979.29878 } , // ...
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::mm)->GetEntries()"       , 176881           } , {"get2DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::mm)->GetEntries()"       , 285676       } , // ...
      {"get2DProjection(projection_type::DIR_TIME_V, filter_type::none, scale_type::mm)->GetSumOfWeights()"  , 176881.810976    } , {"get2DProjection(projection_type::DIR_TIME_V, filter_type::threshold, scale_type::mm)->GetSumOfWeights()"  , 285676.20122 } , // ...
      {"get2DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::mm)->GetEntries()"       , 304271           } , {"get2DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::mm)->GetEntries()"       , 399243       } , // ...
      {"get2DProjection(projection_type::DIR_TIME_W, filter_type::none, scale_type::mm)->GetSumOfWeights()"  , 187307.9634145   } , {"get2DProjection(projection_type::DIR_TIME_W, filter_type::threshold, scale_type::mm)->GetSumOfWeights()"  , 282279.72561 } , // comparison1[39]
      {"GetChannels(0,0)->GetEntries()"          , 131072           } , // comparison1[40]
      {"GetChannels(0,0)->GetSumOfWeights()"     , -5029.57317073   } , // comparison1[41]
      {"GetChannels_raw(0,0)->GetEntries()"      , 139264           } , // comparison1[42]
      {"GetChannels_raw(0,0)->GetSumOfWeights()" , -5029.57317073   } , // comparison1[43]


      {"GetTotalCharge(-1, -1, -1, -1, filter_type::none)"       , 520098.542683  } , {"GetTotalCharge(-1, -1, -1, -1, filter_type::threshold)"       , 794935.22561   } ,  //comparison2[0]
      {"GetTotalCharge(DIR_U, -1, -1, -1, filter_type::none)"    , 155908.768293  } , {"GetTotalCharge(DIR_U, -1, -1, -1, filter_type::threshold)"    , 226979.29878   } ,  // ...
      {"GetTotalCharge(DIR_U, -1, 1, -1, filter_type::none)"     , 1407.329268293 } , {"GetTotalCharge(DIR_U, -1, 1, -1, filter_type::threshold)"     , 0              } ,  // ...
      {"GetTotalCharge(DIR_U, 1, 58, -1, filter_type::none)"     , 367.042682927  } , {"GetTotalCharge(DIR_U, 1, 58, -1, filter_type::threshold)"     , 0              } ,  // ...
      {"GetTotalCharge(-1, -1, -1, 128, filter_type::none)"      , 2890.25609756  } , {"GetTotalCharge(-1, -1, -1, 128, filter_type::threshold)"      , 3425.77439024  } ,  // ...
      {"GetTotalCharge(DIR_U, -1, -1, 128, filter_type::none)"   , 950.109756098  } , {"GetTotalCharge(DIR_U, -1, -1, 128, filter_type::threshold)"   , 1032.90243902  } ,  // ...
      {"GetTotalCharge(DIR_U, 1,  -1, 128, filter_type::none)"   , -79.378        } , {"GetTotalCharge(DIR_U, 1,  -1, 128, filter_type::threshold)"   , 0              } ,  // ...
      {"GetMaxCharge(-1, -1, -1, filter_type::none)"             , 1735.09146341  } , {"GetMaxCharge(-1, -1, -1, filter_type::threshold)"             , 1732.16463415  } ,  // ...
      {"GetMaxCharge(DIR_U, -1, -1, filter_type::none)"          , 1392.21341463  } , {"GetMaxCharge(DIR_U, -1, -1, filter_type::threshold)"          , 1385.27439024  } ,  // ...
      {"GetMaxCharge(DIR_U, -1, 1, filter_type::none)"           , 23.7256        } , {"GetMaxCharge(DIR_U, -1, 1, filter_type::threshold)"           , 0              } ,  // ...
      {"GetMaxCharge(DIR_U, 1, 58, filter_type::none)"           , 22.2561        } , {"GetMaxCharge(DIR_U, 1, 58, filter_type::threshold)"           , 0              } ,  // ...
      {"GetMaxChargePos(-1, filter_type::none)->maxTime"         , 58             } , {"GetMaxChargePos(-1, filter_type::threshold)->maxTime"         , 58             } ,  // ...
      {"GetMaxChargePos(-1, filter_type::none)->maxStrip"        , 65             } , {"GetMaxChargePos(-1, filter_type::threshold)->maxStrip"        , 65             } ,  // ...
      {"GetMaxChargePos(DIR_U, filter_type::none)->maxTime"      , 58             } , {"GetMaxChargePos(DIR_U, filter_type::threshold)->maxTime"      , 58             } ,  // ...
      {"GetMaxChargePos(DIR_U, filter_type::none)->maxStrip"     , 65             } , {"GetMaxChargePos(DIR_U, filter_type::threshold)->maxStrip"     , 65             } ,  // ...
      {"GetSignalRange(-1, filter_type::none)->minTime"          , 2              } , {"GetSignalRange(-1, filter_type::threshold)->minTime"          , 33             } ,  // ...
      {"GetSignalRange(-1, filter_type::none)->maxTime"          , 500            } , {"GetSignalRange(-1, filter_type::threshold)->maxTime"          , 251            } ,  // ...
      {"GetSignalRange(-1, filter_type::none)->minStrip"         , 1              } , {"GetSignalRange(-1, filter_type::threshold)->minStrip"         , 46             } ,  // ...
      {"GetSignalRange(-1, filter_type::none)->maxStrip"         , 226            } , {"GetSignalRange(-1, filter_type::threshold)->maxStrip"         , 216            } ,  // ...
      {"GetSignalRange(DIR_U, filter_type::none)->minTime"       , 3              } , {"GetSignalRange(DIR_U, filter_type::threshold)->minTime"       , 37             } ,  // ...
      {"GetSignalRange(DIR_U, filter_type::none)->maxTime"       , 501            } , {"GetSignalRange(DIR_U, filter_type::threshold)->maxTime"       , 250            } ,  // ...
      {"GetSignalRange(DIR_U, filter_type::none)->minStrip"      , 1              } , {"GetSignalRange(DIR_U, filter_type::threshold)->minStrip"      , 46             } ,  // ...
      {"GetSignalRange(DIR_U, filter_type::none)->maxStrip"      , 132            } , {"GetSignalRange(DIR_U, filter_type::threshold)->maxStrip"      , 81             } ,  // ...
      {"GetMultiplicity(false, -1, -1, -1, filter_type::none)"   , 1018           } , {"GetMultiplicity(false, -1, -1, -1, filter_type::threshold)"   , 112            } ,  // ...
      {"GetMultiplicity(false, DIR_U, -1, -1, filter_type::none)", 131            } , {"GetMultiplicity(false, DIR_U, -1, -1, filter_type::threshold)", 25             } ,  // ...
      {"GetMultiplicity(false, DIR_V, -1, -1, filter_type::none)", 224            } , {"GetMultiplicity(false, DIR_V, -1, -1, filter_type::threshold)", 43             } ,  // ...
      {"GetMultiplicity(false, DIR_W, -1, -1, filter_type::none)", 225            } , {"GetMultiplicity(false, DIR_W, -1, -1, filter_type::threshold)", 29             } ,  // ...
      {"GetMultiplicity(false, DIR_U, 0, -1, filter_type::none)" , 0              } , {"GetMultiplicity(false, DIR_U, 0, -1, filter_type::threshold)" , 0              } ,  // ...
      {"GetMultiplicity(false, DIR_V, 0, -1, filter_type::none)" , 74             } , {"GetMultiplicity(false, DIR_V, 0, -1, filter_type::threshold)" , 28             } ,  // ...
      {"GetMultiplicity(false, DIR_W, 0, -1, filter_type::none)" , 74             } , {"GetMultiplicity(false, DIR_W, 0, -1, filter_type::threshold)" , 15             } ,  // ...
      {"GetMultiplicity(true, -1, -1, -1, filter_type::none)"    , 290917         } , {"GetMultiplicity(true, -1, -1, -1, filter_type::threshold)"    , 5874           } ,  // ...
      {"GetMultiplicity(true, DIR_U, -1, -1, filter_type::none)" , 65868          } , {"GetMultiplicity(true, DIR_U, -1, -1, filter_type::threshold)" , 1747           } ,  // ...
      {"GetMultiplicity(true, DIR_V, -1, -1, filter_type::none)" , 112275         } , {"GetMultiplicity(true, DIR_V, -1, -1, filter_type::threshold)" , 2266           } ,  // ...
      {"GetMultiplicity(true, DIR_W, -1, -1, filter_type::none)" , 112774         } , {"GetMultiplicity(true, DIR_W, -1, -1, filter_type::threshold)" , 1861           } ,  // ...
      {"GetMultiplicity(true, DIR_U, 0, -1, filter_type::none)"  , 0              } , {"GetMultiplicity(true, DIR_U, 0, -1, filter_type::threshold)"  , 0              } ,  // ...
      {"GetMultiplicity(true, DIR_U, 0, 70, filter_type::none)"  , 0              } , {"GetMultiplicity(true, DIR_U, 0, 70, filter_type::threshold)"  , 0              } ,  // ...
      {"GetMultiplicity(true, DIR_U, -1, 70, filter_type::none)" , 499            } , {"GetMultiplicity(true, DIR_U, -1, 70, filter_type::threshold)" , 104            }    //comparison2[36]

  };
  int check=0;
  auto myEventPtr = myEventSource->getCurrentEvent();
  for(int i=89;i<90;++i){
    myEventSource->loadFileEntry(i);
  
    std::cout<<myEventPtr->GetEventInfo()<<std::endl;
    check += testHits(myEventPtr, filter_type::none , comparison_none1, comparison_none2, Test_Reference, Test_Reference_Titles);
    check += testHits(myEventPtr, filter_type::threshold , comparison_threshold1, comparison_threshold2, Test_Reference, Test_Reference_Titles);
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
