#ifndef TPCRECO_DATATYPES_FILTERS_H
#define TPCRECO_DATATYPES_FILTERS_H
#include <set>
#include <cstddef>
#include <iostream>
#include <memory>

namespace tpcreco {
namespace filters {

struct TotalChargeUpperBound {
  const double upperBound;
  template <class Event> bool operator()(Event &event) {
    return event.getCurrentEvent()->GetTotalCharge() < upperBound;
  }
};

struct TotalChargeLowerBound {
  const double lowerBound;
  template <class Event> bool operator()(Event &event) {
    return event.getCurrentEvent()->GetTotalCharge() > lowerBound;
  }
};

struct MaxChargeUpperBound {
  const double upperBound;
  template <class Event> bool operator()(Event &event) {
    return event.getCurrentEvent()->GetMaxCharge() < upperBound;
  }
};

struct MaxChargeLowerBound {
  const double lowerBound;
  template <class Event> bool operator()(Event &event) {
    return event.getCurrentEvent()->GetMaxCharge() > lowerBound;
  }
};

class EventIdInSet {
public:
  EventIdInSet() = default;
  EventIdInSet(std::initializer_list<size_t> indices) : indices(indices) {}
  template <class Event> bool operator()(Event &event) {
    return indices.size()==0 || (indices.size() && indices.find(event.getCurrentEvent()->GetEventInfo().GetEventId()) != indices.end());
  }
  void insert(size_t index) { indices.insert(index); }

private:
  std::set<size_t> indices;
};

class RecoProngInSet {
public:
  RecoProngInSet() = default;
  RecoProngInSet(std::initializer_list<size_t> indices) : indices(indices) {}
  template <class EventSource> bool operator()(EventSource &eventSrc) {
    return indices.size()==0 || 
           (indices.size() && 
            indices.find(eventSrc.getRecoEvent().getSegments().size()) != indices.end());
  }
  void insert(size_t index) { indices.insert(index); }


private:
  std::set<size_t> indices;
};

} // namespace filters
} // namespace tpcreco

#endif // TPCRECO_DATATYPES_FILTERS_H
