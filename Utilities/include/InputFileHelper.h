#ifndef UTILITIES_INPUT_FILE_HELPER_H_
#define UTILITIES_INPUT_FILE_HELPER_H_
#include "RunIdParser.h"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/tokenizer.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace InputFileHelper {
std::vector<std::string> tokenize(const std::string &text) {
  boost::char_separator<char> sep(",");
  boost::tokenizer<boost::char_separator<char>> tokenizer(text, sep);
  return std::vector<std::string>(tokenizer.begin(), tokenizer.end());
}

template <class It> std::string getExtension(It begin, It end) {
  if (begin == end) {
    throw std::logic_error("No input files.");
  }
  auto extension = boost::filesystem::path(*begin).extension();
  auto hasCommonExtension =
      std::all_of(begin, end, [&extension](const auto &i) {
        return boost::filesystem::path(i).extension() == extension;
      });
  if (hasCommonExtension) {
    return extension.string();
  }
  throw std::runtime_error("No common extension in input files.");
}

template <class It, class Rep, class Period>
void discoverFiles(const std::string &input,
                   std::chrono::duration<Rep, Period> delay, It iterator) {
  auto inputId = RunIdParser(input);
  if (inputId.AsAdId() < 0) {
    return;
  }
  auto inputPath = boost::filesystem::path(input);
  auto directory =
      boost::filesystem::directory_iterator(inputPath.parent_path());

  std::vector<std::string> inputFiles;
  for (const auto &entry : directory) {
    auto path = entry.path();
    if (!boost::filesystem::is_regular_file(path) ||
        path.extension() != ".graw") {
      continue;
    }
    try {
      auto id = RunIdParser(path.string());
      if (inputId.isClose(id, delay) && id.AsAdId() >= 0 &&
          id.fileId() == inputId.fileId()) {
        (*iterator) = path.string();
        ++iterator;
      }
    } catch (const std::logic_error &e) {
    }
  }
}

template <class Rep, class Period>
std::string discoverFilesCSV(const std::string &input,
                             std::chrono::duration<Rep, Period> delay,
                             std::string separator = ",") {
  std::vector<std::string> files;
  discoverFiles(input, delay, std::back_inserter(files));
  std::sort(files.begin(), files.end(), [](auto lhs, auto rhs) {
    auto lParser = RunIdParser(lhs);
    auto rParser = RunIdParser(rhs);
    return std::make_tuple(lParser.CoBoId(), lParser.AsAdId()) <
           std::make_tuple(rParser.CoBoId(), rParser.AsAdId());
  });
  return boost::algorithm::join(files, separator);
}

} // namespace InputFileHelper

#endif // UTILITIES_INPUT_FILE_HELPER_H_