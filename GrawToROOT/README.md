# GrawToROOT

Reading of GRAW files written by the GET electronics (CoBo boards).
The module is self-contained: it needs neither the GET software libraries
nor the XCFG frame format files.

## Classes

* `graw::GrawFile` ([GrawFile.h](include/TPCReco/GrawFile.h)) – opens a GRAW file
  and gives random access to its frames: `nFrames()`, `readHeader(i, header)`,
  `readFrame(i, frame)`. `update()` picks up frames appended to a file that
  is still being written (online mode).
* `graw::Frame` ([GrawFrame.h](include/TPCReco/GrawFrame.h)) – decoded frame:
  header (`graw::FrameHeader`) and samples of one ASAD board,
  `sample(aget, channel, cell)`, `forEachSample(aget, channel, f)`.
* `PedestalCalculatorGRAW` – pedestal and FPN subtraction of a `graw::Frame`.

The event sources `EventSourceGRAW` (all ASAD boards in one file) and
`EventSourceMultiGRAW` (one file per ASAD board) in the EventSources module
are built on top of these classes.

## GRAW file format

A GRAW file is a sequence of MFM frames, one frame per ASAD board per event.
Multi-byte fields are big-endian (unless bit 7 of the meta type byte is set).

| Offset [B] | Size [B] | Field |
|-----------:|---------:|-------|
| 0  | 1 | meta type: bit 7 little-endian, bit 6 blob, bits 0-3 log2(block size) |
| 1  | 3 | frame size [blocks] |
| 4  | 1 | data source |
| 5  | 2 | frame type: 1 partial readout, 2 full readout |
| 7  | 1 | revision |
| 8  | 2 | header size [blocks] |
| 10 | 2 | item size [B] |
| 12 | 4 | item count |
| 16 | 6 | event time |
| 22 | 4 | event index |
| 26 | 1 | CoBo index |
| 27 | 1 | ASAD index |
| 28 | 2 | read offset |
| 30 | 1 | status |
| 31 | 4 x 9 | AGET hit patterns |
| 67 | 4 x 2 | AGET multiplicities |
| 75 | 4 | window out (revision >= 3) |
| 79 | 4 x 2 | AGET last read cell (revision >= 4) |

Data items follow the header:

* full readout (frame type 2), 2 B items: AGET index in bits 14-15, ADC value in
  bits 0-11. Items of each AGET come in a fixed order: cell 0 channels 0-67,
  cell 1 channels 0-67, ... All frames have the same size
  (256 + 4 x 68 x 512 x 2 = 278784 B for revision 5), so frame offsets are computed.
* partial readout (frame type 1), 4 B items: AGET (bits 30-31), channel (23-29),
  cell (14-22), ADC value (0-11). Frames differ in size, so the reader builds an
  index of frame offsets by reading the frame headers once.

## Tests

* `GrawFile_tst` – decoding of the test data in `EventSources/test/testData`
  and of synthetic files (partial readout, frames of different sizes, trailing
  bytes, growing file).
* `GrawFileVsGET_tst` – built only if the GET software is found: compares
  frame headers and all samples with the GET reader.
* `GrawToROOT_tst` (FileOutput module) – conversion of the test data to ROOT.

```Shell
cd build
ctest -R Graw --output-on-failure
```
