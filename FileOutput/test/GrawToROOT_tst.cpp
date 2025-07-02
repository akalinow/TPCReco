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
#include "TPCReco/FileOutput.h"


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void createOutputFile(boost::property_tree::ptree &config) {

  std::shared_ptr<EventSourceBase> myEventSource = EventSourceFactory::makeEventSourceObject(config);

  FileOutput myFileOutput(config);

  int nEvents = config.get<int>("input.readNEvents");

  for(int iEvent=0;iEvent<nEvents;++iEvent) {
    myEventSource->loadFileEntry(iEvent);
    myFileOutput.update(myEventSource); 
  }
}
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
namespace fs = boost::filesystem;
class GrawToROOTTest : public ::testing::Test {
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
    std::string tempDirWithFilePrefix = directory+"PEventTPC";
 
    int argc = 7;
    char *argv[] = {(char*)"ConfigManager_tst",
		    (char*)"--meta.configJson",const_cast<char *>(testJSON.data()),
        (char*)"--output.fileName",const_cast<char *>(tempDirWithFilePrefix.data()),
        (char*)"--input.readNEvents",const_cast<char *>("3")
      };

    ConfigManager cm;
    myConfig = cm.getConfig(argc, argv);
    int status = chdir("../../resources");

    // convert the graw file to root file
    createOutputFile(myConfig);

    // Read first event for comparison
    std::shared_ptr<EventSourceBase> myEventSource = EventSourceFactory::makeEventSourceObject(myConfig);
    myEventPtr = myEventSource->getCurrentPEvent();
    myEventSource->loadFileEntry(0);

    // load the root file
    std::string grawFileName = myConfig.get<std::string>("input.dataFile","");
    rootFileName = directory+InputFileHelper::makeOutputFileName(grawFileName, "PEventTPC");
    rootTreeName = "TPCData";
    rootfile = new TFile(rootFileName.c_str());
    tree = (TTree*)rootfile->Get(rootTreeName.c_str());
    tree->SetBranchAddress("Event", &rootEventPtr);
    tree->GetEntry(0);
  }

  static void TearDownTestSuite() { fs::remove_all(directory); }
};

std::string GrawToROOTTest::directory = "";
boost::property_tree::ptree GrawToROOTTest::myConfig;
std::shared_ptr<PEventTPC> GrawToROOTTest::myEventPtr;
PEventTPC* GrawToROOTTest::rootEventPtr;
std::string GrawToROOTTest::rootFileName;
std::string GrawToROOTTest::rootTreeName;
TFile* GrawToROOTTest::rootfile;
TTree* GrawToROOTTest::tree;


TEST_F(GrawToROOTTest, createROOTFileName)
{
  std::string grawFileName = "../testData/CoBo0_AsAd0_2022-04-12T08-03-44.531_0000.graw,"
                             "../testData/CoBo0_AsAd1_2022-04-12T08-03-44.533_0000.graw,"
                             "../testData/CoBo0_AsAd2_2022-04-12T08-03-44.536_0000.graw,"
                             "../testData/CoBo0_AsAd3_2022-04-12T08-03-44.540_0000.graw";
  std::string testFileName = InputFileHelper::makeOutputFileName(grawFileName, "PEventTPC");
  EXPECT_EQ(testFileName, "PEventTPC_2022-04-12T08-03-44.540_0000.root");
}


TEST_F(GrawToROOTTest, convertGRAWFile)
{
  ASSERT_EQ(rootfile->IsZombie(), false);
  ASSERT_EQ(rootfile->IsOpen(), true);
  EXPECT_STREQ(tree->GetName(), rootTreeName.c_str());

  if(myConfig.get<int>("input.readNEvents") > 0)
    EXPECT_EQ(tree->GetEntries(), myConfig.get<int>("input.readNEvents"));

  auto grawChargeMap = myEventPtr->GetChargeMap();
  auto rootChargeMap = rootEventPtr->GetChargeMap();

  EXPECT_EQ(grawChargeMap, rootChargeMap);

  rootfile->Close();
  EXPECT_EQ(rootfile->IsOpen(), false);
}

