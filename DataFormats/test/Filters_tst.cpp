#include "TPCReco/Filters.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

struct EventInfoMock{
  MOCK_METHOD(uint32_t, GetEventId, (), (const));
};
struct EventMock {
  MOCK_METHOD(double, GetTotalCharge, (), (const));
  MOCK_METHOD(double, GetMaxCharge, (), (const));
  const EventInfoMock& GetEventInfo(){return eventInfo;}
  EventInfoMock eventInfo;
};

using namespace tpcreco::filters;
using ::testing::Return;

TEST(Filters, TotalChargeLowerBound) {
  TotalChargeLowerBound filter{0.5};
  EventMock event;
  EXPECT_CALL(event, GetTotalCharge()).Times(1).WillRepeatedly(Return(0.0));
  EXPECT_FALSE(filter(event));
  EXPECT_CALL(event, GetTotalCharge()).Times(1).WillRepeatedly(Return(1.0));
  EXPECT_TRUE(filter(event));
}

TEST(Filters, TotalChargeUpperBound) {
  TotalChargeUpperBound filter{0.5};
  EventMock event;
  EXPECT_CALL(event, GetTotalCharge()).Times(1).WillRepeatedly(Return(0.0));
  EXPECT_TRUE(filter(event));
  EXPECT_CALL(event, GetTotalCharge()).Times(1).WillRepeatedly(Return(1.0));
  EXPECT_FALSE(filter(event));
}

TEST(Filters, MaxChargeLowerBound) {
  MaxChargeLowerBound filter{0.5};
  EventMock event;
  EXPECT_CALL(event, GetMaxCharge()).Times(1).WillRepeatedly(Return(0.0));
  EXPECT_FALSE(filter(event));
  EXPECT_CALL(event, GetMaxCharge()).Times(1).WillRepeatedly(Return(1.0));
  EXPECT_TRUE(filter(event));
}

TEST(Filters, MaxChargeUpperBound) {
  MaxChargeUpperBound filter{0.5};
  EventMock event;
  EXPECT_CALL(event, GetMaxCharge()).Times(1).WillRepeatedly(Return(0.0));
  EXPECT_TRUE(filter(event));
  EXPECT_CALL(event, GetMaxCharge()).Times(1).WillRepeatedly(Return(1.0));
  EXPECT_FALSE(filter(event));
}

TEST(Filters, IndexInSetInitalizer_list) {
  IndexInSet filter = {2, 4};
  EventMock event;
  EXPECT_CALL(event.eventInfo, GetEventId()).Times(1).WillOnce(Return(1));
  EXPECT_FALSE(filter(event));
  EXPECT_CALL(event.eventInfo, GetEventId()).Times(1).WillOnce(Return(2));
  EXPECT_TRUE(filter(event));
}

TEST(Filters, IndexInSetInsertion) {
  IndexInSet filter;
  EventMock event;
  EXPECT_CALL(event.eventInfo, GetEventId()).Times(::testing::AtMost(1)).WillRepeatedly(Return(1));
  EXPECT_TRUE(filter(event)); // set is empty => event ID filter has no effect
  filter.insert(2);
  EXPECT_CALL(event.eventInfo, GetEventId()).Times(1).WillRepeatedly(Return(2));
  EXPECT_TRUE(filter(event)); // set is non-empty => event ID filter is active
  EXPECT_CALL(event.eventInfo, GetEventId()).Times(1).WillRepeatedly(Return(1));
  EXPECT_FALSE(filter(event)); // set is non-empty => event ID filter is active
}
