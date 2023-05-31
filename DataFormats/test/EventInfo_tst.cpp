#include "TPCReco/EventInfo.h"
#include "gtest/gtest.h"

using eventraw::EventInfo;
using tpcreco::eventAbsoluteTime;
using tpcreco::eventRelativeTime;

TEST(EventInfo, RelativeTime) {
  EventInfo info;
  info.SetEventTimestamp(0);
  EXPECT_EQ(eventRelativeTime(info).count(), 0);
  info.SetEventTimestamp(10);
  EXPECT_EQ(eventRelativeTime(info).count(), 10);
}

TEST(EventInfo, AbsoluteTime) {
  EventInfo info;
  info.SetRunId(19700101000000);
  info.SetEventTimestamp(0);
  EXPECT_EQ(eventAbsoluteTime(info).time_since_epoch().count(), 0);
  info.SetEventTimestamp(10);
  EXPECT_EQ(eventAbsoluteTime(info).time_since_epoch().count(), 10);

  info.SetRunId(19700101000002);
  info.SetEventTimestamp(3);
  EXPECT_EQ(eventAbsoluteTime(info).time_since_epoch().count(), 2e8 + 3);

  info.SetEventTimestamp(1e8);
  EXPECT_EQ(eventAbsoluteTime(info).time_since_epoch().count(), 3e8);
  EXPECT_EQ(std::chrono::duration_cast<std::chrono::seconds>(
                eventAbsoluteTime(info).time_since_epoch())
                .count(),
            3);

  info.SetRunId(20221208120031);
  info.SetEventTimestamp(0);
  EXPECT_EQ(eventAbsoluteTime(info).time_since_epoch().count(), 1670500831e8);
  info.SetEventTimestamp(100);
  EXPECT_EQ(eventAbsoluteTime(info).time_since_epoch().count(),
            1670500831e8 + 100);
}