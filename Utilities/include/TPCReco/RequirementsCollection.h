#ifndef TPCRECO_UTILITIES_REQUIREMENTS_COLLECTION_H_
#define TPCRECO_UTILITIES_REQUIREMENTS_COLLECTION_H_
#include <algorithm>
#include <functional>
#include <vector>
template <class T> class RequirementsCollection {
public:
  RequirementsCollection() = default;

  RequirementsCollection(
      std::initializer_list<std::function<bool(const T &)>> requirements)
      : requirements(requirements) {}

  template <class Req> void push_back(Req &&requirement) {
    requirements.push_back(requirement);
  }

  bool operator()(const T &candidate) const {
    return std::all_of(std::cbegin(requirements), std::cend(requirements),
                       [&candidate](auto f) { return f(candidate); });
  }

  void clear() noexcept { requirements.clear(); }

  size_t size() const noexcept { return requirements.size(); }

private:
  std::vector<std::function<bool(const T &)>> requirements;
};

#endif // TPCRECO_UTILITIES_REQUIREMENTS_COLLECTION_H_