#include "CoBoClock.h"
#include "gtest/gtest.h"

using namespace tpcreco;

TEST(CoBoTimeTickTest, ratio) {
  EXPECT_EQ(cobo_time_tick::num, std::nano::num);
  EXPECT_EQ(cobo_time_tick::den * 10, std::nano::den);
}

TEST(CoBoTimeUnitTest, toNanoseconds) {
  auto coboUnits = cobo_time_unit(1);
  auto nanoseconds =
      std::chrono::duration_cast<std::chrono::nanoseconds>(coboUnits);
  EXPECT_EQ(nanoseconds.count(), 10);

  // integer conversion with no precion loss -> duration_cast not required
  nanoseconds = coboUnits;
  EXPECT_EQ(nanoseconds.count(), 10);
}

TEST(CoBoTimeUnitTest, fromNanoseconds) {

  // integer conversion with precion loss -> duration_cast required
  auto nanoseconds = std::chrono::nanoseconds(10);
  auto coboUnits = std::chrono::duration_cast<cobo_time_unit>(nanoseconds);
  EXPECT_EQ(coboUnits.count(), 1);

  nanoseconds = std::chrono::nanoseconds(17);
  coboUnits = std::chrono::duration_cast<cobo_time_unit>(nanoseconds);
  EXPECT_EQ(coboUnits.count(), 1);

  nanoseconds = std::chrono::nanoseconds(9);
  coboUnits = std::chrono::duration_cast<cobo_time_unit>(nanoseconds);
  EXPECT_EQ(coboUnits.count(), 0);
}

TEST(CoBoTimeUnitTest, fromSeconds) {
  auto seconds = std::chrono::seconds(2);
  auto coboUnits = std::chrono::duration_cast<cobo_time_unit>(seconds);
  EXPECT_EQ(coboUnits.count(), 2e8);

  // integer conversion with no precion loss -> duration_cast not required
  coboUnits = seconds;
  EXPECT_EQ(coboUnits.count(), 2e8);
}

TEST(CoBoTimeUnitTest, toSeconds) {
  // integer conversion with precion loss -> duration_cast required
  auto coboUnits = cobo_time_unit(1);
  auto seconds = std::chrono::duration_cast<std::chrono::seconds>(coboUnits);
  EXPECT_EQ(seconds.count(), 0);

  coboUnits = cobo_time_unit(1234567890);
  seconds = std::chrono::duration_cast<std::chrono::seconds>(coboUnits);
  EXPECT_EQ(seconds.count(), 12);
}

TEST(CoBoTimeUnitTest, toSecondsFloating) {
  auto coboUnits = cobo_time_unit(1);
  auto seconds =
      std::chrono::duration_cast<std::chrono::duration<double>>(coboUnits);
  EXPECT_FLOAT_EQ(seconds.count(), 1e-8);

  // floating point conversion -> duration_cast not required
  coboUnits = cobo_time_unit(1234567890);
  seconds = coboUnits;
  EXPECT_FLOAT_EQ(seconds.count(), 12.34567890);
}

TEST(CoBoTimeUnitTest, toMilisecondsFloating) {
  auto coboUnits = cobo_time_unit(1);
  auto seconds =
      std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(coboUnits);
  EXPECT_FLOAT_EQ(seconds.count(), 1e-5);

  // floating point conversion -> duration_cast not required
  coboUnits = cobo_time_unit(1234567890);
  seconds = coboUnits;
  EXPECT_FLOAT_EQ(seconds.count(), 12345.67890);
}

TEST(CoBoClockTest, nowTimePoint) {
  auto now = CoBoClock::now();
  auto then = now + std::chrono::seconds(5);
  EXPECT_EQ((then - now).count(), static_cast<int>(5e8));
}

TEST(CoBoClockTest, now) {
  auto now = CoBoClock::now();
  auto later = now + std::chrono::seconds(5);
  EXPECT_EQ((later - now).count(), static_cast<int>(5e8));
}

TEST(CoBoClockTest, fromTimeStruct) {
  std::tm tm{};
  tm.tm_year = 2020 - 1900;
  tm.tm_mon = 10 - 1;
  tm.tm_mday = 15;
  tm.tm_hour = 12;
  tm.tm_min = 30;
  tm.tm_sec = 5;
  tm.tm_isdst = -1;
  auto somewhen = CoBoClock::from_time_t(std::mktime(&tm));
  tm.tm_sec += 10;
  auto later = CoBoClock::from_time_t(std::mktime(&tm));
  EXPECT_EQ((later - somewhen).count(), static_cast<int>(10e8));
}