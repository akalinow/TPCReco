<<<<<<< HEAD
#include "DiffAnalysis.h"
#include "ConfigManager.h"
=======
#include "TPCReco/DiffAnalysis.h"
>>>>>>> f354324fc0e2a0130807f8471dda39732124fe4f
#include <boost/program_options.hpp>
#include <iostream>
#include <string>

int main(int argc, char **argv) {
  ConfigManager cm;
  boost::property_tree::ptree tree = cm.getConfig(argc,argv);
  auto inputName = tree.get("input","");
  auto referenceName = tree.get("reference","");
  auto analysis = tpcreco::analysis::diff::Analysis(inputName, referenceName);

  if (!(tree.count("no-segments"))) {
    analysis.getDetailSink()->addCheck(
        tpcreco::analysis::diff::checks::checkSegments);
  }
  if (!(tree.count("no-type"))) {
    analysis.getDetailSink()->addCheck(
        tpcreco::analysis::diff::checks::checkType);
  }
  if (tree.count("no-info")) {
    analysis.getDetailSink()->resetTreeInfo();
    analysis.getExtraSink()->resetTreeInfo();
  }
  if (tree.count("no-presence")) {
    analysis.getExtraSink()->disable(true);
  }

  analysis.run();

  return 0;
}