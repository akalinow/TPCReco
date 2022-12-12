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
  if (filterType == filter_type::none) { Filter_string = "raw"; }
  if (filterType == filter_type::threshold) { Filter_string = "Threshold"; }

  //std::cout << KBLU << "1D projection on strips: U, V, W [raw]" << RST << std::endl;
  bool Title_raw = std::string(aEventPtr->get1DProjection(projection_type::DIR_V, filterType, scale_type::raw)->GetTitle()) == "Event-89 selected by " + Filter_string + " from V integrated over time";
  bool V_Strip = std::string(aEventPtr->get1DProjection(projection_type::DIR_V, filterType, scale_type::raw)->GetXaxis()->GetTitle()) == "V [strip]";
  bool Charge_strip = std::string(aEventPtr->get1DProjection(projection_type::DIR_V, filterType, scale_type::raw)->GetYaxis()->GetTitle()) == "Charge/strip [arb.u.]";
  error_list_bool.push_back(Title_raw);
  error_list_bool.push_back(V_Strip);
  error_list_bool.push_back(Charge_strip);
  
  bool Title_raw_time = std::string(aEventPtr->get1DProjection(projection_type::DIR_TIME, filterType, scale_type::raw)->GetTitle()) == "Event-89 selected by " + Filter_string + " from time ";
  bool Time_bin = std::string(aEventPtr->get1DProjection(projection_type::DIR_TIME, filterType, scale_type::raw)->GetXaxis()->GetTitle()) == "Time [bin]";
  bool Charge_time_bin = std::string(aEventPtr->get1DProjection(projection_type::DIR_TIME, filterType, scale_type::raw)->GetYaxis()->GetTitle()) == "Charge/time bin [arb.u.]";
  error_list_bool.push_back(Title_raw_time);
  error_list_bool.push_back(Time_bin);
  error_list_bool.push_back(Charge_time_bin);
  
  bool  Title_raw_V = std::string(aEventPtr->get2DProjection(projection_type::DIR_TIME_V, filterType, scale_type::raw)->GetTitle()) == "Event-89 selected by " + Filter_string + " from V";
  bool  Time_bin_V = std::string(aEventPtr->get2DProjection(projection_type::DIR_TIME_V, filterType, scale_type::raw)->GetXaxis()->GetTitle()) == "Time [bin]";
  bool  V_Strip_ = std::string(aEventPtr->get2DProjection(projection_type::DIR_TIME_V, filterType, scale_type::raw)->GetYaxis()->GetTitle()) == "V [strip]";
  error_list_bool.push_back(Title_raw_V);
  error_list_bool.push_back(Time_bin_V);
  error_list_bool.push_back(V_Strip_);

  bool  Title_raw_V_ = std::string(aEventPtr->get2DProjection(projection_type::DIR_TIME_V, filterType, scale_type::mm)->GetTitle()) == "Event-89 selected by " + Filter_string + " from V";
  bool  Time_mm = std::string(aEventPtr->get2DProjection(projection_type::DIR_TIME_V, filterType, scale_type::mm)->GetXaxis()->GetTitle()) == "Time [mm]";
  bool  V_Strip_mm = std::string(aEventPtr->get2DProjection(projection_type::DIR_TIME_V, filterType, scale_type::mm)->GetYaxis()->GetTitle()) == "V [mm]";
  error_list_bool.push_back(Title_raw_V_);
  error_list_bool.push_back(Time_mm);
  error_list_bool.push_back(V_Strip_mm);
  
  std::shared_ptr<TH1D> U_raw = aEventPtr->get1DProjection(projection_type::DIR_U, filterType, scale_type::raw);
  bool U_raw_bool = (std::string(U_raw->GetName()) == "h" + Filter_string + "_Upro_evt89" && int(U_raw->GetEntries()) == comparision1[0] && abs(double(U_raw->GetSumOfWeights()) - comparision1[1]) < epsilon);
  std::shared_ptr<TH1D> V_raw = aEventPtr->get1DProjection(projection_type::DIR_V, filterType, scale_type::raw);
  bool V_raw_bool = (std::string(V_raw->GetName()) == "h" + Filter_string + "_Vpro_evt89" && int(V_raw->GetEntries()) == comparision1[2] && abs(double(V_raw->GetSumOfWeights()) - comparision1[3]) < epsilon);
  std::shared_ptr<TH1D> W_raw = aEventPtr->get1DProjection(projection_type::DIR_W, filterType, scale_type::raw);
  bool W_raw_bool = (std::string(W_raw->GetName()) == "h" + Filter_string + "_Wpro_evt89" && int(W_raw->GetEntries()) == comparision1[4] && abs(double(W_raw->GetSumOfWeights()) - comparision1[5]) < epsilon);
  error_list_bool.push_back(U_raw_bool);
  error_list_bool.push_back(V_raw_bool);
  error_list_bool.push_back(W_raw_bool);

  //std::cout << KBLU << "1D projection on strips U, V, W [mm]" << RST << std::endl;
  std::shared_ptr<TH1D> U_mm = aEventPtr->get1DProjection(projection_type::DIR_U, filterType, scale_type::mm);
  bool U_mm_bool = (std::string(U_mm->GetName()) == "h" + Filter_string + "_Upro_mm_evt89" && int(U_mm->GetEntries()) == comparision1[6] && abs(double(U_mm->GetSumOfWeights()) - comparision1[7]) < epsilon);
  std::shared_ptr<TH1D> V_mm = aEventPtr->get1DProjection(projection_type::DIR_V, filterType, scale_type::mm);
  bool V_mm_bool = (std::string(V_mm->GetName()) == "h" + Filter_string + "_Vpro_mm_evt89" && int(V_mm->GetEntries()) == comparision1[8] && abs(double(V_mm->GetSumOfWeights()) - comparision1[9]) < epsilon);
  std::shared_ptr<TH1D> W_mm = aEventPtr->get1DProjection(projection_type::DIR_W, filterType, scale_type::mm);
  bool W_mm_bool = (std::string(W_mm->GetName()) == "h" + Filter_string + "_Wpro_mm_evt89" && int(W_mm->GetEntries()) == comparision1[10] && abs(double(W_mm->GetSumOfWeights()) - comparision1[11]) < epsilon);
  error_list_bool.push_back(U_mm_bool);
  error_list_bool.push_back(V_mm_bool);
  error_list_bool.push_back(W_mm_bool);
  
  //std::cout << KBLU << "1D projection on time : global, U, V, W [raw]" << RST << std::endl;
  std::shared_ptr<TH1D> TTime = aEventPtr->get1DProjection(projection_type::DIR_TIME, filterType, scale_type::raw);
  bool TTime_bool = (std::string(TTime->GetName()) == "h" + Filter_string + "_timetime_evt89" && int(TTime->GetEntries()) == comparision1[12] && abs(double(TTime->GetSumOfWeights()) - comparision1[13]) < epsilon);
  std::shared_ptr<TH1D> UTime = aEventPtr->get1DProjection(projection_type::DIR_TIME_U, filterType, scale_type::raw);
  bool UTime_bool = (std::string(UTime->GetName()) == "h" + Filter_string + "_Utime_evt89" && int(UTime->GetEntries()) == comparision1[14] && abs(double(UTime->GetSumOfWeights()) - comparision1[15]) < epsilon);
  std::shared_ptr<TH1D> VTime = aEventPtr->get1DProjection(projection_type::DIR_TIME_V, filterType, scale_type::raw);
  bool VTime_bool = (std::string(VTime->GetName()) == "h" + Filter_string + "_Vtime_evt89" && int(VTime->GetEntries()) == comparision1[16] && abs(double(VTime->GetSumOfWeights()) - comparision1[17]) < epsilon);
  std::shared_ptr<TH1D> WTime = aEventPtr->get1DProjection(projection_type::DIR_TIME_W, filterType, scale_type::raw);
  bool WTime_bool = (std::string(WTime->GetName()) == "h" + Filter_string + "_Wtime_evt89" && int(WTime->GetEntries()) == comparision1[18] && abs(double(WTime->GetSumOfWeights()) - comparision1[19]) < epsilon);
  error_list_bool.push_back(TTime_bool);
  error_list_bool.push_back(UTime_bool);
  error_list_bool.push_back(VTime_bool);
  error_list_bool.push_back(WTime_bool);

  //std::cout << KBLU << "1D projection on time : global, U, V, W [mm]" << RST << std::endl;
  std::shared_ptr<TH1D> TTime_mm = aEventPtr->get1DProjection(projection_type::DIR_TIME, filterType, scale_type::mm);
  bool TTime_mm_bool = (std::string(TTime_mm->GetName()) == "h" + Filter_string + "_timetime_mm_evt89" && int(TTime_mm->GetEntries()) == comparision1[20] && abs(double(TTime_mm->GetSumOfWeights()) - comparision1[21]) < epsilon);
  std::shared_ptr<TH1D> UTime_mm = aEventPtr->get1DProjection(projection_type::DIR_TIME_U, filterType, scale_type::mm);
  bool UTime_mm_bool = (std::string(UTime_mm->GetName()) == "h" + Filter_string + "_Utime_mm_evt89" && int(UTime_mm->GetEntries()) == comparision1[22] && abs(double(UTime_mm->GetSumOfWeights()) - comparision1[23]) < epsilon);
  std::shared_ptr<TH1D> VTime_mm = aEventPtr->get1DProjection(projection_type::DIR_TIME_V, filterType, scale_type::mm);
  bool VTime_mm_bool = (std::string(VTime_mm->GetName()) == "h" + Filter_string + "_Vtime_mm_evt89" && int(VTime_mm->GetEntries()) == comparision1[24] && abs(double(VTime_mm->GetSumOfWeights()) - comparision1[25]) < epsilon);
  std::shared_ptr<TH1D> WTime_mm = aEventPtr->get1DProjection(projection_type::DIR_TIME_W, filterType, scale_type::mm);
  bool WTime_mm_bool = (std::string(WTime_mm->GetName()) == "h" + Filter_string + "_Wtime_mm_evt89" && int(WTime_mm->GetEntries()) == comparision1[26] && abs(double(WTime_mm->GetSumOfWeights()) - comparision1[27]) < epsilon);
  error_list_bool.push_back(TTime_mm_bool);
  error_list_bool.push_back(UTime_mm_bool);
  error_list_bool.push_back(VTime_mm_bool);
  error_list_bool.push_back(WTime_mm_bool);
  
  //std::cout << KBLU << "2D projection time vs strips: U, V, W [raw]" << RST << std::endl;
  std::shared_ptr<TH2D> U_vs_time = aEventPtr->get2DProjection(projection_type::DIR_TIME_U, filterType, scale_type::raw);
  bool U_vs_time_bool = (std::string(U_vs_time->GetName()) == "h" + Filter_string + "_U_vs_time_evt89" && int(U_vs_time->GetEntries()) == comparision1[28] && abs(double(U_vs_time->GetSumOfWeights()) - comparision1[29]) < epsilon);
  std::shared_ptr<TH2D> V_vs_time = aEventPtr->get2DProjection(projection_type::DIR_TIME_V, filterType, scale_type::raw);
  bool V_vs_time_bool = (std::string(V_vs_time->GetName()) == "h" + Filter_string + "_V_vs_time_evt89" && int(V_vs_time->GetEntries()) == comparision1[30] && abs(double(V_vs_time->GetSumOfWeights()) - comparision1[31]) < epsilon);
  std::shared_ptr<TH2D> W_vs_time = aEventPtr->get2DProjection(projection_type::DIR_TIME_W, filterType, scale_type::raw);
  bool W_vs_time_bool = (std::string(W_vs_time->GetName()) == "h" + Filter_string + "_W_vs_time_evt89" && int(W_vs_time->GetEntries()) == comparision1[32] && abs(double(W_vs_time->GetSumOfWeights()) - comparision1[33]) < epsilon);
  error_list_bool.push_back(U_vs_time_bool);
  error_list_bool.push_back(V_vs_time_bool);
  error_list_bool.push_back(W_vs_time_bool);

  //std::cout << KBLU << "2D projection on time : global, U, V, W [mm]" << RST << std::endl;
  std::shared_ptr<TH2D> U_vs_time_mm = aEventPtr->get2DProjection(projection_type::DIR_TIME_U, filterType, scale_type::mm);
  bool U_vs_time_mm_bool = (std::string(U_vs_time_mm->GetName()) == "h" + Filter_string + "_U_vs_time_mm_evt89" && int(U_vs_time_mm->GetEntries()) == comparision1[34] && abs(double(U_vs_time_mm->GetSumOfWeights()) - comparision1[35]) < epsilon);
  std::shared_ptr<TH2D> V_vs_time_mm = aEventPtr->get2DProjection(projection_type::DIR_TIME_V, filterType, scale_type::mm);
  bool V_vs_time_mm_bool = (std::string(V_vs_time_mm->GetName()) == "h" + Filter_string + "_V_vs_time_mm_evt89" && int(V_vs_time_mm->GetEntries()) == comparision1[36] && abs(double(V_vs_time_mm->GetSumOfWeights()) - comparision1[37]) < epsilon);
  std::shared_ptr<TH2D> W_vs_time_mm = aEventPtr->get2DProjection(projection_type::DIR_TIME_W, filterType, scale_type::mm);
  bool W_vs_time_mm_bool = (std::string(W_vs_time_mm->GetName()) == "h" + Filter_string + "_W_vs_time_mm_evt89" && int(W_vs_time_mm->GetEntries()) == comparision1[38] && abs(double(W_vs_time_mm->GetSumOfWeights()) - comparision1[39]) < epsilon);
  error_list_bool.push_back(U_vs_time_mm_bool);
  error_list_bool.push_back(V_vs_time_mm_bool);
  error_list_bool.push_back(W_vs_time_mm_bool);
  
  std::vector<std::string> error_list = { "Title_raw", "V_Strip", "Charge_strip" "Title_raw_time", "Time_bin", "Charge_time_bin", "Title_raw_V", "Time_bin_V", "V_Strip_", "Title_raw_V_", "Time_mm", "V_Strip_mm", "U_raw", "V_raw", "W_raw", "U_mm", "V_mm", "W_mm", "TTime", "UTime", "VTime", "WTime", "TTime_mm", "UTime_mm", "VTime_mm", "WTime_mm", "U_vs_time", "V_vs_time", "W_vs_time", "U_vs_time_mm", "V_vs_time_mm", "W_vs_time_mm", "charge", "charge_DIR_U", "charge_DIR_U_strip1", "charge_DIR_U_sec1_strip_58", "charge_time_cell128", "charge_DIR_U_time_cell128", "charge_DIR_U_sec1_time_cell28", "max_charge", "max_charge_DIR_U", "max_charge_DIR_U_strip1", "max_charge_DIR_U_sec1_strip58", "max_charge_time", "max_charge_channel", "max_charge_time_DIR_U", "max_charge_strip_DIR_U", "min_time", "max_time", "min_time_DIR_U", "max_time_DIR_U", "min_strip_DIR_U", "max_strip_DIR_U", "multiplicity_total", "multiplicity_DIR_U", "multiplicity_DIR_V", "multiplicity_DIR_W", "multiplicity_DIR_U_0", "multiplicity_DIR_V_0", "multiplicity_DIR_W_0", "Nhits_total", "Nhits_DIR_U", "Nhits_DIR_V", "Nhits_DIR_W", "Nhits_DIR_U_0", "Nhits_DIR_U_0_70", "NhitsMerged_DIR_U_70", "Channels" ,"Channels_raw" };
  
  bool charge = (aEventPtr->GetTotalCharge(-1, -1, -1, -1, filterType) - comparision2[0]) < epsilon;                             
  bool charge_DIR_U = (aEventPtr->GetTotalCharge(DIR_U, -1, -1, -1, filterType) - comparision2[1]) < epsilon;                    
  bool charge_DIR_U_strip1 = (aEventPtr->GetTotalCharge(DIR_U, -1, 1, -1, filterType) - comparision2[2]) < epsilon;              
  bool charge_DIR_U_sec1_strip_58 = (aEventPtr->GetTotalCharge(DIR_U, 1, 58, -1, filterType) - comparision2[3]) < epsilon;       
  bool charge_time_cell128 = (aEventPtr->GetTotalCharge(-1, -1, -1, 128, filterType) - comparision2[4]) < epsilon;               
  bool charge_DIR_U_time_cell128 = (aEventPtr->GetTotalCharge(DIR_U, -1, -1, 128, filterType) - comparision2[5]) < epsilon;      
  bool charge_DIR_U_sec1_time_cell28 = (aEventPtr->GetTotalCharge(DIR_U, 1,  -1, 128, filterType) - comparision2[6]) < epsilon;  
  error_list_bool.push_back(charge);
  error_list_bool.push_back(charge_DIR_U);
  error_list_bool.push_back(charge_DIR_U_strip1);
  error_list_bool.push_back(charge_DIR_U_sec1_strip_58);
  error_list_bool.push_back(charge_time_cell128);
  error_list_bool.push_back(charge_DIR_U_time_cell128);
  error_list_bool.push_back(charge_DIR_U_sec1_time_cell28);

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

  std::vector<double> comparison_none2 = { 520098.542683 , 155908.768293 , 1407.329268293 , 367.042682927 , 2890.25609756 , 950.109756098 , -79.378 , 1735.09146341 , 1392.21341463 , 23.7256 , 22.2561 , 58 , 0 ,  58 , 65 , 2 , 500 , 3 , 501 , 1 , 132 , 1018 , 131 , 224 , 225 , 0 , 74 , 74 , 290917 , 65868  , 112275  , 112774  , 0 , 0 , 499};

  std::vector<double> comparison_threshold2 = { 794935.22561 , 226979.29878 , 0 , 0 , 3425.77439024 , 1032.90243902 , 0 , 1732.16463415 , 1385.27439024 , 0 , 0 , 58 , 0 , 58 , 65 , 33 , 251 , 37 , 250 , 46 , 81 , 112 , 25 , 43 , 29 , 0 , 28 , 15 , 5874 , 1747 , 2266 , 1861 , 0 , 0 , 104};

  std::vector<double> comparison_none1 = { 155908 ,155908.76829307 ,176881, 176881.810976, 188406, 188406.182927, 155908, 155908.768293 ,176881, 176882 - 0.189024, 188406, 188406 + 0.182927, 520098, 520098.542683, 155908, 155908.768293, 176881, 176881.81097561, 187307, 187307.96341464,
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
