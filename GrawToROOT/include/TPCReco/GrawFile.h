#ifndef TPCRECO_GRAWFILE_H
#define TPCRECO_GRAWFILE_H

// Reader for GRAW files (MFM frames written by CoBo boards).
// Self-contained replacement of the GET software readers
// (TGrawFile, GET::getGrawFrame, mfm::Frame): needs neither the GET
// libraries nor the XCFG frame format files.
//
// File layout: a sequence of MFM frames. Each frame starts with an 8 B
// primary header (meta type, frame size, data source, frame type, revision),
// followed by the rest of the CoBo header and the data items.
// All multi-byte fields are big-endian unless the meta type says otherwise.
//
// Frame access is random: frame offsets are either computed (all frames of
// the same size, which is always the case for full readout data) or taken
// from an index built by walking the primary headers once.

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

#include "TPCReco/GrawFrame.h"

namespace graw {

class GrawFile {
public:
  GrawFile() = default;
  explicit GrawFile(const std::string &path) { open(path); }

  GrawFile(const GrawFile &) = delete;
  GrawFile &operator=(const GrawFile &) = delete;

  // Open a file and prepare the frame index. Returns false on error.
  bool open(const std::string &path);
  void close();
  bool isOpen() const { return in_.is_open(); }
  const std::string &path() const { return path_; }

  // Number of complete frames in the file.
  std::size_t nFrames() const { return uniform_ ? nUniformFrames_ : offsets_.size(); }

  // Re-check the file size and add frames written since the last call
  // (online mode). Returns the number of frames.
  std::size_t update();

  // Read and decode the header of frame i (counted from 0).
  bool readHeader(std::size_t i, FrameHeader &header);

  // Read and decode frame i: header and samples.
  bool readFrame(std::size_t i, Frame &frame);

  // Decoders, exposed for testing.
  static bool decodePrimaryHeader(const uint8_t *buffer, std::size_t size, FrameHeader &header);
  static bool decodeHeader(const uint8_t *buffer, std::size_t size, FrameHeader &header);
  static bool decodeData(const uint8_t *buffer, std::size_t size, Frame &frame);

  static constexpr std::size_t kPrimaryHeaderSize = 8;   // [B]
  static constexpr std::size_t kMinCoBoHeaderSize = 75;  // [B], revisions 0-2

private:
  bool frameOffset(std::size_t i, uint64_t &offset);
  bool readBytes(uint64_t offset, std::size_t size, std::vector<uint8_t> &buffer);
  bool readFrameHeaderAt(uint64_t offset, FrameHeader &header);
  void scanFrom(uint64_t offset);
  void switchToScan();
  uint64_t currentFileSize();

  std::ifstream in_;
  std::string path_;
  uint64_t fileSize_{0};

  // all frames of the same size: offset = i*frameSize
  bool uniform_{false};
  FrameHeader firstHeader_;
  std::size_t nUniformFrames_{0};

  // general case: offsets of frames found by walking the primary headers
  std::vector<uint64_t> offsets_;
  uint64_t scanEnd_{0};

  std::vector<uint8_t> buffer_;
};

} // namespace graw
#endif
