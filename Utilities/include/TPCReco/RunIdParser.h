#ifndef RUN_ID_PARSER_H_
#define RUN_ID_PARSER_H_
#include "TPCReco/CoBoClock.h"
#include <array>
#include <chrono>
#include <exception>
#include <regex>
#include <utility>

class RunId {
public:
  RunId(long runId) noexcept { repr = runId; }
  inline operator long() const noexcept { return repr; }
  using time_point = tpcreco::cobo_time_point;

  bool inline isRegular() const noexcept { return repr >= 1000000; }
  time_point toTimePoint() const;

  static const std::string facetFormat;
private:
  time_point toTimePoint_() const;
  long repr = 0;
};

class RunIdParser {
public:
  RunIdParser(const std::string &name);
  class ParseError : public std::logic_error {
  public:
    ParseError(const std::string &message) : std::logic_error(message) {}
  };

  using time_point = tpcreco::cobo_time_point;

  inline RunId runId() const noexcept { return RunId(rundId_); }
  inline unsigned long fileId() const noexcept { return fileId_; }
  // AsAd id
  // returns -1 if no information
  inline int AsAdId() const noexcept { return AsAdId_; };
  // CoBoid
  // returns -1 if no information
  inline int CoBoId() const noexcept { return CoBoId_; };
  // time point corressponding to parsed time (runId + miliseconds)
  inline time_point exactTimePoint() const noexcept { return exactTimePoint_; }

  template <class Rep, class Period>
  bool isClose(const time_point &other,
               std::chrono::duration<Rep, Period> delay) const {
    return (exactTimePoint() > other ? exactTimePoint() - other
                                     : other - exactTimePoint()) <= delay;
  }

  template <class Rep, class Period>
  bool isClose(const RunIdParser &other,
               std::chrono::duration<Rep, Period> delay) const {
    return isClose(other.exactTimePoint(), delay);
  }
  
  [[deprecated("Use RunId(id).toTimePoint() instead")]]
  static time_point timePointFromRunId(RunId id) {
    return id.toTimePoint();
  }

private:

  long rundId_;
  unsigned long fileId_;
  int AsAdId_ = -1;
  int CoBoId_ = -1;
  time_point exactTimePoint_;

  class Positions {
  public:
    Positions(size_t year, size_t month, size_t day, size_t hour,
              size_t minutes, size_t seconds, size_t miliseconds, size_t fileId,
              size_t cobo, size_t asad);
    const size_t year;
    const size_t month;
    const size_t day;
    const size_t hour;
    const size_t minutes;
    const size_t seconds;
    const size_t miliseconds;
    const size_t fileId;
    const size_t cobo;
    const size_t asad;
    inline size_t max() const noexcept { return max_; }

  private:
    size_t max_;
  };

  static const std::array<std::pair<std::regex, Positions>, 2> regexes;
  void matchResults(const std::smatch &match, const Positions &positions);
};

#endif // RUN_ID_PARSER_H_