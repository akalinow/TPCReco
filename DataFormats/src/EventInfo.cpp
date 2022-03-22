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
std::ostream& eventraw::operator<<(std::ostream& os, const eventraw::EventInfo& einfo) {
  os << "EventInfo: "
     <<" run timestamp: "<<einfo.GetRunId()
     <<" id=" << einfo.GetEventId() << ", timestamp=" << einfo.GetEventTimestamp()
     <<" type: "<<einfo.GetEventType().to_string();
  return os;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
