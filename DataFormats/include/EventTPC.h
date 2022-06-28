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
#include "PEventTPC.h"

class EventTPC {

 private:

  void fillMergedSectionsMap();
  void fillAuxMaps(const std::set<PEventTPC::chargeMapType::key_type> & keyLists);
  void updateMaxChargeMaps(const PEventTPC::chargeMapType::key_type & key,
			   double value);
  void addEnvelope(PEventTPC::chargeMapType::key_type key,
		   std::set<PEventTPC::chargeMapType::key_type> & keyList,
		   double delta_timece,
		   double delta_strip);

  std::map<filter_type, std::set<PEventTPC::chargeMapType::key_type> > keyLists;

  void filterHits(filter_type filterType);
  void create3DHistoTemplate();
  void updateHistosCache(filter_type filterType);
  void scale1DHistoToMM(TH1D *h1D, projection_type projType) const;
  void scale2DHistoToMM(TH2D *h2D, projection_type projType) const;
  void setHistoLabels(TH1 *h1,
		      projection_type projType,
		      filter_type filterType,
		      scale_type scaleType) const;

  std::map<filter_type, bool> histoCacheUpdated = {{filter_type::none, false},
						   {filter_type::threshold, false},
						   {filter_type::island, false}};
  std::map<filter_type, std::shared_ptr<TH3D> > a3DHistoRawMap;
  std::shared_ptr<TH3D> a3DHistoRawPtr;
  
  eventraw::EventInfo myEventInfo;
  std::shared_ptr<GeometryTPC> myGeometryPtr;  //! transient data member
  
  PEventTPC::chargeMapType chargeMapWithSections; // key=(STRIP_DIR [0-2], SECTION [0-2], STRIP_NUM [1-1024], TIME_CELL [0-511])
  std::map<MultiKey3, double> chargeMapMergedSections; // key=(STRIP_DIR [0-2], STRIP_NUM [1-1024], TIME_CELL [0-511])

  
  std::map<MultiKey2, int> asadMap; // key=(COBO_id, ASAD_id [0-3])    
   
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

  const eventraw::EventInfo & GetEventInfo() const { return myEventInfo; };
  inline GeometryTPC * GetGeoPtr() const { return myGeometryPtr.get(); }

  // maximal charge from strips of a given direction
  double GetMaxCharge(int strip_dir=-1, int strip_section=-1, int strip_number=-1,
		      filter_type filterType=filter_type::none);   

  // total charge from strips of a given direction
  double GetTotalCharge(int strip_dir=-1, int strip_section=-1, int strip_number=-1,
			int time_cell=-1, filter_type filterType=filter_type::none);

  // arrival time and strip number of the maximal charge from strips of a given direction
  // or arrival time of the maximal charge from all strips
  std::tuple<int,int> GetMaxChargePos(int aStrip_dir=-1, filter_type filterType=filter_type::none);
  
  int GetMaxChargeChannel() const;               // global channel number with the maximal charge from all strips
  std::shared_ptr<TH2D> GetChannels(int cobo_idx, int asad_idx); // valid range [0-1][0-3]
  std::shared_ptr<TH2D> GetChannels_raw(int cobo_idx, int asad_idx); // valid range [0-1][0-3]
  
  std::shared_ptr<TH1D> get1DProjection(projection_type projType,
					filter_type filterType,
					scale_type scaleType);

  std::shared_ptr<TH2D> get2DProjection(projection_type projType,
					filter_type filterType,
					scale_type scaleType);

  // Number of hit or strips in a given direction, section, strip number.
  // Hits arou counted if countHits==true
  long GetMultiplicity(bool countHits,
		       int strip_dir, int strip_section, int strip_number,
		       filter_type filterType);

  // Min time,max time, min strip, max strip in a given direction (all sections)
  std::tuple<int,int,int,int> GetSignalRange(int aStrip_dir, filter_type filterType);

  friend std::ostream& operator<<(std::ostream& os, const EventTPC& e);
 
};
#endif
