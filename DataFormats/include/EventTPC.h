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


class EventTPC : public std::enable_shared_from_this<EventTPC> {
  //  friend class SigClusterTPC;
 private:
  int64_t event_id, run_id;
  std::shared_ptr<GeometryTPC> myGeometryPtr;
  
  std::map<MultiKey3, double, multikey3_less> chargeMap; // key=(STRIP_DIR [0-2], STRIP_NUM [1-1024], TIME_CELL [0-511])
  std::map<MultiKey2, double, multikey2_less> maxChargeMap; // key=(STRIP_DIR [0-2], STRIP_NUM [1-1024])
  std::map<MultiKey2, double, multikey2_less> totalChargeMap; // key=(STRIP_DIR [0-2], STRIP_NUM [1-1024])
  std::map<MultiKey2, double, multikey2_less> totalChargeMap2; // key=(STRIP_DIR [0-2], TIME_CELL [0-511])
  std::map<int, double> totalChargeMap3; // key=TIME_CELL [0-511]

  bool initOK;      // is geometry valid?
  int time_rebin;   // how many raw data time bins to merge (default=1, i.e. none)

  double max_charge[3];       // maximal value from each strip direction [0-2]
  int max_charge_timing[3];   // range [0-511], RAW time cells
  int max_charge_strip[3];    // range [1-1024]
  double glb_max_charge;
  int glb_max_charge_timing;  // range [0-511]
  int glb_max_charge_channel; // range [0-1023]
  double tot_charge[3];
  double glb_tot_charge;

 public:
  EventTPC();

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

  inline auto GetGeoPtr() const { return myGeometryPtr; }
  inline int64_t GetEventId() const { return event_id; }
  inline bool IsOK() const { return initOK; }

  double GetMaxCharge();                   // maximal charge from all strips

  SigClusterTPC GetOneCluster(double thr, int delta_strips, int delta_timecells); // applies clustering threshold to all space-time data points 
  
  std::shared_ptr<TH2D> GetStripVsTime(const SigClusterTPC &cluster, projection strip_dir);        // clustered hits only, valid dir range [0-2]
  std::shared_ptr<TH2D> GetStripVsTime(projection strip_dir);                               // whole event, all strip dirs
  std::shared_ptr<TH2D> GetStripVsTimeInMM(const SigClusterTPC &cluster, projection strip_dir);  // valid range [0-2]

  std::vector<std::shared_ptr<TH2D>> Get2D(const SigClusterTPC &cluster, double radius,          // clustered hits only,
			   int rebin_space=EVENTTPC_DEFAULT_STRIP_REBIN,   // projections on: XY, XZ, YZ planes
			   int rebin_time=EVENTTPC_DEFAULT_TIME_REBIN, 
			   int method=EVENTTPC_DEFAULT_RECO_METHOD);  

  std::shared_ptr<TH3D> Get3D(const SigClusterTPC &cluster, double radius,                       // clustered hits only, 3D view
	      int rebin_space=EVENTTPC_DEFAULT_STRIP_REBIN, 
	      int rebin_time=EVENTTPC_DEFAULT_TIME_REBIN, 
	      int method=EVENTTPC_DEFAULT_RECO_METHOD);
};

#endif