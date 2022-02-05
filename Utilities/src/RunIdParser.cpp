#include "RunIdParser.h"
#include <algorithm>
#include <exception>
#include <sstream>
const std::array<std::pair<std::regex, RunIdParser::Positions>, 1>
    RunIdParser::regexes = {
        std::make_pair(std::regex(".+(\\d{4})-(\\d{2})-(\\d{2})T(\\d{2}):("
                                  "\\d{2}):(\\d{2})\\.\\d{3}_(\\d{4}).+"),
                       Positions{1, 2, 3, 4, 5, 6, 7})};

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
  std::stringstream stream;
  auto fillStream = [&stream, &match](size_t position) mutable {
    if (position) {
      stream << match[position];
    } else {
      stream << 0;
    }
  };
  fillStream(positions.year);
  fillStream(positions.month);
  fillStream(positions.day);
  fillStream(positions.hour);
  fillStream(positions.minutes);
  fillStream(positions.seconds);
  stream >> rundId_;
  stream.str("");
  stream.clear();
  fillStream(positions.fileId);
  stream >> fileId_;
}

RunIdParser::Positions::Positions(size_t year, size_t month, size_t day,
                                  size_t hour, size_t minutes, size_t seconds,
                                  size_t fileId)
    : year(year), month(month), day(day), hour(hour), minutes(minutes),
      seconds(seconds), fileId(fileId) {
  std::array<size_t, 7> elements{year,    month,   day,   hour,
                                 minutes, seconds, fileId};
  auto it = std::find_if(elements.begin(), elements.end(),
                         [](size_t i) { return i > 0; });
  if (it == elements.end()) {
    throw std::logic_error("No non-zero position");
  }
  max_ = *std::max_element(elements.begin(), elements.end());
}