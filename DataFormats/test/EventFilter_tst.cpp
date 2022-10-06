#include "TPCReco/EventFilter.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <boost/property_tree/json_parser.hpp>
#include <functional>
#include <sstream>
namespace pt = boost::property_tree;
using ::testing::Return;

struct EventMock {
  MOCK_METHOD(double, GetTotalCharge, (), (const));
  MOCK_METHOD(double, GetMaxCharge, (), (const));
  MOCK_METHOD(size_t, GetEventId, (), (const));
};

class EventFilterTest : public ::testing::Test {
public:
  EventFilter<std::function<bool(const EventMock &)>> filter;
  EventMock event;
  pt::ptree ptree;
};

TEST_F(EventFilterTest, DefaultBehaviour) { EXPECT_TRUE(filter.pass(event)); }

TEST_F(EventFilterTest, Enabled_disabled) {
  std::stringstream config{R"(
{
  "eventFilter": {
    "enabled": false,
    "totalChargeLowerBound": 1000
  }
}
  )"};

  pt::read_json(config, ptree);
  filter.setConditions(ptree);
  EXPECT_CALL(event, GetTotalCharge()).WillRepeatedly(Return(0));
  EXPECT_TRUE(filter.pass(event));
  filter.setEnabled(true);
  EXPECT_FALSE(filter.pass(event));
  filter.setEnabled(false);
  EXPECT_TRUE(filter.pass(event));
  filter.setDisabled(false);
  EXPECT_FALSE(filter.pass(event));
  filter.setDisabled(true);
  EXPECT_TRUE(filter.pass(event));
}

TEST_F(EventFilterTest, totalCharge) {
  std::stringstream config{R"(
{
  "eventFilter": {
    "totalChargeLowerBound": 1000,
    "totalChargeUpperBound": 2000
  }
}
  )"};

  pt::read_json(config, ptree);
  filter.setConditions(ptree);
  filter.setEnabled(true);
  EXPECT_CALL(event, GetTotalCharge()).WillRepeatedly(Return(100));
  EXPECT_FALSE(filter.pass(event));
  EXPECT_CALL(event, GetTotalCharge()).WillRepeatedly(Return(1500));
  EXPECT_TRUE(filter.pass(event));
  EXPECT_CALL(event, GetTotalCharge()).WillRepeatedly(Return(5000));
  EXPECT_FALSE(filter.pass(event));
}

TEST_F(EventFilterTest, maxCharge) {
  std::stringstream config{R"(
{
   "eventFilter": {
     "maxChargeLowerBound": 1000,
     "maxChargeUpperBound": 2000
   }
 }
   )"};

  pt::read_json(config, ptree);
  filter.setConditions(ptree);
  filter.setEnabled(true);
  EXPECT_CALL(event, GetMaxCharge()).WillRepeatedly(Return(100));
  EXPECT_FALSE(filter.pass(event));
  EXPECT_CALL(event, GetMaxCharge()).WillRepeatedly(Return(1500));
  EXPECT_TRUE(filter.pass(event));
  EXPECT_CALL(event, GetMaxCharge()).WillRepeatedly(Return(5000));
  EXPECT_FALSE(filter.pass(event));
}

TEST_F(EventFilterTest, eventId) {
  std::stringstream config{R"(
{
  "eventFilter": {
    "events": [
      1,
      2
    ]
  }
}
   )"};

  pt::read_json(config, ptree);
  filter.setConditions(ptree);
  filter.setEnabled(true);
  EXPECT_CALL(event, GetEventId()).Times(1).WillRepeatedly(Return(0));
  EXPECT_FALSE(filter.pass(event));
  EXPECT_CALL(event, GetEventId()).Times(1).WillRepeatedly(Return(1));
  EXPECT_TRUE(filter.pass(event));
  EXPECT_CALL(event, GetEventId()).Times(1).WillRepeatedly(Return(2));
  EXPECT_TRUE(filter.pass(event));
  EXPECT_CALL(event, GetEventId()).Times(1).WillRepeatedly(Return(3));
  EXPECT_FALSE(filter.pass(event));
}