#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <iterator>
#include <fstream>
#include <utility>
#include <algorithm> // for find_if

#include "TH1D.h"
#include "TH2D.h"
#include "TH3F.h"

#include "GeometryTPC.h"
#include "EventTPC.h"

/* ============= SPACE-TIME CLUSTER CLASS ===========*/

SigClusterTPC::SigClusterTPC(EventTPC *e)
  : 
    evt_ptr(e),
    initOK(false), 
    glb_min_time( -1 ),
    glb_max_time( -1 ),
    glb_max_charge( 0.0 ),
    glb_max_charge_timing( -1 ),
    glb_max_charge_channel( -1 ),
    glb_tot_charge( 0.0 ) {
  
  for(int idir=0; idir<3; idir++) {
    nhits[idir]=0;
    nstrips[idir]=0;
    min_strip[idir]=-1;
    max_strip[idir]=-1;
    min_time[idir]=-1;
    max_time[idir]=-1;
    max_charge[idir]=0.0;
    max_charge_timing[idir]=-1;
    max_charge_strip[idir]=-1;
    tot_charge[idir]=0.0;
  }
  nhitsMap.clear();
  hitList.clear();
  hitListByTimeDir.clear();
  hitListByDir.clear();
  totalChargeMap.clear();   // 2-key map: strip_dir, strip_number
  totalChargeMap2.clear();  // 2-key map: strip_dir, time_cell
  totalChargeMap3.clear();  // 1-key map: time_cell
  maxChargeMap.clear();     // 2-key map: strip_dir, strip_number 
  if(evt_ptr && evt_ptr->IsOK()) initOK=true;
  
}

// list of SELECTED hits corresponding to a given STRIP_DIR[0-2],
// return value=key(REBIN_TIME_CELL [0-511], STRIP_NUM [1-1024])
std::vector<MultiKey2> SigClusterTPC::GetHitListByDir(int strip_dir) {
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: return hitListByDir[strip_dir];
  };
  std::vector<MultiKey2> empty;
  return empty;
}

bool SigClusterTPC::AddByStrip(int strip_dir, int strip_number, int time_cell) {  // valid range [0-2][1-1024][0-511]
  if(!IsOK() || 
     time_cell<0 || time_cell>=evt_ptr->GetGeoPtr()->GetAgetNtimecells() || 
     strip_number<1 || strip_number>evt_ptr->GetGeoPtr()->GetDirNstrips(strip_dir)) return false;
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: {
    MultiKey3 mkey3(strip_dir, strip_number, time_cell);
    MultiKey2 mkey2(time_cell, strip_dir);

    MultiKey2 mkey_total(strip_dir, strip_number);
    MultiKey2 mkey_total2(strip_dir, time_cell);
    MultiKey2 mkey_maxval(strip_dir, strip_number);

    MultiKey2 mkey2_CellStrip(time_cell, strip_number);

    // hitList - check if hit is unique
    if(find_if(hitList.begin(), hitList.end(), mkey3)==hitList.end()) {

      // add new hit
      hitList.push_back(mkey3);
      hitListByTimeDir[mkey2].push_back(strip_number);
      hitListByDir[strip_dir].push_back(mkey2_CellStrip);

      // update hit statistics
      MultiKey2 mkey(strip_dir, strip_number);
      nhits[strip_dir]++;
      if(nhitsMap.find(mkey)==nhitsMap.end()) {
	nstrips[strip_dir]++;  // increment counter only once for a given strip
      }
      nhitsMap[mkey]++; // increment counter for each time-cell of a given strip

      // update space-time envelope
      if( max_strip[strip_dir] < strip_number ) max_strip[strip_dir] = strip_number;
      if( min_strip[strip_dir] > strip_number || min_strip[strip_dir]<1 ) min_strip[strip_dir] = strip_number;
      if( max_time[strip_dir] < time_cell ) max_time[strip_dir] = time_cell;
      if( min_time[strip_dir] > time_cell || min_time[strip_dir]<1 ) min_time[strip_dir] = time_cell;
      if( glb_max_time < time_cell ) glb_max_time = time_cell;
      if( glb_min_time > time_cell || glb_min_time<1 ) glb_min_time = time_cell;

      double val = evt_ptr->GetValByStrip(strip_dir, strip_number, time_cell);

      // update charge integrals

      tot_charge[strip_dir] += val;
      glb_tot_charge += val;

      // totalChargeMap - check if strip exists
      std::map<MultiKey2, double, multikey2_less>::iterator it_total;
      if( (it_total=totalChargeMap.find(mkey_total))==totalChargeMap.end() ) {
	// add new total charge per strip
	totalChargeMap[mkey_total]=val;
      } else {
	// update already existing total charge per strip
	it_total->second += val;
      }
      
      // totalChargeMap2 - check if strip exists
      std::map<MultiKey2, double, multikey2_less>::iterator it_total2;
      if( (it_total2=totalChargeMap2.find(mkey_total2))==totalChargeMap2.end() ) {
	// add new total value per strip
	totalChargeMap2[mkey_total2]=val;
      } else {
	// update already existing total value per strip
	it_total2->second += val;
      }
      
      // totalChargeMap3 - check if strip exists
      std::map<int, double>::iterator it_total3;
      if( (it_total3=totalChargeMap3.find(time_cell))==totalChargeMap3.end() ) {
	// add new total value per strip
	totalChargeMap3[time_cell]=val;
      } else {
	// update already existing total value per strip
	it_total3->second += val;
      }

      // update charge maxima

      // maxChargeMap - check if strip exists
      std::map<MultiKey2, double, multikey2_less>::iterator it_maxval;
      if( (it_maxval=maxChargeMap.find(mkey_maxval))==maxChargeMap.end() ) {
	// add new max value per strip
	maxChargeMap[mkey_maxval]=val;
      } else {
	// update already existing max value per strip
	if(val > it_maxval->second) it_maxval->second = val;
      }
    
      if( val > max_charge[strip_dir] ) { 
	max_charge[strip_dir]=val;
	max_charge_timing[strip_dir]=time_cell;
	max_charge_strip[strip_dir]=strip_number;
	if( val > glb_max_charge ) {
	  glb_max_charge=val;
	  glb_max_charge_timing=time_cell;
	  glb_max_charge_channel=evt_ptr->GetGeoPtr()->Global_strip2normal(strip_dir, strip_number);
	}
      }

    }
    return true;
  }
  };
  return false;  
}               
               
bool SigClusterTPC::AddByStrip(StripTPC *strip, int time_cell) {  // valid range [0-511]
  if(strip) return AddByStrip( strip->Dir(), strip->Num(), time_cell);
  return false;
}

bool SigClusterTPC::AddByGlobalChannel(int glb_channel_idx, int time_cell) {  // valid range [0-1023][0-511]
  return AddByStrip(evt_ptr->GetGeoPtr()->GetStripByGlobal(glb_channel_idx), time_cell);
}

bool SigClusterTPC::AddByGlobalChannel_raw(int glb_raw_channel_idx, int time_cell) {  // valid range [0-1023+4*N][0-511]
  return AddByStrip(evt_ptr->GetGeoPtr()->GetStripByGlobal_raw(glb_raw_channel_idx), time_cell);
}

bool SigClusterTPC::AddByAgetChannel(int cobo_idx, int asad_idx, int aget_idx, int channel_idx, int time_cell) {  // valid range [0-1][0-3][0-3][0-63][0-511]
  return AddByStrip(evt_ptr->GetGeoPtr()->GetStripByAget(cobo_idx, asad_idx, aget_idx, channel_idx), time_cell);
}

bool SigClusterTPC::AddByAgetChannel_raw(int cobo_idx, int asad_idx, int aget_idx, int raw_channel_idx, int time_cell) {  // valid range [0-1][0-3][0-3][0-67][0-511]
  return AddByStrip(evt_ptr->GetGeoPtr()->GetStripByAget_raw(cobo_idx, asad_idx, aget_idx, raw_channel_idx), time_cell);
}

/* OLD, SLOWER IMPLEMENTATION
bool SigClusterTPC::CheckByStrip(int strip_dir, int strip_number, int time_cell) {  // valid range [0-2][1-1024][0-511]
  if(!IsOK() || time_cell<0 || time_cell>=512 || strip_number<1 || strip_number>evt_ptr->GetGeoPtr()->GetDirNstrips(strip_dir)) return false;
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: return find_if(hitList.begin(), hitList.end(), MultiKey3(strip_dir, strip_number, time_cell))!=hitList.end();
  };
  return false;  
}
*/
bool SigClusterTPC::CheckByStrip(int strip_dir, int strip_number, int time_cell) {  // valid range [0-2][1-1024][0-511]
  if(!IsOK() || time_cell<0 || time_cell>=512 || strip_number<1 || strip_number>evt_ptr->GetGeoPtr()->GetDirNstrips(strip_dir)) return false;
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: return find_if(hitListByDir[strip_dir].begin(), hitListByDir[strip_dir].end(), MultiKey2(time_cell, strip_number))!=hitListByDir[strip_dir].end();
  };
  return false;  
}

bool SigClusterTPC::CheckByStrip(StripTPC *strip, int time_cell) {  // valid range [0-511]
  if(strip) return CheckByStrip(strip->Dir(), strip->Num(), time_cell);
  return false;
}

bool SigClusterTPC::CheckByGlobalChannel(int glb_channel_idx, int time_cell) {  // valid range [0-1023][0-511]
  return CheckByStrip(evt_ptr->GetGeoPtr()->GetStripByGlobal(glb_channel_idx), time_cell);
}

bool SigClusterTPC::CheckByGlobalChannel_raw(int glb_raw_channel_idx, int time_cell) {  // valid range [0-(1023+ASAD_N[0]*4+...)][0-511]
  return CheckByStrip(evt_ptr->GetGeoPtr()->GetStripByGlobal_raw(glb_raw_channel_idx), time_cell);
}

bool SigClusterTPC::CheckByAgetChannel(int cobo_idx, int asad_idx, int aget_idx, int channel_idx, int time_cell) {  // valid range [0-1][0-3][0-3][0-63][0-511]
  return CheckByStrip(evt_ptr->GetGeoPtr()->GetStripByAget(cobo_idx, asad_idx, aget_idx, channel_idx), time_cell);
}

bool SigClusterTPC::CheckByAgetChannel_raw(int cobo_idx, int asad_idx, int aget_idx, int raw_channel_idx, int time_cell) {  // valid range [0-1][0-3][0-3][0-67][0-511]
  return CheckByStrip(evt_ptr->GetGeoPtr()->GetStripByAget_raw(cobo_idx, asad_idx, aget_idx, raw_channel_idx), time_cell);
}

long SigClusterTPC::GetNhitsByStrip(int strip_dir, int strip_num) {   // # of hits in a given strip
  if(!IsOK() || strip_num<1 || strip_num>evt_ptr->GetGeoPtr()->GetDirNstrips(strip_dir)) return ERROR;
  switch(strip_dir) {
  case DIR_U: 
  case DIR_V: 
  case DIR_W:
    MultiKey2 mkey(strip_dir, strip_num);

    // check if hit exists
    std::map<MultiKey2, int, multikey2_less>::iterator it;
    if( (it=nhitsMap.find(mkey))!=nhitsMap.end() ) {
      return it->second;
    }
    return 0;
  };
  return ERROR;
}

long SigClusterTPC::GetNhits(int strip_dir) {   // # of hits in a given direction
  if(!IsOK()) return ERROR;
  switch(strip_dir) {
  case DIR_U: 
  case DIR_V: 
  case DIR_W: return nhits[strip_dir];
  };
  return ERROR;
}

long SigClusterTPC::GetNhits() {  // global # of hits
  if(!IsOK()) return ERROR;
  return nhits[DIR_U]+nhits[DIR_V]+nhits[DIR_W];
}

int SigClusterTPC::GetMultiplicity(int strip_dir) {  // # of strips with hits in a given direction
  if(!IsOK()) return ERROR;
  switch(strip_dir) {
  case DIR_U: 
  case DIR_V: 
  case DIR_W: return nstrips[strip_dir];
  };
  return ERROR;
}

int SigClusterTPC::GetMultiplicity() {  // global # of strips with hits from all strips
  if(!IsOK()) return ERROR;
  return nstrips[DIR_U]+nstrips[DIR_V]+nstrips[DIR_W];
}  

int SigClusterTPC::GetMinStrip(int strip_dir) {  // minimal strip number in a given direction
  if(!IsOK()) return ERROR;
  switch(strip_dir) {
  case DIR_U: 
  case DIR_V: 
  case DIR_W: return min_strip[strip_dir];
  };
  return ERROR;
}

int SigClusterTPC::GetMaxStrip(int strip_dir) { // maximal strip number in a given direction
  if(!IsOK()) return ERROR;
  switch(strip_dir) {
  case DIR_U: 
  case DIR_V: 
  case DIR_W: return max_strip[strip_dir];
  };
  return ERROR;
}

int SigClusterTPC::GetMinTime(int strip_dir) {  // minimal time cell in a given direction
  if(!IsOK()) return ERROR;
  switch(strip_dir) {
  case DIR_U: 
  case DIR_V: 
  case DIR_W: return min_time[strip_dir];
  };
  return ERROR;
}

int SigClusterTPC::GetMaxTime(int strip_dir) {  // maximal time cell in a given direction
  if(!IsOK()) return ERROR;
  switch(strip_dir) {
  case DIR_U: 
  case DIR_V: 
  case DIR_W: return max_time[strip_dir];
  };
  return ERROR;
}

int SigClusterTPC::GetMinTime() {  // minimal time cell from all strips
  if(!IsOK()) return ERROR;
  return glb_min_time;
}

int SigClusterTPC::GetMaxTime() {  // maximal time cell from all cluster hits
  if(!IsOK()) return ERROR;
  return glb_max_time;
}

double SigClusterTPC::GetMaxCharge(int strip_dir, int strip_number) { // maximal charge from cluster hits in a given strip (if any)
  if(!IsOK()) return 0.0;
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: 
    MultiKey2 mkey(strip_dir, strip_number);

    // check if hit is unique
    std::map<MultiKey2, double, multikey2_less>::iterator it;
    if( (it=maxChargeMap.find(mkey))==maxChargeMap.end() ) {
      return 0.0;
    }
    return it->second;
  };
  return 0.0;    
}

double SigClusterTPC::GetMaxCharge(int strip_dir) {  // maximal charge from all cluster hits in a given direction
  if(!IsOK()) return 0.0;
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: return max_charge[strip_dir];
  };
  return 0.0;    
}

double SigClusterTPC::GetMaxCharge() {  // maximal charge from all cluster hits
  if(!IsOK()) return 0.0;
  return glb_max_charge;
}

int SigClusterTPC::GetMaxChargeTime(int strip_dir) {  // arrival time of the maximal charge from cluster hits in a given direction
  if(!IsOK()) return ERROR;
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: return max_charge_timing[strip_dir];
  };
  return ERROR;
}

int SigClusterTPC::GetMaxChargeStrip(int strip_dir) {  // strip number with the maximal charge from cluster hits in a given direction
  if(!IsOK()) return ERROR;
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: return max_charge_strip[strip_dir];
  };
  return ERROR;
}

int SigClusterTPC::GetMaxChargeTime() {  // arrival time of the maximal charge from all cluster hits
  if(!IsOK()) return ERROR;
  return glb_max_charge_timing;
}

int SigClusterTPC::GetMaxChargeChannel() {  // global channel number with the maximal charge from cluster hits
  if(!IsOK()) return ERROR;
  return glb_max_charge_channel;
}

double SigClusterTPC::GetTotalCharge(int strip_dir, int strip_number) { // charge integral from cluster hits in a given strip (if any)
  if(!IsOK()) return 0.0;
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: 
    MultiKey2 mkey(strip_dir, strip_number);

    // check if hit is unique
    std::map<MultiKey2, double, multikey2_less>::iterator it;
    if( (it=totalChargeMap.find(mkey))==totalChargeMap.end() ) {
      return 0.0;
    }
    return it->second;
  };
  return 0.0;    
}

double SigClusterTPC::GetTotalCharge(int strip_dir) {  // charge integral from all cluster hits in a given direction
  if(!IsOK()) return 0.0;
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: return tot_charge[strip_dir];
  };
  return 0.0;    
}

double SigClusterTPC::GetTotalCharge() {   // charge integral from all cluster hits
  if(!IsOK()) return 0.0;
  return glb_tot_charge;
}

double SigClusterTPC::GetTotalChargeByTimeCell(int strip_dir, int time_cell) { // charge integral from a single time cell from all cluster hits in a given direction
  if(!IsOK() || time_cell<0 || time_cell>=512) {
    return 0.0;
  }
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: 
    MultiKey2 mkey(strip_dir, time_cell);

    // check if time slice is unique
    std::map<MultiKey2, double, multikey2_less>::iterator it;
    if( (it=totalChargeMap2.find(mkey))!=totalChargeMap2.end() ) {
      return it->second;
    }
  };
  return 0.0;
}

double SigClusterTPC::GetTotalChargeByTimeCell(int time_cell) { // charge integral from a single time cell from all cluster hits
  if(!IsOK()) return 0.0;
  
  // check if time slice is unique
  std::map<int, double>::iterator it;
  if( (it=totalChargeMap3.find(time_cell))!=totalChargeMap3.end() ) {
    return it->second;
  }
  return 0.0;
}


/* ============= TPC EVENT CLASS ===========*/

EventTPC::EventTPC()
  : myGeometryPtr(0), initOK(false), 
    time_rebin( 1 ), 
    glb_max_charge( 0.0 ),
    glb_max_charge_timing( -1 ),
    glb_max_charge_channel( -1 ),
    glb_tot_charge( 0.0 ) {

  for(int idir=0; idir<3; idir++) {
    max_charge[idir]=0.0;
    max_charge_timing[idir]=-1;
    max_charge_strip[idir]=-1;
    tot_charge[idir]=0.0;
  }
  totalChargeMap.clear();  // 2-key map: strip_dir, strip_number
  totalChargeMap2.clear();  // 2-key map: strip_dir, time_cell
  totalChargeMap3.clear();  // 1-key map: time_cell
  maxChargeMap.clear();    // 2-key map: strip_dir, strip_number 
  chargeMap.clear();       // 3-key map: strip_dir, strip_number, time_cell  
}

void EventTPC::SetGeoPtr(std::shared_ptr<GeometryTPC> aPtr) {
  myGeometryPtr = aPtr;
  if(myGeometryPtr && myGeometryPtr->IsOK()) initOK=true;
  }

bool EventTPC::SetTimeRebin(int rebin) {
  if(rebin<1) return false;
  time_rebin=rebin;
  return true;
}

bool EventTPC::AddValByStrip(int strip_dir, int strip_number, int time_cell, double val) {  // valid range [0-2][1-1024][0-511]
  if(!IsOK() || time_cell<0 || time_cell>=myGeometryPtr->GetAgetNtimecells() || strip_number<1 || strip_number>myGeometryPtr->GetDirNstrips(strip_dir)) return false;
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: {
    MultiKey3 mkey(strip_dir, strip_number, time_cell);
    MultiKey2 mkey_total(strip_dir, strip_number);
    MultiKey2 mkey_total2(strip_dir, time_cell);
    MultiKey2 mkey_maxval(strip_dir, strip_number);

    // chargeMap - check if hit is unique
    std::map<MultiKey3, double, multikey3_less>::iterator it;
    double new_val=0.0;
    if( (it=chargeMap.find(mkey))==chargeMap.end() ) {

      // add new hit
      chargeMap[mkey]=val;
      new_val=val;

    } else {

      // update already existing hit
      it->second += val;
      new_val=it->second;

    }

    // update charge integrals

    tot_charge[strip_dir] += val;
    glb_tot_charge += val;

    // totalChargeMap - check if strip exists
    std::map<MultiKey2, double, multikey2_less>::iterator it_total;
    if( (it_total=totalChargeMap.find(mkey_total))==totalChargeMap.end() ) {
      // add new total value per strip
      totalChargeMap[mkey_total]=val;
    } else {
      // update already existing total value per strip
      it_total->second += val;
    }

    // totalChargeMap2 - check if strip exists
    std::map<MultiKey2, double, multikey2_less>::iterator it_total2;
    if( (it_total2=totalChargeMap2.find(mkey_total2))==totalChargeMap2.end() ) {
      // add new total value per strip
      totalChargeMap2[mkey_total2]=val;
    } else {
      // update already existing total value per strip
      it_total2->second += val;
    }

    // totalChargeMap3 - check if strip exists
    std::map<int, double>::iterator it_total3;
    if( (it_total3=totalChargeMap3.find(time_cell))==totalChargeMap3.end() ) {
      // add new total value per strip
      totalChargeMap3[time_cell]=val;
    } else {
      // update already existing total value per strip
      it_total3->second += val;
    }

    // upadate charge maxima

    // maxChargeMap - check if strip exists
    std::map<MultiKey2, double, multikey2_less>::iterator it_maxval;
    if( (it_maxval=maxChargeMap.find(mkey_maxval))==maxChargeMap.end() ) {
      // add new max value per strip
      maxChargeMap[mkey_maxval]=new_val;
    } else {
      // update already existing max value per strip
      if(new_val > it_maxval->second) it_maxval->second = new_val;
    }
    
    if( new_val > max_charge[strip_dir] ) { 
      max_charge[strip_dir]=new_val;
      max_charge_timing[strip_dir]=time_cell;
      max_charge_strip[strip_dir]=strip_number;
      if( new_val > glb_max_charge ) {
	glb_max_charge=new_val;
	glb_max_charge_timing=time_cell;
	glb_max_charge_channel=myGeometryPtr->Global_strip2normal(strip_dir, strip_number);
	}
    }

    return true;
  }
  };
  return false;  
}


bool EventTPC::AddValByStrip(StripTPC *strip, int time_cell, double val) {  // valid range [0-511]
  if(strip) return AddValByStrip(strip->Dir(), strip->Num(), time_cell, val);
  return false;
}
bool EventTPC::AddValByGlobalChannel(int glb_channel_idx, int time_cell, double val) {  // valid range [0-1023][0-511]
  return AddValByStrip(myGeometryPtr->GetStripByGlobal(glb_channel_idx), time_cell, val);
}
bool EventTPC::AddValByGlobalChannel_raw(int glb_raw_channel_idx, int time_cell, double val) {  // valid range [0-1023+4*N][0-511]
  return AddValByStrip(myGeometryPtr->GetStripByGlobal_raw(glb_raw_channel_idx), time_cell, val);
}
bool EventTPC::AddValByAgetChannel(int cobo_idx, int asad_idx, int aget_idx, int channel_idx, int time_cell, double val) {  // valid range [0-1][0-3][0-3][0-63][0-511]
  return AddValByStrip(myGeometryPtr->GetStripByAget(cobo_idx, asad_idx, aget_idx, channel_idx), time_cell, val);
}
bool EventTPC::AddValByAgetChannel_raw(int cobo_idx, int asad_idx, int aget_idx, int raw_channel_idx, int time_cell, double val) {  // valid range [0-1][0-3][0-3][0-67][0-511]
  return AddValByStrip(myGeometryPtr->GetStripByAget_raw(cobo_idx, asad_idx, aget_idx, raw_channel_idx), time_cell, val);
}

double EventTPC::GetValByStrip(int strip_dir, int strip_number, int time_cell/*, bool &result*/) {  // valid range [0-2][1-1024][0-511]
  //result=false;
  if(!IsOK() || time_cell<0 || time_cell>=512 || strip_number<1 || strip_number>myGeometryPtr->GetDirNstrips(strip_dir)) {
    return 0.0;
  }
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: {
    MultiKey3 mkey(strip_dir, strip_number, time_cell);

    // check if hit is unique
    std::map<MultiKey3, double, multikey3_less>::iterator it;
    if( (it=chargeMap.find(mkey))==chargeMap.end() ) {
      return 0.0;
    }
    //result=true;
    return it->second;
  }
  };
  return 0.0;    
}

double EventTPC::GetValByStrip(StripTPC *strip, int time_cell/*, bool &result*/) {  // valid range [0-511]
  if(strip) return GetValByStrip(strip->Dir(), strip->Num(), time_cell); //, result);
  return false;
}

double EventTPC::GetValByGlobalChannel(int glb_channel_idx, int time_cell/*, bool &result*/) {  // valid range [0-1023][0-511]
  return GetValByStrip(myGeometryPtr->GetStripByGlobal(glb_channel_idx), time_cell); //, result);
}

double EventTPC::GetValByGlobalChannel_raw(int glb_raw_channel_idx, int time_cell/*, bool &result*/) {  // valid range [0-1023+4*N][0-511]
  return GetValByStrip(myGeometryPtr->GetStripByGlobal_raw(glb_raw_channel_idx), time_cell); //, result);
}

double EventTPC::GetValByAgetChannel(int cobo_idx, int asad_idx, int aget_idx, int channel_idx, int time_cell/*, bool &result*/) {  // valid range [0-1][0-3][0-3][0-63][0-511]
  return GetValByStrip(myGeometryPtr->GetStripByAget(cobo_idx, asad_idx, aget_idx, channel_idx), time_cell); //, result);
}

double EventTPC::GetValByAgetChannel_raw(int cobo_idx, int asad_idx, int aget_idx, int raw_channel_idx, int time_cell/*, bool &result*/) {  // valid range [0-1][0-3][0-3][0-67][0-511]
  return GetValByStrip(myGeometryPtr->GetStripByAget(cobo_idx, asad_idx, aget_idx, raw_channel_idx), time_cell); //, result);
}

double EventTPC::GetMaxCharge(int strip_dir, int strip_number) { // maximal charge from single strip of a given direction 
  if(!IsOK()) return 0.0;
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: 
    MultiKey2 mkey(strip_dir, strip_number);

    // check if hit is unique
    std::map<MultiKey2, double, multikey2_less>::iterator it;
    if( (it=maxChargeMap.find(mkey))==maxChargeMap.end() ) {
      return 0.0;
    }
    return it->second;
  };
  return 0.0;    
}

double EventTPC::GetMaxCharge(int strip_dir) {  // maximal charge from strips of a given direction
  if(!IsOK()) return 0.0;
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: return max_charge[strip_dir];
  };
  return 0.0;    
}

double EventTPC::GetMaxCharge() {  // maximal charge from all strips
  if(!IsOK()) return 0.0;
  return glb_max_charge;
}

int EventTPC::GetMaxChargeTime(int strip_dir) {  // arrival time of the maximal charge from strips of a given direction
  if(!IsOK()) return ERROR;
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: return max_charge_timing[strip_dir];
  };
  return ERROR;    
}

int EventTPC::GetMaxChargeStrip(int strip_dir) {  // strip number with the maximal charge in a given direction
  if(!IsOK()) return ERROR;
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: return max_charge_strip[strip_dir];
  };
  return ERROR;    
}

int EventTPC::GetMaxChargeTime() {  // arrival time of the maximal charge from all strips
  if(!IsOK()) return ERROR;
  return glb_max_charge_timing;
}

int EventTPC::GetMaxChargeChannel() {  // global channel number with the maximal charge from all strips
  if(!IsOK()) return ERROR;
  return glb_max_charge_channel;
}

double EventTPC::GetTotalCharge(int strip_dir, int strip_number) { // charge integral from single strip of a given direction 
  if(!IsOK()) return 0.0;
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: 
    MultiKey2 mkey(strip_dir, strip_number);

    // check if hit is unique
    std::map<MultiKey2, double, multikey2_less>::iterator it;
    if( (it=totalChargeMap.find(mkey))==totalChargeMap.end() ) {
      return 0.0;
    }
    return it->second;
  };
  return 0.0;    
}

double EventTPC::GetTotalCharge(int strip_dir) {  // charge integral from strips of a given direction
  if(!IsOK()) return 0.0;
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: return tot_charge[strip_dir];
  };
  return 0.0;    
}

double EventTPC::GetTotalCharge() {   // charge integral from all strips
  if(!IsOK()) return 0.0;
  return glb_tot_charge;
}

double EventTPC::GetTotalChargeByTimeCell(int strip_dir, int time_cell) { // charge integral from a single time cell from all strips in a given direction
  if(!IsOK() || time_cell<0 || time_cell>=512) {
    return 0.0;
  }
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: 
    MultiKey2 mkey(strip_dir, time_cell);

    // check if time slice is unique
    std::map<MultiKey2, double, multikey2_less>::iterator it;
    if( (it=totalChargeMap2.find(mkey))!=totalChargeMap2.end() ) {
      return it->second;
    }
  };
  return 0.0;
}

double EventTPC::GetTotalChargeByTimeCell(int time_cell) { // charge integral from a single time cell from all strips
  if(!IsOK()) return 0.0;
  
  // check if time slice is unique
  std::map<int, double>::iterator it;
  if( (it=totalChargeMap3.find(time_cell))!=totalChargeMap3.end() ) {
    return it->second;
  }
  return 0.0;
}

SigClusterTPC EventTPC::GetOneCluster(double thr, int delta_strips, int delta_timecells) {  // applies clustering threshold to all space-time data points
  SigClusterTPC cluster(this);

  // getting cluster seed hits
  std::map<MultiKey3, double, multikey3_less>::iterator it;
  for( it=chargeMap.begin(); it!=chargeMap.end(); ++it ) {
    if( it->second > thr ) cluster.AddByStrip( (it->first).key1, (it->first).key2, (it->first).key3 );
    // debug - dump the whole event as a single cluster
    //    cluster.AddByStrip( (it->first).key1, (it->first).key2, (it->first).key3 );
    // debug - dump the whole event as a single cluster
  }

  // debug 
  //  std::cout << ">>>> GetSigCluster: nhits=" << cluster.GetNhits() << ", chargeMap.size=" << chargeMap.size() << std::endl;
  std::cout << Form(">>>> GetSigCluster: BEFORE ENVELOPE: nhits(%d)/nhits(%d)/nhits(%d)=%ld/%ld/%ld",
	       DIR_U, DIR_V, DIR_W,
	       cluster.GetNhits(DIR_U), 
	       cluster.GetNhits(DIR_V), 
	       cluster.GetNhits(DIR_W) ) << std::endl;
  // debug

  // adding envelope to the seed hits
  std::vector<MultiKey3> oldList = cluster.GetHitList(); // make a copy of list of SEED-hits
  std::vector<MultiKey3>::iterator it2;

  // loop thru SEED-hits
  for( it2=oldList.begin(); it2!=oldList.end(); ++it2 ) {

    // unpack coordinates
    const int strip_dir = (*it2).key1;
    const int strip_num = (*it2).key2;
    const int time_cell = (*it2).key3;
    const int strip_range[2] = { MAXIMUM(1, strip_num - delta_strips), 
				 MINIMUM(myGeometryPtr->GetDirNstrips(strip_dir), strip_num + delta_strips) };
    const int timecell_range[2] = { MAXIMUM(0, time_cell - delta_timecells), 
				    MINIMUM(myGeometryPtr->GetAgetNtimecells()-1, time_cell + delta_timecells) };
    for(int icell=timecell_range[0]; icell<=timecell_range[1]; icell++) {
      for(int istrip=strip_range[0]; istrip<=strip_range[1]; istrip++) {
	if(icell==time_cell && istrip==strip_num) continue; // exclude existing seed hits
	MultiKey3 mkey3(strip_dir, istrip, icell);
	if(chargeMap.find(mkey3)==chargeMap.end()) continue; // exclude non-existing space-time coordinates
	// add new space-time point
	if(find_if(oldList.begin(), oldList.end(), mkey3)==oldList.end()) {
	  cluster.AddByStrip( strip_dir, istrip, icell );
	} 
      } 
    } 
  }

  // debug
  std::cout << Form(">>>> GetSigCluster: AFTER ENVELOPE:  nhits(%d)/nhits(%d)/nhits(%d)=%ld/%ld/%ld",
	       DIR_U, DIR_V, DIR_W,
	       cluster.GetNhits(DIR_U), 
	       cluster.GetNhits(DIR_V), 
	       cluster.GetNhits(DIR_W) ) << std::endl;
  // debug

  return cluster;
}

TH1D *EventTPC::GetStripProjection(SigClusterTPC &cluster, int strip_dir) {  // valid range [0-2]
  TH1D *h = NULL;
  if(!IsOK() || !cluster.IsOK()) return h;
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: {
    if(cluster.GetNhits(strip_dir)<1) break;
    h = new TH1D( Form("hclust_%spro_evt%lld", myGeometryPtr->GetDirName(strip_dir), event_id), 
		  Form("Event-%lld: Clustered hits from %s strips integrated over time;%s strip no.;Charge/strip [arb.u.]", 
		       event_id, myGeometryPtr->GetDirName(strip_dir), myGeometryPtr->GetDirName(strip_dir)),
		  myGeometryPtr->GetDirNstrips(strip_dir),
		  1.0 - 0.5,
		  1.*myGeometryPtr->GetDirNstrips(strip_dir)+0.5 );
    // fill new histogram
    if(h) {
      //bool res;
      for(int strip_num=cluster.GetMinStrip(strip_dir); strip_num<=cluster.GetMaxStrip(strip_dir); strip_num++) 
	for(int icell=cluster.GetMinTime(strip_dir); icell<=cluster.GetMaxTime(strip_dir); icell++) {
	  if( cluster.CheckByStrip(strip_dir, strip_num, icell) ) {
	    h->Fill(1.*strip_num, GetValByStrip(strip_dir, strip_num, icell/*, res*/));
	}
      }
    }
  }
  };
  return h;
}

TH1D *EventTPC::GetStripProjection(int strip_dir) {  // whole event, valid range [0-2]
  TH1D *h = NULL;
  if(!IsOK()) return h;
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: {
    h = new TH1D( Form("hraw_%spro_evt%lld", myGeometryPtr->GetDirName(strip_dir), event_id), 
                  Form("Event-%lld: Raw signals from %s strips integrated over time;%s strip no.;Charge/strip [arb.u.]", 
                       event_id, myGeometryPtr->GetDirName(strip_dir), myGeometryPtr->GetDirName(strip_dir)),
                  myGeometryPtr->GetDirNstrips(strip_dir),
                  1.0 - 0.5,
                  1.*myGeometryPtr->GetDirNstrips(strip_dir)+0.5 );
    // fill new histogram
    if(h) {
      //bool res;
      for(int strip_num=1; strip_num<=myGeometryPtr->GetDirNstrips(strip_dir); strip_num++) {
        for(int icell=0; icell<myGeometryPtr->GetAgetNtimecells(); icell++) {
          double val = GetValByStrip(strip_dir, strip_num, icell);
          if(val!=0.0) h->Fill(1.*strip_num, val);
        }
      }
    }
  }
  };
  return h;
}

TH1D *EventTPC::GetTimeProjection(SigClusterTPC &cluster, int strip_dir) {  // valid range [0-2]
  TH1D *h = NULL;
  if(!IsOK() || !cluster.IsOK()) return h;
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: {
    if(cluster.GetNhits(strip_dir)<1) break;
    h = new TH1D( Form("hclust_%stime_evt%lld", myGeometryPtr->GetDirName(strip_dir), event_id),
		  Form("Event-%lld: Clustered hits from %s strips;Time bin [arb.u.];Charge/bin [arb.u.]", 
		       event_id, myGeometryPtr->GetDirName(strip_dir)),
		  myGeometryPtr->GetAgetNtimecells(),
		  0.0-0.5, 
		  1.*myGeometryPtr->GetAgetNtimecells()-0.5 ); // ends at 511.5 (cells numbered from 0 to 511)
    // fill new histogram
    if(h) {
      //bool res;
      for(int strip_num=cluster.GetMinStrip(strip_dir); strip_num<=cluster.GetMaxStrip(strip_dir); strip_num++) {
	for(int icell=cluster.GetMinTime(strip_dir); icell<=cluster.GetMaxTime(strip_dir); icell++) {
	  if( cluster.CheckByStrip(strip_dir, strip_num, icell) ) {
	    h->Fill(1.*icell, GetValByStrip(strip_dir, strip_num, icell/*, res*/));
	  }
	}
      }
    }
  }
  };
  return h;
}

TH1D *EventTPC::GetTimeProjection(SigClusterTPC &cluster) {  // all strips
  TH1D *h = NULL;
  if(!IsOK() || !cluster.IsOK() || cluster.GetNhits()==0 ) return h;
  h = new TH1D( Form("hclust_time_evt%lld", event_id),
	        Form("Event-%lld: Clustered hits from all strips;Time bin [arb.u.];Charge/bin [arb.u.]", event_id), 
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
	  if( cluster.CheckByStrip(strip_dir, strip_num, icell) ) {
	    h->Fill(1.*icell, GetValByStrip(strip_dir, strip_num, icell/*, res*/));
	  }
	}
      }
    }
  }
  return h;
}

TH1D *EventTPC::GetTimeProjection() {  // whole event, all strips
  TH1D *h = NULL;
  if(!IsOK()) return h;
  h = new TH1D( Form("hraw_time_evt%lld", event_id),
	        Form("Event-%lld: Raw signals from all strips;Time bin [arb.u.];Charge/bin [arb.u.]", event_id), 
		myGeometryPtr->GetAgetNtimecells(),
		0.0-0.5, 
		1.*myGeometryPtr->GetAgetNtimecells()-0.5 ); // ends at 511.5 (cells numbered from 0 to 511)
  // fill new histogram
  if(h) {
    int counter=0;
    for(int icobo=0; icobo<myGeometryPtr->GetCoboNboards(); icobo++) {
      for(int iasad=0; iasad<myGeometryPtr->GetAsadNboards(icobo); iasad++) {
	for(int ichan=0; ichan<myGeometryPtr->GetAgetNchannels()*myGeometryPtr->GetAgetNchips(); ichan++) {
	  counter++;
	  for(int icell=0; icell<=myGeometryPtr->GetAgetNtimecells(); icell++) {
	    double val = GetValByGlobalChannel(counter, icell);
	    if(val!=0.0) h->Fill(1.*icell, val); 
	  }
	}
      }
    }
  }
  return h;
}

TH2D *EventTPC::GetStripVsTime(SigClusterTPC &cluster, int strip_dir) {  // valid range [0-2]
  TH2D *h = NULL;
  if(!IsOK() || !cluster.IsOK()) return h;
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: {
    if( cluster.GetNhits(strip_dir)<1 ) break;
    h = new TH2D( Form("hclust_%s_vs_time_evt%lld", myGeometryPtr->GetDirName(strip_dir), event_id),
		  Form("Event-%lld: Clustered hits from %s strips;Time bin [arb.u.];%s strip no.;Charge/bin [arb.u.]",
		       event_id, myGeometryPtr->GetDirName(strip_dir), myGeometryPtr->GetDirName(strip_dir)),
		  myGeometryPtr->GetAgetNtimecells(),
		  0.0-0.5, 
		  1.*myGeometryPtr->GetAgetNtimecells()-0.5, // ends at 511.5 (cells numbered from 0 to 511)
		  myGeometryPtr->GetDirNstrips(strip_dir),
		  1.0-0.5,
		  1.*myGeometryPtr->GetDirNstrips(strip_dir)+0.5 );

    // debug
    //    std::cout << Form(">>>>>>>> EventTPC::GetStripVsTime: BOOKING: h = %p, h.entries=%ld", h, 
    //		 (long)(h ? h->GetEntries() : 0) ) << std::endl;
    //    std::cout << Form(">>>>>>>> EventTPC::GetStripVsTime: CLUSTER: [%d, %d] x [%d, %d], max=%lf (evt.max=%lf)",
    //		 cluster.GetMinStrip(strip_dir), cluster.GetMaxStrip(strip_dir),
    //		 cluster.GetMinTime(strip_dir), cluster.GetMaxTime(strip_dir),
    //		 cluster.GetMaxCharge(strip_dir),
    //		 GetMaxCharge(strip_dir) ) << std::endl;
    // debug

    // fill new histogram
    if(h) {
      for(int strip_num=cluster.GetMinStrip(strip_dir); strip_num<=cluster.GetMaxStrip(strip_dir); strip_num++) {
	for(int icell=cluster.GetMinTime(strip_dir); icell<=cluster.GetMaxTime(strip_dir); icell++) {
	  if( cluster.CheckByStrip(strip_dir, strip_num, icell) ) {
	    h->Fill(1.*icell, 1.*strip_num, GetValByStrip(strip_dir, strip_num, icell));
	  //	    std::cout << Form(">>>>>>>> EventTPC::GetStripVsTime: fill [cell=%d, strip=%d]", icell, strip_num) << std::endl;
	  }
	  // debug
	  //	  double val = GetValByStrip(strip_dir, strip_num, icell);
	  //	  h->Fill(1.*icell, 1.*strip_num, val);
	  // debug
	}
      }
    }
  }
  };

  // debug
  //  std::cout << Form(">>>>>>>> EventTPC::GetStripVsTime: END: h = %p, h.entries=%ld", h, 
  //	       (long)(h ? h->GetEntries() : 0) ) << std::endl;
  // debug

  return h;
}

TH2D *EventTPC::GetStripVsTime(int strip_dir) {  // valid range [0-2]
  TH2D *h = NULL;
  if(!IsOK()) return h;
  switch(strip_dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W: {
    h = new TH2D( Form("hraw_%s_vs_time_evt%lld", myGeometryPtr->GetDirName(strip_dir), event_id),
		  Form("Event-%lld: Raw signals from %s strips;Time bin [arb.u.];%s strip no.;Charge/bin [arb.u.]",
		       event_id, myGeometryPtr->GetDirName(strip_dir), myGeometryPtr->GetDirName(strip_dir)),
		  myGeometryPtr->GetAgetNtimecells(),
		  0.0-0.5, 
		  1.*myGeometryPtr->GetAgetNtimecells()-0.5, // ends at 511.5 (cells numbered from 0 to 511)
		  myGeometryPtr->GetDirNstrips(strip_dir),
		  1.0-0.5,
		  1.*myGeometryPtr->GetDirNstrips(strip_dir)+0.5 );
    // fill new histogram
    if(h) {
      for(int strip_num=1; strip_num<=myGeometryPtr->GetDirNstrips(strip_dir); strip_num++) {
	for(int icell=0; icell<myGeometryPtr->GetAgetNtimecells(); icell++) {
	  double val = GetValByStrip(strip_dir, strip_num, icell);
	  if(val!=0.0) h->Fill(1.*icell, 1.*strip_num, val); 
	}
      }
    }
  }
  };
  return h;
}

// get three projections on: XY, XZ, YZ planes
std::vector<TH2D*> EventTPC::Get2D(SigClusterTPC &cluster, double radius, int rebin_space, int rebin_time, int method) { 

  //  const bool rebin_flag=false;
  TH2D *h1 = NULL;
  TH2D *h2 = NULL;
  TH2D *h3 = NULL;
  std::vector<TH2D*> hvec;
  hvec.resize(0);

  if(!IsOK() || !cluster.IsOK() || 
     cluster.GetNhits(DIR_U)<1 || cluster.GetNhits(DIR_V)<1 || cluster.GetNhits(DIR_W)<1 ) return hvec;

  // loop over time slices and match hits in space
  const int time_cell_min = MAXIMUM( cluster.min_time[DIR_U], MAXIMUM( cluster.min_time[DIR_V], cluster.min_time[DIR_W] ));
  const int time_cell_max = MINIMUM( cluster.max_time[DIR_U], MINIMUM( cluster.max_time[DIR_V], cluster.max_time[DIR_W] ));

  //  std::cout << Form(">>>> EventId = %lld", event_id) << std::endl;
  //  std::cout << Form(">>>> Time cell range = [%d, %d]", time_cell_min, time_cell_max) << std::endl;

  for(int icell=time_cell_min; icell<=time_cell_max; icell++) {

    std::vector<int> hits[3] = {
      cluster.hitListByTimeDir[MultiKey2(icell, DIR_U)],
      cluster.hitListByTimeDir[MultiKey2(icell, DIR_V)],
      cluster.hitListByTimeDir[MultiKey2(icell, DIR_W)] };

    //   std::cout << Form(">>>> Number of hits: time cell=%d: U=%d / V=%d / W=%d",
    //		      icell, (int)hits[DIR_U].size(), (int)hits[DIR_V].size(), (int)hits[DIR_W].size()) << std::endl;

    // check if there is at least one hit in each direction
    if(hits[DIR_U].size()==0 || hits[DIR_V].size()==0 || hits[DIR_W].size()==0) continue;
    
    std::map<int, int> n_match[3]; // map of number of matched points for each strip, key=STRIP_NUM [1-1024]
    std::map<MultiKey3, TVector2, multikey3_less> hitPos; // key=(STRIP_NUM_U, STRIP_NUM_V, STRIP_NUM_W), value=(X [mm],Y [mm])
   
    // loop over hits and confirm matching in space
    for(int i0=0; i0<(int)hits[0].size(); i0++) {
      StripTPC *strip0 = myGeometryPtr->GetStripByDir(DIR_U, hits[0].at(i0));
      for(int i1=0; i1<(int)hits[1].size(); i1++) {
	StripTPC *strip1 = myGeometryPtr->GetStripByDir(DIR_V, hits[1].at(i1));
	for(int i2=0; i2<(int)hits[2].size(); i2++) {
	  StripTPC *strip2 = myGeometryPtr->GetStripByDir(DIR_W, hits[2].at(i2));

	  //	  std::cout << Form(">>>> Checking triplet: time_cell=%d: U=%d / V=%d / W=%d",
	  //			    icell, hits[0].at(i0), hits[1].at(i1), hits[2].at(i2)) << std::endl;

	  TVector2 pos;
	  if( myGeometryPtr->MatchCrossPoint( strip0, strip1, strip2, radius, pos )) {
	    (n_match[DIR_U])[hits[0].at(i0)]++;
	    (n_match[DIR_V])[hits[1].at(i1)]++;
	    (n_match[DIR_W])[hits[2].at(i2)]++;
	    hitPos[ MultiKey3( hits[0].at(i0), hits[1].at(i1), hits[2].at(i2)) ]=pos;
	    //	    std::cout << Form(">>>> Checking triplet: result = TRUE" ) << std::endl;
	  } else {
	    //	    std::cout << Form(">>>> Checking triplet: result = FALSE" ) << std::endl;
	  }
	}
      }
    }
    //    std::cout << Form(">>>> Number of matches: time_cell=%d, triplets=%d", icell, (int)hitPos.size()) << std::endl;
    if(hitPos.size()<1) continue;

    const Double_t zfactor=myGeometryPtr->GetVdrift()*10.0/myGeometryPtr->GetSamplingRate(); // [mm] : [vdrift]=cm/us, [rate]=MHz=1E6/s=1/us
    const Double_t zoffset=0.0; // myGeometryPtr->GetDriftCageZmin()-myGeometryPtr->GetVdrift()*10.0*myGeometryPtr->GetTriggerDelay(); // [mm]

    // book histograms before first fill
    if(h1==NULL && h2==NULL && h3==NULL) {

      StripTPC* s[6] = {
	myGeometryPtr->GetStripByDir(DIR_U, 1),
	myGeometryPtr->GetStripByDir(DIR_U, myGeometryPtr->GetDirNstrips(DIR_U)),
	myGeometryPtr->GetStripByDir(DIR_V, 1),
	myGeometryPtr->GetStripByDir(DIR_V, myGeometryPtr->GetDirNstrips(DIR_V)),
	myGeometryPtr->GetStripByDir(DIR_W, 1),
	myGeometryPtr->GetStripByDir(DIR_W, myGeometryPtr->GetDirNstrips(DIR_W))
      };

      double xmin=1E30;
      double xmax=-1E30;
      double ymin=1E30;
      double ymax=-1E30;
      double zmin=0.0-0.5;  // time_cell_min;
      double zmax=511.+0.5; // time_cell_max;

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
      }
      xmin-=myGeometryPtr->GetStripPitch()*0.3;
      xmax+=myGeometryPtr->GetStripPitch()*0.7;
      ymin-=myGeometryPtr->GetPadPitch()*0.3;
      ymax+=myGeometryPtr->GetPadPitch()*0.7;
      //zmin-=5;
      //zmax+=5;
      
      int nx = (int)( (xmax-xmin)/myGeometryPtr->GetStripPitch()-1 );
      int ny = (int)( (ymax-ymin)/myGeometryPtr->GetPadPitch()-1 );
      int nz = (int)( zmax-zmin );

      zmin = zmin*zfactor + zoffset; // [mm]
      zmax = zmax*zfactor + zoffset; // [mm]

      // rebin in space
      if(rebin_space>1) {
	nx /= rebin_space;
	ny /= rebin_space;
      }

      // rebin in time
      if(rebin_time>1) {
	nz /= rebin_time;
      }

      //      std::cout << Form(">>>> XY histogram: range=[%lf, %lf] x [%lf, %lf], nx=%d, ny=%d",
      //      			xmin, xmax, ymin, ymax, nx, ny) << std::endl;

      h1 = new TH2D( Form("hrecoXY_evt%lld", event_id),
		     Form("Event-%lld: Projection in XY;X [mm];Y [mm];Charge/bin [arb.u.]", event_id),
		     nx, xmin, xmax, ny, ymin, ymax );
		     
      //      std::cout << Form(">>>> XZ histogram: range=[%lf, %lf] x [%lf, %lf], nx=%d, nz=%d",
      //      			xmin, xmax, zmin, zmax, nx, nz) << std::endl;
		     
      h2 = new TH2D( Form("hrecoXZ_evt%lld", event_id),
		     Form("Event-%lld: Projection in XZ;X [mm];Z [mm];Charge/bin [arb.u.]", event_id),
		     nx, xmin, xmax, nz, zmin, zmax );

      //      std::cout << Form(">>>> YZ histogram: range=[%lf, %lf] x [%lf, %lf], nx=%d, nz=%d",
      //      			ymin, ymax, zmin, zmax, ny, nz) << std::endl;
		     
      h3 = new TH2D( Form("hrecoYZ_evt%lld", event_id),
		     Form("Event-%lld: Projection in YZ;Y [mm];Z [mm];Charge/bin [arb.u.]", event_id),
		     ny, ymin, ymax, nz, zmin, zmax );
    }

    // needed for method #2 only:
    // loop over matched hits and update fraction map
    std::map<MultiKey3, double, multikey3_less> fraction[3]; // for U,V,W local charge projections
    std::map<MultiKey3, TVector2, multikey3_less>::iterator it1, it2;

    for(it1=hitPos.begin(); it1!=hitPos.end(); it1++) {

      int u1=(it1->first).key1;
      int v1=(it1->first).key2;
      int w1=(it1->first).key3;
      double qtot[3] = {0., 0., 0.};  // sum of charges along three directions (for a given time range)
      double    q[3] = {0., 0., 0.};  // charge in a given strip (for a given time range)
      q[DIR_U] = GetValByStrip(DIR_U, u1, icell);
      q[DIR_V] = GetValByStrip(DIR_V, v1, icell);
      q[DIR_W] = GetValByStrip(DIR_W, w1, icell);

      // loop over directions
      for(it2=hitPos.begin(); it2!=hitPos.end(); it2++) {
	int u2=(it2->first).key1;
	int v2=(it2->first).key2;
	int w2=(it2->first).key3;
	
	if(u1==u2) {
	  qtot[DIR_V] += GetValByStrip(DIR_V, v2, icell);
	  qtot[DIR_W] += GetValByStrip(DIR_W, w2, icell);
	}
	if(v1==v2) {
	  qtot[DIR_W] += GetValByStrip(DIR_W, w2, icell);
	  qtot[DIR_U] += GetValByStrip(DIR_U, u2, icell);
	}
	if(w1==w2){
	  qtot[DIR_U] += GetValByStrip(DIR_U, u2, icell);
	  qtot[DIR_V] += GetValByStrip(DIR_V, v2, icell);
	}
      }
      fraction[DIR_U].insert(std::pair<MultiKey3, double>(it1->first, q[DIR_U] / qtot[DIR_U] ));
      fraction[DIR_V].insert(std::pair<MultiKey3, double>(it1->first, q[DIR_V] / qtot[DIR_V] ));
      fraction[DIR_W].insert(std::pair<MultiKey3, double>(it1->first, q[DIR_W] / qtot[DIR_W] ));
    }

    // loop over matched hits and fill histograms
    if(h1 && h2 && h3) {

      std::map<MultiKey3, TVector2, multikey3_less>::iterator it;
      for(it=hitPos.begin(); it!=hitPos.end(); it++) {

	double val = 0.0;

	switch (method) {

	case 0: // mehtod #1 - divide charge equally 
	  val = 
	    GetValByStrip(DIR_U, (it->first).key1, icell) / n_match[0].at((it->first).key1) +
	    GetValByStrip(DIR_V, (it->first).key2, icell) / n_match[1].at((it->first).key2) +
	    GetValByStrip(DIR_W, (it->first).key3, icell) / n_match[2].at((it->first).key3);
	  break;

	case 1: // method #2 - divide charge according to charge-fraction in two other directions

	  val = 
	    GetValByStrip(DIR_U, 
			  (it->first).key1, icell)*0.5*( fraction[DIR_V].at(it->first) + fraction[DIR_W].at(it->first) ) +
	    GetValByStrip(DIR_V, 
			  (it->first).key2, icell)*0.5*( fraction[DIR_W].at(it->first) + fraction[DIR_U].at(it->first) ) +
	    GetValByStrip(DIR_W, 
			  (it->first).key3, icell)*0.5*( fraction[DIR_U].at(it->first) + fraction[DIR_V].at(it->first) );
	  break;

	default: 
	  val=0.0;

	}; // end of switch (method)...
	
	Double_t z=icell*zfactor+zoffset;	
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


// get 3D histogram of clustered hits
TH3F *EventTPC::Get3D(SigClusterTPC &cluster, double radius, int rebin_space, int rebin_time, int method) {

  TH3F *h = NULL;

  if(!IsOK() || !cluster.IsOK() || 
     cluster.GetNhits(DIR_U)<1 || cluster.GetNhits(DIR_V)<1 || cluster.GetNhits(DIR_W)<1 ) return h;

  // loop over time slices and match hits in space
  const int time_cell_min = MAXIMUM( cluster.min_time[DIR_U], MAXIMUM( cluster.min_time[DIR_V], cluster.min_time[DIR_W] ));
  const int time_cell_max = MINIMUM( cluster.max_time[DIR_U], MINIMUM( cluster.max_time[DIR_V], cluster.max_time[DIR_W] ));

  //  std::cout << Form(">>>> EventId = %lld", event_id) << std::endl;
  //  std::cout << Form(">>>> Time cell range = [%d, %d]", time_cell_min, time_cell_max) << std::endl;

  for(int icell=time_cell_min; icell<=time_cell_max; icell++) {

    std::vector<int> hits[3] = {
      cluster.hitListByTimeDir[MultiKey2(icell, DIR_U)],
      cluster.hitListByTimeDir[MultiKey2(icell, DIR_V)],
      cluster.hitListByTimeDir[MultiKey2(icell, DIR_W)] };

    //   std::cout << Form(">>>> Number of hits: time cell=%d: U=%d / V=%d / W=%d",
    //		      icell, (int)hits[DIR_U].size(), (int)hits[DIR_V].size(), (int)hits[DIR_W].size()) << std::endl;

    // check if there is at least one hit in each direction
    if(hits[DIR_U].size()==0 || hits[DIR_V].size()==0 || hits[DIR_W].size()==0) continue;
    
    std::map<int, int> n_match[3]; // map of number of matched points for each strip, key=STRIP_NUM [1-1024]
    std::map<MultiKey3, TVector2, multikey3_less> hitPos; // key=(STRIP_NUM_U, STRIP_NUM_V, STRIP_NUM_W), value=(X [mm],Y [mm])
   
    // loop over hits and confirm matching in space
    for(int i0=0; i0<(int)hits[0].size(); i0++) {
      StripTPC *strip0 = myGeometryPtr->GetStripByDir(DIR_U, hits[0].at(i0));
      for(int i1=0; i1<(int)hits[1].size(); i1++) {
	StripTPC *strip1 = myGeometryPtr->GetStripByDir(DIR_V, hits[1].at(i1));
	for(int i2=0; i2<(int)hits[2].size(); i2++) {
	  StripTPC *strip2 = myGeometryPtr->GetStripByDir(DIR_W, hits[2].at(i2));

	  //	  std::cout << Form(">>>> Checking triplet: time_cell=%d: U=%d / V=%d / W=%d",
	  //			    icell, hits[0].at(i0), hits[1].at(i1), hits[2].at(i2)) << std::endl;

	  TVector2 pos;
	  if( myGeometryPtr->MatchCrossPoint( strip0, strip1, strip2, radius, pos )) {
	    (n_match[DIR_U])[hits[0].at(i0)]++;
	    (n_match[DIR_V])[hits[1].at(i1)]++;
	    (n_match[DIR_W])[hits[2].at(i2)]++;
	    hitPos[ MultiKey3( hits[0].at(i0), hits[1].at(i1), hits[2].at(i2)) ]=pos;
	    //	    std::cout << Form(">>>> Checking triplet: result = TRUE" ) << std::endl;
	  } else {
	    //	    std::cout << Form(">>>> Checking triplet: result = FALSE" ) << std::endl;
	  }
	}
      }
    }
    //    std::cout << Form(">>>> Number of matches: time_cell=%d, triplets=%d", icell, (int)hitPos.size()) << std::endl;
    if(hitPos.size()<1) continue;

    const Double_t zfactor=myGeometryPtr->GetVdrift()*10.0/myGeometryPtr->GetSamplingRate(); // [mm] : [vdrift]=cm/us, [rate]=MHz=1E6/s=1/us
    const Double_t zoffset=0.0; // myGeometryPtr->GetDriftCageZmin()-myGeometryPtr->GetVdrift()*10.0*myGeometryPtr->GetTriggerDelay(); // [mm]

    // book 3D histogram before first fill
    if(h==NULL) {

      StripTPC* s[6] = {
	myGeometryPtr->GetStripByDir(DIR_U, 1),
	myGeometryPtr->GetStripByDir(DIR_U, myGeometryPtr->GetDirNstrips(DIR_U)),
	myGeometryPtr->GetStripByDir(DIR_V, 1),
	myGeometryPtr->GetStripByDir(DIR_V, myGeometryPtr->GetDirNstrips(DIR_V)),
	myGeometryPtr->GetStripByDir(DIR_W, 1),
	myGeometryPtr->GetStripByDir(DIR_W, myGeometryPtr->GetDirNstrips(DIR_W))
      };

      double xmin=1E30;
      double xmax=-1E30;
      double ymin=1E30;
      double ymax=-1E30;
      double zmin=0.0-0.5;  // time_cell_min;
      double zmax=511.+0.5; // time_cell_max;

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
	/*
	if(s[i]->Offset().X()>xmax) xmax=s[i]->Offset().X();
	if(s[i]->Offset().X()<xmin) xmin=s[i]->Offset().X();
	if(s[i]->Offset().Y()>ymax) ymax=s[i]->Offset().Y();
	if(s[i]->Offset().Y()<ymin) ymin=s[i]->Offset().Y();
	*/
      }
      xmin-=myGeometryPtr->GetStripPitch()*0.3;
      xmax+=myGeometryPtr->GetStripPitch()*0.7;
      ymin-=myGeometryPtr->GetPadPitch()*0.3;
      ymax+=myGeometryPtr->GetPadPitch()*0.7;
      
      int nx = (int)( (xmax-xmin)/myGeometryPtr->GetStripPitch()-1 );
      int ny = (int)( (ymax-ymin)/myGeometryPtr->GetPadPitch()-1 );
      int nz = (int)( zmax-zmin );

      zmin = zmin*zfactor + zoffset; // [mm]
      zmax = zmax*zfactor + zoffset; // [mm]

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

      h = new TH3F( Form("hreco3D_evt%lld", event_id),
		    Form("Event-%lld: 3D reco in XYZ;X [mm];Y [mm];Z [mm]", event_id),
		    nx, xmin, xmax, ny, ymin, ymax, nz, zmin, zmax );
    }

    // needed for method #2 only:
    // loop over matched hits and update fraction map
    std::map<MultiKey3, double, multikey3_less> fraction[3]; // for U,V,W local charge projections
    std::map<MultiKey3, TVector2, multikey3_less>::iterator it1, it2;

    for(it1=hitPos.begin(); it1!=hitPos.end(); it1++) {

      int u1=(it1->first).key1;
      int v1=(it1->first).key2;
      int w1=(it1->first).key3;
      double qtot[3] = {0., 0., 0.};  // sum of charges along three directions (for a given time range)
      double    q[3] = {0., 0., 0.};  // charge in a given strip (for a given time range)
      q[DIR_U] = GetValByStrip(DIR_U, u1, icell);
      q[DIR_V] = GetValByStrip(DIR_V, v1, icell);
      q[DIR_W] = GetValByStrip(DIR_W, w1, icell);

      // loop over directions
      for(it2=hitPos.begin(); it2!=hitPos.end(); it2++) {
	int u2=(it2->first).key1;
	int v2=(it2->first).key2;
	int w2=(it2->first).key3;
	
	if(u1==u2) {
	  qtot[DIR_V] += GetValByStrip(DIR_V, v2, icell);
	  qtot[DIR_W] += GetValByStrip(DIR_W, w2, icell);
	}
	if(v1==v2) {
	  qtot[DIR_W] += GetValByStrip(DIR_W, w2, icell);
	  qtot[DIR_U] += GetValByStrip(DIR_U, u2, icell);
	}
	if(w1==w2){
	  qtot[DIR_U] += GetValByStrip(DIR_U, u2, icell);
	  qtot[DIR_V] += GetValByStrip(DIR_V, v2, icell);
	}
      }
      fraction[DIR_U].insert(std::pair<MultiKey3, double>(it1->first, q[DIR_U] / qtot[DIR_U] ));
      fraction[DIR_V].insert(std::pair<MultiKey3, double>(it1->first, q[DIR_V] / qtot[DIR_V] ));
      fraction[DIR_W].insert(std::pair<MultiKey3, double>(it1->first, q[DIR_W] / qtot[DIR_W] ));
    }

    // loop over matched hits and fill histograms
    if(h) {

      std::map<MultiKey3, TVector2, multikey3_less>::iterator it;
      for(it=hitPos.begin(); it!=hitPos.end(); it++) {

	double val = 0.0;

	switch (method) {

	case 0: // mehtod #1 - divide charge equally 
	  val = 
	    GetValByStrip(DIR_U, (it->first).key1, icell) / n_match[0].at((it->first).key1) +
	    GetValByStrip(DIR_V, (it->first).key2, icell) / n_match[1].at((it->first).key2) +
	    GetValByStrip(DIR_W, (it->first).key3, icell) / n_match[2].at((it->first).key3);
	  break;

	case 1: // method #2 - divide charge according to charge-fraction in two other directions

	  val = 
	    GetValByStrip(DIR_U, 
			  (it->first).key1, icell)*0.5*( fraction[DIR_V].at(it->first) + fraction[DIR_W].at(it->first) ) +
	    GetValByStrip(DIR_V, 
			  (it->first).key2, icell)*0.5*( fraction[DIR_W].at(it->first) + fraction[DIR_U].at(it->first) ) +
	    GetValByStrip(DIR_W, 
			  (it->first).key3, icell)*0.5*( fraction[DIR_U].at(it->first) + fraction[DIR_V].at(it->first) );
	  break;
	  
	default: 
	  val=0.0;
	  
	}; // end of switch (method)...

	Double_t z=icell*zfactor+zoffset;
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
    StripTPC* s[6] = {
      myGeometryPtr->GetStripByDir(DIR_U, 1),
      myGeometryPtr->GetStripByDir(DIR_U, myGeometryPtr->GetDirNstrips(DIR_U)),
      myGeometryPtr->GetStripByDir(DIR_V, 1),
      myGeometryPtr->GetStripByDir(DIR_V, myGeometryPtr->GetDirNstrips(DIR_V)),
      myGeometryPtr->GetStripByDir(DIR_W, 1),
      myGeometryPtr->GetStripByDir(DIR_W, myGeometryPtr->GetDirNstrips(DIR_W))
    };
    double xmin=1E30;
    double xmax=-1E30;
    double ymin=1E30;
    double ymax=-1E30;
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
    
    std::cout << Form(">>>> U-V correlation test histogram: range=[%lf, %lf] x [%lf, %lf], nx=%d, ny=%d",
		      xmin, xmax, ymin, ymax, nx, ny) << std::endl;
    
    h = new TH2D( "h_test_XY_UV_2d",
		  "Intersection of U-V strips;X [mm];Y [mm]; V strip no.",
		  nx, xmin, xmax, ny, ymin, ymax );
  }

  // loop over all strip numbers and check hits strip intersection
  for(int i0=1; i0<=myGeometryPtr->GetDirNstrips(DIR_U); i0++) {
    StripTPC *strip0 = myGeometryPtr->GetStripByDir(DIR_U, i0);
    for(int i1=1; i1<(int)myGeometryPtr->GetDirNstrips(DIR_V); i1++) {
      StripTPC *strip1 = myGeometryPtr->GetStripByDir(DIR_V, i1);
      TVector2 pos;
      if( myGeometryPtr->GetCrossPoint( strip0, strip1, pos) ) {
    	h->SetBinContent( h->FindBin( pos.X(), pos.Y() ), 1.*i0);
      }
    }
  }

  std::cout << ">>>> U-V correlation test histogram: END" << std::endl;

  return h;
}

TH2D *EventTPC::GetXY_TestVW(TH2D *h) { // test (unphysical) histogram
  //  TH2D *h = NULL;

  if(!IsOK()) return h;

  if(h==NULL) {
    StripTPC* s[6] = {
      myGeometryPtr->GetStripByDir(DIR_U, 1),
      myGeometryPtr->GetStripByDir(DIR_U, myGeometryPtr->GetDirNstrips(DIR_U)),
      myGeometryPtr->GetStripByDir(DIR_V, 1),
      myGeometryPtr->GetStripByDir(DIR_V, myGeometryPtr->GetDirNstrips(DIR_V)),
      myGeometryPtr->GetStripByDir(DIR_W, 1),
      myGeometryPtr->GetStripByDir(DIR_W, myGeometryPtr->GetDirNstrips(DIR_W))
    };
    double xmin=1E30;
    double xmax=-1E30;
    double ymin=1E30;
    double ymax=-1E30;
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
    
    std::cout << Form(">>>> V-W correlation test histogram: range=[%lf, %lf] x [%lf, %lf], nx=%d, ny=%d",
		      xmin, xmax, ymin, ymax, nx, ny) << std::endl;
    
    h = new TH2D( "h_test_XY_VW_2d",
		  "Intersection of V-W strips;X [mm];Y [mm]; V strip no.",
		  nx, xmin, xmax, ny, ymin, ymax );
  }

  // loop over all strip numbers and check hits strip intersection
  for(int i0=1; i0<(int)myGeometryPtr->GetDirNstrips(DIR_V); i0++) {
    StripTPC *strip0 = myGeometryPtr->GetStripByDir(DIR_V, i0);
    for(int i1=1; i1<(int)myGeometryPtr->GetDirNstrips(DIR_W); i1++) {
      StripTPC *strip1 = myGeometryPtr->GetStripByDir(DIR_W, i1);
      TVector2 pos;
      if( myGeometryPtr->GetCrossPoint( strip0, strip1, pos) ) {
	h->SetBinContent( h->FindBin( pos.X(), pos.Y() ), 1.*i0);
      }
    }
  }

  return h;
}

TH2D *EventTPC::GetXY_TestWU(TH2D *h) { // test (unphysical) histogram
  //  TH2D *h = NULL;

  if(!IsOK()) return h;

  if(h==NULL) {
    StripTPC* s[6] = {
      myGeometryPtr->GetStripByDir(DIR_U, 1),
      myGeometryPtr->GetStripByDir(DIR_U, myGeometryPtr->GetDirNstrips(DIR_U)),
      myGeometryPtr->GetStripByDir(DIR_V, 1),
      myGeometryPtr->GetStripByDir(DIR_V, myGeometryPtr->GetDirNstrips(DIR_V)),
      myGeometryPtr->GetStripByDir(DIR_W, 1),
      myGeometryPtr->GetStripByDir(DIR_W, myGeometryPtr->GetDirNstrips(DIR_W))
    };
    double xmin=1E30;
    double xmax=-1E30;
    double ymin=1E30;
    double ymax=-1E30;
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
    
    std::cout << Form(">>>> W-U correlation test histogram: range=[%lf, %lf] x [%lf, %lf], nx=%d, ny=%d",
    		      xmin, xmax, ymin, ymax, nx, ny) << std::endl;
    
    h = new TH2D( "h_test_XY_WU_2d",
		  "Intersection of W-U strips;X [mm];Y [mm]; W strip no.",
		  nx, xmin, xmax, ny, ymin, ymax );
  }

  // loop over all strip numbers and check hits strip intersection
  for(int i0=1; i0<(int)myGeometryPtr->GetDirNstrips(DIR_W); i0++) {
    StripTPC *strip0 = myGeometryPtr->GetStripByDir(DIR_W, i0);
    for(int i1=1; i1<(int)myGeometryPtr->GetDirNstrips(DIR_U); i1++) {
      StripTPC *strip1 = myGeometryPtr->GetStripByDir(DIR_U, i1);
      TVector2 pos;
      if( myGeometryPtr->GetCrossPoint( strip0, strip1, pos) ) {
	h->SetBinContent( h->FindBin( pos.X(), pos.Y() ), 1.*i0);
      }
    }
  }

  return h;
}

