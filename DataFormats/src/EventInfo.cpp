#include "EventInfo.h"

#include <bitset>


////////////////////////////////////////////////
////////////////////////////////////////////////
std::ostream& eventraw::operator<<(std::ostream& os, const eventraw::EventInfo& einfo) {
  os << "EventInfo: "
     <<" run timestamp: "<<einfo.getRunId()
     <<" id=" << einfo.getEventId() << ", timestamp=" << einfo.getEventTimestamp()
     <<" type: "<<einfo.getEventType().to_string();
  return os;
}
////////////////////////////////////////////////
////////////////////////////////////////////////
