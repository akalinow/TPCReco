#include "EventFilter.h"
#include <limits>
bool EventFilter::pass(EventTPC &event) const {

  if(!enabled){
    return true;
  }

  if (!indices.empty() && ( indices.find(event.GetEventInfo().GetEventId()) == indices.end() ) ) {
    return false;
  }
  if(event.GetTotalCharge()<conditions.get("totalChargeLowerBound",-std::numeric_limits<double>::max())){
    return false;
  }
  if(event.GetTotalCharge()>conditions.get("totalChargeUpperBound",std::numeric_limits<double>::max())){
    return false;
  }
  if(event.GetMaxCharge()<conditions.get("maxChargeLowerBound",-std::numeric_limits<double>::max())){
    return false;
  }
  if(event.GetMaxCharge()>conditions.get("maxChargeUpperBound",std::numeric_limits<double>::max())){
    return false;
  }
  return true;
}

void EventFilter::setConditions(
    const boost::property_tree::ptree &aConditions) {
  auto conditionsNode = aConditions.find("eventFilter");
  conditions = conditionsNode == aConditions.not_found()
                   ? aConditions
                   : conditionsNode->second;
  enabled=conditions.get("enabled",false);
  auto indicesNode=conditions.find("events");
  if(indicesNode!= conditions.not_found()){
    indices.clear();
    for(auto i: indicesNode->second){
      indices.insert(i.second.get_value<int>());
    }
  }
}

const boost::property_tree::ptree EventFilter::getConditions() const {
  return conditions;
}
