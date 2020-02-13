#include "SigClusterTPC.h"
#include "EventTPC.h"

class { //object keeping temporary envelope containers outside of SigClusterTPC
	friend class SigClusterTPC;
private:
	std::set<std::tuple<direction, int, int>> hitList; // temporary list of selected envelope space-time cells for further analysis where return
								 // value=key(STRIP_DIR [0-2], STRIP_NUM [1-1024], REBIN_TIME_CELL [0-511])
	std::set< std::tuple<int, direction, int>> hitListByTimeDir; // same temporary list of selected envelope space-time cells as a map
														  // with key=(REBIN_TIME_CELL [0-511], STRIP_DIR [0-2]) 
														  // that returns vector of STRIP_NUMBERS 
} envelope;

/* ============= SPACE-TIME CLUSTER CLASS ===========*/

SigClusterTPC::SigClusterTPC(EventTPC& e)
	: evt_ref(e) {}

bool SigClusterTPC::AddHit(std::tuple<direction, int, int> hit) {  // valid range [0-2][1-1024][0-511]
	auto strip_dir = std::get<0>(hit);
	auto strip_number = std::get<1>(hit);
	auto time_cell = std::get<2>(hit);
	if (time_cell < 0 || time_cell >= Geometry().GetAgetNtimecells() ||
		strip_number < 1 || strip_number > Geometry().GetDirNstrips(strip_dir)) return false;

	hitListByTimeDir.insert({ time_cell,strip_dir,strip_number });
	hitList.insert(hit);
	return true;
}

bool SigClusterTPC::AddEnvelopeHit(std::tuple<direction, int, int> hit) {  // valid range [0-2][1-1024][0-511]
	auto strip_dir = std::get<0>(hit);
	auto strip_number = std::get<1>(hit);
	auto time_cell = std::get<2>(hit);
	if (time_cell < 0 || time_cell >= Geometry().GetAgetNtimecells() ||
		strip_number < 1 || strip_number > Geometry().GetDirNstrips(strip_dir)) return false;

	envelope.hitList.insert(hit);
	envelope.hitListByTimeDir.insert({ time_cell,strip_dir,strip_number });
	return true;
}

void SigClusterTPC::UpdateStats() {
	// count hits
	for (auto dir : dir_vec_UVW) {
		nhits[dir] = std::distance(hitList.lower_bound({ dir, std::numeric_limits<int>::min(),std::numeric_limits<int>::min() }), hitList.upper_bound({ dir, std::numeric_limits<int>::max(),std::numeric_limits<int>::max() }));
	}
}

void SigClusterTPC::Combine() {
	hitList.insert(envelope.hitList.begin(), envelope.hitList.end()); // insert envelope hit list
	hitListByTimeDir.insert(envelope.hitListByTimeDir.begin(), envelope.hitListByTimeDir.end());
	envelope.hitList.clear(); //clear envelope hit list
	envelope.hitListByTimeDir.clear();
}

size_t SigClusterTPC::GetNhits(direction strip_dir) const {   // # of hits in a given direction
	return nhits.at(strip_dir);
}

std::shared_ptr<TH2D> SigClusterTPC::GetStripVsTimeInMM(direction strip_dir) {  // valid range [0-2]
	auto event_id = evt_ref().EventId();
	bool err_flag = false;
	double zmin = 0.0 - 0.5;  // time_cell_min;
	double zmax = 511. + 0.5; // time_cell_max;  
	double minTimeInMM = Geometry().Timecell2pos(zmin);
	double maxTimeInMM = Geometry().Timecell2pos(zmax);

	auto firstStrip_offset_vec = (*Geometry().GetStripByDir(strip_dir, 1))().offset_vec;
	auto lastStrip_offset_vec = (*Geometry().GetStripByDir(strip_dir, Geometry().GetDirNstrips(strip_dir)))().offset_vec;

	double minStripInMM = (firstStrip_offset_vec + Geometry().GetReferencePoint()) * Geometry().GetStripPitchVector(strip_dir);
	double maxStripInMM = (lastStrip_offset_vec + Geometry().GetReferencePoint()) * Geometry().GetStripPitchVector(strip_dir);

	if (minStripInMM > maxStripInMM) {
		std::swap(minStripInMM, maxStripInMM);
	}

	std::shared_ptr<TH2D> result = std::make_shared<TH2D>(Form("hraw_%s_vs_time_evt%lld", Geometry().GetDirName(strip_dir).c_str(), event_id),
		Form("Event-%lld: Raw signals from %s strips;Time direction [mm];%s strip direction [mm];Charge/bin [arb.u.]",
			event_id, Geometry().GetDirName(strip_dir).c_str(), Geometry().GetDirName(strip_dir).c_str()),
		Geometry().GetAgetNtimecells(), minTimeInMM, maxTimeInMM,
		Geometry().GetDirNstrips(strip_dir), minStripInMM, maxStripInMM);

	// fill new histogram
	auto min_hit = hitList.lower_bound({ strip_dir, std::numeric_limits<int>::min(), std::numeric_limits<int>::min() });
	auto max_hit = hitList.upper_bound({ strip_dir, std::numeric_limits<int>::max(), std::numeric_limits<int>::max() });
	for (auto hit = min_hit; hit != max_hit; hit++) {
		auto strip_num = std::get<1>(*hit);
		auto icell = std::get<2>(*hit);
		double val = evt_ref.GetValByStrip(strip_dir, strip_num, icell);
		auto x = Geometry().Timecell2pos(icell);
		auto y = Geometry().Strip2posUVW(strip_dir, strip_num);
		result->Fill(x, y, val);
	}
	return result;
}

// get three directions on: XY, XZ, YZ planes
Reconstr_hist&& SigClusterTPC::Get(double radius, int rebin_space, int rebin_time, int method) {

	static std::function<std::pair<bool, TVector2>(std::array<int, 3>, double)> fn = [](std::array<int, 3> nums, double rad)->std::pair<bool, TVector2> {
		TVector2 pos;
		return { Geometry().MatchCrossPoint(nums, rad, pos), pos };
	};
	static RV_Storage<std::pair<bool, TVector2>, std::array<int, 3>, double> MatchCrossPoint_functor(fn); //return value manager, saves already found crossing strips

	Reconstr_hist h_all;
	bool err_flag = false;
	if (std::any_of(dir_vec_UVW.begin(), dir_vec_UVW.end(), [&](direction dir_) { return GetNhits(dir_) < 1; })) return std::move(h_all);

	//std::cout << Form(">>>> EventId = %lld", event_id) << std::endl;

	for (auto hits_in_time_cell_begin = hitListByTimeDir.begin(); hits_in_time_cell_begin != hitListByTimeDir.end(); hits_in_time_cell_begin = hitListByTimeDir.upper_bound({ std::get<0>(*hits_in_time_cell_begin), std::numeric_limits<direction>::max(), std::numeric_limits<int>::max() })) { //iterate over time cells
		auto icell = std::get<0>(*hits_in_time_cell_begin);
		std::map<direction, std::vector<int>> hits_strip_nums_in_dir;
		for (auto dir : dir_vec_UVW) {
			auto min_hit = hitListByTimeDir.lower_bound({ icell, dir, std::numeric_limits<int>::min() });
			auto max_hit = hitListByTimeDir.upper_bound({ icell, dir, std::numeric_limits<int>::max() });
			for (auto hit = min_hit; hit != max_hit; hit++) {
				hits_strip_nums_in_dir[dir].push_back(std::get<2>(*hit));
			}
		}
		// check if there is at least one hit in each direction
		if (std::any_of(dir_vec_UVW.begin(), dir_vec_UVW.end(), [&](direction dir) { return hits_strip_nums_in_dir[dir].size() == 0; })) continue;

		//   std::cout << Form(">>>> Number of hits: time cell=%d: U=%d / V=%d / W=%d",
		//		      icell, (int)hits[int(direction::U)].size(), (int)hits[int(direction::V)].size(), (int)hits[int(direction::W)].size()) << std::endl;

		std::map<std::array<int, 3>, TVector2> hitPos; // key=(STRIP_NUM_U, STRIP_NUM_V, STRIP_NUM_W), value=(X [mm],Y [mm])
		std::array<int, 3> pos;
		// loop over hits and confirm matching in space
		for (auto& hit_strip_num_U : hits_strip_nums_in_dir[direction::U]) {
			for (auto& hit_strip_num_V : hits_strip_nums_in_dir[direction::V]) {
				for (auto& hit_strip_num_W : hits_strip_nums_in_dir[direction::W]) {
					pos = { hit_strip_num_U, hit_strip_num_V, hit_strip_num_W };
					auto result = MatchCrossPoint_functor(pos, radius);
					if (std::get<0>(result)) {
						hitPos[pos] = std::get<1>(result);
					}
				}
			}
		}
		//    std::cout << Form(">>>> Number of matches: time_cell=%d, triplets=%d", icell, (int)hitPos.size()) << std::endl;
		if (hitPos.size() == 0) continue;

		// book 2D and 3D histogram before first fill

		double xmin, xmax, ymin, ymax;
		std::tie(xmin, xmax, ymin, ymax) = Geometry().rangeXY();

		double time_cell_min = 0.0 - 0.5;
		double time_cell_max = 511. + 0.5;

		int nx = (int)((xmax - xmin) / Geometry().GetStripPitch() - 1);
		int ny = (int)((ymax - ymin) / Geometry().GetPadPitch() - 1);
		int nz = (int)(time_cell_max - time_cell_min);

		double zmin = Geometry().Timecell2pos(time_cell_min);
		double zmax = Geometry().Timecell2pos(time_cell_max);

		// rebin in space
		if (rebin_space > 1) {
			nx /= rebin_space;
			ny /= rebin_space;
		}

		// rebin in time
		if (rebin_time > 1) {
			nz /= rebin_time;
		}
		auto event_id = evt_ref().EventId();
		std::cout << Form(">>>> XYZ histogram: range=[%lf, %lf] x [%lf, %lf] x [%lf, %lf], nx=%d, ny=%d, nz=%d",
			xmin, xmax, ymin, ymax, zmin, zmax, nx, ny, nz) << std::endl;

		h_all.second = std::make_shared<TH3D>(Form("hreco3D_evt%lld", event_id),
			Form("Event-%lld: 3D reco in XYZ;X [mm];Y [mm];Z [mm]", event_id),
			nx, xmin, xmax, ny, ymin, ymax, nz, zmin, zmax);
		//      std::cout << Form(">>>> XY histogram: range=[%lf, %lf] x [%lf, %lf], nx=%d, ny=%d",
		//      			xmin, xmax, ymin, ymax, nx, ny) << std::endl;

		h_all.first = std::make_shared<TH2D>(Form("hrecoXY_evt%lld", event_id),
			Form("Event-%lld: Projection in XY;X [mm];Y [mm];Charge/bin [arb.u.]", event_id),
			nx, xmin, xmax, ny, ymin, ymax);

		// needed for method #2 only:
		// loop over matched hits and update fraction map
		std::map<direction, std::map<std::array<int, 3>, double>> fraction; // for U,V,W local charge directions

		for (auto& it1 : hitPos) {
			std::map<direction, int> uvw1;
			std::map<direction, double> charge_along_direction;  // sum of charges along three directions (for a given time range)
			for (auto dir : dir_vec_UVW) {
				uvw1[dir] = it1.first[int(dir)];
				charge_along_direction[dir] = evt_ref.GetValByStrip(dir, uvw1[dir], icell);
			}

			// loop over directions
			for (auto& it2 : hitPos) {
				std::map<direction, int> uvw2;
				for (auto dir : dir_vec_UVW) {
					uvw2[dir] = it2.first[int(dir)];
				}

				for (auto dir1 : dir_vec_UVW) {
					if (uvw1[dir1] == uvw2[dir1]) {
						for (auto dir2 : dir_vec_UVW) {
							if (dir2 != dir1) {
								charge_along_direction[dir2] += evt_ref.GetValByStrip(dir2, uvw2[dir2], icell);
							}
						}
					}
				}
			}
			for (auto dir : dir_vec_UVW) {
				auto charge_in_strip = evt_ref.GetValByStrip(dir, uvw1[dir], icell); // charge in a given strip (for a given time range)
				fraction[dir].insert({ it1.first, charge_in_strip / charge_along_direction[dir] });
			}
		}

		// loop over matched hits and fill histograms
		for (auto&& it : hitPos) {

			double val = 0.5 * // divide charge according to charge-fraction in two other directions
				std::inner_product(dir_vec_UVW.begin(), dir_vec_UVW.end(), it.first.begin(), 0.0, std::plus<>(), [&](direction dir, int key) {
				return evt_ref.GetValByStrip(dir, key, icell) * std::inner_product(fraction.begin(), fraction.end(), dir_vec_UVW.begin(), 0.0, std::plus<>(), [&](auto& map_, direction dir2) {
					return (dir2 != dir ? map_.second.at(it.first) : 0.0);
				});
			});
			double z = Geometry().Timecell2pos(icell);
			h_all.second->Fill((it.second).X(), (it.second).Y(), z, val);
			h_all.first->Fill((it.second).X(), (it.second).Y(), val);
		}
	}
	return std::move(h_all);
}
