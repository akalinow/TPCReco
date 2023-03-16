#ifndef _EventFilter_H_
#define _EventFilter_H_

#include "TPCReco/RequirementsCollection.h"
#include "TPCReco/Filters.h"
#include <boost/property_tree/ptree.hpp>

template <class T> class EventFilter {
public:
  template <class Event> bool pass(Event &event) {
    return enabled ? filters(event) : true;
  }
  void setConditions(const boost::property_tree::ptree &conditions);
  void setEnabled(bool enabled) { this->enabled = enabled; }
  void setDisabled(bool disabled) { enabled = !disabled; }
  bool isEnabled() const { return enabled; }
  bool isDisabled() const { return !enabled; }

private:
  bool enabled = false;
  RequirementsCollection<T> filters;
};

template <class Event>
void EventFilter<Event>::setConditions(
    const boost::property_tree::ptree &conditions) {
  auto nodeIt = conditions.find("eventFilter");
  if (nodeIt == conditions.not_found()) {
    return;
  }

  filters.clear();
  auto node = nodeIt->second;

  enabled = node.get("enabled", false);

  auto value = node.get_optional<double>("maxChargeUpperBound");
  if (value) {
    filters.push_back(tpcreco::filters::MaxChargeUpperBound{*value});
  }

  value = node.get_optional<double>("maxChargeLowerBound");
  if (value) {
    filters.push_back(tpcreco::filters::MaxChargeLowerBound{*value});
  }

  value = node.get_optional<double>("totalChargeLowerBound");
  if (value) {
    filters.push_back(tpcreco::filters::TotalChargeLowerBound{*value});
  }

  value = node.get_optional<double>("totalChargeUpperBound");
  if (value) {
    filters.push_back(tpcreco::filters::TotalChargeUpperBound{*value});
  }

  auto events = nodeIt->second.get_child_optional("events");
  if (events) {
    tpcreco::filters::IndexInSet set;
    for (const auto &index : *events) {
      set.insert(index.second.get_value<size_t>());
    }
    filters.push_back(std::move(set));
  }
}
#endif // _EventFilter_H_
