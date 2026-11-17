#include "TPCReco/ResourcePath.h"

#include <cstdlib>
#include <sstream>

#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

namespace ResourcePath {
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::vector<std::string> searchDirectories() {
  std::vector<std::string> directories;
  if (const char *fromEnvironment = std::getenv("TPCRECO_RESOURCES")) {
    if (*fromEnvironment) directories.push_back(fromEnvironment);
  }
  directories.push_back(TPCRECO_INSTALL_RESOURCE_DIR);
  directories.push_back(TPCRECO_RESOURCE_DIR);
  return directories;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string resolve(const std::string &path) {
  if (path.empty()) return path;
  const fs::path aPath(path);
  boost::system::error_code error;
  if (aPath.is_absolute() || fs::exists(aPath, error)) return path;
  for (const auto &directory : searchDirectories()) {
    const fs::path candidate = fs::path(directory) / aPath;
    if (fs::exists(candidate, error)) return candidate.lexically_normal().string();
  }
  return path;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::string resolveList(const std::string &paths, char separator) {
  std::stringstream input(paths), output;
  std::string item;
  bool first = true;
  while (std::getline(input, item, separator)) {
    if (!first) output << separator;
    output << resolve(item);
    first = false;
  }
  return output.str();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
} // namespace ResourcePath
