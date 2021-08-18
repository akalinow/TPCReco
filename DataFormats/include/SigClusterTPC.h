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
  std::vector<MultiKey4> hitList2;// list of selected space-time cells (per section) for further analysis where return
                                  // value=key(STRIP_DIR [0-2], STRIP_SEC [0-2], STRIP_NUM [1-1024], REBIN_TIME_CELL [0-511])
  std::map<MultiKey2, 
    std::vector<int>, multikey2_less> hitListByTimeDir;// same list of selected space-time cells as a map
                                                        // with key=(REBIN_TIME_CELL [0-511], STRIP_DIR [0-2]) 
                                                        // that returns vector of STRIP_NUMBERS 
  std::map<MultiKey3, 
    std::vector<int>, multikey3_less> hitListByTimeDir2;// same list of selected space-time cells (per section) as a map
                                                        // with key=(REBIN_TIME_CELL [0-511], STRIP_DIR [0-2], STRIP_SEC [0-2]) 
                                                        // that returns vector of STRIP_NUMBERS 
  std::map<int, std::vector<MultiKey2> > hitListByDir;  // same list of selected space-time cells as a map
                                                        // with key=STRIP_DIR [0-2]) 
                                                        // that returns key=(REBIN_TIME_CELL [0-511], STRIP_NUM [1-1024]
  std::map<int, std::vector<MultiKey3> > hitListByDir2; // same list of selected space-time cells (per section) as a map
                                                        // with key=STRIP_DIR [0-2]) 
                                                        // that returns key=(REBIN_TIME_CELL [0-511], STRIP_SEC [0-2], STRIP_NUM [1-1024]
  bool initOK;                    // is geometry valid?

  // statistics variables
  long nhits[3];   // number of space-time cells in a given U,V,W direction (per section)
  std::map<MultiKey2, int, multikey2_less> nhitsMap; // number of space-time cells per merged strip, 
                                                     // key=(STRIP_DIR [0-2], STRIP_NUM [1-1024])
  std::map<MultiKey3, int, multikey3_less> nhitsMap2;// number of space-time cells per strip per section, 
                                                     // key=(STRIP_DIR [0-2], STRIP_SEC [0-2], STRIP_NUM [1-1024])
  std::map<MultiKey2, int, multikey2_less> nhitsMap3;// number of space-time cells per direction per section, 
                                                     // key=(STRIP_DIR [0-2], STRIP_SEC [0-2])
  int nstrips[3];   // number of merged strips with some hits in a given direction
  int min_strip[3]; // minimal merged strip number for each direction (-1=error)
  int max_strip[3]; // maximal merged strip number for each direction (-1=error)
  int min_time[3];  // minimal RAW time cell for each direction (-1=error)
  int max_time[3];  // maximal RAW time cell for each direction (-1=error)
  int glb_min_time; // minimal RAW time cell from all strips (-1=error)
  int glb_max_time; // maximal RAW time cell from all strips (-1=error)

  double max_charge[3];              // (per section)
  int max_charge_timing[3];          // range [0-511] (per section)
  int max_charge_strip[3];           // range [1-1024] (per section)
  double glb_max_charge;             // (per section) (per section)
  int glb_max_charge_timing;         // range [0-511] (per section)
  int glb_max_charge_channel;        // range [0-1023] (per section)
  double tot_charge[3];              // charge integral
  double glb_tot_charge;             // charge integral

  std::map<MultiKey2, double, multikey2_less> maxChargeMap; // key=(STRIP_DIR [0-2], STRIP_NUM [1-1024])
  std::map<MultiKey3, double, multikey3_less> maxChargeMap2; // key=(STRIP_DIR [0-2], STRIP_SEC [0-2], STRIP_NUM [1-1024])
  std::map<MultiKey2, double, multikey2_less> totalChargeMap; // key=(STRIP_DIR [0-2], STRIP_NUM [1-1024])
  std::map<MultiKey2, double, multikey2_less> totalChargeMap2; // key=(STRIP_DIR [0-2], TIME_CELL [0-511])
  std::map<int, double> totalChargeMap3; // key=TIME_CELL [0-511]
  std::map<MultiKey3, double, multikey3_less> totalChargeMap4; // key=(STRIP_DIR [0-2], STRIP_SEC [0-2], STRIP_NUM [1-1024])
  std::map<MultiKey3, double, multikey3_less> totalChargeMap5; // key=(STRIP_DIR [0-2], STRIP_SEC [0-2], TIME_CELL [0-511])

 public:

  SigClusterTPC() {}; //default contructor 
  
  SigClusterTPC(EventTPC *e); // constructor needs a pointer to the existing event
  inline std::vector<MultiKey3> GetHitListMerged() const { return hitList; } // list of ALL hits, value=key(STRIP_DIR [0-2], STRIP_NUM [1-1024], REBIN_TIME_CELL [0-511])
  inline std::vector<MultiKey4> GetHitList() const { return hitList2; } // list of ALL hits, value=key(STRIP_DIR [0-2], STRIP_SEC [0-2], STRIP_NUM [1-1024], REBIN_TIME_CELL [0-511])
  std::vector<MultiKey2> GetHitListByDirMerged(int strip_dir) const; // list of SELECTED hits corresponding to a given STRIP_DIR[0-2], value=key(REBIN_TIME_CELL [0-511], STRIP_NUM [1-1024])
  std::vector<MultiKey3> GetHitListByDir(int strip_dir) const; // list of SELECTED hits corresponding to a given STRIP_DIR[0-2], value=key(REBIN_TIME_CELL [0-511], STRIP_SEC [0-2], STRIP_NUM [1-1024])

  const std::map<MultiKey2, std::vector<int>, multikey2_less> & GetHitListByTimeDirMerged() const;
  const std::map<MultiKey3, std::vector<int>, multikey3_less> & GetHitListByTimeDir() const;
  
  inline EventTPC* GetEvtPtr() const{ return evt_ptr; }

  // helper methods for inserting data points
  // they return TRUE on success and FALSE on error
  bool AddByStrip(StripTPC* strip, int time_cell);                     // valid range [0-511]
  bool AddByStrip(int strip_dir, int strip_section, int strip_number, int time_cell);// valid range [0-2][0-2][1-1024][0-511]
  bool AddByGlobalChannel(int glb_channel_idx, int time_cell);         // valid range [0-1023][0-511]
  bool AddByGlobalChannel_raw(int glb_raw_channel_idx, int time_cell); // valid range [0-(1023+4*N)][0-511]
  bool AddByAgetChannel(int cobo_idx, int asad_idx, int aget_idx, int channel_idx, int time_cell); // valid range [0-1][0-3][0-3][0-63][0-511]
  bool AddByAgetChannel_raw(int cobo_idx, int asad_idx, int aget_idx, int raw_channel_idx, int time_cell); // valid range [0-1][0-3][0-3][0-67][0-511]

  // helper methods for checking cluster membership
  // they return TRUE for member data points and FALSE for non-member data points
  bool CheckByStrip(StripTPC* strip, int time_cell) const;                   // valid range [0-511]
  bool CheckByStrip(int strip_dir, int strip_section, int strip_number, int time_cell) const;  // valid range [0-2][0-2][1-1024][0-511]
  bool CheckByStripMerged(int strip_dir, int strip_number, int time_cell) const;  // valid range [0-2][1-1024][0-511] (if at least one section was hit)
  bool CheckByGlobalChannel(int glb_channel_idx, int time_cell) const;         // valid range [0-1023][0-511]
  bool CheckByGlobalChannel_raw(int glb_raw_channel_idx, int time_cell) const; // valid range [0-(1023+4*N)][0-511]
  bool CheckByAgetChannel(int cobo_idx, int asad_idx, int aget_idx, int channel_idx, int time_cell) const; // valid range [0-1][0-3][0-3][0-63][0-511]
  bool CheckByAgetChannel_raw(int cobo_idx, int asad_idx, int aget_idx, int raw_channel_idx, int time_cell) const; // valid range [0-1][0-3][0-3][0-67][0-511]

  inline bool IsOK() const { return initOK; }

  // statistics
  long GetNhitsByStrip(int strip_dir, int strip_section, int strip_num) const;  // # of hits in a given strip (per section)
  long GetNhitsByStripMerged(int strip_dir, int strip_num) const;  // # of hits in a given merged strip (all sections)
  long GetNhits(int strip_dir, int strip_section) const;      // # of hits in a given direction (per section)
  long GetNhits(int strip_dir) const;                         // # of hits in a given direction (all sections)
  long GetNhits() const;                                      // global # of hits
  int GetMultiplicity(int strip_dir, int strip_section) const;// # of strips with hits in a given direction (per section)
  int GetMultiplicity(int strip_dir) const;                   // # of strips with hits in a given direction (all sections)
  int GetMultiplicity() const;                                // global # of strips with hits from all strips
  int GetMinStrip(int strip_dir) const; // minimal merged strip number in a given direction (all sections)
  int GetMaxStrip(int strip_dir) const; // maximal merged strip number in a given direction (all sections)
  int GetMinTime(int strip_dir) const;  // minimal RAW time cell in a given direction (all sections)
  int GetMaxTime(int strip_dir) const;  // maximal RAW time cell in a given direction (all sections)
  int GetMinTime() const;               // minimal RAW time cell from all strips
  int GetMaxTime() const;               // maximal RAW time cell from all strips

  double GetMaxCharge() const;                   // maximal charge from all cluster hits (per section)
  double GetMaxCharge(int strip_dir) const;      // maximal charge from all cluster hits in a given direction (per section)
  double GetMaxCharge(int strip_dir, int strip_number) const; // maximal charge from cluster hits in a given strip (per section, if any)
  double GetMaxCharge(int strip_dir, int strip_section, int strip_number) const; // maximal charge from cluster hits in a given strip (per section, if any)
  int GetMaxChargeStrip(int strip_dir) const;    // strip number with the maximal charge in a given direction (per section)
  int GetMaxChargeTime() const;                  // RAW arrival time of the maximal charge from all strips (per section)
  int GetMaxChargeTime(int strip_dir) const;     // arrival time of the maximal charge in a given direction (per section)
  int GetMaxChargeChannel() const;               // global channel number with the maximal charge from all cluster hits (per section)
  double GetTotalCharge() const;                 // charge integral from all cluster hits
  double GetTotalCharge(int strip_dir) const;    // charge integral from all cluster hits in a given direction
  double GetTotalCharge(int strip_dir, int strip_number) const; // charge integral from cluster hits in a given merged merged strip (all sections, if any)
  double GetTotalCharge(int strip_dir, int strip_section, int strip_number) const; // charge integral from cluster hits in a given strip (per section, if any)
  double GetTotalChargeByTimeCell(int strip_dir, int strip_section, int time_cell) const; // charge integral from a single time cell from all cluster hits in a given direction (per section, if any)
  double GetTotalChargeByTimeCell(int strip_dir, int time_cell) const; // charge integral from a single time cell from all cluster hits in a given direction (all sections, if any)
  double GetTotalChargeByTimeCell(int time_cell) const; // charge integral from a single time cell from all cluster hits (if any)
};

#endif
