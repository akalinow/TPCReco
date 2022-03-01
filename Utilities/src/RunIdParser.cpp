#include "RunIdParser.h"
#include <algorithm>
#include <ctime>
#include <exception>
#include <iomanip>
#include <sstream>
const std::array<std::pair<std::regex, RunIdParser::Positions>, 2>
    RunIdParser::regexes = {
        std::make_pair(
            std::regex("^.*CoBo(\\d)_AsAd(\\d)_(\\d{4})-(\\d{2})-(\\d{2})T(\\d{"
                       "2}):(\\d{2}):(\\d{2})\\.(\\d{3})_(\\d{4}).+$"),
            Positions{3, 4, 5, 6, 7, 8, 9, 10, 1, 2}),
        std::make_pair(std::regex("^.*(\\d{4})-(\\d{2})-(\\d{2})T(\\d{2}):("
                                  "\\d{2}):(\\d{2})\\.(\\d{3})_(\\d{4}).*$"),
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
  throw std::logic_error("Couldn't parse " + name);
}

void RunIdParser::matchResults(const std::smatch &match,
                               const Positions &positions) {

  auto get = [match](size_t pos) { return pos ? std::stoi(match[pos]) : 0; };
  CoBoId_ = positions.cobo ? std::stoi(match[positions.cobo]) : -1;
  AsAdId_ = positions.asad ? std::stoi(match[positions.asad]) : -1;
  struct std::tm tm;
  tm.tm_year = get(positions.year) - 1900;
  tm.tm_mon = get(positions.month) - 1;
  tm.tm_mday = get(positions.day);
  tm.tm_hour = get(positions.hour);
  tm.tm_min = get(positions.minutes);
  tm.tm_sec = get(positions.seconds);
  auto tp = time_point::clock::from_time_t(std::mktime(&tm));
  timePoint_ = tp + std::chrono::milliseconds(get(positions.miliseconds));
  std::stringstream stream;
  stream << std::put_time(&tm, "%Y%m%d%H%M%S");
  stream >> rundId_;
  stream.str("");
  stream.clear();
  stream << match[positions.fileId];
  stream >> fileId_;
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
    throw std::logic_error("No non-zero position");
  }
  max_ = *std::max_element(elements.begin(), elements.end());
}