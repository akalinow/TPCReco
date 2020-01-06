#include "EventTPC.h"

EventTPC::EventTPC(std::shared_ptr<GeometryTPC> geo_ptr) {
	SetGeoPtr(geo_ptr);
	Clear(); 
}


void EventTPC::Clear() {

	glb_max_charge = 0.0;

	chargeMap.clear();
}


void EventTPC::SetGeoPtr(std::shared_ptr<GeometryTPC> aPtr) {
	assert(aPtr != nullptr);
	EvtGeometryPtr = aPtr;
}

bool EventTPC::AddValByStrip(std::shared_ptr<StripTPC> strip, int time_cell, double val) {  // valid range [0-2][1-1024][0-511]
	auto op = (*strip)();
	if (time_cell < 0 || time_cell >= EvtGeometryPtr->GetAgetNtimecells() || op.num<1 || op.num>EvtGeometryPtr->GetDirNstrips(op.dir)) return false;

	chargeMap[op.dir][op.num][time_cell] += val; //update hit or add new one

	// upadate charge maxima
	glb_max_charge = std::max(glb_max_charge, chargeMap[op.dir][op.num][time_cell]);

	return true;
}

bool EventTPC::AddValByAgetChannel(int cobo_idx, int asad_idx, int aget_idx, int channel_idx, int time_cell, double val) {  // valid range [0-1][0-3][0-3][0-63][0-511]
	return AddValByStrip(EvtGeometryPtr->GetStripByAget(cobo_idx, asad_idx, aget_idx, channel_idx), time_cell, val);
}

double EventTPC::GetValByStrip(projection strip_dir, int strip_number, int time_cell/*, bool &result*/) {  // valid range [0-2][1-1024][0-511]
  //result=false;
	if (!IsDIR_UVW(strip_dir) || time_cell < 0 || time_cell >= 512 || strip_number<1 || strip_number>EvtGeometryPtr->GetDirNstrips(strip_dir)) {
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
				auto max_strip_num = std::min(EvtGeometryPtr->GetDirNstrips(strip_dir), strip_num + delta_strips);
				auto min_time_cell = std::max(0, time_cell - delta_timecells);
				auto max_time_cell = std::min(EvtGeometryPtr->GetAgetNtimecells() - 1, time_cell + delta_timecells);
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

	std::shared_ptr<TH2D> result = std::make_shared<TH2D>(Form("hraw_%s_vs_time_evt%lld", EvtGeometryPtr->GetDirName(strip_dir).c_str(), event_id),
		Form("Event-%lld: Raw signals from %s strips;Time bin [arb.u.];%s strip no.;Charge/bin [arb.u.]",
			event_id, EvtGeometryPtr->GetDirName(strip_dir).c_str(), EvtGeometryPtr->GetDirName(strip_dir).c_str()),
		EvtGeometryPtr->GetAgetNtimecells(),
		0.0 - 0.5,
		1. * EvtGeometryPtr->GetAgetNtimecells() - 0.5, // ends at 511.5 (cells numbered from 0 to 511)
		EvtGeometryPtr->GetDirNstrips(strip_dir),
		1.0 - 0.5,
		1. * EvtGeometryPtr->GetDirNstrips(strip_dir) + 0.5);
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

std::shared_ptr<TH2D> EventTPC::GetStripVsTimeInMM(std::shared_ptr<SigClusterTPC> cluster, projection strip_dir) {  // valid range [0-2]

	bool err_flag = false;
	double zmin = 0.0 - 0.5;  // time_cell_min;
	double zmax = 511. + 0.5; // time_cell_max;  
	double minTimeInMM = EvtGeometryPtr->Timecell2pos(zmin, err_flag);
	double maxTimeInMM = EvtGeometryPtr->Timecell2pos(zmax, err_flag);
	
	auto firstStrip_offset_vec = (*EvtGeometryPtr->GetStripByDir(strip_dir, 1))().offset_vec;
	auto lastStrip_offset_vec = (*EvtGeometryPtr->GetStripByDir(strip_dir, EvtGeometryPtr->GetDirNstrips(strip_dir)))().offset_vec;

	double minStripInMM = (firstStrip_offset_vec + EvtGeometryPtr->GetReferencePoint()) * EvtGeometryPtr->GetStripPitchVector(strip_dir);
	double maxStripInMM = (lastStrip_offset_vec + EvtGeometryPtr->GetReferencePoint()) * EvtGeometryPtr->GetStripPitchVector(strip_dir);

	if (minStripInMM > maxStripInMM) {
		std::swap(minStripInMM, maxStripInMM);
	}

	std::shared_ptr<TH2D> result = std::make_shared<TH2D>(Form("hraw_%s_vs_time_evt%lld", EvtGeometryPtr->GetDirName(strip_dir).c_str(), event_id),
		Form("Event-%lld: Raw signals from %s strips;Time direction [mm];%s strip direction [mm];Charge/bin [arb.u.]",
			event_id, EvtGeometryPtr->GetDirName(strip_dir).c_str(), EvtGeometryPtr->GetDirName(strip_dir).c_str()),
		EvtGeometryPtr->GetAgetNtimecells(), minTimeInMM, maxTimeInMM,
		EvtGeometryPtr->GetDirNstrips(strip_dir), minStripInMM, maxStripInMM);

	// fill new histogram
	double x = 0.0, y = 0.0;

	auto HitListOneDir = cluster->GetHitList()[strip_dir];
	for (auto& by_strip_num : HitListOneDir) {
		for (auto icell : by_strip_num.second) {
			auto strip_num = by_strip_num.first;
			double val = GetValByStrip(strip_dir, strip_num, icell);
			x = EvtGeometryPtr->Timecell2pos(icell, err_flag);
			y = EvtGeometryPtr->Strip2posUVW(strip_dir, strip_num);
			result->Fill(x, y, val);
		}
	}
	return result;
}

// get three projections on: XY, XZ, YZ planes
Reconstr_hist EventTPC::Get(std::shared_ptr<SigClusterTPC> cluster, double radius, int rebin_space, int rebin_time, int method) {

	//  const bool rebin_flag=false;
	Reconstr_hist h_all;
	bool err_flag = false;

	if (cluster->GetNhits(projection::DIR_U) < 1 || cluster->GetNhits(projection::DIR_V) < 1 || cluster->GetNhits(projection::DIR_W) < 1) return h_all;

	//std::cout << Form(">>>> EventId = %lld", event_id) << std::endl;
	//std::cout << Form(">>>> Time cell range = [%d, %d]", time_cell_min, time_cell_max) << std::endl;

	auto& hitListByTimeDir = cluster->GetHitListByTimeDir();

	for (auto& by_time_cell : hitListByTimeDir) {
		auto icell = by_time_cell.first;
		// check if there is at least one hit in each direction
		if (std::any_of(proj_vec_UVW.begin(), proj_vec_UVW.end(), [&](projection dir_) { return by_time_cell.second[dir_].size() == 0; })) continue;

		std::set<int> hits[3];
		std::transform(proj_vec_UVW.begin(), proj_vec_UVW.end(), &hits[0], [&](projection dir_) { return by_time_cell.second[dir_]; });

		//   std::cout << Form(">>>> Number of hits: time cell=%d: U=%d / V=%d / W=%d",
		//		      icell, (int)hits[int(projection::DIR_U)].size(), (int)hits[int(projection::DIR_V)].size(), (int)hits[int(projection::DIR_W)].size()) << std::endl;

		std::array<std::map<int, int>, 3> n_match; // map of number of matched points for each strip, key=STRIP_NUM [1-1024]
		std::map<MultiKey3, TVector2> hitPos; // key=(STRIP_NUM_U, STRIP_NUM_V, STRIP_NUM_W), value=(X [mm],Y [mm])

		// loop over hits and confirm matching in space
		for (auto& strip_num_U : hits[0]) {
			auto strip_U = EvtGeometryPtr->GetStripByDir(projection::DIR_U, strip_num_U);
			for (auto& strip_num_V : hits[1]) {
				auto strip_V = EvtGeometryPtr->GetStripByDir(projection::DIR_V, strip_num_V);
				for (auto& strip_num_W : hits[2]) {
					auto strip_W = EvtGeometryPtr->GetStripByDir(projection::DIR_W, strip_num_W);

					//	  std::cout << Form(">>>> Checking triplet: time_cell=%d: U=%d / V=%d / W=%d",
					//			    icell, hits[0].at(i0), hits[1].at(i1), hits[2].at(i2)) << std::endl;

					TVector2 pos;
					if (EvtGeometryPtr->MatchCrossPoint(strip_U, strip_V, strip_W, radius, pos)) {
						(n_match[int(projection::DIR_U)])[strip_num_U]++;
						(n_match[int(projection::DIR_V)])[strip_num_V]++;
						(n_match[int(projection::DIR_W)])[strip_num_W]++;
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
		std::tie(xmin, xmax, ymin, ymax) = EvtGeometryPtr->rangeXY();

		double zmin = 0.0 - 0.5;  // time_cell_min;
		double zmax = 511. + 0.5; // time_cell_max;

		int nx = (int)((xmax - xmin) / EvtGeometryPtr->GetStripPitch() - 1);
		int ny = (int)((ymax - ymin) / EvtGeometryPtr->GetPadPitch() - 1);
		int nz = (int)(zmax - zmin);

		zmin = EvtGeometryPtr->Timecell2pos(zmin, err_flag);
		zmax = EvtGeometryPtr->Timecell2pos(zmax, err_flag);

		// rebin in space
		if (rebin_space > 1) {
			nx /= rebin_space;
			ny /= rebin_space;
		}

		// rebin in time
		if (rebin_time > 1) {
			nz /= rebin_time;
		}

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
		std::array<std::map<MultiKey3, double, multikey3_less>, 3> fraction; // for U,V,W local charge projections

		for (auto& it1 : hitPos) {
			std::map<projection, int> uvw1;
			for (auto dir : proj_vec_UVW) {
				uvw1[dir] = it1.first.key[int(dir)];
			}

			double qtot[3] = { 0., 0., 0. };  // sum of charges along three directions (for a given time range)
			double    q[3] = { 0., 0., 0. };  // charge in a given strip (for a given time range)
			for (auto dir : proj_vec_UVW) {
				q[int(dir)] = GetValByStrip(dir, uvw1[dir], icell);
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
								qtot[int(dir2)] += GetValByStrip(dir2, uvw2[dir2], icell);
							}
						}
					}
				}
			}
			for (auto dir : proj_vec_UVW) {
				fraction[int(dir)].insert(std::pair<MultiKey3, double>(it1.first, q[int(dir)] / qtot[int(dir)]));
			}
		}

		// loop over matched hits and fill histograms
		if (h_all.second != nullptr && h_all.first[projection::DIR_XY] != nullptr && h_all.first[projection::DIR_XZ] != nullptr && h_all.first[projection::DIR_YZ] != nullptr) {

			for (auto&& it : hitPos) {

				double val = 0.0;

				switch (method) {

				case 0: // mehtod #1 - divide charge equally
					val = std::inner_product(proj_vec_UVW.begin(), proj_vec_UVW.end(), (it.first).key, 0.0, std::plus<>(), [&](projection dir, int key) {
						return GetValByStrip(dir, key, icell) / n_match[0].at(key);
					});
					break;

				case 1: // method #2 - divide charge according to charge-fraction in two other directions
					val = 0.5 *
						std::inner_product(proj_vec_UVW.begin(), proj_vec_UVW.end(), (it.first).key, 0.0, std::plus<>(), [&](projection dir, int key) {
						return GetValByStrip(dir, key, icell) * std::inner_product(fraction.begin(), fraction.end(), proj_vec_UVW.begin(), 0.0, std::plus<>(), [&](auto& map_, projection dir2) {
							return (dir2 != dir ? map_.at(it.first) : 0.0);
						});
					});
					break;

				default:
					val = 0.0;
				}; // end of switch (method)...
				double z = EvtGeometryPtr->Timecell2pos(icell, err_flag);
				h_all.second->Fill((it.second).X(), (it.second).Y(), z, val);
				h_all.first[projection::DIR_XY]->Fill((it.second).X(), (it.second).Y(), val);
				h_all.first[projection::DIR_XZ]->Fill((it.second).X(), z, val);
				h_all.first[projection::DIR_YZ]->Fill((it.second).Y(), z, val);

			}
		}
	}
	return h_all;
}
