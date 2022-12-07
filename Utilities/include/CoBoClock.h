#ifndef TPCRECO_UTILITIES_COBO_CLOCK_H_
#define TPCRECO_UTILITIES_COBO_CLOCK_H_

#include <boost/date_time/posix_time/posix_time.hpp>
#include <chrono>
namespace tpcreco {
using cobo_time_tick = std::ratio<1L, 100000000L>;
using cobo_time_unit = std::chrono::duration<long, cobo_time_tick>;

// Clock with time precision used by GET electronics
// Clock epoch is unix epoch: 1970 Jan 1, 00:00:000
class CoBoClock {
public:
  using duration = cobo_time_unit;
  using rep = duration::rep;
  using period = duration::period;
  using time_point = std::chrono::time_point<CoBoClock, duration>;

  static constexpr bool is_steady = false;

  static time_point now() noexcept {
    return time_point(std::chrono::microseconds(
        (boost::posix_time::microsec_clock::local_time() - epoch)
            .total_microseconds()));
  }

  // Boost date_time API
  // allows for portable calendar dates without thread-unsafe dependency on
  // local timezone (looking at you C-like API)
  static time_point from_ptime(const boost::posix_time::ptime &t) {
    return std::chrono::time_point_cast<duration>(
        time_point(std::chrono::seconds((t - epoch).total_seconds())));
  }
  
private:
  static boost::posix_time::ptime epoch;
};

using cobo_time_point = CoBoClock::time_point;

} // namespace tpcreco
#endif //TPCRECO_UTILITIES_COBO_CLOCK_H_