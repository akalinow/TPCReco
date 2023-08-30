#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <iomanip>
#include <unistd.h>
#include "gtest/gtest.h"

#include "TPCReco/EventSourceFactory.h"
#include "TPCReco/ConfigManager.h"
#include "TPCReco/colorText.h"

#include "dataEventTPC.h" 

class EventTPCTest : public ::testing::Test {
public:
  static std::shared_ptr<EventTPC> myEventPtr;

  static void SetUpTestSuite() {
  
    std::string testJSON = std::string(std::getenv("HOME"))+".tpcreco/config/test.json";
    int argc = 3;
    char *argv[] = {(char*)"ConfigManager_tst", 
                  (char*)"--meta.configJson",const_cast<char *>(testJSON.data())};
                
    ConfigManager cm;
    boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);
    int status = chdir("../../resources");     
    std::shared_ptr<EventSourceBase> myEventSource = EventSourceFactory::makeEventSourceObject(myConfig);
  
    myEventPtr = myEventSource->getCurrentEvent(); 
    myEventSource->loadFileEntry(9); 
  }
  static void TearDownTestSuite() {}
};

std::shared_ptr<EventTPC> EventTPCTest::myEventPtr(0);


///////////////////////////////////////  
///////////////////////////////////////  
TEST_F(EventTPCTest, get1DProjection_Titles) {
 for (auto projections : Projections1D) {
            for (auto filter : FilterTypes) {
                std::shared_ptr<TH1D> Test = myEventPtr->get1DProjection(projections.first, filter.first, scale_type::raw);
                std::string Test_String = "get1DProjection(" + projections.second + ", " + filter.second + ", scale_type::raw)";
                EXPECT_EQ(std::string(Test->GetTitle()), Test_Reference_Titles.at(Test_String + "->GetTitle()"));
                EXPECT_EQ(std::string(Test->GetXaxis()->GetTitle()), Test_Reference_Titles.at(Test_String + "->GetXaxis()->GetTitle()"));
                EXPECT_EQ(std::string(Test->GetYaxis()->GetTitle()), Test_Reference_Titles.at(Test_String + "->GetYaxis()->GetTitle()"));
            }
        }
}
///////////////////////////////////////  
///////////////////////////////////////  
TEST_F(EventTPCTest, get2DProjection_Titles) {
        for (auto scale : ScaleTypes) {
            for (auto filter : FilterTypes) {
                std::shared_ptr<TH2D> Test = myEventPtr->get2DProjection(projection_type::DIR_TIME_V, filter.first, scale.first);
                std::string Test_String = "get2DProjection(projection_type::DIR_TIME_V, " + filter.second + ", " + scale.second + ")";
                EXPECT_EQ(std::string(Test->GetTitle()), Test_Reference_Titles.at(Test_String + "->GetTitle()"));
                EXPECT_EQ(std::string(Test->GetXaxis()->GetTitle()), Test_Reference_Titles.at(Test_String + "->GetXaxis()->GetTitle()"));
                EXPECT_EQ(std::string(Test->GetYaxis()->GetTitle()), Test_Reference_Titles.at(Test_String + "->GetYaxis()->GetTitle()"));
            }
        }
}
///////////////////////////////////////  
///////////////////////////////////////      
TEST_F(EventTPCTest, get1DProjection) {    
        for (auto projection : ProjectionTypes1D) {
            for (auto filter : FilterTypes) {
                for (auto scale : ScaleTypes) {
                    std::shared_ptr<TH1D> Test = myEventPtr->get1DProjection(std::get<0>(projection), std::get<0>(filter), std::get<0>(scale));
                    std::string Test_String = "get1DProjection(" + std::get<1>(projection) + ", " + std::get<1>(filter) + ", " + std::get<1>(scale) + ")";
                    EXPECT_EQ(std::string(Test->GetName()), Test_Reference_Titles.at(Test_String + "->GetName()"));
                    EXPECT_DOUBLE_EQ(Test->GetEntries(), Test_Reference.at(Test_String + "->GetEntries()"));
                    EXPECT_DOUBLE_EQ(Test->GetSumOfWeights(), Test_Reference.at(Test_String + "->GetSumOfWeights()"));
                    }
                }
            }
        }  
///////////////////////////////////////  
///////////////////////////////////////          
TEST_F(EventTPCTest, get2DProjection) {  
    for (auto projection : ProjectionTypes2D) {
            for (auto filter : FilterTypes) {
                for (auto scale : ScaleTypes) {
                    std::shared_ptr<TH2D> Test = myEventPtr->get2DProjection(std::get<0>(projection), std::get<0>(filter), std::get<0>(scale));
                    std::string Test_String = "get2DProjection(" + std::get<1>(projection) + ", " + std::get<1>(filter) + ", " + std::get<1>(scale) + ")";
                    EXPECT_EQ(std::string(Test->GetName()), Test_Reference_Titles.at(Test_String + "->GetName()"));
                    EXPECT_DOUBLE_EQ(Test->GetEntries(), Test_Reference.at(Test_String + "->GetEntries()"));
                    EXPECT_DOUBLE_EQ(Test->GetSumOfWeights(), Test_Reference.at(Test_String + "->GetSumOfWeights()"));
            }   
        }
    }
}
///////////////////////////////////////  
///////////////////////////////////////  
TEST_F(EventTPCTest, getChannels) { 
    std::string Test_String = "GetChannels(0,0)"; 
    std::shared_ptr<TH2D> Test = myEventPtr->GetChannels(0, 0);
    EXPECT_EQ(std::string(Test->GetName()), Test_Reference_Titles.at(Test_String + "->GetName()"));
    EXPECT_EQ(Test->GetEntries(), Test_Reference.at(Test_String + "->GetEntries()"));
    EXPECT_DOUBLE_EQ(Test->GetSumOfWeights(), Test_Reference.at(Test_String + "->GetSumOfWeights()"));
}
///////////////////////////////////////  
///////////////////////////////////////  
TEST_F(EventTPCTest, getChannels_raw) { 
    std::string Test_String = "GetChannels_raw(0,0)"; 
    std::shared_ptr<TH2D> Test = myEventPtr->GetChannels_raw(0, 0);
    EXPECT_EQ(std::string(Test->GetName()), Test_Reference_Titles.at(Test_String + "->GetName()"));
    EXPECT_EQ(Test->GetEntries(), Test_Reference.at(Test_String + "->GetEntries()"));
    EXPECT_DOUBLE_EQ(Test->GetSumOfWeights(), Test_Reference.at(Test_String + "->GetSumOfWeights()"));
}
///////////////////////////////////////  
///////////////////////////////////////            
TEST_F(EventTPCTest, GetTotalCharge) { 
        for (auto charge : Test_GetTotalCharge) {
            for (auto filter : FilterTypes) {
                std::string Test_String = "GetTotalCharge(" + charge.second + ", " + filter.second+")";                 
                double Test = myEventPtr->GetTotalCharge(std::get<0>(charge.first), std::get<1>(charge.first), std::get<2>(charge.first), std::get<3>(charge.first), filter.first);                                
                EXPECT_DOUBLE_EQ(Test, Test_Reference.at(Test_String));                
            }
        }
}
///////////////////////////////////////  
///////////////////////////////////////  
TEST_F(EventTPCTest, GetMaxCharge) { 
        for (auto maxCharge : Test_GetMaxCharge) {
            for (auto filter : FilterTypes) {
                std::string Test_String = "GetMaxCharge(" + maxCharge.second + ", " + filter.second+")"; 
                double Test = myEventPtr->GetMaxCharge(std::get<0>(maxCharge.first), std::get<1>(maxCharge.first), std::get<2>(maxCharge.first), filter.first);  
                EXPECT_DOUBLE_EQ(Test, Test_Reference.at(Test_String));
            }
        }
}
///////////////////////////////////////  
///////////////////////////////////////  
TEST_F(EventTPCTest, GetMaxChargePos) { 
        for (auto MaxChargePos : Test_GetMaxChargePos) {
            for (auto filter : FilterTypes) {
                std::string Test_String = "GetMaxChargePos(" + MaxChargePos.second + ", " + filter.second + ")";
                std::tie(maxTime, maxStrip) = myEventPtr->GetMaxChargePos(MaxChargePos.first, filter.first);  
                EXPECT_EQ(maxTime, Test_Reference.at(Test_String+"->maxTime"));
                EXPECT_EQ(maxStrip, Test_Reference.at(Test_String+"->maxStrip"));
            }
        }
}
///////////////////////////////////////  
///////////////////////////////////////  
TEST_F(EventTPCTest, GetSignalRange) { 
        for (auto maxChargePos : Test_GetMaxChargePos) {
            for (auto filter : FilterTypes) {                
                std::string Test_String = "GetSignalRange(" + maxChargePos.second + ", " + filter.second + ")";                
                std::tie(minTime, maxTime, minStrip, maxStrip) = myEventPtr->GetSignalRange(maxChargePos.first, filter.first);
                EXPECT_EQ(minTime, Test_Reference.at(Test_String+"->minTime"));
                EXPECT_EQ(maxTime, Test_Reference.at(Test_String+"->maxTime"));
                EXPECT_EQ(minStrip, Test_Reference.at(Test_String+"->minStrip"));
                EXPECT_EQ(maxStrip, Test_Reference.at(Test_String+"->maxStrip"));
            }
        }
}
///////////////////////////////////////  
///////////////////////////////////////  
TEST_F(EventTPCTest, GetMultiplicity) { 
        for (auto multiplicity : Test_GetMultiplicity) {
            for (auto filter : FilterTypes) {                                
                std::string Test_String = "GetMultiplicity" + multiplicity.second + ", " + filter.second + ")";
                int Test = myEventPtr->GetMultiplicity(std::get<0>(multiplicity.first), std::get<1>(multiplicity.first), 
                                                       std::get<2>(multiplicity.first), std::get<3>(multiplicity.first), 
                                                       filter.first);
                EXPECT_EQ(Test, Test_Reference.at(Test_String));               
            }
        }
}
///////////////////////////////////////  
///////////////////////////////////////  