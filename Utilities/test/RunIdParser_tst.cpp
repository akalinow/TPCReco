#include "RunIdParser.h"
#include "gtest/gtest.h"

TEST(RunIdParser_Test, IncorrectNames) {
  EXPECT_THROW(
      RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T092536.627_0015.graw.gz"),
      std::logic_error);
  EXPECT_THROW(
      RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08-09:25:36.627_0015.graw.gz"),
      std::logic_error);
}

TEST(RunIdParser_GETFormat_Test, correctNames) {
  EXPECT_NO_THROW(
      RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:25:36.627_0015.graw.gz"));
  EXPECT_NO_THROW(RunIdParser("CoBo_2018-06-06T16:59:32.375_0000.graw"));
  EXPECT_NO_THROW(RunIdParser("CoBo1_AsAd2_2018-04-19T16:39:05.506_0001.graw"));
}

TEST(RunIdParser_GETFormat_Test, fileId) {
  EXPECT_EQ(
      RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:25:36.627_0015.graw.gz")
          .fileId(),
      15);
  EXPECT_EQ(RunIdParser("CoBo_2018-06-06T16:59:32.375_0000.graw").fileId(), 0);
  EXPECT_EQ(
      RunIdParser("CoBo1_AsAd2_2018-04-19T16:39:05.506_0001.graw").fileId(), 1);
}

TEST(RunIdParser_GETFormat_Test, RunId) {
  EXPECT_EQ(
      RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:25:36.627_0015.graw.gz")
          .runId(),
      20210908092536);
  EXPECT_EQ(RunIdParser("CoBo_2018-06-06T16:59:32.375_0000.graw").runId(),
            20180606165932);
  EXPECT_EQ(
      RunIdParser("CoBo1_AsAd2_2018-04-19T16:39:05.506_0001.graw").runId(),
      20180419163905);
}

TEST(RunIdParser_GETFormat_Test, CoBoId) {
  EXPECT_EQ(
      RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:25:36.627_0015.graw.gz")
          .CoBoId(),
      -1);
  EXPECT_EQ(RunIdParser("CoBo_2018-06-06T16:59:32.375_0000.graw").CoBoId(), -1);
  EXPECT_EQ(
      RunIdParser("CoBo1_AsAd2_2018-04-19T16:39:05.506_0001.graw").CoBoId(), 1);
}

TEST(RunIdParser_GETFormat_Test, AsAdId) {
  EXPECT_EQ(
      RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:25:36.627_0015.graw.gz")
          .AsAdId(),
      -1);
  EXPECT_EQ(RunIdParser("CoBo_2018-06-06T16:59:32.375_0000.graw").AsAdId(), -1);
  EXPECT_EQ(
      RunIdParser("CoBo1_AsAd2_2018-04-19T16:39:05.506_0001.graw").AsAdId(), 2);
}

TEST(RunIdParser_Test, timeComparison) {
  EXPECT_LE(
      RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:26:36.627_0015.graw.gz")
              .timePoint() -
          RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:26:36.626_0015.graw.gz")
              .timePoint(),
      std::chrono::milliseconds(2));
  EXPECT_GE(
      RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:26:36.627_0015.graw.gz")
              .timePoint() -
          RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:26:36.623_0015.graw.gz")
              .timePoint(),
      std::chrono::milliseconds(2));

  EXPECT_TRUE(
      RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:26:36.627_0015.graw.gz")
          .isClose(
              RunIdParser(
                  "CoBo_ALL_AsAd_ALL_2021-09-08T09:26:36.620_0015.graw.gz"),
              std::chrono::milliseconds(10)));
}

TEST(RunIdParser_Test, RootFiles) {
  EXPECT_NO_THROW(RunIdParser("EventTPC_2021-06-22T12:01:56.568_1.root"));
  EXPECT_EQ(
      RunIdParser("EventTPC_2021-06-22T12:01:56.568_1.root").fileId(), 1);
  EXPECT_EQ(RunIdParser("EventTPC_2021-06-22T12:01:56.568_1.root").runId(),
            20210622120156);
  EXPECT_EQ(
      RunIdParser("EventTPC_2021-06-22T12:01:56.568_1.root").CoBoId(), -1);
  EXPECT_EQ(
      RunIdParser("EventTPC_2021-06-22T12:01:56.568_1.root").AsAdId(), -1);
}

TEST(RunIdParser_Test, RecoFiles) {
  EXPECT_NO_THROW(RunIdParser("Reco_EventTPC_2021-06-22T12:01:56.568_1.root"));
  EXPECT_EQ(
      RunIdParser("Reco_EventTPC_2021-06-22T12:01:56.568_1.root").fileId(), 1);
  EXPECT_EQ(RunIdParser("Reco_EventTPC_2021-06-22T12:01:56.568_1.root").runId(),
            20210622120156);
  EXPECT_EQ(
      RunIdParser("Reco_EventTPC_2021-06-22T12:01:56.568_1.root").CoBoId(), -1);
  EXPECT_EQ(
      RunIdParser("Reco_EventTPC_2021-06-22T12:01:56.568_1.root").AsAdId(), -1);
}

TEST(RunIdParser_Test, RecoFilesAppended) {
  EXPECT_NO_THROW(RunIdParser("Reco_EventTPC_2021-06-22T12:01:56.568_1-2.root"));
  EXPECT_EQ(
      RunIdParser("Reco_EventTPC_2021-06-22T12:01:56.568_1-2.root").fileId(), 1);
  EXPECT_EQ(RunIdParser("Reco_EventTPC_2021-06-22T12:01:56.568_1-2.root").runId(),
            20210622120156);
  EXPECT_EQ(
      RunIdParser("Reco_EventTPC_2021-06-22T12:01:56.568_1-2.root").CoBoId(), -1);
  EXPECT_EQ(
      RunIdParser("Reco_EventTPC_2021-06-22T12:01:56.568_1-2.root").AsAdId(), -1);
}

TEST(RunIdParser_Test, OtherFormatsMinus) {
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12-01-56.568_0003.graw").fileId(), 3);
  EXPECT_EQ(RunIdParser("CoBo0_AsAd2_2021-06-22T12-01-56.568_0003.graw").runId(),
            20210622120156);
  EXPECT_EQ(
      RunIdParser("CoBo_2021-06-22T12-01-56.568_0003.graw").CoBoId(), -1);
  EXPECT_EQ(
      RunIdParser("CoBo_2021-06-22T12-01-56.568_0003.graw").AsAdId(), -1);
}

TEST(RunIdParser_Test, OtherFormatsMinus_multigraw) {
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12-01-56.568_0003.graw").fileId(), 3);
  EXPECT_EQ(RunIdParser("CoBo0_AsAd2_2021-06-22T12-01-56.568_0003.graw").runId(),
            20210622120156);
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12-01-56.568_0003.graw").CoBoId(), 0);
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12-01-56.568_0003.graw").AsAdId(), 2);
}

TEST(RunIdParser_Test, OtherFormatsUnderscore) {
  EXPECT_EQ(
      RunIdParser("CoBo_2021-06-22T12_01_56.568_0003.graw").fileId(), 3);
  EXPECT_EQ(RunIdParser("CoBo_2021-06-22T12_01_56.568_0003.graw").runId(),
            20210622120156);
  EXPECT_EQ(
      RunIdParser("CoBo_2021-06-22T12_01_56.568_0003.graw").CoBoId(), -1);
  EXPECT_EQ(
      RunIdParser("CoBo_2021-06-22T12_01_56.568_0003.graw").AsAdId(), -1);
}

TEST(RunIdParser_Test, OtherFormatsUnderscore_multigraw) {
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12_01_56.568_0003.graw").fileId(), 3);
  EXPECT_EQ(RunIdParser("CoBo0_AsAd2_2021-06-22T12_01_56.568_0003.graw").runId(),
            20210622120156);
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12_01_56.568_0003.graw").CoBoId(), 0);
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12_01_56.568_0003.graw").AsAdId(), 2);
}

TEST(RunIdParser_Test, OtherFormatsSpace) {
  EXPECT_EQ(
      RunIdParser("CoBo_2021-06-22T12 01 56.568_0003.graw").fileId(), 3);
  EXPECT_EQ(RunIdParser("CoBo_2021-06-22T12 01 56.568_0003.graw").runId(),
            20210622120156);
  EXPECT_EQ(
      RunIdParser("CoBo_2021-06-22T12 01 56.568_0003.graw").CoBoId(), -1);
  EXPECT_EQ(
      RunIdParser("CoBo_2021-06-22T12 01 56.568_0003.graw").AsAdId(), -1);
}

TEST(RunIdParser_Test, OtherFormatsSpace_multigraw) {
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12 01 56.568_0003.graw").fileId(), 3);
  EXPECT_EQ(RunIdParser("CoBo0_AsAd2_2021-06-22T12 01 56.568_0003.graw").runId(),
            20210622120156);
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12 01 56.568_0003.graw").CoBoId(), 0);
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12 01 56.568_0003.graw").AsAdId(), 2);
}