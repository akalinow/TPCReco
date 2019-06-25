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

#include "SigClusterTPC.h"
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

