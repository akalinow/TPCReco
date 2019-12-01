#include "SigClusterTPC.h"
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
  if(evt_ptr != nullptr && evt_ptr->IsOK()) initOK=true;
  
}

// list of SELECTED hits corresponding to a given STRIP_DIR[0-2],
// return value=key(REBIN_TIME_CELL [0-511], STRIP_NUM [1-1024])
std::vector<MultiKey2> SigClusterTPC::GetHitListByDir(projection strip_dir) const{
	if (IsDIR_UVW(strip_dir)) {
		return hitListByDir.find(int(strip_dir))->second;
	};
	return std::vector<MultiKey2>();
}

 const std::map<MultiKey2, std::vector<int>, multikey2_less> & SigClusterTPC::GetHitListByTimeDir() const{
  return hitListByTimeDir;
}

bool SigClusterTPC::AddByStrip(projection strip_dir, int strip_number, int time_cell) {  // valid range [0-2][1-1024][0-511]
  if(!IsOK() || 
     time_cell<0 || time_cell>=evt_ptr->GetGeoPtr()->GetAgetNtimecells() || 
     strip_number<1 || strip_number>evt_ptr->GetGeoPtr()->GetDirNstrips(strip_dir)) return false;
  if (IsDIR_UVW(strip_dir)) {
	  MultiKey3 mkey3(int(strip_dir), strip_number, time_cell);
    MultiKey2 mkey2(time_cell, int(strip_dir));

    MultiKey2 mkey_total(int(strip_dir), strip_number);
    MultiKey2 mkey_total2(int(strip_dir), time_cell);
    MultiKey2 mkey_maxval(int(strip_dir), strip_number);

    MultiKey2 mkey2_CellStrip(time_cell, strip_number);

    // hitList - check if hit is unique
    if(find_if(hitList.begin(), hitList.end(), mkey3)==hitList.end()) {

      // add new hit
      hitList.push_back(mkey3);
      hitListByTimeDir[mkey2].push_back(strip_number);
      hitListByDir[int(strip_dir)].push_back(mkey2_CellStrip);

      // update hit statistics
      MultiKey2 mkey(int(strip_dir), strip_number);
      nhits[int(strip_dir)]++;
      if(nhitsMap.find(mkey)==nhitsMap.end()) {
	nstrips[int(strip_dir)]++;  // increment counter only once for a given strip
      }
      nhitsMap[mkey]++; // increment counter for each time-cell of a given strip

      // update space-time envelope
      max_strip[int(strip_dir)] = std::max(max_strip[int(strip_dir)], strip_number);
      if( min_strip[int(strip_dir)] > strip_number || min_strip[int(strip_dir)]<1 ) min_strip[int(strip_dir)] = strip_number;
	  max_time[int(strip_dir)] = std::max(max_time[int(strip_dir)], strip_number);
      if( min_time[int(strip_dir)] > time_cell || min_time[int(strip_dir)]<1 ) min_time[int(strip_dir)] = time_cell;
      glb_max_time = std::max(glb_max_time, time_cell);
      if( glb_min_time > time_cell || glb_min_time<1 ) glb_min_time = time_cell;

      double val = evt_ptr->GetValByStrip(strip_dir, strip_number, time_cell);

      // update charge integrals

      tot_charge[int(strip_dir)] += val;
      glb_tot_charge += val;

      // totalChargeMap - check if strip exists
      auto it_total=totalChargeMap.find(mkey_total);
      if( it_total == totalChargeMap.end() ) {
	// add new total charge per strip
	totalChargeMap[mkey_total]=val;
      } else {
	// update already existing total charge per strip
	it_total->second += val;
      }
      
      // totalChargeMap2 - check if strip exists
      auto it_total2 = totalChargeMap2.find(mkey_total2);
      if(it_total2 == totalChargeMap2.end() ) {
	// add new total value per strip
	totalChargeMap2[mkey_total2]=val;
      } else {
	// update already existing total value per strip
	it_total2->second += val;
      }
      
      // totalChargeMap3 - check if strip exists
      auto it_total3 = totalChargeMap3.find(time_cell);
      if(it_total3 == totalChargeMap3.end() ) {
	// add new total value per strip
	totalChargeMap3[time_cell]=val;
      } else {
	// update already existing total value per strip
	it_total3->second += val;
      }

      // update charge maxima

      // maxChargeMap - check if strip exists
      auto it_maxval = maxChargeMap.find(mkey_maxval);
      if(it_maxval == maxChargeMap.end() ) {
	// add new max value per strip
	maxChargeMap[mkey_maxval]=val;
      } else {
	// update already existing max value per strip
	if(val > it_maxval->second) it_maxval->second = val;
      }
    
      if( val > max_charge[int(strip_dir)] ) {
	max_charge[int(strip_dir)]=val;
	max_charge_timing[int(strip_dir)]=time_cell;
	max_charge_strip[int(strip_dir)]=strip_number;
	if( val > glb_max_charge ) {
	  glb_max_charge=val;
	  glb_max_charge_timing=time_cell;
	  glb_max_charge_channel=evt_ptr->GetGeoPtr()->Global_strip2normal(int(strip_dir), strip_number);
	}
      }

    }
    return true;
  }
  return false;  
}               
               
bool SigClusterTPC::AddByStrip(std::shared_ptr<StripTPC> strip, int time_cell) {  // valid range [0-511]
  if(strip) return AddByStrip(static_cast<projection>(strip->Dir()), strip->Num(), time_cell);
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

bool SigClusterTPC::CheckByStrip(projection strip_dir, int strip_number, int time_cell) const{  // valid range [0-2][1-1024][0-511]
  if(!IsOK() || time_cell<0 || time_cell>=512 || strip_number<1 || strip_number>evt_ptr->GetGeoPtr()->GetDirNstrips(strip_dir)) return false;
  if (IsDIR_UVW(strip_dir)) {
	  return std::find_if(hitListByDir.find(int(strip_dir))->second.begin(),
			     hitListByDir.find(int(strip_dir))->second.end(),
			     MultiKey2(time_cell, strip_number))!=hitListByDir.find(int(strip_dir))->second.end();
  };
  return false;  
}

bool SigClusterTPC::CheckByStrip(std::shared_ptr<StripTPC> strip, int time_cell) const{  // valid range [0-511]
  if(strip != nullptr) return CheckByStrip(static_cast<projection>(strip->Dir()), strip->Num(), time_cell);
  return false;
}

bool SigClusterTPC::CheckByGlobalChannel(int glb_channel_idx, int time_cell) const{  // valid range [0-1023][0-511]
  return CheckByStrip(evt_ptr->GetGeoPtr()->GetStripByGlobal(glb_channel_idx), time_cell);
}

bool SigClusterTPC::CheckByGlobalChannel_raw(int glb_raw_channel_idx, int time_cell) const{  // valid range [0-(1023+ASAD_N[0]*4+...)][0-511]
  return CheckByStrip(evt_ptr->GetGeoPtr()->GetStripByGlobal_raw(glb_raw_channel_idx), time_cell);
}

bool SigClusterTPC::CheckByAgetChannel(int cobo_idx, int asad_idx, int aget_idx, int channel_idx, int time_cell) const{  // valid range [0-1][0-3][0-3][0-63][0-511]
  return CheckByStrip(evt_ptr->GetGeoPtr()->GetStripByAget(cobo_idx, asad_idx, aget_idx, channel_idx), time_cell);
}

bool SigClusterTPC::CheckByAgetChannel_raw(int cobo_idx, int asad_idx, int aget_idx, int raw_channel_idx, int time_cell) const{  // valid range [0-1][0-3][0-3][0-67][0-511]
  return CheckByStrip(evt_ptr->GetGeoPtr()->GetStripByAget_raw(cobo_idx, asad_idx, aget_idx, raw_channel_idx), time_cell);
}

long SigClusterTPC::GetNhitsByStrip(projection strip_dir, int strip_num) const{   // # of hits in a given strip
  if(!IsOK() || strip_num<1 || strip_num>evt_ptr->GetGeoPtr()->GetDirNstrips(strip_dir)) return ERROR;
  if (IsDIR_UVW(strip_dir)) {
	  MultiKey2 mkey(int(strip_dir), strip_num);

    // check if hit exists
    std::map<MultiKey2, int, multikey2_less>::const_iterator it = nhitsMap.find(mkey);
    if(it!=nhitsMap.end()) {
      return it->second;
    }
    else return 0;
  };
  return ERROR;
}

long SigClusterTPC::GetNhits(projection strip_dir) const{   // # of hits in a given direction
	if (IsDIR_UVW(strip_dir) && IsOK()) {
	  return nhits[int(strip_dir)];
  };
  return ERROR;
}

long SigClusterTPC::GetNhits() const{  // global # of hits
  if(!IsOK()) return ERROR;
  return nhits[int(projection::DIR_U)]+nhits[int(projection::DIR_V)]+nhits[int(projection::DIR_W)];
}

int SigClusterTPC::GetMultiplicity(projection strip_dir) const{  // # of strips with hits in a given direction
	if (IsDIR_UVW(strip_dir) && IsOK()) {
	  return nstrips[int(strip_dir)];
  };
  return ERROR;
}

int SigClusterTPC::GetMultiplicity() const{  // global # of strips with hits from all strips
  if(!IsOK()) return ERROR;
  return nstrips[int(projection::DIR_U)]+nstrips[int(projection::DIR_V)]+nstrips[int(projection::DIR_W)];
}  

int SigClusterTPC::GetMinStrip(projection strip_dir) const{  // minimal strip number in a given direction
  if (IsDIR_UVW(strip_dir) && IsOK()) {
	  return min_strip[int(strip_dir)];
  };
  return ERROR;
}

int SigClusterTPC::GetMaxStrip(projection strip_dir) const{ // maximal strip number in a given direction
	if (IsDIR_UVW(strip_dir) && IsOK()) {
	  return max_strip[int(strip_dir)];
  };
  return ERROR;
}

int SigClusterTPC::GetMinTime(projection strip_dir) const{  // minimal time cell in a given direction
	if (IsDIR_UVW(strip_dir) && IsOK()) {
	  return min_time[int(strip_dir)];
  };
  return ERROR;
}

int SigClusterTPC::GetMaxTime(projection strip_dir) const{  // maximal time cell in a given direction
	if (IsDIR_UVW(strip_dir) && IsOK()) {
	  return max_time[int(strip_dir)];
  };
  return ERROR;
}

int SigClusterTPC::GetMinTime() const{  // minimal time cell from all strips
  if(!IsOK()) return ERROR;
  return glb_min_time;
}

int SigClusterTPC::GetMaxTime() const{  // maximal time cell from all cluster hits
  if(!IsOK()) return ERROR;
  return glb_max_time;
}

double SigClusterTPC::GetMaxCharge(projection strip_dir, int strip_number) const{ // maximal charge from cluster hits in a given strip (if any)
	if (IsDIR_UVW(strip_dir) && IsOK()) {
		MultiKey2 mkey(int(strip_dir), strip_number);

    // check if hit is unique
		std::map<MultiKey2, double, multikey2_less>::const_iterator it = maxChargeMap.find(mkey);
		if(it!=maxChargeMap.end()) {
			return it->second;
		}
		else return 0.0;
	};
	return 0.0;    
}

double SigClusterTPC::GetMaxCharge(projection strip_dir) const{  // maximal charge from all cluster hits in a given direction
	if (IsDIR_UVW(strip_dir) && IsOK()) {
	  return max_charge[int(strip_dir)];
  };
  return 0.0;    
}

double SigClusterTPC::GetMaxCharge() const{  // maximal charge from all cluster hits
  if(!IsOK()) return 0.0;
  return glb_max_charge;
}

int SigClusterTPC::GetMaxChargeTime(projection strip_dir) const{  // arrival time of the maximal charge from cluster hits in a given direction
	if (IsDIR_UVW(strip_dir) && IsOK()) {
	  return max_charge_timing[int(strip_dir)];
  };
  return ERROR;
}

int SigClusterTPC::GetMaxChargeStrip(projection strip_dir) const{  // strip number with the maximal charge from cluster hits in a given direction
	if (IsDIR_UVW(strip_dir) && IsOK()) {
	  return max_charge_strip[int(strip_dir)];
  };
  return ERROR;
}

int SigClusterTPC::GetMaxChargeTime() const{  // arrival time of the maximal charge from all cluster hits
  if(!IsOK()) return ERROR;
  return glb_max_charge_timing;
}

int SigClusterTPC::GetMaxChargeChannel() const{  // global channel number with the maximal charge from cluster hits
  if(!IsOK()) return ERROR;
  return glb_max_charge_channel;
}

double SigClusterTPC::GetTotalCharge(projection strip_dir, int strip_number) const{ // charge integral from cluster hits in a given strip (if any)
	if (IsDIR_UVW(strip_dir) && IsOK()) {
    MultiKey2 mkey(int(strip_dir), strip_number);

    // check if hit is unique
    auto it = totalChargeMap.find(mkey);
    if(it!=totalChargeMap.end()) {
      return it->second;
    }
    else return 0.0;
  };
  return 0.0;    
}

double SigClusterTPC::GetTotalCharge(projection strip_dir) const{  // charge integral from all cluster hits in a given direction
	if (IsDIR_UVW(strip_dir) && IsOK()) {
	  return tot_charge[int(strip_dir)];
  };
  return 0.0;    
}

double SigClusterTPC::GetTotalCharge() const{   // charge integral from all cluster hits
  if(!IsOK()) return 0.0;
  return glb_tot_charge;
}

double SigClusterTPC::GetTotalChargeByTimeCell(projection strip_dir, int time_cell) const{ // charge integral from a single time cell from all cluster hits in a given direction
  if(!IsOK() || time_cell<0 || time_cell>=512) {
    return 0.0;
  }
  if (IsDIR_UVW(strip_dir)) {
	  MultiKey2 mkey(int(strip_dir), time_cell);

    // check if time slice is unique
    auto it = totalChargeMap2.find(mkey);
    if(it!=totalChargeMap2.end() ) {
      return it->second;
    }
    else return 0.0;
  };
  return 0.0;
}

double SigClusterTPC::GetTotalChargeByTimeCell(int time_cell) const{ // charge integral from a single time cell from all cluster hits
  if(!IsOK()) return 0.0;
  
  // check if time slice is unique
  auto it = totalChargeMap3.find(time_cell);
  if(it!=totalChargeMap3.end()){
    return it->second;
  }
  return 0.0;
}