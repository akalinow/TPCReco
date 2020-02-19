#include "EventCharges.h"

void EventCharges::Clear() {

	is_glb_max_charge_calculated = false;
	chargeMap.clear();
}

bool EventCharges::AddValByStrip(std::shared_ptr<Geometry_Strip> strip, int time_cell, double val) {  // valid range [0-2][1-1024][0-511]
	auto op = (*strip)();
	if (time_cell < 0 || time_cell >= Geometry().GetAgetNtimecells()) return false;
	chargeMap[{op.dir, op.num, time_cell}] += val; //update hit or add new one

	return true;
}

bool EventCharges::AddValByAgetChannel(int cobo_idx, int asad_idx, int aget_idx, int channel_idx, int time_cell, double val) {  // valid range [0-1][0-3][0-3][0-63][0-511]
	return AddValByStrip(Geometry().GetStripByAget(cobo_idx, asad_idx, aget_idx, channel_idx), time_cell, val);
}

double EventCharges::GetValByStrip(direction strip_dir, int strip_number, int time_cell) const {  // valid range [0-2][1-1024][0-511]
	// check if hit is unique
	auto it = chargeMap.find({ strip_dir,strip_number,time_cell });
	if (it != chargeMap.end()) {
		return it->second;
	}
	return 0.0;
}

double EventCharges::GetMaxCharge() {  // maximal charge from all strips
	if (!is_glb_max_charge_calculated) {
		glb_max_charge = std::max_element(chargeMap.begin(), chargeMap.end())->second;
		is_glb_max_charge_calculated = true;
	}
	return glb_max_charge;
}

std::shared_ptr<EventHits> EventCharges::GetHitsObject(double thr, int delta_strips, int delta_timecells) {  // applies clustering threshold to all space-time data points
	std::shared_ptr<EventHits> hits = std::make_shared<EventHits>(*this);

	// getting seed hits
	for (auto& charge : chargeMap) {
		if (charge.second > thr) hits->AddHit(charge.first);
	}

	// debug 
	//  std::cout << ">>>> GetSigCluster: nhits=" << hits->GetNhits() << ", chargeMap.size=" << chargeMap.size() << std::endl;
	std::cout << Form(">>>> GetSigCluster: BEFORE ENVELOPE: nhits(%d)/nhits(%d)/nhits(%d)=%ld/%ld/%ld",
		int(direction::U), int(direction::V), int(direction::W),
		hits->GetNhits(direction::U),
		hits->GetNhits(direction::V),
		hits->GetNhits(direction::W)) << std::endl;
	// debug

	// adding envelope to the seed hits
	// loop thru SEED-hits
	for (auto& hit : hits->GetHitList()) {
		const auto strip_dir = std::get<0>(hit);
		const auto strip_num = std::get<1>(hit);
		const auto time_cell = std::get<2>(hit);
		auto min_strip = chargeMap.lower_bound({ strip_dir, strip_num - delta_strips, time_cell - delta_timecells });
		auto max_strip = chargeMap.upper_bound({ strip_dir, strip_num + delta_strips, time_cell + delta_timecells });
		for (auto strip = min_strip; std::get<1>(strip->first) <= strip_num + delta_strips; strip = chargeMap.upper_bound({ strip_dir, std::get<1>(strip->first), std::numeric_limits<int>::max() })) {
			auto min_time_cell = chargeMap.lower_bound({ strip_dir, std::get<1>(strip->first), time_cell - delta_timecells });
			auto max_time_cell = chargeMap.upper_bound({ strip_dir, std::get<1>(strip->first), time_cell + delta_timecells });
			for (auto charge = min_time_cell; charge != max_time_cell; charge++) {
				if (charge->second < 0) continue; // exclude negative values (due to pedestal subtraction)
				hits->AddEnvelopeHit(charge->first); // add new space-time point
			}
		}
	}
	hits->Combine();
	hits->UpdateStats();
	// debug
	std::cout << Form(">>>> GetSigCluster: AFTER ENVELOPE:  nhits(%d)/nhits(%d)/nhits(%d)=%ld/%ld/%ld",
		int(direction::U), int(direction::V), int(direction::W),
		hits->GetNhits(direction::U),
		hits->GetNhits(direction::V),
		hits->GetNhits(direction::W)) << std::endl;
	// debug

	return hits;
}
