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

#include "dataEventTPC.h" // File with data for tests and directory of data files

std::vector<bool> error_list_bool;  // Vector storing test results
double epsilon = 1e-5;  // Value used to compare double values

  
  // get1DProjection Titles Test
    void get1DProjection_Titles_Test(std::shared_ptr<EventTPC> aEventPtr, std::map<std::string, std::string> Test_Reference_Titles) {
        for (auto projections : Projections1D) {
            for (auto filter : FilterTypes) {
                std::shared_ptr<TH1D> Test = aEventPtr->get1DProjection(projections.first, filter.first, scale_type::raw);
                std::string Test_String = "get1DProjection(" + projections.second + ", " + filter.second + ", scale_type::raw)";
                if (bool(std::string(Test->GetTitle()) == Test_Reference_Titles[Test_String + "->GetTitle()"])) { error_list_bool.push_back(true); }
                else { std::cout << KRED << Test_String + "->GetTitle()" << RST << std::endl; error_list_bool.push_back(false); }
                if (bool(std::string(Test->GetXaxis()->GetTitle()) == Test_Reference_Titles[Test_String + "->GetXaxis()->GetTitle()"])) { error_list_bool.push_back(true); }
                else { std::cout << KRED << Test_String + "->GetXaxis()->GetTitle()" << RST << std::endl; error_list_bool.push_back(false); }
                if (bool(std::string(Test->GetYaxis()->GetTitle()) == Test_Reference_Titles[Test_String + "->GetYaxis()->GetTitle()"])) { error_list_bool.push_back(true); }
                else { std::cout << KRED << Test_String + "->GetYaxis()->GetTitle()" << RST << std::endl; error_list_bool.push_back(false); }
            }
        }
    }

  // get2DProjection Titles Test
    void get2DProjection_Titles_Test(std::shared_ptr<EventTPC> aEventPtr, std::map<std::string, std::string> Test_Reference_Titles) {
        for (auto scale : ScaleTypes) {
            for (auto filter : FilterTypes) {
                std::shared_ptr<TH2D> Test = aEventPtr->get2DProjection(projection_type::DIR_TIME_V, filter.first, scale.first);
                std::string Test_String = "get2DProjection(projection_type::DIR_TIME_V, " + filter.second + ", " + scale.second + ")";
                if (bool(std::string(Test->GetTitle()) == Test_Reference_Titles[Test_String + "->GetTitle()"])) { error_list_bool.push_back(true); }
                else { std::cout << KRED << Test_String + "->GetTitle()" << RST << std::endl; error_list_bool.push_back(false); }
                if (bool(std::string(Test->GetXaxis()->GetTitle()) == Test_Reference_Titles[Test_String + "->GetXaxis()->GetTitle()"])) { error_list_bool.push_back(true); }
                else { std::cout << KRED << Test_String + "->GetXaxis()->GetTitle()" << RST << std::endl; error_list_bool.push_back(false); }
                if (bool(std::string(Test->GetYaxis()->GetTitle()) == Test_Reference_Titles[Test_String + "->GetYaxis()->GetTitle()"])) { error_list_bool.push_back(true); }
                else { std::cout << KRED << Test_String + "->GetYaxis()->GetTitle()" << RST << std::endl; error_list_bool.push_back(false); }
            }
        }
    }

  // get1DProjection Test
    void get1DProjection_Test(std::shared_ptr<EventTPC> aEventPtr, std::map<std::string, double> Test_Reference, std::map<std::string, std::string> Test_Reference_Titles) {
        for (auto projection : ProjectionTypes1D) {
            for (auto filter : FilterTypes) {
                for (auto scale : ScaleTypes) {
                    std::shared_ptr<TH1D> Test = aEventPtr->get1DProjection(std::get<0>(projection), std::get<0>(filter), std::get<0>(scale));
                    std::string Test_String = "get1DProjection(" + std::get<1>(projection) + ", " + std::get<1>(filter) + ", " + std::get<1>(scale) + ")";
                    if (std::string(Test->GetName()) == Test_Reference_Titles[Test_String + "->GetName()"] &&
                        int(Test->GetEntries()) == Test_Reference[Test_String + "->GetEntries()"] &&
                        abs(double(Test->GetSumOfWeights())) - Test_Reference[Test_String + "->GetSumOfWeights()"] < epsilon) {
                        error_list_bool.push_back(true);
                    }
                    else { std::cout << KRED << Test_String << RST << std::endl; error_list_bool.push_back(false); }
                }
            }
        }
    }
  
  // get2DProjection Test
    void get2DProjection_Test(std::shared_ptr<EventTPC> aEventPtr, std::map<std::string, double> Test_Reference, std::map<std::string, std::string> Test_Reference_Titles) {
        for (auto projection : ProjectionTypes2D) {
            for (auto filter : FilterTypes) {
                for (auto scale : ScaleTypes) {
                    std::shared_ptr<TH2D> Test = aEventPtr->get2DProjection(std::get<0>(projection), std::get<0>(filter), std::get<0>(scale));
                    std::string Test_String = "get2DProjection(" + std::get<1>(projection) + ", " + std::get<1>(filter) + ", " + std::get<1>(scale) + ")";
                    if (std::string(Test->GetName()) == Test_Reference_Titles[Test_String + "->GetName()"] &&
                        int(Test->GetEntries()) == Test_Reference[Test_String + "->GetEntries()"] &&
                        abs(double(Test->GetSumOfWeights())) - Test_Reference[Test_String + "->GetSumOfWeights()"] < epsilon) {
                        error_list_bool.push_back(true);
                    }
                    else { std::cout << KRED << Test_String << RST << std::endl; error_list_bool.push_back(false); }
                }
            }
        }
        std::string Test_Channel_String = "GetChannels(0,0)"; std::string Test_Channel_raw_String = "GetChannels_raw(0,0)";
        std::shared_ptr<TH2D> Test_Channel = aEventPtr->GetChannels(0, 0); std::shared_ptr<TH2D> Test_Channel_raw = aEventPtr->GetChannels_raw(0, 0);
        // GetChannels(0,0) Test
        if (std::string(Test_Channel->GetName()) == Test_Reference_Titles[Test_Channel_String + "->GetName()"] &&
            int(Test_Channel->GetEntries()) == Test_Reference[Test_Channel_String + "->GetEntries()"] &&
            abs(double(Test_Channel->GetSumOfWeights()) - Test_Reference[Test_Channel_String + "->GetSumOfWeights()"]) < epsilon) {
            error_list_bool.push_back(true);
        }
        else { std::cout << KRED << Test_Channel_String << RST << std::endl; error_list_bool.push_back(false); }
        // GetChannels_raw(0,0) Test
        if (std::string(Test_Channel_raw->GetName()) == Test_Reference_Titles[Test_Channel_raw_String + "->GetName()"] &&
            int(Test_Channel_raw->GetEntries()) == Test_Reference[Test_Channel_raw_String + "->GetEntries()"] &&
            abs(double(Test_Channel_raw->GetSumOfWeights()) - Test_Reference[Test_Channel_raw_String + "->GetSumOfWeights()"]) < epsilon) {
            error_list_bool.push_back(true);
        }
        else { std::cout << KRED << Test_Channel_raw_String << RST << std::endl; error_list_bool.push_back(false); }
    }
    
        

  
  // GetTotalCharge Test
    void GetTotalCharge_Test(std::shared_ptr<EventTPC> aEventPtr, std::map<std::string, double> Test_Reference) {
        for (auto charge : Test_GetTotalCharge) {
            for (auto filter : FilterTypes) {
                std::string Test_String = "GetTotalCharge" + charge.second + ", " + filter.second + ")";
                double Test = aEventPtr->GetTotalCharge(std::get<0>(charge.first), std::get<1>(charge.first), std::get<2>(charge.first), std::get<3>(charge.first), filter.first);
                if (bool((Test - Test_Reference[Test_String]) < epsilon)) { error_list_bool.push_back(true); }
                else { std::cout << KRED << Test_String << RST << std::endl; error_list_bool.push_back(false); }
            }
        }
    }

  
  // GetMaxCharge Test
    void GetMaxCharge_Test(std::shared_ptr<EventTPC> aEventPtr, std::map<std::string, double> Test_Reference) {
        for (auto MaxCharge : Test_GetMaxCharge) {
            for (auto filter : FilterTypes) {
                std::string Test_String = "GetMaxCharge" + MaxCharge.second + ", " + filter.second + ")";
                double Test = aEventPtr->GetMaxCharge(std::get<0>(MaxCharge.first), std::get<1>(MaxCharge.first), std::get<2>(MaxCharge.first), filter.first);
                if (bool((Test - Test_Reference[Test_String]) < epsilon)) { error_list_bool.push_back(true); }
                else { std::cout << KRED << Test_String << RST << std::endl; error_list_bool.push_back(false); }
            }
        }
    }

  // GetMaxChargePos Test
    void GetMaxChargePos_Test(std::shared_ptr<EventTPC> aEventPtr, std::map<std::string, double> Test_Reference) {
        for (auto MaxChargePos : Test_GetMaxChargePos) {
            for (auto filter : FilterTypes) {
                std::string Test_String = "GetMaxChargePos(" + MaxChargePos.second + ", " + filter.second + ")";
                std::tie(maxTime, maxStrip) = aEventPtr->GetMaxChargePos(MaxChargePos.first, filter.first);
                if (bool(maxTime - Test_Reference[Test_String + "->maxTime"] == 0)) { error_list_bool.push_back(true); }
                else { std::cout << KRED << Test_String + "->maxTime" << RST << std::endl; error_list_bool.push_back(false); }
                if (bool(maxStrip - Test_Reference[Test_String + "->maxStrip"] == 0)) { error_list_bool.push_back(true); }
                else { std::cout << KRED << Test_Reference[Test_String + "->maxStrip"] << RST << std::endl; error_list_bool.push_back(false); }
            }
        }
    }

  // GetSignalRange Test
    void GetSignalRange_Test(std::shared_ptr<EventTPC> aEventPtr, std::map<std::string, double> Test_Reference) {
        for (auto MaxChargePos : Test_GetMaxChargePos) {
            for (auto filter : FilterTypes) {
                std::string Test_String = "GetSignalRange(" + MaxChargePos.second + ", " + filter.second + ")";
                std::tie(minTime, maxTime, minStrip, maxStrip) = aEventPtr->GetSignalRange(MaxChargePos.first, filter.first);
                if (bool(minTime - Test_Reference[Test_String + "->minTime"] == 0)) { error_list_bool.push_back(true); }
                else { std::cout << KRED << Test_String + "->minTime" << RST << std::endl; error_list_bool.push_back(false); }
                if (bool(maxTime - Test_Reference[Test_String + "->maxTime"] == 0)) { error_list_bool.push_back(true); }
                else { std::cout << KRED << Test_String + "->maxTime" << RST << std::endl; error_list_bool.push_back(false); }
                if (bool(minStrip - Test_Reference[Test_String + "->minStrip"] == 0)) { error_list_bool.push_back(true); }
                else { std::cout << KRED << Test_String + "->minStrip" << RST << std::endl; error_list_bool.push_back(false); }
                if (bool(maxStrip - Test_Reference[Test_String + "->maxStrip"] == 0)) { error_list_bool.push_back(true); }
                else { std::cout << KRED << Test_String + "->maxStrip" << RST << std::endl; error_list_bool.push_back(false); }
            }
        }
    }

  // GetMultiplicity Test
    void GetMultiplicity_Test(std::shared_ptr<EventTPC> aEventPtr, std::map<std::string, double> Test_Reference) {
        for (auto multiplicity : Test_GetMultiplicity) {
            for (auto filter : FilterTypes) {
                std::string Test_String = "GetMultiplicity" + multiplicity.second + ", " + filter.second + ")";
                double Test = aEventPtr->GetMultiplicity(std::get<0>(multiplicity.first), std::get<1>(multiplicity.first), std::get<2>(multiplicity.first), std::get<3>(multiplicity.first), filter.first);
                if (bool(Test - Test_Reference[Test_String] == 0)) { error_list_bool.push_back(true); }
                else { std::cout << KRED << Test_String << RST << std::endl; error_list_bool.push_back(false); }
            }
        }
    }
  


/////////////////////////////////////
/////////////////////////////////////
int main(int argc, char *argv[]) {

  std::shared_ptr<EventSourceBase> myEventSource = std::make_shared<EventSourceMultiGRAW>(geometryFileName);  // 
  myEventSource->loadDataFile(dataFileName);                                                                  // Loading Data File
  std::cout << "File with " << myEventSource->numberOfEntries() << " frames opened." << std::endl;            // 

  auto myEventPtr = myEventSource->getCurrentEvent();
  for(int i=89;i<90;++i){  // Testing a specific event
    myEventSource->loadFileEntry(i);
  
    std::cout<<myEventPtr->GetEventInfo()<<std::endl;
    get1DProjection_Titles_Test(myEventPtr, Test_Reference_Titles);
    get2DProjection_Titles_Test(myEventPtr, Test_Reference_Titles);
    get1DProjection_Test(myEventPtr, Test_Reference,Test_Reference_Titles);
    get2DProjection_Test(myEventPtr, Test_Reference,Test_Reference_Titles);
    GetTotalCharge_Test(myEventPtr, Test_Reference);
    GetMaxCharge_Test(myEventPtr, Test_Reference);
    GetMaxChargePos_Test(myEventPtr, Test_Reference);
    GetSignalRange_Test(myEventPtr, Test_Reference);
    GetMultiplicity_Test(myEventPtr, Test_Reference);
  }
  int check = error_list_bool.size(); 
  for (std::vector<bool>::iterator it = error_list_bool.begin(); it != error_list_bool.end(); ++it) { check -= *it; } // checking if all Tests returns correct values

  if(check>0){return -1;}
  return 0;
}
/////////////////////////////////////
/////////////////////////////////////
