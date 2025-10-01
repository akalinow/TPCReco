#include "TPCReco/EventFilter.h"
#include "TPCReco/EventInfo.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include <boost/property_tree/json_parser.hpp>
#include <functional>
#include <sstream>
#include <vector>

namespace pt = boost::property_tree;
using ::testing::Return;

struct EventInfoMock{
  EventInfoMock & operator =(const EventInfoMock &){};
  MOCK_METHOD(uint32_t, GetEventId, (), (const));
};

struct Track3DMock {
  Track3DMock() = default;
  Track3DMock(const Track3DMock&){}
  Track3DMock & operator =(const Track3DMock &){ return *this; }
  MOCK_METHOD(std::vector<int>, getSegments, (), (const));
};

struct EventTPCMock {
  EventTPCMock() = default;
  EventTPCMock(const EventTPCMock&){}
  MOCK_METHOD(double, GetTotalCharge, (), (const));
  MOCK_METHOD(double, GetMaxCharge, (), (const));
  const EventInfoMock& GetEventInfo(){ return eventInfo;};
  EventInfoMock eventInfo;
};

struct TrackBuilderMock
{
  TrackBuilderMock() = default;
  TrackBuilderMock(const TrackBuilderMock&){}
  const Track3DMock & getTrack3D(int){ return track3D; }

  Track3DMock track3D;
};


struct EventSourceMock {
  std::shared_ptr<EventTPCMock> getCurrentEvent() const { return eventTPCPtr; }
  std::shared_ptr<TrackBuilderMock> getTrackBuilder() const { return trackBuilderPtr; }
  const Track3DMock & getRecoEvent() const { return trackBuilderPtr->getTrack3D(0); }

  std::shared_ptr<TrackBuilderMock> trackBuilderPtr;
  std::shared_ptr<EventTPCMock> eventTPCPtr;
};

class EventFilterTest : public ::testing::Test {
public:
  EventFilter<std::function<bool(EventSourceMock &)>> filter;
  EventSourceMock eventSource;
  pt::ptree ptree;
};

TEST_F(EventFilterTest, DefaultBehaviour) { EXPECT_TRUE(filter.pass(eventSource)); }

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

  EventSourceMock eventSource;
  auto eventTPCMockPtr = std::make_shared<EventTPCMock>();
  eventSource.eventTPCPtr = eventTPCMockPtr;

  EXPECT_CALL(*eventTPCMockPtr, GetTotalCharge()).WillRepeatedly(Return(0));
  EXPECT_TRUE(filter.pass(eventSource));
  filter.setEnabled(true);
  EXPECT_FALSE(filter.pass(eventSource));
  filter.setEnabled(false);
  EXPECT_TRUE(filter.pass(eventSource));
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

  EventSourceMock eventSource;
  auto eventTPCMockPtr = std::make_shared<EventTPCMock>();
  eventSource.eventTPCPtr = eventTPCMockPtr;

  EXPECT_CALL(*eventTPCMockPtr, GetTotalCharge()).WillRepeatedly(Return(100));
  EXPECT_FALSE(filter.pass(eventSource));
  EXPECT_CALL(*eventTPCMockPtr, GetTotalCharge()).WillRepeatedly(Return(1500));
  EXPECT_TRUE(filter.pass(eventSource));
  EXPECT_CALL(*eventTPCMockPtr, GetTotalCharge()).WillRepeatedly(Return(5000));
  EXPECT_FALSE(filter.pass(eventSource));
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

  EventSourceMock eventSource;
  auto eventTPCMockPtr = std::make_shared<EventTPCMock>();
  eventSource.eventTPCPtr = eventTPCMockPtr;

  EXPECT_CALL(*eventTPCMockPtr, GetMaxCharge()).WillRepeatedly(Return(100));
  EXPECT_FALSE(filter.pass(eventSource));
  EXPECT_CALL(*eventTPCMockPtr, GetMaxCharge()).WillRepeatedly(Return(1500));
  EXPECT_TRUE(filter.pass(eventSource));
  EXPECT_CALL(*eventTPCMockPtr, GetMaxCharge()).WillRepeatedly(Return(5000));
  EXPECT_FALSE(filter.pass(eventSource));
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

  EventSourceMock eventSource;
  auto eventTPCMockPtr = std::make_shared<EventTPCMock>();
  eventSource.eventTPCPtr = eventTPCMockPtr;

  EventInfoMock eventInfoMock;
  eventTPCMockPtr->eventInfo = eventInfoMock;

  EXPECT_CALL(eventTPCMockPtr->eventInfo, GetEventId()).WillOnce(Return(0));
  EXPECT_FALSE(filter.pass(eventSource));
  EXPECT_CALL(eventTPCMockPtr->eventInfo, GetEventId()).WillOnce(Return(1));
  EXPECT_TRUE(filter.pass(eventSource));
  EXPECT_CALL(eventTPCMockPtr->eventInfo, GetEventId()).WillOnce(Return(2));
  EXPECT_TRUE(filter.pass(eventSource));
  EXPECT_CALL(eventTPCMockPtr->eventInfo, GetEventId()).WillOnce(Return(3));
  EXPECT_FALSE(filter.pass(eventSource));
}

TEST_F(EventFilterTest, recoProngs) {
  std::stringstream config{R"(
{
   "eventFilter": {
     "recoProngs": [2]
   }
 }
   )"};


  pt::read_json(config, ptree);
  filter.setConditions(ptree);
  filter.setEnabled(true);

  EventSourceMock eventSource;
  auto trackBuilderMockPtr = std::make_shared<TrackBuilderMock>();
  Track3DMock track3DMock;

  eventSource.trackBuilderPtr = trackBuilderMockPtr;
  trackBuilderMockPtr->track3D = track3DMock;

  EXPECT_CALL(trackBuilderMockPtr->track3D, getSegments()).WillOnce(Return(std::vector<int>{1}));
  EXPECT_FALSE(filter.pass(eventSource));
  EXPECT_CALL(trackBuilderMockPtr->track3D, getSegments()).WillOnce(Return(std::vector<int>{1, 2}));
  EXPECT_TRUE(filter.pass(eventSource));
  EXPECT_CALL(trackBuilderMockPtr->track3D, getSegments()).WillOnce(Return(std::vector<int>{1, 2, 3}));
  EXPECT_FALSE(filter.pass(eventSource));

}
