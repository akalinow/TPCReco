// Validation of the GET-independent GRAW reader against the GET software.
// Built only when the GET software is found.

#include <unistd.h>

#include <string>
#include <vector>

#include <get/GDataChannel.h>
#include <get/GDataFrame.h>
#include <get/GDataSample.h>
#include <get/graw2dataframe.h>

#include "gtest/gtest.h"
#include "TPCReco/GrawFile.h"

namespace {
const std::string testDataDir = TESTDATA_DIR;
const std::vector<std::string> testFiles = {
    "CoBo0_AsAd0_2022-04-12T08-03-44.531_0000.graw", "CoBo0_AsAd1_2022-04-12T08-03-44.533_0000.graw",
    "CoBo0_AsAd2_2022-04-12T08-03-44.536_0000.graw", "CoBo0_AsAd3_2022-04-12T08-03-44.540_0000.graw"};

class GrawFileVsGET : public ::testing::TestWithParam<std::string> {
protected:
  static void SetUpTestSuite() {
    // GET reads the frame formats from ./CoboFormats.xcfg
    ASSERT_EQ(chdir(RESOURCES_DIR), 0);
  }
};
} // namespace

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TEST_P(GrawFileVsGET, sameFramesHeadersAndSamples) {
  const std::string path = testDataDir + GetParam();
  graw::GrawFile file(path);
  const std::size_t nFramesGET = GET::getGrawFramesNumber(path);
  ASSERT_EQ(file.nFrames(), nFramesGET);

  graw::Frame frame;
  GET::GDataFrame frameGET;
  for (std::size_t i = 0; i < nFramesGET; ++i) {
    SCOPED_TRACE("frame " + std::to_string(i));
    ASSERT_TRUE(file.readFrame(i, frame));
    ASSERT_TRUE(GET::getGrawFrame(path, i + 1, frameGET));  // GET counts frames from 1

    const auto &h = frame.header;
    const auto &hGET = frameGET.fHeader;
    EXPECT_EQ(h.revision, hGET.fRevision);
    EXPECT_EQ(h.dataSource, hGET.fDataSource);
    EXPECT_EQ(h.eventTime, hGET.fEventTime);
    EXPECT_EQ(h.eventIdx, hGET.fEventIdx);
    EXPECT_EQ(h.coboIdx, hGET.fCoboIdx);
    EXPECT_EQ(h.asadIdx, hGET.fAsadIdx);
    EXPECT_EQ(h.readOffset, hGET.fReadOffset);
    EXPECT_EQ(h.status, hGET.fStatus);
    EXPECT_EQ(h.windowOut, hGET.fWindowOut);
    for (unsigned aget = 0; aget < graw::kAgetsPerAsad; ++aget) {
      EXPECT_EQ(h.multiplicity[aget], hGET.fMult[aget]);
      EXPECT_EQ(h.lastCell[aget], hGET.fLastCellIdx[aget]);
      // GET (version 20251024) leaves the hit patterns empty for full readout
      // frames, although the frame header has the bits set (checked in GrawFile_tst).
      if (hGET.fHitPatterns[aget].CountBits() == 0) continue;
      for (unsigned channel = 0; channel < graw::kChannelsPerAget; ++channel) {
        EXPECT_EQ(h.hitPattern[aget].test(channel), hGET.fHitPatterns[aget].TestBitNumber(channel));
      }
    }

    std::size_t nSamplesGET = 0, nSamples = 0;
    for (unsigned aget = 0; aget < graw::kAgetsPerAsad; ++aget) {
      for (unsigned channel = 0; channel < graw::kChannelsPerAget; ++channel) {
        nSamples += frame.nSamples(aget, channel);
        GET::GDataChannel *channelGET = frameGET.SearchChannel(aget, channel);
        if (!channelGET) {
          EXPECT_FALSE(frame.hasChannel(aget, channel));
          continue;
        }
        ASSERT_EQ(frame.nSamples(aget, channel), unsigned(channelGET->fNsamples));
        for (int s = 0; s < channelGET->fNsamples; ++s) {
          auto *sample = static_cast<GET::GDataSample *>(channelGET->fSamples.At(s));
          ++nSamplesGET;
          ASSERT_TRUE(frame.hasSample(aget, channel, sample->fBuckIdx));
          ASSERT_EQ(frame.sample(aget, channel, sample->fBuckIdx), sample->fValue)
              << "aget " << aget << " channel " << channel << " cell " << sample->fBuckIdx;
        }
      }
    }
    EXPECT_EQ(nSamples, nSamplesGET);
  }
}

INSTANTIATE_TEST_SUITE_P(TestData, GrawFileVsGET, ::testing::ValuesIn(testFiles));
