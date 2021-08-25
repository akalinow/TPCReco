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

#include "TH1D.h"
#include "TH2D.h"
#include "TH3F.h"

#include "GeometryTPC.h"

#define EVENTTPC_DEFAULT_RECO_METHOD 1  // 0 = equal charge division along the strip
                                        // 1 = weighted charge division from complementary strip directions
#define EVENTTPC_DEFAULT_STRIP_REBIN 2  // number of strips to rebin [1-1024] 
#define EVENTTPC_DEFAULT_TIME_REBIN  5  // number of time cells to rebin [1-512]

class TrackSegment3D;
class SigClusterTPC;

class EventTPC {
  //  friend class SigClusterTPC;
 private:
  Long64_t event_id, event_number, run_id, event_time;
  std::shared_ptr<GeometryTPC> myGeometryPtr;  //! transient data member
  
  std::map<MultiKey3, double, multikey3_less> chargeMap; // key=(STRIP_DIR [0-2], STRIP_NUM [1-1024], TIME_CELL [0-511])
  std::map<MultiKey4, double, multikey4_less> chargeMap2; // key=(STRIP_DIR [0-2], SECTION [0-2], STRIP_NUM [1-1024], TIME_CELL [0-511])
  std::map<MultiKey2, double, multikey2_less> maxChargeMap; // key=(STRIP_DIR [0-2], STRIP_NUM [1-1024])
  std::map<MultiKey3, double, multikey3_less> maxChargeMap2; // key=(STRIP_DIR [0-2], SECTION [0-2], STRIP_NUM [1-1024])
  std::map<MultiKey2, double, multikey2_less> totalChargeMap; // key=(STRIP_DIR [0-2], STRIP_NUM [1-1024])
  std::map<MultiKey2, double, multikey2_less> totalChargeMap2; // key=(STRIP_DIR [0-2], TIME_CELL [0-511])
  std::map<int, double> totalChargeMap3; // key=TIME_CELL [0-511]
  std::map<MultiKey3, double, multikey3_less> totalChargeMap4; // key=(STRIP_DIR [0-2], SECTION [0-2], STRIP_NUM [1-1024])
  std::map<MultiKey3, double, multikey3_less> totalChargeMap5; // key=(STRIP_DIR [0-2], SECTION [0-2], TIME_CELL [0-511])

  bool initOK;      // is geometry valid?
  int time_rebin;   // how many raw data time bins to merge (default=1, i.e. none)

  double max_charge[3];       // maximal value from each strip direction [0-2]
  int max_charge_timing[3];   // range [0-511], RAW time cells
  int max_charge_strip[3];    // range [1-1024]
  double glb_max_charge;
  int glb_max_charge_timing;  // range [0-511]
  int glb_max_charge_channel; // range [0-1023]
  double tot_charge[3];       // integral per strip direction
  double glb_tot_charge;      // integral per event

 public:
  EventTPC();

  ~EventTPC(){};

  void Clear();

  void SetGeoPtr(std::shared_ptr<GeometryTPC> aPtr);
  void SetEventId(Long64_t aId) { event_id = aId; };
  void SetEventNumber(Long64_t aNumber) { event_number = aNumber; };
  void SetEventTime(Long64_t aTime) { event_time = aTime; };
  void SetRunId(Long64_t aId) { run_id =  aId; };
  // helper methods for inserting data points
  // they return TRUE on success and FALSE on error
  bool AddValByStrip(StripTPC* strip, int time_cell, double val);                      // valid range [0-511]
  bool AddValByStrip(int strip_dir, int strip_section, int strip_number, int time_cell, double val);     // valid range [0-2][0-2][1-1024][0-511]
  bool AddValByGlobalChannel(int glb_channel_idx, int time_cell, double val);         // valid range [0-1023][0-511]
  bool AddValByGlobalChannel_raw(int glb_raw_channel_idx, int time_cell, double val); // valid range [0-1023+4*N][0-511]
  bool AddValByAgetChannel(int cobo_idx, int asad_idx, int aget_idx, int channel_idx, int time_cell, double val); // valid range [0-1][0-3][0-3][0-63][0-511]
  bool AddValByAgetChannel_raw(int cobo_idx, int asad_idx, int aget_idx, int raw_channel_idx, int time_cell, double val); // valid range [0-1][0-3][0-3][0-67][0-511]
  
  // helper methods for extracting data points
  // they return 0.0 for non-existing data points
  double GetValByStrip(StripTPC* strip, int time_cell/*, bool &result*/);                   // valid range [0-511]
  double GetValByStrip(int strip_dir, int strip_section, int strip_number, int time_cell/*, bool &result*/);  // valid range [0-2][1-1024][0-511]
  double GetValByStripMerged(int strip_dir, int strip_number, int time_cell/*, bool &result*/);  // valid range [0-2][1-1024][0-511] (all sections)
  double GetValByGlobalChannel(int glb_channel_idx, int time_cell/*, bool &result*/);         // valid range [0-1023][0-511]
  double GetValByGlobalChannel_raw(int glb_raw_channel_idx, int time_cell/*, bool &result*/); // valid range [0-1023+4*N][0-511]
  double GetValByAgetChannel(int cobo_idx, int asad_idx, int aget_idx, int channel_idx, int time_cell/*, bool &result*/); // valid range [0-1][0-3][0-3][0-63][0-511]
  double GetValByAgetChannel_raw(int cobo_idx, int asad_idx, int aget_idx, int raw_channel_idx, int time_cell/*, bool &result*/); // valid range [0-1][0-3][0-3][0-67][0-511]

  inline GeometryTPC * GetGeoPtr() const { return myGeometryPtr.get(); }
  inline Long64_t GetEventId() const { return event_id; }
  inline Long64_t GetEventTime() const { return event_time; }
  inline Long64_t GetEventNumber() const { return event_number; }
  inline Long64_t GetRunId() const { return run_id; }
  inline bool IsOK() const { return initOK; }
  inline int GetTimeRebin() const { return time_rebin; }       
  bool SetTimeRebin(int rebin); // HAS NO EFFECT YET !!!!

  double GetMaxCharge() const;             // maximal charge from all strips
  double GetMaxCharge(int strip_dir);      // maximal charge from strips of a given direction
  double GetMaxCharge(int strip_dir, int strip_number);      // maximal charge from merged strip of a given direction (all sections)
  double GetMaxCharge(int strip_dir, int strip_section, int strip_number);      // maximal charge from single strip of a given direction (per section)
  int GetMaxChargeTime(int strip_dir);     // arrival time of the maximal charge from strips of a given direction
  int GetMaxChargeStrip(int strip_dir);    // strip number with the maximal charge in a given direction 
  int GetMaxChargeTime();                  // arrival time of the maximal charge from all strips
  int GetMaxChargeChannel();               // global channel number with the maximal charge from all strips
  double GetTotalCharge() const;           // charge integral from all strips
  double GetTotalCharge(int strip_dir);    // charge integral from strips of a given direction 
  double GetTotalCharge(int strip_dir, int strip_number); // charge integral from merged strip of a given direction (all sections) 
  double GetTotalCharge(int strip_dir, int strip_section, int strip_number); // charge integral from single strip of a given direction (per section) 
  double GetTotalChargeByTimeCell(int time_cell); // charge integral from a single time cell from all strips
  double GetTotalChargeByTimeCell(int strip_dir, int time_cell); // charge integral from a single time cell from all merged strips in a given direction (all sections)
  double GetTotalChargeByTimeCell(int strip_dir, int strip_section, int time_cell); // charge integral from a single time cell from all strips in a given direction (per section)

  SigClusterTPC GetOneCluster(double thr, int delta_strips, int delta_timecells); // applies clustering threshold to all space-time data points 
  
  std::shared_ptr<TH1D> GetStripProjection(const SigClusterTPC &cluster, int strip_dir);    // clustered hits only, valid dir range [0-2]
  TH1D *GetTimeProjection(const SigClusterTPC &cluster, int strip_dir);     // clustered hits only, valid dir range [0-2]
  TH1D *GetTimeProjection(const SigClusterTPC &cluster);                    // clustered hits only, all strip dirs
  TH1D *GetStripProjection(int strip_dir);                            // whole,event, valid dir range [0-2]
  TH1D *GetTimeProjection(int strip_dir);                             // whole,event, valid dir range [0-2]
  TH1D *GetTimeProjection();                                          // whole event, all strip dirs
  std::shared_ptr<TH1D> GetStripProjectionInMM(const SigClusterTPC &cluster, int strip_dir);    // clustered hits only, valid dir range [0-2]
  std::shared_ptr<TH1D> GetTimeProjectionInMM(const SigClusterTPC &cluster, int strip_dir);     // clustered hits only, valid dir range [0-2]
  std::shared_ptr<TH1D> GetTimeProjectionInMM(const SigClusterTPC &cluster);                    // clustered hits only, all strip dirs
  std::shared_ptr<TH1D> GetStripProjectionInMM(int strip_dir);                            // whole,event, valid dir range [0-2]
  std::shared_ptr<TH1D> GetTimeProjectionInMM(int strip_dir);                             // whole,event, valid dir range [0-2]
  std::shared_ptr<TH1D> GetTimeProjectionInMM();                                          // whole event, all strip dirs
  
  std::shared_ptr<TH2D> GetStripVsTime(const SigClusterTPC &cluster, int strip_dir);        // clustered hits only, valid dir range [0-2]
  std::shared_ptr<TH2D> GetStripVsTime(int strip_dir);                               // whole event, all strip dirs
  std::shared_ptr<TH2D> GetStripVsTimeInMM(const SigClusterTPC &cluster, int strip_dir);  // valid range [0-2]
  std::shared_ptr<TH2D> GetStripVsTimeInMM(int strip_dir);  // whole event, valid range [0-2]
  std::shared_ptr<TH2D> GetChannels(int cobo_idx, int asad_idx); // valid range [0-1][0-3]
  std::shared_ptr<TH2D> GetChannels_raw(int cobo_idx, int asad_idx); // valid range [0-1][0-3]

  std::vector<TH2D*> Get2D(const SigClusterTPC &cluster, double radius,          // clustered hits only,
			   int rebin_space=EVENTTPC_DEFAULT_STRIP_REBIN,   // projections on: XY, XZ, YZ planes
			   int rebin_time=EVENTTPC_DEFAULT_TIME_REBIN, 
			   int method=EVENTTPC_DEFAULT_RECO_METHOD);  

  TH3D *Get3DFrame(int rebin_space, int rebin_time) const; //frame for plotting 3D reconstruction
  
  TH3D *Get3D(const SigClusterTPC &cluster, double radius,                       // clustered hits only, 3D view
	      int rebin_space=EVENTTPC_DEFAULT_STRIP_REBIN, 
	      int rebin_time=EVENTTPC_DEFAULT_TIME_REBIN, 
	      int method=EVENTTPC_DEFAULT_RECO_METHOD);  

  TH2D *GetXY_TestUV(TH2D *h=NULL); // auxillary functions for x-check 
  TH2D *GetXY_TestVW(TH2D *h=NULL); // auxillary functions for x-check 
  TH2D *GetXY_TestWU(TH2D *h=NULL); // auxillary functions for x-check 
    
};
#endif
