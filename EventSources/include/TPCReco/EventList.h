#ifndef TPCRECO_EVENTSOURCES_EVENT_LIST_H_
#define TPCRECO_EVENTSOURCES_EVENT_LIST_H_

#include <cstddef>
#include <string>
#include <vector>

/// One event of an event list (input.eventList), used by tpcReco and tpcWebGUI.
struct ListedEvent {
  long runId{-1};          // run id YYYYMMDDhhmmss (eventraw::EventInfo::GetRunId()), -1: any run
  long entry{-1};          // entry (frame) of the event in its GRAW chunk, -1: unknown
  unsigned long eventId{0};
  bool hasRun() const { return runId>=0; }
  bool hasEntry() const { return entry>=0; }
};

/// Reads an event list file. One event per line, fields separated by commas and/or spaces:
///   runId , entry , eventId   (e.g. the processing logs: run id YYYYMMDDhhmmss, entry in the chunk, event id)
///   runId , eventId
///   eventId                   (any run)
/// Empty lines and lines starting with '#' are skipped; the order of the file is kept.
/// Throws std::runtime_error when the file cannot be opened or a line cannot be parsed.
std::vector<ListedEvent> readEventList(const std::string & fileName);

/// Number of frames in a GRAW file (all chunks of a run but the last have the same number).
/// Returns 0 when the file cannot be read.
std::size_t grawFramesPerChunk(const std::string & grawFile);

/// GRAW chunk of a listed event with a known entry: (eventId - entry)/framesPerChunk,
/// -1 when it cannot be computed (no entry, framesPerChunk 0, eventId < entry).
int listedEventChunk(const ListedEvent & aEvent, std::size_t framesPerChunk);

#endif // TPCRECO_EVENTSOURCES_EVENT_LIST_H_
