#include "TPCReco/DiffAnalysis.h"
#include "TPCReco/ConfigManager.h"
#include <boost/property_tree/ptree.hpp>
#include <string>

#include <TROOT.h>
#include <TApplication.h>

int main(int argc, char **argv) {

  // use only a subset of all allowed parameters
  ConfigManager cm( {"recoDiff.input","recoDiff.reference",
	"recoDiff.no-segments","recoDiff.no-type","recoDiff.no-presence","recoDiff.no-info"} );
  boost::property_tree::ptree tree = cm.getConfig(argc,argv);
  if(cm.isHelpMode()) return 0; // nothing more to do, exit

  auto recoInputName = tree.get<std::string>("recoDiff.input");
  auto recoReferenceName = tree.get<std::string>("recoDiff.reference");
  auto analysis = tpcreco::analysis::diff::Analysis(recoInputName, recoReferenceName);

  if (!(tree.get<bool>("recoDiff.no-segments"))) {
    analysis.getDetailSink()->addCheck(
        tpcreco::analysis::diff::checks::checkSegments);
  }
  if (!(tree.get<bool>("recoDiff.no-type"))) {
    analysis.getDetailSink()->addCheck(
        tpcreco::analysis::diff::checks::checkType);
  }
  if (tree.get<bool>("recoDiff.no-info")) {
    analysis.getDetailSink()->resetTreeInfo();
    analysis.getExtraSink()->resetTreeInfo();
  }
  if (tree.get<bool>("recoDiff.no-presence")) {
    analysis.getExtraSink()->disable(true);
  }

  analysis.run();

  return 0;
}
