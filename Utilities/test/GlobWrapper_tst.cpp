#include "TPCReco/GlobWrapper.h"
#include "gtest/gtest.h"

#include <boost/filesystem.hpp>
#include <fstream>
namespace fs = boost::filesystem;

class GlobWrapperTest : public ::testing::Test {
public:
  static std::string directory;

  static std::string createFile(const std::string &name) {
    auto file = directory + name;
    std::ofstream{file};
    return file;
  }

  static void TearDownTestSuite() { fs::remove_all(directory); }

  static void SetUpTestSuite() {
    directory = (fs::temp_directory_path() / fs::unique_path()).string() +
                fs::path::preferred_separator;
    fs::create_directories(directory);

    createFile("CoBo_ALL_AsAd_ALL_2021-07-12T11:02:15.328_0000.graw");
    createFile("CoBo_ALL_AsAd_ALL_2021-07-12T11:02:15.328_0001.graw");
    createFile("CoBo_ALL_AsAd_ALL_2021-07-12T12:03:40.125_0000.graw");
  }
};

std::string GlobWrapperTest::directory = "";

TEST_F(GlobWrapperTest, GlobWrapper) {
  auto paths = globWrapper(directory + "CoBo_ALL_AsAd_ALL_2021-07-12T11:02:15.328_*.graw");
  EXPECT_EQ(paths.at(0),
            directory + "CoBo_ALL_AsAd_ALL_2021-07-12T11:02:15.328_0000.graw");
  EXPECT_EQ(paths.at(1),
            directory + "CoBo_ALL_AsAd_ALL_2021-07-12T11:02:15.328_0001.graw");
  EXPECT_THROW(paths.at(3), std::out_of_range);
}
