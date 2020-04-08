#ifndef __EventHits_H__
#define __EventHits_H__

// TPC event class.
// VERSION: pon, 24 cze 2019, 14:14:30 CEST

#include <cstdlib>
#include <vector>
#include <map>
#include <string>
#include <memory>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <map>
#include <iterator>
#include <fstream>
#include <utility>
#include <algorithm> // for find_if
#include <set>
#include <cstdint>

#include "CommonDefinitions.h"

using hitList_type = std::set<position_reduced>;
using hitListByTimeDir_type = std::set< position_by_time_reduced>;

// Space-time mask for signal clusters defined as a class
class EventHits {
	friend class HistoManager;
private:
	hitList_type hitList; // list of selected space-time cells for further analysis where return
								 // value=key(STRIP_DIR [0-2], STRIP_NUM [1-1024], REBIN_TIME_CELL [0-511])
	hitListByTimeDir_type hitListByTimeDir; // same list of selected space-time cells as a map
														  // with key=(REBIN_TIME_CELL [0-511], STRIP_DIR [0-2]) 
	std::map<direction, size_t> nhits;   // number of space-time cells in a given U,V,W direction

public:

	EventHits() = default;
	inline const decltype(hitList)& GetHitList() const { return hitList; } // list of ALL hits, value=key(STRIP_DIR [0-2], STRIP_NUM [1-1024], REBIN_TIME_CELL [0-511])

	// helper methods for inserting data points
	// they return TRUE on success and FALSE on error
	void AddHit(position hit);     // valid range [0-2][1-1024][0-511]
	void AddEnvelopeHit(position hit);     // valid range [0-2][0-2][1-1024][0-511]

	// statistics
	size_t GetNhits(direction strip_dir) const;                        // # of hits in a given direction
	size_t GetNhits() const;                        // # of all hits

	void Combine(); //combine hit list with envelope hit list
};

#endif
