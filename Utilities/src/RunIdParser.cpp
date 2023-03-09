#include "TPCReco/RunIdParser.h"
#include <algorithm>
#include <boost/date_time/time_facet.hpp>
#include <ctime>
#include <iomanip>
#include <locale>
#include <sstream>

const std::string RunId::facetFormat = "%Y%m%d%H%M%S";

RunId::time_point RunId::toTimePoint() const {
  if(isRegular()){
     return toTimePoint_();
  }
  return time_point{};
}

RunId::time_point RunId::toTimePoint_() const {
  std::stringstream stream;
  stream << repr;
  stream.exceptions(std::stringstream::failbit | std::stringstream::badbit);
  stream.imbue(
      std::locale(stream.getloc(),
                  new boost::posix_time::time_input_facet(RunId::facetFormat)));
  boost::posix_time::ptime ptime;
  stream >> ptime;
  return time_point::clock::from_ptime(ptime);
}

const std::array<std::pair<std::regex, RunIdParser::Positions>, 2>
    RunIdParser::regexes = {
        std::make_pair(
            std::regex("^.*CoBo(\\d)_AsAd(\\d)_(\\d{4})-(\\d{2})-(\\d{2})T(\\d{"
                       "2})\\D(\\d{2})\\D(\\d{2})\\.(\\d{3})_(\\d+).*$"),
            Positions{3, 4, 5, 6, 7, 8, 9, 10, 1, 2}),
        std::make_pair(std::regex("^.*(\\d{4})-(\\d{2})-(\\d{2})T(\\d{2})\\D("
                                  "\\d{2})\\D(\\d{2})\\.(\\d{3})_(\\d+).*$"),
                       Positions{1, 2, 3, 4, 5, 6, 7, 8, 0, 0})

};

RunIdParser::RunIdParser(const std::string &name) {
  for (auto &element : regexes) {
    auto &regex = element.first;
    auto &positions = element.second;
    std::smatch match;
    if (std::regex_match(name, match, regex)) {
      if (positions.max() >= match.size()) {
        continue;
      }
      matchResults(match, positions);
      return;
    }
  }
  throw ParseError("RunIdParser: Couldn't parse file name: " + name);
}

void RunIdParser::matchResults(const std::smatch &match,
                               const Positions &positions) {

  auto get = [match](size_t pos) { return pos ? std::stoi(match[pos]) : 0; };
  CoBoId_ = positions.cobo ? std::stoi(match[positions.cobo]) : -1;
  AsAdId_ = positions.asad ? std::stoi(match[positions.asad]) : -1;
  auto date = boost::gregorian::date(get(positions.year), get(positions.month),
                                     get(positions.day));
  auto duration = boost::posix_time::hours(get(positions.hour)) +
                  boost::posix_time::minutes(get(positions.minutes)) +
                  boost::posix_time::seconds(get(positions.seconds));
  auto ptime = boost::posix_time::ptime(date, duration);
  auto timePoint = time_point::clock::from_ptime(ptime);
  exactTimePoint_ =
      timePoint + std::chrono::milliseconds(get(positions.miliseconds));
  std::stringstream stream;
  stream.imbue(std::locale(stream.getloc(), new boost::posix_time::time_facet(
                                                RunId::facetFormat.c_str())));
  stream << ptime;
  stream >> rundId_;
  fileId_ = std::stoul(match[positions.fileId]);
}

RunIdParser::Positions::Positions(size_t year, size_t month, size_t day,
                                  size_t hour, size_t minutes, size_t seconds,
                                  size_t miliseconds, size_t fileId,
                                  size_t cobo, size_t asad)
    : year(year), month(month), day(day), hour(hour), minutes(minutes),
      seconds(seconds), miliseconds(miliseconds), fileId(fileId), cobo(cobo),
      asad(asad) {
  std::array<size_t, 7> elements{year,    month,   day,   hour,
                                 minutes, seconds, fileId};
  auto it = std::find_if(elements.begin(), elements.end(),
                         [](size_t i) { return i > 0; });
  if (it == elements.end()) {
    throw std::logic_error("RunIdParser: No non-zero position.");
  }
  max_ = *std::max_element(elements.begin(), elements.end());
}
