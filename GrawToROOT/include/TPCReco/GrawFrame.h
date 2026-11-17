#ifndef TPCRECO_GRAWFRAME_H
#define TPCRECO_GRAWFRAME_H

// Decoded content of a single GRAW (MFM) frame written by a CoBo board:
// the data of one ASAD board (4 AGET chips x 68 channels x 512 time cells)
// for one event. Replaces GET::GDataFrame from the GET software.

#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace graw {

constexpr unsigned kAgetsPerAsad = 4;
constexpr unsigned kChannelsPerAget = 68;   // including FPN channels
constexpr unsigned kCellsPerChannel = 512;  // AGET switched capacitor array

// MFM frame types of CoBo data
constexpr uint16_t kFrameTypePartialReadout = 1;  // items: aget, channel, cell, sample (4 B)
constexpr uint16_t kFrameTypeFullReadout = 2;     // items: aget, sample (2 B), implicit order

struct FrameHeader {
  // MFM primary header
  bool littleEndian{false};
  bool blob{false};
  uint32_t blockSize{0};   // [B]
  uint32_t frameSize{0};   // [B]
  uint8_t dataSource{0};
  uint16_t frameType{0};
  uint8_t revision{0};
  uint32_t headerSize{0};  // [B]
  uint16_t itemSize{0};    // [B]
  uint32_t itemCount{0};
  // CoBo header
  uint64_t eventTime{0};
  uint32_t eventIdx{0};
  uint8_t coboIdx{0};
  uint8_t asadIdx{0};
  uint16_t readOffset{0};
  uint8_t status{0};
  std::array<std::bitset<72>, kAgetsPerAsad> hitPattern{};  // bit = AGET channel
  std::array<uint16_t, kAgetsPerAsad> multiplicity{};
  uint32_t windowOut{0};                                  // revision >= 3
  std::array<uint16_t, kAgetsPerAsad> lastCell{};         // revision >= 4

  bool isCoBoData() const {
    return frameType == kFrameTypePartialReadout || frameType == kFrameTypeFullReadout;
  }
};

class Frame {
public:
  Frame();

  FrameHeader header;

  // Remove all samples (header is kept).
  void clear();

  // Mark all samples as present (used by full readout decoding).
  void setAllPresent();

  bool hasChannel(unsigned aget, unsigned channel) const {
    return nSamples_[channelIndex(aget, channel)] > 0;
  }
  unsigned nSamples(unsigned aget, unsigned channel) const {
    return nSamples_[channelIndex(aget, channel)];
  }
  bool hasSample(unsigned aget, unsigned channel, unsigned cell) const {
    const std::size_t i = sampleIndex(aget, channel, cell);
    return (mask_[i / 64] >> (i % 64)) & 1u;
  }
  // ADC value of a sample; 0 if the sample was not read out.
  uint16_t sample(unsigned aget, unsigned channel, unsigned cell) const {
    return values_[sampleIndex(aget, channel, cell)];
  }

  void setSample(unsigned aget, unsigned channel, unsigned cell, uint16_t value);

  // Call f(cell, value) for each read-out sample of a channel, in cell order.
  template <class F>
  void forEachSample(unsigned aget, unsigned channel, F &&f) const {
    if (!hasChannel(aget, channel)) return;
    const std::size_t first = sampleIndex(aget, channel, 0);
    for (unsigned cell = 0; cell < kCellsPerChannel; ++cell) {
      const std::size_t i = first + cell;
      if ((mask_[i / 64] >> (i % 64)) & 1u) f(cell, values_[i]);
    }
  }

  // Raw access: values ordered as [aget][channel][cell].
  uint16_t *data() { return values_.data(); }
  const uint16_t *data() const { return values_.data(); }

  static std::size_t channelIndex(unsigned aget, unsigned channel) {
    return aget * kChannelsPerAget + channel;
  }
  static std::size_t sampleIndex(unsigned aget, unsigned channel, unsigned cell) {
    return channelIndex(aget, channel) * kCellsPerChannel + cell;
  }

private:
  static constexpr std::size_t kNSamples = kAgetsPerAsad * kChannelsPerAget * kCellsPerChannel;
  std::vector<uint16_t> values_;
  std::vector<uint64_t> mask_;
  std::array<uint16_t, kAgetsPerAsad * kChannelsPerAget> nSamples_{};
};

} // namespace graw
#endif
