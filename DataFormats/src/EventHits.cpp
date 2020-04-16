#include "EventHits.h"
#include "EventCharges.h"

class { //object keeping temporary envelope containers outside of EventHits
	friend class EventHits;
private:
	hitList_type hitList; // temporary list of selected envelope space-time cells for further analysis where return
								 // value=key(STRIP_DIR [0-2], STRIP_NUM [1-1024], REBIN_TIME_CELL [0-511])
	hitListByTimeDir_type hitListByTimeDir; // same temporary list of selected envelope space-time cells as a map
														  // with key=(REBIN_TIME_CELL [0-511], STRIP_DIR [0-2], STRIP_NUM [1-1024]) 
														  // that returns vector of STRIP_NUMBERS 
} envelope;

/* ============= SPACE-TIME CLUSTER CLASS ===========*/

void EventHits::AddHit(position hit) {  // valid range [0-2][1-1024][0-511]
	auto strip_dir = std::get<0>(hit);
	auto section = std::get<1>(hit);
	auto strip_number = std::get<2>(hit);
	auto time_cell = std::get<3>(hit);
	if (time_cell < 0 || time_cell >= Geometry().GetAgetNtimecells() ||
		strip_number < 1 || strip_number > Geometry().GetDirNstrips(strip_dir)) return;

	hitListByTimeDir.insert(to_by_time(to_reduced(hit)));
	hitList.insert(to_reduced(hit));
}

void EventHits::AddEnvelopeHit(position hit) {  // valid range [0-2][1-1024][0-511]
	auto strip_dir = std::get<0>(hit);
	auto section = std::get<1>(hit);
	auto strip_number = std::get<2>(hit);
	auto time_cell = std::get<3>(hit);
	if (time_cell < 0 || time_cell >= Geometry().GetAgetNtimecells() ||
		strip_number < 1 || strip_number > Geometry().GetDirNstrips(strip_dir)) return;

	envelope.hitList.insert(to_reduced(hit));
	envelope.hitListByTimeDir.insert(to_by_time(to_reduced(hit)));
}

void EventHits::Combine() {
	hitList.insert(envelope.hitList.begin(), envelope.hitList.end()); // insert envelope hit list
	hitListByTimeDir.insert(envelope.hitListByTimeDir.begin(), envelope.hitListByTimeDir.end());
	envelope.hitList.clear(); //clear envelope hit list
	envelope.hitListByTimeDir.clear();
}

size_t EventHits::GetNhits(direction strip_dir) const {   // # of hits in a given direction
	return std::distance(
		hitList.lower_bound(position_reduced{ strip_dir, std::numeric_limits<int>::min(), std::numeric_limits<int>::min() }),
		hitList.upper_bound(position_reduced{ strip_dir, std::numeric_limits<int>::max(), std::numeric_limits<int>::max() }));
}

size_t EventHits::GetNhits() const {
	auto hits = size_t();
	for (auto dir : dirs) {
		hits += GetNhits(dir);
	}
	return hits;
}