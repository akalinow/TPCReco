#include "GeometryTPC.h" 

GeometryTPC::GeometryTPC(std::string  fname, bool debug) 
  : initOK(false), 
    COBO_N(0), 
    AGET_Nchips(4), 
    AGET_Nchan(64), 
    AGET_Nchan_fpn(-1), 
    AGET_Nchan_raw(-1), 
    AGET_Ntimecells(512),
    pad_size(0.), 
    pad_pitch(0.), 
    strip_pitch(0.), 
    vdrift(0.), 
    sampling_rate(0.), 
    trigger_delay(0.), 
    drift_zmin(0.), 
    drift_zmax(0.),
    grid_nx(25),
    grid_ny(25),
    _debug(debug)
 {
   if(_debug) {
     std::cout << "GeometryTPC::Constructor - Started..." << std::endl;
   }

   tp = std::make_shared<TH2Poly>();
   tp->SetName("h_uvw_dummy");

   // default REFERENCE POINT position on XY plane
   reference_point.Set(0., 0.);

   // FPN channels in each AGET chip                                                                               
   FPN_chanId.resize(0);
   FPN_chanId.push_back(11);
   FPN_chanId.push_back(22);
   FPN_chanId.push_back(45);
   FPN_chanId.push_back(56);
   AGET_Nchan_fpn = FPN_chanId.size();
   AGET_Nchan_raw = AGET_Nchan + AGET_Nchan_fpn;
   
   // Set indices and names of strip coordinates
   dir2name.clear();
   dir2name.insert(std::pair<projection, std::string>(projection::DIR_U, "U"));
   dir2name.insert(std::pair<projection, std::string>(projection::DIR_V, "V"));
   dir2name.insert(std::pair<projection, std::string>(projection::DIR_W, "W"));
   dir2name.insert(std::pair<projection, std::string>(FPN_CH, "FPN"));
   
   name2dir.clear();
   for(auto&& it: dir2name) {
     name2dir.insert(std::pair<std::string, projection>(it.second, it.first));
   }

   // Load config file    
   initOK = Load(fname);

   if(_debug) {
     std::cout << "GeometryTPC::Constructor - Ended..." << std::endl;
   }
 }

bool GeometryTPC::Load(std::string fname) {
	if (_debug) {
		std::cout << "GeometryTPC::Load - Started..." << std::endl;
	}

	std::cout << "\n==== INITIALIZING TPC GEOMETRY - START ====\n\n";

	initOK = false;

	stripN.clear();
	ASAD_N.clear();
	fStripMap.clear();

	std::ifstream f(fname);
	std::map<projection, double> angle;
	std::vector<std::string> file_lines;
	auto find_line = [](std::vector<std::string>& vec, std::string str) {
		return std::find_if(/*std::execution::par, */vec.begin(), vec.end(), [&](std::string& str_) { return str_.find(str) != std::string::npos; }); //C++17
	};

	if (f.is_open() && f) {
		{
			//read file to memory
			std::string line;
			while (getline(f, line)) {
				file_lines.push_back(line);
			}
			f.close();
		}
		{
			// set U,V,W angles
			auto it = find_line(file_lines, "ANGLES:");
			if (it != file_lines.end() &&
				(sscanf(it->c_str(), "ANGLES: %lf %lf %lf", &angle[projection::DIR_U], &angle[projection::DIR_V], &angle[projection::DIR_W]) == 3)) {
				for (auto dir : proj_vec_UVW) {
					angle[dir] = fmod(fmod(angle[dir], 360.) + 360., 360.); // convert to range [0, 360 deg[ range 
				}
				if (fmod(angle[projection::DIR_U], 180.) != fmod(angle[projection::DIR_V], 180.) &&  // reject parallel / anti-parallel duplicates 
					fmod(angle[projection::DIR_V], 180.) != fmod(angle[projection::DIR_W], 180.) &&  // reject parallel / anti-parallel duplicates 
					fmod(angle[projection::DIR_W], 180.) != fmod(angle[projection::DIR_U], 180.)) { // reject parallel / anti-parallel duplicates 
					std::cout << Form("Angle of U/V/W strips wrt X axis = %lf / %lf / %lf deg",
						angle[projection::DIR_U], angle[projection::DIR_V], angle[projection::DIR_W])
						<< std::endl;
				}
				else {
					std::cerr << "ERROR: Wrong U/V/W angles !!!" << std::endl;
					return false;
				}
			}
			else {
				std::cerr << "ERROR: Wrong U/V/W angles !!!" << std::endl;
				return false;
			}
		}
		{
			// set pad size
			auto it = find_line(file_lines, "DIAMOND SIZE:");
			if (it != file_lines.end() && //line exists
				(sscanf(it->c_str(), "DIAMOND SIZE: %lf", &pad_size) == 1) && //line constains values
				pad_size > 0.0) { //values are correct
				std::cout << "Length of diamond edge = " << pad_size << " mm" << std::endl;
			}
			else {
				std::cerr << "ERROR: Wrong pad size !!!" << std::endl;
				if (_debug) {
					std::cout << "GeometryTPC::Load - Abort (1)" << std::endl;
				}
				return false;
			}
			pad_pitch = pad_size * std::sqrt(3.);
			strip_pitch = pad_size * 1.5;
		}
		{
			// set REFERECE POINT offset [mm]
			auto it = find_line(file_lines, "REFERENCE POINT:");
			double xoff = 0.0, yoff = 0.0;
			if (it != file_lines.end() &&
				(sscanf(it->c_str(), "REFERENCE POINT: %lf %lf", &xoff, &yoff) == 2) &&
				fabs(xoff) < 500. && fabs(yoff) < 500.) { // [mm]
				reference_point.Set(xoff, yoff);
				std::cout << "Reference point offset = [" << reference_point.X() << " mm, " << reference_point.Y() << " mm]" << std::endl;
			}
			else {
				std::cerr << "ERROR: Reference point coordinate >500 mm !!!" << std::endl;
				if (_debug) {
					std::cout << "GeometryTPC::Load - Abort (2)" << std::endl;
				}
				return false;
			}
		}
		{
			// set electron drift velocity [cm/us]
			auto it = find_line(file_lines, "DRIFT VELOCITY:");
			if (it != file_lines.end() &&
				(sscanf(it->c_str(), "DRIFT VELOCITY: %lf", &vdrift) == 1) &&
				vdrift > 0.0) {
				std::cout << "Drift velocity = " << vdrift << " cm/us" << std::endl;
			}
			else {
				std::cerr << "ERROR: Wrong drift velocity !!!" << std::endl;
				if (_debug) {
					std::cout << "GeometryTPC::Load - Abort (3)" << std::endl;
				}
				return false;
			}
		}
		{
			// set electronics sampling rate [MHz]
			auto it = find_line(file_lines, "SAMPLING RATE:");
			if (it != file_lines.end() &&
				(sscanf(it->c_str(), "SAMPLING RATE: %lf", &sampling_rate) == 1) &&
				sampling_rate > 0.0) {
				std::cout << "Sampling rate = " << sampling_rate <<" MHz" << std::endl;
			}
			else {
				std::cerr << "ERROR: Wrong sampling rate !!!" << std::endl;
				if (_debug) {
					std::cout << "GeometryTPC::Load - Abort (4)" << std::endl;
				}
				return false;
			}
		}
		{
			// set electronics trigger delay [us]
			auto it = find_line(file_lines, "TRIGGER DELAY:");
			if (it != file_lines.end() &&
				(sscanf(it->c_str(), "TRIGGER DELAY: %lf", &trigger_delay) == 1) &&
				fabs(trigger_delay) < 1000.) {
				std::cout << "Trigger delay = " << trigger_delay << " us" << std::endl;
			}
			else {
				std::cerr << "ERROR: Trigger delay >1000 us !!!" << std::endl;
				if (_debug) {
					std::cout << "GeometryTPC::Load - Abort (5)" << std::endl;
				}
				return false;
			}
		}
		{
			// set drift cage acceptance limits along Z-axis [mm]
			auto it = find_line(file_lines, "DRIFT CAGE ACCEPTANCE:");
			if (it != file_lines.end() &&
				(sscanf(it->c_str(), "DRIFT CAGE ACCEPTANCE: %lf %lf", &drift_zmin, &drift_zmax) == 2) &&
				fabs(drift_zmin) < 200. && fabs(drift_zmax) < 200. && drift_zmin < drift_zmax) {// [mm]
				std::cout << "Drift cage Z-axis acceptance = [" << drift_zmin << " mm, " << drift_zmax << " mm]" << std::endl;
			}
			else {
				std::cerr << "ERROR: Drift cage acceptance limits mismatched or beyond 200 mm !!!" << std::endl;
				if (_debug) {
					std::cout << "GeometryTPC::Load - Abort (6)" << std::endl;
				}
				return false;
			}
		}
		{
			// set unit vectors (along strips) and strip pitch vectors (perpendicular to strips)
			std::for_each(proj_vec_UVW.begin(), proj_vec_UVW.end(), [&](auto proj) {
				strip_unit_vec[proj].Set(std::cos(angle[proj] * deg_to_rad), std::sin(angle[proj] * deg_to_rad));  });

			pitch_unit_vec[projection::DIR_U] = -1.0 * (strip_unit_vec[projection::DIR_W] + strip_unit_vec[projection::DIR_V]).Unit();
			pitch_unit_vec[projection::DIR_V] = (strip_unit_vec[projection::DIR_U] + strip_unit_vec[projection::DIR_W]).Unit();
			pitch_unit_vec[projection::DIR_W] = (strip_unit_vec[projection::DIR_V] - strip_unit_vec[projection::DIR_U]).Unit();

			bool found = false;
			for (const auto& line : file_lines) {
				std::map<std::string, projection>::iterator it;
				char name[4];
				int cobo = 0, asad = 0, aget, strip_num, chan_num;
				double offset_in_pads, offset_in_strips, length_in_pads;

				if ((sscanf(line.c_str(), "%1s %d %d %d %d %d %lf %lf %lf",   // NEW FORMAT (several ASADs)
					name,
					&strip_num, &cobo, &asad, &aget, &chan_num,
					&offset_in_pads, &offset_in_strips, &length_in_pads) == 9 ||
					(sscanf(line.c_str(), "%1s %d %d %d %lf %lf %lf",         // LEGACY FORMAT (1 ASAD only)
						name,
						&strip_num, &aget, &chan_num,
						&offset_in_pads, &offset_in_strips, &length_in_pads) == 7 && (cobo = 0) == 0 && (asad = 0) == 0)) &&
					strip_num >= 1 &&
					cobo >= 0 &&
					asad >= 0 &&
					aget >= 0 && aget <= AGET_Nchips &&
					chan_num >= 0 && chan_num < AGET_Nchan &&
					length_in_pads >= 1.0 &&
					std::string(name) != "FPN" &&                     // veto any FPN entries and
					(it = name2dir.find(std::string(name))) != name2dir.end()) {    // accept only U,V,W entries

					if (!found) {
						found = true;
						std::cout << "DIR     STRIP   COBO    ASAD    AGET    AGET_CH OFF_PAD OFF_STR LENGTH\n";
					}
					std::cout << Form("%-8s%-8d%-8d%-8d%-8d%-8d%-8.1lf%-8.1lf%-8.1lf", name, strip_num, cobo, asad, aget, chan_num, offset_in_pads, offset_in_strips, length_in_pads) << std::endl;

					auto dir = it->second; // strip direction index

					// DEBUG
					if (_debug) {
						std::cout << "DIRNAME=" << std::string(name) << " / DIR=" << dir << " / STRIPNUM=" << strip_num
							<< " / COBO=" << cobo << " / ASAD=" << asad
							<< " / AGET=" << aget << " / CHANNUM=" << chan_num << "\n";
					}
					// DEBUG
					if (arrayByAget[cobo][asad][aget][chan_num] == nullptr) {

						// create new strip
						int chan_num_raw = Aget_normal2raw(chan_num);
						TVector2 offset = offset_in_strips * strip_pitch * pitch_unit_vec[dir] + offset_in_pads * pad_pitch * strip_unit_vec[dir];
						double length = length_in_pads * pad_pitch;
						std::shared_ptr<StripTPC> strip = std::make_shared<StripTPC>(dir, strip_num, cobo, asad, aget, chan_num, chan_num_raw,
							strip_unit_vec[dir], offset, length);

						// update map (by: COBO board, ASAD board, AGET chip, AGET normal/raw channel)
						arrayByAget[cobo][asad][aget][chan_num] = strip; // StripTPC(dir, strip_num);

						// update reverse map (by: strip direction, strip number)
						stripArray[dir][strip_num] = strip;

						// update maximal ASAD index (by: COBO board) 
						if (ASAD_N[cobo] == 0) { //CHECK IF CORRECT
							ASAD_N[cobo] = asad + 1;  // ASAD indexing starts from 0
							if (cobo >= COBO_N) COBO_N = cobo + 1; // COBO indexing starts from 0
						}
						else {
							if (asad >= ASAD_N[cobo]) ASAD_N[cobo] = asad + 1; // ASAD indexing starts from 0
						}

						// update number of strips in each direction
						if (stripN.find(dir) == stripN.end()) stripN[dir] = 1;
						else stripN[dir]++;

						// DEBUG
						if (_debug) {
							auto op = (*strip)();
							std::cout << ">>> ADDED NEW STRIP:"
								<< "KEY=[COBO=" << cobo << ", ASAD=" << asad << ", AGET=" << aget << ", CHAN=" << chan_num
								<< "]  VAL=[DIR=" << dir << ", STRIP=" << strip_num << "], "
								<< "NSTRIPS[DIR=" << dir << "]=" << stripN[dir] << ", "
								<< "   map_by_AGET=(" << op.dir << ","
								<< op.num << "), "
								<< "   map_by_STRIP=(" << op.coboId << ","
								<< op.asadId << ","
								<< op.agetId << ","
								<< op.agetCh << ")"
								<< "\n";
						}
						// DEBUG

					}
					else {
						std::cout << "WARNING: Ignored duplicated keyword: COBO=" << cobo << ", ASAD=" << asad
							<< ", AGET=" << aget << ", CHANNEL=" << chan_num << " !!!\n";
					}

				}
			}
		}
	}
	else {
		std::cout << "\n==== INITIALIZING TPC GEOMETRY - END ====\n\n";
		std::cerr << "ERROR: Unable to open config file: " << fname << "!!!\n";
		if (_debug) {
			std::cout << "GeometryTPC::Load - Abort (7)" << std::endl;
		}
		return false;
	}

	// sanity checks
	for (int icobo = 0; icobo < COBO_N; icobo++) {
		if (ASAD_N[icobo] == 0) { //CHECK IF CORRECT
			std::cerr << "ERROR: Number of ASAD boards for COBO " << icobo << " is not defined !!!" << std::endl;
			if (_debug) {
				std::cout << "GeometryTPC::Load - Abort (8)" << std::endl;
			}
			return false;
		}
	}

	// adding # of FPN channels to stripN
	stripN[FPN_CH] = FPN_chanId.size();

	// setting initOK=true at this stage is needed for TH2PolyInit and certain getter functions 
	initOK = true; //FIX ME

	// print statistics
	std::cout << std::endl
		<< "Geometry config file = " << fname << std::endl;
	for (auto& proj : proj_vec_UVW) {
		std::cout << "Total number of " << this->GetDirName(proj) << " strips = " << this->GetDirNstrips(proj) << std::endl;
	}
	std::cout << "Number of active channels per AGET chip = " << this->GetAgetNchannels() << std::endl;
	std::cout << "Number of " << this->GetDirName(FPN_CH) << " channels per AGET chip = " << this->GetDirNstrips(FPN_CH) << std::endl;
	std::cout << "Number of raw channels per AGET chip = " << this->GetAgetNchannels_raw() << std::endl;
	std::cout << "Number of AGET chips per ASAD board = " << this->GetAgetNchips() << std::endl;
	std::cout << "Number of ASAD boards = " << this->GetAsadNboards() << std::endl;
	std::cout << "Number of COBO boards = " << this->GetCoboNboards() << std::endl;

	// now initialize TH2Poly (while initOK=true) and set initOK flag according to the result 

	std::cout << "\n==== INITIALIZING TPC GEOMETRY - END ====\n\n";

	if (!InitTH2Poly()) {
		std::cerr << "ERROR: Cannot initialize TH2Poly !!!" << std::endl;
		if (_debug) {
			std::cout << "GeometryTPC::Load - Abort (9)" << std::endl;
		}
		return false;
	}

	if (_debug) {
		std::cout << "GeometryTPC::Load - Ended..." << std::endl;
	}
	return true;
}

bool GeometryTPC::InitTH2Poly() {

  if(_debug) {
    std::cout << "GeometryTPC::InitTH2Poly - Started..." << std::endl;
  }

  fStripMap.clear();

  // sanity checks
  if( grid_nx<1 || grid_ny<1) {
    if(_debug) {
      std::cout << "GeometryTPC::InitTH2Poly - Abort (1)" << std::endl;
    }
    return false;
  }

  // find X and Y range according to geo_ptr
  double xmin = 1E30;
  double xmax = -1E30;
  double ymin = 1E30;
  double ymax = -1E30; 
  for (auto& by_dir : stripArray) {
	  for (auto strip : by_dir.second) {
		  if (strip.second == nullptr) continue;
		  auto op = (*strip.second)();
		  TVector2 point1 = reference_point + op.offset_vec
			  - op.unit_vec * 0.5 * pad_pitch;
		  TVector2 point2 = point1 + op.unit_vec * op.length;
		  xmin = std::min({ xmin, point1.X(), point2.X() });
		  xmax = std::max({ xmax, point1.X(), point2.X() });
		  ymin = std::min({ ymin, point1.Y(), point2.Y() });
		  ymax = std::max({ ymax, point1.Y(), point2.Y() });
	  }
  }
  tp = std::make_shared<TH2Poly>("h_uvw","GeometryTPC::TH2Poly;X;Y;Charge", grid_nx, xmin, xmax, grid_ny, ymin, ymax);
  if(tp == nullptr) {
    if(_debug) {
      std::cout << "GeometryTPC::InitTH2Poly - Abort (2)" << std::endl;
    }
    return false;
  }
  tp->SetFloat(true); // allow to expand outside of the current limits

  ///////// DEBUG
  //  int strip_counter = 0;
  ///////// DEBUG

  // create TPolyBin corresponding to each StripTPC  
  for (auto& dir_strips : stripArray) {
	  ///////// DEBUG
	  if (_debug) {
		  std::cout << "GeometryTPC::InitTH2Poly: nstrips[" << dir_strips.first << "]=" << this->GetDirNstrips(dir_strips.first) << std::endl;
	  }
	  ///////// DEBUG

	  for (auto strip : dir_strips.second) {

		  if (strip.second == nullptr) continue;
		  auto op = (*strip.second)();

		  // create diamond-shaped pad for a given direction
		  std::vector<TVector2> offset_vec;
		  offset_vec.push_back(TVector2(0., 0.));
		  offset_vec.push_back(op.unit_vec.Rotate(pi / 6.) * pad_size);
		  offset_vec.push_back(op.unit_vec * pad_pitch);
		  offset_vec.push_back(op.unit_vec.Rotate(-pi / 6.) * pad_size);
		  offset_vec.push_back(TVector2(0., 0.));

		  TVector2 point0 = reference_point + op.offset_vec - op.unit_vec * 0.5 * pad_pitch;

		  const int npads = (int)(op.length / pad_pitch);
		  const int npoints = npads * offset_vec.size();
		  TGraph* g = new TGraph(npoints);
		  int ipoint = 0;
		  for (int ipad = 0; ipad < npads; ipad++) {
			  for (auto& off_vec_elem : offset_vec) {
				  TVector2 corner = point0 + op.unit_vec * ipad * pad_pitch + off_vec_elem;
				  g->SetPoint(ipoint, corner.X(), corner.Y());
				  ipoint++;
			  }
		  }

		  // create new TH2PolyBin

		  // DEBUG
		  if (_debug) {
			  std::cout << "TH2POLY ADDBIN: BEFORE CREATING BIN: GetNumberOfBins=" << tp->GetNumberOfBins() << std::endl;
		  }
		  // DEBUG      

		  // primary method:
		  //      const int ibin = tp->AddBin(g);

		  // alternative method due to bin endexing bug in TH2Poly::AddBin() implementation:
		  const int nbins_old = tp->GetNumberOfBins();
		  int ibin = tp->AddBin(g);
		  const int nbins_new = tp->GetNumberOfBins();
		  if (nbins_new > nbins_old) {
			  TH2PolyBin* bin = (TH2PolyBin*)tp->GetBins()->At(tp->GetNumberOfBins() - 1);
			  if (bin) ibin = bin->GetBinNumber();
		  }

		  // DEBUG
		  if (_debug) {
			  std::cout << "TH2POLY ADDBIN: AFTER CREATING BIN: GetNumberOfBins=" << tp->GetNumberOfBins() << std::endl;
			  std::cout << "TH2POLY ADDBIN: "
				  << "TH2POLY_IBIN=" << ibin
				  //		  << ", TH2POLYBIN_PTR=" << bin 	          
				  << ", DIR=" << this->GetDirName(dir_strips.first) << ", NUM=" << op.num
				  << ", NPADS=" << npads
				  << ", NPOINTS=" << npoints << " (" << ipoint << ")"
				  << std::endl;
		  }
		  // DEBUG      

		  ///////// DEBUG
		  // strip_counter++;
		  ///////// DEBUG

		  // DEBUG

		  if (ibin < 1) { // failed to create new bin
			  std::cerr << "ERROR: Failed to create new TH2Poly bin: TH2POLY->AddBin()=" << ibin
				  << ", DIR=" << this->GetDirName(dir_strips.first) << ", NUM=" << op.num
				  << ", NPADS=" << npads
				  << ", NPOINTS=" << npoints << ", TGraph->Print():"
				  << std::endl;
			  g->Print();
			  std::cerr << std::endl << std::flush;
			  return false;
			  //	continue; 
		  }

		  // update strip map
		  SetTH2PolyStrip(ibin, strip.second);

		  // DEBUG
		  if (_debug) {
			  std::cout << "TH2POLY ADDBIN: DIR=" << this->GetDirName(dir_strips.first) << ", NUM=" << op.num
				  << ", TH2POLY_IBIN=" << ibin
				  << ", NPADS=" << npads
				  << ", NPOINTS=" << npoints << " (" << ipoint << ")"
				  << std::endl;
		  }
		  // DEBUG

	  }
  }

  if(_debug) {
    std::cout << "GeometryTPC::InitTH2Poly - Ended..." << std::endl;
  }

  return (fStripMap.size() > 0);
}

void GeometryTPC::SetTH2PolyStrip(int ibin, std::shared_ptr<StripTPC> s) {
  if(s != nullptr) fStripMap[ibin]=s;
}

int GeometryTPC::GetDirNstrips(projection dir) {
  if (IsDIR_UVW(dir) || dir == FPN_CH) {
	  return stripN[dir];
  };
  return ERROR;
}

std::string GeometryTPC::GetDirName(projection dir) {
  if ((IsDIR_UVW(dir) || dir == FPN_CH)) {
	  return dir2name.at(dir); // dir2name[dir];
  };
  return std::string(); // "ERROR";
}

std::shared_ptr<StripTPC> GeometryTPC::GetStripByAget(int COBO_idx, int ASAD_idx, int AGET_idx, int channel_idx) { // valid range [0-1][0-3][0-3][0-63]
  return arrayByAget[COBO_idx][ASAD_idx][AGET_idx][channel_idx]; // ERROR
}

std::shared_ptr<StripTPC> GeometryTPC::GetStripByDir(projection dir, int num) { // valid range [0-2][1-1024]
    if (num < stripArray[dir].size()) {
        return stripArray[dir][num];
    }
    return std::shared_ptr<StripTPC>();
}

int GeometryTPC::Aget_normal2raw(int channel_idx) { // valid range [0-63]
  if(channel_idx<0 || channel_idx>=AGET_Nchan) return ERROR;
  int raw_channel_idx = channel_idx;
  for(auto& elem : FPN_chanId) {
    if(elem <= raw_channel_idx) ++raw_channel_idx;
  }
  return raw_channel_idx;
}

int GeometryTPC::Aget_fpn2raw(int FPN_idx) { // valid range [0-3]
  if(FPN_idx<0 || FPN_idx>=stripN[FPN_CH]) return ERROR;
  return FPN_chanId.at(FPN_idx);
}
                         
int GeometryTPC::Global_normal2normal(int COBO_idx, int ASAD_idx, int aget_idx, int channel_idx) {   // valid range [0-1][0-3][0-3][0-63]
  if(COBO_idx<0 || COBO_idx>=COBO_N || ASAD_idx<0 || ASAD_idx>ASAD_N[COBO_idx] || aget_idx<0 || aget_idx>=AGET_Nchips ||
     channel_idx<0 || channel_idx>=AGET_Nchan ) return ERROR;
  int ASAD_offset = std::accumulate(ASAD_N.begin(), ASAD_N.end(), 0);
  return ((ASAD_offset+ASAD_idx)*AGET_Nchips + aget_idx)*AGET_Nchan + channel_idx; 
}

// Checks if 2 strips are crossing inside the active area of TPC,
// on success (true) also returns the calculated offset vector wrt ORIGIN POINT (0,0)
bool GeometryTPC::GetCrossPoint(std::shared_ptr<StripTPC> strip1, std::shared_ptr<StripTPC> strip2, TVector2 &point) {
  if(strip1 == nullptr || strip2 == nullptr) return false;
  auto
      op1 = (*strip1)(),
      op2 = (*strip2)();
  const double u1[2] = { op1.unit_vec.X(), op1.unit_vec.Y() };
  const double u2[2] = { op2.unit_vec.X(), op2.unit_vec.Y() };
  double W = -u1[0]*u2[1] + u1[1]*u2[0];
  if(fabs(W)<NUM_TOLERANCE) return false;
  const double offset[2] = { op2.offset_vec.X() - op1.offset_vec.X(),
                 op2.offset_vec.Y() - op1.offset_vec.Y() };
  double W1 = -offset[0]*u2[1] + offset[1]*u2[0]; 
  double W2 = u1[0]*offset[1] - u1[1]*offset[0];
  double len1 = W1/W;
  double len2 = W2/W;
  double residual = 0.5*pad_pitch;
  if(len1<-residual-NUM_TOLERANCE || len2<-residual-NUM_TOLERANCE || 
     len1 > op1.length + NUM_TOLERANCE || len2 > op2.length + NUM_TOLERANCE) return false;
  point.Set(op1.offset_vec.X() + len1* op1.unit_vec.X(), op1.offset_vec.Y() + len1*op1.unit_vec.Y() );
  point = point + reference_point;
  return true;
}

// Checks if 3 strips are crossing inside the active area of TPC within a given tolerance (radius in [mm]),
// on success (true) also returns the average calculated offset vector wrt ORIGIN POINT (0,0)
bool GeometryTPC::MatchCrossPoint(std::shared_ptr<StripTPC> strip1, std::shared_ptr<StripTPC> strip2, std::shared_ptr<StripTPC> strip3, double radius, TVector2 &point) {
  TVector2 points[3];
  const double rad2= pow(radius,2);
  if( !this->GetCrossPoint(strip1, strip2, points[0]) ||
      !this->GetCrossPoint(strip2, strip3, points[1]) ||
      !this->GetCrossPoint(strip3, strip1, points[2])   ) return false;
  if( (points[0]-points[1]).Mod2()>rad2 ||
      (points[1]-points[2]).Mod2()>rad2 ||
      (points[2]-points[0]).Mod2()>rad2   ) return false;
  point=(points[0]+points[1]+points[2])/3.0; // return average position wrt ORIGIN POINT (0,0)
  return true;
}

TVector2 GeometryTPC::GetStripPitchVector(projection dir) { // XY ([mm],[mm])
  if (IsDIR_UVW(dir)) {
	  return pitch_unit_vec.at(dir);
  };
  return TVector2(0, 0); // ERROR  
}

// Calculates (signed) distance in [mm] of projection of point (X=0,Y=0) from
// projection of the central axis of the existing strip (DIR, NUM) on
// the strip pitch axis for a given DIR family.
// The "err_flag" is set to TRUE if:
// - geometry has not been initialized properly, or
// - input strip DIR and/or NUM are invalid
double GeometryTPC::Strip2posUVW(projection dir, int num) {
  return this->Strip2posUVW( this->GetStripByDir(dir, num));
}

// Calculates (signed) distance in [mm] of projection of point (X=0,Y=0) from
// projection of the central axis of the existing strip (StripTPC object) on
// the strip pitch axis for a given strip family.
// The "err_flag" is set to TRUE if:
// - geometry has not been initialized properly, or
// - input StripTPC object is invalid
double GeometryTPC::Strip2posUVW(std::shared_ptr<StripTPC> strip) {
  if (strip != nullptr) {
	  auto op = (*strip)();
	  return (reference_point + op.offset_vec) * pitch_unit_vec[op.dir]; // [mm]
  }
  return 0.0; // ERROR
}

// Calculates position in [mm] along Z-axis corresponding to
// the given (fractional) time cell number [0-511]
// The "err_flag" is set to TRUE if:
// - geometry has not been initialized properly, or
// - input time-cell is outside of the valid range [0-511]
double GeometryTPC::Timecell2pos(double position_in_cells, bool &err_flag) {
  err_flag=true;
  if(position_in_cells>=0.0 && position_in_cells<=AGET_Ntimecells) err_flag=false; // check range
  return (position_in_cells-AGET_Ntimecells)/sampling_rate*vdrift*10.0 + trigger_delay*vdrift*10.0; // [mm]
}

std::tuple<double, double, double, double> GeometryTPC::rangeXY(){

	auto strip_arr = GetStrips();

  double xmin=1E30;
  double xmax=-1E30;
  double ymin=1E30;
  double ymax=-1E30;

  for(auto& strip : strip_arr) {
      auto op = (*strip)();
    if(!strip) continue;
    double x, y;
    TVector2 vec= op.offset_vec + GetReferencePoint();
    x=vec.X();
    y=vec.Y();
    if(x>xmax) xmax=x;
    if(x<xmin) xmin=x;
    if(y>ymax) ymax=y;
    if(y<ymin) ymin=y;
    vec = vec + op.unit_vec * op.length;
    if(x>xmax) xmax=x;
    if(x<xmin) xmin=x;
    if(y>ymax) ymax=y;
    if(y<ymin) ymin=y;
  }
  xmin-=GetStripPitch()*0.3;
  xmax+=GetStripPitch()*0.7;
  ymin-=GetPadPitch()*0.3;
  ymax+=GetPadPitch()*0.7;

  return std::tuple<double, double, double, double>(xmin, xmax, ymin, ymax);
}

// Returns vector of first and last strips in each dir
std::vector<std::shared_ptr<StripTPC>> GeometryTPC::GetStrips() {
	std::vector<std::shared_ptr<StripTPC>> _strips_;
	for (auto& _dir_ : proj_vec_UVW) {
		_strips_.push_back(GetStripByDir(_dir_, 1)); //first strip
		_strips_.push_back(GetStripByDir(_dir_, GetDirNstrips(_dir_))); //last strip
	}
	return _strips_;
}
  
//ClassImp(StripTPC)
//ClassImp(GeometryTPC)
