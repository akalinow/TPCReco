#include "SigClusterTPC.h"
#include "EventTPC.h"
/* ============= SPACE-TIME CLUSTER CLASS ===========*/

SigClusterTPC::SigClusterTPC(EventTPC& e)
  : evt_ref(e) {}

 decltype(SigClusterTPC::hitListByTimeDir) & SigClusterTPC::GetHitListByTimeDir() {
  return hitListByTimeDir;
}

 bool SigClusterTPC::AddByStrip(projection strip_dir, int strip_number, int time_cell) {  // valid range [0-2][1-1024][0-511]
	 if (time_cell < 0 || time_cell >= Geometry().GetAgetNtimecells() ||
		 strip_number < 1 || strip_number > Geometry().GetDirNstrips(strip_dir) ||
		 !IsDIR_UVW(strip_dir)) return false;

	 hitListByTimeDir[time_cell][strip_dir].insert(strip_number);
     hitList[strip_dir][strip_number].insert(time_cell);
	 return true;
 }

 void SigClusterTPC::UpdateStats() {
     // count hits
     for (auto& by_dir : hitList) {
         nhits[by_dir.first] = 0;
         for (auto& by_strip_num : by_dir.second) {
             nhits[by_dir.first] += by_strip_num.second.size();
         }
     }
 }

size_t SigClusterTPC::GetNhits(projection strip_dir) const {   // # of hits in a given direction
	return nhits.at(strip_dir);
}

std::shared_ptr<TH2D> SigClusterTPC::GetStripVsTimeInMM(projection strip_dir) {  // valid range [0-2]
	auto event_id = evt_ref.GetEventId();
	bool err_flag = false;
	double zmin = 0.0 - 0.5;  // time_cell_min;
	double zmax = 511. + 0.5; // time_cell_max;  
	double minTimeInMM = Geometry().Timecell2pos(zmin, err_flag);
	double maxTimeInMM = Geometry().Timecell2pos(zmax, err_flag);

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
	double x = 0.0, y = 0.0;

	for (auto& by_strip_num : hitList[strip_dir]) {
		for (auto icell : by_strip_num.second) {
			auto strip_num = by_strip_num.first;
			double val = evt_ref.GetValByStrip(strip_dir, strip_num, icell);
			x = Geometry().Timecell2pos(icell, err_flag);
			y = Geometry().Strip2posUVW(strip_dir, strip_num);
			result->Fill(x, y, val);
		}
	}
	return result;
}

// get three projections on: XY, XZ, YZ planes
Reconstr_hist SigClusterTPC::Get(double radius, int rebin_space, int rebin_time, int method) {

	//  const bool rebin_flag=false;
	Reconstr_hist h_all;
	bool err_flag = false;
	if (std::any_of(proj_vec_UVW.begin(), proj_vec_UVW.end(), [&](projection dir_) { return GetNhits(dir_) < 1; })) return h_all;

	//std::cout << Form(">>>> EventId = %lld", event_id) << std::endl;
	//std::cout << Form(">>>> Time cell range = [%d, %d]", time_cell_min, time_cell_max) << std::endl;

	for (auto& by_time_cell : hitListByTimeDir) {
		auto icell = by_time_cell.first;
		auto hits_by_time_cell = by_time_cell.second;
		// check if there is at least one hit in each direction
		if (std::any_of(proj_vec_UVW.begin(), proj_vec_UVW.end(), [&](projection dir_) { return by_time_cell.second[dir_].size() == 0; })) continue;

		//   std::cout << Form(">>>> Number of hits: time cell=%d: U=%d / V=%d / W=%d",
		//		      icell, (int)hits[int(projection::DIR_U)].size(), (int)hits[int(projection::DIR_V)].size(), (int)hits[int(projection::DIR_W)].size()) << std::endl;

		std::map<projection, std::map<int, int>> n_match; // map of number of matched points for each strip, key=STRIP_NUM [1-1024]
		std::map<MultiKey3, TVector2> hitPos; // key=(STRIP_NUM_U, STRIP_NUM_V, STRIP_NUM_W), value=(X [mm],Y [mm])

		// loop over hits and confirm matching in space
		for (auto& strip_num_U : hits_by_time_cell[projection::DIR_U]) {
			auto strip_U = Geometry().GetStripByDir(projection::DIR_U, strip_num_U);
			for (auto& strip_num_V : hits_by_time_cell[projection::DIR_V]) {
				auto strip_V = Geometry().GetStripByDir(projection::DIR_V, strip_num_V);
				for (auto& strip_num_W : hits_by_time_cell[projection::DIR_W]) {
					auto strip_W = Geometry().GetStripByDir(projection::DIR_W, strip_num_W);

					//	  std::cout << Form(">>>> Checking triplet: time_cell=%d: U=%d / V=%d / W=%d",
					//			    icell, hits[0].at(i0), hits[1].at(i1), hits[2].at(i2)) << std::endl;

					TVector2 pos;
					if (Geometry().MatchCrossPoint(strip_U, strip_V, strip_W, radius, pos)) {
						(n_match[projection::DIR_U])[strip_num_U]++;
						(n_match[projection::DIR_V])[strip_num_V]++;
						(n_match[projection::DIR_W])[strip_num_W]++;
						hitPos[MultiKey3(strip_num_U, strip_num_V, strip_num_W)] = pos;
						//	    std::cout << Form(">>>> Checking triplet: result = TRUE" ) << std::endl;
					}
					else {
						//	    std::cout << Form(">>>> Checking triplet: result = FALSE" ) << std::endl;
					}
				}
			}
		}
		//    std::cout << Form(">>>> Number of matches: time_cell=%d, triplets=%d", icell, (int)hitPos.size()) << std::endl;
		if (hitPos.size() == 0) continue;

		// book histograms/3D histogram before first fill

		double xmin, xmax, ymin, ymax;
		std::tie(xmin, xmax, ymin, ymax) = Geometry().rangeXY();

		double zmin = 0.0 - 0.5;  // time_cell_min;
		double zmax = 511. + 0.5; // time_cell_max;

		int nx = (int)((xmax - xmin) / Geometry().GetStripPitch() - 1);
		int ny = (int)((ymax - ymin) / Geometry().GetPadPitch() - 1);
		int nz = (int)(zmax - zmin);

		zmin = Geometry().Timecell2pos(zmin, err_flag);
		zmax = Geometry().Timecell2pos(zmax, err_flag);

		// rebin in space
		if (rebin_space > 1) {
			nx /= rebin_space;
			ny /= rebin_space;
		}

		// rebin in time
		if (rebin_time > 1) {
			nz /= rebin_time;
		}
		auto event_id = evt_ref.GetEventId();
		std::cout << Form(">>>> XYZ histogram: range=[%lf, %lf] x [%lf, %lf] x [%lf, %lf], nx=%d, ny=%d, nz=%d",
			xmin, xmax, ymin, ymax, zmin, zmax, nx, ny, nz) << std::endl;

		h_all.second = std::make_shared<TH3D>(Form("hreco3D_evt%lld", event_id),
			Form("Event-%lld: 3D reco in XYZ;X [mm];Y [mm];Z [mm]", event_id),
			nx, xmin, xmax, ny, ymin, ymax, nz, zmin, zmax);
		//      std::cout << Form(">>>> XY histogram: range=[%lf, %lf] x [%lf, %lf], nx=%d, ny=%d",
		//      			xmin, xmax, ymin, ymax, nx, ny) << std::endl;

		h_all.first[projection::DIR_XY] = std::make_shared<TH2D>(Form("hrecoXY_evt%lld", event_id),
			Form("Event-%lld: Projection in XY;X [mm];Y [mm];Charge/bin [arb.u.]", event_id),
			nx, xmin, xmax, ny, ymin, ymax);

		//      std::cout << Form(">>>> XZ histogram: range=[%lf, %lf] x [%lf, %lf], nx=%d, nz=%d",
		//      			xmin, xmax, zmin, zmax, nx, nz) << std::endl;

		h_all.first[projection::DIR_XZ] = std::make_shared<TH2D>(Form("hrecoXZ_evt%lld", event_id),
			Form("Event-%lld: Projection in XZ;X [mm];Z [mm];Charge/bin [arb.u.]", event_id),
			nx, xmin, xmax, nz, zmin, zmax);

		//      std::cout << Form(">>>> YZ histogram: range=[%lf, %lf] x [%lf, %lf], nx=%d, nz=%d",
		//      			ymin, ymax, zmin, zmax, ny, nz) << std::endl;

		h_all.first[projection::DIR_YZ] = std::make_shared<TH2D>(Form("hrecoYZ_evt%lld", event_id),
			Form("Event-%lld: Projection in YZ;Y [mm];Z [mm];Charge/bin [arb.u.]", event_id),
			ny, ymin, ymax, nz, zmin, zmax);

		// needed for method #2 only:
		// loop over matched hits and update fraction map
		std::map<projection, std::map<MultiKey3, double, multikey3_less>> fraction; // for U,V,W local charge projections

		for (auto& it1 : hitPos) {
			std::map<projection, int> uvw1;
			for (auto dir : proj_vec_UVW) {
				uvw1[dir] = it1.first.key[int(dir)];
			}

			std::map<projection, double> charge_along_direction;  // sum of charges along three directions (for a given time range)
			for (auto dir : proj_vec_UVW) {
				charge_along_direction[dir] = evt_ref.GetValByStrip(dir, uvw1[dir], icell);
			}

			// loop over directions
			for (auto& it2 : hitPos) {
				std::map<projection, int> uvw2;
				for (auto dir : proj_vec_UVW) {
					uvw2[dir] = it2.first.key[int(dir)];
				}

				for (auto dir1 : proj_vec_UVW) {
					if (uvw1[dir1] == uvw2[dir1]) {
						for (auto dir2 : proj_vec_UVW) {
							if (dir2 != dir1) {
								charge_along_direction[dir2] += evt_ref.GetValByStrip(dir2, uvw2[dir2], icell);
							}
						}
					}
				}
			}
			for (auto dir : proj_vec_UVW) {
				auto charge_in_strip = evt_ref.GetValByStrip(dir, uvw1[dir], icell); // charge in a given strip (for a given time range)
				fraction[dir].insert(std::pair<MultiKey3, double>(it1.first, charge_in_strip / charge_along_direction[dir]));
			}
		}

		// loop over matched hits and fill histograms
		if (h_all.second != nullptr && h_all.first[projection::DIR_XY] != nullptr && h_all.first[projection::DIR_XZ] != nullptr && h_all.first[projection::DIR_YZ] != nullptr) {

			for (auto&& it : hitPos) {

				double val = 0.0;

				switch (method) {

				case 0: // mehtod #1 - divide charge equally
					val = std::inner_product(proj_vec_UVW.begin(), proj_vec_UVW.end(), (it.first).key, 0.0, std::plus<>(), [&](projection dir, int key) {
						return evt_ref.GetValByStrip(dir, key, icell) / n_match[dir].at(key);
					});
					break;

				case 1: // method #2 - divide charge according to charge-fraction in two other directions
					val = 0.5 *
						std::inner_product(proj_vec_UVW.begin(), proj_vec_UVW.end(), (it.first).key, 0.0, std::plus<>(), [&](projection dir, int key) {
						return evt_ref.GetValByStrip(dir, key, icell) * std::inner_product(fraction.begin(), fraction.end(), proj_vec_UVW.begin(), 0.0, std::plus<>(), [&](auto& map_, projection dir2) {
							return (dir2 != dir ? map_.second.at(it.first) : 0.0);
						});
					});
					break;

				}; // end of switch (method)...
				double z = Geometry().Timecell2pos(icell, err_flag);
				h_all.second->Fill((it.second).X(), (it.second).Y(), z, val);
				h_all.first[projection::DIR_XY]->Fill((it.second).X(), (it.second).Y(), val);
				h_all.first[projection::DIR_XZ]->Fill((it.second).X(), z, val);
				h_all.first[projection::DIR_YZ]->Fill((it.second).Y(), z, val);

			}
		}
	}
	return h_all;
}
