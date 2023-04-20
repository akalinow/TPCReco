#include "DiffAnalysis.h"
#include "ConfigManager.h"
#include <boost/program_options.hpp>
#include <iostream>
#include <string>

int main(int argc, char **argv) {
  std::vector<std::string> requiredOptions = {"input", "reference"};
  boost::property_tree::ptree tree = getConfig(argc,argv,requiredOptions);
  auto inputName = tree.get("input","");
  auto referenceName = tree.get("reference","");
  auto analysis = tpcreco::analysis::diff::Analysis(inputName, referenceName);

  if (!tree.get("no-segments","")) {
    analysis.getDetailSink()->addCheck(
        tpcreco::analysis::diff::checks::checkSegments);
  }
  if (!("no-type","")) {
    analysis.getDetailSink()->addCheck(
        tpcreco::analysis::diff::checks::checkType);
  }
  if (tree.get("no-info","")) {
    analysis.getDetailSink()->resetTreeInfo();
    analysis.getExtraSink()->resetTreeInfo();
  }
  if (tree.get("no-presence","")) {
    analysis.getExtraSink()->disable(true);
  }

  analysis.run();

  return 0;
}