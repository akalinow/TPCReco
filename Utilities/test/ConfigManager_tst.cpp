#include "TPCReco/ConfigManager.h"

#include <boost/filesystem.hpp>
#include <stdexcept>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include "gtest/gtest.h"

//////////////////////////
//////////////////////////
namespace fs = boost::filesystem;
class ConfigManagerTest : public ::testing::Test {
public:
  static std::string directory;
  static std::string dummyAllowedOptionsJson;
  static std::string dummyInvalidAllowedOptionsJson1;
  static std::string dummyInvalidAllowedOptionsJson2;
  static std::string dummyValidJson1;
  static std::string dummyValidJson2;
  static std::string dummyValidJson3;
  static std::string dummyInvalidJson1;
  static std::string dummyInvalidJson2;
  static std::string dummyInvalidJson3;

  static void createDummyJson(const std::string &name, const std::string &config) {
    if(name.rfind(".json")!=name.size()-5 || name.size()<=5) throw;
    std::ofstream f(name);
    f << config;
    f.close();
  }
  static void createDummyAllowedOptionsJsonFiles() {
    const std::string nameOpt1=directory + dummyAllowedOptionsJson;
    const std::string nameOpt2=directory + dummyInvalidAllowedOptionsJson1;
    const std::string nameOpt3=directory + dummyInvalidAllowedOptionsJson2;
    const std::string configOpt1{R"####(
{
    "configJson":{
        "group":"meta",
        "type" : "vector<string>",
        "defaultValue" : "",
        "description" : "dummy JSON file(s) test"
    },
    "scalar":{
        "group":"group",
        "type" : "string",
        "defaultValue" : "A B C D",
        "description" : "Test string"
    },
    "vectorI":{
        "group":"group1",
        "type" : "vector<int>",
        "defaultValue" : [ -1, "-1*2", "-1*3" ],
        "description" : "Test vector<int>"
    },
    "vectorU":{
        "group":"group1",
        "type" : "vector<unsigned int>",
        "defaultValue" : [ 1, "1+1", "1+1+1" ],
        "description" : "Test vector<int>"
    },
    "vectorF":{
        "group":"group2",
        "type" : "vector<float>",
        "defaultValue" : [ 1.1, "1.1+1.1", "3*1.1" ],
        "description" : "Test vector<float>"
    },
    "vectorD":{
        "group":"group2",
        "type" : "vector<double>",
        "defaultValue" : [ 1.1e-11, 2.2e-12, 3.3e-13 ],
        "description" : "Test vector<double>"
    },
    "vectorB":{
        "group":"group3",
        "type" : "vector<bool>",
        "defaultValue" : [ true, false, "true||false" ],
        "description" : "Test vector<float>"
    },
    "vectorS":{
        "group":"group4",
        "type" : "vector<string>",
        "defaultValue" : [ "A A", "BB BB", "CCC CCC" ],
        "description" : "Test vector<string>"
    },
    "paramTwoPi":{
        "group":"someAnalysis",
        "type" : "double",
        "defaultValue" : "M_PI*2",
        "description" : "Test scalar option"
    },
    "param1000":{
        "group":"someAnalysis",
        "type" : "int",
        "defaultValue" : 1000,
        "description" : "Test scalar option"
    },
    "flagTrue":{
        "group":"someAnalysis",
        "type" : "bool",
        "defaultValue" : true,
        "description" : "Test switch flag"
    },
    "paramPtree":{
        "group":"someAnalysis",
        "type" : "ptree",
        "defaultValue" : { "parameter555": "555" },
        "description" : "Test BOOST ptree"
    },
    "vectorPtree":{
        "group":"someAnalysis",
        "type" : "vector<ptree>",
        "defaultValue" : [],
        "description" : "Test BOOST vector<ptree>"
    }
}
    )####"}; // MARK=#### allows to use () for params/expressions
    const std::string configOpt2{R"####(
{
    "vectorWRONG":{
        "group":"--group.WRONG.name",
        "type" : "vector<int>",
        "defaultValue" : [ ],
        "description" : "Test vector<int>"
    }
}
    )####"}; // MARK=#### allows to use () for params/expressions
    const std::string configOpt3{R"####(
{
    "configJson":{
        "group":"meta",
        "type" : "vector<string>",
        "defaultValue" : "",
        "description" : "dummy JSON file(s) test"
    },
    "paramPtree":{
        "group":"someAnalysis",
        "type" : "ptree",
        "defaultValue" : { "parameter555": "555" },
        "description" : "Test BOOST ptree"
    },
    "paramWRONG":{
        "group":"someAnalysis.paramPtree",
        "type" : "string",
        "defaultValue" : "WRONG",
        "description" : "Test of conflicting names"
    }
}
    )####"}; // MARK=#### allows to use () for params/expressions
    createDummyJson(nameOpt1, configOpt1);
    createDummyJson(nameOpt2, configOpt2);
    createDummyJson(nameOpt3, configOpt3);
  }

  static void createDummyJsonFiles() {
    const std::string nameJ1=directory + dummyValidJson1;
    const std::string nameJ2=directory + dummyValidJson2;
    const std::string nameJ3=directory + dummyValidJson3;
    const std::string nameIJ1=directory + dummyInvalidJson1;
    const std::string nameIJ2=directory + dummyInvalidJson2;
    const std::string nameIJ3=directory + dummyInvalidJson3;
    const std::string configJ1{R"####(
{
    "group1":{
        "vectorI": [ ]
    },
    "group2":{
        "vectorD": [ 0.0, "sin(M_PI/2)" ]
    }
}
    )####"}; // MARK=#### allows to use () for params/expressions
    const std::string configJ2{R"####(
{
    "group1":{
        "vectorI": [5, 5, 5, 5]
    },
    "group4":{
        "vectorS": ["5 5", "55 55", "555 555", "5555 5555"]
    }
}
    )####"}; // MARK=#### allows to use () for params/expressions
    const std::string configJ3{R"####(
{   "someAnalysis" : {
        "paramPtree" : {
            "param1" : 1,
            "param2" : 2,
            "vectorABC" : [ "A", "B", "C" ],
            "paramPtree" : {
                "paramAA" : "AA",
                "vector56" : [ 5, 6 ]
            }
        },
        "vectorPtree" : [
            { "paramPtree1" : {
                 "paramA": "a"
              }
            },
            { "paramPtree2" : "b"
            }
        ]
    }
}
    )####"}; // MARK=#### allows to use () for params/expressions
    const std::string configIJ1{R"####(
{
    "group2":{
        "vectorUNKNOWN": [ ]
    }
}
    )####"}; // MARK=#### allows to use () for params/expressions
    const std::string configIJ2{R"####(
{
    "someAnalysis":{
        "paramPtree":[
            { "paramWRONG1": "Vector element 1" },
            { "paramWRONG2": "Vector element 2" }
	]
    }
}
    )####"}; // MARK=#### allows to use () for params/expressions
    const std::string configIJ3{R"####(
{
    "someAnalysis":{
        "vectorPtree":{
	    "paramWRONG1": "Non-vector element 1",
            "paramWRONG2": "Non-vector element 2"
	}
    }
}
    )####"}; // MARK=#### allows to use () for params/expressions
    createDummyJson(nameJ1, configJ1);
    createDummyJson(nameJ2, configJ2);
    createDummyJson(nameJ3, configJ3);
    createDummyJson(nameIJ1, configIJ1);
    createDummyJson(nameIJ2, configIJ2);
    createDummyJson(nameIJ3, configIJ3);
  }

  static void TearDownTestSuite() { fs::remove_all(directory); }

  static void SetUpTestSuite() {
    directory = (fs::temp_directory_path() / fs::unique_path()).string() +
                fs::path::preferred_separator;
    dummyAllowedOptionsJson = "dummyAllowedOptions.json";
    dummyInvalidAllowedOptionsJson1 = "dummyInvalidAllowedOptions1.json";
    dummyInvalidAllowedOptionsJson2 = "dummyInvalidAllowedOptions2.json";
    dummyValidJson1 = "test1.json";
    dummyValidJson2 = "test2.json";
    dummyValidJson3 = "test3.json";
    dummyInvalidJson1 = "test4.json";
    dummyInvalidJson2 = "test5.json";
    dummyInvalidJson3 = "test6.json";
    fs::create_directories(directory);
    createDummyJsonFiles();
    createDummyAllowedOptionsJsonFiles();
  }
};
std::string ConfigManagerTest::directory = "";
std::string ConfigManagerTest::dummyAllowedOptionsJson = "";
std::string ConfigManagerTest::dummyInvalidAllowedOptionsJson1 = "";
std::string ConfigManagerTest::dummyInvalidAllowedOptionsJson2 = "";
std::string ConfigManagerTest::dummyValidJson1 = "";
std::string ConfigManagerTest::dummyValidJson2 = "";
std::string ConfigManagerTest::dummyValidJson3 = "";
std::string ConfigManagerTest::dummyInvalidJson1 = "";
std::string ConfigManagerTest::dummyInvalidJson2 = "";
std::string ConfigManagerTest::dummyInvalidJson3 = "";
//////////////////////////
//////////////////////////
TEST_F(ConfigManagerTest, emptyCmdLine) {

  int argc = 1;
  char *argv[] = {(char*)"ConfigManager_tst"};
  ConfigManager cm;
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);

}
//////////////////////////
//////////////////////////
TEST_F(ConfigManagerTest, unknownCmdLineParam) {

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
TEST_F(ConfigManagerTest, defaultParam) {

  int argc = 1;
  char *argv[] = {(char*)"ConfigManager_tst"};
  ConfigManager cm;
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);
  double beamEnergy = myConfig.get<double>("beamParameters.energy");
  EXPECT_DOUBLE_EQ(beamEnergy, 0.0);

}
//////////////////////////
//////////////////////////
TEST_F(ConfigManagerTest, paramFromCmdLine) {

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
TEST_F(ConfigManagerTest, paramFromJSON) {

  int argc = 3;
  std::string testJSON = std::string(std::getenv("HOME"))+"/.tpcreco/config/test.json";
  char *argv[] = {(char*)"ConfigManager_tst", 
                  (char*)"--meta.configJson",const_cast<char *>(testJSON.data())};
  ConfigManager cm;
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);
  double beamEnergy = myConfig.get<double>("beamParameters.energy");
  EXPECT_DOUBLE_EQ(beamEnergy, 99.9);

}
//////////////////////////
//////////////////////////
TEST_F(ConfigManagerTest, helpCmdLine) {

  int argc = 2;
  char *argv[] = {(char*)"ConfigManager_tst", (char*)"--help"};
  ConfigManager cm;
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);

}
//////////////////////////
//////////////////////////
TEST_F(ConfigManagerTest, defaultGenericScalarParam) {

  int argc = 1;
  char *argv[] = {(char*)"ConfigManager_tst"};
  std::string optionsJSON = ConfigManagerTest::directory + ConfigManagerTest::dummyAllowedOptionsJson;
  ConfigManager cm( {}, optionsJSON );
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);

  EXPECT_EQ( myConfig.get<int>("someAnalysis.param1000") , 1000 );
  EXPECT_DOUBLE_EQ( myConfig.get<double>("someAnalysis.paramTwoPi") , M_PI*2 );
  EXPECT_EQ( myConfig.get<bool>("someAnalysis.flagTrue") , true );
}
//////////////////////////
//////////////////////////
TEST_F(ConfigManagerTest, restrictOptions) {

  int argc = 1;
  char *argv[] = {(char*)"ConfigManager_tst"};

  EXPECT_NO_THROW({
      ConfigManager cm;
      boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);
      auto energy = myConfig.get<float>("beamParameters.energy"); // allowed
      auto file = myConfig.get<std::string>("meta.configDumpJson"); // allowed
    });
  EXPECT_NO_THROW({
      ConfigManager cm( {"beamParameters.energy"} );
      boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);
      auto energy = myConfig.get<float>("beamParameters.energy"); // allowed
    });
  EXPECT_THROW({
      try
	{
	  ConfigManager cm( {"beamParameters.energy"} );
	  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);
	  auto file = myConfig.get<std::string>("meta.configDumpJson"); // not-allowed
	}
      catch ( const std::exception& e )
	{
	  EXPECT_STREQ( "No such node (meta.configDumpJson)", e.what() );
	  throw;
	}
    }, std::exception );
}
//////////////////////////
//////////////////////////
TEST_F(ConfigManagerTest, defaultGenericVectorParam) {

  int argc = 1;
  char *argv[] = {(char*)"ConfigManager_tst"};
  std::string optionsJSON = ConfigManagerTest::directory + ConfigManagerTest::dummyAllowedOptionsJson;
  ConfigManager cm( {}, optionsJSON );
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);

  EXPECT_EQ( cm.getVector<std::vector<int>>("group1.vectorI") , std::vector<int>( {-1, -1*2, -1*3} ));
  EXPECT_EQ( cm.getVector<std::vector<unsigned int>>("group1.vectorU") , std::vector<unsigned int>( {1, 1+1, 1+1+1} ));
  EXPECT_EQ( cm.getVector<std::vector<float>>("group2.vectorF") , std::vector<float>( {1.1, 1.1+1.1, 3*1.1} ));
  EXPECT_EQ( cm.getVector<std::vector<double>>("group2.vectorD") , std::vector<double>( {1.1e-11, 2.2e-12, 3.3e-13} ));
  EXPECT_EQ( cm.getVector<std::vector<bool>>("group3.vectorB") , std::vector<bool>( {true, false, true||false} ));
  EXPECT_EQ( cm.getVector<std::vector<std::string>>("group4.vectorS") , std::vector<std::string>( {"A A", "BB BB", "CCC CCC"} ));
}
//////////////////////////
//////////////////////////
TEST_F(ConfigManagerTest, paramFromGenericJSON) {

  int argc = 3;
  std::string optionsJSON = ConfigManagerTest::directory + ConfigManagerTest::dummyAllowedOptionsJson;
  std::string testJSON = ConfigManagerTest::directory + ConfigManagerTest::dummyValidJson1;
  ConfigManager cm( {}, optionsJSON );
  char *argv[] = {(char*)"ConfigManager_tst",
		  (char*)"--meta.configJson", const_cast<char *>(testJSON.data())};
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);

  EXPECT_EQ( cm.getVector<std::vector<bool>>("group3.vectorB").size(), 3 ); // default
  EXPECT_EQ( cm.getVector<std::vector<std::string>>("group4.vectorS").size() , 3 ); // default
  EXPECT_EQ( cm.getVector<std::vector<int>>("group1.vectorI").size() , 0 ); // 1st JSON
  EXPECT_EQ( cm.getVector<std::vector<double>>("group2.vectorD").size() , 2 ); // 1st JSON
  EXPECT_DOUBLE_EQ( cm.getVector<std::vector<double>>("group2.vectorD").at(1) , sin(M_PI/2) ); // 1st JSON
}
//////////////////////////
//////////////////////////
TEST_F(ConfigManagerTest, paramFromTwoGenericJSONs) {

  int argc = 4;
  std::string optionsJSON = ConfigManagerTest::directory + ConfigManagerTest::dummyAllowedOptionsJson;
  std::string testJSON1 = ConfigManagerTest::directory + ConfigManagerTest::dummyValidJson1;
  std::string testJSON2 = ConfigManagerTest::directory + ConfigManagerTest::dummyValidJson2;
  ConfigManager cm( {}, optionsJSON );
  char *argv[] = {(char*)"ConfigManager_tst",
		  (char*)"--meta.configJson", const_cast<char *>(testJSON1.data()), const_cast<char *>(testJSON2.data())};
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);

  EXPECT_EQ( cm.getVector<std::vector<bool>>("group3.vectorB").size(), 3 ); // default
  EXPECT_EQ( cm.getVector<std::vector<double>>("group2.vectorD").size() , 2 ); // 1st JSON
  EXPECT_EQ( cm.getVector<std::vector<int>>("group1.vectorI").size() , 4 ); // 2nd JSON
  EXPECT_EQ( cm.getVector<std::vector<std::string>>("group4.vectorS").size() , 4 ); // 2nd JSON
  EXPECT_EQ( cm.getVector<std::vector<std::string>>("group4.vectorS").at(3) , "5555 5555" ); // 2nd JSON
}
//////////////////////////
//////////////////////////
TEST_F(ConfigManagerTest, invalidParamFromGenericJSON) {

  int argc = 3;
  std::string optionsJSON = ConfigManagerTest::directory + ConfigManagerTest::dummyAllowedOptionsJson;
  std::string testJSON = ConfigManagerTest::directory + ConfigManagerTest::dummyInvalidJson1;
  ConfigManager cm( {}, optionsJSON );
  char *argv[] = {(char*)"ConfigManager_tst",
		  (char*)"--meta.configJson", const_cast<char *>(testJSON.data())};
  EXPECT_THROW({
      try
        {
	  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);
        }
      catch( const std::exception& e )
        {
	  EXPECT_STREQ( "wrong JSON file", e.what() );
	  throw;
        }
    }, std::exception );
}
//////////////////////////
//////////////////////////
TEST_F(ConfigManagerTest, invalidDefaultGenericAllowedParam) {

  int argc = 1;
  EXPECT_THROW({
      try
        {
	  std::string optionsJSON = ConfigManagerTest::directory + ConfigManagerTest::dummyInvalidAllowedOptionsJson1;
	  ConfigManager cm( {}, optionsJSON );
	  char *argv[] = {(char*)"ConfigManager_tst"};
	  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);
        }
      catch( const std::exception& e )
        {
	  EXPECT_STREQ( "wrong ptree node name", e.what() );
	  throw;
        }
    }, std::exception );
  EXPECT_THROW({
      try
        {
	  std::string optionsJSON = ConfigManagerTest::directory + ConfigManagerTest::dummyInvalidAllowedOptionsJson2;
	  ConfigManager cm( {}, optionsJSON );
	  char *argv[] = {(char*)"ConfigManager_tst"};
	  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);
        }
      catch( const std::exception& e )
        {
	  EXPECT_STREQ( "duplicated ptree node name", e.what() );
	  throw;
        }
    }, std::exception );
}
//////////////////////////
//////////////////////////
TEST_F(ConfigManagerTest, expressionMyValue) {

  std::stringstream ss;
  auto mval = ConfigManager::myValue<double>();

  // good case - math expression, type DOUBLE
  ss.str("1./sin(M_PI/6.)");
  ss >> mval;
  EXPECT_DOUBLE_EQ( boost::lexical_cast<double>(mval), 2.0 );

  // wrong case - math expression, type DOUBLE
  EXPECT_THROW({
      try {
	ss.clear();
	ss.str("WRONG_EXPRESSION");
	ss >> mval;
      }
      catch( const std::exception& e )
        {
	  EXPECT_STREQ( "wrong math expression", e.what() );
	  throw;
        }
    }, std::exception );
}
//////////////////////////
//////////////////////////
TEST_F(ConfigManagerTest, expressionMyVector) {

  std::stringstream ss;
  auto mvecI = ConfigManager::myVector<int>();
  auto mvecF = ConfigManager::myVector<float>();

  // good case - vector of math expressions, type INTEGER
  ss.str(R"####(
1+0
10/5
sqrt(25)
)####");
  ss >> mvecI;
  EXPECT_EQ( boost::lexical_cast<std::string>(mvecI), "[ \"1\", \"2\", \"5\" ]" );

  // good case - vector of math expressions, type FLOAT
  ss.clear();
  ss.str(R"####(
exp(0)
(int)M_PI
M_PI/atan(1.0)
)####");
  ss >> mvecF;
  EXPECT_EQ( boost::lexical_cast<std::string>(mvecF), "[ \"1\", \"3\", \"4\" ]" );
}
//////////////////////////
//////////////////////////
TEST_F(ConfigManagerTest, boostPtreeGetters) {

  int argc = 1;
  char *argv[] = {(char*)"ConfigManager_tst"};
  std::string optionsJSON = ConfigManagerTest::directory + ConfigManagerTest::dummyAllowedOptionsJson;
  ConfigManager cm( {}, optionsJSON );
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);

  EXPECT_DOUBLE_EQ( cm.getScalar<double>("someAnalysis.paramTwoPi"), ConfigManager::getScalar<double>(myConfig, "someAnalysis.paramTwoPi") );
  EXPECT_EQ( cm.getVector<std::vector<int>>("group1.vectorI"), ConfigManager::getVector<std::vector<int>>(myConfig, "group1.vectorI") );
}
//////////////////////////
//////////////////////////
TEST_F(ConfigManagerTest, paramPtreeFromGenericJSON) {

  int argc = 3;
  std::string optionsJSON = ConfigManagerTest::directory + ConfigManagerTest::dummyAllowedOptionsJson;
  std::string testJSON = ConfigManagerTest::directory + ConfigManagerTest::dummyValidJson3;
  ConfigManager cm( {}, optionsJSON );
  char *argv[] = {(char*)"ConfigManager_tst",
		  (char*)"--meta.configJson", const_cast<char *>(testJSON.data())};
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);

  EXPECT_EQ( ConfigManager::getVector<std::vector<int>>(myConfig, "someAnalysis.paramPtree.paramPtree.vector56"), std::vector<int>({5, 6}) );
  EXPECT_EQ( myConfig.get<std::string>("someAnalysis.paramPtree.paramPtree.paramAA"), "AA" );
  EXPECT_EQ( myConfig.get_child("someAnalysis.paramPtree.vectorABC").size(), 3 );
}
//////////////////////////
//////////////////////////
TEST_F(ConfigManagerTest, paramPtreeFromCmdLine) {

  int argc = 3;
  std::string optionsJSON = ConfigManagerTest::directory + ConfigManagerTest::dummyAllowedOptionsJson;
  ConfigManager cm( {}, optionsJSON );
  char *argv[] = {(char*)"ConfigManager_tst",
		  (char*)"--someAnalysis.paramPtree",
		  (char*)"{ \"vector56\": [ 5, 6 ], \"paramAA\": \"AA\" }"};
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);

  EXPECT_EQ( ConfigManager::getVector<std::vector<int>>(myConfig, "someAnalysis.paramPtree.vector56"), std::vector<int>({5, 6}) );
  EXPECT_EQ( myConfig.get<std::string>("someAnalysis.paramPtree.paramAA"), "AA" );
}
//////////////////////////
//////////////////////////
TEST_F(ConfigManagerTest, paramVectorPtreeFromCmdLine) {

  int argc = 6;
  std::string optionsJSON = ConfigManagerTest::directory + ConfigManagerTest::dummyAllowedOptionsJson;
  ConfigManager cm( {}, optionsJSON );
  char *argv[] = {(char*)"ConfigManager_tst",
		  (char*)"--someAnalysis.vectorPtree",
		  (char*)"[",
		  (char*)"{\"vector56\": [ 5, 6 ] }",
		  (char*)"{\"paramAA\": \"AA\"}",
		  (char*)"]"};
  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);
  auto pt = myConfig.get_child("someAnalysis.vectorPtree");
  size_t index=0;
  BOOST_FOREACH(auto &item, pt) {
    if(index==0) EXPECT_EQ( item.second.count("vector56"), 1 );
    if(index==1) EXPECT_EQ( item.second.count("paramAA"), 1 );
    index++;
  }
  EXPECT_NO_THROW({
      BOOST_FOREACH(auto item, pt) { // 1st level
	std::cout << "1st-level child: key: " << item.first << ", size: " << item.second.size() << ", value: " << item.second.get_value<std::string>() << std::endl;
	if(item.second.size()) {
	  BOOST_FOREACH(auto item2, item.second) {
	    std::cout << "2nd-level child: key: " << item2.first << ", size: " << item2.second.size() << ", value: " << item2.second.get_value<std::string>() << std::endl;
	    if(item2.second.size()) {
	      BOOST_FOREACH(auto item3, item2.second) {
		std::cout << "3rd-level child: key: " << item3.first << ", size: " << item3.second.size() << ", value: " << item3.second.get_value<std::string>() << std::endl;
	      }
	    }
	  }
	}
      }
    });
}
//////////////////////////
//////////////////////////
TEST_F(ConfigManagerTest, invalidPtreeFromGenericJSON) {

  int argc = 3;
  std::string optionsJSON = ConfigManagerTest::directory + ConfigManagerTest::dummyAllowedOptionsJson;
  std::string testJSON = ConfigManagerTest::directory + ConfigManagerTest::dummyInvalidJson2;
  ConfigManager cm( {}, optionsJSON );
  char *argv[] = {(char*)"ConfigManager_tst",
		  (char*)"--meta.configJson", const_cast<char *>(testJSON.data())};
  EXPECT_THROW({
      try
        {
	  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);
        }
      catch( const std::exception& e )
        {
	  EXPECT_STREQ( "wrong JSON file", e.what() );
	  throw;
        }
    }, std::exception );
}
//////////////////////////
//////////////////////////
TEST_F(ConfigManagerTest, invalidVectorPtreeFromGenericJSON) {

  int argc = 3;
  std::string optionsJSON = ConfigManagerTest::directory + ConfigManagerTest::dummyAllowedOptionsJson;
  std::string testJSON = ConfigManagerTest::directory + ConfigManagerTest::dummyInvalidJson3;
  ConfigManager cm( {}, optionsJSON );
  char *argv[] = {(char*)"ConfigManager_tst",
		  (char*)"--meta.configJson", const_cast<char *>(testJSON.data())};
  EXPECT_THROW({
      try
        {
	  boost::property_tree::ptree myConfig = cm.getConfig(argc, argv);
        }
      catch( const std::exception& e )
        {
	  EXPECT_STREQ( "wrong JSON file", e.what() );
	  throw;
        }
    }, std::exception );
}
//////////////////////////
//////////////////////////
