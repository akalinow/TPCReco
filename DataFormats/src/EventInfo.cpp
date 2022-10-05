#include <bitset>

#include "EventInfo.h"

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
     <<" event id=" << einfo.GetEventId()
     <<"timestamps"
     <<" run: "<<einfo.GetRunId()
     <<" event:" << einfo.GetEventTimestamp()
     <<" type: "<<einfo.GetEventType().to_string()
     <<" pedestal subtracted? "<<einfo.GetPedestalSubtracted();
  return os;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
