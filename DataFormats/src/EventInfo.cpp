#include <bitset>

#include "TPCReco/EventInfo.h"
#include "TPCReco/RunIdParser.h"
////////////////////////////////////////////////
////////////////////////////////////////////////
void eventraw::EventInfo::reset(){

  runId = 0;
  eventId = 0;   // 4-bytes
  timestamp = 0; // 6-bytes in 10ns CLK units (100 MHz)
  eventType = 0;
  pedestalSubtracted = false;

}
////////////////////////////////////////////////
////////////////////////////////////////////////
std::ostream& eventraw::operator<<(std::ostream& os, const eventraw::EventInfo& einfo) {
  os << "EventInfo: "
     <<" event_id=" << einfo.GetEventId()
     <<" run_id=" << einfo.GetRunId()
     <<" timestamp=" << einfo.GetEventTimestamp()
     <<" type_bits=" << einfo.GetEventType().to_string()
     <<" pedestal_subtracted=" << einfo.GetPedestalSubtracted();
  return os;
}
namespace tpcreco {
cobo_time_unit eventRelativeTime(const eventraw::EventInfo &info) noexcept {
  return cobo_time_unit(info.GetEventTimestamp());
}

cobo_time_point eventAbsoluteTime(const eventraw::EventInfo &info) {
  return RunId{info.GetRunId()}.toTimePoint() + eventRelativeTime(info);
}
} // namespace tpcreco
////////////////////////////////////////////////
////////////////////////////////////////////////
