#include "TTreeOps.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <boost/variant.hpp>
#include <vector>

using namespace tpcreco::utilities;
using ::testing::InSequence;
using ::testing::Return;
using ::testing::StrictMock;

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
  static void TearDownTestSuite() { delete tree; }
};

TTree *TreeDuplicationTest::tree = new TTree("test tree", "test tree");
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

TEST_F(TreeDuplicationTest, CloneUnique) {
  auto clone = cloneUnique(tree, nullptr);
  std::vector<double> expectedData = {.8, .6, .7, .1, .0};
  EXPECT_EQ(clone->GetEntries(), expectedData.size());
  for (int i = 0; i < clone->GetEntries(); ++i) {
    clone->GetEntry(i);
    EXPECT_FLOAT_EQ(data, expectedData[i]);
  }
}

struct DetailCheckMock {
  void operator()(int a, int b) { return invokeHelper(a, b); }
  MOCK_METHOD(void, invokeHelper, (int, int));
};

struct ExtraProcessorMock : boost::static_visitor<> {
  void operator()(Dispatched<0, int> v) { return invokeHelper(v); }
  MOCK_METHOD(void, invokeHelper, ((Dispatched<0, int>)));
  void operator()(Dispatched<1, int> v) { return invokeHelper(v); }
  MOCK_METHOD(void, invokeHelper, ((Dispatched<1, int>)));
};

TEST(TreeDiffTest, diffDispatch) {
  InSequence sequnce;
  std::vector<int> v1 = {1, 2, 3, 6, 7, 8, 9, 11};
  std::vector<int> v2 = {1, 2, 4, 5, 9, 12};
  auto range1 = boost::make_iterator_range(v1.data(), v1.data() + v1.size());
  auto range2 = boost::make_iterator_range(v2.data(), v2.data() + v2.size());

  StrictMock<DetailCheckMock> detailCheck;
  StrictMock<ExtraProcessorMock> visitor;
  EXPECT_CALL(detailCheck, invokeHelper(1, 1));
  EXPECT_CALL(detailCheck, invokeHelper(2, 2));
  EXPECT_CALL(visitor, invokeHelper(Dispatched<0, int>{3}));
  EXPECT_CALL(visitor, invokeHelper(Dispatched<1, int>{4}));
  EXPECT_CALL(visitor, invokeHelper(Dispatched<1, int>{5}));
  EXPECT_CALL(visitor, invokeHelper(Dispatched<0, int>{6}));
  EXPECT_CALL(visitor, invokeHelper(Dispatched<0, int>{7}));
  EXPECT_CALL(visitor, invokeHelper(Dispatched<0, int>{8}));
  EXPECT_CALL(detailCheck, invokeHelper(9, 9));
  EXPECT_CALL(visitor, invokeHelper(Dispatched<0, int>{11}));
  EXPECT_CALL(visitor, invokeHelper(Dispatched<1, int>{12}));
  diffDispatcher(
      range1, range2, std::less<int>(), std::equal_to<int>(), detailCheck,
      [&visitor](boost::variant<Dispatched<0, int>, Dispatched<1, int>>
                     variant) mutable { variant.apply_visitor(visitor); });
}
