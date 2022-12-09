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

template <class Rep, class Period, class InputIterator, class OutputIterator>
void discoverFiles(RunIdParser::time_point timePoint, unsigned long fileId,
                   std::chrono::duration<Rep, Period> delay,
                   InputIterator begin, InputIterator end,
                   OutputIterator output) {
  std::for_each(begin, end, [&](const auto &entry) {
    auto path = boost::filesystem::path(entry);
    if (!boost::filesystem::is_regular_file(path)) {
      return;
    }
    try {
      auto id = RunIdParser(path.string());
      if (id.isClose(timePoint, delay) && id.AsAdId() >= 0 &&
          id.fileId() == fileId) {
        (*output) = path.string();
        ++output;
      }
    } catch (const std::logic_error &e) {
    }
  });
}

// convenience function
template <class OutputIterator, class Rep, class Period>
void discoverFiles(const std::string &inputFilename,
                   std::chrono::duration<Rep, Period> delay,
                   OutputIterator output) {
  auto inputId = RunIdParser(inputFilename);
  if (inputId.AsAdId() < 0) {
    return;
  }
  auto inputPath = boost::filesystem::path(inputFilename);
  auto parentPath = inputPath.has_parent_path()
                        ? inputPath.parent_path()
                        : boost::filesystem::current_path();
  auto begin = boost::filesystem::directory_iterator(parentPath);
  auto end = boost::filesystem::directory_iterator{};
  discoverFiles(inputId.exactTimePoint(), inputId.fileId(), delay, begin, end,
                output);
}

template <class Sequence>
std::string join(Sequence sequence, std::string separator) {
  std::sort(sequence.begin(), sequence.end(), [](auto lhs, auto rhs) {
    auto lParser = RunIdParser(lhs);
    auto rParser = RunIdParser(rhs);
    return std::make_tuple(lParser.fileId(), lParser.CoBoId(),
                           lParser.AsAdId()) <
           std::make_tuple(rParser.fileId(), rParser.CoBoId(),
                           rParser.AsAdId());
  });
  return boost::algorithm::join(sequence, separator);
}

// convenience function
template <class Rep, class Period>
std::string discoverFilesCSV(const std::string &input,
                             std::chrono::duration<Rep, Period> delay,
                             std::string separator = ",") {
  std::vector<std::string> files;
  discoverFiles(input, delay, std::back_inserter(files));
  return join(files, separator);
}

template <class FilesIterator, class ExtensionsContainer>
FilesIterator filterExtensions(FilesIterator first, FilesIterator last,
                               const ExtensionsContainer &extensions) {
  return std::remove_if(first, last, [&extensions](const auto &entry) {
    auto extension = boost::filesystem::path(entry).extension().string();
    return extensions.count(extension) == 0;
  });
}
} // namespace InputFileHelper

#endif // UTILITIES_INPUT_FILE_HELPER_H_