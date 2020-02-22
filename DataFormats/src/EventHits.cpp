#include "EventHits.h"
#include "EventCharges.h"

class { //object keeping temporary envelope containers outside of EventHits
	friend class EventHits;
private:
	std::set<std::tuple<direction, int, int>> hitList; // temporary list of selected envelope space-time cells for further analysis where return
								 // value=key(STRIP_DIR [0-2], STRIP_NUM [1-1024], REBIN_TIME_CELL [0-511])
	std::set< std::tuple<int, direction, int>> hitListByTimeDir; // same temporary list of selected envelope space-time cells as a map
														  // with key=(REBIN_TIME_CELL [0-511], STRIP_DIR [0-2]) 
														  // that returns vector of STRIP_NUMBERS 
} envelope;

/* ============= SPACE-TIME CLUSTER CLASS ===========*/

void EventHits::AddHit(std::tuple<direction, int, int> hit) {  // valid range [0-2][1-1024][0-511]
	auto strip_dir = std::get<0>(hit);
	auto strip_number = std::get<1>(hit);
	auto time_cell = std::get<2>(hit);
	if (time_cell < 0 || time_cell >= Geometry().GetAgetNtimecells() ||
		strip_number < 1 || strip_number > Geometry().GetDirNstrips(strip_dir)) return;

	hitListByTimeDir.insert({ time_cell,strip_dir,strip_number });
	hitList.insert(hit);
}

void EventHits::AddEnvelopeHit(std::tuple<direction, int, int> hit) {  // valid range [0-2][1-1024][0-511]
	auto strip_dir = std::get<0>(hit);
	auto strip_number = std::get<1>(hit);
	auto time_cell = std::get<2>(hit);
	if (time_cell < 0 || time_cell >= Geometry().GetAgetNtimecells() ||
		strip_number < 1 || strip_number > Geometry().GetDirNstrips(strip_dir)) return;

	envelope.hitList.insert(hit);
	envelope.hitListByTimeDir.insert({ time_cell,strip_dir,strip_number });
}

void EventHits::Combine() {
	hitList.insert(envelope.hitList.begin(), envelope.hitList.end()); // insert envelope hit list
	hitListByTimeDir.insert(envelope.hitListByTimeDir.begin(), envelope.hitListByTimeDir.end());
	envelope.hitList.clear(); //clear envelope hit list
	envelope.hitListByTimeDir.clear();
}

size_t EventHits::GetNhits(direction strip_dir) const {   // # of hits in a given direction
	return std::distance(hitList.lower_bound({ strip_dir, std::numeric_limits<int>::min(),std::numeric_limits<int>::min() }), hitList.upper_bound({ strip_dir, std::numeric_limits<int>::max(),std::numeric_limits<int>::max() }));
}
