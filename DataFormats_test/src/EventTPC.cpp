#include "EventTPC.h"

namespace test{

void EventTPC::Clear() {

	glb_max_charge = 0.0;
	is_glb_max_charge_calculated = false;
	chargeMap.clear();
}

bool EventTPC::AddValByStrip(std::shared_ptr<Geometry_Strip> strip, int time_cell, double val) {  // valid range [0-2][1-1024][0-511]
	auto op = (*strip)();
	//if (time_cell < 0 || time_cell >= Geometry().GetAgetNtimecells()) return false;
	chargeMap[{op.dir, op.num, time_cell}] += val; //update hit or add new one

	return true;
}

double EventTPC::GetValByStrip(projection strip_dir, int strip_number, int time_cell) const {  // valid range [0-2][1-1024][0-511]
	// check if hit is unique
	auto it = chargeMap.find({ strip_dir,strip_number,time_cell });
	if (it != chargeMap.end()) {
		return it->second;
	}
	return 0.0;
}

double EventTPC::GetMaxCharge() {  // maximal charge from all strips
	if (!is_glb_max_charge_calculated) {
		glb_max_charge = std::max_element(chargeMap.begin(), chargeMap.end())->second;
		is_glb_max_charge_calculated = true;
	}
	return glb_max_charge;
}

}
