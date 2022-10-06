#include "TPCReco/RequirementsCollection.h"
#include "gtest/gtest.h"
#include <functional>
class GreaterThan {
public:
  GreaterThan(int value) : value(value) {}
  bool operator()(int a) { return a > value; }

private:
  int value;
};

TEST(RequirementsCollection, InitializerList) {
  RequirementsCollection<std::function<bool(int)>> cuts = {GreaterThan{3},
                                      [](auto i) { return i < 7; }};
  EXPECT_EQ(cuts.size(), 2);
  cuts.clear();
  EXPECT_EQ(cuts.size(), 0);
}

TEST(RequirementsCollection, ElementInsertion) {
  RequirementsCollection<std::function<bool(int)>> cuts;
  EXPECT_EQ(cuts.size(), 0);
  GreaterThan predicate(3);
  cuts.push_back(predicate);
  EXPECT_EQ(cuts.size(), 1);
  cuts.clear();
  EXPECT_EQ(cuts.size(), 0);
}

TEST(RequirementsCollection, Logic) {
  RequirementsCollection<std::function<bool(int)>> cuts;
  GreaterThan predicate(3);
  cuts.push_back(predicate);
  cuts.push_back([](auto i) { return i < 7; });
  EXPECT_FALSE(cuts(2));
  EXPECT_TRUE(cuts(4));
  EXPECT_FALSE(cuts(9));
}

TEST(RequirementsCollection, LogicWithInitializerList) {
  RequirementsCollection<std::function<bool(int)>> cuts = {GreaterThan{3},
                                      [](auto i) { return i < 7; }};
  EXPECT_FALSE(cuts(2));
  EXPECT_TRUE(cuts(4));
  EXPECT_FALSE(cuts(9));
}
