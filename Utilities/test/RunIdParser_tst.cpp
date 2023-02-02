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

TEST(RunIdParser_GETFormat_Test, nonStandardNames) {
  EXPECT_NO_THROW(
      RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09 25 36.627_0015.graw.gz"));
  EXPECT_NO_THROW(RunIdParser("CoBo1_AsAd2_2018-04-19T16 39 05.506_0001.graw"));
  EXPECT_NO_THROW(RunIdParser("CoBo_2018-06-06T16 59 32.375_0000.graw"));

  EXPECT_NO_THROW(
      RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09?25?36.627_0015.graw.gz"));
  EXPECT_NO_THROW(RunIdParser("CoBo1_AsAd2_2018-04-19T16?39?05.506_0001.graw"));
  EXPECT_NO_THROW(RunIdParser("CoBo_2018-06-06T16?59?32.375_0000.graw"));

  EXPECT_NO_THROW(
      RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09%25%36.627_0015.graw.gz"));
  EXPECT_NO_THROW(RunIdParser("CoBo1_AsAd2_2018-04-19T16%39%05.506_0001.graw"));
  EXPECT_NO_THROW(RunIdParser("CoBo_2018-06-06T16%59%32.375_0000.graw"));

  EXPECT_NO_THROW(RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09\x5"
                              "25\x5"
                              "36.627_0015.graw.gz"));
  EXPECT_NO_THROW(RunIdParser("CoBo1_AsAd2_2018-04-19T16\x5"
                              "39\x5"
                              "05.506_0001.graw"));
  EXPECT_NO_THROW(RunIdParser("CoBo_2018-06-06T16\x5"
                              "59\x5"
                              "32.375_0000.graw"));
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
              .exactTimePoint() -
          RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:26:36.626_0015.graw.gz")
              .exactTimePoint(),
      std::chrono::milliseconds(2));
  EXPECT_GE(
      RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:26:36.627_0015.graw.gz")
              .exactTimePoint() -
          RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:26:36.623_0015.graw.gz")
              .exactTimePoint(),
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
  EXPECT_EQ(RunIdParser("EventTPC_2021-06-22T12:01:56.568_1.root").fileId(), 1);
  EXPECT_EQ(RunIdParser("EventTPC_2021-06-22T12:01:56.568_1.root").runId(),
            20210622120156);
  EXPECT_EQ(RunIdParser("EventTPC_2021-06-22T12:01:56.568_1.root").CoBoId(),
            -1);
  EXPECT_EQ(RunIdParser("EventTPC_2021-06-22T12:01:56.568_1.root").AsAdId(),
            -1);
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
  EXPECT_NO_THROW(
      RunIdParser("Reco_EventTPC_2021-06-22T12:01:56.568_1-2.root"));
  EXPECT_EQ(
      RunIdParser("Reco_EventTPC_2021-06-22T12:01:56.568_1-2.root").fileId(),
      1);
  EXPECT_EQ(
      RunIdParser("Reco_EventTPC_2021-06-22T12:01:56.568_1-2.root").runId(),
      20210622120156);
  EXPECT_EQ(
      RunIdParser("Reco_EventTPC_2021-06-22T12:01:56.568_1-2.root").CoBoId(),
      -1);
  EXPECT_EQ(
      RunIdParser("Reco_EventTPC_2021-06-22T12:01:56.568_1-2.root").AsAdId(),
      -1);
}

TEST(RunIdParser_Test, OtherFormatsMinus) {
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12-01-56.568_0003.graw").fileId(), 3);
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12-01-56.568_0003.graw").runId(),
      20210622120156);
  EXPECT_EQ(RunIdParser("CoBo_2021-06-22T12-01-56.568_0003.graw").CoBoId(), -1);
  EXPECT_EQ(RunIdParser("CoBo_2021-06-22T12-01-56.568_0003.graw").AsAdId(), -1);
}

TEST(RunIdParser_Test, OtherFormatsMinus_multigraw) {
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12-01-56.568_0003.graw").fileId(), 3);
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12-01-56.568_0003.graw").runId(),
      20210622120156);
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12-01-56.568_0003.graw").CoBoId(), 0);
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12-01-56.568_0003.graw").AsAdId(), 2);
}

TEST(RunIdParser_Test, OtherFormatsUnderscore) {
  EXPECT_EQ(RunIdParser("CoBo_2021-06-22T12_01_56.568_0003.graw").fileId(), 3);
  EXPECT_EQ(RunIdParser("CoBo_2021-06-22T12_01_56.568_0003.graw").runId(),
            20210622120156);
  EXPECT_EQ(RunIdParser("CoBo_2021-06-22T12_01_56.568_0003.graw").CoBoId(), -1);
  EXPECT_EQ(RunIdParser("CoBo_2021-06-22T12_01_56.568_0003.graw").AsAdId(), -1);
}

TEST(RunIdParser_Test, OtherFormatsUnderscore_multigraw) {
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12_01_56.568_0003.graw").fileId(), 3);
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12_01_56.568_0003.graw").runId(),
      20210622120156);
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12_01_56.568_0003.graw").CoBoId(), 0);
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12_01_56.568_0003.graw").AsAdId(), 2);
}

TEST(RunIdParser_Test, OtherFormatsSpace) {
  EXPECT_EQ(RunIdParser("CoBo_2021-06-22T12 01 56.568_0003.graw").fileId(), 3);
  EXPECT_EQ(RunIdParser("CoBo_2021-06-22T12 01 56.568_0003.graw").runId(),
            20210622120156);
  EXPECT_EQ(RunIdParser("CoBo_2021-06-22T12 01 56.568_0003.graw").CoBoId(), -1);
  EXPECT_EQ(RunIdParser("CoBo_2021-06-22T12 01 56.568_0003.graw").AsAdId(), -1);
}

TEST(RunIdParser_Test, OtherFormatsSpace_multigraw) {
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12 01 56.568_0003.graw").fileId(), 3);
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12 01 56.568_0003.graw").runId(),
      20210622120156);
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12 01 56.568_0003.graw").CoBoId(), 0);
  EXPECT_EQ(
      RunIdParser("CoBo0_AsAd2_2021-06-22T12 01 56.568_0003.graw").AsAdId(), 2);
}

TEST(RunIdParser_Test, timePointFromRunId) {
  EXPECT_TRUE(
      RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:26:36.127_0015.graw.gz")
          .isClose(RunId(20210908092636).toTimePoint(),
                   std::chrono::seconds(1)));

  auto parsed =
      RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:26:36.127_0015.graw.gz");
  EXPECT_TRUE(
      parsed.isClose(parsed.runId().toTimePoint(), std::chrono::seconds(1)));
}

TEST(RunIdParser_Test, unixTimestamp) {
  EXPECT_EQ(
      std::chrono::duration_cast<std::chrono::seconds>(
          RunIdParser("CoBo_ALL_AsAd_ALL_1970-01-01T00:00:00.127_0001.graw")
              .exactTimePoint()
              .time_since_epoch())
          .count(),
      0);

  EXPECT_EQ(
      std::chrono::duration_cast<std::chrono::seconds>(
          RunIdParser("CoBo_ALL_AsAd_ALL_1970-01-01T00:00:01.127_0001.graw")
              .exactTimePoint()
              .time_since_epoch())
          .count(),
      1);

  EXPECT_EQ(
      std::chrono::duration_cast<std::chrono::seconds>(
          RunIdParser("CoBo_ALL_AsAd_ALL_1970-01-02T00:00:00.127_0001.graw")
              .exactTimePoint()
              .time_since_epoch())
          .count(),
      24 * 3600);

  EXPECT_EQ(
      std::chrono::duration_cast<std::chrono::seconds>(
          RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:26:36.127_0015.graw.gz")
              .exactTimePoint()
              .time_since_epoch())
          .count(),
      1631093196);
}

TEST(RunIdParser_Test, unixTimestampFromRunId) {
  EXPECT_EQ(std::chrono::duration_cast<std::chrono::seconds>(
                RunId(19700101000000).toTimePoint().time_since_epoch())
                .count(),
            0);

  EXPECT_EQ(std::chrono::duration_cast<std::chrono::seconds>(
                RunId(19700101000001).toTimePoint().time_since_epoch())
                .count(),
            1);

  EXPECT_EQ(std::chrono::duration_cast<std::chrono::seconds>(
                RunId(20210908092636).toTimePoint().time_since_epoch())
                .count(),
            1631093196);
}

TEST(RunIdParser_Test, timePointVsExactTimePoint) {
  auto parsed =
      RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:26:36.127_0015.graw.gz");
  EXPECT_EQ(
      (parsed.exactTimePoint() - parsed.runId().toTimePoint()).count(),
      std::chrono::duration_cast<typename RunIdParser::time_point::duration>(
          std::chrono::milliseconds(127))
          .count());
}

TEST(RunIdParser_Test, parsingTimePointVsRunIdTimePoint) {
  EXPECT_EQ(
      (RunIdParser("CoBo_ALL_AsAd_ALL_2021-09-08T09:26:36.127_0015.graw.gz")
           .runId()
           .toTimePoint() -
       RunId(20210908092636).toTimePoint())
          .count(),
      0);
}

TEST(RunIdParser_Test, RunIdIsRegular) {
    EXPECT_EQ(RunId(0).isRegular(), false);
    EXPECT_EQ(RunId(1).isRegular(), false);
    EXPECT_EQ(RunId(999999).isRegular(), false);
    EXPECT_EQ(RunId(1000000).isRegular(), true);
}


TEST(RunIdParser_Test, RunIdToTimePointIrregular) {
    EXPECT_EQ(RunId(0).toTimePoint().time_since_epoch().count(), 0);
    EXPECT_EQ(RunId(1).toTimePoint().time_since_epoch().count(), 0);
    EXPECT_EQ(RunId(999999).toTimePoint().time_since_epoch().count(), 0);
    EXPECT_THROW(RunId(9999999).toTimePoint().time_since_epoch().count(), std::exception);
}