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
    isOK_TH2Poly(false),
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
   dir2name.insert(std::pair<projection, std::string>(projection(FPN_CH), "FPN"));
   
   name2dir.clear();
   for(auto&& it: dir2name) {
     name2dir.insert(std::pair<std::string, projection>(it.second, it.first));
   }
   
   // Load config file    
   Load(fname);

   if(_debug) {
     std::cout << "GeometryTPC::Constructor - Ended..." << std::endl;
   }
 }

bool GeometryTPC::Load(std::string fname) { 
  if(_debug) {
    std::cout << "GeometryTPC::Load - Started..." << std::endl;
  }

  std::cout << "\n==== INITIALIZING TPC GEOMETRY - START ====\n\n";

  initOK=false;

  mapByAget.clear();
  mapByAget_raw.clear();
  mapByStrip.clear();
  stripN.clear();
  ASAD_N.clear();
  fStripMap.clear();

  std::string line;
  std::ifstream f(fname);
  std::map<int, double> angle;

  if (f.is_open()) {

    // set U,V,W angles
    f.seekg (0, f.beg);
    while( getline(f,line) ) 
      {
	double angle1=0., angle2=0., angle3=0.;
	if(sscanf(line.c_str(),"ANGLES: %lf %lf %lf",&angle1, &angle2, &angle3)==3) {
	  angle1 = fmod( fmod(angle1, 360.)+360., 360.); // convert to range [0, 360 deg[ range 
	  angle2 = fmod( fmod(angle2, 360.)+360., 360.); // convert to range [0, 360 deg[ range 
	  angle3 = fmod( fmod(angle3, 360.)+360., 360.); // convert to range [0, 360 deg[ range 
	  if( fmod(angle1, 180.)!=fmod(angle2, 180.) &&  // reject parallel / anti-parallel duplicates 
	      fmod(angle2, 180.)!=fmod(angle3, 180.) &&  // reject parallel / anti-parallel duplicates 
	      fmod(angle3, 180.)!=fmod(angle1, 180.) ) { // reject parallel / anti-parallel duplicates 
	    angle[int(projection::DIR_U)] = angle1;
	    angle[int(projection::DIR_V)] = angle2;
	    angle[int(projection::DIR_W)] = angle3;
	    std::cout << Form("Angle of U/V/W strips wrt X axis = %lf / %lf / %lf deg", 
			      angle[int(projection::DIR_U)], angle[int(projection::DIR_V)], angle[int(projection::DIR_W)])
		      << std::endl;
	    break;
	  } else {
	    std::cerr << "ERROR: Wrong U/V/W angles !!!" << std::endl;	    
	    return initOK;
	  }
	}
      }

    // set pad size
    f.seekg (0, f.beg);
    while( getline(f,line) ) 
      {
	double val=0.;
	if(sscanf(line.c_str(),"DIAMOND SIZE: %lf",&val)==1) {
	  if(val>0.0) {
	    pad_size = val;
	    std::cout << Form("Length of diamond edge = %lf mm", pad_size)
		      << std::endl;
	    break;
	  } else {
	    std::cerr << "ERROR: Wrong pad size !!!" << std::endl;
	    if(_debug) {
	      std::cout << "GeometryTPC::Load - Abort (1)" << std::endl;
	    }
	    return initOK;
	  }
	}
      }
    pad_pitch   = pad_size * std::sqrt(3.);
    strip_pitch = pad_size * 1.5;

    // set REFERECE POINT offset [mm]
    f.seekg (0, f.beg);
    while( getline(f,line) ) 
      {
	double xoff=0.0, yoff=0.0;
	if(sscanf(line.c_str(),"REFERENCE POINT: %lf %lf",&xoff, &yoff)==2) {
	  if(fabs(xoff)<500. && fabs(yoff)<500.) { // [mm]
	    reference_point.Set(xoff, yoff);
	    std::cout << Form("Reference point offset = [%lf mm, %lf mm]", 
			      reference_point.X(), reference_point.Y())
		      << std::endl;
	    break;
	  } else {
	    std::cerr << "ERROR: Reference point coordinate >500 mm !!!" << std::endl;
	    if(_debug) {
	      std::cout << "GeometryTPC::Load - Abort (2)" << std::endl;
	    }
	    return initOK;	    	    
	  }
	}
      }

    // set electron drift velocity [cm/us]
    f.seekg (0, f.beg);
    while( getline(f,line) ) 
      {
	double val=0.;
	if(sscanf(line.c_str(),"DRIFT VELOCITY: %lf",&val)==1) {
	  if(val>0.0) {
	    vdrift = val;
	    std::cout << Form("Drift velocity = %lf cm/us", vdrift)
		      << std::endl;
	    break;
	  } else {
	    std::cerr << "ERROR: Wrong drift velocity !!!" << std::endl;
	    if(_debug) {
	      std::cout << "GeometryTPC::Load - Abort (3)" << std::endl;
	    }
	    return initOK;	    
	  }
	}
      }

    // set electronics sampling rate [MHz]
    f.seekg (0, f.beg);
    while( getline(f,line) ) 
      {
	double val=0.;
	if(sscanf(line.c_str(),"SAMPLING RATE: %lf",&val)==1) {
	  if(val>0.0) {
	    sampling_rate = val;
	    std::cout << Form("Sampling rate = %lf MHz", sampling_rate)
		      << std::endl;
	    break;
	  } else {
	    std::cerr << "ERROR: Wrong sampling rate !!!" << std::endl;
	    if(_debug) {
	      std::cout << "GeometryTPC::Load - Abort (4)" << std::endl;
	    }
	    return initOK;	    
	  }
	}
      }

    // set electronics trigger delay [us]
    f.seekg (0, f.beg);
    while( getline(f,line) ) 
      {
	double val=0.;
	if(sscanf(line.c_str(),"TRIGGER DELAY: %lf",&val)==1) {
	  if(fabs(val)<1000.) {
	    trigger_delay = val;
	    std::cout << Form("Trigger delay = %lf us", trigger_delay)
		      << std::endl;
	    break;
	  } else {
	    std::cerr << "ERROR: Trigger delay >1000 us !!!" << std::endl;
	    if(_debug) {
	      std::cout << "GeometryTPC::Load - Abort (5)" << std::endl;
	    }
	    return initOK;	    
	  }
	}
      }
	
    // set drift cage acceptance limits along Z-axis [mm]
    f.seekg (0, f.beg);
    while( getline(f,line) ) 
      {
	double val1=0.0, val2=0.0;
	if(sscanf(line.c_str(),"DRIFT CAGE ACCEPTANCE: %lf %lf",&val1, &val2)==2) {
	  if(fabs(val1)<200. && fabs(val2)<200. && val1<val2) { // [mm]
	    drift_zmin = val1;
	    drift_zmax = val2;
	    std::cout << Form("Drift cage Z-axis acceptance = [%lf mm, %lf mm]", 
			      drift_zmin, drift_zmax)
		      << std::endl;
	    break;
	  } else {
	    std::cerr << "ERROR: Drift cage acceptance limits mismatched or beyond 200 mm !!!" << std::endl;
	    if(_debug) {
	      std::cout << "GeometryTPC::Load - Abort (6)" << std::endl;
	    }
	    return initOK;	    	    
	  }
	}
      }

    // set unit vectors (along strips) and strip pitch vectors (perpendicular to strips)
    strip_unit_vec[int(projection::DIR_U)].Set( std::cos( angle[int(projection::DIR_U)]*TMath::DegToRad() ),
		std::sin( angle[int(projection::DIR_U)]*TMath::DegToRad() )  );
    strip_unit_vec[int(projection::DIR_V)].Set( std::cos( angle[int(projection::DIR_V)]*TMath::DegToRad() ),
		std::sin( angle[int(projection::DIR_V)]*TMath::DegToRad() )  );
    strip_unit_vec[int(projection::DIR_W)].Set( std::cos( angle[int(projection::DIR_W)]*TMath::DegToRad() ),
		std::sin( angle[int(projection::DIR_W)]*TMath::DegToRad() )  );

    pitch_unit_vec[int(projection::DIR_U)] = -1.0*( strip_unit_vec[int(projection::DIR_W)] + strip_unit_vec[int(projection::DIR_V)] ).Unit();
    pitch_unit_vec[int(projection::DIR_V)] =      ( strip_unit_vec[int(projection::DIR_U)] + strip_unit_vec[int(projection::DIR_W)] ).Unit();
    pitch_unit_vec[int(projection::DIR_W)] =      ( strip_unit_vec[int(projection::DIR_V)] - strip_unit_vec[int(projection::DIR_U)] ).Unit();

    bool found=false;
    f.seekg (0, f.beg);
    while ( getline (f,line) )
      {
	std::map<std::string, projection>::iterator it;
	char name[4];
	int cobo=0, asad=0, aget, strip_num, chan_num;
	double offset_in_pads, offset_in_strips, length_in_pads;

	if( (sscanf(line.c_str(), "%1s %d %d %d %d %d %lf %lf %lf",   // NEW FORMAT (several ASADs)
		    name, 
		    &strip_num, &cobo, &asad, &aget, &chan_num,
		    &offset_in_pads, &offset_in_strips, &length_in_pads)==9 || 
	     (sscanf(line.c_str(), "%1s %d %d %d %lf %lf %lf",         // LEGACY FORMAT (1 ASAD only)
		     name, 
		     &strip_num, &aget, &chan_num,
		     &offset_in_pads, &offset_in_strips, &length_in_pads)==7 && (cobo=0)==0 && (asad=0)==0 )) && 
	   strip_num>=1 &&
	   cobo>=0 &&
	   asad>=0 &&
	   aget>=0 && aget<=AGET_Nchips && 
	   chan_num>=0 && chan_num<AGET_Nchan &&
	   length_in_pads>=1.0 &&
	   std::string(name)!="FPN" &&                     // veto any FPN entries and
	   (it=name2dir.find(name))!=name2dir.end() ) {    // accept only U,V,W entries
	  
	  if(!found) {
	    found=true;
	    std::cout << "DIR     STRIP   COBO    ASAD    AGET    AGET_CH OFF_PAD OFF_STR LENGTH\n";
	  }
	  std::cout << Form("%-8s%-8d%-8d%-8d%-8d%-8d%-8.1lf%-8.1lf%-8.1lf", name, strip_num, cobo, asad, aget, chan_num, offset_in_pads, offset_in_strips, length_in_pads) << std::endl;

	  auto dir = it->second; // strip direction index

	  // DEBUG
	  if(_debug) {
	    std::cout << "DIRNAME=" << std::string(name) << " / DIR=" << int(dir) << " / STRIPNUM=" << strip_num 
		      << " / COBO=" << cobo << " / ASAD=" << asad
		      << " / AGET=" << aget << " / CHANNUM=" << chan_num << "\n";
	  }
	  // DEBUG

	  if(mapByAget.find(MultiKey4(cobo, asad, aget, chan_num))==mapByAget.end()) {

	    // create new strip
	    int chan_num_raw = Aget_normal2raw(chan_num);
	    TVector2 offset = offset_in_strips * strip_pitch * pitch_unit_vec[int(dir)] + offset_in_pads * pad_pitch * strip_unit_vec[int(dir)];
	    double length = length_in_pads * pad_pitch;
	    std::shared_ptr<StripTPC> strip = std::make_shared<StripTPC>(int(dir), strip_num, cobo, asad, aget, chan_num, chan_num_raw, 
					   strip_unit_vec[int(dir)], offset, length);

	    // update map (by: COBO board, ASAD board, AGET chip, AGET normal/raw channel)
	    mapByAget[MultiKey4(cobo, asad, aget,chan_num)]=strip ; // StripTPC(dir, strip_num);
	    mapByAget_raw[MultiKey4(cobo, asad, aget,chan_num_raw)]=strip; // StripTPC(dir, strip_num);

	    // update reverse map (by: strip direction, strip number)
	    if(IsDIR_UVW(dir)) {
	      mapByStrip[MultiKey2(int(dir), strip_num)]=strip; //Global_normal2normal(aget, chan_num); 
	    }

	    // update maximal ASAD index (by: COBO board) 
	    if(ASAD_N.find(cobo)==ASAD_N.end()) {
	      ASAD_N[cobo]=asad+1;  // ASAD indexing starts from 0
	      if(cobo>=COBO_N) COBO_N=cobo+1; // COBO indexing starts from 0
	    } else {
	      if(asad>=ASAD_N[cobo]) ASAD_N[cobo]=asad+1; // ASAD indexing starts from 0
	    }
	    
	    // update number of strips in each direction
	    if(stripN.find(int(dir))==stripN.end()) stripN[int(dir)] = 1;
	    else stripN[int(dir)]++;
	    
	    // DEBUG
		auto mkey4 = MultiKey4(cobo, asad, aget, chan_num);
	    if(_debug) {
            auto strip_int = (*mapByStrip[MultiKey2(int(dir), strip_num)])();
	      std::cout << ">>> ADDED NEW STRIP:" 
			<< "KEY=[COBO=" << cobo << ", ASAD=" << asad << ", AGET=" << aget << ", CHAN=" << chan_num 
			<<"]  VAL=[DIR=" << int(dir) << ", STRIP=" << strip_num << "], "
			<< "NSTRIPS[DIR="<< int(dir) <<"]=" << stripN[int(dir)] << ", "
			<< "   map_by_AGET=(" << (*mapByAget[mkey4])().dir << "," 
			<< (*mapByAget[mkey4])().num << "), "
			<< "   map_by_STRIP=(" << strip_int.coboId << ","
			<< strip_int.asadId << ","
			<< strip_int.agetId << ","
			<< strip_int.agetCh << ")"
			<<"\n";  
	    }
	    // DEBUG
	    
	  } else {
	    std::cout << "WARNING: Ignored duplicated keyword: COBO=" << cobo << ", ASAD=" << asad 
		      << ", AGET="<<aget<<", CHANNEL="<<chan_num<<" !!!\n";
	  } 
	  
	}
      }
    f.close();
  }
  else {
    std::cout << "\n==== INITIALIZING TPC GEOMETRY - END ====\n\n";
    std::cerr << "ERROR: Unable to open config file: " << fname << "!!!\n";
    if(_debug) {
      std::cout << "GeometryTPC::Load - Abort (7)" << std::endl;
    }
    return initOK;
  }
	
  // sanity checks
  for(int icobo=0; icobo<COBO_N; icobo++) {
    if(ASAD_N.find(icobo)==ASAD_N.end()) {
      std::cerr << "ERROR: Number of ASAD boards for COBO " << icobo << " is not defined !!!" << std::endl;
      if(_debug) {
	std::cout << "GeometryTPC::Load - Abort (8)" << std::endl;
      }
      return initOK;
    }
  }

  // adding FPN channels to mapByAget_raw
  for(int icobo=0; icobo<COBO_N; icobo++)
    for(int iasad=0; iasad<ASAD_N[icobo]; iasad++)
      for(int ichip=0; ichip<AGET_Nchips; ichip++) 
	for (unsigned i=0; i<FPN_chanId.size(); i++) {
	  mapByAget_raw[MultiKey4(icobo, iasad, ichip, FPN_chanId[i])] = std::make_shared<StripTPC>(FPN_CH, i+1, icobo, iasad, ichip, ERROR, FPN_chanId[i], 
										    TVector2(), TVector2(), 0.0);
	}

  // adding # of FPN channels to stripN
  stripN[FPN_CH]=FPN_chanId.size();

  // setting initOK=true at this stage is needed for TH2PolyInit and certain getter functions
  initOK = true;

  // print statistics
  std::cout << std::endl 
	    << "Geometry config file = " << fname << std::endl;
  std::cout << "Total number of " << this->GetDirName(projection::DIR_U) << " strips = " << this->GetDirNstrips(projection::DIR_U) << std::endl;
  std::cout << "Total number of " << this->GetDirName(projection::DIR_V) << " strips = " << this->GetDirNstrips(projection::DIR_V) << std::endl;
  std::cout << "Total number of " << this->GetDirName(projection::DIR_W) << " strips = " << this->GetDirNstrips(projection::DIR_W) << std::endl;
  std::cout << "Number of active channels per AGET chip = " << this->GetAgetNchannels() << std::endl;
  std::cout << "Number of " << this->GetDirName(static_cast<projection>(FPN_CH)) << " channels per AGET chip = " << this->GetDirNstrips(static_cast<projection>(FPN_CH)) << std::endl;
  std::cout << "Number of raw channels per AGET chip = " << this->GetAgetNchannels_raw() << std::endl;
  std::cout << "Number of AGET chips per ASAD board = " << this->GetAgetNchips() << std::endl;
  std::cout << "Number of ASAD boards = " << this->GetAsadNboards() << std::endl;
  std::cout << "Number of COBO boards = " << this->GetCoboNboards() << std::endl;

  // now initialize TH2Poly (while initOK=true) and set initOK flag according to the result 
  initOK = InitTH2Poly();

  std::cout << "\n==== INITIALIZING TPC GEOMETRY - END ====\n\n";

  if(!initOK) {
    std::cerr << "ERROR: Cannot initialize TH2Poly !!!" << std::endl;
    if(_debug) {
      std::cout << "GeometryTPC::Load - Abort (9)" << std::endl;
    }
    return initOK;
  }

  if(_debug) {
    std::cout << "GeometryTPC::Load - Ended..." << std::endl;
  }
  return initOK; 
}

bool GeometryTPC::InitTH2Poly() {

  if(_debug) {
    std::cout << "GeometryTPC::InitTH2Poly - Started..." << std::endl;
  }

  isOK_TH2Poly=false;
  fStripMap.clear();

  // sanity checks
  if( grid_nx<1 || grid_ny<1 || !initOK) {
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
  for(auto&& dir : proj_vec_UVW) {
    for(int num=1; num<=this->GetDirNstrips(static_cast<projection>(dir)); num++) {
		auto s = this->GetStripByDir(dir, num);
        auto op = (*s)();
      if(!s) continue;
      TVector2 point1 = reference_point + op.offset_vec 
	- op.unit_vec*0.5*pad_pitch;
      TVector2 point2 = point1 + op.unit_vec*op.length;
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
  for (auto&& dir : proj_vec_UVW) {
    ///////// DEBUG
    if(_debug) {
      std::cout << "GeometryTPC::InitTH2Poly: nstrips[" << int(dir) << "]=" << this->GetDirNstrips(dir) << std::endl;
    }
    ///////// DEBUG

    for(int num=1; num<=this->GetDirNstrips(dir); num++) {

      auto s = this->GetStripByDir(dir, num);
      auto op = (*s)();
      if(s == nullptr) continue;

      // create diamond-shaped pad for a given direction
      std::vector<TVector2> offset_vec;
      offset_vec.push_back(TVector2(0.,0.));
      offset_vec.push_back(op.unit_vec.Rotate(pi/6.)*pad_size);
      offset_vec.push_back(op.unit_vec *pad_pitch);
      offset_vec.push_back(op.unit_vec.Rotate(-pi/6.)*pad_size);
      offset_vec.push_back(TVector2(0.,0.));

      TVector2 point0 = reference_point + op.offset_vec - op.unit_vec *0.5*pad_pitch;
      
      const int npads = (int)(op.length/pad_pitch );
      const int npoints = npads * offset_vec.size();
      TGraph *g = new TGraph(npoints);
      int ipoint=0;
      for(int ipad=0; ipad<npads; ipad++) {
	for(size_t icorner=0; icorner<offset_vec.size(); icorner++) {
	  TVector2 corner = point0 + op.unit_vec *ipad*pad_pitch + offset_vec[icorner];
	  g->SetPoint(ipoint, corner.X(), corner.Y());
	  ipoint++;
	}
      }
      
      // create new TH2PolyBin

      // DEBUG
      if(_debug) {
	std::cout << "TH2POLY ADDBIN: BEFORE CREATING BIN: GetNumberOfBins=" << tp->GetNumberOfBins() << std::endl;
      }
      // DEBUG      

      // primary method:
      //      const int ibin = tp->AddBin(g);

      // alternative method due to bin endexing bug in TH2Poly::AddBin() implementation:
      const int nbins_old = tp->GetNumberOfBins();
      int ibin = tp->AddBin(g);
      const int nbins_new = tp->GetNumberOfBins();
      if(nbins_new>nbins_old) {
	TH2PolyBin *bin = (TH2PolyBin*) tp->GetBins()->At( tp->GetNumberOfBins()-1 );
	if(bin) ibin = bin->GetBinNumber();
      }

      // DEBUG
      if(_debug) {
	std::cout << "TH2POLY ADDBIN: AFTER CREATING BIN: GetNumberOfBins=" << tp->GetNumberOfBins() << std::endl;
	std::cout << "TH2POLY ADDBIN: " 
		  << "TH2POLY_IBIN=" << ibin
	  //		  << ", TH2POLYBIN_PTR=" << bin 	          
		  << ", DIR=" << this->GetDirName(dir) << ", NUM=" << num
		  << ", NPADS=" << npads 
		  << ", NPOINTS=" << npoints << " (" << ipoint << ")"
		  << std::endl;
      }
      // DEBUG      

      ///////// DEBUG
      // strip_counter++;
      ///////// DEBUG

      // DEBUG
      
      if(ibin<1) { // failed to create new bin
	std::cerr << "ERROR: Failed to create new TH2Poly bin: TH2POLY->AddBin()=" << ibin
		  << ", DIR=" << this->GetDirName(dir) << ", NUM=" << num
		  << ", NPADS=" << npads 
		  << ", NPOINTS=" << npoints << ", TGraph->Print():" 
		  << std::endl;
	g->Print();
	std::cerr << std::endl << std::flush;
	return false;
	//	continue; 
      }

      // update strip map
      SetTH2PolyStrip(ibin, s);

      // DEBUG
      if(_debug) {
	std::cout << "TH2POLY ADDBIN: DIR=" << this->GetDirName(dir) << ", NUM=" << num
		  << ", TH2POLY_IBIN=" << ibin
		  << ", NPADS=" << npads 
		  << ", NPOINTS=" << npoints << " (" << ipoint << ")"
		  << std::endl;
      }
      // DEBUG

    }
  }

  // final result
  if(fStripMap.size()>0) isOK_TH2Poly=true;

  if(_debug) {
    std::cout << "GeometryTPC::InitTH2Poly - Ended..." << std::endl;
  }

  return isOK_TH2Poly;
}

void GeometryTPC::SetTH2PolyStrip(int ibin, std::shared_ptr<StripTPC> s) {
  if(s != nullptr) fStripMap[ibin]=s;
}

int GeometryTPC::GetDirNstrips(projection dir) {
  if (IsOK() && (IsDIR_UVW(dir) || dir == projection(FPN_CH))) {
	  return stripN[int(dir)];
  };
  return ERROR;
}

std::string GeometryTPC::GetDirName(projection dir) {
  if ((IsDIR_UVW(dir) || dir == projection(FPN_CH)) && IsOK()) {
	  return dir2name.at(dir); // dir2name[dir];
  };
  return std::string(); // "ERROR";
}

std::shared_ptr<StripTPC> GeometryTPC::GetStripByAget(int COBO_idx, int ASAD_idx, int AGET_idx, int channel_idx) { // valid range [0-1][0-3][0-3][0-63]
  auto it = mapByAget.find(MultiKey4(COBO_idx, ASAD_idx, AGET_idx, channel_idx));
  if( it != mapByAget.end() && IsOK()) {
    return it->second;
  }
  return nullptr; // ERROR
}

std::shared_ptr<StripTPC> GeometryTPC::GetStripByGlobal(int global_channel_idx) { // valid range [0-1023]
  if(global_channel_idx<0) return nullptr; // ERROR
  int AGET_idx = (int) (global_channel_idx / AGET_Nchan);         // relative AGET index
  int ASAD_idx = (int) (AGET_idx / AGET_Nchips);                  // relative ASAD index
  int channel_idx = global_channel_idx % AGET_Nchan;
  int counter=0;
  for(int icobo=0; icobo<COBO_N; icobo++)
    for(int iasad=0; iasad<ASAD_N[icobo]; iasad++) {
      if(counter==ASAD_idx) {
	return this->GetStripByAget(icobo, iasad, AGET_idx, channel_idx);
      }
      counter++;
    }
  return nullptr; // ERROR
}
                                     
std::shared_ptr<StripTPC> GeometryTPC::GetStripByAget_raw(int COBO_idx, int ASAD_idx, int AGET_idx, int raw_channel_idx) { // valid range [0-1][0-3][0-3][0-67]
  auto it = mapByAget_raw.find(MultiKey4(COBO_idx, ASAD_idx, AGET_idx, raw_channel_idx));
  if( it != mapByAget_raw.end() && IsOK()) {
    return it->second;
  }
  return nullptr; // ERROR
}

std::shared_ptr<StripTPC> GeometryTPC::GetStripByDir(projection dir, int num) { // valid range [0-2][1-1024]
  return this->GetStripByGlobal( Global_strip2normal(int(dir), num) );
}

int GeometryTPC::Aget_normal2raw(int channel_idx) { // valid range [0-63]
  if(/*!IsOK() || */channel_idx<0 || channel_idx>=AGET_Nchan) return ERROR;
  int raw_channel_idx = channel_idx;
  for(unsigned i=0; i<FPN_chanId.size(); ++i) {
    if(FPN_chanId[i]<raw_channel_idx) ++raw_channel_idx;
    if(FPN_chanId[i]==raw_channel_idx) ++raw_channel_idx;
  }
  return raw_channel_idx;
}

int GeometryTPC::Global_normal2raw(int COBO_idx, int ASAD_idx, int aget_idx, int channel_idx) { // valid range [0-1][0-3][0-3][0-63]
  int glb_raw_channel_idx = Aget_normal2raw(channel_idx);
  if( glb_raw_channel_idx==ERROR || COBO_idx<0 || COBO_idx>=COBO_N || ASAD_idx<0 || ASAD_idx>ASAD_N[COBO_idx] || aget_idx<0 || aget_idx>=AGET_Nchips) return ERROR;
  int ASAD_offset=0;
  for(int icobo=0; icobo<COBO_idx; icobo++) {
    ASAD_offset += ASAD_N[icobo];
  }
  return ((ASAD_offset+ASAD_idx)*AGET_Nchips + aget_idx)*AGET_Nchan_raw + glb_raw_channel_idx; 
}

int GeometryTPC::Aget_fpn2raw(int FPN_idx) { // valid range [0-3]
  if(/*!IsOK() || */FPN_idx<0 || FPN_idx>=stripN[FPN_CH]) return ERROR;
  return FPN_chanId.at(FPN_idx);
}
                         
int GeometryTPC::Global_normal2normal(int COBO_idx, int ASAD_idx, int aget_idx, int channel_idx) {   // valid range [0-1][0-3][0-3][0-63]
  if(/*!IsOK() || */COBO_idx<0 || COBO_idx>=COBO_N || ASAD_idx<0 || ASAD_idx>ASAD_N[COBO_idx] || aget_idx<0 || aget_idx>=AGET_Nchips ||
     channel_idx<0 || channel_idx>=AGET_Nchan ) return ERROR;
  int ASAD_offset=0;
  for(int icobo=0; icobo<COBO_idx; icobo++) {
    ASAD_offset += ASAD_N[icobo];
  }
  return ((ASAD_offset+ASAD_idx)*AGET_Nchips + aget_idx)*AGET_Nchan + channel_idx; 
}

int GeometryTPC::Global_strip2normal(int dir, int num) {                 // valid range [0-2][1-92]
  auto it=mapByStrip.find(MultiKey2(dir, num));
  if (it != mapByStrip.end() && it->second) {
      auto op = (*it->second)();
      return Global_normal2normal(op.coboId, op.asadId, op.agetId, op.agetCh);
  }
  return ERROR;
}

// Checks if 2 strips are crossing inside the active area of TPC,
// on success (true) also returns the calculated offset vector wrt ORIGIN POINT (0,0)
bool GeometryTPC::GetCrossPoint(std::shared_ptr<StripTPC> strip1, std::shared_ptr<StripTPC> strip2, TVector2 &point) {
  if(!strip1 || !strip2) return false;
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
  if (IsDIR_UVW(dir) && IsOK()) {
	  return pitch_unit_vec.at(int(dir));
  };
  return TVector2(0, 0); // ERROR  
}

// Calculates (signed) distance in [mm] of projection of point (X=0,Y=0) from
// projection of the central axis of the existing strip (DIR, NUM) on
// the strip pitch axis for a given DIR family.
// The "err_flag" is set to TRUE if:
// - geometry has not been initialized properly, or
// - input strip DIR and/or NUM are invalid
double GeometryTPC::Strip2posUVW(projection dir, int num, bool &err_flag) {
  return this->Strip2posUVW( this->GetStripByDir(dir, num), err_flag );
}

// Calculates (signed) distance in [mm] of projection of point (X=0,Y=0) from
// projection of the central axis of the existing strip (StripTPC object) on
// the strip pitch axis for a given strip family.
// The "err_flag" is set to TRUE if:
// - geometry has not been initialized properly, or
// - input StripTPC object is invalid
double GeometryTPC::Strip2posUVW(std::shared_ptr<StripTPC> strip, bool &err_flag) {
  err_flag=true;
  auto op = (*strip)();
  if (IsDIR_UVW(static_cast<projection>(op.dir)) && strip != nullptr) {
	  err_flag=false; // valid strip
    return (reference_point + op.offset_vec)*pitch_unit_vec[op.dir]; // [mm]
  };
  return 0.0; // ERROR
}

// Calculates position in [mm] along Z-axis corresponding to
// the given (fractional) time cell number [0-511]
// The "err_flag" is set to TRUE if:
// - geometry has not been initialized properly, or
// - input time-cell is outside of the valid range [0-511]
double GeometryTPC::Timecell2pos(double position_in_cells, bool &err_flag) {
  err_flag=true;
  if(!IsOK()) return 0.0; // ERROR
  if(position_in_cells>=0.0 && position_in_cells<=AGET_Ntimecells) err_flag=false; // check range
  return (position_in_cells-AGET_Ntimecells)/sampling_rate*vdrift*10.0 + trigger_delay*vdrift*10.0; // [mm]
}

std::tuple<double, double, double, double> GeometryTPC::rangeXY(){

	auto s = GetStrips();

  double xmin=1E30;
  double xmax=-1E30;
  double ymin=1E30;
  double ymax=-1E30;

  for(int i=0; i<6; i++) {
      auto op = (*s[i])();
    if(!s[i]) continue;
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

std::vector<std::shared_ptr<StripTPC>> GeometryTPC::GetStrips() {
	std::vector<std::shared_ptr<StripTPC>> _strips_;
	for (auto& _dir_ : proj_vec_UVW) {
		_strips_.push_back(GetStripByDir(_dir_, 1));
		_strips_.push_back(GetStripByDir(_dir_, GetDirNstrips(_dir_)));
	}
	return _strips_;
}
  
//ClassImp(StripTPC)
//ClassImp(GeometryTPC)
