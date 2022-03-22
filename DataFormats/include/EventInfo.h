#ifndef _EventInfo_H_
#define _EventInfo_H_

#include <iostream>
#include <memory>
#include <bitset>

namespace eventraw {

  class EventInfo {
  public:
    EventInfo(){};

    ~EventInfo(){};

    time_t getRunId() const {return runId;}

    uint32_t getEventId() const {return eventId;}

    uint64_t getEventTimestamp() const {return timestamp;}

    std::bitset<64> getEventType() const {return std::bitset<64>(eventType);}

    void SetRunId(time_t aRunId) {runId = aRunId;}

    void SetEventId(uint32_t aEventId) {eventId = aEventId;}

    void SetEventTimestamp(uint64_t aTimestamp) {timestamp = aTimestamp;}

    void SetEventType(unsigned long aEventType) {eventType = aEventType;}

    inline EventInfo(std::shared_ptr<EventInfo> &einfo) {
      runId = einfo->getRunId();
      eventId = einfo->getEventId();
      timestamp = einfo->getEventTimestamp();
      eventType = einfo->getEventType().to_ulong();
    }

  private:

    time_t runId{0};
    uint32_t eventId{0};   // 4-bytes
    uint64_t timestamp{0}; // 6-bytes in 10ns CLK units (100 MHz)
    unsigned long eventType{0};
    
    static const uint eventTypeBits = 64;

  };

  
  std::ostream& operator<<(std::ostream& os, const eventraw::EventInfo& einfo);

}
#endif

