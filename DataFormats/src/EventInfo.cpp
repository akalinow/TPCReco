#include <bitset>

#include "EventInfo.h"
#include "EventTPC.h"

////////////////////////////////////////////////
////////////////////////////////////////////////
eventraw::EventInfo::EventInfo(std::shared_ptr<EventTPC> aEventTPC){

  set(aEventTPC);
  
}
////////////////////////////////////////////////
////////////////////////////////////////////////
void eventraw::EventInfo::set(std::shared_ptr<EventTPC> aEventTPC){

  SetEventId(aEventTPC->GetEventId());
  SetEventTimestamp(aEventTPC->GetEventTime());
  
}
////////////////////////////////////////////////
////////////////////////////////////////////////
void eventraw::EventInfo::reset(){

  runId = 0;
  eventId = 0;   // 4-bytes
  timestamp = 0; // 6-bytes in 10ns CLK units (100 MHz)
  eventType = 0;

}
////////////////////////////////////////////////
////////////////////////////////////////////////
std::ostream& eventraw::operator<<(std::ostream& os, const eventraw::EventInfo& einfo) {
  os << "EventInfo: "
     <<" run timestamp: "<<einfo.GetRunId()
     <<" event id=" << einfo.GetEventId() << ", timestamp=" << einfo.GetEventTimestamp()
     <<" type: "<<einfo.GetEventType().to_string();
  return os;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
