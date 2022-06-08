#ifndef __PEVENTTPC_H__
#define __PEVENTTPC_H__

/// TPC event class.
///
/// VERSION: 05 May 2018

#include <cstdlib>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <map>
#include <iterator>
#include <fstream>
#include <utility>
#include <algorithm> 
#include <numeric>

#include "EventInfo.h"
#include "StripTPC.h"

class PEventTPC {

private:

  std::map<std::tuple<int, int, int, int>, double> chargeMap; 

  eventraw::EventInfo myEventInfo;

public:

  PEventTPC() = default;

  ~PEventTPC() = default;

  const decltype(myEventInfo)& GetEventInfo() const { return myEventInfo; };
  
  void SetEventInfo(decltype(myEventInfo)& aEvInfo) {myEventInfo = aEvInfo; };

  void Clear();

  bool AddValByStrip(std::shared_ptr<StripTPC> strip, int time_cell, double val);                     
  
  friend std::ostream& operator<<(std::ostream& os, const PEventTPC& e);
};


#endif
