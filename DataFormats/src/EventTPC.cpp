#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm> 

#include "TH1D.h"
#include "TH2D.h"
#include "TH3F.h"

#include "EventTPC.h"
#include "TrackSegmentTPC.h"

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
EventTPC::EventTPC(){

  int n_directions = 3;
  max_charge.resize(n_directions);
  max_charge_timing.resize(n_directions);
  max_charge_strip.resize(n_directions);
  tot_charge.resize(n_directions);

  Clear();
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::Clear(){

  SetGeoPtr(0);
  initOK = false;

  glb_max_charge = 0.0;
  glb_max_charge_timing = -1;
  glb_max_charge_channel = -1;
  glb_tot_charge = 0.0;

  for(int idir=DIR_U; idir<DIR_W; ++idir) {
    max_charge.at(idir)=0.0;
    max_charge_timing.at(idir)=-1;
    max_charge_strip.at(idir)=-1;
    tot_charge.at(idir)=0.0;   
  }  
  totalChargeMap.clear();  // 2-key map: strip_dir, strip_number
  totalChargeMap2.clear();  // 2-key map: strip_dir, time_cell
  totalChargeMap3.clear();  // 1-key map: time_cell
  totalChargeMap4.clear();  // 3-key map: strip_dir, strip_section, strip_number
  totalChargeMap5.clear();  // 3-key map: strip_dir, strip_section, time_cell
  maxChargeMap.clear();    // 2-key map: strip_dir, strip_number
  maxChargeMap2.clear();   // 3-key map: strip_dir, strip_section, strip_number 
  chargeMap.clear();
  chargeMapWithSections.clear();
  asadMap.clear();
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::SetGeoPtr(std::shared_ptr<GeometryTPC> aPtr) {
  myGeometryPtr = aPtr;
  initOK = myGeometryPtr && myGeometryPtr->IsOK();
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::SetChargeMap(const PEventTPC::chargeMapType & aChargeMap){

  chargeMapWithSections = aChargeMap;

}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::fillAuxMaps(){

  for(auto const & item: chargeMapWithSections){
    auto key = item.first;
    auto value = item.second;

    const int strip_dir = std::get<0>(key);
    const int strip_sec = std::get<1>(key);
    const int strip_num = std::get<2>(key);
    const int time_cell = std::get<3>(key);

    MultiKey2 mkey_total(strip_dir, strip_num);
    MultiKey2 mkey_total2(strip_dir, time_cell);
    MultiKey3 mkey_total4(strip_dir, strip_sec, strip_num);
    MultiKey3 mkey_total5(strip_dir, strip_sec, time_cell);
    MultiKey2 mkey_maxval(strip_dir, strip_num);
    MultiKey3 mkey_maxval2(strip_dir, strip_sec, strip_num);

    glb_tot_charge += value;
    tot_charge[strip_dir] += value;
    totalChargeMap[mkey_total] += value;
    totalChargeMap2[mkey_total2] += value;
    totalChargeMap3[time_cell] += value;
    totalChargeMap4[mkey_total4] += value;
    totalChargeMap5[mkey_total5] += value;
    maxChargeMap[mkey_maxval] = value>maxChargeMap[mkey_maxval]? value : maxChargeMap[mkey_maxval];
    maxChargeMap2[mkey_maxval2] = value>maxChargeMap2[mkey_maxval2]? value : maxChargeMap2[mkey_maxval2];
    updateMaxChargeMaps(key, value);
   
    auto strip=myGeometryPtr->GetStripByDir(strip_dir, strip_sec, strip_num);
    if(strip) asadMap[MultiKey2(strip->CoboId(), strip->AsadId())] += 1;
  }
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::updateMaxChargeMaps(const PEventTPC::chargeMapType::key_type & key,
				   double value){

  const int strip_dir = std::get<0>(key);
  const int strip_sec = std::get<1>(key);
  const int strip_num = std::get<2>(key);
  const int time_cell = std::get<3>(key);
  
  if(value > max_charge[strip_dir] ) { 
    max_charge[strip_dir] = value;
    max_charge_timing[strip_dir] = time_cell;
    max_charge_strip[strip_dir] = strip_num;
    if(value > glb_max_charge ) {
      glb_max_charge = value;
      glb_max_charge_timing = time_cell;
      glb_max_charge_channel = myGeometryPtr->Global_strip2normal(strip_dir, strip_sec, strip_num);
    }
  }  
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
bool EventTPC::CheckAsadNboards() const {
  return IsOK() && myGeometryPtr->GetAsadNboards()==(int)asadMap.size();
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetValByStrip(int strip_dir, int strip_section, int strip_number, int time_cell) const {
  return chargeMapWithSections.at({strip_dir,  strip_section, strip_number, time_cell});
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetValByStrip(std::shared_ptr<StripTPC> strip, int time_cell) const {
  if(strip) return GetValByStrip(strip->Dir(), strip->Section(), strip->Num(), time_cell);
  return 0.0;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetValByStripMerged(int strip_dir, int strip_number, int time_cell) const{
  return chargeMap.at({strip_dir, strip_number, time_cell});
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetMaxCharge(int strip_dir, int strip_section, int strip_number) const {
  return maxChargeMap2.at({strip_dir, strip_section, strip_number});
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetMaxCharge(int strip_dir, int strip_number) const { 
  return maxChargeMap.at({strip_dir, strip_number});
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetMaxCharge(int strip_dir) const { 
  return max_charge.at(strip_dir);
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetMaxCharge() const { 
  if(!IsOK()) return 0.0;
  return glb_max_charge;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
int EventTPC::GetMaxChargeTime(int strip_dir) const {
  return max_charge_timing.at(strip_dir);
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
int EventTPC::GetMaxChargeStrip(int strip_dir) const {
  return max_charge_strip.at(strip_dir);
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
int EventTPC::GetMaxChargeTime() const {
  if(!IsOK()) return ERROR;
  return glb_max_charge_timing;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
int EventTPC::GetMaxChargeChannel() const { 
  if(!IsOK()) return ERROR;
  return glb_max_charge_channel;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetTotalCharge(int strip_dir, int strip_section, int strip_number) const {
  return totalChargeMap4.at({strip_dir, strip_section, strip_number});
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetTotalCharge(int strip_dir, int strip_number) const {
  return totalChargeMap.at({strip_dir, strip_number});
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetTotalCharge(int strip_dir) const{
  return tot_charge.at(strip_dir);
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetTotalCharge() const {
  if(!IsOK()) return 0.0;
  return glb_tot_charge;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetTotalChargeByTimeCell(int strip_dir, int strip_section, int time_cell) const {
  return totalChargeMap5.at({strip_dir, strip_section, time_cell});
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetTotalChargeByTimeCell(int strip_dir, int time_cell) const {
  return totalChargeMap2.at({strip_dir, time_cell});
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
double EventTPC::GetTotalChargeByTimeCell(int time_cell) const {
  return totalChargeMap3.at({time_cell});
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::FilterHits(const std::string method){

  double chargeThreshold = 35.0;
  int delta_timecells = 3;
  int delta_strips = 3;
  
  std::set<PEventTPC::chargeMapType::key_type> keyList;
  
  for(const auto & item: chargeMapWithSections){
    auto key = item.first;
    auto value = item.second;
    if(value>chargeThreshold){
      keyList.insert(key);
      addEnvelope(key, keyList, delta_timecells, delta_strips);
    }
  }
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void EventTPC::addEnvelope(PEventTPC::chargeMapType::key_type key,
			   std::set<PEventTPC::chargeMapType::key_type> & keyList,
			   double delta_timecells,
			   double delta_strips){

  int iDir = std::get<0>(key);
  int iSection = std::get<1>(key);

  for(int iCell=std::get<3>(key)-delta_timecells;
      iCell<=std::get<3>(key)+delta_timecells;++iCell){
    for(int iStrip=std::get<2>(key)-delta_strips;
	iStrip<=std::get<2>(key)+delta_strips;++iStrip){
      auto neighbourKey = std::make_tuple(iDir, iSection, iStrip, iCell);
      keyList.insert(neighbourKey);
    }
  }
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
std::shared_ptr<TH1D> EventTPC::GetStripProjection(projection projType,
						   filter_type filterType,
						   scale_type scaleType){

  bool err_flag = false;
  auto aHisto=std::shared_ptr<TH1D>();
  aHisto.reset(getHisto(projType, scaleType));
  
  auto event_id = myEventInfo.GetEventId();

  double position = 0.0;
  for(const auto & key: keyLists.at(filterType)){
    if(std::get<0>(key)!=strip_dir) continue;

    if(scale==scale_type::raw) position = std::get<0>(key);
    else if(scale==scale_type::mm) position = myGeometryPtr->Strip2posUVW(std::get<0>(key), std::get<2>(key), err_flag);
    
    aHisto->Fill(position, chargeMapWithSections.at(key));
  }
  return aHisto; 
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
TH1* EventTPC::getHisto(projection projType, scale_type scaleType){

  TH1* aHisto = 0;
  /*
  
      h = new TH1D( Form("hclust_%spro_evt%d", myGeometryPtr->GetDirName(strip_dir), event_id),
		    Form("Event-%d: Clustered hits from %s strips integrated over time;%s strip no.;Charge/strip [arb.u.]", 
			 event_id, myGeometryPtr->GetDirName(strip_dir), myGeometryPtr->GetDirName(strip_dir)),
		    myGeometryPtr->GetDirNstrips(strip_dir),
		    1.0 - 0.5,
		    1.*myGeometryPtr->GetDirNstrips(strip_dir)+0.5 ));

std::tie(minStripInMM, maxStripInMM) = myGeometryPtr->rangeStripDirInMM(strip_dir); 
 h= new TH1D( Form("hclust_%spro_mm_evt%d", myGeometryPtr->GetDirName(strip_dir), event_id),
	      Form("Event-%d: Clustered hits from %s strips integrated over time;%s strip direction [mm];Charge/bin [arb.u.]",
		   event_id, myGeometryPtr->GetDirName(strip_dir), myGeometryPtr->GetDirName(strip_dir)),
	      myGeometryPtr->GetDirNStripsMerged(strip_dir), minStripInMM, maxStripInMM));
  */
  return aHisto;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
TH1D *EventTPC::GetTimeProjection(const SigClusterTPC &cluster, int strip_dir) {  // valid range [0-2]
  TH1D *h = NULL;
  if(!IsOK() || !cluster.IsOK()) return h;
  auto event_id = myEventInfo.GetEventId();
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: {
    if(cluster.GetNhits(strip_dir)<1) break;
    h = new TH1D( Form("hclust_%stime_evt%d", myGeometryPtr->GetDirName(strip_dir), event_id),
		  Form("Event-%d: Clustered hits from %s strips;Time bin [arb.u.];Charge/bin [arb.u.]", 
		       event_id, myGeometryPtr->GetDirName(strip_dir)),
		  myGeometryPtr->GetAgetNtimecells(),
		  0.0-0.5, 
		  1.*myGeometryPtr->GetAgetNtimecells()-0.5 ); // ends at 511.5 (cells numbered from 0 to 511)
    // fill new histogram
    if(h) {
      //bool res;
      for(int strip_num=cluster.GetMinStrip(strip_dir); strip_num<=cluster.GetMaxStrip(strip_dir); strip_num++) {
	for(int icell=cluster.GetMinTime(strip_dir); icell<=cluster.GetMaxTime(strip_dir); icell++) {
	  if( cluster.CheckByStripMerged(strip_dir, strip_num, icell) ) {
	    h->Fill(1.*icell, GetValByStripMerged(strip_dir, strip_num, icell/*, res*/));
	  }
	}
      }
    }
  }
  };
  return h;
}

TH1D *EventTPC::GetTimeProjection(const SigClusterTPC &cluster) {  // all strips
  TH1D *h = NULL;
  auto event_id = myEventInfo.GetEventId();
  if(!IsOK() || !cluster.IsOK() || cluster.GetNhits()==0 ) return h;
  h = new TH1D( Form("hclust_time_evt%d", event_id),
	        Form("Event-%d: Clustered hits from all strips;Time bin [arb.u.];Charge/bin [arb.u.]", event_id), 
		myGeometryPtr->GetAgetNtimecells(),
		0.0-0.5, 
		1.*myGeometryPtr->GetAgetNtimecells()-0.5 ); // ends at 511.5 (cells numbered from 0 to 511)
  // fill new histogram
  if(h) {
    //bool res;
    for(int strip_dir=0; strip_dir<3; strip_dir++) {
      if( cluster.GetMultiplicity(strip_dir)<1 ) continue;
      for(int strip_num=cluster.GetMinStrip(strip_dir); strip_num<=cluster.GetMaxStrip(strip_dir); strip_num++) {
	for(int icell=cluster.GetMinTime(strip_dir); icell<=cluster.GetMaxTime(strip_dir); icell++) {
	  if( cluster.CheckByStripMerged(strip_dir, strip_num, icell) ) {
	    h->Fill(1.*icell, GetValByStripMerged(strip_dir, strip_num, icell/*, res*/));
	  }
	}
      }
    }
  }
  return h;
}

TH1D *EventTPC::GetTimeProjection(int strip_dir) {  // whole event, valid range [0-2]
  TH1D *h = nullptr;
  if(!IsOK()) return h;
  auto event_id = myEventInfo.GetEventId();
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: {
    h = new TH1D( Form("hraw_%stime_evt%d", myGeometryPtr->GetDirName(strip_dir), event_id), 
                  Form("Event-%d: Raw signals from %s strips;Time bin [arb.u.];Charge/bin [arb.u.]", 
                       event_id, myGeometryPtr->GetDirName(strip_dir)),
                  myGeometryPtr->GetAgetNtimecells(),
                  1.0 - 0.5,
                  1.*myGeometryPtr->GetAgetNtimecells()-0.5 );
    // fill new histogram
    if(h) {
      //bool res;
      for(int strip_num=1; strip_num<=myGeometryPtr->GetDirNstrips(strip_dir); strip_num++) {
        for(int icell=0; icell<myGeometryPtr->GetAgetNtimecells(); icell++) {
          double val = GetValByStripMerged(strip_dir, strip_num, icell);
	  //          if(val) h->Fill(1.*icell, val);
	  h->Fill(1.*icell, val);
        }
      }
    }
  }
  };
  return h;
}

TH1D *EventTPC::GetTimeProjection() {  // whole event, all strips
  TH1D *h = NULL;
  auto event_id = myEventInfo.GetEventId();
  if(!IsOK()) return h;
  h = new TH1D( Form("hraw_time_evt%d", event_id),
	        Form("Event-%d: Raw signals from all strips;Time bin [arb.u.];Charge/bin [arb.u.]", event_id), 
		myGeometryPtr->GetAgetNtimecells(),
		0.0-0.5, 
		1.*myGeometryPtr->GetAgetNtimecells()-0.5 ); // ends at 511.5 (cells numbered from 0 to 511)
  // fill new histogram
  if(h) {
    int glb_channel_idx=0;
    for(int icobo=0; icobo<myGeometryPtr->GetCoboNboards(); icobo++) {
      for(int iasad=0; iasad<myGeometryPtr->GetAsadNboards(icobo); iasad++) {
	for(int ichan=0; ichan<myGeometryPtr->GetAgetNchannels()*myGeometryPtr->GetAgetNchips(); ichan++) {
	  ++glb_channel_idx;
	  for(int icell=0; icell<=myGeometryPtr->GetAgetNtimecells(); icell++) {
	    double val = GetValByStrip(myGeometryPtr->GetStripByGlobal(glb_channel_idx), icell);
	    h->Fill(1.*icell, val); 
	  }
	}
      }
    }
  }
  return h;
}

std::shared_ptr<TH1D> EventTPC::GetTimeProjectionInMM(const SigClusterTPC &cluster, int strip_dir) {  // valid range [0-2]

  if(!IsOK()) return std::shared_ptr<TH1D>();
  bool err_flag;
  double zmin=0.0-0.5;  // time_cell_min;
  double zmax=myGeometryPtr->GetAgetNtimecells()-0.5; // time_cell_max;  
  double minTimeInMM = myGeometryPtr->Timecell2pos(zmin, err_flag);
  double maxTimeInMM = myGeometryPtr->Timecell2pos(zmax, err_flag);
  auto event_id = myEventInfo.GetEventId();
  std::shared_ptr<TH1D> result(new TH1D( Form("hclust_%stime_mm_evt%d", myGeometryPtr->GetDirName(strip_dir), event_id),
					 Form("Event-%d: Clustered hits from %s strips;Drift direction [mm];Charge/bin [arb.u.]",
					      event_id, myGeometryPtr->GetDirName(strip_dir)),
					 myGeometryPtr->GetAgetNtimecells(), minTimeInMM, maxTimeInMM));
  
  // fill new histogram
  double x = 0.0;
  for(int strip_num=cluster.GetMinStrip(strip_dir); strip_num<=cluster.GetMaxStrip(strip_dir); strip_num++) {
    for(int icell=cluster.GetMinTime(strip_dir); icell<=cluster.GetMaxTime(strip_dir); icell++) {
      if( cluster.CheckByStripMerged(strip_dir, strip_num, icell) ) {
	x = myGeometryPtr->Timecell2pos(icell, err_flag);
	if(err_flag) continue;
	double val = GetValByStripMerged(strip_dir, strip_num, icell);
	result->Fill(x, val);
      }
    }
  }
  return result;
}

std::shared_ptr<TH1D> EventTPC::GetTimeProjectionInMM(const SigClusterTPC &cluster) {  // all strips

  if(!IsOK()) return std::shared_ptr<TH1D>();
  bool err_flag;
  double zmin=0.0-0.5;  // time_cell_min;
  double zmax=myGeometryPtr->GetAgetNtimecells()-0.5; // time_cell_max;  
  double minTimeInMM = myGeometryPtr->Timecell2pos(zmin, err_flag);
  double maxTimeInMM = myGeometryPtr->Timecell2pos(zmax, err_flag);
  auto event_id = myEventInfo.GetEventId();
  std::shared_ptr<TH1D> result(new TH1D( Form("hclust_time_mm_evt%d", event_id),
					 Form("Event-%d: Clustered hits from all strips;Drift direction [mm];Charge/bin [arb.u.]",
					 event_id),
					 myGeometryPtr->GetAgetNtimecells(), minTimeInMM, maxTimeInMM));

  // fill new histogram
  double x = 0.0;
  for(int strip_dir=0; strip_dir<3; strip_dir++) {
    for(int strip_num=cluster.GetMinStrip(strip_dir); strip_num<=cluster.GetMaxStrip(strip_dir); strip_num++) {
      for(int icell=cluster.GetMinTime(strip_dir); icell<=cluster.GetMaxTime(strip_dir); icell++) {
	if( cluster.CheckByStripMerged(strip_dir, strip_num, icell) ) {
	  x = myGeometryPtr->Timecell2pos(icell, err_flag);
	  if(err_flag) continue;
	  double val = GetValByStripMerged(strip_dir, strip_num, icell);
	  result->Fill(x, val);
	}
      }
    }
  }
  return result;
}

std::shared_ptr<TH1D> EventTPC::GetTimeProjectionInMM(int strip_dir) {  // whole event, valid range [0-2]

  if(!IsOK()) return std::shared_ptr<TH1D>();
  bool err_flag;
  auto minStripNum=myGeometryPtr->GetDirMinStripMerged(strip_dir);
  auto maxStripNum=myGeometryPtr->GetDirMaxStripMerged(strip_dir);
  double zmin=0.0-0.5;  // time_cell_min;
  double zmax=myGeometryPtr->GetAgetNtimecells()-0.5; // time_cell_max;  
  double minTimeInMM = myGeometryPtr->Timecell2pos(zmin, err_flag);
  double maxTimeInMM = myGeometryPtr->Timecell2pos(zmax, err_flag);
  auto event_id = myEventInfo.GetEventId();
  std::shared_ptr<TH1D> result(new TH1D( Form("hraw_%stime_mm_evt%d", myGeometryPtr->GetDirName(strip_dir), event_id),
					 Form("Event-%d: Raw signals from %s strips;Drift direction [mm];Charge/bin [arb.u.]",
					 event_id, myGeometryPtr->GetDirName(strip_dir)),
					 myGeometryPtr->GetAgetNtimecells(), minTimeInMM, maxTimeInMM));

  // fill new histogram
  double x = 0.0;
  for(int strip_num=minStripNum; strip_num<=maxStripNum; strip_num++) {
    for(int icell=0; icell<myGeometryPtr->GetAgetNtimecells(); icell++) {
      x = myGeometryPtr->Timecell2pos(icell, err_flag);
      if(err_flag) continue;
      double val = GetValByStripMerged(strip_dir, strip_num, icell);
      //      if(val) result->Fill(x, val);
      result->Fill(x, val);
    }
  }
  return result;
}

std::shared_ptr<TH1D> EventTPC::GetTimeProjectionInMM() {  // whole event, all strips

  if(!IsOK()) return std::shared_ptr<TH1D>();
  bool err_flag;
  double zmin=0.0-0.5;  // time_cell_min;
  double zmax=myGeometryPtr->GetAgetNtimecells()-0.5; // time_cell_max;  
  double minTimeInMM = myGeometryPtr->Timecell2pos(zmin, err_flag);
  double maxTimeInMM = myGeometryPtr->Timecell2pos(zmax, err_flag);
  auto event_id = myEventInfo.GetEventId();
  std::shared_ptr<TH1D> result(new TH1D( Form("hraw_time_mm_evt%d", event_id),
					 Form("Event-%d: Raw signals from all strips;Drift direction [mm];Charge/bin [arb.u.]",
					 event_id),
					 myGeometryPtr->GetAgetNtimecells(), minTimeInMM, maxTimeInMM));

  // fill new histogram
  double x = 0.0;
  for(int strip_dir=0; strip_dir<3; strip_dir++) {
    auto minStripNum=myGeometryPtr->GetDirMinStripMerged(strip_dir);
    auto maxStripNum=myGeometryPtr->GetDirMaxStripMerged(strip_dir);
    for(int strip_num=minStripNum; strip_num<=maxStripNum; strip_num++) {
      for(int icell=0; icell<myGeometryPtr->GetAgetNtimecells(); icell++) {
	x = myGeometryPtr->Timecell2pos(icell, err_flag);
	if(err_flag) continue;
	double val = GetValByStripMerged(strip_dir, strip_num, icell);
	//	if(val) result->Fill(x, val);
	result->Fill(x, val);
      }
    }
  }
  return result;
}

std::shared_ptr<TH2D> EventTPC::GetStripVsTime(const SigClusterTPC &cluster, int strip_dir){  

  if(!IsOK() || !cluster.IsOK()) return 0;
  auto event_id = myEventInfo.GetEventId();
  std::shared_ptr<TH2D>result(new TH2D( Form("hclust_%s_vs_time_evt%d", myGeometryPtr->GetDirName(strip_dir), event_id),
					Form("Event-%d: Clustered hits from %s strips;Time bin [arb.u.];%s strip no.;Charge/bin [arb.u.]",
					     event_id, myGeometryPtr->GetDirName(strip_dir), myGeometryPtr->GetDirName(strip_dir)),
					myGeometryPtr->GetAgetNtimecells(),
					0.0-0.5, 
					1.*myGeometryPtr->GetAgetNtimecells()-0.5, // ends at 511.5 (cells numbered from 0 to 511)
					myGeometryPtr->GetDirNstrips(strip_dir),
					1.0-0.5,
					1.*myGeometryPtr->GetDirNstrips(strip_dir)+0.5 ));
  
  for(int strip_num=cluster.GetMinStrip(strip_dir); strip_num<=cluster.GetMaxStrip(strip_dir); strip_num++) {
    for(int icell=cluster.GetMinTime(strip_dir); icell<=cluster.GetMaxTime(strip_dir); icell++) {
      if( cluster.CheckByStripMerged(strip_dir, strip_num, icell) ) {
	result->Fill(1.*icell, 1.*strip_num, GetValByStripMerged(strip_dir, strip_num, icell));
      }
    }
  }
  
  return result;
}
std::shared_ptr<TH2D> EventTPC::GetStripVsTime(int strip_dir){  // valid range [0-2]

  if(!IsOK()) return std::shared_ptr<TH2D>();
  auto minStripNum=myGeometryPtr->GetDirMinStripMerged(strip_dir);
  auto maxStripNum=myGeometryPtr->GetDirMaxStripMerged(strip_dir);
  auto event_id = myEventInfo.GetEventId();
  std::shared_ptr<TH2D> result(new TH2D( Form("hraw_%s_vs_time_evt%d", myGeometryPtr->GetDirName(strip_dir), event_id),
					 Form("Event-%d: Raw signals from %s strips;Time bin [arb.u.];%s strip no.;Charge/bin [arb.u.]",
					      event_id, myGeometryPtr->GetDirName(strip_dir), myGeometryPtr->GetDirName(strip_dir)),
					 myGeometryPtr->GetAgetNtimecells(),
					 0.0-0.5, 
					 1.*myGeometryPtr->GetAgetNtimecells()-0.5, // ends at 511.5 (cells numbered from 0 to 511)
					 maxStripNum-minStripNum+1,
					 minStripNum-0.5,
					 1*maxStripNum+0.5 ));
  // fill new histogram
  for(int strip_num=minStripNum; strip_num<=maxStripNum; strip_num++) {
    for(int icell=0; icell<myGeometryPtr->GetAgetNtimecells(); icell++) {
      double val = GetValByStripMerged(strip_dir, strip_num, icell);
      //      if(val) result->Fill(1.*icell, 1.*strip_num, val);
      result->Fill(1.*icell, 1.*strip_num, val); 
    }
  }
  return result;
}

std::shared_ptr<TH2D> EventTPC::GetChannels(int cobo_idx, int asad_idx){ // valid range [0-1][0-3]
  //////// DEBUG
  //  std::cout << "GetChannels: cobo=" << cobo_idx << ", asad=" << asad_idx << std::endl << std::flush;
  //  std::cout << "GetChannels: NasadBoards=" << myGeometryPtr->GetAsadNboards(cobo_idx) << std::endl << std::flush;
  //////// DEBUG  
  if(!IsOK() || myGeometryPtr->GetAsadNboards(cobo_idx)==0 ||
     asad_idx<0 || asad_idx>=myGeometryPtr->GetAsadNboards(cobo_idx)) {
    //////// DEBUG
    //    std::cout << "GetChannels: ERROR" << std::endl << std::flush;
     //////// DEBUG  
    return std::shared_ptr<TH2D>();
  }
  auto event_id = myEventInfo.GetEventId();
  std::shared_ptr<TH2D> result(new TH2D( Form("hraw_cobo%i_asad%i_signal_evt%d", cobo_idx, asad_idx,event_id),
					 Form("Event-%d: Raw signals from CoBo %i AsAd %i;Time bin [arb.u.];Global AsAd channel no.;Charge/bin [arb.u.]",
					      event_id, cobo_idx, asad_idx),
					 myGeometryPtr->GetAgetNtimecells(),
					 0.0-0.5, 
					 1.*myGeometryPtr->GetAgetNtimecells()-0.5, // ends at 511.5 (cells numbered from 0 to 511)
					myGeometryPtr->GetAgetNchips()*myGeometryPtr->GetAgetNchannels()+1,
					 -0.5,
					 myGeometryPtr->GetAgetNchips()*myGeometryPtr->GetAgetNchannels()+0.5 ));
  // fill new histogram
  for(int aget_num=0; aget_num<myGeometryPtr->GetAgetNchips(); ++aget_num) {
    for(int aget_ch=0; aget_ch<myGeometryPtr->GetAgetNchannels();++aget_ch){
      for(int icell=0; icell<myGeometryPtr->GetAgetNtimecells(); icell++) {
	double val = GetValByStrip(myGeometryPtr->GetStripByAget(cobo_idx, asad_idx, aget_num, aget_ch), icell);	
	result->Fill(1.*icell, aget_num*myGeometryPtr->GetAgetNchannels()+aget_ch, val); 
      }
    }
  }
  return result;
}

std::shared_ptr<TH2D> EventTPC::GetChannels_raw(int cobo_idx, int asad_idx){ // valid range [0-1][0-3]
  if(!IsOK() || myGeometryPtr->GetAsadNboards(cobo_idx)==0 ||
     asad_idx<0 || asad_idx>=myGeometryPtr->GetAsadNboards(cobo_idx)) {return std::shared_ptr<TH2D>();}
  auto event_id = myEventInfo.GetEventId();
  std::shared_ptr<TH2D> result(new TH2D( Form("hraw_cobo%i_asad%i_signal_fpn_evt%d", cobo_idx, asad_idx,event_id),
					 Form("Event-%d: Raw signals from Cobo %i Asad %i;Time bin [arb.u.];Global raw channel no.;Charge/bin [arb.u.]",
					      event_id, cobo_idx, asad_idx),
					 myGeometryPtr->GetAgetNtimecells(),
					 0.0-0.5, 
					 1.*myGeometryPtr->GetAgetNtimecells()-0.5, // ends at 511.5 (cells numbered from 0 to 511)
					myGeometryPtr->GetAgetNchips()*myGeometryPtr->GetAgetNchannels_raw()+1,
					 -0.5,
					 myGeometryPtr->GetAgetNchips()*myGeometryPtr->GetAgetNchannels_raw()+0.5 ));
  // fill new histogram
  for(int aget_num=0; aget_num<myGeometryPtr->GetAgetNchips(); ++aget_num) {
    for(int aget_ch=0; aget_ch<myGeometryPtr->GetAgetNchannels_raw();++aget_ch){
      for(int icell=0; icell<myGeometryPtr->GetAgetNtimecells(); icell++) {
	double val = GetValByStrip(myGeometryPtr->GetStripByAget_raw(cobo_idx, asad_idx, aget_num, aget_ch), icell);
	result->Fill(1.*icell, aget_num*myGeometryPtr->GetAgetNchannels_raw()+aget_ch, val); 
      }
    }
  }
  return result;
}

std::shared_ptr<TH2D> EventTPC::GetStripVsTimeInMM(const SigClusterTPC &cluster, int strip_dir){  // valid range [0-2]

  if(!IsOK()) return std::shared_ptr<TH2D>();
  bool err_flag;
  double zmin=0.0-0.5;  // time_cell_min;
  double zmax=myGeometryPtr->GetAgetNtimecells()-0.5; // time_cell_max;  
  double minTimeInMM = myGeometryPtr->Timecell2pos(zmin, err_flag);
  double maxTimeInMM = myGeometryPtr->Timecell2pos(zmax, err_flag);
  auto event_id = myEventInfo.GetEventId();
  //////////// DEBUG
  //  std::cout << "GetStripVsTimeInMM: "
  //	    << "strip_dir=" << strip_dir
  //	    << ", GetDirMinStripMerged=" << myGeometryPtr->GetDirMinStripMerged(strip_dir)
  //    	    << ", GetDirMaxStripMerged=" << myGeometryPtr->GetDirMaxStripMerged(strip_dir) 
  //	    << std::endl << std::flush;
  //  std::cout << "GetStripVsTimeInMM: List of sections for DIR=" << myGeometryPtr->GetDirName(strip_dir) << ":";
  //  for(unsigned int isec=0; isec<myGeometryPtr->GetDirSectionIndexList(strip_dir).size(); isec++) {
  //    std::cout << " " << myGeometryPtr->GetDirSectionIndexList(strip_dir).at(isec);
  //  }
  //  std::cout << std::endl << std::flush;
  //////////// DEBUG

  // - loop over all sections in a given family of strips
  // - check both ends of the first and the last strip in a given section
  double minStripInMM=1E30;
  double maxStripInMM=-1E30;
  std::tie(minStripInMM, maxStripInMM) = myGeometryPtr->rangeStripDirInMM(strip_dir);
  /*
  for(unsigned int isec=0; isec<myGeometryPtr->GetDirSectionIndexList(strip_dir).size(); isec++) {
    int section=myGeometryPtr->GetDirSectionIndexList(strip_dir).at(isec); // strip section index
    double distance1, distance2; // distance between projection of one of the strip end-points and projection of the origin (0,0) along U/V/W pitch axis
    StripTPC *firstStrip = myGeometryPtr->GetStripByDir(strip_dir, section,
							myGeometryPtr->GetDirMinStrip(strip_dir, section));
    distance1 = (firstStrip->Offset() + myGeometryPtr->GetReferencePoint())*myGeometryPtr->GetStripPitchVector(strip_dir);
    distance2 = (firstStrip->Offset() + firstStrip->Length()*firstStrip->Unit() + myGeometryPtr->GetReferencePoint())*myGeometryPtr->GetStripPitchVector(strip_dir);
    if(distance1>maxStripInMM) maxStripInMM=distance1;
    if(distance1<minStripInMM) minStripInMM=distance1;
    if(distance2>maxStripInMM) maxStripInMM=distance2;
    if(distance2<minStripInMM) minStripInMM=distance2;
    
    StripTPC *lastStrip = myGeometryPtr->GetStripByDir(strip_dir, section,
						       myGeometryPtr->GetDirMaxStrip(strip_dir, section));
    distance1 = (lastStrip->Offset() + myGeometryPtr->GetReferencePoint())*myGeometryPtr->GetStripPitchVector(strip_dir);
    distance2 = (lastStrip->Offset() + lastStrip->Length()*lastStrip->Unit() + myGeometryPtr->GetReferencePoint())*myGeometryPtr->GetStripPitchVector(strip_dir);
    if(distance1>maxStripInMM) maxStripInMM=distance1;
    if(distance1<minStripInMM) minStripInMM=distance1;
    if(distance2>maxStripInMM) maxStripInMM=distance2;
    if(distance2<minStripInMM) minStripInMM=distance2;
    
  } // end of loop
  */
  //////////// DEBUG
  //  std::cout << "GetStripVsTimeInMM: Min[mm]=" << minStripInMM << ", Max[mm]=" << maxStripInMM << std::endl << std::flush;
  //////////// DEBUG

  std::shared_ptr<TH2D> result(new TH2D( Form("hclust_%s_vs_time_mm_evt%d", myGeometryPtr->GetDirName(strip_dir), event_id),
					 Form("Event-%d: Clustered hits from %s strips;Drift direction [mm];%s strip direction [mm];Charge/bin [arb.u.]",
					 event_id, myGeometryPtr->GetDirName(strip_dir), myGeometryPtr->GetDirName(strip_dir)),
					 myGeometryPtr->GetAgetNtimecells(), minTimeInMM, maxTimeInMM,
					 myGeometryPtr->GetDirNStripsMerged(strip_dir), minStripInMM, maxStripInMM));

  // fill new histogram
  double x = 0.0, y = 0.0;
  for(int strip_num=cluster.GetMinStrip(strip_dir); strip_num<=cluster.GetMaxStrip(strip_dir); strip_num++) {
    for(int icell=cluster.GetMinTime(strip_dir); icell<=cluster.GetMaxTime(strip_dir); icell++) {
      if( cluster.CheckByStripMerged(strip_dir, strip_num, icell) ) {
	x = myGeometryPtr->Timecell2pos(icell, err_flag);
	if(err_flag) continue;
	y = myGeometryPtr->Strip2posUVW(strip_dir, strip_num, err_flag);	
	if(err_flag) continue;
	double val = GetValByStripMerged(strip_dir, strip_num, icell);
	if(val>0) result->Fill(x, y, val);///HACK AK
      }
    }
  }
  return result;
}

std::shared_ptr<TH2D> EventTPC::GetStripVsTimeInMM(int strip_dir){  // valid range [0-2]

  if(!IsOK()) return std::shared_ptr<TH2D>();
  auto minStripNum=myGeometryPtr->GetDirMinStripMerged(strip_dir);
  auto maxStripNum=myGeometryPtr->GetDirMaxStripMerged(strip_dir);
  bool err_flag;
  double zmin=0.0-0.5;  // time_cell_min;
  double zmax=myGeometryPtr->GetAgetNtimecells()-0.5; // time_cell_max;  
  double minTimeInMM = myGeometryPtr->Timecell2pos(zmin, err_flag);
  double maxTimeInMM = myGeometryPtr->Timecell2pos(zmax, err_flag);
  auto event_id = myEventInfo.GetEventId();
  //////////// DEBUG
  //  std::cout << "GetStripVsTimeInMM: "
  //	    << "strip_dir=" << strip_dir
  //	    << ", GetDirMinStripMerged=" << myGeometryPtr->GetDirMinStripMerged(strip_dir)
  //    	    << ", GetDirMaxStripMerged=" << myGeometryPtr->GetDirMaxStripMerged(strip_dir) 
  //	    << std::endl << std::flush;
  //  std::cout << "GetStripVsTimeInMM: List of sections for DIR=" << myGeometryPtr->GetDirName(strip_dir) << ":";
  //  for(unsigned int isec=0; isec<myGeometryPtr->GetDirSectionIndexList(strip_dir).size(); isec++) {
  //    std::cout << " " << myGeometryPtr->GetDirSectionIndexList(strip_dir).at(isec);
  //  }
  //  std::cout << std::endl << std::flush;
  //////////// DEBUG

  double minStripInMM=1E30;
  double maxStripInMM=-1E30;
  std::tie(minStripInMM, maxStripInMM) = myGeometryPtr->rangeStripDirInMM(strip_dir);
  /*
  // - loop over all sections in a given family of strips
  // - check both ends of the first and the last strip in a given section
  for(unsigned int isec=0; isec<myGeometryPtr->GetDirSectionIndexList(strip_dir).size(); isec++) {
    int section=myGeometryPtr->GetDirSectionIndexList(strip_dir).at(isec); // strip section index
    double distance1, distance2; // distance between projection of one of the strip end-points and projection of the origin (0,0) along U/V/W pitch axis
    StripTPC *firstStrip = myGeometryPtr->GetStripByDir(strip_dir, section,
							myGeometryPtr->GetDirMinStrip(strip_dir, section));
    distance1 = (firstStrip->Offset() + myGeometryPtr->GetReferencePoint())*myGeometryPtr->GetStripPitchVector(strip_dir);
    distance2 = (firstStrip->Offset() + firstStrip->Length()*firstStrip->Unit() + myGeometryPtr->GetReferencePoint())*myGeometryPtr->GetStripPitchVector(strip_dir);
    if(distance1>maxStripInMM) maxStripInMM=distance1;
    if(distance1<minStripInMM) minStripInMM=distance1;
    if(distance2>maxStripInMM) maxStripInMM=distance2;
    if(distance2<minStripInMM) minStripInMM=distance2;
    
    StripTPC *lastStrip = myGeometryPtr->GetStripByDir(strip_dir, section,
						       myGeometryPtr->GetDirMaxStrip(strip_dir, section));
    distance1 = (lastStrip->Offset() + myGeometryPtr->GetReferencePoint())*myGeometryPtr->GetStripPitchVector(strip_dir);
    distance2 = (lastStrip->Offset() + lastStrip->Length()*lastStrip->Unit() + myGeometryPtr->GetReferencePoint())*myGeometryPtr->GetStripPitchVector(strip_dir);
    if(distance1>maxStripInMM) maxStripInMM=distance1;
    if(distance1<minStripInMM) minStripInMM=distance1;
    if(distance2>maxStripInMM) maxStripInMM=distance2;
    if(distance2<minStripInMM) minStripInMM=distance2;
    
  } // end of loop
  */
  //////////// DEBUG
  //  std::cout << "GetStripVsTimeInMM: DIR=" << myGeometryPtr->GetDirName(strip_dir) << ", Min[mm]=" << minStripInMM << ", Max[mm]=" << maxStripInMM << ", Nstrips=" << myGeometryPtr->GetDirNStripsMerged(strip_dir) << std::endl << std::flush;
  //////////// DEBUG
  std::shared_ptr<TH2D> result(new TH2D( Form("hraw_%s_vs_time_mm_evt%d", myGeometryPtr->GetDirName(strip_dir), event_id),
					 Form("Event-%d: Raw signals from %s strips;Drift direction [mm];%s strip direction [mm];Charge/bin [arb.u.]",
					 event_id, myGeometryPtr->GetDirName(strip_dir), myGeometryPtr->GetDirName(strip_dir)),
					 myGeometryPtr->GetAgetNtimecells(), minTimeInMM, maxTimeInMM,
					 myGeometryPtr->GetDirNStripsMerged(strip_dir), minStripInMM, maxStripInMM));

  // fill new histogram
  double x = 0.0, y = 0.0;
  for(int strip_num=minStripNum; strip_num<=maxStripNum; strip_num++) {
    for(int icell=0; icell<myGeometryPtr->GetAgetNtimecells(); icell++) {
      x = myGeometryPtr->Timecell2pos(icell, err_flag);
      //////////// DEBUG
      //      std::cout << "GetStripVsTimeInMM: Timecell2pos: X[mm]=" << x << ", err_flag=" << err_flag << std::endl << std::flush;
      //////////// DEBUG
      if(err_flag) continue;
      y = myGeometryPtr->Strip2posUVW(strip_dir, strip_num, err_flag);	
      //////////// DEBUG
      //      std::cout << "GetStripVsTimeInMM: Strip2pos Y[mm]=" << y << ", err_flag=" << err_flag << std::endl << std::flush;
      //////////// DEBUG
      if(err_flag) continue;
      double val = GetValByStripMerged(strip_dir, strip_num, icell);
      //      if(val) result->Fill(x, y, val);
      result->Fill(x, y, val);
    }
  }
  return result;
}

// get ALL three projections on: XY, XZ, YZ planes
std::vector<TH2D*> EventTPC::Get2D(const SigClusterTPC &cluster, double radius, int rebin_space, int rebin_time, int method) { 

  //  const bool rebin_flag=false;
  TH2D *h1 = NULL;
  TH2D *h2 = NULL;
  TH2D *h3 = NULL;
  std::vector<TH2D*> hvec;
  hvec.resize(0);
  bool err_flag = false;

  if(!IsOK() || !cluster.IsOK() || 
     cluster.GetNhits(DIR_U)<1 || cluster.GetNhits(DIR_V)<1 || cluster.GetNhits(DIR_W)<1 ) return hvec;

  // loop over time slices and match hits in space
  const int time_cell_min = MAXIMUM( cluster.min_time[DIR_U], MAXIMUM( cluster.min_time[DIR_V], cluster.min_time[DIR_W] ));
  const int time_cell_max = MINIMUM( cluster.max_time[DIR_U], MINIMUM( cluster.max_time[DIR_V], cluster.max_time[DIR_W] ));

  ////////// DEBUG 
  //  std::cout << Form(">>>> EventId = %d", event_id) << std::endl;
  //  std::cout << Form(">>>> Time cell range = [%d, %d]", time_cell_min, time_cell_max) << std::endl;
  ////////// DEBUG 

  const std::map<MultiKey2, std::vector<int> > & hitListByTimeDirMerged = cluster.GetHitListByTimeDirMerged();
  
  for(int icell=time_cell_min; icell<=time_cell_max; icell++) {
    if((hitListByTimeDirMerged.find(MultiKey2(icell, DIR_U))==hitListByTimeDirMerged.end()) ||
       (hitListByTimeDirMerged.find(MultiKey2(icell, DIR_V))==hitListByTimeDirMerged.end()) ||
       (hitListByTimeDirMerged.find(MultiKey2(icell, DIR_W))==hitListByTimeDirMerged.end())) continue;
    
    std::vector<int> hits[3] = {
				hitListByTimeDirMerged.find(MultiKey2(icell, DIR_U))->second,
				hitListByTimeDirMerged.find(MultiKey2(icell, DIR_V))->second,
				hitListByTimeDirMerged.find(MultiKey2(icell, DIR_W))->second};

    ////////// DEBUG 
    //   std::cout << Form(">>>> Number of hits: time cell=%d: U=%d / V=%d / W=%d",
    //		      icell, (int)hits[DIR_U].size(), (int)hits[DIR_V].size(), (int)hits[DIR_W].size()) << std::endl;
    ////////// DEBUG 

    // check if there is at least one hit in each direction
    if(hits[DIR_U].size()==0 || hits[DIR_V].size()==0 || hits[DIR_W].size()==0) continue;
    
    std::map<int, int> n_match[3]; // map of number of matched points for each merged strip, key=STRIP_NUM [1-1024]
    std::map<MultiKey3, TVector2> hitPos; // key=(STRIP_NUM_U, STRIP_NUM_V, STRIP_NUM_W), value=(X [mm],Y [mm])
   
    // loop over hits and confirm matching in space
    for(int i0=0; i0<(int)hits[0].size(); i0++) {
      for(auto iter0=myGeometryPtr->GetDirSectionIndexList(DIR_U).begin();
	  iter0!=myGeometryPtr->GetDirSectionIndexList(DIR_U).end(); iter0++) {
	std::shared_ptr<StripTPC> strip0 = myGeometryPtr->GetStripByDir(DIR_U, *iter0, hits[0].at(i0));
	for(int i1=0; i1<(int)hits[1].size(); i1++) {
	  for(auto iter1=myGeometryPtr->GetDirSectionIndexList(DIR_V).begin();
	      iter1!=myGeometryPtr->GetDirSectionIndexList(DIR_V).end(); iter1++) {
	    std::shared_ptr<StripTPC> strip1 = myGeometryPtr->GetStripByDir(DIR_V, *iter1, hits[1].at(i1));
	    for(int i2=0; i2<(int)hits[2].size(); i2++) {
	      for(auto iter2=myGeometryPtr->GetDirSectionIndexList(DIR_W).begin();
		  iter2!=myGeometryPtr->GetDirSectionIndexList(DIR_W).end(); iter2++) {
		std::shared_ptr<StripTPC> strip2 = myGeometryPtr->GetStripByDir(DIR_W, *iter2, hits[2].at(i2));
		
		////////// DEBUG 
		//	  std::cout << Form(">>>> Checking triplet: time_cell=%d: U=%d / V=%d / W=%d",
		//			    icell, hits[0].at(i0), hits[1].at(i1), hits[2].at(i2)) << std::endl;
		////////// DEBUG 

		TVector2 pos;
		if( myGeometryPtr->MatchCrossPoint( strip0, strip1, strip2, radius, pos )) {
		  (n_match[DIR_U])[hits[0].at(i0)]++;
		  (n_match[DIR_V])[hits[1].at(i1)]++;
		  (n_match[DIR_W])[hits[2].at(i2)]++;
		  MultiKey3 mkey(hits[0].at(i0), hits[1].at(i1), hits[2].at(i2));
		  // accept only first matched 2D postion 
		  if(hitPos.find(mkey)!=hitPos.end()) continue;
		  hitPos[mkey]=pos;
		  ////////// DEBUG 
		  //      std::cout << Form(">>>> Checking triplet: result = TRUE" ) << std::endl;
		  ////////// DEBUG 
		} else {
		  ////////// DEBUG 
		  //	  std::cout << Form(">>>> Checking triplet: result = FALSE" ) << std::endl;
		  ////////// DEBUG 
		}
	      }
	    }
	  }
	}
      }
    }
    ////////// DEBUG 
    //    std::cout << Form(">>>> Number of matches: time_cell=%d, triplets=%d", icell, (int)hitPos.size()) << std::endl;
    ////////// DEBUG 
    if(hitPos.size()<1) continue;

    // book histograms before first fill
    if(h1==NULL && h2==NULL && h3==NULL) {

      double xmin, xmax, ymin, ymax;
      std::tie(xmin, xmax, ymin, ymax) = myGeometryPtr->rangeXY();
      
      double zmin=0.0-0.5;  // time_cell_min;
      double zmax=myGeometryPtr->GetAgetNtimecells()-0.5; // time_cell_max;  
      
      int nx = (int)( (xmax-xmin)/myGeometryPtr->GetStripPitch()-1 );
      int ny = (int)( (ymax-ymin)/myGeometryPtr->GetPadPitch()-1 );
      int nz = (int)( zmax-zmin );
      auto event_id = myEventInfo.GetEventId();
      zmin = myGeometryPtr->Timecell2pos(zmin, err_flag);
      zmax = myGeometryPtr->Timecell2pos(zmax, err_flag);

      // rebin in space
      if(rebin_space>1) {
	nx /= rebin_space;
	ny /= rebin_space;
      }

      // rebin in time
      if(rebin_time>1) {
	nz /= rebin_time;
      }

      ////////// DEBUG 
      //      std::cout << Form(">>>> XY histogram: range=[%lf, %lf] x [%lf, %lf], nx=%d, ny=%d",
      //      			xmin, xmax, ymin, ymax, nx, ny) << std::endl;
      ////////// DEBUG 

      h1 = new TH2D( Form("hrecoXY_evt%d", event_id),
		     Form("Event-%d: Projection in XY;X [mm];Y [mm];Charge/bin [arb.u.]", event_id),
		     nx, xmin, xmax, ny, ymin, ymax );
		     
      ////////// DEBUG 
      //      std::cout << Form(">>>> XZ histogram: range=[%lf, %lf] x [%lf, %lf], nx=%d, nz=%d",
      //      			xmin, xmax, zmin, zmax, nx, nz) << std::endl;
      ////////// DEBUG 
		     
      h2 = new TH2D( Form("hrecoXZ_evt%d", event_id),
		     Form("Event-%d: Projection in XZ;X [mm];Z [mm];Charge/bin [arb.u.]", event_id),
		     nx, xmin, xmax, nz, zmin, zmax );

      ////////// DEBUG 
      //      std::cout << Form(">>>> YZ histogram: range=[%lf, %lf] x [%lf, %lf], nx=%d, nz=%d",
      //      			ymin, ymax, zmin, zmax, ny, nz) << std::endl;
      ////////// DEBUG 
		     
      h3 = new TH2D( Form("hrecoYZ_evt%d", event_id),
		     Form("Event-%d: Projection in YZ;Y [mm];Z [mm];Charge/bin [arb.u.]", event_id),
		     ny, ymin, ymax, nz, zmin, zmax );
    }

    // needed for method #2 only:
    // loop over matched hits and update fraction map
    std::map<MultiKey3, double> fraction[3]; // for U,V,W local charge projections
    std::map<MultiKey3, TVector2>::iterator it1, it2;

    for(it1=hitPos.begin(); it1!=hitPos.end(); it1++) {

      int u1=std::get<0>(it1->first);
      int v1=std::get<1>(it1->first);
      int w1=std::get<2>(it1->first);
      double qtot[3] = {0., 0., 0.};  // sum of charges along three directions (for a given time range)
      double    q[3] = {0., 0., 0.};  // charge in a given strip (for a given time range)
      q[DIR_U] = GetValByStripMerged(DIR_U, u1, icell);
      q[DIR_V] = GetValByStripMerged(DIR_V, v1, icell);
      q[DIR_W] = GetValByStripMerged(DIR_W, w1, icell);

      // loop over directions
      for(it2=hitPos.begin(); it2!=hitPos.end(); it2++) {
	int u2=std::get<0>(it2->first);
	int v2=std::get<1>(it2->first);
	int w2=std::get<2>(it2->first);
	
	if(u1==u2) {
	  qtot[DIR_V] += GetValByStripMerged(DIR_V, v2, icell);
	  qtot[DIR_W] += GetValByStripMerged(DIR_W, w2, icell);
	}
	if(v1==v2) {
	  qtot[DIR_W] += GetValByStripMerged(DIR_W, w2, icell);
	  qtot[DIR_U] += GetValByStripMerged(DIR_U, u2, icell);
	}
	if(w1==w2){
	  qtot[DIR_U] += GetValByStripMerged(DIR_U, u2, icell);
	  qtot[DIR_V] += GetValByStripMerged(DIR_V, v2, icell);
	}
      }
      fraction[DIR_U].insert(std::pair<MultiKey3, double>(it1->first, q[DIR_U] / qtot[DIR_U] ));
      fraction[DIR_V].insert(std::pair<MultiKey3, double>(it1->first, q[DIR_V] / qtot[DIR_V] ));
      fraction[DIR_W].insert(std::pair<MultiKey3, double>(it1->first, q[DIR_W] / qtot[DIR_W] ));
    }

    // loop over matched hits and fill histograms
    if(h1 && h2 && h3) {

      std::map<MultiKey3, TVector2>::iterator it;
      for(it=hitPos.begin(); it!=hitPos.end(); it++) {

	double val = 0.0;

	switch (method) {

	case 0: // mehtod #1 - divide charge equally 
	  val = 
	    GetValByStripMerged(DIR_U, std::get<0>(it->first), icell) / n_match[0].at(std::get<0>(it->first)) +
	    GetValByStripMerged(DIR_V, std::get<1>(it->first), icell) / n_match[1].at(std::get<1>(it->first)) +
	    GetValByStripMerged(DIR_W, std::get<2>(it->first), icell) / n_match[2].at(std::get<2>(it->first));
	  break;

	case 1: // method #2 - divide charge according to charge-fraction in two other directions

	  val = 
	    GetValByStripMerged(DIR_U, 
				std::get<0>(it->first), icell)*0.5*( fraction[DIR_V].at(it->first) + fraction[DIR_W].at(it->first) ) +
	    GetValByStripMerged(DIR_V, 
				std::get<1>(it->first), icell)*0.5*( fraction[DIR_W].at(it->first) + fraction[DIR_U].at(it->first) ) +
	    GetValByStripMerged(DIR_W, 
				std::get<2>(it->first), icell)*0.5*( fraction[DIR_U].at(it->first) + fraction[DIR_V].at(it->first) );
	  break;

	default: 
	  val=0.0;

	}; // end of switch (method)...
	
	Double_t z=myGeometryPtr->Timecell2pos(icell, err_flag);
	h1->Fill( (it->second).X(), (it->second).Y(), val );
	h2->Fill( (it->second).X(), z, val );
	h3->Fill( (it->second).Y(), z, val );

      }
    }
  }
  if(h1 && h2 && h3) {
    hvec.push_back(h1);
    hvec.push_back(h2);
    hvec.push_back(h3);
  }
  return hvec;
}

//frame for plotting 3D reconstruction 
TH3D *EventTPC::Get3DFrame(int rebin_space, int rebin_time) const{

  bool err_flag = false;
  double xmin=1E30;
  double xmax=-1E30;
  double ymin=1E30;
  double ymax=-1E30;
  double zmin=0.0-0.5;  // time_cell_min;
  double zmax=myGeometryPtr->GetAgetNtimecells()-0.5; // time_cell_max;  
  /*
  StripTPC* s[6] = {
		    myGeometryPtr->GetStripByDir(DIR_U, 1),
		    myGeometryPtr->GetStripByDir(DIR_U, myGeometryPtr->GetDirNstrips(DIR_U)),
		    myGeometryPtr->GetStripByDir(DIR_V, 1),
		    myGeometryPtr->GetStripByDir(DIR_V, myGeometryPtr->GetDirNstrips(DIR_V)),
		    myGeometryPtr->GetStripByDir(DIR_W, 1),
		    myGeometryPtr->GetStripByDir(DIR_W, myGeometryPtr->GetDirNstrips(DIR_W))
  };
  for(int i=0; i<6; i++) {
    if(!s[i]) continue;
    double x, y;
    TVector2 vec=s[i]->Offset()+myGeometryPtr->GetReferencePoint();
    x=vec.X();
    y=vec.Y();
    if(x>xmax) xmax=x;
    if(x<xmin) xmin=x;
    if(y>ymax) ymax=y;
    if(y<ymin) ymin=y;
    vec = vec + s[i]->Unit()*s[i]->Length();
    if(x>xmax) xmax=x;
    if(x<xmin) xmin=x;
    if(y>ymax) ymax=y;
    if(y<ymin) ymin=y;
    //      if(s[i]->Offset().X()>xmax) xmax=s[i]->Offset().X();
    //      if(s[i]->Offset().X()<xmin) xmin=s[i]->Offset().X();
    //      if(s[i]->Offset().Y()>ymax) ymax=s[i]->Offset().Y();
    //      if(s[i]->Offset().Y()<ymin) ymin=s[i]->Offset().Y();
  }
  xmin-=myGeometryPtr->GetStripPitch()*0.3;
  xmax+=myGeometryPtr->GetStripPitch()*0.7;
  ymin-=myGeometryPtr->GetPadPitch()*0.3;
  ymax+=myGeometryPtr->GetPadPitch()*0.7;

  int nx = (int)( (xmax-xmin)/myGeometryPtr->GetStripPitch()-1 );
  int ny = (int)( (ymax-ymin)/myGeometryPtr->GetPadPitch()-1 );
  int nz = (int)( zmax-zmin );
  */
  std::tie(xmin, xmax, ymin, ymax) = myGeometryPtr->rangeXY();
  int nx = (int)( (xmax-xmin)/myGeometryPtr->GetPadPitch()-1 );
  int ny = (int)( (ymax-ymin)/myGeometryPtr->GetStripPitch()-1 );
  int nz = (int)( zmax-zmin );
  auto event_id = myEventInfo.GetEventId();
  zmin = myGeometryPtr->Timecell2pos(zmin, err_flag);
  zmax = myGeometryPtr->Timecell2pos(zmax, err_flag);

  // rebin in space
  if(rebin_space>1) {
    nx /= rebin_space;
    ny /= rebin_space;
  }

  // rebin in time
  if(rebin_time>1) {
    nz /= rebin_time;
  }

  std::cout << Form(">>>> XYZ histogram: range=[%lf, %lf] x [%lf, %lf] x [%lf, %lf], nx=%d, ny=%d, nz=%d",
		    xmin, xmax, ymin, ymax, zmin, zmax, nx, ny, nz) << std::endl;

  TH3D *h = new TH3D( Form("hreco3D_evt%d", event_id),
		      Form("Event-%d: 3D reco in XYZ;X [mm];Y [mm];Z [mm]", event_id),
		      nx, xmin, xmax, ny, ymin, ymax, nz, zmin, zmax );
  return h;
}


// get 3D histogram of clustered hits
TH3D *EventTPC::Get3D(const SigClusterTPC &cluster, double radius, int rebin_space, int rebin_time, int method) {

  TH3D *h = NULL;
  bool err_flag = false;

  if(!IsOK() || !cluster.IsOK() || 
     cluster.GetNhits(DIR_U)<1 || cluster.GetNhits(DIR_V)<1 || cluster.GetNhits(DIR_W)<1 ) return h;

  // loop over time slices and match hits in space
  const int time_cell_min = MAXIMUM( cluster.min_time[DIR_U], MAXIMUM( cluster.min_time[DIR_V], cluster.min_time[DIR_W] ));
  const int time_cell_max = MINIMUM( cluster.max_time[DIR_U], MINIMUM( cluster.max_time[DIR_V], cluster.max_time[DIR_W] ));

  ////////// DEBUG 
  //std::cout << Form(">>>> EventId = %d", event_id) << std::endl;
  //std::cout << Form(">>>> Time cell range = [%d, %d]", time_cell_min, time_cell_max) << std::endl;
  ////////// DEBUG 

  const std::map<MultiKey2, std::vector<int> > & hitListByTimeDirMerged = cluster.GetHitListByTimeDirMerged();
  
  for(int icell=time_cell_min; icell<=time_cell_max; icell++) {
    if((hitListByTimeDirMerged.find(MultiKey2(icell, DIR_U))==hitListByTimeDirMerged.end()) ||
       (hitListByTimeDirMerged.find(MultiKey2(icell, DIR_V))==hitListByTimeDirMerged.end()) ||
       (hitListByTimeDirMerged.find(MultiKey2(icell, DIR_W))==hitListByTimeDirMerged.end())) continue;
    
    std::vector<int> hits[3] = {
      hitListByTimeDirMerged.find(MultiKey2(icell, DIR_U))->second,
      hitListByTimeDirMerged.find(MultiKey2(icell, DIR_V))->second,
      hitListByTimeDirMerged.find(MultiKey2(icell, DIR_W))->second};
    /*
      const std::map<MultiKey2, std::vector<int> > & hitListByTimeDir = cluster.GetHitListByTimeDir();
      
      for(int icell=time_cell_min; icell<=time_cell_max; icell++) {
      
      if((hitListByTimeDir.find(MultiKey2(icell, DIR_U))==hitListByTimeDir.end()) ||
      (hitListByTimeDir.find(MultiKey2(icell, DIR_V))==hitListByTimeDir.end()) ||
      (hitListByTimeDir.find(MultiKey2(icell, DIR_W))==hitListByTimeDir.end())) continue;
      
      std::vector<int> hits[3] = {
      hitListByTimeDir.find(MultiKey2(icell, DIR_U))->second,
      hitListByTimeDir.find(MultiKey2(icell, DIR_V))->second,
      hitListByTimeDir.find(MultiKey2(icell, DIR_W))->second};
    */
    ////////// DEBUG 
    //   std::cout << Form(">>>> Number of hits: time cell=%d: U=%d / V=%d / W=%d",
    //		      icell, (int)hits[DIR_U].size(), (int)hits[DIR_V].size(), (int)hits[DIR_W].size()) << std::endl;
    ////////// DEBUG 
    
    // check if there is at least one hit in each direction
    if(hits[DIR_U].size()==0 || hits[DIR_V].size()==0 || hits[DIR_W].size()==0) continue;
    
    std::map<int, int> n_match[3]; // map of number of matched points for each strip, key=STRIP_NUM [1-1024]
    std::map<MultiKey3, TVector2> hitPos; // key=(STRIP_NUM_U, STRIP_NUM_V, STRIP_NUM_W), value=(X [mm],Y [mm])

    // loop over hits and confirm matching in space
    for(int i0=0; i0<(int)hits[0].size(); i0++) {
      for(auto iter0=myGeometryPtr->GetDirSectionIndexList(DIR_U).begin();
	  iter0!=myGeometryPtr->GetDirSectionIndexList(DIR_U).end(); iter0++) {
	std::shared_ptr<StripTPC> strip0 = myGeometryPtr->GetStripByDir(DIR_U, *iter0, hits[0].at(i0));
	for(int i1=0; i1<(int)hits[1].size(); i1++) {
	  for(auto iter1=myGeometryPtr->GetDirSectionIndexList(DIR_V).begin();
	      iter1!=myGeometryPtr->GetDirSectionIndexList(DIR_V).end(); iter1++) {
	    std::shared_ptr<StripTPC> strip1 = myGeometryPtr->GetStripByDir(DIR_V, *iter1, hits[1].at(i1));
	    for(int i2=0; i2<(int)hits[2].size(); i2++) {
	      for(auto iter2=myGeometryPtr->GetDirSectionIndexList(DIR_W).begin();
		  iter2!=myGeometryPtr->GetDirSectionIndexList(DIR_W).end(); iter2++) {
		std::shared_ptr<StripTPC> strip2 = myGeometryPtr->GetStripByDir(DIR_W, *iter2, hits[2].at(i2));
		
		////////// DEBUG 
		//	  std::cout << Form(">>>> Checking triplet: time_cell=%d: U=%d / V=%d / W=%d",
		//			    icell, hits[0].at(i0), hits[1].at(i1), hits[2].at(i2)) << std::endl;
		////////// DEBUG 

		TVector2 pos;
		if( myGeometryPtr->MatchCrossPoint( strip0, strip1, strip2, radius, pos )) {
		  (n_match[DIR_U])[hits[0].at(i0)]++;
		  (n_match[DIR_V])[hits[1].at(i1)]++;
		  (n_match[DIR_W])[hits[2].at(i2)]++;
		  MultiKey3 mkey(hits[0].at(i0), hits[1].at(i1), hits[2].at(i2));
		  // accept only first matched 2D postion 
		  if(hitPos.find(mkey)!=hitPos.end()) continue;
		  hitPos[mkey]=pos;
		  ////////// DEBUG 
		  //	    std::cout << Form(">>>> Checking triplet: result = TRUE" ) << std::endl;
		  ////////// DEBUG 
		} else {
		  ////////// DEBUG 
		  //	    std::cout << Form(">>>> Checking triplet: result = FALSE" ) << std::endl;
		  ////////// DEBUG 
		}
	      }
	    }
	  }
	}
      }
    }
    //    std::cout << Form(">>>> Number of matches: time_cell=%d, triplets=%d", icell, (int)hitPos.size()) << std::endl;
    if(hitPos.size()<1) continue;

    // book 3D histogram before first fill
    if(h==NULL) h = Get3DFrame(rebin_space, rebin_time); 
    // needed for method #2 only:
    // loop over matched hits and update fraction map
    std::map<MultiKey3, double> fraction[3]; // for U,V,W local charge projections
    std::map<MultiKey3, TVector2>::iterator it1, it2;

    for(it1=hitPos.begin(); it1!=hitPos.end(); it1++) {

      int u1=std::get<0>(it1->first);
      int v1=std::get<1>(it1->first);
      int w1=std::get<2>(it1->first);
      std::vector<double> qtot = {0., 0., 0.};  // sum of charges along three directions (for a given time range)
      std::vector<double> q = {0., 0., 0.};  // charge in a given strip (for a given time range)
      q[DIR_U] = GetValByStripMerged(DIR_U, u1, icell);
      q[DIR_V] = GetValByStripMerged(DIR_V, v1, icell);
      q[DIR_W] = GetValByStripMerged(DIR_W, w1, icell);

      // loop over directions
      for(it2=hitPos.begin(); it2!=hitPos.end(); it2++) {
	int u2=std::get<0>(it2->first);
	int v2=std::get<1>(it2->first);
	int w2=std::get<2>(it2->first);
	
	if(u1==u2) {
	  qtot[DIR_V] += GetValByStripMerged(DIR_V, v2, icell);
	  qtot[DIR_W] += GetValByStripMerged(DIR_W, w2, icell);
	}
	if(v1==v2) {
	  qtot[DIR_W] += GetValByStripMerged(DIR_W, w2, icell);
	  qtot[DIR_U] += GetValByStripMerged(DIR_U, u2, icell);
	}
	if(w1==w2){
	  qtot[DIR_U] += GetValByStripMerged(DIR_U, u2, icell);
	  qtot[DIR_V] += GetValByStripMerged(DIR_V, v2, icell);
	}
      }
      fraction[DIR_U].insert(std::pair<MultiKey3, double>(it1->first, q[DIR_U] / qtot[DIR_U] ));
      fraction[DIR_V].insert(std::pair<MultiKey3, double>(it1->first, q[DIR_V] / qtot[DIR_V] ));
      fraction[DIR_W].insert(std::pair<MultiKey3, double>(it1->first, q[DIR_W] / qtot[DIR_W] ));
    }
    
    // loop over matched hits and fill histograms
    if(h) {

      std::map<MultiKey3, TVector2>::iterator it;
      for(it=hitPos.begin(); it!=hitPos.end(); it++) {

	double val = 0.0;

	switch (method) {

	case 0: // mehtod #1 - divide charge equally
	  val = 
	    GetValByStripMerged(DIR_U, std::get<0>(it->first), icell) / n_match[0].at(std::get<0>(it->first)) +
	    GetValByStripMerged(DIR_V, std::get<1>(it->first), icell) / n_match[1].at(std::get<1>(it->first)) +
	    GetValByStripMerged(DIR_W, std::get<2>(it->first), icell) / n_match[2].at(std::get<2>(it->first));
	  break;

	case 1: // method #2 - divide charge according to charge-fraction in two other directions
	  val = 
	    GetValByStripMerged(DIR_U, 
			  std::get<0>(it->first), icell)*0.5*( fraction[DIR_V].at(it->first) + fraction[DIR_W].at(it->first) ) +
	    GetValByStripMerged(DIR_V, 
			  std::get<1>(it->first), icell)*0.5*( fraction[DIR_W].at(it->first) + fraction[DIR_U].at(it->first) ) +
	    GetValByStripMerged(DIR_W, 
			  std::get<2>(it->first), icell)*0.5*( fraction[DIR_U].at(it->first) + fraction[DIR_V].at(it->first) );
	  break;
	  
	default: 
	  val=0.0;
	}; // end of switch (method)...
	Double_t z=myGeometryPtr->Timecell2pos(icell, err_flag);
	h->Fill( (it->second).X(), (it->second).Y(), z, val );
      }
    }
  }
  return h;
}


TH2D *EventTPC::GetXY_TestUV(TH2D *h) { // test (unphysical) histogram
  //TH2D *h = NULL;

  if(!IsOK()) return h;

  if(h==NULL) {
    double xmin=1E30;
    double xmax=-1E30;
    double ymin=1E30;
    double ymax=-1E30;
    /*
    StripTPC* s[6] = {
      myGeometryPtr->GetStripByDir(DIR_U, 1),
      myGeometryPtr->GetStripByDir(DIR_U, myGeometryPtr->GetDirNstrips(DIR_U)),
      myGeometryPtr->GetStripByDir(DIR_V, 1),
      myGeometryPtr->GetStripByDir(DIR_V, myGeometryPtr->GetDirNstrips(DIR_V)),
      myGeometryPtr->GetStripByDir(DIR_W, 1),
      myGeometryPtr->GetStripByDir(DIR_W, myGeometryPtr->GetDirNstrips(DIR_W))
    };
    for(int i=0; i<6; i++) {
      if(!s[i]) continue;
      if(s[i]->Offset().X()>xmax) xmax=s[i]->Offset().X();
      if(s[i]->Offset().X()<xmin) xmin=s[i]->Offset().X();
      if(s[i]->Offset().Y()>ymax) ymax=s[i]->Offset().Y();
      if(s[i]->Offset().Y()<ymin) ymin=s[i]->Offset().Y();
    }
    xmin-=myGeometryPtr->GetStripPitch()*0.3;
    xmax+=myGeometryPtr->GetStripPitch()*0.7;
    ymin-=myGeometryPtr->GetPadPitch()*0.3;
    ymax+=myGeometryPtr->GetPadPitch()*0.7;

    int nx = (int)( (xmax-xmin)/myGeometryPtr->GetStripPitch()-1 );
    int ny = (int)( (ymax-ymin)/myGeometryPtr->GetPadPitch()-1 );
    */
    std::tie(xmin, xmax, ymin, ymax) = myGeometryPtr->rangeXY();
    int nx = (int)( (xmax-xmin)/myGeometryPtr->GetPadPitch()-1 );
    int ny = (int)( (ymax-ymin)/myGeometryPtr->GetStripPitch()-1 );
    
    ////////// DEBUG 
    std::cout << Form(">>>> U-V correlation test histogram: range=[%lf, %lf] x [%lf, %lf], nx=%d, ny=%d",
		      xmin, xmax, ymin, ymax, nx, ny) << std::endl;
    ////////// DEBUG 
    
    h = new TH2D( "h_test_XY_UV_2d",
		  "Intersection of U-V strips;X [mm];Y [mm]; V strip no.",
		  nx, xmin, xmax, ny, ymin, ymax );
  }

  // loop over all strip numbers and check hits strip intersection
  for(int i0=1; i0<=myGeometryPtr->GetDirNstrips(DIR_U); i0++) {
    if(i0!=1) continue;
    for(auto iter0=myGeometryPtr->GetDirSectionIndexList(DIR_U).begin();
	iter0!=myGeometryPtr->GetDirSectionIndexList(DIR_U).end(); iter0++) {
      std::shared_ptr<StripTPC> strip0 = myGeometryPtr->GetStripByDir(DIR_U, *iter0, i0);
      for(int i1=1; i1<(int)myGeometryPtr->GetDirNstrips(DIR_V); i1++) {
	for(auto iter1=myGeometryPtr->GetDirSectionIndexList(DIR_V).begin();
	    iter1!=myGeometryPtr->GetDirSectionIndexList(DIR_V).end(); iter1++) {
	  std::shared_ptr<StripTPC> strip1 = myGeometryPtr->GetStripByDir(DIR_V, *iter1, i1);
	  TVector2 pos;
	  if( myGeometryPtr->GetCrossPoint( strip0, strip1, pos) ) {
	    h->SetBinContent( h->FindBin( pos.X(), pos.Y() ), 1.*i0);
	  }
	}
      }
    }
  }

  ////////// DEBUG 
  std::cout << ">>>> U-V correlation test histogram: END" << std::endl;
  ////////// DEBUG 

  return h;
}

TH2D *EventTPC::GetXY_TestVW(TH2D *h) { // test (unphysical) histogram
  //  TH2D *h = NULL;

  if(!IsOK()) return h;

  if(h==NULL) {
    double xmin=1E30;
    double xmax=-1E30;
    double ymin=1E30;
    double ymax=-1E30;
    /*
    StripTPC* s[6] = {
      myGeometryPtr->GetStripByDir(DIR_U, 1),
      myGeometryPtr->GetStripByDir(DIR_U, myGeometryPtr->GetDirNstrips(DIR_U)),
      myGeometryPtr->GetStripByDir(DIR_V, 1),
      myGeometryPtr->GetStripByDir(DIR_V, myGeometryPtr->GetDirNstrips(DIR_V)),
      myGeometryPtr->GetStripByDir(DIR_W, 1),
      myGeometryPtr->GetStripByDir(DIR_W, myGeometryPtr->GetDirNstrips(DIR_W))
    };
    for(int i=0; i<6; i++) {
      if(!s[i]) continue;
      if(s[i]->Offset().X()>xmax) xmax=s[i]->Offset().X();
      if(s[i]->Offset().X()<xmin) xmin=s[i]->Offset().X();
      if(s[i]->Offset().Y()>ymax) ymax=s[i]->Offset().Y();
      if(s[i]->Offset().Y()<ymin) ymin=s[i]->Offset().Y();
    }
    xmin-=myGeometryPtr->GetStripPitch()*0.3;
    xmax+=myGeometryPtr->GetStripPitch()*0.7;
    ymin-=myGeometryPtr->GetPadPitch()*0.3;
    ymax+=myGeometryPtr->GetPadPitch()*0.7;
    
    int nx = (int)( (xmax-xmin)/myGeometryPtr->GetStripPitch()-1 );
    int ny = (int)( (ymax-ymin)/myGeometryPtr->GetPadPitch()-1 );
    */   
    std::tie(xmin, xmax, ymin, ymax) = myGeometryPtr->rangeXY();
    int nx = (int)( (xmax-xmin)/myGeometryPtr->GetPadPitch()-1 );
    int ny = (int)( (ymax-ymin)/myGeometryPtr->GetStripPitch()-1 );

    ////////// DEBUG 
    std::cout << Form(">>>> V-W correlation test histogram: range=[%lf, %lf] x [%lf, %lf], nx=%d, ny=%d",
		      xmin, xmax, ymin, ymax, nx, ny) << std::endl;
    ////////// DEBUG 
    
    h = new TH2D( "h_test_XY_VW_2d",
		  "Intersection of V-W strips;X [mm];Y [mm]; V strip no.",
		  nx, xmin, xmax, ny, ymin, ymax );
  }

  // loop over all strip numbers and check hits strip intersection
  for(int i0=1; i0<(int)myGeometryPtr->GetDirNstrips(DIR_V); i0++) {
    if(i0!=1) continue;
    for(auto iter0=myGeometryPtr->GetDirSectionIndexList(DIR_V).begin();
	iter0!=myGeometryPtr->GetDirSectionIndexList(DIR_V).end(); iter0++) {
      std::shared_ptr<StripTPC> strip0 = myGeometryPtr->GetStripByDir(DIR_V, *iter0, i0);
      for(int i1=1; i1<(int)myGeometryPtr->GetDirNstrips(DIR_W); i1++) {
	for(auto iter1=myGeometryPtr->GetDirSectionIndexList(DIR_W).begin();
	    iter1!=myGeometryPtr->GetDirSectionIndexList(DIR_W).end(); iter1++) {
	  std::shared_ptr<StripTPC> strip1 = myGeometryPtr->GetStripByDir(DIR_W, *iter1, i1);
	  TVector2 pos;
	  if( myGeometryPtr->GetCrossPoint( strip0, strip1, pos) ) {
	    h->SetBinContent( h->FindBin( pos.X(), pos.Y() ), 1.*i0);
	  }
	}
      }
    }
  }

  ////////// DEBUG 
  std::cout << ">>>> V-W correlation test histogram: END" << std::endl;
  ////////// DEBUG 

  return h;
}

TH2D *EventTPC::GetXY_TestWU(TH2D *h) { // test (unphysical) histogram
  //  TH2D *h = NULL;

  if(!IsOK()) return h;

  if(h==NULL) {
    double xmin=1E30;
    double xmax=-1E30;
    double ymin=1E30;
    double ymax=-1E30;
    /*    StripTPC* s[6] = {
      myGeometryPtr->GetStripByDir(DIR_U, 1),
      myGeometryPtr->GetStripByDir(DIR_U, myGeometryPtr->GetDirNstrips(DIR_U)),
      myGeometryPtr->GetStripByDir(DIR_V, 1),
      myGeometryPtr->GetStripByDir(DIR_V, myGeometryPtr->GetDirNstrips(DIR_V)),
      myGeometryPtr->GetStripByDir(DIR_W, 1),
      myGeometryPtr->GetStripByDir(DIR_W, myGeometryPtr->GetDirNstrips(DIR_W))
    };
    for(int i=0; i<6; i++) {
      if(!s[i]) continue;
      if(s[i]->Offset().X()>xmax) xmax=s[i]->Offset().X();
      if(s[i]->Offset().X()<xmin) xmin=s[i]->Offset().X();
      if(s[i]->Offset().Y()>ymax) ymax=s[i]->Offset().Y();
      if(s[i]->Offset().Y()<ymin) ymin=s[i]->Offset().Y();
    }
    xmin-=myGeometryPtr->GetStripPitch()*0.3;
    xmax+=myGeometryPtr->GetStripPitch()*0.7;
    ymin-=myGeometryPtr->GetPadPitch()*0.3;
    ymax+=myGeometryPtr->GetPadPitch()*0.7;
    
    int nx = (int)( (xmax-xmin)/myGeometryPtr->GetStripPitch()-1 );
    int ny = (int)( (ymax-ymin)/myGeometryPtr->GetPadPitch()-1 );
    */
    std::tie(xmin, xmax, ymin, ymax) = myGeometryPtr->rangeXY();
    int nx = (int)( (xmax-xmin)/myGeometryPtr->GetPadPitch()-1 );
    int ny = (int)( (ymax-ymin)/myGeometryPtr->GetStripPitch()-1 );

    ////////// DEBUG 
    std::cout << Form(">>>> W-U correlation test histogram: range=[%lf, %lf] x [%lf, %lf], nx=%d, ny=%d",
    		      xmin, xmax, ymin, ymax, nx, ny) << std::endl;
    ////////// DEBUG 
    
    h = new TH2D( "h_test_XY_WU_2d",
		  "Intersection of W-U strips;X [mm];Y [mm]; W strip no.",
		  nx, xmin, xmax, ny, ymin, ymax );
  }

  // loop over all strip numbers and check hits strip intersection
  for(int i0=1; i0<(int)myGeometryPtr->GetDirNstrips(DIR_W); i0++) {
    if(i0!=1) continue;
    for(auto iter0=myGeometryPtr->GetDirSectionIndexList(DIR_W).begin();
	iter0!=myGeometryPtr->GetDirSectionIndexList(DIR_W).end(); iter0++) {
      std::shared_ptr<StripTPC> strip0 = myGeometryPtr->GetStripByDir(DIR_W, *iter0, i0);
      for(int i1=1; i1<(int)myGeometryPtr->GetDirNstrips(DIR_U); i1++) {
	for(auto iter1=myGeometryPtr->GetDirSectionIndexList(DIR_U).begin();
	    iter1!=myGeometryPtr->GetDirSectionIndexList(DIR_U).end(); iter1++) {
	  std::shared_ptr<StripTPC> strip1 = myGeometryPtr->GetStripByDir(DIR_U, *iter1, i1);
	  TVector2 pos;
	  if( myGeometryPtr->GetCrossPoint( strip0, strip1, pos) ) {
	    h->SetBinContent( h->FindBin( pos.X(), pos.Y() ), 1.*i0);
	  }
	}
      }
    }
  }

  ////////// DEBUG 
  std::cout << ">>>> W-U correlation test histogram: END" << std::endl;
  ////////// DEBUG

  return h;
}
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
// overloading << operator
std::ostream& operator<<(std::ostream& os, const EventTPC& e) {
  os << "EventTPC: " << e.GetEventInfo();
  return os;
}
/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////
