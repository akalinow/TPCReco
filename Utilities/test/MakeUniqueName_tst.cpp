#include "MakeUniqueName.h"
#include "gtest/gtest.h"

#include <boost/filesystem.hpp>
#include <fstream>
namespace fs = boost::filesystem;

class MakeUniqueNameTest : public ::testing::Test {
public:
  static std::string directory;

  static std::string createFile(const std::string &name) {
    auto file = directory + name;
    std::ofstream{file};
    return file;
  }

  static void TearDownTestSuite() { fs::remove_all(directory); }

  static void SetUpTestSuite() {
    directory = (fs::temp_directory_path() / fs::unique_path()).string()+fs::path::preferred_separator;
    fs::create_directories(directory);

    createFile("Reco_CoBo_ALL_AsAd_ALL_2021-07-12T11:02:15.328_0000.root");
    createFile("Reco_CoBo_ALL_AsAd_ALL_2021-07-12T11:02:15.328_0001.root");
    createFile("Reco_CoBo_ALL_AsAd_ALL_2021-07-12T11:02:15.328_0001-1.root");
    createFile("Reco_CoBo_ALL_AsAd_ALL_2021-07-12T11:02:15.328_0001-2.root");
  }
};

std::string MakeUniqueNameTest::directory = "";

TEST_F(MakeUniqueNameTest, NotExists) {
  EXPECT_EQ(MakeUniqueName(
                directory +
                "Reco_CoBo_ALL_AsAd_ALL_2021-07-12T11:02:15.328_0003.root"),
            directory +
                "Reco_CoBo_ALL_AsAd_ALL_2021-07-12T11:02:15.328_0003.root");
}

TEST_F(MakeUniqueNameTest, Exists) {
  EXPECT_EQ(MakeUniqueName(
                directory +
                "Reco_CoBo_ALL_AsAd_ALL_2021-07-12T11:02:15.328_0000.root"),
            directory +
                "Reco_CoBo_ALL_AsAd_ALL_2021-07-12T11:02:15.328_0000-1.root");
  EXPECT_EQ(MakeUniqueName(
                directory +
                "Reco_CoBo_ALL_AsAd_ALL_2021-07-12T11:02:15.328_0001.root"),
            directory +
                "Reco_CoBo_ALL_AsAd_ALL_2021-07-12T11:02:15.328_0001-3.root");
}