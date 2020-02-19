#ifndef __EVENTTPC_H__
#define __EVENTTPC_H__

/// TPC event class.
///
/// VERSION: 05 May 2018

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
#include <numeric>

#include "TH1D.h"
#include "TH2D.h"
#include "TH3F.h"
#include "CommonDefinitions.h"
#include "EventInformation.h"
#include "GeometryTPC.h"
#include "EventHits.h"

class EventCharges {
	friend class HistoManager;
private:

	std::map< std::tuple<direction, int, int>, double> chargeMap; // key=(STRIP_DIR [0-2], STRIP_NUM [1-1024], TIME_CELL [0-511])

	double glb_max_charge = 0.0;

	bool is_glb_max_charge_calculated = false;

	EventInformation event_info;

public:

	decltype(event_info)& operator()() { return event_info; };
	decltype(event_info)& Info() { return event_info; };

	EventCharges() = default;

	~EventCharges() = default;

	void Clear();

	// helper methods for inserting data points
	// they return TRUE on success and FALSE on error
	bool AddValByStrip(std::shared_ptr<Geometry_Strip> strip, int time_cell, double val);                      // valid range [0-511]
	bool AddValByAgetChannel(int cobo_idx, int asad_idx, int aget_idx, int channel_idx, int time_cell, double val); // valid range [0-1][0-3][0-3][0-63][0-511]

	// helper methods for extracting data points
	// they return 0.0 for non-existing data points
	double GetValByStrip(direction strip_dir, int strip_number, int time_cell) const;  // valid range [0-2][1-1024][0-511]

	double GetMaxCharge();                   // maximal charge from all strips

	std::shared_ptr<EventHits> GetHitsObject(double thr, int delta_strips, int delta_timecells); // applies clustering threshold to all space-time data points 
};

#endif