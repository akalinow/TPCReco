#include "SigClusterTPC.h"
#include "EventTPC.h"
/* ============= SPACE-TIME CLUSTER CLASS ===========*/

SigClusterTPC::SigClusterTPC(EventTPC& e)
  : 
    evt_ref(e),
    initOK(false) {
  
  for(int idir=0; idir<3; idir++) {
    nhits[idir]=0;
    min_strip[idir]=-1;
    max_strip[idir]=-1;
    min_time[idir]=-1;
    max_time[idir]=-1;
  }
  hitList.clear();
  hitListByTimeDir.clear();
  hitListByDir.clear();
  if(evt_ref.IsOK()) initOK=true;
  
}

// list of SELECTED hits corresponding to a given STRIP_DIR[0-2],
// return value=key(REBIN_TIME_CELL [0-511], STRIP_NUM [1-1024])
std::set<MultiKey2> SigClusterTPC::GetHitListByDir(projection strip_dir) const{
	if (IsDIR_UVW(strip_dir)) {
		return hitListByDir.find(strip_dir)->second;
	};
	return std::set<MultiKey2>();
}

 const std::map<MultiKey2, std::vector<int>, multikey2_less> & SigClusterTPC::GetHitListByTimeDir() const{
  return hitListByTimeDir;
}

bool SigClusterTPC::AddByStrip(projection strip_dir, int strip_number, int time_cell) {  // valid range [0-2][1-1024][0-511]
  if(!IsOK() || 
     time_cell<0 || time_cell>=evt_ref.GetGeoPtr()->GetAgetNtimecells() || 
     strip_number<1 || strip_number>evt_ref.GetGeoPtr()->GetDirNstrips(strip_dir)) return false;
  if (IsDIR_UVW(strip_dir)) {
	  MultiKey3 mkey3(int(strip_dir), strip_number, time_cell);
    MultiKey2 mkey2(time_cell, int(strip_dir));

    MultiKey2 mkey_total(int(strip_dir), strip_number);
    MultiKey2 mkey_total2(int(strip_dir), time_cell);
    MultiKey2 mkey_maxval(int(strip_dir), strip_number);

    MultiKey2 mkey2_CellStrip(time_cell, strip_number);

    // hitList - check if hit is unique
    if(hitList.find(mkey3)==hitList.end()) {

      // add new hit
      hitList.insert(mkey3);
      hitListByTimeDir[mkey2].push_back(strip_number);
      hitListByDir[strip_dir].insert(mkey2_CellStrip);

      // update hit statistics
      nhits[int(strip_dir)]++;

      // update space-time envelope
      max_strip[int(strip_dir)] = std::max(max_strip[int(strip_dir)], strip_number);
      if( min_strip[int(strip_dir)] > strip_number || min_strip[int(strip_dir)]<1 ) min_strip[int(strip_dir)] = strip_number;
	  max_time[int(strip_dir)] = std::max(max_time[int(strip_dir)], time_cell);
      if( min_time[int(strip_dir)] > time_cell || min_time[int(strip_dir)]<1 ) min_time[int(strip_dir)] = time_cell;

      double val = evt_ref.GetValByStrip(strip_dir, strip_number, time_cell);

    }
    return true;
  }
  return false;  
}

bool SigClusterTPC::CheckByStrip(projection strip_dir, int strip_number, int time_cell) const{  // valid range [0-2][1-1024][0-511]
  if(!IsOK() || time_cell<0 || time_cell>=512 || strip_number<1 || strip_number>evt_ref.GetGeoPtr()->GetDirNstrips(strip_dir)) return false;
  if (IsDIR_UVW(strip_dir)) {
      auto temp = hitListByDir.find(strip_dir);
      return temp->second.find(MultiKey2(time_cell, strip_number)) != temp->second.end();
  };
  return false;  
}

long SigClusterTPC::GetNhits(projection strip_dir) const{   // # of hits in a given direction
	if (IsDIR_UVW(strip_dir) && IsOK()) {
        return nhits[int(strip_dir)];
    };
    return ERROR;
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