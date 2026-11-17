#ifndef TPCRECO_RESOURCEPATH_H
#define TPCRECO_RESOURCEPATH_H

// Resolution of relative paths to resource files (geometry, configuration,
// ML models, test data), so that applications do not have to be started
// from the resources directory.
//
// A relative path that does not exist relative to the current directory is
// looked up, in this order, relative to:
//   1. the directory in the TPCRECO_RESOURCES environment variable (if set),
//   2. the installed resources directory (<install prefix>/resources/),
//   3. the resources directory of the source tree.
// The paths in the configuration files are written relative to the resources
// directory, e.g. "./geometry_ELITPC.dat" or "../config/montecarlo.json".

#include <string>
#include <vector>

namespace ResourcePath {

// Directories searched for resources, in the search order.
std::vector<std::string> searchDirectories();

// Path to an existing file or directory: the path itself if it is absolute,
// empty, or exists relative to the current directory, otherwise the first
// existing path relative to a resources directory. If nothing exists,
// the path is returned unchanged.
std::string resolve(const std::string &path);

// resolve() applied to each element of a separated list of paths.
std::string resolveList(const std::string &paths, char separator = ',');

} // namespace ResourcePath
#endif
