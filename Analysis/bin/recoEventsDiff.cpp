#include "TPCReco/DiffAnalysis.h"
#include <boost/program_options.hpp>
#include <iostream>
#include <string>

boost::optional<boost::program_options::variables_map>
parseCmdLineArgs(int argc, char **argv) {

  boost::program_options::options_description cmdLineOptDesc("Basic usage");
  cmdLineOptDesc.add_options()("help", "produce help message")(
      "input", boost::program_options::value<std::string>()->required(),
      "input root file")(
      "reference", boost::program_options::value<std::string>()->required(),
      "reference root file");

  boost::program_options::options_description extraCmdLineOptDesc(
      "Extra OPTIONS");
  extraCmdLineOptDesc.add_options()("no-type", "skip comparing event type")(
      "no-segments", "skip comparing number of segments")(
      "no-presence", "skip printing extra and missing events")(
      "no-info", "skip printing file and tree info on every line");
  cmdLineOptDesc.add(extraCmdLineOptDesc);

  boost::program_options::positional_options_description cmdLinePosDesc;
  cmdLinePosDesc.add("input", 1).add("reference", 1);
  boost::program_options::variables_map varMap;

  try {
    boost::program_options::store(
        boost::program_options::command_line_parser(argc, argv)
            .options(cmdLineOptDesc)
            .positional(cmdLinePosDesc)
            .run(),
        varMap);
    if (varMap.count("help")) {
      std::cout << "recoEventsDiff [--help] [OPTIONS] <input> <reference> "
                << "\nCompare reco TTrees by entry index\n\n"
                << cmdLineOptDesc << '\n';
      return boost::none;
    }
    boost::program_options::notify(varMap);
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n' << cmdLineOptDesc << '\n';
    return boost::none;
  }
  return varMap;
}

int main(int argc, char **argv) {
  auto varMap = parseCmdLineArgs(argc, argv);
  if (!varMap) {
    return 1;
  };
  auto inputName = (*varMap)["input"].as<std::string>();
  auto referenceName = (*varMap)["reference"].as<std::string>();
  auto analysis = tpcreco::analysis::diff::Analysis(inputName, referenceName);

  if (!varMap->count("no-segments")) {
    analysis.getDetailSink()->addCheck(
        tpcreco::analysis::diff::checks::checkSegments);
  }
  if (!varMap->count("no-type")) {
    analysis.getDetailSink()->addCheck(
        tpcreco::analysis::diff::checks::checkType);
  }
  if (varMap->count("no-info")) {
    analysis.getDetailSink()->resetTreeInfo();
    analysis.getExtraSink()->resetTreeInfo();
  }
  if (varMap->count("no-presence")) {
    analysis.getExtraSink()->disable(true);
  }

  analysis.run();

  return 0;
}