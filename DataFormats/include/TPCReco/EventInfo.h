#ifndef _EventInfo_H_
#define _EventInfo_H_

#include <iostream>
#include <memory>
#include <bitset>
#include <Rtypes.h>
#include "TPCReco/CoBoClock.h"

class EventTPC;

namespace eventraw {

  class EventInfo {
  public:
    EventInfo(){};

    ~EventInfo(){};

    void reset();

    long GetRunId() const {return runId;}

    uint32_t GetEventId() const {return eventId;}

    ULong_t GetEventTimestamp() const {return timestamp;}

    std::bitset<64> GetEventType() const {return std::bitset<64>(eventType);}

    bool GetPedestalSubtracted() const {return pedestalSubtracted;}

    void SetRunId(long aRunId) {runId = aRunId;}

    void SetEventId(uint32_t aEventId) {eventId = aEventId;}

    void SetEventTimestamp(ULong_t aTimestamp) {timestamp = aTimestamp;}

    void SetEventType(unsigned long aEventType) {eventType = aEventType;}

    void SetPedestalSubtracted(bool isSubtracted) {pedestalSubtracted = isSubtracted;}

    inline EventInfo(std::shared_ptr<EventInfo> &einfo) {
      runId = einfo->GetRunId();
      eventId = einfo->GetEventId();
      timestamp = einfo->GetEventTimestamp();
      eventType = einfo->GetEventType().to_ulong();
      pedestalSubtracted = einfo->GetPedestalSubtracted();
    }

  private:

    long runId{0};
    uint32_t eventId{0};   // 4-bytes
    ULong_t timestamp{0}; // 6-bytes in 10ns CLK units (100 MHz)
    unsigned long eventType{0};
    bool pedestalSubtracted{false};
    
    static const uint eventTypeBits = 64;

  };

  
  std::ostream& operator<<(std::ostream& os, const eventraw::EventInfo& einfo);

}

namespace tpcreco {
cobo_time_unit eventRelativeTime(const eventraw::EventInfo &info) noexcept;
cobo_time_point eventAbsoluteTime(const eventraw::EventInfo &info);
} // namespace tpcreco

#endif

