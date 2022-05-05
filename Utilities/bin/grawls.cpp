#include "InputFileHelper.h"
#include <boost/program_options.hpp>
#include <iostream>
#include <string>
#include <vector>

boost::program_options::variables_map parseCmdLineArgs(int argc, char **argv) {

  boost::program_options::options_description cmdLineOptDesc("Allowed options");
  cmdLineOptDesc.add_options()("help", "produce help message")(
      "input,i", boost::program_options::value<std::string>()->required(),
      "string - reference file")(
      "sep", boost::program_options::value<std::string>()->default_value("\n"),
      "string - separator")("ms",
                            boost::program_options::value<int>()->required(),
                            "int - delay in ms");

  boost::program_options::variables_map varMap;
  try {
    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, cmdLineOptDesc),
        varMap);
    if (varMap.count("help")) {
      std::cout << "grawls"
                << "\nList graw files by run timestamp\n";
      std::cout << cmdLineOptDesc << std::endl;
      exit(0);
    }
    boost::program_options::notify(varMap);
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
  auto delay = varMap["ms"].as<int>();
  auto separator = varMap["sep"].as<std::string>();
  try {
    std::cout << InputFileHelper::discoverFilesCSV(
                     input, std::chrono::milliseconds(delay), separator)
              << std::endl;
  } catch (const std::exception &e) {
    std::cerr << "grawls: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}