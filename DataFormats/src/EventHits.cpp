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

EventHits::EventHits(EventCharges& e)
	: evt_ref(e) {}

bool EventHits::AddHit(std::tuple<direction, int, int> hit) {  // valid range [0-2][1-1024][0-511]
	auto strip_dir = std::get<0>(hit);
	auto strip_number = std::get<1>(hit);
	auto time_cell = std::get<2>(hit);
	if (time_cell < 0 || time_cell >= Geometry().GetAgetNtimecells() ||
		strip_number < 1 || strip_number > Geometry().GetDirNstrips(strip_dir)) return false;

	hitListByTimeDir.insert({ time_cell,strip_dir,strip_number });
	hitList.insert(hit);
	return true;
}

bool EventHits::AddEnvelopeHit(std::tuple<direction, int, int> hit) {  // valid range [0-2][1-1024][0-511]
	auto strip_dir = std::get<0>(hit);
	auto strip_number = std::get<1>(hit);
	auto time_cell = std::get<2>(hit);
	if (time_cell < 0 || time_cell >= Geometry().GetAgetNtimecells() ||
		strip_number < 1 || strip_number > Geometry().GetDirNstrips(strip_dir)) return false;

	envelope.hitList.insert(hit);
	envelope.hitListByTimeDir.insert({ time_cell,strip_dir,strip_number });
	return true;
}

void EventHits::UpdateStats() {
	// count hits
	for (auto dir : dir_vec_UVW) {
		nhits[dir] = std::distance(hitList.lower_bound({ dir, std::numeric_limits<int>::min(),std::numeric_limits<int>::min() }), hitList.upper_bound({ dir, std::numeric_limits<int>::max(),std::numeric_limits<int>::max() }));
	}
}

void EventHits::Combine() {
	hitList.insert(envelope.hitList.begin(), envelope.hitList.end()); // insert envelope hit list
	hitListByTimeDir.insert(envelope.hitListByTimeDir.begin(), envelope.hitListByTimeDir.end());
	envelope.hitList.clear(); //clear envelope hit list
	envelope.hitListByTimeDir.clear();
}

size_t EventHits::GetNhits(direction strip_dir) const {   // # of hits in a given direction
	return nhits.at(strip_dir);
}
