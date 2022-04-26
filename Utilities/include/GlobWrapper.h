#ifndef UTILITIES_GLOB_WRAPPER_H_
#define UTILITIES_GLOB_WRAPPER_H_
#include <string>
#include <vector>

// searches for all the paths matching pattern according to rules used by shell.
// The flags is made up of bitwise OR of constants declared in <glob.h>
std::vector<std::string> globWrapper(const std::string &pattern, int flags = 0);

#endif // UTILITIES_GLOB_WRAPPER_H_