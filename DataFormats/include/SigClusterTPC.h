#ifndef __SIGCLUSTERTPC_H__
#define __SIGCLUSTERTPC_H__

// TPC event class.
// VERSION: pon, 24 cze 2019, 14:14:30 CEST

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
#include <set>
#include <cstdint>

#include "CommonDefinitions.h"

class GeometryTPC;
class EventTPC;

// Space-time mask for signal clusters defined as a class
class SigClusterTPC {
    friend class EventTPC;
 private:
     EventTPC& evt_ref;              // pointer to the existing TPC geometry
     std::set<std::tuple<direction, int, int>> hitList; // list of selected space-time cells for further analysis where return
                                  // value=key(STRIP_DIR [0-2], STRIP_NUM [1-1024], REBIN_TIME_CELL [0-511])
  std::set< std::tuple<int, direction, int>> hitListByTimeDir; // same list of selected space-time cells as a map
                                                        // with key=(REBIN_TIME_CELL [0-511], STRIP_DIR [0-2]) 
                                                        // that returns vector of STRIP_NUMBERS 

  // statistics variables
  std::map<direction, size_t> nhits;   // number of space-time cells in a given U,V,W direction

 public:
  
  SigClusterTPC(EventTPC& e); // constructor needs a pointer to the existing event
  inline decltype(hitList) GetHitList() const { return hitList; } // list of ALL hits, value=key(STRIP_DIR [0-2], STRIP_NUM [1-1024], REBIN_TIME_CELL [0-511])

  // helper methods for inserting data points
  // they return TRUE on success and FALSE on error
  bool AddHit(std::tuple<direction, int, int> hit);     // valid range [0-2][1-1024][0-511]

  // statistics
  size_t GetNhits(direction strip_dir) const;                        // # of hits in a given direction

  void UpdateStats(); //update all statistics variables

  std::shared_ptr<TH2D> GetStripVsTimeInMM(direction strip_dir);  // valid range [0-2]

  Reconstr_hist Get(double radius,          // clustered hits only, / clustered hits only, 3D view
      int rebin_space = EVENTTPC_DEFAULT_STRIP_REBIN,   // directions on: XY, XZ, YZ planes / all planes
      int rebin_time = EVENTTPC_DEFAULT_TIME_REBIN,
      int method = EVENTTPC_DEFAULT_RECO_METHOD);
};

#endif
