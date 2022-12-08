#ifndef TPCRECO_UTILITIES_UTILS_EVENT_INFO
#define TPCRECO_UTILITIES_UTILS_EVENT_INFO


#include "CoBoClock.h"
#include "EventInfo.h"

namespace tpcreco {
cobo_time_unit eventRelativeTime(const eventraw::EventInfo &info) noexcept;
cobo_time_point eventAbsoluteTime(const eventraw::EventInfo &info);

} // namespace tpcreco
#endif //TPCRECO_UTILITIES_UTILS_EVENT_INFO
