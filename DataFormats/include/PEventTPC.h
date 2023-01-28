#ifndef __PEVENTTPC_H__
#define __PEVENTTPC_H__

/// TPC event class.
///
/// VERSION: 05 May 2018

#include <map>

#include "EventInfo.h"
#include "StripTPC.h"

class PEventTPC {

private:

  eventraw::EventInfo myEventInfo;

public:

  typedef std::map<std::tuple<int, int, int, int>, double> chargeMapType;

  PEventTPC() = default;

  ~PEventTPC() = default;

  const decltype(myEventInfo)& GetEventInfo() const { return myEventInfo; };

  const chargeMapType & GetChargeMap() const { return myChargeMap;}

  void Clear();
  
  void SetEventInfo(decltype(myEventInfo)& aEvInfo) {myEventInfo = aEvInfo; };

  bool AddValByStrip(const std::shared_ptr<StripTPC> & strip, int time_cell, double val);                     
  
  friend std::ostream& operator<<(std::ostream& os, const PEventTPC& e);

  private:

  chargeMapType myChargeMap;
  
};


#endif
