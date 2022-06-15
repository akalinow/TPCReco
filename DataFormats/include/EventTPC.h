#ifndef __EVENTTPC_H__
#define __EVENTTPC_H__

#include <cstdlib>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <memory>

#include "TH1D.h"
#include "TH2D.h"
#include "TH3F.h"

#include "EventInfo.h"
#include "GeometryTPC.h"
#include "SigClusterTPC.h"
#include "PEventTPC.h"

#define EVENTTPC_DEFAULT_RECO_METHOD 1  // 0 = equal charge division along the strip
                                        // 1 = weighted charge division from complementary strip directions
#define EVENTTPC_DEFAULT_STRIP_REBIN 2  // number of strips to rebin [1-1024] 
#define EVENTTPC_DEFAULT_TIME_REBIN  5  // number of time cells to rebin [1-512]

class EventTPC {

 private:

  void fillAuxMaps();
  void updateMaxChargeMaps(const PEventTPC::chargeMapType::key_type & key,
			   double value);
  void addEnvelope(PEventTPC::chargeMapType::key_type key,
		   std::set<PEventTPC::chargeMapType::key_type> & keyList,
		   double delta_timece,
		   double delta_strip);

  std::shared_ptr<TH1D> getEmpty1DHisto(projection_type aProj, scale_type scaleType) const;

  std::map<filter_type, std::set<PEventTPC::chargeMapType::key_type> > keyLists;

  void filterHits(filter_type filterType);

  double get1DPosition(PEventTPC::chargeMapType::key_type key,
		       projection_type projType, scale_type scaleType) const;

  void create3DHistos(scale_type scaleType);
  void updateHistosCache(filter_type filterType);

  bool histoCacheUpdated{false};
  std::shared_ptr<TH3D> a3DHistoRawPtr, a3DHistoMMPtr;
  
  eventraw::EventInfo myEventInfo;
  std::shared_ptr<GeometryTPC> myGeometryPtr;  //! transient data member
  SigClusterTPC myCluster;                     //! transient data member
  
  std::map<MultiKey3, double> chargeMap; // key=(STRIP_DIR [0-2], STRIP_NUM [1-1024], TIME_CELL [0-511])
  std::map<MultiKey4, double> chargeMap2; // key=(STRIP_DIR [0-2], SECTION [0-2], STRIP_NUM [1-1024], TIME_CELL [0-511])

  PEventTPC::chargeMapType chargeMapWithSections; // key=(STRIP_DIR [0-2], SECTION [0-2], STRIP_NUM [1-1024], TIME_CELL [0-511])
  std::map<MultiKey2, double> maxChargeMap; // key=(STRIP_DIR [0-2], STRIP_NUM [1-1024])
  std::map<MultiKey3, double> maxChargeMap2; // key=(STRIP_DIR [0-2], SECTION [0-2], STRIP_NUM [1-1024])
  std::map<MultiKey2, double> totalChargeMap; // key=(STRIP_DIR [0-2], STRIP_NUM [1-1024])
  std::map<MultiKey2, double> totalChargeMap2; // key=(STRIP_DIR [0-2], TIME_CELL [0-511])
  std::map<int, double> totalChargeMap3; // key=TIME_CELL [0-511]
  std::map<MultiKey3, double> totalChargeMap4; // key=(STRIP_DIR [0-2], SECTION [0-2], STRIP_NUM [1-1024])
  std::map<MultiKey3, double> totalChargeMap5; // key=(STRIP_DIR [0-2], SECTION [0-2], TIME_CELL [0-511])
  std::map<MultiKey2, int> asadMap; // key=(COBO_id, ASAD_id [0-3])
    
  bool initOK;      // is geometry valid?
  int time_rebin;   // how many raw data time bins to merge (default=1, i.e. none)

  std::vector<double> max_charge;       // maximal value from each strip direction [0-2]
  std::vector<int> max_charge_timing;   // range [0-511], RAW time cells
  std::vector<int> max_charge_strip;    // range [1-1024]
  double glb_max_charge;
  int glb_max_charge_timing;  // range [0-511]
  int glb_max_charge_channel; // range [0-1023]
  std::vector<double> tot_charge;       // integral per strip direction
  double glb_tot_charge;      // integral per event

 public:
  EventTPC();

  ~EventTPC(){};

  void Clear();

  void SetChargeMap(const PEventTPC::chargeMapType & aChargeMap);
  void SetGeoPtr(std::shared_ptr<GeometryTPC> aPtr);  
  void SetEventInfo(const decltype(myEventInfo)& aEvInfo) {myEventInfo = aEvInfo; };

  bool CheckAsadNboards() const ; // verifies that all AsAd boards are present in this event

  double GetValByStrip(int strip_dir, int strip_section, int strip_number, int time_cell) const; // valid range [0-2][X-Y][1-1024][0-511] 
  double GetValByStrip(std::shared_ptr<StripTPC> strip, int time_cell) const;                   // valid range [0-511]
  double GetValByStripMerged(int strip_dir, int strip_number, int time_cell) const;  // valid range [0-2][1-1024][0-511] (all sections)

  const decltype(myEventInfo)& GetEventInfo() const { return myEventInfo; };
  inline GeometryTPC * GetGeoPtr() const { return myGeometryPtr.get(); }
  inline bool IsOK() const { return initOK; }
  inline int GetTimeRebin() const { return time_rebin; }       

  double GetMaxCharge() const;             // maximal charge from all strips
  double GetMaxCharge(int strip_dir) const;      // maximal charge from strips of a given direction
  double GetMaxCharge(int strip_dir, int strip_number) const;      // maximal charge from merged strip of a given direction (all sections)
  double GetMaxCharge(int strip_dir, int strip_section, int strip_number) const;      // maximal charge from single strip of a given direction (per section)
  int GetMaxChargeTime(int strip_dir) const;     // arrival time of the maximal charge from strips of a given direction
  int GetMaxChargeStrip(int strip_dir) const;    // strip number with the maximal charge in a given direction 
  int GetMaxChargeTime() const;                  // arrival time of the maximal charge from all strips
  int GetMaxChargeChannel() const;               // global channel number with the maximal charge from all strips
  double GetTotalCharge() const;           // charge integral from all strips
  double GetTotalCharge(int strip_dir) const;    // charge integral from strips of a given direction 
  double GetTotalCharge(int strip_dir, int strip_number) const; // charge integral from merged strip of a given direction (all sections) 
  double GetTotalCharge(int strip_dir, int strip_section, int strip_number) const; // charge integral from single strip of a given direction (per section) 
  double GetTotalChargeByTimeCell(int time_cell) const; // charge integral from a single time cell from all strips
  double GetTotalChargeByTimeCell(int strip_dir, int time_cell) const; // charge integral from a single time cell from all merged strips in a given direction (all sections)
  double GetTotalChargeByTimeCell(int strip_dir, int strip_section, int time_cell) const; // charge integral from a single time cell from all strips in a given direction (per section)
  
  std::shared_ptr<TH2D> GetStripVsTime(const SigClusterTPC &cluster, int strip_dir);        // clustered hits only, valid dir range [0-2]
  std::shared_ptr<TH2D> GetStripVsTime(int strip_dir);                               // whole event, all strip dirs
  std::shared_ptr<TH2D> GetStripVsTimeInMM(const SigClusterTPC &cluster, int strip_dir);  // valid range [0-2]
  std::shared_ptr<TH2D> GetStripVsTimeInMM(int strip_dir);  // whole event, valid range [0-2]
  std::shared_ptr<TH2D> GetChannels(int cobo_idx, int asad_idx); // valid range [0-1][0-3]
  std::shared_ptr<TH2D> GetChannels_raw(int cobo_idx, int asad_idx); // valid range [0-1][0-3]

  std::shared_ptr<TH1D> get1DProjection(projection_type projType,
					filter_type filterType,
					scale_type scaleType);

  std::vector<TH2D*> Get2D(const SigClusterTPC &cluster, double radius,          // clustered hits only,
			   int rebin_space=EVENTTPC_DEFAULT_STRIP_REBIN,   // projections on: XY, XZ, YZ planes
			   int rebin_time=EVENTTPC_DEFAULT_TIME_REBIN, 
			   int method=EVENTTPC_DEFAULT_RECO_METHOD);  

  TH3D *Get3DFrame(int rebin_space, int rebin_time) const; //frame for plotting 3D reconstruction
  
  TH3D *Get3D(const SigClusterTPC &cluster, double radius,                       // clustered hits only, 3D view
	      int rebin_space=EVENTTPC_DEFAULT_STRIP_REBIN, 
	      int rebin_time=EVENTTPC_DEFAULT_TIME_REBIN, 
	      int method=EVENTTPC_DEFAULT_RECO_METHOD);  

  ///Methods to remove
  void MakeOneCluster(double thr=-1, int delta_strips=5, int delta_timecells=25){} 
  const SigClusterTPC & GetOneCluster() const {return myCluster;}

  std::shared_ptr<TH1D> GetStripProjection(const SigClusterTPC&, int&) { return std::shared_ptr<TH1D>();};
  TH1D* GetStripProjection(int&) { return 0;};//TEMP FIX
  std::shared_ptr<TH1D> GetStripProjectionInMM(const SigClusterTPC &cluster, int strip_dir) {return std::shared_ptr<TH1D>();};
  std::shared_ptr<TH1D> GetStripProjectionInMM(int strip_dir) {return std::shared_ptr<TH1D>();};
  
  TH1D *GetTimeProjection(const SigClusterTPC &cluster, int strip_dir){return 0;};
  TH1D *GetTimeProjection(const SigClusterTPC &cluster){return 0;};                
  TH1D *GetTimeProjection(int strip_dir){return 0;};                             
  TH1D *GetTimeProjection(){return 0;};                                          

  std::shared_ptr<TH1D> GetTimeProjectionInMM(const SigClusterTPC &cluster, int strip_dir){ return std::shared_ptr<TH1D>();}; 
  std::shared_ptr<TH1D> GetTimeProjectionInMM(const SigClusterTPC &cluster){ return std::shared_ptr<TH1D>();};                
  std::shared_ptr<TH1D> GetTimeProjectionInMM(int strip_dir){ return std::shared_ptr<TH1D>();};                             
  std::shared_ptr<TH1D> GetTimeProjectionInMM(){ return std::shared_ptr<TH1D>();};          
  /////

  friend std::ostream& operator<<(std::ostream& os, const EventTPC& e);
};
#endif
