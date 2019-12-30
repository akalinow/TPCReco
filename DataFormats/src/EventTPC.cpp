#include "EventTPC.h"

EventTPC::EventTPC() { Clear(); }


void EventTPC::Clear() {

	EvtGeometryPtr.reset();
	initOK = false;

	time_rebin = 1;
	glb_max_charge = 0.0;

	chargeMap.clear();
}


void EventTPC::SetGeoPtr(std::shared_ptr<GeometryTPC> aPtr) {
	EvtGeometryPtr = aPtr;
	if (EvtGeometryPtr && EvtGeometryPtr->IsOK()) initOK = true;
}

bool EventTPC::AddValByStrip(std::shared_ptr<StripTPC> strip, int time_cell, double val) {  // valid range [0-2][1-1024][0-511]
	auto op = (*strip)();
	if (!IsOK() || time_cell < 0 || time_cell >= EvtGeometryPtr->GetAgetNtimecells() || op.num<1 || op.num>EvtGeometryPtr->GetDirNstrips(op.dir)) return false;
	if (IsDIR_UVW(op.dir)) {
		{
		MultiKey3 mkey((int)op.dir, op.num, time_cell);
		MultiKey2 mkey_total((int)op.dir, op.num);
		MultiKey2 mkey_total2((int)op.dir, time_cell);
		MultiKey2 mkey_maxval((int)op.dir, op.num);

		// chargeMap - check if hit is unique
		auto it = chargeMap.find(mkey);
		double new_val = 0.0;
		if (it == chargeMap.end()) {

			// add new hit
			chargeMap[mkey] = val;
			new_val = val;

		}
		else {

			// update already existing hit
			it->second += val;
			new_val = it->second;

		}

		// upadate charge maxima
		if (new_val > glb_max_charge) {
			glb_max_charge = new_val;
		}

		return true;
	}
	};
	return false;
}

bool EventTPC::AddValByAgetChannel(int cobo_idx, int asad_idx, int aget_idx, int channel_idx, int time_cell, double val) {  // valid range [0-1][0-3][0-3][0-63][0-511]
	return AddValByStrip(EvtGeometryPtr->GetStripByAget(cobo_idx, asad_idx, aget_idx, channel_idx), time_cell, val);
}

double EventTPC::GetValByStrip(projection strip_dir, int strip_number, int time_cell/*, bool &result*/) {  // valid range [0-2][1-1024][0-511]
  //result=false;
	if (!IsOK() || time_cell < 0 || time_cell >= 512 || strip_number<1 || strip_number>EvtGeometryPtr->GetDirNstrips(strip_dir)) {
		return 0.0;
	}

	if (IsDIR_UVW(strip_dir)) {
		MultiKey3 mkey(int(strip_dir), strip_number, time_cell);

		// check if hit is unique
		auto it = chargeMap.find(mkey);
		if (it != chargeMap.end()) {
			//result=true;
			return it->second;
		}
	};
	return 0.0;
}

double EventTPC::GetMaxCharge() {  // maximal charge from all strips
	if (!IsOK()) return 0.0;
	return glb_max_charge;
}

std::shared_ptr<SigClusterTPC> EventTPC::GetOneCluster(double thr, int delta_strips, int delta_timecells) {  // applies clustering threshold to all space-time data points
	std::shared_ptr<SigClusterTPC> cluster = std::make_shared<SigClusterTPC>(*this);

	// getting cluster seed hits
	for (auto&& it: chargeMap) {
		if (it.second > thr) cluster->AddByStrip(static_cast<projection>(it.first.key[0]), it.first.key[1], it.first.key[2]);
		// debug - dump the whole event as a single cluster
		//    cluster->AddByStrip( (it->first).key[0], (it->first).key[1], (it->first).key[2] );
		// debug - dump the whole event as a single cluster
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
	for (auto&& it2 : oldList) {

		// unpack coordinates
		const projection strip_dir = static_cast<projection>(it2.key[0]);
		const int strip_num = it2.key[1];
		const int time_cell = it2.key[2];
		const int strip_range[2] = { std::max(1, strip_num - delta_strips),
					 std::min(EvtGeometryPtr->GetDirNstrips(strip_dir), strip_num + delta_strips) };
		const int timecell_range[2] = { std::max(0, time_cell - delta_timecells),
						std::min(EvtGeometryPtr->GetAgetNtimecells() - 1, time_cell + delta_timecells) };
		for (int icell = timecell_range[0]; icell <= timecell_range[1]; icell++) {
			for (int istrip = strip_range[0]; istrip <= strip_range[1]; istrip++) {
				if (icell == time_cell && istrip == strip_num) continue; // exclude existing seed hits
				MultiKey3 mkey3(int(strip_dir), istrip, icell);
				if (chargeMap.find(mkey3) == chargeMap.end()) continue; // exclude non-existing space-time coordinates
				if (chargeMap.find(mkey3)->second < 0) continue; // exclude negative values (due to pedestal subtraction)
				// add new space-time point
				if (oldList.find(mkey3) == oldList.end()) {
					cluster->AddByStrip(strip_dir, istrip, icell);
				}
			}
		}
	}

	// debug
	std::cout << Form(">>>> GetSigCluster: AFTER ENVELOPE:  nhits(%d)/nhits(%d)/nhits(%d)=%ld/%ld/%ld",
		int(projection::DIR_U), int(projection::DIR_V), int(projection::DIR_W),
		cluster->GetNhits(projection::DIR_U),
		cluster->GetNhits(projection::DIR_V),
		cluster->GetNhits(projection::DIR_W)) << std::endl;
	// debug

	return cluster;
}

std::shared_ptr<TH2D> EventTPC::GetStripVsTime(std::shared_ptr<SigClusterTPC> cluster, projection strip_dir) {

	if (!IsOK() || !cluster->IsOK()) return std::shared_ptr<TH2D>();

	std::shared_ptr<TH2D> result = std::make_shared<TH2D>(Form("hclust_%s_vs_time_evt%lld", EvtGeometryPtr->GetDirName(strip_dir).c_str(), event_id),
		Form("Event-%lld: Clustered hits from %s strips;Time bin [arb.u.];%s strip no.;Charge/bin [arb.u.]",
			event_id, EvtGeometryPtr->GetDirName(strip_dir).c_str(), EvtGeometryPtr->GetDirName(strip_dir).c_str()),
		EvtGeometryPtr->GetAgetNtimecells(),
		0.0 - 0.5,
		1. * EvtGeometryPtr->GetAgetNtimecells() - 0.5, // ends at 511.5 (cells numbered from 0 to 511)
		EvtGeometryPtr->GetDirNstrips(strip_dir),
		1.0 - 0.5,
		1. * EvtGeometryPtr->GetDirNstrips(strip_dir) + 0.5);

	for (int strip_num = cluster->GetMinStrip(strip_dir); strip_num <= cluster->GetMaxStrip(strip_dir); strip_num++) {
		for (int icell = cluster->GetMinTime(strip_dir); icell <= cluster->GetMaxTime(strip_dir); icell++) {
			if (cluster->CheckByStrip(strip_dir, strip_num, icell)) {
				result->Fill(1. * icell, 1. * strip_num, GetValByStrip(strip_dir, strip_num, icell));
			}
		}
	}

	return result;
}

std::shared_ptr<TH2D> EventTPC::GetStripVsTime(projection strip_dir) {  // valid range [0-2]

	if (!IsOK()) return std::shared_ptr<TH2D>();

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
	for (int strip_num = 1; strip_num <= EvtGeometryPtr->GetDirNstrips(strip_dir); strip_num++) {
		for (int icell = 0; icell < EvtGeometryPtr->GetAgetNtimecells(); icell++) {
			double val = GetValByStrip(strip_dir, strip_num, icell);
			result->Fill(1. * icell, 1. * strip_num, val);
		}
	}
	return result;
}

std::shared_ptr<TH2D> EventTPC::GetStripVsTimeInMM(std::shared_ptr<SigClusterTPC> cluster, projection strip_dir) {  // valid range [0-2]

	if (!IsOK()) return std::shared_ptr<TH2D>();

	bool err_flag;
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

	for (int strip_num = cluster->GetMinStrip(strip_dir); strip_num <= cluster->GetMaxStrip(strip_dir); strip_num++) {
		for (int icell = cluster->GetMinTime(strip_dir); icell <= cluster->GetMaxTime(strip_dir); icell++) {
			if (cluster->CheckByStrip(strip_dir, strip_num, icell)) {
				double val = GetValByStrip(strip_dir, strip_num, icell);
				x = EvtGeometryPtr->Timecell2pos(icell, err_flag);
				y = EvtGeometryPtr->Strip2posUVW(strip_dir, strip_num, err_flag);
				result->Fill(x, y, val);
			}
		}
	}
	return result;
}

// get three projections on: XY, XZ, YZ planes
template <Dims dimensions>
std::pair<std::vector<std::shared_ptr<TH2D>>, std::shared_ptr<TH3D>> EventTPC::Get(std::shared_ptr<SigClusterTPC> cluster, double radius, int rebin_space, int rebin_time, int method) {

	//  const bool rebin_flag=false;
	std::shared_ptr<TH3D> h;
	std::shared_ptr<TH2D> h1, h2, h3;
	std::vector<std::shared_ptr<TH2D>> hvec;
	bool err_flag = false;

	if (!IsOK() || !cluster->IsOK() ||
		cluster->GetNhits(projection::DIR_U) < 1 || cluster->GetNhits(projection::DIR_V) < 1 || cluster->GetNhits(projection::DIR_W) < 1) return std::pair<std::vector<std::shared_ptr<TH2D>>, std::shared_ptr<TH3D>>();

	// loop over time slices and match hits in space
	const int time_cell_min = *std::max_element(&cluster->min_time[0], &cluster->min_time[3]);
	const int time_cell_max = *std::min_element(&cluster->max_time[0], &cluster->max_time[3]);

	//std::cout << Form(">>>> EventId = %lld", event_id) << std::endl;
	//std::cout << Form(">>>> Time cell range = [%d, %d]", time_cell_min, time_cell_max) << std::endl;

	const auto& hitListByTimeDir = cluster->GetHitListByTimeDir();

	for (int icell = time_cell_min; icell <= time_cell_max; icell++) {

		if (std::any_of(proj_vec_UVW.begin(), proj_vec_UVW.end(), [&](auto dir_) { return hitListByTimeDir.find(MultiKey2(icell, int(dir_))) == hitListByTimeDir.end(); })) continue;

		std::vector<int> hits[3];
		std::transform(proj_vec_UVW.begin(), proj_vec_UVW.end(), &hits[0], [&](auto proj) { return hitListByTimeDir.find(MultiKey2(icell, int(proj)))->second; });

		//   std::cout << Form(">>>> Number of hits: time cell=%d: U=%d / V=%d / W=%d",
		//		      icell, (int)hits[int(projection::DIR_U)].size(), (int)hits[int(projection::DIR_V)].size(), (int)hits[int(projection::DIR_W)].size()) << std::endl;

		// check if there is at least one hit in each direction
		if (std::any_of(&hits[0], &hits[3], [&](auto x) {return x.size() == 0; })) continue;

		std::array<std::map<int, int>, 3> n_match; // map of number of matched points for each strip, key=STRIP_NUM [1-1024]
		std::map<MultiKey3, TVector2, multikey3_less> hitPos; // key=(STRIP_NUM_U, STRIP_NUM_V, STRIP_NUM_W), value=(X [mm],Y [mm])

		// loop over hits and confirm matching in space
		for (auto& it0 : hits[0]) {
			auto strip0 = EvtGeometryPtr->GetStripByDir(projection::DIR_U, it0);
			for (auto& it1 : hits[1]) {
				auto strip1 = EvtGeometryPtr->GetStripByDir(projection::DIR_V, it1);
				for (auto& it2 : hits[2]) {
					auto strip2 = EvtGeometryPtr->GetStripByDir(projection::DIR_W, it2);

					//	  std::cout << Form(">>>> Checking triplet: time_cell=%d: U=%d / V=%d / W=%d",
					//			    icell, hits[0].at(i0), hits[1].at(i1), hits[2].at(i2)) << std::endl;

					TVector2 pos;
					if (EvtGeometryPtr->MatchCrossPoint(strip0, strip1, strip2, radius, pos)) {
						(n_match[int(projection::DIR_U)])[it0]++;
						(n_match[int(projection::DIR_V)])[it1]++;
						(n_match[int(projection::DIR_W)])[it2]++;
						hitPos[MultiKey3(it0, it1, it2)] = pos;
						//	    std::cout << Form(">>>> Checking triplet: result = TRUE" ) << std::endl;
					}
					else {
						//	    std::cout << Form(">>>> Checking triplet: result = FALSE" ) << std::endl;
					}
				}
			}
		}
		//    std::cout << Form(">>>> Number of matches: time_cell=%d, triplets=%d", icell, (int)hitPos.size()) << std::endl;
		if (hitPos.size() < 1) continue;

		// book histograms/3D histogram before first fill
		if ((dimensions == D3 ? (h == nullptr) : (h1 == nullptr && h2 == nullptr && h3 == nullptr))) {

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

			if (dimensions == D3) {
				std::cout << Form(">>>> XYZ histogram: range=[%lf, %lf] x [%lf, %lf] x [%lf, %lf], nx=%d, ny=%d, nz=%d",
					xmin, xmax, ymin, ymax, zmin, zmax, nx, ny, nz) << std::endl;

				h = std::make_shared<TH3D>(Form("hreco3D_evt%lld", event_id),
					Form("Event-%lld: 3D reco in XYZ;X [mm];Y [mm];Z [mm]", event_id),
					nx, xmin, xmax, ny, ymin, ymax, nz, zmin, zmax);
			}
			else {
				//      std::cout << Form(">>>> XY histogram: range=[%lf, %lf] x [%lf, %lf], nx=%d, ny=%d",
				//      			xmin, xmax, ymin, ymax, nx, ny) << std::endl;

				h1 = std::make_shared<TH2D>(Form("hrecoXY_evt%lld", event_id),
					Form("Event-%lld: Projection in XY;X [mm];Y [mm];Charge/bin [arb.u.]", event_id),
					nx, xmin, xmax, ny, ymin, ymax);

				//      std::cout << Form(">>>> XZ histogram: range=[%lf, %lf] x [%lf, %lf], nx=%d, nz=%d",
				//      			xmin, xmax, zmin, zmax, nx, nz) << std::endl;

				h2 = std::make_shared<TH2D>(Form("hrecoXZ_evt%lld", event_id),
					Form("Event-%lld: Projection in XZ;X [mm];Z [mm];Charge/bin [arb.u.]", event_id),
					nx, xmin, xmax, nz, zmin, zmax);

				//      std::cout << Form(">>>> YZ histogram: range=[%lf, %lf] x [%lf, %lf], nx=%d, nz=%d",
				//      			ymin, ymax, zmin, zmax, ny, nz) << std::endl;

				h3 = std::make_shared<TH2D>(Form("hrecoYZ_evt%lld", event_id),
					Form("Event-%lld: Projection in YZ;Y [mm];Z [mm];Charge/bin [arb.u.]", event_id),
					ny, ymin, ymax, nz, zmin, zmax);
			}
		}

		// needed for method #2 only:
		// loop over matched hits and update fraction map
		std::array<std::map<MultiKey3, double, multikey3_less>, 3> fraction; // for U,V,W local charge projections

		for (auto&& it1 : hitPos) {

			int u1 = (it1.first).key[0];
			int v1 = (it1.first).key[1];
			int w1 = (it1.first).key[2];
			double qtot[3] = { 0., 0., 0. };  // sum of charges along three directions (for a given time range)
			double    q[3] = { 0., 0., 0. };  // charge in a given strip (for a given time range)
			for (auto& dir : proj_vec_UVW) {
				q[int(dir)] = GetValByStrip(dir, (it1.first).key[int(dir)], icell);
			}

			// loop over directions
			for (auto&& it2 : hitPos) {
				int u2 = (it2.first).key[0];
				int v2 = (it2.first).key[1];
				int w2 = (it2.first).key[2];

				if (u1 == u2) {
					qtot[int(projection::DIR_V)] += GetValByStrip(projection::DIR_V, v2, icell);
					qtot[int(projection::DIR_W)] += GetValByStrip(projection::DIR_W, w2, icell);
				}
				if (v1 == v2) {
					qtot[int(projection::DIR_W)] += GetValByStrip(projection::DIR_W, w2, icell);
					qtot[int(projection::DIR_U)] += GetValByStrip(projection::DIR_U, u2, icell);
				}
				if (w1 == w2) {
					qtot[int(projection::DIR_U)] += GetValByStrip(projection::DIR_U, u2, icell);
					qtot[int(projection::DIR_V)] += GetValByStrip(projection::DIR_V, v2, icell);
				}
			}
			for (auto&& strip_dir : proj_vec_UVW) {
				fraction[int(strip_dir)].insert(std::pair<MultiKey3, double>(it1.first, q[int(strip_dir)] / qtot[int(strip_dir)]));
			}
		}

		// loop over matched hits and fill histograms
		if ((dimensions == D3 ? (h != nullptr) : (h1 != nullptr && h2 != nullptr && h3 != nullptr))) {

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
				if (dimensions == D3) {
					h->Fill((it.second).X(), (it.second).Y(), z, val);
				}
				else {
					h1->Fill((it.second).X(), (it.second).Y(), val);
					h2->Fill((it.second).X(), z, val);
					h3->Fill((it.second).Y(), z, val);
				}

			}
		}
	}
	if (dimensions == D2) {
		if (h1 != nullptr && h2 != nullptr && h3 != nullptr) {
			hvec.push_back(h1);
			hvec.push_back(h2);
			hvec.push_back(h3);
		}
	}
	return std::make_pair(hvec, h);
}
