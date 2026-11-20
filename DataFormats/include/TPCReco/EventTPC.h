#ifndef __EVENTTPC_H__
#define __EVENTTPC_H__

#include <ostream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <memory>

#include <boost/property_tree/ptree.hpp>

#include <TH1D.h>
#include <TH2D.h>
#include <TH3D.h>

#include "TPCReco/EventInfo.h"
#include "TPCReco/GeometryTPC.h"
#include "TPCReco/PEventTPC.h"

class EventTPC {
  
 public:
  EventTPC();

  ~EventTPC();

  void Clear();

  void SetChargeMap(const PEventTPC::chargeMapType & aChargeMap);
  void SetEventInfo(const eventraw::EventInfo & aEvInfo);
  void SetGeoPtr(std::shared_ptr<GeometryTPC> aPtr);

  void setHitFilterConfig(filter_type filterType, const boost::property_tree::ptree &config);

  // valid range [0-2][X-Y][1-1024][0-511] 
  double GetValByStrip(int strip_dir, int strip_section, int strip_number, int time_cell) const;

  // time_cell valid range [0-511]
  double GetValByStrip(std::shared_ptr<StripTPC> strip, int time_cell) const;

  // valid range [0-2][1-1024][0-511] (all sections)
  double GetValByStripMerged(int strip_dir, int strip_number, int time_cell);

  // keys (STRIP_DIR, SECTION, STRIP_NUM, TIME_CELL) of the hits passing a given filter.
  // The threshold and fraction filters add an envelope of neighbouring cells, which
  // can contain keys without a charge entry (GetValByStrip returns 0 for them).
  const std::set<PEventTPC::chargeMapType::key_type> & GetFilteredKeys(filter_type filterType);

  const eventraw::EventInfo & GetEventInfo() const { return myEventInfo; };
  inline GeometryTPC * GetGeoPtr() const { return myGeometryPtr.get(); }

  std::shared_ptr<TH1D> get1DProjection(definitions::projection_type projType,
					filter_type filterType,
					scale_type scaleType);

  std::shared_ptr<TH2D> get2DProjection(definitions::projection_type projType,
					filter_type filterType,
					scale_type scaleType);

  // maximal charge from strips of a given direction
  double GetMaxCharge(int strip_dir=-1, int strip_section=-1, int strip_number=-1,
		      filter_type filterType=filter_type::none);   

  // total charge from strips of a given direction
  double GetTotalCharge(int strip_dir=-1, int strip_section=-1, int strip_number=-1,
			int time_cell=-1, filter_type filterType=filter_type::none);

  // arrival time and strip number of the maximal charge from strips of a given direction
  // or arrival time of the maximal charge from all strips
  std::tuple<int,int> GetMaxChargePos(int aStrip_dir=-1, filter_type filterType=filter_type::none);

  // Number of hit or strips in a given direction, section, strip number.
  // If countHits==true individial hits (strip, time bin) are counted
  long GetMultiplicity(bool countHits,
		       int strip_dir, int strip_section, int strip_number,
		       filter_type filterType);

  // Min time,max time, min strip, max strip in a given direction (all sections)
  std::tuple<int,int,int,int> GetSignalRange(int aStrip_dir, filter_type filterType);

   // global channel number with the maximal charge from all strips
  int GetMaxChargeChannel() const;

  // valid range [0-1][0-3]
  std::shared_ptr<TH2D> GetChannels(int cobo_idx, int asad_idx);

  // valid range [0-1][0-3]
  std::shared_ptr<TH2D> GetChannels_raw(int cobo_idx, int asad_idx); 

  private:

  void filterHits(filter_type filterType);

  void addEnvelope(PEventTPC::chargeMapType::key_type key,
		   std::set<PEventTPC::chargeMapType::key_type> & keyList,
		   int delta_strips,
		   int delta_timecells);

  void create3DHistoTemplate();

  // Whole-event properties without filter (max charge, total charge, number of hits), computed in one
  // pass over the charge map by SetEventInfo. Building the 3D histogram cache for them cost ~40% of the
  // CPU of reading raw data (every event, also those rejected by the event filter).
  void computeWholeEventProperties();
  bool wholeEventPropsValid{false}; //!
  double wholeEventMaxCharge{0};    //! max over (dir, strip, time cell) of the charge summed over sections (>= 0)
  double wholeEventTotalCharge{0};  //!
  long wholeEventHits{0};           //! number of distinct (dir, strip, time cell), sections merged
  
  void updateHistosCache(filter_type filterType);
  
  void scale1DHistoToMM(TH1D *h1D, definitions::projection_type projType) const;
  
  void scale2DHistoToMM(TH2D *h2D, definitions::projection_type projType) const;
  
  void setHistoLabels(TH1 *h1,
		      definitions::projection_type projType,
		      filter_type filterType,
		      scale_type scaleType) const;

  std::map<filter_type, bool> histoCacheUpdated = {{filter_type::none, false},
						   {filter_type::threshold, false},
						   {filter_type::island, false},
						   {filter_type::fraction, false}};
  
  std::map<filter_type, std::shared_ptr<TH3D> > a3DHistoRawMap;
  std::shared_ptr<TH3D> a3DHistoRawPtr;
  
  eventraw::EventInfo myEventInfo;
  std::shared_ptr<GeometryTPC> myGeometryPtr;  

  // key=(STRIP_DIR [0-2], SECTION [0-2], STRIP_NUM [1-1024], TIME_CELL [0-511])
  PEventTPC::chargeMapType chargeMapWithSections;

  std::map<filter_type, std::set<PEventTPC::chargeMapType::key_type> > keyLists;

  std::map<filter_type, boost::property_tree::ptree> filterConfigs;

  friend std::ostream& operator<<(std::ostream& os, const EventTPC& e);
 
};
#endif
