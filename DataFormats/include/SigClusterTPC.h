#pragma once
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

#include "MultiKey.h"
#include "GeometryTPC.h"
#include "EventTPC.h"

class EventTPC;

// Space-time mask for signal clusters defined as a class
class SigClusterTPC {
  friend class EventTPC;
 private:
  EventTPC *evt_ptr;              // pointer to the existing TPC geometry
  std::vector<MultiKey3> hitList; // list of selected space-time cells for further analysis where return
                                  // value=key(STRIP_DIR [0-2], STRIP_NUM [1-1024], REBIN_TIME_CELL [0-511])
  std::map<MultiKey2, 
    std::vector<int>, multikey2_less> hitListByTimeDir; // same list of selected space-time cells as a map
                                                        // with key=(REBIN_TIME_CELL [0-511], STRIP_DIR [0-2]) 
                                                        // that returns vector of STRIP_NUMBERS 
  std::map<int, std::vector<MultiKey2> > hitListByDir;  // same list of selected space-time cells as a map
                                                        // with key=STRIP_DIR [0-2]) 
                                                        // that returns key=(REBIN_TIME_CELL [0-511], STRIP_NUM [1-1024]
  bool initOK;                    // is geometry valid?

  // statistics variables
  long nhits[3];   // number of space-time cells in a given U,V,W direction
  std::map<MultiKey2, int, multikey2_less> nhitsMap; // number of space-time cells per strip, 
                                                     // key=(STRIP_DIR [0-2], STRIP_NUM [1-1024])
  int nstrips[3];   // number of strips with some hits in a given direction
  int min_strip[3]; // minimal strip number for each direction (-1=error)
  int max_strip[3]; // maximal strip number for each direction (-1=error)
  int min_time[3];  // minimal RAW time cell for each direction (-1=error)
  int max_time[3];  // maximal RAW time cell for each direction (-1=error)
  int glb_min_time; // minimal RAW time cell from all strips (-1=error)
  int glb_max_time; // maximal RAW time cell from all strips (-1=error)

  double max_charge[3];
  int max_charge_timing[3];   // range [0-511]
  int max_charge_strip[3];    // range [1-1024]
  double glb_max_charge;
  int glb_max_charge_timing;  // range [0-511]
  int glb_max_charge_channel; // range [0-1023]
  double tot_charge[3];
  double glb_tot_charge;

  std::map<MultiKey2, double, multikey2_less> maxChargeMap; // key=(STRIP_DIR [0-2], STRIP_NUM [1-1024])
  std::map<MultiKey2, double, multikey2_less> totalChargeMap; // key=(STRIP_DIR [0-2], STRIP_NUM [1-1024])
  std::map<MultiKey2, double, multikey2_less> totalChargeMap2; // key=(STRIP_DIR [0-2], TIME_CELL [0-511])
  std::map<int, double> totalChargeMap3; // key=TIME_CELL [0-511]

 public:

  SigClusterTPC() {}; //default contructor 
  
  SigClusterTPC(EventTPC *e); // constructor needs a pointer to the existing event
  inline std::vector<MultiKey3> GetHitList() const { return hitList; } // list of ALL hits, value=key(STRIP_DIR [0-2], STRIP_NUM [1-1024], REBIN_TIME_CELL [0-511])
  std::vector<MultiKey2> GetHitListByDir(projection strip_dir) const; // list of SELECTED hits corresponding to a given STRIP_DIR[0-2], value=key(REBIN_TIME_CELL [0-511], STRIP_NUM [1-1024])

  const std::map<MultiKey2, std::vector<int>, multikey2_less> & GetHitListByTimeDir() const;
  
  inline EventTPC* GetEvtPtr() const{ return evt_ptr; }

  // helper methods for inserting data points
  // they return TRUE on success and FALSE on error
  bool AddByStrip(std::shared_ptr<StripTPC> strip, int time_cell);                     // valid range [0-511]
  bool AddByStrip(projection strip_dir, int strip_number, int time_cell);     // valid range [0-2][1-1024][0-511]
  bool AddByGlobalChannel(int glb_channel_idx, int time_cell);         // valid range [0-1023][0-511]
  bool AddByGlobalChannel_raw(int glb_raw_channel_idx, int time_cell); // valid range [0-(1023+4*N)][0-511]
  bool AddByAgetChannel(int cobo_idx, int asad_idx, int aget_idx, int channel_idx, int time_cell); // valid range [0-1][0-3][0-3][0-63][0-511]
  bool AddByAgetChannel_raw(int cobo_idx, int asad_idx, int aget_idx, int raw_channel_idx, int time_cell); // valid range [0-1][0-3][0-3][0-67][0-511]

  // helper methods for checking cluster membership
  // they return TRUE for member data points and FALSE for non-member data points
  bool CheckByStrip(std::shared_ptr<StripTPC> strip, int time_cell) const;                   // valid range [0-511]
  bool CheckByStrip(projection strip_dir, int strip_number, int time_cell) const;  // valid range [0-2][1-1024][0-511]
  bool CheckByGlobalChannel(int glb_channel_idx, int time_cell) const;         // valid range [0-1023][0-511]
  bool CheckByGlobalChannel_raw(int glb_raw_channel_idx, int time_cell) const; // valid range [0-(1023+4*N)][0-511]
  bool CheckByAgetChannel(int cobo_idx, int asad_idx, int aget_idx, int channel_idx, int time_cell) const; // valid range [0-1][0-3][0-3][0-63][0-511]
  bool CheckByAgetChannel_raw(int cobo_idx, int asad_idx, int aget_idx, int raw_channel_idx, int time_cell) const; // valid range [0-1][0-3][0-3][0-67][0-511]

  inline bool IsOK() const { return initOK; }

  // statistics
  long GetNhitsByStrip(projection strip_dir, int strip_num) const;  // # of hits in a given strip
  long GetNhits(projection strip_dir) const;                        // # of hits in a given direction
  long GetNhits() const;                                     // global # of hits
  int GetMultiplicity(projection strip_dir) const;                  // # of strips with hits in a given direction
  int GetMultiplicity() const;                               // global # of strips with hits from all strips
  int GetMinStrip(projection strip_dir) const; // minimal strip number in a given direction
  int GetMaxStrip(projection strip_dir) const; // maximal strip number in a given direction
  int GetMinTime(projection strip_dir) const;  // minimal RAW time cell in a given direction
  int GetMaxTime(projection strip_dir) const;  // maximal RAW time cell in a given direction
  int GetMinTime() const;               // minimal RAW time cell from all strips
  int GetMaxTime() const;               // maximal RAW time cell from all strips

  double GetMaxCharge() const;                   // maximal charge from all cluster hits
  double GetMaxCharge(projection strip_dir) const;      // maximal charge from all cluster hits in a given direction
  double GetMaxCharge(projection strip_dir, int strip_number) const; // maximal charge from cluster hits in a given strip (if any)
  int GetMaxChargeStrip(projection strip_dir) const;    // strip number with the maximal charge in a given direction 
  int GetMaxChargeTime() const;                  // RAW arrival time of the maximal charge from all strips
  int GetMaxChargeTime(projection strip_dir) const;     // arrival time of the maximal charge in a given direction
  int GetMaxChargeChannel() const;               // global channel number with the maximal charge from all cluster hits
  double GetTotalCharge() const;                 // charge integral from all cluster hits
  double GetTotalCharge(projection strip_dir) const;    // charge integral from all cluster hits in a given direction
  double GetTotalCharge(projection strip_dir, int strip_number) const; // charge integral from cluster hits in a given strip (if any)
  double GetTotalChargeByTimeCell(projection strip_dir, int time_cell) const; // charge integral from a single time cell from all cluster hits in a given direction
  double GetTotalChargeByTimeCell(int time_cell) const; // charge integral from a single time cell from all cluster hits (if any)
};

#endif
