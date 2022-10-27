#ifndef TPCRECO_UTILITIES_REQUIREMENTS_COLLECTION_H_
#define TPCRECO_UTILITIES_REQUIREMENTS_COLLECTION_H_
#include <algorithm>
#include <vector>
template <class T> class RequirementsCollection {
public:
  RequirementsCollection() = default;

  RequirementsCollection(std::initializer_list<T> requirements)
      : requirements(requirements) {}

  void push_back(T &&requirement) { requirements.push_back(requirement); }
  template <class... Args> bool operator()(Args &&...args) {
    return std::all_of(
        std::begin(requirements), std::end(requirements),
        [&args...](T f) { return f(std::forward<Args>(args)...); });
  }

  void clear() noexcept { requirements.clear(); }

  size_t size() const noexcept { return requirements.size(); }

private:
  std::vector<T> requirements;
};

template <class Req> class CountedRequirement {
public:
  CountedRequirement(Req &&requirement) : requirement(requirement) {}
  template <class... Args> bool operator()(Args &&...args) {
    auto isPassing = requirement(std::forward<Args>(args)...);
    if (isPassing) {
      ++count;
    }
    return isPassing;
  }
  size_t getCount() const noexcept { return count; }
  void resetCount() { count = 0; }

private:
  size_t count = 0;
  Req requirement;
};

template <class T, class... Args> CountedRequirement<T> make_counted(Args &&... function) {
  return CountedRequirement<T>(std::forward<Args>(function)...);
}

#endif // TPCRECO_UTILITIES_REQUIREMENTS_COLLECTION_H_