#include "RunIdParser.h"
#include "gtest/gtest.h"

TEST(RunIdParser_Test, IncorrectNames) {
  EXPECT_THROW(RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T092536.627_0015.graw.gz"), std::logic_error);
  EXPECT_THROW(RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08-09:25:36.627_0015.graw.gz"), std::logic_error);
}

TEST(RunIdParser_GETFormat_Test, correctNames) {
  EXPECT_NO_THROW(RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:25:36.627_0015.graw.gz"));
  EXPECT_NO_THROW(RunIdParser("CoBo_2018-06-06T16:59:32.375_0000.graw"));
  EXPECT_NO_THROW(RunIdParser("CoBo0_AsAd0_2018-04-19T16:39:05.506_0001.graw"));
}

TEST(RunIdParser_GETFormat_Test, fileId) {
  EXPECT_EQ(RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:25:36.627_0015.graw.gz").fileId(), 15);
  EXPECT_EQ(RunIdParser("CoBo_2018-06-06T16:59:32.375_0000.graw").fileId(), 0);
  EXPECT_EQ(RunIdParser("CoBo0_AsAd0_2018-04-19T16:39:05.506_0001.graw").fileId(), 1);
}

TEST(RunIdParser_GETFormat_Test, RunId) {
  EXPECT_EQ(RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:25:36.627_0015.graw.gz").runId(), 20210908092536);
  EXPECT_EQ(RunIdParser("CoBo_2018-06-06T16:59:32.375_0000.graw").runId(), 20180606165932);
  EXPECT_EQ(RunIdParser("CoBo0_AsAd0_2018-04-19T16:39:05.506_0001.graw").runId(), 20180419163905);
}

TEST(RunIdParser_GETFormat_Test, CoBoId) {
  EXPECT_EQ(RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:25:36.627_0015.graw.gz").CoBoId(), -1);
  EXPECT_EQ(RunIdParser("CoBo_2018-06-06T16:59:32.375_0000.graw").CoBoId(), -1);
  EXPECT_EQ(RunIdParser("CoBo0_AsAd0_2018-04-19T16:39:05.506_0001.graw").CoBoId(), 0);
}

TEST(RunIdParser_GETFormat_Test, AsAdId) {
  EXPECT_EQ(RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:25:36.627_0015.graw.gz").AsadId(), -1);
  EXPECT_EQ(RunIdParser("CoBo_2018-06-06T16:59:32.375_0000.graw").AsadId(), -1);
  EXPECT_EQ(RunIdParser("CoBo0_AsAd0_2018-04-19T16:39:05.506_0001.graw").runId(), 0);
}
