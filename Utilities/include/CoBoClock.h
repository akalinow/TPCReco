#ifndef TPCRECO_UTILITIES_COBO_CLOCK_H_
#define TPCRECO_UTILITIES_COBO_CLOCK_H_

#include <chrono>

namespace tpcreco {
using cobo_time_tick = std::ratio<1L, 100000000L>;
using cobo_time_unit = std::chrono::duration<long, cobo_time_tick>;

class CoBoClock {
  public:
  using duration = cobo_time_unit;
  using rep = duration::rep;
  using period = duration::period;
  using time_point = std::chrono::time_point<CoBoClock, duration>;

  static constexpr bool is_steady = false;

  static time_point now() noexcept {
    return time_point(std::chrono::duration_cast<duration>(
        std::chrono::system_clock::now().time_since_epoch()));
  }

  // Map to C API
  static std::time_t to_time_t(const time_point &t) noexcept {
    return std::time_t(
        std::chrono::duration_cast<duration>(t.time_since_epoch()).count());
  }

  static time_point from_time_t(std::time_t t) noexcept {
    return std::chrono::time_point_cast<duration>(
        time_point(std::chrono::seconds(t)));
  }
};

using cobo_time_point = CoBoClock::time_point;

} // namespace tpcreco
#endif //TPCRECO_UTILITIES_COBO_CLOCK_H_