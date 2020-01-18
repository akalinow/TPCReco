#ifndef __EVENTTPC_H__
#define __EVENTTPC_H__

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
#include <algorithm> // for find_if
#include <numeric>

#include "TH1D.h"
#include "TH2D.h"
#include "TH3F.h"
#include "MultiKey.h"
#include "CommonDefinitions.h"

#include "GeometryTPC.h"
#include "SigClusterTPC.h"

class EventTPC {
  //  friend class SigClusterTPC;
 private:
  int64_t event_id, run_id;
  
  std::map<projection, std::map<int, std::map<int, double>>> chargeMap; // key=(STRIP_DIR [0-2], STRIP_NUM [1-1024], TIME_CELL [0-511])

  double glb_max_charge = 0.0;

 public:
  EventTPC() = default;

  ~EventTPC() = default;

  void Clear();

  void SetEventId(int64_t aId) { event_id = aId; };
  void SetRunId(int64_t aId) { run_id =  aId; };
  // helper methods for inserting data points
  // they return TRUE on success and FALSE on error
  bool AddValByStrip(std::shared_ptr<StripTPC> strip, int time_cell, double val);                      // valid range [0-511]
  bool AddValByAgetChannel(int cobo_idx, int asad_idx, int aget_idx, int channel_idx, int time_cell, double val); // valid range [0-1][0-3][0-3][0-63][0-511]
  
  // helper methods for extracting data points
  // they return 0.0 for non-existing data points
  double GetValByStrip(projection strip_dir, int strip_number, int time_cell/*, bool &result*/);  // valid range [0-2][1-1024][0-511]

  inline int64_t GetEventId() const { return event_id; }

  double GetMaxCharge();                   // maximal charge from all strips

  std::shared_ptr<SigClusterTPC> GetOneCluster(double thr, int delta_strips, int delta_timecells); // applies clustering threshold to all space-time data points 
  
  std::shared_ptr<TH2D> GetStripVsTime(projection strip_dir);                               // whole event, all strip dirs
};

#endif