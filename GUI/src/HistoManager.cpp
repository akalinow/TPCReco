#include <cstdlib>
#include <iostream>
#include <tuple>

#include "TCanvas.h"
#include "TH2D.h"
#include "TH3D.h"
#include "TSpectrum2.h"
#include "TVector3.h"
#include "TPolyLine3D.h"
#include "TView.h"

#include "GeometryTPC.h"
#include "EventCharges.h"

#include "HistoManager.h"

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
HistoManager& HistogramManager() {
	static HistoManager hist_manager;
	return hist_manager;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::setEvent(std::shared_ptr<EventCharges> aEvent) {

	chargesObject = aEvent;

	hitsObject = myTkBuilder.setEvent(aEvent);
	myTkBuilder.initialize();
	myTkBuilder.reconstruct();

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::getCartesianProjection(direction strip_dir) {

	return GetStripVsTimeInMM(strip_dir);

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TH2D&& HistoManager::getRawStripVsTime(direction strip_dir) {

	auto&& hProjection = GetStripVsTime(strip_dir);
	double varianceX = hProjection.GetCovariance(1, 1);
	double varianceY = hProjection.GetCovariance(2, 2);
	double varianceXY = hProjection.GetCovariance(1, 2);

	std::vector<int> nStrips = { 72, 92, 92 };

	std::cout << " varianceX*12: " << varianceX * 12 / 450 / 450
		<< " varianceY*12: " << varianceY * 12 / nStrips[int(strip_dir)] / nStrips[int(strip_dir)]
		<< " varianceXY: " << varianceXY
		<< std::endl;

	return std::move(hProjection);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::getRecHitStripVsTime(direction strip_dir) {

	return myTkBuilder.getRecHits2D(strip_dir);

}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
Reconstr_hist HistoManager::getReconstruction(bool force) {
	if (!reconstruction_done || force) {
		double radius = 2.0;
		hitsObject = myTkBuilder.GetHits();
		reconstruction = Get(radius);
		reconstruction_done = true;
	}
	return reconstruction;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH3D> HistoManager::get3DReconstruction(bool force) {
	return getReconstruction(force).second;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::get2DReconstruction(bool force) {
	return getReconstruction(force).first;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawTrack3D(TVirtualPad* aPad) {

	aPad->cd();
	const Track3D& aTrack3D = myTkBuilder.getTrack3D(0);
	const TrackSegment3DCollection& trackSegments = aTrack3D.getSegments();
	if (trackSegments.size() == 0) return;

	TPolyLine3D aPolyLine;
	aPolyLine.SetLineWidth(2);
	aPolyLine.SetLineColor(2);

	aPolyLine.SetPoint(0,
		trackSegments.front().getStart().X(),
		trackSegments.front().getStart().Y(),
		trackSegments.front().getStart().Z());

	for (auto& aSegment : trackSegments) {
		aPolyLine.SetPoint(aPolyLine.GetLastPoint() + 1,
			aSegment.getEnd().X(),
			aSegment.getEnd().Y(),
			aSegment.getEnd().Z());
	}
	aPolyLine.DrawClone();
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawTrack3DProjectionXY(TVirtualPad* aPad) {

	aPad->cd();
	const Track3D& aTrack3D = myTkBuilder.getTrack3D(0);

	int lineColor = 2;
	TLine aSegment2DLine;
	aSegment2DLine.SetLineWidth(2);
	for (const auto& aItem : aTrack3D.getSegments()) {
		auto& start = aItem.getStart();
		auto& end = aItem.getEnd();
		aSegment2DLine.SetLineColor(lineColor++);
		aSegment2DLine.DrawLine(start.X(), start.Y(), end.X(), end.Y());
	}
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawTrack2DSeed(direction strip_dir, TVirtualPad* aPad) {

	const TrackSegment2D& aSegment2D = myTkBuilder.getSegment2D(strip_dir);
	const TVector3& start = aSegment2D.getStart();
	const TVector3& end = aSegment2D.getEnd();

	TLine aSegment2DLine;
	aSegment2DLine.SetLineWidth(2);
	aSegment2DLine.SetLineColor(2);
	aPad->cd();
	aSegment2DLine.DrawLine(start.X(), start.Y(), end.X(), end.Y());
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawTrack3DProjectionTimeStrip(direction strip_dir, TVirtualPad* aPad) {

	aPad->cd();
	const Track3D& aTrack3D = myTkBuilder.getTrack3D(0);

	int lineColor = 2;
	TLine aSegment2DLine;
	aSegment2DLine.SetLineWidth(2);
	double minX = 999.0, minY = 999.0;
	double maxX = -999.0, maxY = -999.0;

	for (const auto& aItem : aTrack3D.getSegments()) {
		const TrackSegment2D& aSegment2DProjection = aItem.get2DProjection(strip_dir, 0, aItem.getLength());
		const TVector3& start = aSegment2DProjection.getStart();
		const TVector3& end = aSegment2DProjection.getEnd();
		aSegment2DLine.SetLineColor(lineColor++);
		aSegment2DLine.DrawLine(start.X(), start.Y(), end.X(), end.Y());

		minY = std::min({ minY, start.Y(), end.Y() });
		maxY = std::max({ maxY, start.Y(), end.Y() });
		minX = std::min({ minX, start.X(), end.X() });
		maxX = std::max({ maxX, start.X(), end.X() });
	}
	minX -= 5;
	minY -= 5;

	double delta = std::max(std::abs(maxX - minX),
		std::abs(maxY - minY));
	maxX = minX + delta;
	maxY = minY + delta;

	TH2D* hFrame = (TH2D*)aPad->GetListOfPrimitives()->At(0);
	if (hFrame != nullptr) {
		hFrame->GetXaxis()->SetRangeUser(minX, maxX);
		hFrame->GetYaxis()->SetRangeUser(minY, maxY);
	}
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
void HistoManager::drawChargeAlongTrack3D(TVirtualPad* aPad) const {

	const Track3D& aTrack3D = myTkBuilder.getTrack3D(0);

	aPad->cd();
	TGraph aGr = aTrack3D.getChargeProfile();
	//TGraph aGr = aTrack3D.getHitDistanceProfile();
	aGr.SetTitle("Charge distribution along track.;d[track length];charge[arbitrary units]");
	aGr.SetLineWidth(2);
	aGr.SetLineColor(2);
	aGr.DrawClone("AL");
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
TH2D&& HistoManager::GetStripVsTime(direction strip_dir) {  // valid range [0-2]

	TH2D result{
		Form(
			"hraw_%s_vs_time_evt%lld",
			Geometry().GetDirName(strip_dir).c_str(),
			chargesObject->event_info.EventId()),
		Form(
			"Event-%lld: Raw signals from %s strips;Time bin [arb.u.];%s strip no.;Charge/bin [arb.u.]",
			chargesObject->event_info.EventId(),
			Geometry().GetDirName(strip_dir).c_str(),
			Geometry().GetDirName(strip_dir).c_str()),
		Geometry().GetAgetNtimecells(),
		0.0 - 0.5,
		1. * Geometry().GetAgetNtimecells() - 0.5, // ends at 511.5 (cells numbered from 0 to 511)
		Geometry().GetDirNstrips(strip_dir),
		1.0 - 0.5,
		1. * Geometry().GetDirNstrips(strip_dir) + 0.5 };
	// fill new histogram
	auto min_strip = chargesObject->chargeMap.lower_bound(position{ strip_dir, 0, std::numeric_limits<int>::min(), std::numeric_limits<int>::min() });
	auto max_strip = chargesObject->chargeMap.upper_bound(position{ strip_dir, section_max - 1, std::numeric_limits<int>::max(), std::numeric_limits<int>::max() });
	for (auto charge = min_strip; charge != max_strip; charge++) {
		auto strip_num = std::get<1>(charge->first);
		auto time_cell = std::get<2>(charge->first);
		auto val = charge->second;
		result.Fill(time_cell, strip_num, val);
	}
	return std::move(result);
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH2D> HistoManager::GetStripVsTimeInMM(direction strip_dir) {  // valid range [0-2]
	auto event_id = chargesObject->Info().EventId();
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

	std::shared_ptr<TH2D> result = std::make_shared<TH2D>(
		Form(
			"hraw_%s_vs_time_evt%lld",
			Geometry().GetDirName(strip_dir).c_str(),
			event_id),
		Form(
			"Event-%lld: Raw signals from %s strips;Time direction [mm];%s strip direction [mm];Charge/bin [arb.u.]",
			event_id,
			Geometry().GetDirName(strip_dir).c_str(),
			Geometry().GetDirName(strip_dir).c_str()),
		Geometry().GetAgetNtimecells(),
		minTimeInMM,
		maxTimeInMM,
		Geometry().GetDirNstrips(strip_dir),
		minStripInMM,
		maxStripInMM);

	// fill new histogram
	auto min_hit = hitsObject->hitList.lower_bound(position_reduced{ strip_dir, std::numeric_limits<int>::min(), std::numeric_limits<int>::min() });
	auto max_hit = hitsObject->hitList.upper_bound(position_reduced{ strip_dir, std::numeric_limits<int>::max(), std::numeric_limits<int>::max() });
	for (auto hit = min_hit; hit != max_hit; hit++) {
		auto strip_num = std::get<1>(*hit);
		auto icell = std::get<2>(*hit);
		double val = chargesObject->GetValByStrip(strip_dir, strip_num, icell);
		auto x = Geometry().Timecell2pos(icell);
		auto y = Geometry().Strip2posUVW(strip_dir, strip_num);
		result->Fill(x, y, val);
	}
	return result;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
// get three directions on: XY, XZ, YZ planes
Reconstr_hist&& HistoManager::Get(double radius, int rebin_space, int rebin_time, int method) {

	static std::function<std::pair<bool, TVector2>(std::array<int, 3>, double)> fn = [](std::array<int, 3> nums, double rad)->std::pair<bool, TVector2> {
		TVector2 pos;
		return { Geometry().MatchCrossPoint(nums, rad, pos), pos };
	};
	static RV_Storage<std::pair<bool, TVector2>, std::array<int, 3>, double> MatchCrossPoint_functor(fn); //return value manager, saves already found crossing strips

	Reconstr_hist h_all;
	bool err_flag = false;
	if (std::any_of(dirs.begin(), dirs.end(), [&](direction dir_) { return hitsObject->GetNhits(dir_) < 1; })) return std::move(h_all);

	//std::cout << Form(">>>> EventId = %lld", event_id) << std::endl;

	for (auto hits_in_time_cell_begin = hitsObject->hitListByTimeDir.begin(); hits_in_time_cell_begin != hitsObject->hitListByTimeDir.end(); hits_in_time_cell_begin = hitsObject->hitListByTimeDir.upper_bound(position_by_time_reduced{ std::get<0>(*hits_in_time_cell_begin), std::numeric_limits<direction>::max(), std::numeric_limits<int>::max() })) { //iterate over time cells
		auto icell = std::get<0>(*hits_in_time_cell_begin);
		std::map<direction, std::vector<int>> hits_strip_nums_in_dir;
		for (auto dir : dirs) {
			auto min_hit = hitsObject->hitListByTimeDir.lower_bound(position_by_time_reduced{ icell, dir, std::numeric_limits<int>::min() });
			auto max_hit = hitsObject->hitListByTimeDir.upper_bound(position_by_time_reduced{ icell, dir, std::numeric_limits<int>::max() });
			for (auto hit = min_hit; hit != max_hit; hit++) {
				hits_strip_nums_in_dir[dir].push_back(std::get<2>(*hit));
			}
		}
		// check if there is at least one hit in each direction
		if (std::any_of(dirs.begin(), dirs.end(), [&](direction dir) { return hits_strip_nums_in_dir[dir].size() == 0; })) continue;

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
		auto event_id = chargesObject->Info().EventId();
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
			for (auto dir : dirs) {
				uvw1[dir] = it1.first[int(dir)];
				charge_along_direction[dir] = chargesObject->GetValByStrip(dir, uvw1[dir], icell);
			}

			// loop over directions
			for (auto& it2 : hitPos) {
				std::map<direction, int> uvw2;
				for (auto dir : dirs) {
					uvw2[dir] = it2.first[int(dir)];
				}

				for (auto dir1 : dirs) {
					if (uvw1[dir1] == uvw2[dir1]) {
						for (auto dir2 : dirs) {
							if (dir2 != dir1) {
								charge_along_direction[dir2] += chargesObject->GetValByStrip(dir2, uvw2[dir2], icell);
							}
						}
					}
				}
			}
			for (auto dir : dirs) {
				auto charge_in_strip = chargesObject->GetValByStrip(dir, uvw1[dir], icell); // charge in a given strip (for a given time range)
				fraction[dir].insert({ it1.first, charge_in_strip / charge_along_direction[dir] });
			}
		}

		// loop over matched hits and fill histograms
		for (auto&& it : hitPos) {

			double val = 0.5 * // divide charge according to charge-fraction in two other directions
				std::inner_product(dirs.begin(), dirs.end(), it.first.begin(), 0.0, std::plus<>(), [&](direction dir, int key) {
				return chargesObject->GetValByStrip(dir, key, icell) * std::inner_product(fraction.begin(), fraction.end(), dirs.begin(), 0.0, std::plus<>(), [&](auto& map_, direction dir2) {
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
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH1D> HistoManager::GetTimeProjection_Hits(direction strip_dir) {  // valid range [0-2]
	auto result = std::make_shared<TH1D>(Form("hclust_%stime_evt%lld", Geometry().GetDirName(strip_dir).c_str(), chargesObject->Info().EventId),
		Form("Event-%lld: Clustered hits from %s strips;Time bin [arb.u.];Charge/bin [arb.u.]",
			chargesObject->Info().EventId, Geometry().GetDirName(strip_dir).c_str()),
		Geometry().GetAgetNtimecells(),
		0.0 - 0.5,
		1. * Geometry().GetAgetNtimecells() - 0.5); // ends at 511.5 (cells numbered from 0 to 511)
	if (hitsObject->GetNhits(strip_dir) < 1) return result;
	// fill new histogram
	auto min_hit = hitsObject->hitList.lower_bound(std::make_tuple( strip_dir, std::numeric_limits<int>::min(), std::numeric_limits<int>::min() ));
	auto max_hit = hitsObject->hitList.upper_bound(std::make_tuple( strip_dir, std::numeric_limits<int>::max(), std::numeric_limits<int>::max() ));
	for (auto hit = min_hit; hit != max_hit; hit++) {
		auto strip_num = std::get<1>(*hit);
		auto icell = std::get<2>(*hit);
		double val = chargesObject->GetValByStrip(strip_dir, strip_num, icell);
		result->Fill(1. * icell, val);
	}
	return result;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH1D> HistoManager::GetTimeProjection_Hits() {  // all strips
	auto result = std::make_shared<TH1D>(Form("hclust_time_evt%lld", chargesObject->Info().EventId),
		Form("Event-%lld: Clustered hits from all strips;Time bin [arb.u.];Charge/bin [arb.u.]", chargesObject->Info().EventId),
		Geometry().GetAgetNtimecells(),
		0.0 - 0.5,
		1. * Geometry().GetAgetNtimecells() - 0.5); // ends at 511.5 (cells numbered from 0 to 511)
	if (hitsObject->GetNhits() == 0) return result;
	// fill new histogram
	for (auto strip_dir : dirs) {
		auto min_hit = hitsObject->hitList.lower_bound(std::make_tuple( strip_dir, std::numeric_limits<int>::min(), std::numeric_limits<int>::min()));
		auto max_hit = hitsObject->hitList.upper_bound(std::make_tuple( strip_dir, std::numeric_limits<int>::max(), std::numeric_limits<int>::max()));
		for (auto hit = min_hit; hit != max_hit; hit++) {
			auto strip_num = std::get<1>(*hit);
			auto icell = std::get<2>(*hit);
			double val = chargesObject->GetValByStrip(strip_dir, strip_num, icell);
			result->Fill(1. * icell, val);
		}
	}
	return result;
}
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
std::shared_ptr<TH1D> HistoManager::GetTimeProjection_Charges() {  // whole event, all strips
	auto result = std::make_shared<TH1D>(Form("hraw_time_evt%lld", chargesObject->Info().EventId),
		Form("Event-%lld: Raw signals from all strips;Time bin [arb.u.];Charge/bin [arb.u.]", chargesObject->Info().EventId),
		Geometry().GetAgetNtimecells(),
		0.0 - 0.5,
		1. * Geometry().GetAgetNtimecells() - 0.5); // ends at 511.5 (cells numbered from 0 to 511)
  // fill new histogram
	int counter = 0;
	for (int icobo = 0; icobo < Geometry().GetCoboNboards(); icobo++) {
		for (int iasad = 0; iasad < Geometry().GetAsadNboards(icobo); iasad++) {
			for (int ichan = 0; ichan < Geometry().GetAgetNchannels() * Geometry().GetAgetNchips(); ichan++) {
				counter++;
				for (int icell = 0; icell <= Geometry().GetAgetNtimecells(); icell++) {
					double val = chargesObject->GetValByGlobalChannel(counter, icell);
					if (val != 0.0) result->Fill(1. * icell, val);
				}
			}
		}
	}
	return result;
}