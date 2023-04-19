#include "InputFileHelper.h"
#include <boost/program_options.hpp>
#include <iostream>
#include <set>
#include <string>
#include <vector>

void conflicting_options(const boost::program_options::variables_map &vm,
                         const std::string &opt1, const std::string &opt2) {
  if (vm.count(opt1) && !vm[opt1].defaulted() && vm.count(opt2) &&
      !vm[opt2].defaulted()) {
    throw std::logic_error(std::string("Conflicting options '") + opt1 +
                           "' and '" + opt2 + "'.");
  }
}

std::pair<RunIdParser::time_point, size_t>
referencePointHelper(const std::string &input,
                     const boost::program_options::variable_value &chunk,
                     const boost::program_options::variable_value &directory) {
  RunIdParser::time_point point;
  size_t fileId;
  try {
    auto id = RunIdParser(input);
    point = id.timePoint();
    fileId = chunk.empty() ? id.fileId() : chunk.as<size_t>();
  } catch (const RunIdParser::ParseError &e) {
    try {
      point = RunIdParser::timePointFromRunId(input);
      if (chunk.empty()) {
        throw std::logic_error(
            "input in form of run id requires providing chunk");
      }
      if (directory.empty()) {
        throw std::logic_error(
            "input in form of run id requires providing directory");
      }
      fileId = chunk.as<size_t>();
    } catch (const RunIdParser::ParseError &e) {
      throw std::logic_error("Input is neither parseable filename nor run id");
    }
  }
  return std::make_pair(point, fileId);
}

int main(int argc, char **argv) {
  std::vector<std::string> requiredOptions = {"input","ms"};
  boost::property_tree::ptree tree = getConfig(argc,argv,requiredOptions);
  auto input = tree.get("input","");
  auto delay = std::chrono::milliseconds(tree.get("ms",""));
  auto separator = tree.get("separator","");

  std::set<std::string> extensionsSet;
  {
    auto extensions = tree.get("ext","");
    std::transform(std::begin(extensions), std::end(extensions),
                   std::inserter(extensionsSet, std::begin(extensionsSet)),
                   [](auto entry) {
                     return !entry.empty() && entry[0] != '.' ? "." + entry
                                                              : entry;
                   });
  }

  std::vector<std::string> output;
  try {
    auto referencePoint =
        referencePointHelper(input, tree.get("chunk",""), tree.get("directory",""));
        std::string files = tree.get("files","");
    if (files.size()>0) {
      auto sequence = varMap["files"].as<std::vector<std::string>>();
      InputFileHelper::discoverFiles(
          referencePoint.first, referencePoint.second, delay, sequence.begin(),
          sequence.end(), std::back_inserter(output));
    } else {
      boost::filesystem::path dir;
      std::string directory = tree.get("directory","");
      if (directory.size()>0) {
        dir = boost::filesystem::path(directory);
      } else {
        auto inputPath = boost::filesystem::path(input);
        dir = inputPath.has_parent_path() ? inputPath.parent_path()
                                          : boost::filesystem::current_path();
      }
      auto begin = boost::filesystem::directory_iterator(dir);
      auto end = boost::filesystem::directory_iterator();

      InputFileHelper::discoverFiles(referencePoint.first,
                                     referencePoint.second, delay, begin, end,
                                     std::back_inserter(output));
    }
    output.erase(InputFileHelper::filterExtensions(
                     std::begin(output), std::end(output), extensionsSet),
                 std::end(output));
    std::cout << InputFileHelper::join(output, separator) << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "grawls: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}