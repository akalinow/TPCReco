#include "TPCReco/ConfigManager.h"

#include <stdexcept>
#include "gtest/gtest.h"

//////////////////////////
//////////////////////////
TEST(ConfigManagerTest, emptyCmdLine) {

  int argc = 1;
  char *argv[] = {(char*)"ConfigManager_tst"};
  ConfigManager cm;
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);

}
//////////////////////////
//////////////////////////
TEST(ConfigManagerTest, unknownCmdLineParam) {

  int argc = 3;
  char *argv[] = {(char*)"ConfigManager_tst", 
                  (char*)"--AAAA", (char*)"12.1"};
  ConfigManager cm;
  
   EXPECT_THROW({
        try
        {
        boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);         
        }
        catch( const std::exception& e )
        {
            // and this tests that it has the correct message
            EXPECT_STREQ( "unrecognised option '--AAAA'", e.what() );
            throw;
        }
    }, std::exception );

}
//////////////////////////
//////////////////////////
TEST(ConfigManagerTest, defaultParam) {

  int argc = 1;
  char *argv[] = {(char*)"ConfigManager_tst"};
  ConfigManager cm;
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);
  double beamEnergy = myConfig.get<double>("beamParameters.energy");
  EXPECT_DOUBLE_EQ(beamEnergy, 0.0);

}
//////////////////////////
//////////////////////////
TEST(ConfigManagerTest, paramFromCmdLine) {

  int argc = 3;
  char *argv[] = {(char*)"ConfigManager_tst", 
                  (char*)"--beamParameters.energy",(char*)"12.1"};
  ConfigManager cm;
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);
  double beamEnergy = myConfig.get<double>("beamParameters.energy");
  EXPECT_DOUBLE_EQ(beamEnergy, 12.1);

}
//////////////////////////
//////////////////////////
TEST(ConfigManagerTest, paramFromJSON) {

  int argc = 3;
  
  std::string testJSON = std::string(std::getenv("HOME"))+".tpcreco/config/test.json";
  char *argv[] = {(char*)"ConfigManager_tst", 
                  (char*)"--meta.configJson",const_cast<char *>(testJSON.data())};
  ConfigManager cm;
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);
  double beamEnergy = myConfig.get<double>("beamParameters.energy");
  EXPECT_DOUBLE_EQ(beamEnergy, 99.9);

}
//////////////////////////
//////////////////////////
TEST(ConfigManagerTest, helpCmdLine) {

  int argc = 2;
  char *argv[] = {(char*)"ConfigManager_tst", (char*)"--help"};
  ConfigManager cm;
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);

}
//////////////////////////
//////////////////////////