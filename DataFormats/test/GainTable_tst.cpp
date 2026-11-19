// Tests of the charge gain table (run range and pressure -> G_cal, G_raw).
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>

#include <gtest/gtest.h>

#include "TPCReco/GainTable.h"

namespace {
std::string writeFile(const std::string &name, const std::string &content) {
  const std::string path = std::string(::testing::TempDir()) + name;
  std::ofstream(path) << content;
  return path;
}
}  // namespace

TEST(GainTableTest, FindByRunAndPressure) {
  const std::string path = writeFile("gaintable_ok.dat",
                                     "# comment\n"
                                     "20220401000000 20220430235959 190.0 206000 224000  # Apr 190\n"
                                     "\n"
                                     "20220401000000 20220430235959 250 60000 65000\n"
                                     "20220801000000 20220930235959 130 452000 475000\n");
  GainTable aTable(path);
  ASSERT_EQ(aTable.getEntries().size(), 3u);
  GainTable::Entry entry;
  ASSERT_TRUE(aTable.find(20220412064752, 190.0, entry));
  EXPECT_DOUBLE_EQ(entry.gainCal, 206000);
  EXPECT_DOUBLE_EQ(entry.gainRaw, 224000);
  ASSERT_TRUE(aTable.find(20220413131752, 251.3, entry));  // within the pressure tolerance
  EXPECT_DOUBLE_EQ(entry.gainCal, 60000);
  ASSERT_TRUE(aTable.find(20220907081620, 130.0, entry));
  EXPECT_DOUBLE_EQ(entry.gainRaw, 475000);
  EXPECT_FALSE(aTable.find(20220412064752, 130.0, entry));  // April at 130 mbar: not in this table
  EXPECT_FALSE(aTable.find(100, 190.0, entry));             // MC run ID
  std::remove(path.c_str());
}

TEST(GainTableTest, RejectsBadInput) {
  EXPECT_THROW(GainTable("/nonexistent/gain_table.dat"), std::runtime_error);
  for (const std::string &content : {std::string("# only comments\n"), std::string("1 2 190 abc 1\n"),
                                     std::string("2 1 190 1 1\n"), std::string("1 2 190 0 1\n"),
                                     std::string("1 2 190 1\n")}) {
    const std::string path = writeFile("gaintable_bad.dat", content);
    EXPECT_THROW(GainTable aTable(path), std::runtime_error) << content;
    std::remove(path.c_str());
  }
}

TEST(GainTableTest, DistributedTableCoversHIgS2022Setups) {
  GainTable aTable(RESOURCES_DIR "gainTable_HIgS2022.dat");
  GainTable::Entry entry;
  for (const auto &run : {std::make_pair(20220412064752L, 190.0), std::make_pair(20220413131752L, 250.0),
                          std::make_pair(20220414085139L, 130.0), std::make_pair(20220822220311L, 130.0)}) {
    ASSERT_TRUE(aTable.find(run.first, run.second, entry)) << run.first;
    EXPECT_GT(entry.gainCal, 1E4);
    EXPECT_GT(entry.gainRaw, entry.gainCal);  // reconstructed lengths are short: E_dep < E_cal
  }
}
