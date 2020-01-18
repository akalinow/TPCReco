#include "EventTPC.h"

void EventTPC::Clear() {

	glb_max_charge = 0.0;

	chargeMap.clear();
}

bool EventTPC::AddValByStrip(std::shared_ptr<StripTPC> strip, int time_cell, double val) {  // valid range [0-2][1-1024][0-511]
	auto op = (*strip)();
	if (time_cell < 0 || time_cell >= Geometry().GetAgetNtimecells() || op.num<1 || op.num>Geometry().GetDirNstrips(op.dir)) return false;

	chargeMap[op.dir][op.num][time_cell] += val; //update hit or add new one

	// upadate charge maxima
	glb_max_charge = std::max(glb_max_charge, chargeMap[op.dir][op.num][time_cell]);

	return true;
}

bool EventTPC::AddValByAgetChannel(int cobo_idx, int asad_idx, int aget_idx, int channel_idx, int time_cell, double val) {  // valid range [0-1][0-3][0-3][0-63][0-511]
	return AddValByStrip(Geometry().GetStripByAget(cobo_idx, asad_idx, aget_idx, channel_idx), time_cell, val);
}

double EventTPC::GetValByStrip(projection strip_dir, int strip_number, int time_cell/*, bool &result*/) {  // valid range [0-2][1-1024][0-511]
  //result=false;
	if (!IsDIR_UVW(strip_dir) || time_cell < 0 || time_cell >= 512 || strip_number<1 || strip_number>Geometry().GetDirNstrips(strip_dir)) {
		return 0.0;
	}

	// check if hit is unique
	auto it = chargeMap[strip_dir][strip_number].find(time_cell);
	if (it != chargeMap[strip_dir][strip_number].end()) {
		//result=true;
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
	for (auto& strip_dir: chargeMap) {
		for (auto& strip_num : strip_dir.second) {
			for (auto& time_cell : strip_num.second) {
				if (time_cell.second > thr) cluster->AddByStrip(strip_dir.first, strip_num.first, time_cell.first);
				// debug - dump the whole event as a single cluster
				//    cluster->AddByStrip( (it->first).key[0], (it->first).key[1], (it->first).key[2] );
				// debug - dump the whole event as a single cluster
			}
		}
	}

	// debug 
	//  std::cout << ">>>> GetSigCluster: nhits=" << cluster->GetNhits() << ", chargeMap.size=" << chargeMap.size() << std::endl;
	std::cout << Form(">>>> GetSigCluster: BEFORE ENVELOPE: nhits(%d)/nhits(%d)/nhits(%d)=%ld/%ld/%ld",
		int(projection::DIR_U), int(projection::DIR_V), int(projection::DIR_W),
		cluster->GetNhits(projection::DIR_U),
		cluster->GetNhits(projection::DIR_V),
		cluster->GetNhits(projection::DIR_W)) << std::endl;
	// debug

	// adding envelope to the seed hits
	auto oldList = cluster->GetHitList(); // make a copy of list of SEED-hits

	// loop thru SEED-hits
	for (auto& by_strip_dir : oldList) {
		for (auto& by_strip_num : by_strip_dir.second) {
			for (auto& time_cell : by_strip_num.second) {
				// unpack coordinates
				const projection strip_dir = by_strip_dir.first;
				const int strip_num = by_strip_num.first;
				auto min_strip_num = std::max(1, strip_num - delta_strips);
				auto max_strip_num = std::min(Geometry().GetDirNstrips(strip_dir), strip_num + delta_strips);
				auto min_time_cell = std::max(0, time_cell - delta_timecells);
				auto max_time_cell = std::min(Geometry().GetAgetNtimecells() - 1, time_cell + delta_timecells);
				for (int icell = min_time_cell; icell <= max_time_cell; icell++) {
					for (int istrip = min_strip_num; istrip <= max_strip_num; istrip++) {
						if ((icell == time_cell && istrip == strip_num) || // exclude existing seed hits
							(chargeMap[strip_dir][istrip].find(icell) == chargeMap[strip_dir][istrip].end()) || // exclude non-existing space-time coordinates
							(chargeMap[strip_dir][istrip][icell] < 0)) continue; // exclude negative values (due to pedestal subtraction)
						// add new space-time point
						if (oldList[strip_dir][strip_num].find(time_cell) == oldList[strip_dir][strip_num].end()) {
							cluster->AddByStrip(strip_dir, istrip, icell);
						}
					}
				}
			}
		}
	}
	cluster->UpdateStats();
	// debug
	std::cout << Form(">>>> GetSigCluster: AFTER ENVELOPE:  nhits(%d)/nhits(%d)/nhits(%d)=%ld/%ld/%ld",
		int(projection::DIR_U), int(projection::DIR_V), int(projection::DIR_W),
		cluster->GetNhits(projection::DIR_U),
		cluster->GetNhits(projection::DIR_V),
		cluster->GetNhits(projection::DIR_W)) << std::endl;
	// debug

	return cluster;
}

std::shared_ptr<TH2D> EventTPC::GetStripVsTime(projection strip_dir) {  // valid range [0-2]

	std::shared_ptr<TH2D> result = std::make_shared<TH2D>(Form("hraw_%s_vs_time_evt%lld", Geometry().GetDirName(strip_dir).c_str(), event_id),
		Form("Event-%lld: Raw signals from %s strips;Time bin [arb.u.];%s strip no.;Charge/bin [arb.u.]",
			event_id, Geometry().GetDirName(strip_dir).c_str(), Geometry().GetDirName(strip_dir).c_str()),
		Geometry().GetAgetNtimecells(),
		0.0 - 0.5,
		1. * Geometry().GetAgetNtimecells() - 0.5, // ends at 511.5 (cells numbered from 0 to 511)
		Geometry().GetDirNstrips(strip_dir),
		1.0 - 0.5,
		1. * Geometry().GetDirNstrips(strip_dir) + 0.5);
	// fill new histogram
	for (auto& by_strip_num : chargeMap[strip_dir]) {
		for (auto& by_time_cell : by_strip_num.second) {
			auto strip_num = by_strip_num.first;
			auto icell = by_time_cell.first;
			double val = GetValByStrip(strip_dir, strip_num, icell);
			result->Fill(1. * icell, 1. * strip_num, val);
		}
	}
	return result;
}
