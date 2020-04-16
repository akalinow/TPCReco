#include "EventCharges.h"

void EventCharges::Clear() {

	is_glb_max_charge_calculated = false;
	chargeMap.clear();
}

bool EventCharges::AddValByStrip(std::shared_ptr<Geometry_Strip> strip, int time_cell, double val) {  // valid range [0-2][1-1024][0-511]
	auto op = (*strip)();
	if (time_cell < 0 || time_cell >= Geometry().GetAgetNtimecells()) return false;
	chargeMap[position{op.dir, op.section, op.num, time_cell}] += val; //update hit or add new one

	return true;
}

bool EventCharges::AddValByAgetChannel(int cobo_idx, int asad_idx, int aget_idx, int channel_idx, int time_cell, double val) {  // valid range [0-1][0-3][0-3][0-63][0-511]
	return AddValByStrip(Geometry().GetStripByAget(cobo_idx, asad_idx, aget_idx, channel_idx), time_cell, val);
}

double EventCharges::GetValByStrip(direction strip_dir, int strip_number, int time_cell) const {  // valid range [0-2][1-1024][0-511]
	return GetValByStrip(strip_dir, 0, strip_number, time_cell);
}

double EventCharges::GetValByStrip(direction strip_dir, int strip_section, int strip_number, int time_cell) const {  // valid range [0-2][1-1024][0-511]
	auto it = chargeMap.find(position{ strip_dir, strip_section,strip_number,time_cell });
	return (it != chargeMap.end() ? it->second: 0.0);
}

double EventCharges::GetValByStrip(std::shared_ptr<Geometry_Strip> strip, int time_cell) {  // valid range [0-511]
	auto& strip_data = (*strip)();
	return GetValByStrip(strip_data.dir, strip_data.section, strip_data.num, time_cell);
}

double EventCharges::GetValByGlobalChannel(int glb_channel_idx, int time_cell) {  // valid range [0-1023][0-511]
	return GetValByStrip(Geometry().GetStripByGlobal(glb_channel_idx), time_cell);
}

double EventCharges::GetMaxCharge() {  // maximal charge from all strips
	if (!is_glb_max_charge_calculated) {
		glb_max_charge = std::max_element(chargeMap.begin(), chargeMap.end())->second;
		is_glb_max_charge_calculated = true;
	}
	return glb_max_charge;
}

std::shared_ptr<EventHits> EventCharges::GetHits(double thr, int delta_strips, int delta_timecells) {  // applies clustering threshold to all space-time data points
	std::shared_ptr<EventHits> hitsObject = std::make_shared<EventHits>();

	// getting seed hits
	for (auto& charge : chargeMap) {
		if (charge.second > thr) hitsObject->AddHit(charge.first);
	}

	// debug 
	//  std::cout << ">>>> GetSigCluster: nhits=" << hits->GetNhits() << ", chargeMap.size=" << chargeMap.size() << std::endl;
	std::cout << Form(">>>> GetSigCluster: BEFORE ENVELOPE: nhits(%d)/nhits(%d)/nhits(%d)=%ld/%ld/%ld",
		int(direction::U), int(direction::V), int(direction::W),
		hitsObject->GetNhits(direction::U),
		hitsObject->GetNhits(direction::V),
		hitsObject->GetNhits(direction::W)) << std::endl;
	// debug

	// adding envelope to the seed hits
	// loop thru SEED-hits
	for (auto& hit : hitsObject->GetHitList()) {
		const auto strip_dir = std::get<0>(hit);
		const auto strip_num = std::get<1>(hit);
		const auto time_cell = std::get<2>(hit);
		auto min_strip = chargeMap.lower_bound(position{ strip_dir, section_min, strip_num - delta_strips, time_cell - delta_timecells });
		auto max_strip = chargeMap.upper_bound(position{ strip_dir, section_max, strip_num + delta_strips, time_cell + delta_timecells });
		for (auto section : section_indices) {
			for (auto strip_num_local = strip_num - delta_strips; strip_num_local <= strip_num + delta_strips; strip_num_local++) {
				auto min_time_cell = chargeMap.lower_bound(position{ strip_dir, section, strip_num_local, time_cell - delta_timecells });
				auto max_time_cell = chargeMap.upper_bound(position{ strip_dir, section, strip_num_local, time_cell + delta_timecells });
				for (auto charge = min_time_cell; charge != max_time_cell; charge++) {
					if (charge->second < 0) continue; // exclude negative values (due to pedestal subtraction)
					hitsObject->AddEnvelopeHit(charge->first); // add new space-time point
				}
			}
		}
	}
	hitsObject->Combine();
	// debug
	std::cout << Form(">>>> GetSigCluster: AFTER ENVELOPE:  nhits(%d)/nhits(%d)/nhits(%d)=%ld/%ld/%ld",
		int(direction::U), int(direction::V), int(direction::W),
		hitsObject->GetNhits(direction::U),
		hitsObject->GetNhits(direction::V),
		hitsObject->GetNhits(direction::W)) << std::endl;
	// debug

	return hitsObject;
}

void EventCharges::Read(std::string fname) {
	std::fstream file(fname, std::ios::in | std::ios::binary);
	file.seekg(0, std::ios::beg);
	file.read(reinterpret_cast<char*>(&event_info), sizeof(event_info));
	file.read(reinterpret_cast<char*>(&glb_max_charge), sizeof(glb_max_charge));
	file.read(reinterpret_cast<char*>(&is_glb_max_charge_calculated), sizeof(is_glb_max_charge_calculated));
	size_t map_size;
	std::pair<position, double> elem;
	file.read(reinterpret_cast<char*>(&map_size), sizeof(map_size));
	for (size_t index = 0; index < map_size; index++) {
		file.read(reinterpret_cast<char*>(&elem), sizeof(elem));
		chargeMap.insert(elem);
	}
	file.close();
};

void EventCharges::Write(std::string fname) {
	std::fstream file(fname, std::ios::out | std::ios::binary | std::ios::trunc);
	file.write(reinterpret_cast<const char*>(&event_info), sizeof(event_info));
	file.write(reinterpret_cast<const char*>(&glb_max_charge), sizeof(glb_max_charge));
	file.write(reinterpret_cast<const char*>(&is_glb_max_charge_calculated), sizeof(is_glb_max_charge_calculated));
	size_t map_size = chargeMap.size();
	file.write(reinterpret_cast<const char*>(&map_size), sizeof(map_size));
	for (auto& elem : chargeMap) {
		file.write(reinterpret_cast<const char*>(&elem), sizeof(elem));
	}
	file.close();
};
