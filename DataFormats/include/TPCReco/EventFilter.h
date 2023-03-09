#ifndef _EventFilter_H_
#define _EventFilter_H_
#include "TPCReco/EventTPC.h"
#include <boost/property_tree/json_parser.hpp>
#include <set>
class EventFilter {
public:
  bool pass(EventTPC &event) const;
  
  void setConditions(const boost::property_tree::ptree &conditions);
  const boost::property_tree::ptree getConditions() const;
  
  void setEnabled(bool enabled){this->enabled=enabled;}
  void setDisabled(bool disabled){enabled=!disabled;}
  bool isEnabled() const {return enabled;}
  bool isDisabled() const {return !enabled;}

private:
  boost::property_tree::ptree conditions;
  bool enabled=false;
  std::set<size_t> indices;
};
#endif // _EventFilter_H_
