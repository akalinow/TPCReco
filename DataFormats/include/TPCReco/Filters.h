#ifndef TPCRECO_DATATYPES_FILTERS_H
#define TPCRECO_DATATYPES_FILTERS_H
#include <set>
#include <cstddef>

namespace tpcreco {
namespace filters {
struct TotalChargeUpperBound {
  const double upperBound;
  template <class Event> bool operator()(Event &event) {
    return event.GetTotalCharge() < upperBound;
  }
};

struct TotalChargeLowerBound {
  const double lowerBound;
  template <class Event> bool operator()(Event &event) {
    return event.GetTotalCharge() > lowerBound;
  }
};

struct MaxChargeUpperBound {
  const double upperBound;
  template <class Event> bool operator()(Event &event) {
    return event.GetMaxCharge() < upperBound;
  }
};

struct MaxChargeLowerBound {
  const double lowerBound;
  template <class Event> bool operator()(Event &event) {
    return event.GetMaxCharge() > lowerBound;
  }
};

class IndexInSet {
public:
  IndexInSet() = default;
  IndexInSet(std::initializer_list<size_t> indices) : indices(indices) {}
  template <class Event> bool operator()(Event &event) {
    return indices.size()==0 || (indices.size() && indices.find(event.GetEventInfo().GetEventId()) != indices.end());
  }
  void insert(size_t index) { indices.insert(index); }

private:
  std::set<size_t> indices;
};
} // namespace filters
} // namespace tpcreco

#endif // TPCRECO_DATATYPES_FILTERS_H
