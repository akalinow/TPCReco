#include "TPCReco/MakeUniqueName.h"
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;
std::string MakeUniqueName(const std::string &filename) {
  auto path = fs::path(filename);
  if (!fs::exists(path)) {
    return filename;
  }
  size_t i = 0;
  fs::path proposedPath = "";
  do {
    ++i;
    auto proposedFilename = path.stem().string() + "-" + std::to_string(i) +
                            path.extension().string();
    proposedPath = path.parent_path() / fs::path(proposedFilename);
  } while (fs::exists(proposedPath));
  return proposedPath.string();
}
