#ifndef __PEVENTTPC_H__
#define __PEVENTTPC_H__

/// TPC event class.
///
/// VERSION: 05 May 2018

#include <map>

#include "TPCReco/EventInfo.h"
#include "TPCReco/StripTPC.h"

class PEventTPC {

private:

  eventraw::EventInfo myEventInfo;

public:

  typedef std::map<std::tuple<int, int, int, int>, double> chargeMapType;
  static const unsigned int max_strip_dirs{3};
  static const unsigned int max_strip_sections{3};
  static const unsigned int max_strip_numbers{256};
  static const unsigned int max_strip_time_cells{512};

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

  float myChargeArray[max_strip_dirs][max_strip_sections][max_strip_numbers][max_strip_time_cells];
};


#endif
