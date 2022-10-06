#ifndef TPCRECO_UTILITIES_REQUIREMENTS_COLLECTION_H_
#define TPCRECO_UTILITIES_REQUIREMENTS_COLLECTION_H_
#include <algorithm>
#include <functional>
#include <vector>
template <class T> class RequirementsCollection {
public:
  RequirementsCollection() = default;

  RequirementsCollection(std::initializer_list<T> requirements)
      : requirements(requirements) {}

  void push_back(T &&requirement) { requirements.push_back(requirement); }
  template <class Arg> bool operator()(const Arg &candidate) const {
    return std::all_of(std::begin(requirements), std::end(requirements),
                       [&candidate](T f) { return f(candidate); });
  }

  void clear() noexcept { requirements.clear(); }

  size_t size() const noexcept { return requirements.size(); }

private:
  std::vector<T> requirements;
};

#endif // TPCRECO_UTILITIES_REQUIREMENTS_COLLECTION_H_