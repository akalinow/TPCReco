#include "TPCReco/GrawFile.h"

#include <sys/stat.h>

#include <algorithm>
#include <iostream>

namespace graw {
namespace {
/////////////////////////////////////////////////////////
// Unsigned integer of nBytes bytes at buffer[offset].
/////////////////////////////////////////////////////////
uint64_t readUInt(const uint8_t *buffer, std::size_t offset, std::size_t nBytes, bool littleEndian) {
  uint64_t value = 0;
  for (std::size_t k = 0; k < nBytes; ++k) {
    const std::size_t index = littleEndian ? offset + nBytes - 1 - k : offset + k;
    value = (value << 8) | buffer[index];
  }
  return value;
}
/////////////////////////////////////////////////////////
// 9 B AGET hit pattern: bit j of the field corresponds to channel j.
/////////////////////////////////////////////////////////
std::bitset<72> readHitPattern(const uint8_t *buffer, std::size_t offset, bool littleEndian) {
  std::bitset<72> pattern;
  for (std::size_t k = 0; k < 9; ++k) {
    const uint8_t byte = buffer[offset + k];
    const std::size_t significance = littleEndian ? k : 8 - k;  // byte position in the value
    for (unsigned b = 0; b < 8; ++b) {
      if ((byte >> b) & 1u) pattern.set(8 * significance + b);
    }
  }
  return pattern;
}
} // namespace
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool GrawFile::decodePrimaryHeader(const uint8_t *buffer, std::size_t size, FrameHeader &header) {
  if (size < kPrimaryHeaderSize) return false;
  const uint8_t metaType = buffer[0];
  header.littleEndian = (metaType >> 7) & 1u;
  header.blob = (metaType >> 6) & 1u;
  header.blockSize = uint32_t(1) << (metaType & 0x0F);
  const bool le = header.littleEndian;
  header.frameSize = uint32_t(readUInt(buffer, 1, 3, le)) * header.blockSize;
  header.dataSource = buffer[4];
  header.frameType = uint16_t(readUInt(buffer, 5, 2, le));
  header.revision = buffer[7];
  return header.frameSize >= kPrimaryHeaderSize;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool GrawFile::decodeHeader(const uint8_t *buffer, std::size_t size, FrameHeader &header) {
  header = FrameHeader();
  if (!decodePrimaryHeader(buffer, size, header)) return false;
  const bool le = header.littleEndian;
  if (header.blob) {  // blob frames have only the primary header
    header.headerSize = kPrimaryHeaderSize;
    return true;
  }
  if (size < 16) return false;
  header.headerSize = uint32_t(readUInt(buffer, 8, 2, le)) * header.blockSize;
  header.itemSize = uint16_t(readUInt(buffer, 10, 2, le));
  header.itemCount = uint32_t(readUInt(buffer, 12, 4, le));
  if (header.headerSize > header.frameSize) return false;
  if (!header.isCoBoData()) return true;

  const std::size_t needed = header.revision >= 4 ? 87 : header.revision >= 3 ? 79 : kMinCoBoHeaderSize;
  if (size < needed || header.headerSize < needed) return false;
  header.eventTime = readUInt(buffer, 16, 6, le);
  header.eventIdx = uint32_t(readUInt(buffer, 22, 4, le));
  header.coboIdx = buffer[26];
  header.asadIdx = buffer[27];
  header.readOffset = uint16_t(readUInt(buffer, 28, 2, le));
  header.status = buffer[30];
  for (unsigned aget = 0; aget < kAgetsPerAsad; ++aget) {
    header.hitPattern[aget] = readHitPattern(buffer, 31 + 9 * aget, le);
    header.multiplicity[aget] = uint16_t(readUInt(buffer, 67 + 2 * aget, 2, le));
  }
  if (header.revision >= 3) header.windowOut = uint32_t(readUInt(buffer, 75, 4, le));
  if (header.revision >= 4) {
    for (unsigned aget = 0; aget < kAgetsPerAsad; ++aget) {
      header.lastCell[aget] = uint16_t(readUInt(buffer, 79 + 2 * aget, 2, le));
    }
  }
  return true;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool GrawFile::decodeData(const uint8_t *buffer, std::size_t size, Frame &frame) {
  const FrameHeader &header = frame.header;
  frame.clear();
  if (!header.isCoBoData()) return false;
  const std::size_t dataSize = std::size_t(header.itemCount) * header.itemSize;
  if (header.headerSize + dataSize > size) {
    std::cerr << "graw::GrawFile: frame data truncated: " << header.itemCount << " items of "
              << header.itemSize << " B do not fit in " << size << " B" << std::endl;
    return false;
  }
  const uint8_t *items = buffer + header.headerSize;
  const bool le = header.littleEndian;

  if (header.frameType == kFrameTypePartialReadout) {
    // 4 B item: aget[31:30] channel[29:23] cell[22:14] sample[11:0]
    if (header.itemSize != 4) return false;
    for (uint32_t i = 0; i < header.itemCount; ++i) {
      const uint32_t item = uint32_t(readUInt(items, 4 * i, 4, le));
      frame.setSample((item >> 30) & 0x3, (item >> 23) & 0x7F, (item >> 14) & 0x1FF, item & 0xFFF);
    }
    return true;
  }

  // Full readout, 2 B item: aget[15:14] sample[11:0].
  // Items of each AGET come in the order cell 0: channels 0-67, cell 1: channels 0-67, ...
  if (header.itemSize != 2) return false;
  std::array<uint32_t, kAgetsPerAsad> count{};
  uint16_t *values = frame.data();
  for (uint32_t i = 0; i < header.itemCount; ++i) {
    const uint16_t item = uint16_t(le ? (items[2 * i] | (items[2 * i + 1] << 8))
                                      : ((items[2 * i] << 8) | items[2 * i + 1]));
    const unsigned aget = (item >> 14) & 0x3;
    const uint32_t k = count[aget]++;
    const unsigned cell = k / kChannelsPerAget;
    const unsigned channel = k % kChannelsPerAget;
    if (cell >= kCellsPerChannel) continue;  // more items than cells: ignore the excess
    values[Frame::sampleIndex(aget, channel, cell)] = item & 0xFFF;
  }
  const uint32_t nFull = kChannelsPerAget * kCellsPerChannel;
  if (std::all_of(count.begin(), count.end(), [nFull](uint32_t n) { return n == nFull; })) {
    frame.setAllPresent();
  } else {  // incomplete frame: mark only the samples that were read
    for (unsigned aget = 0; aget < kAgetsPerAsad; ++aget) {
      for (uint32_t k = 0; k < std::min(count[aget], nFull); ++k) {
        const unsigned cell = k / kChannelsPerAget, channel = k % kChannelsPerAget;
        frame.setSample(aget, channel, cell, frame.sample(aget, channel, cell));
      }
    }
  }
  return true;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool GrawFile::open(const std::string &path) {
  close();
  path_ = path;
  in_.open(path, std::ios::in | std::ios::binary);
  if (!in_.is_open()) {
    std::cerr << "graw::GrawFile: cannot open file: " << path << std::endl;
    return false;
  }
  update();
  return true;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void GrawFile::close() {
  if (in_.is_open()) in_.close();
  in_.clear();
  path_.clear();
  fileSize_ = 0;
  uniform_ = false;
  firstHeader_ = FrameHeader();
  nUniformFrames_ = 0;
  offsets_.clear();
  scanEnd_ = 0;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
uint64_t GrawFile::currentFileSize() {
  struct stat info;
  if (stat(path_.c_str(), &info) != 0) return 0;
  return uint64_t(info.st_size);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::size_t GrawFile::update() {
  if (!in_.is_open()) return 0;
  fileSize_ = currentFileSize();
  if (uniform_) {
    nUniformFrames_ = fileSize_ / firstHeader_.frameSize;
  } else if (offsets_.empty()) {
    // First look at the file: full readout frames all have the same size,
    // so their offsets can be computed instead of scanned.
    FrameHeader header;
    if (fileSize_ >= kPrimaryHeaderSize && readFrameHeaderAt(0, header) &&
        header.frameType == kFrameTypeFullReadout && fileSize_ >= header.frameSize) {
      uniform_ = true;
      firstHeader_ = header;
      nUniformFrames_ = fileSize_ / header.frameSize;
    } else {
      scanFrom(0);
    }
  } else {
    scanFrom(scanEnd_);
  }
  return nFrames();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void GrawFile::scanFrom(uint64_t offset) {
  std::vector<uint8_t> primary;
  while (offset + kPrimaryHeaderSize <= fileSize_) {
    FrameHeader header;
    if (!readBytes(offset, kPrimaryHeaderSize, primary) ||
        !decodePrimaryHeader(primary.data(), primary.size(), header)) {
      std::cerr << "graw::GrawFile: invalid frame header at offset " << offset << " in file "
                << path_ << ". Remaining bytes ignored." << std::endl;
      break;
    }
    if (offset + header.frameSize > fileSize_) break;  // incomplete frame (still being written?)
    offsets_.push_back(offset);
    offset += header.frameSize;
  }
  scanEnd_ = offset;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void GrawFile::switchToScan() {
  std::cerr << "graw::GrawFile: frames of different sizes in file " << path_
            << ". Building the frame index." << std::endl;
  uniform_ = false;
  nUniformFrames_ = 0;
  offsets_.clear();
  scanFrom(0);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool GrawFile::frameOffset(std::size_t i, uint64_t &offset) {
  if (i >= nFrames()) return false;
  offset = uniform_ ? uint64_t(i) * firstHeader_.frameSize : offsets_[i];
  return true;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool GrawFile::readBytes(uint64_t offset, std::size_t size, std::vector<uint8_t> &buffer) {
  buffer.resize(size);
  in_.clear();
  in_.seekg(std::streamoff(offset), std::ios::beg);
  in_.read(reinterpret_cast<char *>(buffer.data()), std::streamsize(size));
  const std::size_t nRead = std::size_t(in_.gcount());
  buffer.resize(nRead);
  in_.clear();
  return nRead == size;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool GrawFile::readFrameHeaderAt(uint64_t offset, FrameHeader &header) {
  // 128 B cover the CoBo header of all revisions
  const std::size_t size = std::size_t(std::min<uint64_t>(128, fileSize_ - offset));
  if (!readBytes(offset, size, buffer_)) return false;
  return decodeHeader(buffer_.data(), buffer_.size(), header);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool GrawFile::readHeader(std::size_t i, FrameHeader &header) {
  uint64_t offset = 0;
  if (!frameOffset(i, offset)) return false;
  if (!readFrameHeaderAt(offset, header)) return false;
  if (uniform_ && (header.frameSize != firstHeader_.frameSize ||
                   header.frameType != firstHeader_.frameType ||
                   header.littleEndian != firstHeader_.littleEndian)) {
    switchToScan();
    return readHeader(i, header);
  }
  return true;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
bool GrawFile::readFrame(std::size_t i, Frame &frame) {
  if (!readHeader(i, frame.header)) return false;
  uint64_t offset = 0;
  frameOffset(i, offset);
  if (!readBytes(offset, frame.header.frameSize, buffer_)) return false;
  return decodeData(buffer_.data(), buffer_.size(), frame);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
} // namespace graw
