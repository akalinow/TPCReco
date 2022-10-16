#include "TTreeOps.h"
#include <TFile.h>
#include <TTree.h>
#include <boost/program_options.hpp>
#include <iostream>
#include <string>

void conflicting_options(const boost::program_options::variables_map &vm,
                         const std::string &opt1, const std::string &opt2) {
  if (vm.count(opt1) && !vm[opt1].defaulted() && vm.count(opt2) &&
      !vm[opt2].defaulted()) {
    throw std::logic_error(std::string("Conflicting options '--") + opt1 +
                           "' and '--" + opt2 + "'");
  }
}

boost::optional<boost::program_options::variables_map>
parseCmdLineArgs(int argc, char **argv) {

  boost::program_options::options_description cmdLineOptDesc(
      "Allowed command line options");

  cmdLineOptDesc.add_options()("help", "produce help message")(
      "input,i", boost::program_options::value<std::string>()->required(),
      "input file")("output,o", boost::program_options::value<std::string>(),
                    "output file")("inplace", "overwrites the input file");

  boost::program_options::positional_options_description cmdLinePosDesc;
  cmdLinePosDesc.add("input", 1).add("output", 1);
  boost::program_options::variables_map varMap;

  try {
    boost::program_options::store(
        boost::program_options::command_line_parser(argc, argv)
            .options(cmdLineOptDesc)
            .positional(cmdLinePosDesc)
            .run(),
        varMap);
    if (varMap.count("help")) {
      std::cout << "recoEventsClean"
                << "\nRemove duplicated entries from reco TTrees\n"
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
  conflicting_options(*varMap, "output", "inplace");
  if (!varMap->count("output") && !varMap->count("inplace")) {
    std::cerr << "Either '--output' or '--inplace' options must be provided\n";
    return 1;
  }
  auto inputName = (*varMap)["input"].as<std::string>();
  auto *inputFile = TFile::Open(inputName.c_str(), "READ");
  if (!inputFile) {
    std::cerr << "Can't open input file " << inputName << '\n';
    return 1;
  }
  auto *inputTree = static_cast<TTree *>(inputFile->Get("TPCRecoData"));
  if (!inputTree) {
    std::cerr << "No valid TTree in input file\n";
    return 1;
  }
  auto primaryKey = "EventInfo.runId";
  auto secondaryKey = "EventInfo.eventId";
  auto values = inputTree->BuildIndex(primaryKey, secondaryKey);
  if (values != inputTree->GetEntries()) {
    std::cerr << "Can't build index on input tree using " << primaryKey
              << " and " << secondaryKey << '\n';
    return 1;
  }

  auto outputName = varMap->count("inplace")
                        ? inputName
                        : varMap->at("output").as<std::string>();
  auto *outputFile = TFile::Open(outputName.c_str(), "RECREATE");
  if (!outputFile) {
    std::cerr << "Can't open output file " << outputName << '\n';
    return 1;
  }
  auto *outputTree = tpcreco::utilities::cloneUnique(inputTree, outputFile);
  if (!outputTree) {
    std::cerr << "Clonning TTree failed\n";
    return 1;
  }
  outputFile->Write();
  std::cout << "Removed " << inputTree->GetEntries() - outputTree->GetEntries()
            << " duplicated entries keeping the younger\n";
  outputFile->Close();
  inputFile->Close();
  return 0;
}