#ifndef __SIGCLUSTERTPC_H__
#define __SIGCLUSTERTPC_H__

// TPC event class.
// VERSION: pon, 24 cze 2019, 14:14:30 CEST

#include <cstdlib>
#include <vector>
#include <map>
#include <string>
#include <memory>

#include "MultiKey.h"
#include "GeometryTPC.h"

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
  SigClusterTPC(EventTPC *e); // constructor needs a pointer to the existing event
  inline std::vector<MultiKey3> GetHitList() { return hitList; } // list of ALL hits, value=key(STRIP_DIR [0-2], STRIP_NUM [1-1024], REBIN_TIME_CELL [0-511])
  std::vector<MultiKey2> GetHitListByDir(int strip_dir); // list of SELECTED hits corresponding to a given STRIP_DIR[0-2], value=key(REBIN_TIME_CELL [0-511], STRIP_NUM [1-1024])

  inline EventTPC* GetEvtPtr() { return evt_ptr; }

  // helper methods for inserting data points
  // they return TRUE on success and FALSE on error
  bool AddByStrip(StripTPC* strip, int time_cell);                     // valid range [0-511]
  bool AddByStrip(int strip_dir, int strip_number, int time_cell);     // valid range [0-2][1-1024][0-511]
  bool AddByGlobalChannel(int glb_channel_idx, int time_cell);         // valid range [0-1023][0-511]
  bool AddByGlobalChannel_raw(int glb_raw_channel_idx, int time_cell); // valid range [0-(1023+4*N)][0-511]
  bool AddByAgetChannel(int cobo_idx, int asad_idx, int aget_idx, int channel_idx, int time_cell); // valid range [0-1][0-3][0-3][0-63][0-511]
  bool AddByAgetChannel_raw(int cobo_idx, int asad_idx, int aget_idx, int raw_channel_idx, int time_cell); // valid range [0-1][0-3][0-3][0-67][0-511]

  // helper methods for checking cluster membership
  // they return TRUE for member data points and FALSE for non-member data points
  bool CheckByStrip(StripTPC* strip, int time_cell);                   // valid range [0-511]
  bool CheckByStrip(int strip_dir, int strip_number, int time_cell);  // valid range [0-2][1-1024][0-511]
  bool CheckByGlobalChannel(int glb_channel_idx, int time_cell);         // valid range [0-1023][0-511]
  bool CheckByGlobalChannel_raw(int glb_raw_channel_idx, int time_cell); // valid range [0-(1023+4*N)][0-511]
  bool CheckByAgetChannel(int cobo_idx, int asad_idx, int aget_idx, int channel_idx, int time_cell); // valid range [0-1][0-3][0-3][0-63][0-511]
  bool CheckByAgetChannel_raw(int cobo_idx, int asad_idx, int aget_idx, int raw_channel_idx, int time_cell); // valid range [0-1][0-3][0-3][0-67][0-511]

  inline bool IsOK() { return initOK; }

  // statistics
  long GetNhitsByStrip(int strip_dir, int strip_num);  // # of hits in a given strip
  long GetNhits(int strip_dir);                        // # of hits in a given direction
  long GetNhits();                                     // global # of hits
  int GetMultiplicity(int strip_dir);                  // # of strips with hits in a given direction
  int GetMultiplicity();                               // global # of strips with hits from all strips
  int GetMinStrip(int strip_dir); // minimal strip number in a given direction
  int GetMaxStrip(int strip_dir); // maximal strip number in a given direction
  int GetMinTime(int strip_dir);  // minimal RAW time cell in a given direction
  int GetMaxTime(int strip_dir);  // maximal RAW time cell in a given direction
  int GetMinTime();               // minimal RAW time cell from all strips
  int GetMaxTime();               // maximal RAW time cell from all strips

  double GetMaxCharge();                   // maximal charge from all cluster hits
  double GetMaxCharge(int strip_dir);      // maximal charge from all cluster hits in a given direction
  double GetMaxCharge(int strip_dir, int strip_number); // maximal charge from cluster hits in a given strip (if any)
  int GetMaxChargeStrip(int strip_dir);    // strip number with the maximal charge in a given direction 
  int GetMaxChargeTime();                  // RAW arrival time of the maximal charge from all strips
  int GetMaxChargeTime(int strip_dir);     // arrival time of the maximal charge in a given direction
  int GetMaxChargeChannel();               // global channel number with the maximal charge from all cluster hits
  double GetTotalCharge();                 // charge integral from all cluster hits
  double GetTotalCharge(int strip_dir);    // charge integral from all cluster hits in a given direction
  double GetTotalCharge(int strip_dir, int strip_number); // charge integral from cluster hits in a given strip (if any)
  double GetTotalChargeByTimeCell(int strip_dir, int time_cell); // charge integral from a single time cell from all cluster hits in a given direction
  double GetTotalChargeByTimeCell(int time_cell); // charge integral from a single time cell from all cluster hits (if any)
};

#endif