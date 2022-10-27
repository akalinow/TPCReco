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
  RequirementsCollection<std::function<bool(int)>> cuts = {
      GreaterThan{3}, [](auto i) { return i < 7; }};
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
  RequirementsCollection<std::function<bool(int)>> cuts = {
      GreaterThan{3}, [](auto i) { return i < 7; }};
  EXPECT_FALSE(cuts(2));
  EXPECT_TRUE(cuts(4));
  EXPECT_FALSE(cuts(9));
}

TEST(CountedRequirement, count) {
  auto req = CountedRequirement<std::function<bool(int)>>(GreaterThan{3});
  EXPECT_EQ(req.getCount(), 0);

  EXPECT_FALSE(req(2));
  EXPECT_EQ(req.getCount(), 0);

  EXPECT_TRUE(req(5));
  EXPECT_EQ(req.getCount(), 1);

  EXPECT_TRUE(req(6));
  EXPECT_EQ(req.getCount(), 2);

  EXPECT_FALSE(req(0));
  EXPECT_EQ(req.getCount(), 2);
}

TEST(CountedRequirement, reset) {
  auto req = CountedRequirement<std::function<bool(int)>>(GreaterThan{3});
  EXPECT_EQ(req.getCount(), 0);

  EXPECT_TRUE(req(5));
  EXPECT_EQ(req.getCount(), 1);
  req.resetCount();
  EXPECT_EQ(req.getCount(), 0);
  req.resetCount();
  EXPECT_EQ(req.getCount(), 0);
}

TEST(Make_counted, functor) {
  auto req = make_counted<GreaterThan>(3);
  EXPECT_EQ(req.getCount(), 0);
  EXPECT_FALSE(req(2));
  EXPECT_EQ(req.getCount(), 0);
  EXPECT_TRUE(req(5));
  EXPECT_EQ(req.getCount(), 1);
}

TEST(Make_counted, functorAsStdFunction) {
  auto req = make_counted<std::function<bool(int)>>(GreaterThan(3));
  EXPECT_EQ(req.getCount(), 0);
  EXPECT_FALSE(req(2));
  EXPECT_EQ(req.getCount(), 0);
  EXPECT_TRUE(req(5));
  EXPECT_EQ(req.getCount(), 1);
}

TEST(Make_counted, lambda) {
  auto req =
      make_counted<std::function<bool(int)>>([](int i) { return i > 3; });
  EXPECT_EQ(req.getCount(), 0);
  EXPECT_FALSE(req(2));
  EXPECT_EQ(req.getCount(), 0);
  EXPECT_TRUE(req(5));
  EXPECT_EQ(req.getCount(), 1);
}
