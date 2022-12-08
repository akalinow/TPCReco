#include "UtilsEventInfo.h"
#include "RunIdParser.h"
namespace tpcreco {
cobo_time_unit eventRelativeTime(const eventraw::EventInfo &info) noexcept {
  return cobo_time_unit(info.GetEventTimestamp());
}

cobo_time_point eventAbsoluteTime(const eventraw::EventInfo &info) {
  return RunIdParser::timePointFromRunId(info.GetRunId()) +
         eventRelativeTime(info);
}
} // namespace tpcreco
