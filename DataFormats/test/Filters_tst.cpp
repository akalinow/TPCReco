#include "TPCReco/Filters.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace tpcreco::filters;
using ::testing::Return;

struct EventInfoMock
{
  EventInfoMock & operator =(const EventInfoMock &){};
  MOCK_METHOD(uint32_t, GetEventId, (), (const));
};

struct Track3DMock
{
  Track3DMock() = default;
  Track3DMock(const Track3DMock &) {}
  MOCK_METHOD(std::vector<int>, getSegments, (), (const));
};

struct EventTPCMock
{
  EventTPCMock() = default;
  EventTPCMock(const EventTPCMock &) {}
  MOCK_METHOD(double, GetTotalCharge, (), (const));
  MOCK_METHOD(double, GetMaxCharge, (), (const));
  const EventInfoMock &GetEventInfo() { return eventInfo; };
  EventInfoMock eventInfo;
};

struct EventSourceMock
{
  std::shared_ptr<Track3DMock> getRecoEvent() const { return recoEventPtr; }
  std::shared_ptr<EventTPCMock> getCurrentEvent() const { return eventTPCPtr; }

  std::shared_ptr<EventTPCMock> eventTPCPtr;
  std::shared_ptr<Track3DMock> recoEventPtr;
};

TEST(Filters, TotalChargeLowerBound)
{
  TotalChargeLowerBound filter{0.5};

  EventSourceMock event;
  auto eventTPCMockPtr = std::make_shared<EventTPCMock>();
  event.eventTPCPtr = eventTPCMockPtr;

  EXPECT_CALL(*eventTPCMockPtr, GetTotalCharge()).WillOnce(Return(0.0));
  EXPECT_FALSE(filter(event));

  EXPECT_CALL(*eventTPCMockPtr, GetTotalCharge()).WillOnce(Return(1.0));
  EXPECT_TRUE(filter(event));
}

TEST(Filters, TotalChargeUpperBound)
{
  TotalChargeUpperBound filter{0.5};
  EventSourceMock event;
  auto eventTPCMockPtr = std::make_shared<EventTPCMock>();
  event.eventTPCPtr = eventTPCMockPtr;

  EXPECT_CALL(*event.eventTPCPtr, GetTotalCharge()).WillOnce(Return(0.0));
  EXPECT_TRUE(filter(event));
  EXPECT_CALL(*event.eventTPCPtr, GetTotalCharge()).WillOnce(Return(1.0));
  EXPECT_FALSE(filter(event));
}

TEST(Filters, MaxChargeLowerBound)
{
  MaxChargeLowerBound filter{0.5};
  EventSourceMock event;
  auto eventTPCMockPtr = std::make_shared<EventTPCMock>();
  event.eventTPCPtr = eventTPCMockPtr;

  EXPECT_CALL(*event.eventTPCPtr, GetMaxCharge()).WillOnce(Return(0.0));
  EXPECT_FALSE(filter(event));
  EXPECT_CALL(*event.eventTPCPtr, GetMaxCharge()).WillOnce(Return(1.0));
  EXPECT_TRUE(filter(event));
}

TEST(Filters, MaxChargeUpperBound)
{
  MaxChargeUpperBound filter{0.5};
  EventSourceMock event;
  auto eventTPCMockPtr = std::make_shared<EventTPCMock>();
  event.eventTPCPtr = eventTPCMockPtr;

  EXPECT_CALL(*event.eventTPCPtr, GetMaxCharge()).WillOnce(Return(0.0));
  EXPECT_TRUE(filter(event));
  EXPECT_CALL(*event.eventTPCPtr, GetMaxCharge()).WillOnce(Return(1.0));
  EXPECT_FALSE(filter(event));
}

TEST(Filters, EventIdInSetInitalizer_list)
{
  EventIdInSet filter = {2, 4};
  EventSourceMock event;
  auto eventTPCMockPtr = std::make_shared<EventTPCMock>();
  event.eventTPCPtr = eventTPCMockPtr;
  
  EventInfoMock eventInfoMock;
  eventTPCMockPtr->eventInfo = eventInfoMock;

  EXPECT_CALL(eventTPCMockPtr->eventInfo, GetEventId()).Times(1).WillOnce(Return(1));
  EXPECT_FALSE(filter(event));
  EXPECT_CALL(eventTPCMockPtr->eventInfo, GetEventId()).Times(1).WillOnce(Return(2));
  EXPECT_TRUE(filter(event));
}

TEST(Filters, EventIdInSetInsertion)
{
  EventIdInSet filter;
  EventSourceMock event;
  auto eventTPCMockPtr = std::make_shared<EventTPCMock>();
  EventInfoMock eventInfoMock;
  eventTPCMockPtr->eventInfo = eventInfoMock;

  event.eventTPCPtr = eventTPCMockPtr;
  EXPECT_CALL(eventTPCMockPtr->eventInfo, GetEventId()).Times(::testing::AtMost(1)).WillRepeatedly(Return(1));
  EXPECT_TRUE(filter(event)); // set is empty => event ID filter has no effect
  filter.insert(2);
  EXPECT_CALL(eventTPCMockPtr->eventInfo, GetEventId()).Times(1).WillRepeatedly(Return(2));
  EXPECT_TRUE(filter(event)); // set is non-empty => event ID filter is active
  EXPECT_CALL(eventTPCMockPtr->eventInfo, GetEventId()).Times(1).WillRepeatedly(Return(1));
  EXPECT_FALSE(filter(event)); // set is non-empty => event ID filter is active
}
