// Tests of the GET-independent GRAW reader.
// Reference values were obtained with an independent decoder (numpy).

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>

#include "gtest/gtest.h"
#include "TPCReco/GrawFile.h"

namespace fs = boost::filesystem;
using namespace graw;

namespace {
const std::string testDataDir = TESTDATA_DIR;
const std::string fileAsad2 = testDataDir + "CoBo0_AsAd2_2022-04-12T08-03-44.536_0000.graw";
constexpr uint32_t kFullFrameSize = 278784;  // 256 B header + 4*68*512 samples * 2 B

std::vector<char> readFile(const std::string &path) {
  std::ifstream in(path, std::ios::binary);
  return std::vector<char>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

// Partial readout (frame type 1) frame with the given samples.
std::vector<char> makePartialFrame(uint32_t eventIdx, uint8_t asad,
                                   const std::vector<std::array<uint32_t, 4>> &samples) {
  const uint32_t blockSize = 256, headerSize = 256;
  const uint32_t frameSize = headerSize + ((4 * samples.size() + blockSize - 1) / blockSize) * blockSize;
  std::vector<char> frame(frameSize, 0);
  auto put = [&frame](std::size_t offset, uint64_t value, std::size_t n) {
    for (std::size_t k = 0; k < n; ++k) frame[offset + k] = char((value >> (8 * (n - 1 - k))) & 0xFF);
  };
  put(0, 8, 1);                          // big-endian, block size 2^8
  put(1, frameSize / blockSize, 3);
  put(5, kFrameTypePartialReadout, 2);
  put(7, 5, 1);                          // revision
  put(8, headerSize / blockSize, 2);
  put(10, 4, 2);                         // item size
  put(12, samples.size(), 4);
  put(16, 123456, 6);                    // event time
  put(22, eventIdx, 4);
  put(27, asad, 1);
  for (std::size_t i = 0; i < samples.size(); ++i) {
    const auto &s = samples[i];  // aget, channel, cell, value
    put(headerSize + 4 * i, (s[0] << 30) | (s[1] << 23) | (s[2] << 14) | s[3], 4);
  }
  return frame;
}

std::string writeTempFile(const std::vector<char> &content) {
  const std::string path = (fs::temp_directory_path() / fs::unique_path("graw_%%%%%%.graw")).string();
  std::ofstream out(path, std::ios::binary);
  out.write(content.data(), std::streamsize(content.size()));
  return path;
}
} // namespace

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TEST(GrawFile, openAndCountFrames) {
  GrawFile file;
  ASSERT_TRUE(file.open(fileAsad2));
  EXPECT_EQ(file.nFrames(), 10u);
  EXPECT_FALSE(GrawFile().open(testDataDir + "no_such_file.graw"));
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TEST(GrawFile, header) {
  GrawFile file(fileAsad2);
  FrameHeader header;
  ASSERT_TRUE(file.readHeader(3, header));
  EXPECT_FALSE(header.littleEndian);
  EXPECT_EQ(header.blockSize, 256u);
  EXPECT_EQ(header.frameSize, kFullFrameSize);
  EXPECT_EQ(header.frameType, kFrameTypeFullReadout);
  EXPECT_EQ(header.revision, 5);
  EXPECT_EQ(header.headerSize, 256u);
  EXPECT_EQ(header.itemSize, 2);
  EXPECT_EQ(header.itemCount, 139264u);
  EXPECT_EQ(header.eventIdx, 83u);
  EXPECT_EQ(header.eventTime, 424423606u);
  EXPECT_EQ(header.coboIdx, 0);
  EXPECT_EQ(header.asadIdx, 2);
  EXPECT_EQ(header.windowOut, 4294967295u);
  EXPECT_EQ(header.multiplicity[3], 8);
  EXPECT_EQ(header.lastCell[0], 310);
  EXPECT_EQ(header.hitPattern[0].count(), 68u);
  EXPECT_TRUE(header.hitPattern[0].test(67));
  EXPECT_FALSE(header.hitPattern[0].test(68));

  for (std::size_t i = 0; i < file.nFrames(); ++i) {
    ASSERT_TRUE(file.readHeader(i, header));
    EXPECT_EQ(header.eventIdx, 80u + i);
  }
  EXPECT_FALSE(file.readHeader(file.nFrames(), header));
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TEST(GrawFile, fullReadoutSamples) {
  GrawFile file(fileAsad2);
  Frame frame;
  ASSERT_TRUE(file.readFrame(3, frame));
  EXPECT_EQ(frame.header.eventIdx, 83u);
  for (unsigned aget = 0; aget < kAgetsPerAsad; ++aget) {
    for (unsigned channel = 0; channel < kChannelsPerAget; ++channel) {
      EXPECT_EQ(frame.nSamples(aget, channel), kCellsPerChannel);
    }
  }
  EXPECT_EQ(frame.sample(1, 10, 100), 329);
  EXPECT_EQ(frame.sample(3, 67, 511), 294);
  EXPECT_EQ(frame.sample(0, 0, 0), 345);

  long sumAget0 = 0, sumAll = 0;
  for (unsigned aget = 0; aget < kAgetsPerAsad; ++aget) {
    for (unsigned channel = 0; channel < kChannelsPerAget; ++channel) {
      unsigned lastCell = 0;
      frame.forEachSample(aget, channel, [&](unsigned cell, uint16_t value) {
        EXPECT_LE(value, 0xFFF);
        lastCell = cell;
        sumAll += value;
        if (aget == 0) sumAget0 += value;
      });
      EXPECT_EQ(lastCell, kCellsPerChannel - 1);
    }
  }
  EXPECT_EQ(sumAget0, 10582625);
  EXPECT_EQ(sumAll, 42877598);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TEST(GrawFile, trailingBytesAreIgnored) {
  auto content = readFile(fileAsad2);
  content.resize(content.size() + 12, 0);
  const std::string path = writeTempFile(content);
  GrawFile file(path);
  EXPECT_EQ(file.nFrames(), 10u);
  Frame frame;
  EXPECT_TRUE(file.readFrame(9, frame));
  EXPECT_EQ(frame.header.eventIdx, 89u);
  fs::remove(path);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TEST(GrawFile, partialReadout) {
  const std::vector<std::array<uint32_t, 4>> samples = {
      {0, 5, 10, 100}, {0, 5, 11, 200}, {2, 67, 511, 4095}, {3, 0, 0, 1}};
  const std::string path = writeTempFile(makePartialFrame(7, 1, samples));
  GrawFile file(path);
  ASSERT_EQ(file.nFrames(), 1u);
  Frame frame;
  ASSERT_TRUE(file.readFrame(0, frame));
  EXPECT_EQ(frame.header.frameType, kFrameTypePartialReadout);
  EXPECT_EQ(frame.header.eventIdx, 7u);
  EXPECT_EQ(frame.header.asadIdx, 1);
  EXPECT_EQ(frame.nSamples(0, 5), 2u);
  EXPECT_EQ(frame.sample(0, 5, 11), 200);
  EXPECT_EQ(frame.sample(2, 67, 511), 4095);
  EXPECT_TRUE(frame.hasSample(3, 0, 0));
  EXPECT_FALSE(frame.hasSample(3, 0, 1));
  EXPECT_FALSE(frame.hasChannel(1, 0));
  fs::remove(path);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TEST(GrawFile, mixedFrameSizes) {
  // full readout frames followed by a smaller partial readout frame:
  // the reader must leave the fixed frame size mode and index the frames
  const auto original = readFile(fileAsad2);
  std::vector<char> content(original.begin(), original.begin() + 2 * kFullFrameSize);
  const auto partial = makePartialFrame(1000, 2, {{1, 1, 1, 11}});
  content.insert(content.end(), partial.begin(), partial.end());
  content.insert(content.end(), original.begin() + 3 * kFullFrameSize, original.begin() + 4 * kFullFrameSize);
  const std::string path = writeTempFile(content);

  GrawFile file(path);
  Frame frame;
  ASSERT_TRUE(file.readFrame(2, frame));
  EXPECT_EQ(frame.header.eventIdx, 1000u);
  EXPECT_EQ(frame.sample(1, 1, 1), 11);
  EXPECT_EQ(file.nFrames(), 4u);
  ASSERT_TRUE(file.readFrame(3, frame));
  EXPECT_EQ(frame.header.eventIdx, 83u);
  EXPECT_EQ(frame.sample(1, 10, 100), 329);
  fs::remove(path);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TEST(GrawFile, growingFile) {
  const auto content = readFile(fileAsad2);
  const std::string path = writeTempFile(std::vector<char>(content.begin(), content.begin() + kFullFrameSize + 100));
  GrawFile file(path);
  EXPECT_EQ(file.nFrames(), 1u);  // second frame incomplete
  {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(content.data(), 3 * kFullFrameSize);
  }
  EXPECT_EQ(file.update(), 3u);
  FrameHeader header;
  ASSERT_TRUE(file.readHeader(2, header));
  EXPECT_EQ(header.eventIdx, 82u);
  fs::remove(path);
}
