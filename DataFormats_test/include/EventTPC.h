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
#include <algorithm> 
#include <numeric>

#include "TH1D.h"
#include "TH2D.h"
#include "TH3F.h"
#include "CommonDefinitions.h"
#include "Event_Information.h"
#include "Geometry_Strip.h"


namespace test{

class EventTPC {
private:

	std::map< std::tuple<projection, int, int>, double> chargeMap; 

	double glb_max_charge = 0.0;

	bool is_glb_max_charge_calculated = false;

	Event_Information event_info;

public:

	decltype(event_info)& operator()() { return event_info; };
	decltype(event_info)& Info() { return event_info; };

	EventTPC() = default;

	~EventTPC() = default;

	void Clear();

	bool AddValByStrip(std::shared_ptr<Geometry_Strip> strip, int time_cell, double val);                     

	double GetValByStrip(projection strip_dir, int strip_number, int time_cell) const;

	double GetMaxCharge(); 

};
}
  
#endif
