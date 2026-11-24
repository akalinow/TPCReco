#ifndef TPCRECO_EVENTSOURCES_GRAW_CHUNK_PATH_H_
#define TPCRECO_EVENTSOURCES_GRAW_CHUNK_PATH_H_

#include <string>

/// Path of another chunk of the same GRAW stream (same CoBo/AsAd, same run).
///
/// GRAW files are named <prefix>_<YYYY-MM-DDTHH:MM:SS>.<ms>_<chunk>.graw, e.g.
/// CoBo0_AsAd0_2022-04-12T08:03:44.531_0000.graw. The timestamp of a chunk is NOT the same
/// in all chunks of a run: the next chunk of the file above is
/// CoBo0_AsAd0_2022-04-12T08:03:44.540_0001.graw, and the files of one run can differ by ~1 s.
/// The chunk is therefore searched in the directory of filePath: same prefix, chunk number
/// shifted by chunkOffset, timestamp closest to the one of filePath (at most 2 s apart).
/// ':' or '-' are accepted as time separators (files copied to file systems without ':').
///
/// Returns an empty string when no such file exists or the name does not follow the convention.
std::string findGrawChunkPath(const std::string & filePath, int chunkOffset);

#endif // TPCRECO_EVENTSOURCES_GRAW_CHUNK_PATH_H_
