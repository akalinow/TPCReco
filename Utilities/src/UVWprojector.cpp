// Class for 3D event projection to UVW strips.
// VERSION: 11 Feb 2018

#include <cstdlib>
#include <cstddef>  // for: NULL
#include <iostream> // for: cout, cerr, endl
#include <vector>
#include <map>

#include "TROOT.h"
#include "TGraph.h"
#include "TH2.h"
#include "TH2D.h"
#include "TH3D.h"
#include "TH2Poly.h"
#include "TVector2.h"
#include "TRandom3.h"
#include "TMath.h"
#include "TPad.h"

#include "GeometryTPC.h"
#include "UVWprojector.h"


// Setter methods

// constructor
UVWprojector::UVWprojector(int n, int nx, int ny)
	: area_npoints(1),
	isOK_AreaMapping(false),
	isOK_TimeMapping(false),
	input_hist(NULL),
	is_input_2D(false)
{
	SetAreaNpoints(n);
	Geometry().SetTH2PolyPartition(nx, ny);
}

// destructor
UVWprojector::~UVWprojector() {
	if (is_debug) {
		std::cout << "UVWprojector: destructor called..." << std::endl;
	}
	if (input_hist) input_hist->Delete();
}

// change number of points used for random probing of TH2Poly bins
void UVWprojector::SetAreaNpoints(int n) {
	if (n > 0 && n != area_npoints) { area_npoints = n; isOK_AreaMapping = false; }
}

// 3D ionization map: (x [mm], y [mm], z [mm], Q [arb.u.])
void UVWprojector::SetEvent3D(TH3D& h3) {

	// remove remnants of the previous event,
	// but do not alter mapping if the new histogram has exactly the same binning
	TH3D* h = (TH3D*)input_hist;
	if (h) {

		// check X,Y binning
		if (!CheckBinsXY(h, &h3)) {
			fAreaFractionMap.clear();
			isOK_AreaMapping = false;
		}

		// check Z binning
		if (!CheckBinsZ(h, &h3)) {
			fTimeFractionMap.clear();
			isOK_TimeMapping = false;
		}

		input_hist->Delete();
	}

	// make copy of input TH3D
	h = new TH3D(h3);
	input_hist = (TH1*)h;
	if (!input_hist) return;

	is_input_2D = false;

	// update binning/mapping when necessary
	//  if(!isOK_TH2Poly)     isOK_TH2Poly = InitTH2Poly();
	if (!isOK_TimeMapping) isOK_TimeMapping = InitTimeMapping();
	if (!isOK_AreaMapping) isOK_AreaMapping = InitAreaMapping();

	// DEBUG
	if (is_debug) {
		std::cout << "SetEvent3D: Final result: "
			<< "IS_2D=" << is_input_2D << ", TIMEMAP_OK=" << isOK_TimeMapping << ", AREAMAP_OK=" << isOK_AreaMapping << std::endl;
	}
	// DEBUG
}

// 2D ionization map: (x [mm], y [mm], Q [arb.u.])
void UVWprojector::SetEvent2D(TH2D& h2) {

	// remove remnants of the previous event,
	// but do not alter mapping if the new histogram has exactly the same binning
	TH2D* h = (TH2D*)input_hist;
	if (h) {

		// check X,Y binning
		if (!CheckBinsXY(h, &h2)) {
			fAreaFractionMap.clear();
			isOK_AreaMapping = false;
		}

		fTimeFractionMap.clear();
		isOK_TimeMapping = false;
		input_hist->Delete();
	}

	// make copy of input TH2D
	h = new TH2D(h2);
	input_hist = (TH1*)h;
	if (!input_hist) return;

	is_input_2D = true;  // input event contains only time-intergral 

	// update binning/mapping when necessary
	if (!isOK_AreaMapping) isOK_AreaMapping = InitAreaMapping();

	// DEBUG
	if (is_debug) {
		std::cout << "SetEvent2D: Final result: "
			<< "IS_2D=" << is_input_2D << ", AREAMAP_OK=" << isOK_AreaMapping << std::endl;
	}
	// DEBUG
}

void UVWprojector::AddBinContent(Int_t bin, Double_t w) {

	// sanity checks
	auto tp = Geometry().GetTH2Poly();
	if (!tp || !tp->GetBins() || bin > tp->GetNumberOfBins() || bin == 0 || bin < -9) return;
	if (bin < 0) { tp->SetBinContent(bin, tp->GetBinContent(bin) + w); return; }

	((TH2PolyBin*)tp->GetBins()->At(bin - 1))->SetContent(((TH2PolyBin*)tp->GetBins()->At(bin - 1))->GetContent() + w);
	Double_t st[7];
	tp->GetStats(st);
	st[0] += w; //fTsumw   = fTsumw + w;
	//fTsumwx  = fTsumwx + w*x;
	//fTsumwx2 = fTsumwx2 + w*x*x;
	//fTsumwy  = fTsumwy + w*y;
	//fTsumwy2 = fTsumwy2 + w*y*y;
	tp->PutStats(st);
	if (tp->GetSumw2()->GetSize()) tp->GetSumw2()->AddAt(tp->GetSumw2()->At(bin - 1) + w * w, bin - 1);// if (fSumw2.fN) fSumw2.fArray[bin-1] += w*w;
	tp->SetEntries(tp->GetEntries() + 1);// fEntries++;
	tp->SetBinContentChanged(kTRUE);
}

void UVWprojector::SetBinContent(Int_t bin, Double_t w) {

	// sanity checks

	auto tp = Geometry().GetTH2Poly();
	if (!tp || !tp->GetBins() || bin > tp->GetNumberOfBins() || bin == 0 || bin < -9) return;

	if (bin < 0) { tp->SetBinContent(bin, w); return; }
	((TH2PolyBin*)tp->GetBins()->At(bin - 1))->SetContent(w);
}

bool UVWprojector::InitAreaMapping() {

	TH2D* h2 = (TH2D*)input_hist;

	isOK_AreaMapping = false;
	fAreaFractionMap.clear();

	// sanity checks
	auto tp = Geometry().GetTH2Poly();
	if (area_npoints < 1 || !h2 || !tp) {

		// DEBUG
		if (is_debug) {
			std::cerr << "InitAreaMapping: ERROR: Failed sanity checks (1): "
				<< "NPOINTS=" << area_npoints << ", HIST_PTR=" << h2 << ", TH2POLY=" << tp << std::endl;
		}
		// DEBUG

		return false;
	}

	const double xmin = h2->GetXaxis()->GetXmin();  // minimal X-position [mm]
	const double xmax = h2->GetXaxis()->GetXmax();  // maximal X-position [mm]
	const int nxbins = h2->GetNbinsX(); // number of bins along X-axis   
	const double ymin = h2->GetYaxis()->GetXmin();  // minimal Y-position [mm]
	const double ymax = h2->GetYaxis()->GetXmax();  // maximal Y-position [mm]
	const int nybins = h2->GetNbinsY(); // number of bins along Y-axis   

	// sanity checks
	if (xmin >= xmax || ymin >= ymax || nxbins <= 0 || nybins <= 0) {

		// DEBUG
		if (is_debug) {
			std::cerr << "InitAreaMapping: ERROR: Failed sanity checks (2): "
				<< "XMIN=" << xmin << ", XMAX=" << xmax << ", YMIN=" << ymin << ", YMAX=" << ymax << ", NX=" << nxbins << ", NY=" << nybins << std::endl;
		}
		// DEBUG

		return false;
	}

	const double dx = (xmax - xmin) / nxbins;
	const double dy = (ymax - ymin) / nybins;

	// create list of sampling points within [0, dx[ x [0, dy[ rectangle,
	// always include the bin centre and randomly distribute the rest
	std::vector<TVector2> offset;
	offset.push_back(TVector2(0.5 * dx, 0.5 * dy));
	TRandom3* rand = new TRandom3();
	for (int ipoint = 1; ipoint < area_npoints; ipoint++) {
		offset.push_back(TVector2(rand->Rndm() * dx, rand->Rndm() * dy));
	}

	// for each cartesian (X,Y) bin of the event histogram prepare:
	// - list of TH2Poly bins partially enclosed in the cartesian bin
	// - for each TH2Poly bin of the list estimate ratio of the enclosed surface
	//   to the total surface of the cartesian bin
	double weight = 1. / area_npoints;
	for (int ibinx = 0; ibinx < nxbins; ibinx++) {
		for (int ibiny = 0; ibiny < nybins; ibiny++) {
			TVector2 pos(xmin + ibinx * dx, ymin + ibiny * dy);
			for (int ipoint = 0; ipoint < area_npoints; ipoint++) {
				TVector2 probe = pos + offset[ipoint];
				int ibin = tp->FindBin(probe.X(), probe.Y()); // TH2Poly bin index

				// DEBUG - VERY VERBOSE
				//	if(is_debug) {
				//	  std::cout << "InitAreaMapping: Probing (X=" << probe.X() << ", Y=" << probe.Y() <<"): TH2POLY_BIN=" << ibin << std::endl;
				//	}
				// DEBUG - VERY VERBOSE

				if (ibin < 1) continue; // skip underflow/overflow/sea bins
				BinFracMap& bmap = fAreaFractionMap[std::make_tuple(ibinx, ibiny)];
				if (bmap.FracMap.find(ibin) == bmap.FracMap.end()) {
					bmap.FracMap[ibin] = weight;  // assign weight to a new strip
				}
				else {
					bmap.FracMap[ibin] += weight; // increase weight of existing strip 
				}
			} // end of random sampling loop      
		}
	} // end of loop over all (X,Y) bins

	// DEBUG
	if (is_debug) {
		for (auto it : fAreaFractionMap) {
			for (auto it2 : it.second.FracMap) {
				std::cout << "TH2D bin IX=" << std::get<0>(it.first)
					<< ", IY=" << std::get<1>(it.first)
					<< ": TH2POLY_IBIN=" << it2.first
					<< ", WEIGHT=" << it2.second << std::endl;
			}
		}
	}
	// DEBUG

	// final result
	if (fAreaFractionMap.size() > 0) isOK_AreaMapping = true;

	// DEBUG
	if (is_debug) {
		std::cout << "InitAreaMapping: Final result: "
			<< "fAreaFractionMap.size()=" << fAreaFractionMap.size() << ", AREAMAP_OK=" << isOK_AreaMapping << std::endl;
	}
	// DEBUG

	return isOK_AreaMapping;
}

bool UVWprojector::InitTimeMapping() {

	if (is_input_2D) return false; // input event contains only time-intergral 

	TH3D* h3 = (TH3D*)input_hist;

	isOK_TimeMapping = false;
	fTimeFractionMap.clear();

	// sanity checks
	if (!h3) return false;

	const int time_nbins = Geometry().GetAgetNtimecells(); // number of time buckets in digitizing electronics
	const double h3_zmin = h3->GetZaxis()->GetXmin();  // Z-position [mm] closest to readout PCB
	const double h3_zmax = h3->GetZaxis()->GetXmax();  // Z-position [mm] farthest from readout PCB
	const int h3_nbins = h3->GetNbinsZ(); // number of bins along Z-axis   
	const double vdrift = Geometry().GetVdrift(); // electron drift velocity [cm/us]
	const double drift_zmin = Geometry().GetDriftCageZmin(); // drift cage lower acceptance limit [mm]
	const double drift_zmax = Geometry().GetDriftCageZmax(); // drift cage upper acceptance limit [mm]
	const double freq = Geometry().GetSamplingRate(); // electronics sampling rate [MHz]
	const double delay = Geometry().GetTriggerDelay(); // electronics trigger delay [us]

	// sanity checks
	if (vdrift <= 0.0 || freq <= 0.0 || delay < 0.0 || h3_zmin >= h3_zmax || drift_zmin >= drift_zmax ||
		time_nbins <= 0 || h3_nbins <= 0) return false;

	// Z-axis bin boundaries [mm] for event histogram
	std::vector<double> h3_bins;
	const double h3_dz = (h3_zmax - h3_zmin) / h3_nbins;
	for (int ibin = 0; ibin <= h3_nbins; ibin++) {
		h3_bins.push_back(h3_zmin + ibin * h3_dz);
	}

	// Z-axis bin boundaries [mm] for digitizing electronics
	std::vector<double> time_bins;
	const double time_dz = 10. * vdrift / freq; // [mm] = [ 10 mm/cm * cm/us * 1/MHz ]
	const double time_zmin = Geometry().Timecell2pos(0.0); // Z-position of the lower boundary of time cell no. 0]
	const double time_zmax = time_zmin + time_nbins * time_dz; // electronics limit


	for (int ibin = 0; ibin <= time_nbins; ibin++) {
		time_bins.push_back(time_zmin + ibin * time_dz);
	}

	// DEBUG
	if (is_debug) {
		double z1 = (time_zmin < drift_zmin ? drift_zmin : time_zmin); // physical drift cage size limit
		double z2 = (time_zmax > drift_zmax ? drift_zmax : time_zmax); // physical drift cage size limit
		std::cout << "V_drift = " << vdrift << " cm/us" << std::endl;
		std::cout << "Trg_delay = " << delay << " us" << std::endl;
		std::cout << "Input histogram Z-range = [" << h3_zmin
			<< ", " << h3_zmax << "] mm" << std::endl;
		std::cout << "Electronics acceptance Z-range = [" << time_zmin
			<< ", " << time_zmax
			<< "] mm" << std::endl;
		std::cout << "Physical drift cage Z-range = [" << drift_zmin
			<< ", " << drift_zmax
			<< "] mm" << std::endl;
		std::cout << "Effective number of time cells = " << (int)((z2 - z1) / time_dz) << " (out of " << time_nbins << ")" << std::endl;
	}
	// DEBUG

	// for each Z-axis bin (slice) of the event histogram prepare:
	// - list of time cells fully or partially enclosed in the slice
	// - for each time cell of the list calculate ratio of enclosed part od the 
	//   time cell to the total height of the slice
	int time_istart = 0;
	while (time_bins[time_istart] < h3_bins[0] &&
		time_bins[time_istart] < drift_zmax &&
		time_istart < time_nbins) time_istart++;

	for (int h3_ibin = 0; h3_ibin < h3_nbins; h3_ibin++) {

		// DEBUG
		if (is_debug) {
			std::cout << "Slice IZ=" << h3_ibin + 1
				<< ": Z-range = [" << h3_bins[h3_ibin]
				<< ", " << h3_bins[h3_ibin + 1] << "] mm" << std::endl;
		}
		// DEBUG

		double fraction = 0.0;
		if (time_istart > 0) {
			fraction = (time_bins[time_istart] - h3_bins[h3_ibin]) / h3_dz;
			if (fraction <= 1.0 && fraction > 0.0) {
				// add this fraction to [h3_ibin+1][time_istart-1] map
				fTimeFractionMap[h3_ibin + 1].FracMap[time_istart - 1] = fraction;

				// DEBUG
				if (is_debug) {
					std::cout << "Slice IZ=" << h3_ibin + 1
						<< ": lower, partially enclosed: TIME_CELL=" << time_istart - 1
						<< ", WEIGHT=" << fraction << std::endl;
				}
				// DEBUG

			}
		}
		for (int time_ibin = time_istart; time_ibin < time_nbins; time_ibin++) {

			// DEBUG
			if (is_debug) {
				std::cout << "SLICE IZ=" << h3_ibin + 1
					<< ": TIME_CELL=" << time_ibin
					<< ": Z-range = [" << time_bins[time_ibin]
					<< ", " << time_bins[time_ibin + 1] << "] mm" << std::endl;
			}
			// DEBUG

			if (time_bins[time_ibin] + time_dz < h3_bins[h3_ibin + 1] &&
				time_bins[time_ibin] + time_dz < drift_zmax) {
				fraction = time_dz / h3_dz;
				// add this fraction to [h3_ibin+1][time_ibin] map
				fTimeFractionMap[h3_ibin + 1].FracMap[time_ibin] = fraction;

				// DEBUG
				if (is_debug) {
					std::cout << "Slice IZ=" << h3_ibin + 1
						<< ": middle, fully enclosed: TIME_CELL=" << time_ibin
						<< ", WEIGHT=" << fraction << std::endl;
				}
				// DEBUG

			}
			else {
				fraction = (h3_bins[h3_ibin + 1] - time_bins[time_ibin]) / h3_dz;
				if (fraction <= 1.0 && fraction > 0.0 &&
					time_bins[time_ibin] + time_dz < drift_zmax) {
					// add this fraction to [h3_ibin+1][time_ibin] map
					fTimeFractionMap[h3_ibin + 1].FracMap[time_ibin] = fraction;

					// DEBUG
					if (is_debug) {
						std::cout << "Slice IZ=" << h3_ibin + 1
							<< ": upper, partially enclosed: TIME_CELL=" << time_ibin
							<< ", WEIGHT=" << fraction << std::endl;
					}
					// DEBUG
				}
				time_istart = time_ibin + 1;
				break;  // go to the next h3_ibin
			}
		}
	}

	// DEBUG
	if (is_debug) {
		for (auto it : fTimeFractionMap) {
			for (auto it2 : it.second.FracMap) {
				std::cout << "Slice IZ=" << it.first
					<< ": TIME_CELL=" << it2.first
					<< ", WEIGHT=" << it2.second << std::endl;
			}
		}
	}
	// DEBUG

	// final result
	if (fTimeFractionMap.size() > 0) isOK_TimeMapping = true;
	return isOK_TimeMapping;
}

// Getter methods

// Get TH1D of time-integrated strip projection for 3D or 2D event (SELECTED DIRECTION)
std::shared_ptr<TH1D> UVWprojector::GetStripProfile_TH1D(direction dir) {

	// sanity checks
	//  if(!isOK_TH2Poly || !isOK_AreaMapping || !input_hist || dir<0) return NULL;
	if (!isOK_AreaMapping || !input_hist) return NULL;

	// create a 2D clone with same X-Y binning
	TH2D* h2 = (TH2D*)input_hist;
	const double xmin = h2->GetXaxis()->GetXmin();  // minimal X-position [mm]
	const double xmax = h2->GetXaxis()->GetXmax();  // maximal X-position [mm]
	const int nxbins = h2->GetNbinsX(); // number of bins along X-axis   
	const double ymin = h2->GetYaxis()->GetXmin();  // minimal Y-position [mm]
	const double ymax = h2->GetYaxis()->GetXmax();  // maximal Y-position [mm]
	const int nybins = h2->GetNbinsY(); // number of bins along Y-axis   
	// sanity checks
	if (xmin >= xmax || ymin >= ymax || nxbins <= 0 || nybins <= 0) return NULL;
	TH2D* h2temp = new TH2D("h2temp", "",
		nxbins, xmin, xmax,
		nybins, ymin, ymax);

	// Project TH3D (with clipping along Z-axis) to TH2D for 3D event case
	if (!is_input_2D) {
		// sanity checks
		if (!isOK_TimeMapping) return NULL;

		// 3D histogram to be sliced/projected
		TH3D* h3 = (TH3D*)input_hist;
		const double zmin = h3->GetZaxis()->GetXmin();  // Z-position [mm] closest to readout PCB
		const double zmax = h3->GetZaxis()->GetXmax();  // Z-position [mm] farthest from readout PCB
		const int nzbins = h3->GetNbinsZ(); // number of bins along Z-axis   
		// sanity checks
		if (zmin >= zmax || nzbins <= 0) return NULL;

		// loop over all Z-slices
		std::map<int, BinFracMap>::const_iterator it;
		std::map<int, double>::const_iterator it2;

		for (it = fTimeFractionMap.cbegin(); it != fTimeFractionMap.cend(); it++) {
			// calculate weight of each slice, range [0-1]
			double weight = 0.0;
			int iz = it->first;
			if (iz<1 || iz>nzbins) continue;
			for (it2 = (it->second).FracMap.cbegin(); it2 != (it->second).FracMap.cend(); it2++) {
				// DEBUG
				if (is_debug) {
					std::cout << "Slice IZ=" << iz
						<< ": adding contribution of TIME_CELL=" << it2->first
						<< ", WEIGHT=" << it2->second << std::endl;
				}
				// DEBUG

				weight += it2->second;
			}

			// skip slices out of electronics time acceptance window
			if (weight <= 0.0) continue;

			// project Z-slices to h2temp with proper weight
			for (int ix = 1; ix <= nxbins; ix++) {
				for (int iy = 1; iy <= nybins; iy++) {
					h2temp->AddBinContent(h2temp->GetBin(ix, iy),
						h3->GetBinContent(ix, iy, iz) * weight);
					// DEBUG
					if (is_debug) {
						double v = h3->GetBinContent(ix, iy, iz) * weight;
						if (v != 0.0) std::cout << "Slice IZ=" << iz
							<< ": TH2D bin IX=" << ix
							<< ", IY=" << iy
							<< ": adding VAL=" << v
							<< " (weight=" << weight << ")" << std::endl;
					}
					// DEBUG
				}
			}
		}
	}
	else { // 2D event case

   // copy contents of h2 to h2temp as-is
		for (int ix = 1; ix <= nxbins; ix++) {
			for (int iy = 1; iy <= nybins; iy++) {
				h2temp->SetBinContent(h2temp->GetBin(ix, iy),
					h2->GetBinContent(ix, iy));
				// DEBUG
				if (is_debug) {
					std::cout << "TH2D bin IX=" << ix
						<< ", IY=" << iy
						<< ": adding VAL=" << h2->GetBinContent(ix, iy) << std::endl;
				}
				// DEBUG
			}
		}

	}

	// Create TH1D with strip numbers for a given direction
	auto newth1 = std::make_shared<TH1D>("",
		Form("Time-integrated charge for %s-strips;%s strip number;Charge [arb.u.]",
			Geometry().GetDirName(dir).c_str(), Geometry().GetDirName(dir).c_str()),
		Geometry().GetDirNstrips(dir),
		1.,
		(Geometry().GetDirNstrips(dir) + 1) * 1.);

	// Project TH2D content to TH1D
	for (const auto& it : fAreaFractionMap) {
		for (const auto& it2 : it.second.FracMap) {
			int ix = std::get<0>(it.first);
			int iy = std::get<1>(it.first);
			int ibin = it2.first;       // TH2Poly
			auto& strip_data = (*Geometry().GetTH2PolyStrip(ibin))();
			if (strip_data.dir == dir) {
				int strip_num = strip_data.num; // valid range [1-1024] 
				double weight = it2.second;
				if (weight <= 0.0) continue;
				newth1->AddBinContent(strip_num, h2temp->GetBinContent(ix, iy) * weight);

				// DEBUG
				if (is_debug) {
					double v = h2temp->GetBinContent(ix, iy) * weight;
					if (v != 0.0) std::cout << "Strip DIR=" << dir
						<< ", NUM=" << strip_num
						<< ": adding VAL=" << v
						<< " (ix=" << ix
						<< ", iy=" << iy
						<< ", weight=" << weight << ")" << std::endl;
				}
				// DEBUG
			}
		}
	}
	h2temp->Delete();
	return newth1;
}

// Get TH2D of strip vs time projection for 3D event (SELECTED DIRECTION)
std::shared_ptr<TH2D> UVWprojector::GetStripVsTime_TH2D(direction dir) {

	// sanity checks
	if (is_input_2D || !isOK_TimeMapping || !isOK_AreaMapping || !input_hist) return NULL;

	// create a 2D clone with same X-Y binning
	TH2D* h2 = (TH2D*)input_hist;
	const double xmin = h2->GetXaxis()->GetXmin();  // minimal X-position [mm]
	const double xmax = h2->GetXaxis()->GetXmax();  // maximal X-position [mm]
	const int nxbins = h2->GetNbinsX(); // number of bins along X-axis   
	const double ymin = h2->GetYaxis()->GetXmin();  // minimal Y-position [mm]
	const double ymax = h2->GetYaxis()->GetXmax();  // maximal Y-position [mm]
	const int nybins = h2->GetNbinsY(); // number of bins along Y-axis   

	// sanity checks
	if (xmin >= xmax || ymin >= ymax || nxbins <= 0 || nybins <= 0) return NULL;

	// Project TH3D (with clipping along Z-axis) to TH2D for 3D event case

	// 3D histogram to be sliced/projected
	TH3D* h3 = (TH3D*)input_hist;
	const double zmin = h3->GetZaxis()->GetXmin();  // Z-position [mm] closest to readout PCB
	const double zmax = h3->GetZaxis()->GetXmax();  // Z-position [mm] farthest from readout PCB
	const int nzbins = h3->GetNbinsZ(); // number of bins along Z-axis   

	// sanity checks
	if (zmin >= zmax || nzbins <= 0) return NULL;

	// Create TH2D with strip numbers for a given direction
	auto newth2 = std::make_shared<TH2D>("",
		Form("Signal(time) for %s-strips;Time cell;%s strip number",
			Geometry().GetDirName(dir).c_str(), Geometry().GetDirName(dir).c_str()),
		Geometry().GetAgetNtimecells(),
		0.,
		1. * Geometry().GetAgetNtimecells(),
		Geometry().GetDirNstrips(dir),
		1.,
		(Geometry().GetDirNstrips(dir) + 1) * 1.);


	// loop over all Z-slices
	for (const auto& it : fTimeFractionMap) {

		int iz = it.first; // Z-slice

		// sanity checks
		if (iz<1 || iz>nzbins) continue;

		// loop over all mapped time cells for a give Z-slice
		for (const auto& it2 : it.second.FracMap) {

			const double time_ibin = it2.first;
			const double weightT = it2.second;

			// DEBUG
			if (is_debug) {
				std::cout << "Slice IZ=" << iz
					<< ": adding contribution of TIME_CELL=" << it2.first
					<< ", WEIGHT=" << it2.second << std::endl;
			}
			// DEBUG

			// loop over all mapped X,Y bins and project TH3D to TH2D using proper weights
			for (const auto& it3 : fAreaFractionMap) {
				for (const auto& it4 : it3.second.FracMap) {
					int ix = std::get<0>(it3.first);
					int iy = std::get<1>(it3.first);
					int ibin = it4.first;       // TH2Poly bin index

					auto& strip_data = (*Geometry().GetTH2PolyStrip(ibin))();
					if (strip_data.dir == dir) {

						//////////// DEBUG 
						//	    std::cout << "BREAKPOINT: ibin=" << ibin << std::endl << std::flush;
						//	    std::cout << "BREAKPOINT: ibin=" << ibin << ", fStripMap[ibin]=" << fStripMap[ibin] << std::endl << std::flush;
						//////////// DEBUG 

						const int strip_num = strip_data.num; // valid range [1-1024] 
						const double weight = it4.second;

						if (weight <= 0.0) continue;
						newth2->Fill(time_ibin * 1., strip_num, h3->GetBinContent(ix, iy, iz) * weight * weightT);

						// DEBUG
						if (is_debug) {
							double v = h3->GetBinContent(ix, iy, iz) * weight * weightT;
							if (v != 0.0) std::cout << "Strip DIR=" << dir
								<< ", NUM=" << strip_num
								<< ": adding VAL=" << v
								<< " (ix=" << ix
								<< ", iy=" << iy
								<< ", iz=" << iz
								<< ", weight=" << weight * weightT << ")" << std::endl;
						}
						// DEBUG
					}
				}
			}
		}
	}
	return newth2;
}

std::shared_ptr<TH2Poly> UVWprojector::GetStripProfile_TH2Poly() {

	// sanity checks
	auto tp = Geometry().GetTH2Poly();
	if (!tp || !isOK_AreaMapping || !input_hist) {

		// DEBUG
		if (is_debug) {
			std::cerr << "GetStripProfile_TH2Poly: ERROR: Failed sanity checks (1): "
				<< "TH2POLY=" << tp << ", AREAMAP_OK=" << isOK_AreaMapping << ", HIST_PTR=" << input_hist << std::endl;
		}
		// DEBUG

		return NULL;
	}

	// DEBUG
	if (is_debug) {
		std::cout << "TH2POLY ORIG NBINS=" << tp->GetNumberOfBins() << std::endl;
	}
	// DEBUG

	// clear bin contents of the original TH2POLY
	tp->ClearBinContents();

	// create a 2D clone with same X-Y binning
	TH2D* h2 = (TH2D*)input_hist;
	const double xmin = h2->GetXaxis()->GetXmin();  // minimal X-position [mm]
	const double xmax = h2->GetXaxis()->GetXmax();  // maximal X-position [mm]
	const int nxbins = h2->GetNbinsX(); // number of bins along X-axis   
	const double ymin = h2->GetYaxis()->GetXmin();  // minimal Y-position [mm]
	const double ymax = h2->GetYaxis()->GetXmax();  // maximal Y-position [mm]
	const int nybins = h2->GetNbinsY(); // number of bins along Y-axis   

	// sanity checks
	if (xmin >= xmax || ymin >= ymax || nxbins <= 0 || nybins <= 0) {

		// DEBUG
		if (is_debug) {
			std::cerr << "GetStripProfile_TH2Poly: ERROR: Failed sanity checks (2) !!!" << std::endl;
		}
		// DEBUG

		return NULL;
	}
	auto h2temp = std::make_unique<TH2D>("h2temp", "",
		nxbins, xmin, xmax,
		nybins, ymin, ymax);

	// Project TH3D (with clipping along Z-axis) for 3D event case
	if (!is_input_2D) {

		// sanity checks
		if (!isOK_TimeMapping) {

			// DEBUG
			if (is_debug) {
				std::cerr << "GetStripProfile_TH2Poly: ERROR: Failed sanity checks (3) !!!" << std::endl;
			}
			// DEBUG

			return NULL;
		}

		// 3D histogram to be sliced/projected
		TH3D* h3 = (TH3D*)input_hist;
		const double zmin = h3->GetZaxis()->GetXmin();  // Z-position [mm] closest to readout PCB
		const double zmax = h3->GetZaxis()->GetXmax();  // Z-position [mm] farthest from readout PCB
		const int nzbins = h3->GetNbinsZ(); // number of bins along Z-axis   

		// sanity checks
		if (zmin >= zmax || nzbins <= 0) {

			// DEBUG
			if (is_debug) {
				std::cerr << "GetStripProfile_TH2Poly: ERROR: Failed sanity checks (4) !!!" << std::endl;
			}
			// DEBUG

			return NULL;
		}

		// loop over all Z-slices
		for (const auto& it : fTimeFractionMap) {
			// calculate weight of each slice, range [0-1]
			double weight = 0.0;
			int iz = it.first;
			if (iz<1 || iz>nzbins) continue;
			for (const auto& it2 : it.second.FracMap) {
				// DEBUG
				if (is_debug) {
					std::cout << "Slice IZ=" << iz
						<< ": mapping TIME_CELL=" << it2.first
						<< ", WEIGHT=" << it2.second << std::endl;
				}
				// DEBUG

				weight += it2.second;
			}

			// skip slices out of electronics time acceptance window
			if (weight <= 0.0) continue;

			// project Z-slices to h2temp with proper weight
			for (int ix = 1; ix <= nxbins; ix++) {
				for (int iy = 1; iy <= nybins; iy++) {
					h2temp->AddBinContent(h2temp->GetBin(ix, iy),
						h3->GetBinContent(ix, iy, iz) * weight);
					// DEBUG
					if (is_debug) {
						double v = h3->GetBinContent(ix, iy, iz) * weight;
						if (v != 0.0) std::cout << "Slice IZ=" << iz
							<< ": TH2D bin IX=" << ix
							<< ", IY=" << iy
							<< ": adding VAL=" << v
							<< " (weight=" << weight << ")" << std::endl;
					}
					// DEBUG
				}
			}
		}
	}
	else { // 2D event case

   // copy contents of h2 to h2temp as-is
		for (int ix = 1; ix <= nxbins; ix++) {
			for (int iy = 1; iy <= nybins; iy++) {
				h2temp->SetBinContent(h2temp->GetBin(ix, iy),
					h2->GetBinContent(ix, iy));
				// DEBUG
				if (is_debug) {
					std::cout << "TH2D bin IX=" << ix
						<< ", IY=" << iy
						<< ": adding VAL=" << h2->GetBinContent(ix, iy) << std::endl;
				}
				// DEBUG
			}
		}

	}

	// Project TH2D content to TH2Poly  
	for (const auto& it : fAreaFractionMap) {
		for (const auto& it2 : it.second.FracMap) {
			int ix = std::get<0>(it.first);
			int iy = std::get<1>(it.first);
			int ibin = it2.first;
			double weight = it2.second;
			if (weight <= 0.0) continue;

			this->AddBinContent(ibin, h2temp->GetBinContent(ix, iy) * weight); // customized UVWprojector add method for internal TH2Poly

			// DEBUG
			if (is_debug) {
				std::cout << "TH2Poly bin IBIN=" << ibin
					<< ": adding VAL=" << h2temp->GetBinContent(ix, iy) * weight
					<< " (IX=" << ix
					<< ", IY=" << iy
					<< ", weight=" << weight << ", new_val=" << tp->GetBinContent(ibin) << ")" << std::endl;
			}
			// DEBUG
		}
	}

	// create new TH2Poly with same partitioning
	auto th2p = std::make_shared<TH2Poly>(Form("%s_copy", tp->GetName()), tp->GetTitle(),
		tp->GetNbinsX(), tp->GetXaxis()->GetXmin(), tp->GetXaxis()->GetXmax(),
		tp->GetNbinsY(), tp->GetYaxis()->GetXmin(), tp->GetYaxis()->GetXmax());

	// copy all bin shapes and bin contents
	if (th2p) {
		for (int ibin = 1; ibin <= tp->GetNumberOfBins(); ibin++) {
			TH2PolyBin* bin = (TH2PolyBin*)tp->GetBins()->At(ibin - 1);
			int ibin_new = th2p->AddBin(bin->GetPolygon()->Clone());
			th2p->SetBinContent(ibin_new, bin->GetContent());

			// DEBUG
			if (is_debug) {
				std::cout << "GetStripProfile_TH2Poly: IBIN=" << ibin << ", VAL(IBIN)=" << tp->GetBinContent(ibin) << ", VAL(BIN)=" << bin->GetContent() << std::endl
					<< "                         IBIN_NEW=" << ibin_new << ", VAL(IBIN_NEW)=" << th2p->GetBinContent(ibin_new) << std::endl;
			}
			// DEBUG

		}
	}

	// clear bin contents of the original TH2POLY
	tp->ClearBinContents();

	// DEBUG
	if (is_debug) {
		std::cerr << "GetStripProfile_TH2Poly: Final result TH2POLY_PTR=" << th2p << std::endl;
	}
	// DEBUG

	return th2p;
}

bool UVWprojector::CheckBinsXY(TH3D* h1, TH3D* h2) {
	if (!h1 || !h2) return false;
	if (h1->GetNbinsX() != h2->GetNbinsX() || h1->GetNbinsY() != h2->GetNbinsY()) return false;
	if (!(h1->GetXaxis()) || !(h2->GetXaxis()) || !(h1->GetYaxis()) || !(h2->GetYaxis())) return false;
	if (h1->GetXaxis()->GetXmin() != h2->GetXaxis()->GetXmin() ||
		h1->GetXaxis()->GetXmax() != h2->GetXaxis()->GetXmax() ||
		h1->GetYaxis()->GetXmin() != h2->GetYaxis()->GetXmin() ||
		h1->GetYaxis()->GetXmax() != h2->GetYaxis()->GetXmax()) return false;
	return true; // X and Y bins are identical 
}

bool UVWprojector::CheckBinsXY(TH2D* h1, TH2D* h2) {
	if (!h1 || !h2) return false;
	if (h1->GetNbinsX() != h2->GetNbinsX() || h1->GetNbinsY() != h2->GetNbinsY()) return false;
	if (!(h1->GetXaxis()) || !(h2->GetXaxis()) || !(h1->GetYaxis()) || !(h2->GetYaxis())) return false;
	if (h1->GetXaxis()->GetXmin() != h2->GetXaxis()->GetXmin() ||
		h1->GetXaxis()->GetXmax() != h2->GetXaxis()->GetXmax() ||
		h1->GetYaxis()->GetXmin() != h2->GetYaxis()->GetXmin() ||
		h1->GetYaxis()->GetXmax() != h2->GetYaxis()->GetXmax()) return false;
	return true; // X and Y bins are identical 
}

bool UVWprojector::CheckBinsZ(TH3D* h1, TH3D* h2) {
	if (!h1 || !h2) return false;
	if (h1->GetNbinsZ() != h2->GetNbinsZ()) return false;
	if (!(h1->GetZaxis()) || !(h2->GetZaxis())) return false;
	if (h1->GetZaxis()->GetXmin() != h2->GetZaxis()->GetXmin() ||
		h1->GetZaxis()->GetXmax() != h2->GetZaxis()->GetXmax()) return false;
	return true; // Z bins are identical 
}

//ClassImp(UVWprojector)
