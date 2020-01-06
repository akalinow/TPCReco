#include "SigClusterTPC.h"
#include "EventTPC.h"
/* ============= SPACE-TIME CLUSTER CLASS ===========*/

SigClusterTPC::SigClusterTPC(EventTPC& e)
  : evt_ref(e) {}

 decltype(SigClusterTPC::hitListByTimeDir) & SigClusterTPC::GetHitListByTimeDir() {
  return hitListByTimeDir;
}

 bool SigClusterTPC::AddByStrip(projection strip_dir, int strip_number, int time_cell) {  // valid range [0-2][1-1024][0-511]
	 if (time_cell < 0 || time_cell >= evt_ref.GetGeoPtr()->GetAgetNtimecells() ||
		 strip_number<1 || strip_number>evt_ref.GetGeoPtr()->GetDirNstrips(strip_dir) ||
		 !IsDIR_UVW(strip_dir)) return false;

	 hitListByTimeDir[time_cell][strip_dir].insert(strip_number);
     hitList[strip_dir][strip_number].insert(time_cell);
	 return true;
 }

 void SigClusterTPC::UpdateStats() {
     // count hits
     for (auto& by_dir : hitList) {
         nhits[int(by_dir.first)] = 0;
         for (auto& by_strip_num : by_dir.second) {
             nhits[int(by_dir.first)] += by_strip_num.second.size();
         }
     }
 }

size_t SigClusterTPC::GetNhits(projection strip_dir) const {   // # of hits in a given direction
	return nhits[int(strip_dir)];
}