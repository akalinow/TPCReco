#include "InputFileHelper.h"
#include <boost/program_options.hpp>
#include <iostream>
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

boost::program_options::variables_map parseCmdLineArgs(int argc, char **argv) {
  boost::program_options::options_description cmdLineOptDesc("Allowed options");
  cmdLineOptDesc.add_options()("help", "produce help message")(
      "input,i", boost::program_options::value<std::string>()->required(),
      "string - reference file or runId")(
      "chunk", boost::program_options::value<size_t>(), "uint - file chunk")(
      "separator",
      boost::program_options::value<std::string>()->default_value("\n"),
      "string - separator")(
      "directory,d", boost::program_options::value<std::string>(),
      "string - directory to browse. Mutually exclusive with \"files\"")(
      "ms", boost::program_options::value<int>()->required(),
      "int - delay in ms")(
      "files,f",
      boost::program_options::value<std::vector<std::string>>()->multitoken(),
      "strings - list of files to browse. Mutually "
      "exclusive with \"directory\"");

  boost::program_options::positional_options_description cmdLinePosDesc;
  cmdLinePosDesc.add("input", 1).add("files", -1);

  boost::program_options::variables_map varMap;

  try {
    boost::program_options::store(
        boost::program_options::command_line_parser(argc, argv)
            .options(cmdLineOptDesc)
            .positional(cmdLinePosDesc)
            .run(),
        varMap);
    if (varMap.count("help")) {
      std::cout << "grawls"
                << "\nList graw files by run timestamp\n";
      std::cout << cmdLineOptDesc << std::endl;
      exit(0);
    }

    boost::program_options::notify(varMap);
    conflicting_options(varMap, "files", "directory");

  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    std::cout << cmdLineOptDesc << std::endl;
    exit(1);
  }

  return varMap;
}

int main(int argc, char **argv) {
  auto varMap = parseCmdLineArgs(argc, argv);
  auto input = varMap["input"].as<std::string>();
  auto delay = std::chrono::milliseconds(varMap["ms"].as<int>());
  auto separator = varMap["separator"].as<std::string>();
  std::vector<std::string> output;
  try {
    auto referencePoint =
        referencePointHelper(input, varMap["chunk"], varMap["directory"]);
    if (varMap.count("files")) {
      auto sequence = varMap["files"].as<std::vector<std::string>>();
      InputFileHelper::discoverFiles(
          referencePoint.first, referencePoint.second, delay, sequence.begin(),
          sequence.end(), std::back_inserter(output));
    } else {
      boost::filesystem::path dir;
      if (varMap.count("directory")) {
        dir = boost::filesystem::path(varMap["directory"].as<std::string>());
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
    std::cout << InputFileHelper::join(output, separator) << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "grawls: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}