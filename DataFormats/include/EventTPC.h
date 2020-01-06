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
#include <variant> //C++17

#include "TH1D.h"
#include "TH2D.h"
#include "TH3F.h"
#include "MultiKey.h"
#include "CommonDefinitions.h"

#include "GeometryTPC.h"
#include "SigClusterTPC.h"

constexpr auto EVENTTPC_DEFAULT_RECO_METHOD = 1;  // 0 = equal charge division along the strip;
                                        // 1 = weighted charge division from complementary strip directions
constexpr auto EVENTTPC_DEFAULT_STRIP_REBIN = 2;  // number of strips to rebin [1-1024] ;
constexpr auto EVENTTPC_DEFAULT_TIME_REBIN = 5;  // number of time cells to rebin [1-512];

using Reconstr_hist = std::pair<std::map<projection, std::shared_ptr<TH2D>>, std::shared_ptr<TH3D>>;

class EventTPC {
  //  friend class SigClusterTPC;
 private:
  int64_t event_id, run_id;
  std::shared_ptr<GeometryTPC> EvtGeometryPtr;
  
  std::map<projection, std::map<int, std::map<int, double>>> chargeMap; // key=(STRIP_DIR [0-2], STRIP_NUM [1-1024], TIME_CELL [0-511])

  double glb_max_charge;

 public:
  EventTPC(std::shared_ptr<GeometryTPC> geo_ptr);

  ~EventTPC(){};

  void Clear();

  void SetGeoPtr(std::shared_ptr<GeometryTPC> aPtr);
  void SetEventId(int64_t aId) { event_id = aId; };
  void SetRunId(int64_t aId) { run_id =  aId; };
  // helper methods for inserting data points
  // they return TRUE on success and FALSE on error
  bool AddValByStrip(std::shared_ptr<StripTPC> strip, int time_cell, double val);                      // valid range [0-511]
  bool AddValByAgetChannel(int cobo_idx, int asad_idx, int aget_idx, int channel_idx, int time_cell, double val); // valid range [0-1][0-3][0-3][0-63][0-511]
  
  // helper methods for extracting data points
  // they return 0.0 for non-existing data points
  double GetValByStrip(projection strip_dir, int strip_number, int time_cell/*, bool &result*/);  // valid range [0-2][1-1024][0-511]

  inline auto GetGeoPtr() const { return EvtGeometryPtr; }
  inline int64_t GetEventId() const { return event_id; }

  double GetMaxCharge();                   // maximal charge from all strips

  std::shared_ptr<SigClusterTPC> GetOneCluster(double thr, int delta_strips, int delta_timecells); // applies clustering threshold to all space-time data points 
  
  std::shared_ptr<TH2D> GetStripVsTime(projection strip_dir);                               // whole event, all strip dirs
  std::shared_ptr<TH2D> GetStripVsTimeInMM(std::shared_ptr<SigClusterTPC> cluster, projection strip_dir);  // valid range [0-2]

  Reconstr_hist Get(std::shared_ptr<SigClusterTPC> cluster, double radius,          // clustered hits only, / clustered hits only, 3D view
      int rebin_space = EVENTTPC_DEFAULT_STRIP_REBIN,   // projections on: XY, XZ, YZ planes / all planes
      int rebin_time = EVENTTPC_DEFAULT_TIME_REBIN,
      int method = EVENTTPC_DEFAULT_RECO_METHOD);
};

#endif