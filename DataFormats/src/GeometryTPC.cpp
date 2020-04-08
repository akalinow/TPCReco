#include "GeometryTPC.h" 

GeometryTPC& Geometry(std::string fname) {
	static std::once_flag geometry_init_check;
	std::call_once(geometry_init_check, [&]() {
		if (fname == "") {
			throw std::runtime_error("Geometry wasn't initialized before first use");
		}
	});
	static GeometryTPC main_geometry{ fname };
	return main_geometry;
}

/*template <size_t element_index, typename... Types>
std::ostream& tuple_write(std::ostream& output_stream, std::tuple<Types...>& data_tuple) {
	if constexpr (element_index < sizeof...(Types)) {
		output_stream << std::get<element_index>(data_tuple) << "	";
		return tuple_write<element_index + 1>(output_stream, data_tuple);
	}
	else {
		return output_stream;
	}
}

template <typename... Types>
std::ostream& operator<<(std::ostream& output_stream, std::tuple<Types...>& data_tuple) {
	return tuple_write<0>(output_stream, data_tuple);
}

template <size_t element_index, typename... Types>
std::istream& tuple_read(std::istream& input_stream, std::tuple<Types...>& data_tuple) {
	if constexpr (element_index < sizeof...(Types)) {
		input_stream >> std::get<element_index>(data_tuple);
		return tuple_read<element_index + 1>(input_stream, data_tuple);
	}
	else {
		return input_stream;
	}
}

template <typename... Types>
std::istream& operator>>(std::istream& input_stream, std::tuple<Types...>& data_tuple) {
	return tuple_read<0>(input_stream, data_tuple);
}

template <typename... Types>
bool load_var(std::vector<std::string> file_lines, std::string line_name, std::function<bool(Types...)> var_check, std::string units, int error_number, Types&... var_ref) {
	auto it = find_line(file_lines, line_name);
	if (it != file_lines.end() && //line exists
		(std::stringstream(it->substr(it->find(':') + 1)) >> std::tie(var_ref...)) && //line constains values
		var_check(var_ref...)) { //values are correct
		std::cout << line_name << " = " << std::tie(var_ref...) << " " << units << std::endl;
		return true;
	}
	else {
		std::cerr << "ERROR: Wrong " << line_name << " !!!" << std::endl;
		if (is_debug) {
			std::cout << "GeometryTPC::Load - Abort (" << error_number << ")" << std::endl;
		}
		return false;
	}
}*/

//version above not working, workarounds below

template <typename Type>
bool load_var(std::vector<std::string> file_lines, std::string line_name, std::function<bool(Type)> var_check, std::string units, int error_number, Type& var_ref) {
	auto it = find_line(file_lines, line_name);
	if (it != file_lines.end() && //line exists
		(std::stringstream(it->substr(it->find(':') + 1)) >> var_ref) && //line constains values
		var_check(var_ref)) { //values are correct
		std::cout << line_name << " = " << var_ref << " " << units << std::endl;
		return true;
	}
	else {
		std::cerr << "ERROR: Wrong " << line_name << " !!!" << std::endl;
		if (is_debug) {
			std::cout << "GeometryTPC::Load - Abort (" << error_number << ")" << std::endl;
		}
		return false;
	}
}

template <typename Type1, typename Type2>
bool load_var(std::vector<std::string> file_lines, std::string line_name, std::function<bool(Type1, Type2)> var_check, std::string units, int error_number, Type1& var_ref1, Type2& var_ref2) {
	auto it = find_line(file_lines, line_name);
	if (it != file_lines.end() && //line exists
		(std::stringstream(it->substr(it->find(':') + 1)) >> var_ref1 >> var_ref2) && //line constains values
		var_check(var_ref1, var_ref2)) { //values are correct
		std::cout << line_name << " = " << var_ref1 << " " << var_ref2 << " " << units << std::endl;
		return true;
	}
	else {
		std::cerr << "ERROR: Wrong " << line_name << " !!!" << std::endl;
		if (is_debug) {
			std::cout << "GeometryTPC::Load - Abort (" << error_number << ")" << std::endl;
		}
		return false;
	}
}

GeometryTPC::GeometryTPC(std::string fname)
	: COBO_N(0),
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
	reference_point(0., 0.) // default REFERENCE POINT position on XY plane
{
	if (is_debug) {
		std::cout << "GeometryTPC::Constructor - Started..." << std::endl;
	}

	tp = std::make_shared<TH2Poly>();
	tp->SetName("h_uvw_dummy");

	// FPN channels in each AGET chip
	AGET_Nchan_fpn = FPN_chanId.size();
	AGET_Nchan_raw = AGET_Nchan + AGET_Nchan_fpn;

	// Set indices and names of strip coordinates
	dir2name.insert({ direction::U, "U" });
	dir2name.insert({ direction::V, "V" });
	dir2name.insert({ direction::W, "W" });
	dir2name.insert({ direction::FPN_CH, "FPN" });

	for (auto&& it : dir2name) {
		name2dir.insert({ it.second, it.first });
	}

	// Load config file    
	if (!Load(fname)) {
		checkpoint;
		throw std::runtime_error("Load error occured or geometry data is incorrect!");
	}

	if (is_debug) {
		std::cout << "GeometryTPC::Constructor - Ended..." << std::endl;
	}
}

bool GeometryTPC::Load(std::string fname) {
	if (is_debug) {
		std::cout << "GeometryTPC::Load - Started..." << std::endl;
	}

	std::cout << "\n==== INITIALIZING TPC GEOMETRY - START ====\n\n";

	std::ifstream f(fname);
	std::map<direction, double> angle;
	std::vector<std::string> file_lines;

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
			auto it = find_line(file_lines, "ANGLES");
			if (it != file_lines.end() &&
				((std::stringstream(it->substr(it->find(':') + 1)) >> angle[direction::U] >> angle[direction::V] >> angle[direction::W]))) {
				for (auto dir : dirs) {
					angle[dir] = fmod(fmod(angle[dir], 360.) + 360., 360.); // convert to range [0, 360 deg[ range 
				}
				std::cout << Form("Angle of U/V/W strips wrt X axis = %lf / %lf / %lf deg", angle[direction::U], angle[direction::V], angle[direction::W]) << std::endl;
				if (fmod(angle[direction::U], 180.) == fmod(angle[direction::V], 180.) ||  // reject parallel / anti-parallel duplicates 
					fmod(angle[direction::V], 180.) == fmod(angle[direction::W], 180.) ||  // reject parallel / anti-parallel duplicates 
					fmod(angle[direction::W], 180.) == fmod(angle[direction::U], 180.)) { // reject parallel / anti-parallel duplicates 
					std::cerr << "ERROR: Wrong U/V/W angles !!!" << std::endl;
					return false;
				}
			}
			else {
				std::cerr << "ERROR: U/V/W angles not provided !!!" << std::endl;
				return false;
			}
		}
		{
			double xoff = 0.0, yoff = 0.0;
			if (!load_var<double, double>(file_lines, "REFERENCE POINT", { [](double _xoff, double _yoff) {return (fabs(_xoff) < 500. && fabs(_yoff) < 500.); } }, "mm", 2, xoff, yoff) || // set REFERECE POINT offset [mm]
				!load_var<double>(file_lines, "DIAMOND SIZE", { [](double var) {return var > 0.0; } }, "mm", 2, pad_size) || // set pad size
				!load_var<double>(file_lines, "DRIFT VELOCITY", { [](double var) {return var > 0.0; } }, "cm/us", 3, vdrift) || // set electron drift velocity [cm/us]
				!load_var<double>(file_lines, "SAMPLING RATE", { [](double var) {return var > 0.0; } }, "MHz", 4, sampling_rate) || // set electronics sampling rate [MHz]
				!load_var<double>(file_lines, "TRIGGER DELAY", { [](double var) {return std::fabs(var) < 1000.0; } }, "us", 5, trigger_delay) || // set electronics trigger delay [us]
				!load_var<double, double>(file_lines, "DRIFT CAGE ACCEPTANCE", { [](double _drift_zmin, double _drift_zmax) {return (fabs(_drift_zmin) < 200. && fabs(_drift_zmax) < 200. && _drift_zmin < _drift_zmax); } }, "mm", 6, drift_zmin, drift_zmax)) { // set drift cage acceptance limits along Z-axis [mm]
				return false;
			}
			reference_point.Set(xoff, yoff);
			pad_pitch = pad_size * std::sqrt(3.);
			strip_pitch = pad_size * 1.5;
		}
		{
			// set unit vectors (along strips) and strip pitch vectors (perpendicular to strips)
			std::for_each(dirs.begin(), dirs.end(), [&](auto proj) {strip_unit_vec[proj].Set(std::cos(angle[proj] * deg_to_rad), std::sin(angle[proj] * deg_to_rad));  });

			pitch_unit_vec[direction::U] = -1.0 * (strip_unit_vec[direction::W] + strip_unit_vec[direction::V]).Unit();
			pitch_unit_vec[direction::V] = (strip_unit_vec[direction::U] + strip_unit_vec[direction::W]).Unit();
			pitch_unit_vec[direction::W] = (strip_unit_vec[direction::V] - strip_unit_vec[direction::U]).Unit();

			std::cout << "DIRNAME	DIR     STRIP   COBO    ASAD    AGET    AGET_CH OFF_PAD OFF_STR LENGTH\n";
			for (const auto& line : file_lines) {
				std::map<std::string, direction>::iterator it;
				std::string name;
				int section, cobo = 0, asad = 0, aget, strip_num, chan_num, length_in_pads;
				double offset_in_pads, offset_in_strips;
				if (((std::stringstream(line) >> name >> section >> strip_num >> cobo >> asad >> aget >> chan_num >> offset_in_pads >> offset_in_strips >> length_in_pads) || // NEW FORMAT (several ASADs)
					(std::stringstream(line) >> name >> strip_num >> aget >> chan_num >> offset_in_pads >> offset_in_strips >> length_in_pads) && (section = 0) == 0 && (cobo = 0) == 0 && (asad = 0) == 0) && // LEGACY FORMAT (1 ASAD only)
					strip_num >= 1 &&
					cobo >= 0 &&
					asad >= 0 &&
					aget >= 0 && aget <= AGET_Nchips &&
					chan_num >= 0 && chan_num < AGET_Nchan &&
					length_in_pads >= 1.0 &&
					name != "FPN" &&                     // veto any FPN entries and
					(it = name2dir.find(name)) != name2dir.end()) {    // accept only U,V,W entries

					auto dir = it->second; // strip direction index

					std::cout << name << "	" << dir << "	" << strip_num << "	" << cobo << "	" << asad << "	" << aget
						<< "	" << chan_num << "	" << offset_in_pads << "	" << offset_in_strips << "	" << length_in_pads << std::endl;

					if (arrayByAget[{cobo, asad, aget, chan_num}] == nullptr) {

						// create new strip
						int chan_num_raw = Aget_normal2raw(chan_num);
						TVector2 offset = offset_in_strips * strip_pitch * pitch_unit_vec[dir] + offset_in_pads * pad_pitch * strip_unit_vec[dir];
						double length = length_in_pads * pad_pitch;
						auto strip = std::make_shared<Geometry_Strip>(dir, section, strip_num, cobo, asad, aget, chan_num, chan_num_raw, strip_unit_vec[dir], offset, length);

						// update map (by: COBO board, ASAD board, AGET chip, AGET normal/raw channel)
						arrayByAget[{cobo, asad, aget, chan_num}] = strip;

						// update reverse map (by: strip direction, strip number)
						stripArray[{dir, section, strip_num}] = strip;

						// update maximal ASAD index (by: COBO board) 
						if (cobo >= ASAD_N.size()) { //resize ASAD_N if necessary
							ASAD_N.resize(static_cast<size_t>(cobo) + 1);
						}
						ASAD_N[cobo] = std::max(ASAD_N[cobo], asad + 1); // ASAD indexing starts from 0

						// update number of strips in each direction
						stripN[dir]++;

						// DEBUG
						if (is_debug) {
							std::cout << ">>> ADDED NEW STRIP:" << " NSTRIPS[DIR=" << dir << "]=" << stripN[dir] << "\n";
						}
						// DEBUG

					}
					else {
						std::cout << "WARNING: Ignored duplicated keyword: COBO=" << cobo << ", ASAD=" << asad
							<< ", AGET=" << aget << ", CHANNEL=" << chan_num << " !!!\n";
					}

				}
			}
			COBO_N = ASAD_N.size(); // set COBO_N, COBO indexing starts from 0
			ASAD_Nboards = std::accumulate(ASAD_N.begin(), ASAD_N.end(), 0.0);
		}
	}
	else {
		std::cout << "\n==== INITIALIZING TPC GEOMETRY - END ====\n\n";
		std::cerr << "ERROR: Unable to open config file: " << fname << "!!!\n";
		if (is_debug) {
			std::cout << "GeometryTPC::Load - Abort (7)" << std::endl;
		}
		return false;
	}

	// sanity checks
	auto it = std::find(ASAD_N.begin(), ASAD_N.end(), 0);
	if (it != ASAD_N.end()) {
		std::cerr << "ERROR: Number of ASAD boards for COBO " << std::distance(ASAD_N.begin(), it) << " is not defined !!!" << std::endl;
		if (is_debug) {
			std::cout << "GeometryTPC::Load - Abort (8)" << std::endl;
		}
		return false;
	}

	// adding # of FPN channels to stripN
	stripN[direction::FPN_CH] = FPN_chanId.size();

	// print statistics
	std::cout << std::endl
		<< "Geometry config file = " << fname << std::endl;
	for (auto& proj : dirs) {
		std::cout << "Total number of " << GetDirName(proj) << " strips = " << GetDirNstrips(proj) << std::endl;
	}
	std::cout << "Number of active channels per AGET chip = " << GetAgetNchannels() << std::endl;
	std::cout << "Number of " << GetDirName(direction::FPN_CH) << " channels per AGET chip = " << GetDirNstrips(direction::FPN_CH) << std::endl;
	std::cout << "Number of raw channels per AGET chip = " << GetAgetNchannels_raw() << std::endl;
	std::cout << "Number of AGET chips per ASAD board = " << GetAgetNchips() << std::endl;
	std::cout << "Number of ASAD boards = " << GetAsadNboards() << std::endl;
	std::cout << "Number of COBO boards = " << GetCoboNboards() << std::endl;

	std::cout << "\n==== INITIALIZING TPC GEOMETRY - END ====\n\n";

	if (!InitTH2Poly()) {
		std::cerr << "ERROR: Cannot initialize TH2Poly !!!" << std::endl;
		if (is_debug) {
			std::cout << "GeometryTPC::Load - Abort (9)" << std::flush << std::endl;
		}
		return false;
	}
	if (is_debug) {
		std::cout << "GeometryTPC::Load - Ended..." << std::endl;
	}
	return true;
}

bool GeometryTPC::InitTH2Poly() {

	if (is_debug) {
		std::cout << "GeometryTPC::InitTH2Poly - Started..." << std::flush << std::endl;
	}

	fStripMap.clear();

	// sanity checks
	if (grid_nx < 1 || grid_ny < 1) {
		if (is_debug) {
			std::cout << "GeometryTPC::InitTH2Poly - Abort (1)" << std::flush << std::endl;
		}
		return false;
	}

	// find X and Y range according to geo_ptr
	double xmin = std::numeric_limits<double>::infinity();
	double xmax = -std::numeric_limits<double>::infinity();
	double ymin = std::numeric_limits<double>::infinity();
	double ymax = -std::numeric_limits<double>::infinity();

	/*std::cout<<"!!!!!!!!!!!!"<<std::endl;
	for (auto &i : mapByAget){
	   std::cout<<i.second->CoboId()<<std::endl;
	   std::cout<<i.second->AsadId()<<std::endl;
	   std::cout<<i.second->AgetId()<<std::endl;
	   std::cout<<i.second->AgetCh()<<std::endl;
	  std::cout<<i.second->Dir()<<std::endl;
	  std::cout<<i.second->Num()<<std::endl;
	  std::cout<<i.second->GlobalCh()<<std::endl;
	  std::cout<<i.second->GlobalCh_raw()<<std::endl;

	   std::cout<<"!!!!!!!!!!!!"<<std::endl;
	  std::cout<<mapByStrip.find(MultiKey2(i.second->Dir(),i.second->Num()))->second->GlobalCh()<<std::endl;
	  //std::cout<<this->GetStripByGlobal(i.second->GlobalCh())->GlobalCh()<<std::endl;
	}




	std::cout<<"!!!!!!!!!!!!"<<std::endl;*/
	// for(int dir=0; dir<=2; dir++) {
	 //  for(int num=1; num<=this->GetDirNstrips(dir); num++)
	for (auto& strip_it : stripArray) {
		auto& strip_data = (*strip_it.second)();
		TVector2 point1 = reference_point + strip_data.offset_vec - strip_data.unit_vec * 0.5 * pad_pitch;
		TVector2 point2 = point1 + strip_data.unit_vec * strip_data.length();
		if (point1.X() < xmin)      xmin = point1.X();
		else if (point1.X() > xmax) xmax = point1.X();
		if (point2.X() < xmin)      xmin = point2.X();
		else if (point2.X() > xmax) xmax = point2.X();
		if (point1.Y() < ymin)      ymin = point1.Y();
		else if (point1.Y() > ymax) ymax = point1.Y();
		if (point2.Y() < ymin)      ymin = point2.Y();
		else if (point2.Y() > ymax) ymax = point2.Y();
		// }
	}
	tp = std::make_shared<TH2Poly>("h_uvw", "GeometryTPC::TH2Poly;X;Y;Charge", grid_nx, xmin, xmax, grid_ny, ymin, ymax);
	tp->SetFloat(true); // allow to expand outside of the current limits

	///////// DEBUG
	//  int strip_counter = 0;
	///////// DEBUG

	// create TPolyBin corresponding to each StripTPC  
	/*for(int dir=0; dir<=2; dir++) {

	  ///////// DEBUG
	  if(is_debug) {
		std::cout << "GeometryTPC::InitTH2Poly: nstrips[" << dir << "]=" << this->GetDirNstrips(dir) << std::endl;
	  }
	  ///////// DEBUG

	  for(int num=1; num<=this->GetDirNstrips(dir); num++)*/
	for (auto& strip_it : stripArray) {
		auto strip = strip_it.second;
		auto& strip_data = (*strip)();
		// if(!strip_data) continue;
		auto dir = strip_data.dir;
		int section = strip_data.section;
		int num = strip_data.num;
		// create diamond-shaped pad for a given direction
		std::vector<TVector2> offset_vec;
		offset_vec.push_back({ 0., 0. });
		offset_vec.push_back(strip_data.unit_vec.Rotate(TMath::Pi() / 6.) * pad_size);
		offset_vec.push_back(strip_data.unit_vec * pad_pitch);
		TVector2 point0 = reference_point + strip_data.offset_vec - strip_data.unit_vec * 0.5 * pad_pitch;
		int npads = strip_data.npads;
		//  const int npads =int( strip_data->Length()/pad_pitch );
		const int npoints = npads * offset_vec.size();
		auto g = std::make_unique<TGraph>(npoints);
		int ipoint = 0;
		for (int ipad = 0; ipad < npads; ipad++) {
			for (size_t icorner = 0; icorner < offset_vec.size(); icorner++) {
				TVector2 corner = point0 + strip_data.unit_vec * ipad * pad_pitch + offset_vec[icorner];
				g->SetPoint(ipoint, corner.X(), corner.Y());
				ipoint++;
			}
		}
		offset_vec.clear();
		offset_vec.push_back(strip_data.unit_vec.Rotate(-TMath::Pi() / 6.) * pad_size);
		offset_vec.push_back({ 0., 0. });
		for (int ipad = npads - 1; ipad >= 0; ipad--) {
			for (size_t icorner = 0; icorner < offset_vec.size(); icorner++) {
				TVector2 corner = point0 + strip_data.unit_vec * ipad * pad_pitch + offset_vec[icorner];
				g->SetPoint(ipoint, corner.X(), corner.Y());
				ipoint++;
			}
		}
		// create new TH2PolyBin

		// DEBUG
		if (is_debug) {
			std::cout << "TH2POLY ADDBIN: BEFORE CREATING BIN: GetNumberOfBins=" << tp->GetNumberOfBins() << std::endl;
		}
		// DEBUG      

		// primary method:
		//      const int ibin = tp->AddBin(g);

		// alternative method due to bin endexing bug in TH2Poly::AddBin() implementation:
		const int nbins_old = tp->GetNumberOfBins();
		int ibin = tp->AddBin(&*g);
		const int nbins_new = tp->GetNumberOfBins();
		if (nbins_new > nbins_old) {
			TH2PolyBin* bin = (TH2PolyBin*)tp->GetBins()->At(tp->GetNumberOfBins() - 1);
			if (bin != nullptr) ibin = bin->GetBinNumber();
		}

		// DEBUG
		if (is_debug) {
			std::cout << "TH2POLY ADDBIN: AFTER CREATING BIN: GetNumberOfBins=" << tp->GetNumberOfBins() << std::endl;
			std::cout << "TH2POLY ADDBIN: "
				<< "TH2POLY_IBIN=" << ibin
				//		  << ", TH2POLYBIN_PTR=" << bin 	          
				<< ", DIR=" << this->GetDirName(dir) << ", SECTION=" << section << ", NUM=" << num
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
				<< ", DIR=" << this->GetDirName(dir) << ", SECTION=" << section << ", NUM=" << num
				<< ", NPADS=" << npads
				<< ", NPOINTS=" << npoints << ", TGraph->Print():"
				<< std::endl;
			g->Print();
			std::cerr << std::endl << std::flush;
			return false;
			//	continue; 
		}

		// update strip map
		SetTH2PolyStrip(ibin, strip);

		// DEBUG
		if (is_debug) {
			std::cout << "TH2POLY ADDBIN: DIR=" << GetDirName(dir) << ", SECTION=" << section << ", NUM=" << num
				<< ", TH2POLY_IBIN=" << ibin
				<< ", NPADS=" << npads
				<< ", NPOINTS=" << npoints << " (" << ipoint << ")"
				<< std::endl;
		}
		// DEBUG

	//  }
	}

	if (is_debug) {
		std::cout << "GeometryTPC::InitTH2Poly - Ended..." << std::flush << std::endl;
	}

	return (fStripMap.size() > 0);
}

// change cartesian binning of the underlying TH2Poly
void GeometryTPC::SetTH2PolyPartition(int nx, int ny) {
	bool change = false;
	if (nx > 1 && nx != grid_nx) { grid_nx = nx; change = true; }
	if (ny > 1 && ny != grid_ny) { grid_ny = ny; change = true; }
	if (change && tp) tp->ChangePartition(grid_nx, grid_ny);
}

void GeometryTPC::SetTH2PolyStrip(int ibin, std::shared_ptr<Geometry_Strip> s) {
	fStripMap[ibin] = s;
}

std::shared_ptr<Geometry_Strip> GeometryTPC::GetTH2PolyStrip(int ibin) {
	return fStripMap[ibin];
}

int GeometryTPC::GetDirNstrips(direction dir) const {
	return stripN.at(dir);
}

std::string GeometryTPC::GetDirName(direction dir) {
	return dir2name.at(dir);
}

std::shared_ptr<Geometry_Strip> GeometryTPC::GetStripByAget(int COBO_idx, int ASAD_idx, int AGET_idx, int channel_idx) const { // valid range [0-1][0-3][0-3][0-63]
	return arrayByAget.at({ COBO_idx , ASAD_idx,AGET_idx, channel_idx });
}

std::shared_ptr<Geometry_Strip> GeometryTPC::GetStripByDir(direction dir, int num) const { // valid range [0-2][1-1024]
	return stripArray.at({ dir, 0, num });
}

std::shared_ptr<Geometry_Strip> GeometryTPC::GetStripByDir(direction dir, int section, int num) const { // valid range [0-2][1-1024]
	return stripArray.at({ dir, section, num });
}


std::shared_ptr<Geometry_Strip> GeometryTPC::GetStripByGlobal(int global_channel_idx) { // valid range [0-1023]
	if (global_channel_idx >= 0) {
		int channel_idx = global_channel_idx % AGET_Nchan;
		int rest1 = global_channel_idx / AGET_Nchan;
		int AGET_idx = rest1 % AGET_Nchips;
		int rest2 = rest1 / AGET_Nchips;
		int counter = 0;
		for (int icobo = 0; icobo < COBO_N; icobo++) {
			for (int iasad = 0; iasad < ASAD_N[icobo]; iasad++) {
				if (counter == rest2) {
					return GetStripByAget(icobo, iasad, AGET_idx, channel_idx);
				}
				counter++;
			}
		}
	}
	return nullptr; // ERROR
}

int GeometryTPC::Aget_normal2raw(int channel_idx) { // valid range [0-63]
	if (channel_idx < 0 || channel_idx >= AGET_Nchan) return ERROR;
	int raw_channel_idx = channel_idx;
	for (auto& elem : FPN_chanId) {
		if (elem <= raw_channel_idx) ++raw_channel_idx;
	}
	return raw_channel_idx;
}

int GeometryTPC::Aget_fpn2raw(int FPN_idx) { // valid range [0-3]
	return FPN_chanId.at(FPN_idx);
}

int GeometryTPC::Global_normal2normal(int COBO_idx, int ASAD_idx, int aget_idx, int channel_idx) {   // valid range [0-1][0-3][0-3][0-63]
	if (ASAD_idx < 0 || ASAD_idx > ASAD_N[COBO_idx] || aget_idx < 0 || aget_idx >= AGET_Nchips ||
		channel_idx < 0 || channel_idx >= AGET_Nchan) return ERROR;
	int ASAD_offset = std::accumulate(ASAD_N.begin(), ASAD_N.begin() + COBO_idx, 0);
	return ((ASAD_offset + ASAD_idx) * AGET_Nchips + aget_idx) * AGET_Nchan + channel_idx;
}

// Checks if 2 strips are crossing inside the active area of TPC,
// on success (true) also returns the calculated offset vector wrt ORIGIN POINT (0,0)
bool GeometryTPC::GetCrossPoint(std::shared_ptr<Geometry_Strip> strip1, std::shared_ptr<Geometry_Strip> strip2, TVector2& point) {
	auto
		& op1 = (*strip1)(),
		& op2 = (*strip2)();
	const double u1[2] = { op1.unit_vec.X(), op1.unit_vec.Y() };
	const double u2[2] = { op2.unit_vec.X(), op2.unit_vec.Y() };
	double W = -u1[0] * u2[1] + u1[1] * u2[0];
	if (fabs(W) < NUM_TOLERANCE) return false;
	const double offset[2] = { op2.offset_vec.X() - op1.offset_vec.X(),
				   op2.offset_vec.Y() - op1.offset_vec.Y() };
	double W1 = -offset[0] * u2[1] + offset[1] * u2[0];
	double W2 = u1[0] * offset[1] - u1[1] * offset[0];
	double len1 = W1 / W;
	double len2 = W2 / W;
	double residual = 0.5 * pad_pitch;
	if (len1<-residual - NUM_TOLERANCE || len2<-residual - NUM_TOLERANCE ||
		len1 > op1.length() + NUM_TOLERANCE || len2 > op2.length() + NUM_TOLERANCE) return false;
	point.Set(op1.offset_vec.X() + len1 * op1.unit_vec.X(), op1.offset_vec.Y() + len1 * op1.unit_vec.Y());
	point += reference_point;
	return true;
}

// Checks if 3 strips are crossing inside the active area of TPC within a given tolerance (radius in [mm]),
// on success (true) also returns the average calculated offset vector wrt ORIGIN POINT (0,0)
bool GeometryTPC::MatchCrossPoint(std::array<int, 3> strip_nums, double radius, TVector2& point) {
	TVector2 points[3];
	std::array<std::shared_ptr<Geometry_Strip>, 3> strips;
	std::transform(strip_nums.begin(), strip_nums.end(), dirs.begin(), strips.begin(), [&](int strip_num, direction dir) { return GetStripByDir(dir, strip_num); });
	const double rad2 = pow(radius, 2);
	if (!GetCrossPoint(strips[1], strips[2], points[0]) ||
		!GetCrossPoint(strips[2], strips[0], points[1]) ||
		!GetCrossPoint(strips[0], strips[1], points[2]) ||
		(points[0] - points[1]).Mod2() > rad2 ||
		(points[1] - points[2]).Mod2() > rad2 ||
		(points[2] - points[0]).Mod2() > rad2) return false;
	point = (points[0] + points[1] + points[2]) / 3.0; // return average position wrt ORIGIN POINT (0,0)
	return true;
}

TVector2 GeometryTPC::GetStripPitchVector(direction dir) { // XY ([mm],[mm])
	return pitch_unit_vec.at(dir);
}

// Calculates (signed) distance in [mm] of direction of point (X=0,Y=0) from
// direction of the central axis of the existing strip (DIR, NUM) on
// the strip pitch axis for a given DIR family.
double GeometryTPC::Strip2posUVW(direction dir, int num) {
	return Strip2posUVW(GetStripByDir(dir, num));
}

// Calculates (signed) distance in [mm] of direction of point (X=0,Y=0) from
// direction of the central axis of the existing strip (Geometry_Strip object) on
// the strip pitch axis for a given strip family.
double GeometryTPC::Strip2posUVW(std::shared_ptr<Geometry_Strip> strip) {
	auto& op = (*strip)();
	return (reference_point + op.offset_vec) * pitch_unit_vec[op.dir]; // [mm]
}

// Calculates position in [mm] along Z-axis corresponding to
// the given (fractional) time cell number [0-511]
double GeometryTPC::Timecell2pos(double position_in_cells) {
	return (position_in_cells - AGET_Ntimecells) / sampling_rate * vdrift * 10.0 + trigger_delay * vdrift * 10.0; // [mm]
}

std::tuple<double, double, double, double> GeometryTPC::rangeXY() {

	std::vector<double>::iterator xmin, xmax, ymin, ymax;

	std::vector<double> xs, ys;
	for (auto& strip : GetStrips()) {
		auto& op = (*strip)();
		TVector2 vec = op.offset_vec + GetReferencePoint();
		xs.push_back(vec.X());
		ys.push_back(vec.Y());
	}
	std::tie(xmin, xmax) = std::minmax_element(xs.begin(), xs.end());
	std::tie(ymin, ymax) = std::minmax_element(ys.begin(), ys.end());

	*xmin -= GetStripPitch() * 0.3;
	*xmax += GetStripPitch() * 0.7;
	*ymin -= GetPadPitch() * 0.3;
	*ymax += GetPadPitch() * 0.7;

	return { *xmin, *xmax, *ymin, *ymax };
}

// Returns vector of first and last strips in each dir
std::vector<std::shared_ptr<Geometry_Strip>> GeometryTPC::GetStrips() const {
	std::vector<std::shared_ptr<Geometry_Strip>> _strips_;
	for (auto& _dir_ : dirs) {
		_strips_.push_back(GetStripByDir(_dir_, 1)); //first strip
		_strips_.push_back(GetStripByDir(_dir_, GetDirNstrips(_dir_))); //last strip
	}
	return _strips_;
}

double GeometryTPC::Strip2posUVW(direction dir, int section, int num) {
	return Strip2posUVW(GetStripByDir(dir, section, num));
}

// Calculates (signed) distance of projection of (X=0, Y=0) point from
// projection of a given (X,Y) point on the strip pitch axis
// for a given DIR family.
// The "err_flag" is set to TRUE if:
// - geometry has not been initialized properly, or
// - input strip DIR is invalid
double GeometryTPC::Cartesian2posUVW(double x, double y, direction dir) {
	return Cartesian2posUVW({ x, y }, dir); // [mm]
}

// Calculates (signed) distance of projection of (X=0, Y=0) point from
// projection of a given (X,Y) point on the strip pitch axis
// for a given DIR family.
double GeometryTPC::Cartesian2posUVW(TVector2 pos, direction dir) {
	return pos * pitch_unit_vec[dir]; // [mm]
}

void GeometryTPC::Debug() {
	for (auto& strip_it : stripArray) {
		auto& strip_data = (*strip_it.second)();
		auto p = strip_data.offset_vec + reference_point;
		double x = p.X();
		double y = p.Y();
		//auto p=strip_data->Offset()+reference_point;
		std::cout << p.X() << ", " << p.Y() << strip_data.section << std::endl;
		tp->Fill(x, y, strip_data.section + 1);
	}
}

//ClassImp(Geometry_Strip)
//ClassImp(GeometryTPC)