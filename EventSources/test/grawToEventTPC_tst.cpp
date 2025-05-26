#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <iomanip>
#include <unistd.h>
#include <boost/filesystem.hpp>
#include "gtest/gtest.h"
#include "TFile.h"
#include "TTree.h"

#include "TPCReco/EventSourceFactory.h"
#include "TPCReco/ConfigManager.h"
#include "TPCReco/colorText.h"
#include "TPCReco/InputFileHelper.h"
#include "TPCReco/ConvertGrawFile.h"

namespace fs = boost::filesystem;
class grawToEventTPCTest : public ::testing::Test {
public:
  static boost::property_tree::ptree myConfig;
  static std::shared_ptr<PEventTPC> myEventPtr;
  static PEventTPC* rootEventPtr;

  static std::string directory;
  static std::string rootFileName;
  static std::string rootTreeName;
  static TFile* rootfile;
  static TTree* tree;

  static void SetUpTestSuite() {
    directory = (fs::temp_directory_path() / fs::unique_path()).string() +
      fs::path::preferred_separator;
    fs::create_directories(directory);

    // load the graw file
    std::string testJSON = std::string(std::getenv("HOME"))+"/.tpcreco/config/test.json";
    std::string tempDirWithFilePrefix = directory+"TestPEventTPC";
    int argc = 7;
    char *argv[] = {(char*)"ConfigManager_tst",
		    (char*)"--meta.configJson",const_cast<char *>(testJSON.data()),
                    (char*)"--grawToRoot.filePrefix", const_cast<char *>(tempDirWithFilePrefix.data()),
		    (char*)"--grawToRoot.treeName", (char*)"TestTPCData"};
    ConfigManager cm;
    myConfig = cm.getConfig(argc, argv);
    int status = chdir("../../resources");
    std::shared_ptr<EventSourceBase> myEventSource = EventSourceFactory::makeEventSourceObject(myConfig);
  
    myEventPtr = myEventSource->getCurrentPEvent();
    myEventSource->loadFileEntry(0);

    // convert the graw file to root file
    convertGRAWFile(myConfig);

    // load the root file
    std::string grawFileName = myConfig.get<std::string>("input.dataFile","");
    rootFileName = InputFileHelper::makeOutputFileName(grawFileName, myConfig.get<std::string>("grawToRoot.filePrefix"));
    rootTreeName = myConfig.get<std::string>("grawToRoot.treeName");
    rootfile = new TFile(rootFileName.c_str());
    tree = (TTree*)rootfile->Get(rootTreeName.c_str());
    tree->SetBranchAddress("Event", &rootEventPtr);
    tree->GetEntry(0);
  }

  static void TearDownTestSuite() { fs::remove_all(directory); }
};

std::string grawToEventTPCTest::directory = "";
boost::property_tree::ptree grawToEventTPCTest::myConfig;
std::shared_ptr<PEventTPC> grawToEventTPCTest::myEventPtr;
PEventTPC* grawToEventTPCTest::rootEventPtr;
std::string grawToEventTPCTest::rootFileName;
std::string grawToEventTPCTest::rootTreeName;
TFile* grawToEventTPCTest::rootfile;
TTree* grawToEventTPCTest::tree;


TEST(ROOTFileNameTest, createROOTFileName)
{
  std::string grawFileName = "../testData/CoBo0_AsAd0_2022-04-12T08-03-44.531_0000.graw,"
                             "../testData/CoBo0_AsAd1_2022-04-12T08-03-44.533_0000.graw,"
                             "../testData/CoBo0_AsAd2_2022-04-12T08-03-44.536_0000.graw,"
                             "../testData/CoBo0_AsAd3_2022-04-12T08-03-44.540_0000.graw";
  std::string testFileName = InputFileHelper::makeOutputFileName(grawFileName, "PEventTPC");
  EXPECT_EQ(testFileName, "PEventTPC_2022-04-12T08-03-44.540_0000.root");
}


TEST_F(grawToEventTPCTest, convertGRAWFile)
{
  ASSERT_EQ(rootfile->IsZombie(), false);
  ASSERT_EQ(rootfile->IsOpen(), true);
  EXPECT_STREQ(tree->GetName(), "TestTPCData");
  if(myConfig.get<int>("input.readNEvents") > 0)
    EXPECT_EQ(tree->GetEntries(), myConfig.get<int>("input.readNEvents",1));

  auto grawChargeMap = myEventPtr->GetChargeMap();
  auto rootChargeMap = rootEventPtr->GetChargeMap();

  EXPECT_EQ(grawChargeMap, rootChargeMap);

  rootfile->Close();
  EXPECT_EQ(rootfile->IsOpen(), false);
}

