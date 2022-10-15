#include "TTreeOps.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <vector>

using namespace tpcreco::utilities;

class TreeDuplicationTest : public ::testing::Test {
public:
  static TTree *tree;
  static int runId;
  static int eventId;
  static double data;
  static const std::vector<std::pair<int, double>> ids;
  static void SetUpTestSuite() {
    tree->Branch("runId", &runId);
    tree->Branch("eventId", &eventId);
    tree->Branch("data", &data);
    for (auto i : ids) {
      eventId = i.first;
      data = i.second;
      tree->Fill();
    }
    tree->BuildIndex("runId", "eventId");
  }
};
TTree *TreeDuplicationTest::tree = new TTree("", "");
double TreeDuplicationTest::data = 0;
int TreeDuplicationTest::runId = 1;
int TreeDuplicationTest::eventId = 0;
const std::vector<std::pair<int, double>> TreeDuplicationTest::ids = {
    {0, .0}, {1, .1}, {2, .2}, {2, .3}, {2, .4},
    {3, .5}, {3, .6}, {2, .7}, {4, .8}};

TEST_F(TreeDuplicationTest, treeCreation) {
  for (size_t i = 0; i < ids.size(); ++i) {
    tree->GetEntry(i);
    EXPECT_EQ(eventId, ids[i].first);
    EXPECT_EQ(data, ids[i].second);
  }
}

TEST_F(TreeDuplicationTest, nonDuplicates) {
  auto noDup = filterDuplicates(tree);
  EXPECT_THAT(noDup, ::testing::ElementsAreArray({8, 6, 7, 1, 0}));
  std::vector<double> expectedData = {.8, .6, .7, .1, .0};
  int i = 0;
  for (auto entry : noDup) {
    tree->GetEntry(entry);
    EXPECT_FLOAT_EQ(data, expectedData[i]);
    ++i;
  }
}
