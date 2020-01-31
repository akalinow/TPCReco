#include "EventTPC.h"

void EventTPC::Clear() {

	glb_max_charge = 0.0;

	chargeMap.clear();
}

bool EventTPC::AddValByStrip(std::shared_ptr<Geometry_Strip> strip, int time_cell, double val) {  // valid range [0-2][1-1024][0-511]
	auto op = (*strip)();
	if (time_cell < 0 || time_cell >= Geometry().GetAgetNtimecells()) return false;
	auto& ref = chargeMap[{op.dir, op.num, time_cell}];
	ref += val; //update hit or add new one

	// upadate charge maxima
	glb_max_charge = std::max(glb_max_charge, ref);

	return true;
}

bool EventTPC::AddValByAgetChannel(int cobo_idx, int asad_idx, int aget_idx, int channel_idx, int time_cell, double val) {  // valid range [0-1][0-3][0-3][0-63][0-511]
	return AddValByStrip(Geometry().GetStripByAget(cobo_idx, asad_idx, aget_idx, channel_idx), time_cell, val);
}

double EventTPC::GetValByStrip(direction strip_dir, int strip_number, int time_cell) {  // valid range [0-2][1-1024][0-511]
	// check if hit is unique
	auto it = chargeMap.find({ strip_dir,strip_number,time_cell });
	if (it != chargeMap.end()) {
		return it->second;
	}
	return 0.0;
}

double EventTPC::GetMaxCharge() {  // maximal charge from all strips
	return glb_max_charge;
}

std::shared_ptr<SigClusterTPC> EventTPC::GetOneCluster(double thr, int delta_strips, int delta_timecells) {  // applies clustering threshold to all space-time data points
	std::shared_ptr<SigClusterTPC> cluster = std::make_shared<SigClusterTPC>(*this);

	// getting cluster seed hits
	for (auto& charge : chargeMap) {
		if (charge.second > thr) cluster->AddHit(charge.first);
	}

	// debug 
	//  std::cout << ">>>> GetSigCluster: nhits=" << cluster->GetNhits() << ", chargeMap.size=" << chargeMap.size() << std::endl;
	std::cout << Form(">>>> GetSigCluster: BEFORE ENVELOPE: nhits(%d)/nhits(%d)/nhits(%d)=%ld/%ld/%ld",
		int(direction::U), int(direction::V), int(direction::W),
		cluster->GetNhits(direction::U),
		cluster->GetNhits(direction::V),
		cluster->GetNhits(direction::W)) << std::endl;
	// debug

	// adding envelope to the seed hits
	auto oldList = cluster->GetHitList(); // make a copy of list of SEED-hits

	// loop thru SEED-hits
	for (auto& hit : oldList) {
		const auto strip_dir = std::get<0>(hit);
		const auto strip_num = std::get<1>(hit);
		const auto time_cell = std::get<2>(hit);
		auto min_strip = chargeMap.lower_bound({ strip_dir, strip_num - delta_strips, time_cell - delta_timecells });
		auto max_strip = chargeMap.upper_bound({ strip_dir, strip_num + delta_strips, time_cell + delta_timecells });
		for (auto strip = min_strip; std::get<1>(strip->first) <= strip_num + delta_strips; strip = chargeMap.upper_bound({ strip_dir, std::get<1>(strip->first), std::numeric_limits<int>::max() })) {
			auto min_time_cell = chargeMap.lower_bound({ strip_dir, std::get<1>(strip->first), time_cell - delta_timecells });
			auto max_time_cell = chargeMap.upper_bound({ strip_dir, std::get<1>(strip->first), time_cell + delta_timecells });
			for (auto& charge : decltype(chargeMap){ min_time_cell, max_time_cell }) {
				if (charge.second < 0) continue; // exclude negative values (due to pedestal subtraction)
				cluster->AddHit(charge.first); // add new space-time point
			}
		}
	}
	cluster->UpdateStats();
	// debug
	std::cout << Form(">>>> GetSigCluster: AFTER ENVELOPE:  nhits(%d)/nhits(%d)/nhits(%d)=%ld/%ld/%ld",
		int(direction::U), int(direction::V), int(direction::W),
		cluster->GetNhits(direction::U),
		cluster->GetNhits(direction::V),
		cluster->GetNhits(direction::W)) << std::endl;
	// debug

	return cluster;
}

std::shared_ptr<TH2D> EventTPC::GetStripVsTime(direction strip_dir) {  // valid range [0-2]

	std::shared_ptr<TH2D> result = std::make_shared<TH2D>(Form("hraw_%s_vs_time_evt%lld", Geometry().GetDirName(strip_dir).c_str(), event_info.EventId()),
		Form("Event-%lld: Raw signals from %s strips;Time bin [arb.u.];%s strip no.;Charge/bin [arb.u.]",
			event_info.EventId(), Geometry().GetDirName(strip_dir).c_str(), Geometry().GetDirName(strip_dir).c_str()),
		Geometry().GetAgetNtimecells(),
		0.0 - 0.5,
		1. * Geometry().GetAgetNtimecells() - 0.5, // ends at 511.5 (cells numbered from 0 to 511)
		Geometry().GetDirNstrips(strip_dir),
		1.0 - 0.5,
		1. * Geometry().GetDirNstrips(strip_dir) + 0.5);
	// fill new histogram
	for (auto& charge : decltype(chargeMap){
		chargeMap.lower_bound({strip_dir, std::numeric_limits<int>::min(), std::numeric_limits<int>::min()}),
		chargeMap.lower_bound({strip_dir, std::numeric_limits<int>::max(), std::numeric_limits<int>::max()}) }) {
		auto strip_num = std::get<1>(charge.first);
		auto time_cell = std::get<1>(charge.first);
		auto val = charge.second;
		result->Fill(time_cell, strip_num, val);
	}
	return result;
}
