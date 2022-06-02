#include "GlobWrapper.h"
#include <glob.h>
#include <stdexcept>

std::vector<std::string> globWrapper(const std::string &pattern, int flags) {

  class GlobHelper {
  public:
    GlobHelper(const std::string &pattern, int flags) {
      auto ret = glob(pattern.c_str(), flags, nullptr, &pglob);
      if ((ret == 0) || (ret == GLOB_NOMATCH)) {
        return;
      }
      if (ret == GLOB_NOSPACE) {
        throw std::runtime_error("run out of memory");
      }
      if (ret == GLOB_ABORTED) {
        throw std::runtime_error("read error");
      }
    }

    GlobHelper(const GlobHelper &other) = delete;
    GlobHelper &operator=(const GlobHelper &other) = delete;
    GlobHelper(GlobHelper &&other) {
      pglob = std::move(other.pglob);
      other.pglob.gl_pathv = nullptr;
      other.pglob.gl_pathc = 0;
    }
    GlobHelper &operator=(GlobHelper &&other) {
      globfree(&pglob);
      pglob = std::move(other.pglob);
      other.pglob.gl_pathv = nullptr;
      other.pglob.gl_pathc = 0;
      return *this;
    }
    ~GlobHelper() { globfree(&pglob); }

    std::vector<std::string> data() {
      std::vector<std::string> paths;
      for (size_t i = 0; i < pglob.gl_pathc; ++i) {
        paths.emplace_back(pglob.gl_pathv[i]);
      }
      return paths;
    }

  private:
    glob_t pglob;
  };

  return GlobHelper(pattern, flags).data();
}