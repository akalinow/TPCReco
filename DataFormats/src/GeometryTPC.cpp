#include <cstdio> // for: NULL
#include <cstdlib>
#include <fstream>
#include <iostream> // for: cout, cerr, endl
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "TGraph.h"
#include "TH2Poly.h"
#include "TMath.h"
#include "TROOT.h"
#include "TVector2.h"
#include "TVector3.h"
#include "TRandom3.h" // DEBUG

#include "colorText.h"
#include "GeometryTPC.h"
#include "MultiKey.h"

StripTPC::StripTPC(int direction, int section, int number, int cobo_index,
                   int asad_index, int aget_index, int aget_channel,
                   int aget_channel_raw, TVector2 unit_vector,
                   TVector2 offset_vector_in_mm, int number_of_pads,
                   GeometryTPC *g)
    : geo_ptr(g), dir(direction), section(section), num(number),
      coboId(cobo_index), asadId(asad_index), agetId(aget_index),
      agetCh(aget_channel), agetCh_raw(aget_channel_raw), unit_vec(unit_vector),
      offset_vec(offset_vector_in_mm), npads(number_of_pads) {}

int StripTPC::GlobalCh() {
  if (geo_ptr && geo_ptr->IsOK())
    return geo_ptr->Global_normal2normal(coboId, asadId, agetId, agetCh);
  return ERROR;
}

int StripTPC::GlobalCh_raw() {
  if (geo_ptr && geo_ptr->IsOK())
    return geo_ptr->Global_normal2raw(coboId, asadId, agetId, agetCh);
  return ERROR;
}

GeometryTPC::GeometryTPC(const char *fname, bool debug)
    : initOK(false), COBO_N(0), AGET_Nchips(4), AGET_Nchan(64),
      AGET_Nchan_fpn(-1), AGET_Nchan_raw(-1), AGET_Ntimecells(512),
      pad_size(0.), pad_pitch(0.), strip_pitch(0.),
      drift_zmin(0.), drift_zmax(0.),
      grid_nx(25), grid_ny(25), isOK_TH2Poly(false), _debug(debug) {
  if (_debug) {
    std::cout << "GeometryTPC::Constructor - Started..." << std::flush
              << std::endl;
  }

  tp = new TH2Poly();
  tp->SetName("h_uvw_dummy");
  tp_convex = new TGraph();

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
  dir2name.insert(std::pair<int, std::string>(DIR_U, "U"));
  dir2name.insert(std::pair<int, std::string>(DIR_V, "V"));
  dir2name.insert(std::pair<int, std::string>(DIR_W, "W"));
  dir2name.insert(std::pair<int, std::string>(FPN_CH, "FPN"));

  name2dir.clear();
  std::map<int, std::string>::iterator it;
  for (it = dir2name.begin(); it != dir2name.end(); it++) {
    name2dir.insert(std::pair<std::string, int>(it->second, it->first));
  }

  // Load config file
  Load(fname);

  if (_debug) {
    std::cout << "GeometryTPC::Constructor - Ended..." << std::flush
              << std::endl;
  }
}

bool GeometryTPC::Load(const char *fname) {
  if (_debug) {
    std::cout << "GeometryTPC::Load - Started..." << std::flush << std::endl;
  }

  std::cout << "\n==== INITIALIZING TPC GEOMETRY - START ====\n\n";

  initOK = false;

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
    f.seekg(0, f.beg);
    while (getline(f, line)) {
      double angle1 = 0., angle2 = 0., angle3 = 0.;
      if (sscanf(line.c_str(), "ANGLES: %lf %lf %lf", &angle1, &angle2,
                 &angle3) == 3) {
        angle1 = fmod(fmod(angle1, 360.) + 360.,
                      360.); // convert to range [0, 360 deg[ range
        angle2 = fmod(fmod(angle2, 360.) + 360.,
                      360.); // convert to range [0, 360 deg[ range
        angle3 = fmod(fmod(angle3, 360.) + 360.,
                      360.); // convert to range [0, 360 deg[ range
        if (fmod(angle1, 180.) !=
                fmod(angle2,
                     180.) && // reject parallel / anti-parallel duplicates
            fmod(angle2, 180.) !=
                fmod(angle3,
                     180.) && // reject parallel / anti-parallel duplicates
            fmod(angle3, 180.) !=
                fmod(angle1,
                     180.)) { // reject parallel / anti-parallel duplicates
          angle[DIR_U] = angle1;
          angle[DIR_V] = angle2;
          angle[DIR_W] = angle3;
          std::cout
              << Form("Angle of U/V/W strips wrt X axis = %lf / %lf / %lf deg",
                      angle[DIR_U], angle[DIR_V], angle[DIR_W])
              << std::endl;
          break;
        } else {
          std::cerr << "ERROR: Wrong U/V/W angles !!!" << std::endl;
          return initOK;
        }
      }
    }

    // set pad size
    f.seekg(0, f.beg);
    while (getline(f, line)) {
      double val = 0.;
      if (sscanf(line.c_str(), "DIAMOND SIZE: %lf", &val) == 1) {
        if (val > 0.0) {
          pad_size = val;
          std::cout << Form("Length of diamond edge = %lf mm", pad_size)
                    << std::endl;
          break;
        } else {
          std::cerr << "ERROR: Wrong pad size !!!" << std::endl;
          if (_debug) {
            std::cout << "GeometryTPC::Load - Abort (1)" << std::flush
                      << std::endl;
          }
          return initOK;
        }
      }
    }
    pad_pitch = pad_size * TMath::Sqrt(3.);
    strip_pitch = pad_size * 1.5;

    // set REFERECE POINT offset [mm]
    f.seekg(0, f.beg);
    while (getline(f, line)) {
      double xoff = 0.0, yoff = 0.0;
      if (sscanf(line.c_str(), "REFERENCE POINT: %lf %lf", &xoff, &yoff) == 2) {
        if (fabs(xoff) < 500. && fabs(yoff) < 500.) { // [mm]
          reference_point.Set(xoff, yoff);
          std::cout << Form("Reference point offset = [%lf mm, %lf mm]",
                            reference_point.X(), reference_point.Y())
                    << std::endl;
          break;
        } else {
          std::cerr << "ERROR: Reference point coordinate >500 mm !!!"
                    << std::endl;
          if (_debug) {
            std::cout << "GeometryTPC::Load - Abort (2)" << std::flush
                      << std::endl;
          }
          return initOK;
        }
      }
    }

    // set electron drift velocity [cm/us]
    f.seekg(0, f.beg);
    while (getline(f, line)) {
      double val = 0.;
      if (sscanf(line.c_str(), "DRIFT VELOCITY: %lf", &val) == 1) {
        if (val > 0.0) {
          setDriftVelocity(val);
          std::cout << Form("Drift velocity = %lf cm/us", val) << std::endl;
          break;
        } else {
          std::cerr << "ERROR: Wrong drift velocity !!!" << std::endl;
          if (_debug) {
            std::cout << "GeometryTPC::Load - Abort (3)" << std::flush
                      << std::endl;
          }
          return initOK;
        }
      }
    }

    // set electronics sampling rate [MHz]
    f.seekg(0, f.beg);
    while (getline(f, line)) {
      double val = 0.;
      if (sscanf(line.c_str(), "SAMPLING RATE: %lf", &val) == 1) {
        if (val > 0.0) {
          setSamplingRate(val);
          std::cout << Form("Sampling rate = %lf MHz", val)
                    << std::endl;
          break;
        } else {
          std::cerr << "ERROR: Wrong sampling rate !!!" << std::endl;
          if (_debug) {
            std::cout << "GeometryTPC::Load - Abort (4)" << std::flush
                      << std::endl;
          }
          return initOK;
        }
      }
    }

    // set electronics trigger delay [us]
    f.seekg(0, f.beg);
    while (getline(f, line)) {
      double val = 0.;
      if (sscanf(line.c_str(), "TRIGGER DELAY: %lf", &val) == 1) {
        if (fabs(val) < 1000.) {
          setTriggerDelay(val);
          std::cout << Form("Trigger delay = %lf us", val)
                    << std::endl;
          break;
        } else {
          std::cerr << "ERROR: Trigger delay >1000 us !!!" << std::endl;
          if (_debug) {
            std::cout << "GeometryTPC::Load - Abort (5)" << std::flush
                      << std::endl;
          }
          return initOK;
        }
      }
    }

    // set drift cage acceptance limits along Z-axis [mm]
    f.seekg(0, f.beg);
    while (getline(f, line)) {
      double val1 = 0.0, val2 = 0.0;
      if (sscanf(line.c_str(), "DRIFT CAGE ACCEPTANCE: %lf %lf", &val1,
                 &val2) == 2) {
        if (fabs(val1) < 200. && fabs(val2) < 200. && val1 < val2) { // [mm]
          drift_zmin = val1;
          drift_zmax = val2;
          std::cout << Form("Drift cage Z-axis acceptance = [%lf mm, %lf mm]",
                            drift_zmin, drift_zmax)
                    << std::endl;
          break;
        } else {
          std::cerr << "ERROR: Drift cage acceptance limits mismatched or "
                       "beyond 200 mm !!!"
                    << std::endl;
          if (_debug) {
            std::cout << "GeometryTPC::Load - Abort (6)" << std::flush
                      << std::endl;
          }
          return initOK;
        }
      }
    }

    // set unit vectors (along strips) and strip pitch vectors (perpendicular to
    // strips)
    strip_unit_vec[DIR_U].Set(TMath::Cos(angle[DIR_U] * TMath::DegToRad()),
                              TMath::Sin(angle[DIR_U] * TMath::DegToRad()));
    strip_unit_vec[DIR_V].Set(TMath::Cos(angle[DIR_V] * TMath::DegToRad()),
                              TMath::Sin(angle[DIR_V] * TMath::DegToRad()));
    strip_unit_vec[DIR_W].Set(TMath::Cos(angle[DIR_W] * TMath::DegToRad()),
                              TMath::Sin(angle[DIR_W] * TMath::DegToRad()));

    pitch_unit_vec[DIR_U] =
        -1.0 * (strip_unit_vec[DIR_W] + strip_unit_vec[DIR_V]).Unit();
    pitch_unit_vec[DIR_V] =
        (strip_unit_vec[DIR_U] + strip_unit_vec[DIR_W]).Unit();
    pitch_unit_vec[DIR_W] =
        (strip_unit_vec[DIR_V] - strip_unit_vec[DIR_U]).Unit();

    bool found = false;
    f.seekg(0, f.beg);
    while (getline(f, line)) {
      std::map<std::string, int>::iterator it;
      char name[4];
      int section = 0, strip_num, cobo = 0, asad = 0, aget, chan_num;
      double offset_in_pads, offset_in_strips, length_in_pads;

      if ((sscanf(line.c_str(),
                  "%1s %d %d %d %d %d %d %lf %lf %lf", // NEW FORMAT (several
                                                       // ASADs)
                  name, &section, &strip_num, &cobo, &asad, &aget, &chan_num,
                  &offset_in_pads, &offset_in_strips, &length_in_pads) == 10 ||
           (sscanf(line.c_str(),
                   "%1s %d %d %d %lf %lf %lf", // LEGACY FORMAT (1 ASAD only)
                   name, &strip_num, &aget, &chan_num, &offset_in_pads,
                   &offset_in_strips, &length_in_pads) == 7 &&
            (section = 0) == 0 && (cobo = 0) == 0 && (asad = 0) == 0)) &&
          strip_num >= 1 && cobo >= 0 && asad >= 0 && aget >= 0 &&
          aget <= AGET_Nchips && chan_num >= 0 && chan_num < AGET_Nchan &&
          length_in_pads >= 1.0 &&
          std::string(name) != "FPN" && // veto any FPN entries and
          (it = name2dir.find(name)) !=
              name2dir.end()) { // accept only U,V,W entries

        if (!found) {
          found = true;
          std::cout << "DIR  SECTION   STRIP   COBO    ASAD    AGET    AGET_CH "
                       "OFF_PAD OFF_STR LENGTH\n";
        }
        std::cout << Form("%-8s%-8d%-8d%-8d%-8d%-8d%-8d%-8.1lf%-8.1lf%-8.1lf",
                          name, section, strip_num, cobo, asad, aget, chan_num,
                          offset_in_pads, offset_in_strips, length_in_pads)
                  << std::endl;

        int dir = it->second; // strip direction index
        // DEBUG
        if (_debug) {
          std::cout << "DIRNAME=" << std::string(name) << " / DIR=" << dir
                    << " / SECTION=" << section << " / STRIPNUM=" << strip_num
                    << " / COBO=" << cobo << " / ASAD=" << asad
                    << " / AGET=" << aget << " / CHANNUM=" << chan_num << "\n";
        }
        // DEBUG
        if (mapByAget.find(MultiKey4(cobo, asad, aget, chan_num)) ==
            mapByAget.end()) {

          // create new strip
          int chan_num_raw = Aget_normal2raw(chan_num);
          TVector2 offset =
              offset_in_strips * strip_pitch * pitch_unit_vec[dir] +
              offset_in_pads * pad_pitch * strip_unit_vec[dir];
          double length = length_in_pads * pad_pitch;
          StripTPC *strip = new StripTPC(
              dir, section, strip_num, cobo, asad, aget, chan_num, chan_num_raw,
              strip_unit_vec[dir], offset, length_in_pads, this);

          // update map (by: COBO board, ASAD board, AGET chip, AGET normal/raw
          // channel)
          mapByAget[MultiKey4(cobo, asad, aget, chan_num)] =
              strip; // StripTPC(dir, strip_num);
          mapByAget_raw[MultiKey4(cobo, asad, aget, chan_num_raw)] =
              strip; // StripTPC(dir, strip_num);

          // update reverse map (by: strip direction, strip number)
          if (dir == DIR_U || dir == DIR_V || dir == DIR_W) {
            mapByStrip[MultiKey3(dir, section, strip_num)] =
                strip; // Global_normal2normal(aget, chan_num);
          }

          // update maximal ASAD index (by: COBO board)
          if (ASAD_N.find(cobo) == ASAD_N.end()) {
            ASAD_N[cobo] = asad + 1; // ASAD indexing starts from 0
            if (cobo >= COBO_N)
              COBO_N = cobo + 1; // COBO indexing starts from 0
          } else {
            if (asad >= ASAD_N[cobo])
              ASAD_N[cobo] = asad + 1; // ASAD indexing starts from 0
          }

          // update number of strips in each direction
          if (stripN.find(dir) == stripN.end())
            stripN[dir] = 1;
          else
            stripN[dir]++;

          geometryStats.Fill(dir, section, strip_num);

          // DEBUG
          if (_debug) {
            std::cout
                << ">>> ADDED NEW STRIP:"
                << "KEY=[COBO=" << cobo << ", ASAD=" << asad
                << ", AGET=" << aget << ", CHAN=" << chan_num
                << "]  VAL=[DIR=" << dir << ", SECTION=" << section
                << ", STRIP=" << strip_num << "], "
                << "NSTRIPS[DIR=" << dir << "]=" << stripN[dir] << ", "
                << "   map_by_AGET=("
                << mapByAget[MultiKey4(cobo, asad, aget, chan_num)]->Dir()
                << ","
                << mapByAget[MultiKey4(cobo, asad, aget, chan_num)]->Num()
                << "), "
                << "   map_by_STRIP=("
                << mapByStrip[MultiKey3(dir, section, strip_num)]->CoboId()
                << ","
                << mapByStrip[MultiKey3(dir, section, strip_num)]->AsadId()
                << ","
                << mapByStrip[MultiKey3(dir, section, strip_num)]->AgetId()
                << ","
                << mapByStrip[MultiKey3(dir, section, strip_num)]->AgetCh()
                << ")"
                << "\n";
            std::cout << offset_in_pads << " " << offset_in_strips << " "
                      << length_in_pads << " " << length << std::endl;
          }
          // DEBUG

        } else {
          std::cout << "WARNING: Ignored duplicated keyword: COBO=" << cobo
                    << ", ASAD=" << asad << ", AGET=" << aget
                    << ", CHANNEL=" << chan_num << " !!!\n";
        }
      }
    }
    auto retv = LoadAnalog(f);
    f.close();
    if (!retv) {
      return retv;
    }
  } else {
    std::cout << "\n==== INITIALIZING TPC GEOMETRY - END ====\n\n";
    std::cerr <<KRED<< "ERROR: Unable to open config file: " <<RST<< fname << "!!!\n";
    if (_debug) {
      std::cout << "GeometryTPC::Load - Abort (7)" << std::flush << std::endl;
    }
    return initOK;
  }

  // sanity checks
  for (int icobo = 0; icobo < COBO_N; icobo++) {
    if (ASAD_N.find(icobo) == ASAD_N.end()) {
      std::cerr << "ERROR: Number of ASAD boards for COBO " << icobo
                << " is not defined !!!" << std::endl;
      if (_debug) {
        std::cout << "GeometryTPC::Load - Abort (8)" << std::flush << std::endl;
      }
      return initOK;
    }
  }

  // adding FPN channels to mapByAget_raw
  for (int icobo = 0; icobo < COBO_N; icobo++)
    for (int iasad = 0; iasad < ASAD_N[icobo]; iasad++)
      for (int ichip = 0; ichip < AGET_Nchips; ichip++)
        for (uint i = 0; i < FPN_chanId.size(); i++) {
          mapByAget_raw[MultiKey4(icobo, iasad, ichip, FPN_chanId[i])] =
              new StripTPC(FPN_CH, 0, i + 1, icobo, iasad, ichip, ERROR,
                           FPN_chanId[i], TVector2(), TVector2(), 0, this);
        }

  // adding # of FPN channels to stripN
  stripN[FPN_CH] = FPN_chanId.size();

  // setting initOK=true at this stage is needed for TH2PolyInit and certain
  // getter functions
  initOK = true;

  // print statistics
  std::cout << std::endl << "Geometry config file = " << fname << std::endl;
  std::cout << "Total number of " << this->GetDirName(DIR_U)
            << " strips = " << this->GetDirNstrips(DIR_U) << std::endl;
  std::cout << "Total number of " << this->GetDirName(DIR_V)
            << " strips = " << this->GetDirNstrips(DIR_V) << std::endl;
  std::cout << "Total number of " << this->GetDirName(DIR_W)
            << " strips = " << this->GetDirNstrips(DIR_W) << std::endl;
  std::cout << "Number of active channels per AGET chip = "
            << this->GetAgetNchannels() << std::endl;
  std::cout << "Number of " << this->GetDirName(FPN_CH)
            << " channels per AGET chip = " << this->GetDirNstrips(FPN_CH)
            << std::endl;
  std::cout << "Number of raw channels per AGET chip = "
            << this->GetAgetNchannels_raw() << std::endl;
  std::cout << "Number of AGET chips per ASAD board = " << this->GetAgetNchips()
            << std::endl;
  std::cout << "Number of ASAD boards = " << this->GetAsadNboards()
            << std::endl;
  std::cout << "Number of COBO boards = " << this->GetCoboNboards()
            << std::endl;

  geometryStats.print();

  // now initialize TH2Poly (while initOK=true) and set initOK flag according to
  // the result
  initOK = InitTH2Poly();

  std::cout << "\n==== INITIALIZING TPC GEOMETRY - END ====\n\n";

  if (!initOK) {
    std::cerr << "ERROR: Cannot initialize TH2Poly !!!" << std::endl;
    if (_debug) {
      std::cout << "GeometryTPC::Load - Abort (9)" << std::flush << std::endl;
    }
    return initOK;
  }

  if (_debug) {
    std::cout << "GeometryTPC::Load - Ended..." << std::flush << std::endl;
  }
  return initOK;
}

bool GeometryTPC::LoadAnalog(std::istream &f) {
  std::string line;
  bool found = false;
  f.clear();
  f.seekg(0, std::ios::beg);
  while (getline(f, line)) {
    char name[12], rest[1];
    int cobo, asad, aget, chan_num;
    if (sscanf(line.c_str(), "%12s %d %d %d %d %s", name, &cobo, &asad, &aget,
               &chan_num, rest) == 5) {
      if (!found) {
        found = true;
        std::cout << "AUX_name  COBO ASAD  AGET / AGET_CH" << std::endl;
      }
      if (mapByAget.find(MultiKey4(cobo, asad, aget, chan_num)) ==
          mapByAget.end()) {
        std::cout << Form("%-12s%-8d%-8d%-8d%-8d", name, cobo, asad, aget,
                          chan_num)
                  << std::endl;
        // HERE GOES ADDING ANALOG CHANNELS FROM CONFIG
      } else {
        std::cout << "Error channel " << cobo << "  " << asad << "  " << aget
                  << "  " << chan_num << "  already declared as STRIP!"
                  << std::endl;
        return false;
      }
    }
  }
  return true;
}

bool GeometryTPC::InitTH2Poly() {

  if (_debug) {
    std::cout << "GeometryTPC::InitTH2Poly - Started..." << std::flush
              << std::endl;
  }

  isOK_TH2Poly = false;
  fStripMap.clear();

  // sanity checks
  if (grid_nx < 1 || grid_ny < 1 || !initOK) {
    if (_debug) {
      std::cout << "GeometryTPC::InitTH2Poly - Abort (1)" << std::flush
                << std::endl;
    }
    return false;
  }

  // find X and Y range according to geo_ptr
  double xmin = 1E30;
  double xmax = -1E30;
  double ymin = 1E30;
  double ymax = -1E30;

  for (auto &i : mapByStrip) {
    // std::cout<<this->GetDirNstrips(dir)<<std::endl;
    auto s = i.second;
    TVector2 point1 =
        reference_point + s->Offset() - s->Unit() * 0.5 * pad_pitch;
    TVector2 point2 = point1 + s->Unit() * s->Length();
    if (point1.X() < xmin)
      xmin = point1.X();
    else if (point1.X() > xmax)
      xmax = point1.X();
    if (point2.X() < xmin)
      xmin = point2.X();
    else if (point2.X() > xmax)
      xmax = point2.X();
    if (point1.Y() < ymin)
      ymin = point1.Y();
    else if (point1.Y() > ymax)
      ymax = point1.Y();
    if (point2.Y() < ymin)
      ymin = point2.Y();
    else if (point2.Y() > ymax)
      ymax = point2.Y();
    // }
  }
  if (tp) {
    tp->Delete();
    tp = NULL;
  }
  tp = new TH2Poly("h_uvw", "GeometryTPC::TH2Poly;X;Y;Charge", grid_nx, xmin,
                   xmax, grid_ny, ymin, ymax);
  if (!tp) {
    if (_debug) {
      std::cout << "GeometryTPC::InitTH2Poly - Abort (2)" << std::flush
                << std::endl;
    }
    return false;
  }
  tp->SetFloat(true); // allow to expand outside of the current limits

  auto gr=new TGraph(); // collection of initial points for calculating perimeter of the active area
  for (auto &i : mapByStrip) {

    auto *s = i.second;
    // if(!s) continue;
    int dir = s->Dir();
    int section = s->Section();
    int num = s->Num();
    // create diamond-shaped pad for a given direction
    std::vector<TVector2> offset_vec;
    offset_vec.push_back(TVector2(0., 0.));
    offset_vec.push_back(s->Unit().Rotate(TMath::Pi() / 6.) * pad_size);
    offset_vec.push_back(s->Unit() * pad_pitch);
    TVector2 point0 =
        reference_point + s->Offset() - s->Unit() * 0.5 * pad_pitch;
    int npads = s->Npads();
    const int npoints = npads * offset_vec.size();
    TGraph *g = new TGraph(npoints);
    int ipoint = 0;
    for (int ipad = 0; ipad < npads; ipad++) {
      for (size_t icorner = 0; icorner < offset_vec.size(); icorner++) {
        TVector2 corner =
            point0 + s->Unit() * ipad * pad_pitch + offset_vec[icorner];
        g->SetPoint(ipoint, corner.X(), corner.Y());
	if(ipad==0 || ipad==npads-1) gr->SetPoint(gr->GetN(), corner.X(), corner.Y()); // first and last pad
        ipoint++;
      }
    }
    offset_vec.clear();
    offset_vec.push_back(s->Unit().Rotate(-TMath::Pi() / 6.) * pad_size);
    offset_vec.push_back(TVector2(0., 0.));
    for (int ipad = npads - 1; ipad >= 0; ipad--) {
      for (size_t icorner = 0; icorner < offset_vec.size(); icorner++) {
        TVector2 corner =
            point0 + s->Unit() * ipad * pad_pitch + offset_vec[icorner];
        g->SetPoint(ipoint, corner.X(), corner.Y());
	if(ipad==0 || ipad==npads-1) gr->SetPoint(gr->GetN(), corner.X(), corner.Y()); // first and last pad
        ipoint++;
      }
    }
    // create new TH2PolyBin

    // DEBUG
    if (_debug) {
      std::cout << "TH2POLY ADDBIN: BEFORE CREATING BIN: GetNumberOfBins="
                << tp->GetNumberOfBins() << std::endl;
    }
    // DEBUG

    // primary implementation (safe for ROOT version >=6.12):
    //      const int ibin = tp->AddBin(g);

    // alternative implementation due to bin indexing bug in TH2Poly::AddBin()
    // (safe for ROOT version <6.12):
    const int nbins_old = tp->GetNumberOfBins();
    int ibin = tp->AddBin(g);
    const int nbins_new = tp->GetNumberOfBins();
    if (nbins_new > nbins_old) {
      TH2PolyBin *bin =
          (TH2PolyBin *)tp->GetBins()->At(tp->GetNumberOfBins() - 1);
      if (bin)
        ibin = bin->GetBinNumber();
    }

    // DEBUG
    if (_debug) {
      std::cout << "TH2POLY ADDBIN: AFTER CREATING BIN: GetNumberOfBins="
                << tp->GetNumberOfBins() << std::endl;
      std::cout << "TH2POLY ADDBIN: "
                << "TH2POLY_IBIN="
                << ibin
                //		  << ", TH2POLYBIN_PTR=" << bin
                << ", DIR=" << this->GetDirName(dir) << ", SECTION=" << section
                << ", NUM=" << num << ", NPADS=" << npads
                << ", NPOINTS=" << npoints << " (" << ipoint << ")"
                << std::endl;
    }
    // DEBUG

    ///////// DEBUG
    // strip_counter++;
    ///////// DEBUG

    // DEBUG

    if (ibin < 1) { // failed to create new bin
      std::cerr << "ERROR: Failed to create new TH2Poly bin: TH2POLY->AddBin()="
                << ibin << ", DIR=" << this->GetDirName(dir)
                << ", SECTION=" << section << ", NUM=" << num
                << ", NPADS=" << npads << ", NPOINTS=" << npoints
                << ", TGraph->Print():" << std::endl;
      g->Print();
      std::cerr << std::endl << std::flush;
      return false;
      //	continue;
    }

    // update strip map
    SetTH2PolyStrip(ibin, s);

    // DEBUG
    if (_debug) {
      std::cout << "TH2POLY ADDBIN: DIR=" << this->GetDirName(dir)
                << ", SECTION=" << section << ", NUM=" << num
                << ", TH2POLY_IBIN=" << ibin << ", NPADS=" << npads
                << ", NPOINTS=" << npoints << " (" << ipoint << ")"
                << std::endl;
    }
    // DEBUG

    //  }
  }

  // final result
  if (fStripMap.size()>0 && InitActiveAreaConvexHull(gr))
    isOK_TH2Poly = true;

  if (_debug) {
    std::cout << "GeometryTPC::InitTH2Poly - Ended..." << std::flush
              << std::endl;
  }

  return isOK_TH2Poly;
}

// change cartesian binning of the underlying TH2Poly
void GeometryTPC::SetTH2PolyPartition(int nx, int ny) {
  bool change = false;
  if (nx > 1 && nx != grid_nx) {
    grid_nx = nx;
    change = true;
  }
  if (ny > 1 && ny != grid_ny) {
    grid_ny = ny;
    change = true;
  }
  if (change && tp)
    tp->ChangePartition(grid_nx, grid_ny);
}

void GeometryTPC::SetTH2PolyStrip(int ibin, StripTPC *s) {
  if (s)
    fStripMap[ibin] = s;
}

////////////////////////////////////////////////////////////////////////////////
//
// Computes a convex hull of the entire UVW active area.
//
// Employs Graham scan algorithm for finding 2D convex hull for set of 2D points.
// Algorithm is based on the code published in:
// T.H.Cormen, Ch.E.Leiserson, R.L.Rivest, C.Stein
// "Introduction to Algorithms" (2nd ed.), 2001, MIT Press and McGraw-Hill.
//
////////////////////////////////////////////////////////////////////////////////
bool GeometryTPC::InitActiveAreaConvexHull(TGraph *g) {

  //  if(g) { g->Set(0); } else { g=new TGraph(); } // DEBUG
  //  auto rand=new TRandom3(0);
  //  for(auto ipoint=0u; ipoint<50u; ipoint++) {
  //    g->SetPoint(ipoint, rand->Uniform(-100, 100), rand->Uniform(-100, 100));
  //  } // DEBUG
  
  if(!tp || !g) {
    if (_debug) {
      std::cerr << __FUNCTION__ << ": Wrong input TGraph - Abort" << std::flush << std::endl;
    }
    return false;
  }

  // find point P0 with lowest X and lowest Y
  unsigned int grN=g->GetN();
  auto grX=g->GetX();
  auto grY=g->GetY();
  auto ipoint0=0u;
  auto x0=grX[ipoint0];
  auto y0=grY[ipoint0];
  for(auto ipoint=1u; ipoint<grN; ++ipoint) {
    auto x=grX[ipoint];
    auto y=grY[ipoint];
    if(y<y0 || (y==y0 && x<x0)) {
      y0=y;
      x0=x;
      ipoint0=ipoint;
    }
  }

  // convert cartesian coordinates to polar angle and radius
  auto gr2 = new TGraph();
  auto index=0u;
  gr2->SetPoint(index, 0., 0.); // Place P0 at the beginning. By definition Phi=0, R=0 for this point.

  //  if(_debug) { //DEBUG
  //    std::cout << __FUNCTION__ << ": TGRAPH UNSORTED index=" << index << " Phi=" << gr2->GetX()[index] << " rad, R=" << gr2->GetY()[index] << " mm" << std::endl;
  //  } // DEBUG

  for(auto ipoint=1u; ipoint<grN; ++ipoint) {
    if(ipoint==ipoint0) continue; // skip P0 as it is already inserted at the beginning
    index++;
    double dx=grX[ipoint]-x0; // [mm]
    double dy=grY[ipoint]-y0; // [mm]
    double rad = TMath::Sqrt( dx*dx + dy*dy ); // radius [mm]
    double phi=0.0; // polar angle [rad]
    if(dx!=0 && dy!=0) {
      phi = TMath::ATan2( dy, dx );
    }

    //    if(_debug) { //DEBUG
    //      std::cout << __FUNCTION__ << ": TGRAPH UNSORTED index=" << index << " Phi=" << phi << " rad, R=" << rad << " mm" << std::endl;
    //    } // DEBUG

    gr2->SetPoint(index, phi, rad);
  }

  // sort points by polar angle wrt P0 in ascending order, starting from index=1 (not 0)
  // NOTE: this is important due to numerical tolerances for angles close to zero
  gr2->Sort(TGraph::CompareX, true, 1);

  //  if(_debug) { // DEBUG
  //    unsigned int grN=gr2->GetN();
  //    auto grX=gr2->GetX(); // [rad]
  //    auto grY=gr2->GetY(); // [mm]
  //    for(auto ipoint=0u; ipoint<grN; ++ipoint) {
  //      std::cout << __FUNCTION__ << Form(": TGRAPH SORTED index=%d, Phi=%25.18le rad, R=%25.18le mm", ipoint, grX[ipoint], grY[ipoint]) << std::endl;
  //    }
  //  } // DEBUG

  // for points with the same polar angle keep only the farthest point from P0
  grN=gr2->GetN();
  grX=gr2->GetX(); // [rad]
  grY=gr2->GetY(); // [mm]
  std::set<unsigned int> grSet;
  for(auto ipoint=0u; ipoint<grN-1; ++ipoint) {
    for(auto ipoint2=ipoint+1u; ipoint2<grN; ipoint2++) {

      //      if(_debug) { // DEBUG
      //	std::cout << __FUNCTION__ << Form(": TGRAPH SORTED DIFF %d-to-%d, d_Phi=%25.18le rad, d_R=%25.18le mm", ipoint2, ipoint, grX[ipoint2]-grX[ipoint], grY[ipoint2]-grY[ipoint]) << std::endl;
      //      } // DEBUG

      if(fabs(grX[ipoint2]-grX[ipoint])<NUM_TOLERANCE) { // grX[ipoint2]==grX[ipoint] &&
	if( grY[ipoint2]<=grY[ipoint] ) {
	  grSet.insert(ipoint2); // mark to be excluded
	} else {
	  if(ipoint>0) grSet.insert(ipoint); // mark to be excluded
	}
      } else break;
    }
  }

  // convert polar coordinates to cartesian coordinates
  auto gr3=new TGraph(grN-grSet.size());
  index=0u;
  for(auto ipoint=0u; ipoint<grN; ++ipoint) {
    if(grSet.find(ipoint)!=grSet.end()) continue;

    //    if(_debug) { // DEBUG
    //      std::cout << __FUNCTION__ << Form(": TGRAPH SORTED, NO-DUPLICATES index=%d, Phi=%25.18le rad, R=%25.18le mm", index, grX[ipoint], grY[ipoint]) << std::endl;
    //    } // DEBUG

    gr3->SetPoint(index++, x0+grY[ipoint]*cos(grX[ipoint]), y0+grY[ipoint]*sin(grX[ipoint])); // [mm]
  }
  delete gr2; // gr2->Set(0);

  //  if(_debug) { // DEBUG
  //    unsigned int grN=gr3->GetN();
  //    auto grX=gr3->GetX();
  //    auto grY=gr3->GetY();
  //    for(auto ipoint=0u; ipoint<grN; ipoint++) {
  //      std::cout << __FUNCTION__ << Form(": TGRAPH INITIAL CARTESIAN, SORTED, NO-DUPLICATES index=%d, X=%25.18le mm, Y=%25.18le mm", ipoint, grX[ipoint], grY[ipoint]) << std::endl;
  //    }
  //  } // DEBUG

  // perform counter-clockwise scan starting from P0
  grN=gr3->GetN();
  grX=gr3->GetX(); // [mm]
  grY=gr3->GetY(); // [mm]
  std::vector<unsigned int> stack;
  stack.push_back(0); // point[0] always belongs to convex hull
  stack.push_back(1); // point[1] always belongs to convex hull
  stack.push_back(2);

  //  if(_debug) { // DEBUG
  //    for(auto i=0u; i<stack.size(); i++) {
  //      std::cout << __FUNCTION__ << Form(": TGRAPH INITIAL STACK index=%d, X=%25.18le mm, Y=%25.18le mm", i, grX[stack[i]], grY[stack[i]]) << std::endl;
  //    }
  //  } // DEBUG

  // For every point[i]:
  // 1. Keep removing points from the stack while orientation of the triplet of points
  //    {P1, P2, P3} := {next-to-top, top, point[i]} is clockwise (rigth-turn) or colinear
  // 2. Place point[i] on top of the stack.
  for(auto ipoint=3u; ipoint<grN; ++ipoint) {
    auto P3=TVector3(grX[ipoint], grY[ipoint], 0); // point[i] (POINT 3 of the triplet)
    while(stack.size()>1) {
      auto ipointN=stack[stack.size()-2];  // next-to-top of the stack (POINT 1 of the triplet)
      auto ipointT=stack[stack.size()-1];  // top of the stack (POINT 2 of the triplet)
      auto P1=TVector3(grX[ipointN], grY[ipointN], 0);
      auto P2=TVector3(grX[ipointT], grY[ipointT], 0);

      // orientation <0  :CCW rotation => exit while loop
      // orientation >=0 :CW rotation or colinear => remove from top of the stack
      double orientation=-((P2-P1).Cross(P3-P2)).Z();
      if( orientation<-NUM_TOLERANCE ) break;
      //      if( orientation<(double)0.0 ) break;
      stack.pop_back();

    }; // end of while loop

    // add point[i] to the stack
    stack.push_back(ipoint);
  }

  // create TGraph from points left in the stack
  auto gr4=new TGraph(stack.size());
  for(auto ipoint=0u; ipoint<stack.size(); ++ipoint) {
    gr4->SetPoint(ipoint, grX[stack[ipoint]], grY[stack[ipoint]]);

    //    if(_debug) { // DEBUG
    //      std::cout << __FUNCTION__ << Form(": TGRAPH CONVEX HULL index=%d, X=%25.18le mm, Y=%25.18le mm", ipoint, grX[stack[ipoint]], grY[stack[ipoint]]) << std::endl;
    //   } // DEBUG
  }
  delete gr3; // gr3->Set(0);

  // duplicate first point to close the loop
  gr4->SetPoint(gr4->GetN(), x0, y0);

  // store convex hull as TGraph (easily convertible to TH2PolyBin)
  if(tp_convex) {
    tp_convex->Delete();
    tp_convex = NULL;
  }
  tp_convex = new TGraph(gr4->GetN(), gr4->GetX(), gr4->GetY());

  if(_debug) { // DEBUG
    std::cout << __FUNCTION__ << ": Created TGraph with " << tp_convex->GetN() << " points, "
	      << "and starting point (X0=" << x0 << "mm, Y0="<< y0 << "mm)" << std::endl;
  } // DEBUG

  return true;
}

////////////////////////////////////////////////////////////
//
// Returns the stored convex hull of the entire UVW active area
// Optionally excludes outer VETO band [mm]
//
// Result can be converted to TH2PolyBin:
// $ auto hull=GetActiveAreaConvexHull(0.0);
// $ gr->Draw("AL*");
// $ auto bin=new TH2PolyBin(&hull, 1);
// $ bin->GetPolygon()->Draw("AL*");
// $ cout << "Is (x=0, y=0) inside polygon: " << bin->IsInside(0,0) << endl;
//
TGraph GeometryTPC::GetActiveAreaConvexHull(double vetoBand){

  if(vetoBand<0.0) {
    if (_debug) {
      std::cerr << __FUNCTION__ << ": VETO band must not be negative - Abort(1)" << std::endl;
    }
    return TGraph(); // dummy TGraph()
  }
  TGraph result = *tp_convex;
  if(vetoBand==0.0) {
    return result;
  }
  result.Set(result.GetN()-1); // get rid of the very last (duplicted) point closing the loop
  
  // iterate while number of points >=3 and new edges were rejected
  auto counter=0u; // number of iterations
  auto remaining_offset=vetoBand; // requested offset [mm]
  while(result.GetN()>2 && fabs(remaining_offset)>NUM_TOLERANCE) {
    counter++;
    auto input=result;
    
    // compute central point of the input figure
    unsigned int grN=input.GetN();
    auto grX=input.GetX();
    auto grY=input.GetY();
    auto xcenter=grX[0];
    auto ycenter=grY[0];
    for(auto ipoint=1u; ipoint<grN; ipoint++) {
      xcenter += grX[ipoint];
      ycenter += grY[ipoint];
    }
    xcenter /= grN;
    ycenter /= grN;

    // for each vertex calculate respective bisector line
    // (NOTE: use TVector3 for cross product)
    std::vector<TVector3> bi_dir;    // bisector direction
    std::vector<TVector3> bi_pos;    // bisector vertex relative to central point
    std::vector<TVector3> edge_dir;  // edge direction
    std::vector<TVector3> edge_pos;  // edge vertex relative to central point
    for(auto ipoint=0u; ipoint<grN; ipoint++) {
      auto ipoint2=(ipoint+1)%grN;
      auto ipoint3=(ipoint+2)%grN;
      auto point1_pos=TVector3(grX[ipoint]-xcenter, grY[ipoint]-ycenter, 0); // [mm]
      auto point2_pos=TVector3(grX[ipoint2]-xcenter, grY[ipoint2]-ycenter, 0); // [mm]
      auto point3_pos=TVector3(grX[ipoint3]-xcenter, grY[ipoint3]-ycenter, 0); // [mm]
      auto edge12_dir=point2_pos-point1_pos;
      auto edge23_dir=point3_pos-point2_pos;
      bi_pos.push_back( point2_pos );
      bi_dir.push_back( (-1.0*edge12_dir.Unit()+edge23_dir.Unit()).Unit() );
      edge_pos.push_back( point2_pos );
      edge_dir.push_back( edge23_dir.Unit() );
    }

    // for each edge calculate crossing point of bisector lines corresponding to each vertex
    // and store the distance of the caluclated point from the edge
    TGraph grSort;
    for(auto ipoint=0u; ipoint<grN; ipoint++) {
      auto ipoint2=(ipoint+1)%grN;
      auto pos1=bi_pos[ipoint];
      auto dir1=bi_dir[ipoint];
      auto pos2=bi_pos[ipoint2];
      auto dir2=bi_dir[ipoint2];
      auto A=dir1;
      auto B=dir2*(-1.0);
      auto C=pos2-pos1;
      auto detW   = A.X()*B.Y()-A.Y()*B.X();
      auto detW12 = C.X()*B.Y()-C.Y()*B.X();
      if(detW==0.0) {
	if(_debug) {
	  std::cerr << __FUNCTION__ << " Wrong VETO band - Abort(2)" << std::endl;
	}
	return TGraph(); // dummy TGraph
      }
      auto bi_cross_pos=TVector3(pos1.X()+dir1.X()*detW12/detW, pos1.Y()+dir1.Y()*detW12/detW, 0);
      auto rel_pos1=edge_pos[ipoint]-bi_cross_pos;
      auto cross_dist1=(rel_pos1-(rel_pos1*edge_dir[ipoint])*edge_dir[ipoint]).Mag();
      grSort.SetPoint(grSort.GetN(), cross_dist1, ipoint);
    }
    // sort by distance
    grSort.Sort();

    // check all calculated distances and compute safe offset value
    std::set<unsigned int> rejectSet;
    auto prev_dist=0.0;
    auto safe_offset=remaining_offset;
    for (auto grIndex=0u; grIndex<grN; grIndex++) {
      double x, y;
      grSort.GetPoint(grIndex, x, y);
      auto ipoint=(unsigned int)y;
      auto dist=(double)x;

      //      if(_debug) { // DEBUG
      //	std::cout << __FUNCTION__ << ": iter=" << counter
      //		  << ", edge " << ipoint << "-" << (ipoint+1)%grN
      //		  << " dist=" << dist << ", prev_dist=" << prev_dist
      //		  << ", safe_offset=" << safe_offset << std::endl;
      //      } // DEBUG

      if(grIndex>0 && fabs(prev_dist-dist)>NUM_TOLERANCE) break;
      prev_dist=dist;

      // reduce number of points if needed
      if( dist-safe_offset<NUM_TOLERANCE) { // less or nearly equal
	safe_offset=dist;
	rejectSet.insert(ipoint);
	
	//	if(_debug) { // DEBUG
	//	  std::cout << __FUNCTION__ << ": iter=" << counter
	//		    << ", safe_offset=" << safe_offset
	//		    << ", rejected edge " << ipoint << "-" << (ipoint+1)%grN << std::endl;
	//	} // DEBUG
	
      }
    }
    remaining_offset -= safe_offset;
    
    // prepare resulting polygon / input for next iteration
    result.Set(0);
    for(auto ipoint=0u; ipoint<grN; ipoint++) {
      if(rejectSet.find(ipoint)!=rejectSet.end()) continue;
      auto sinus=(edge_dir[ipoint].Cross(bi_dir[ipoint])).Mag();
      auto new_pos=edge_pos[ipoint]+bi_dir[ipoint]*(safe_offset/sinus);
      result.SetPoint(result.GetN(), xcenter+new_pos.X(), ycenter+new_pos.Y());
    }
    
  }; // end of iteration loop

  if(result.GetN()<3) {
    if(_debug) {
      std::cerr << __FUNCTION__ << " Wrong VETO band - Abort(3)" << std::endl;
    }
    return TGraph(); // dummy TGraph
  }

  // duplicate first point to close the loop
  double x0, y0;
  result.GetPoint(0, x0, y0);
  result.SetPoint(result.GetN(), x0, y0);

  if(_debug) { // DEBUG
    std::cout << __FUNCTION__ << ": Created TGraph with " << result.GetN() << " points, "
	      << "and starting point (X0=" << x0 << "mm, Y0="<< y0 << "mm)" << std::endl;
  } // DEBUG

  return result;
}

void GeometryTPC::setDriftVelocity(double v){
  runConditions.setDriftVelocity(v);
}

void GeometryTPC::setSamplingRate(double r){
  runConditions.setSamplingRate(r);
}

void GeometryTPC::setTriggerDelay(double d){
  runConditions.setTriggerDelay(d);
}

StripTPC *GeometryTPC::GetTH2PolyStrip(int ibin) {
  return (fStripMap.find(ibin) == fStripMap.end() ? NULL : fStripMap[ibin]);
}

int GeometryTPC::GetDirNstrips(int dir) {
  if (!IsOK())
    return -1;
  switch (dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W:
    return GetDirNStripsMerged(dir);
    break;
  case FPN_CH:
    return stripN[dir];
    break;
  };
  return ERROR;
}
int GeometryTPC::GetDirNstrips(std::string name) {
  return this->GetDirNstrips(this->GetDirIndex(name));
}
int GeometryTPC::GetDirNstrips(const char *name) {
  return this->GetDirNstrips(this->GetDirIndex(std::string(name)));
}
int GeometryTPC::GetDirNstrips(StripTPC *s) {
  if (s)
    return this->GetDirNstrips(s->Dir());
  return ERROR;
}

const char *GeometryTPC::GetDirName(int dir) {
  if (!IsOK())
    return NULL; // "ERROR";
  switch (dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W:
  case FPN_CH:
    return dir2name.at(dir).c_str(); // dir2name[dir];
  };
  return NULL; // "ERROR";
}

int GeometryTPC::GetDirIndex(std::string name) {
  std::map<std::string, int>::iterator it;
  if ((it = name2dir.find(name)) != name2dir.end())
    return it->second;
  else
    return ERROR;
}
int GeometryTPC::GetDirIndex(const char *name) {
  return this->GetDirIndex(std::string(name));
}

int GeometryTPC::GetDirIndex(int global_channel_idx) {
  StripTPC *strip = this->GetStripByGlobal(global_channel_idx);
  if (strip)
    return strip->Dir();
  return ERROR;
}

int GeometryTPC::GetDirIndex(int COBO_idx, int ASAD_idx, int AGET_idx,
                             int channel_idx) {
  StripTPC *strip =
      this->GetStripByAget(COBO_idx, ASAD_idx, AGET_idx, channel_idx);
  if (strip)
    return strip->Dir();
  return ERROR;
}

int GeometryTPC::GetDirIndex_raw(int global_raw_channel_idx) {
  StripTPC *strip = this->GetStripByGlobal_raw(global_raw_channel_idx);
  if (strip)
    return strip->Dir();
  return ERROR;
}

int GeometryTPC::GetDirIndex_raw(int COBO_idx, int ASAD_idx, int AGET_idx,
                                 int raw_channel_idx) {
  StripTPC *strip =
      this->GetStripByAget_raw(COBO_idx, ASAD_idx, AGET_idx, raw_channel_idx);
  if (strip)
    return strip->Dir();
  return ERROR;
}

const char *GeometryTPC::GetStripName(StripTPC *s) {
  if (!IsOK() || !s)
    return NULL; // "ERROR";
  std::stringstream ss;
  switch (s->Dir()) {
  case DIR_U:
  case DIR_V:
  case DIR_W:
  case FPN_CH: {
    if (s->Num() >= 1 && s->Num() <= stripN.at(s->Dir())) {
      ss << dir2name.at(s->Dir()) << s->Num();
      return ss.str().c_str();
    }
  }
  };
  return NULL; // "ERROR";
}

StripTPC *GeometryTPC::GetStripByAget(
    int COBO_idx, int ASAD_idx, int AGET_idx,
    int channel_idx) { // valid range [0-1][0-3][0-3][0-63]
  if (!IsOK())
    return NULL; // ERROR
  std::map<MultiKey4, StripTPC *, multikey4_less>::iterator it;
  if ((it = mapByAget.find(MultiKey4(COBO_idx, ASAD_idx, AGET_idx,
                                     channel_idx))) != mapByAget.end()) {
    return it->second;
  }
  return NULL; // ERROR
}

StripTPC *
GeometryTPC::GetStripByGlobal(int global_channel_idx) { // valid range [0-1023]
  if (global_channel_idx < 0)
    return NULL; // ERROR
  int channel_idx = global_channel_idx % AGET_Nchan;
  int rest1 = global_channel_idx / AGET_Nchan;
  int AGET_idx = rest1 % AGET_Nchips;
  int rest2 = rest1 / AGET_Nchips;
  int counter = 0;
  for (int icobo = 0; icobo < COBO_N; icobo++) {
    for (int iasad = 0; iasad < ASAD_N[icobo]; iasad++) {
      if (counter == rest2) {
        return this->GetStripByAget(icobo, iasad, AGET_idx, channel_idx);
      }
      counter++;
    }
  }
  return NULL; // ERROR
}

StripTPC *GeometryTPC::GetStripByAget_raw(
    int COBO_idx, int ASAD_idx, int AGET_idx,
    int raw_channel_idx) { // valid range [0-1][0-3][0-3][0-67]
  if (!IsOK())
    return NULL; // ERROR
  std::map<MultiKey4, StripTPC *, multikey4_less>::iterator it;
  if ((it = mapByAget_raw.find(
           MultiKey4(COBO_idx, ASAD_idx, AGET_idx, raw_channel_idx))) !=
      mapByAget_raw.end()) {
    return it->second;
  }
  return NULL; // ERROR
}

StripTPC *GeometryTPC::GetStripByGlobal_raw(
    int global_raw_channel_idx) { // valid range [0-1023]
  if (global_raw_channel_idx < 0)
    return NULL; // ERROR
  int raw_channel_idx = global_raw_channel_idx % AGET_Nchan_raw;
  int rest1 = global_raw_channel_idx / AGET_Nchan_raw;
  int AGET_idx = rest1 % AGET_Nchips;
  int rest2 = rest1 / AGET_Nchips;
  int counter = 0;
  for (int icobo = 0; icobo < COBO_N; icobo++) {
    for (int iasad = 0; iasad < ASAD_N[icobo]; iasad++) {
      if (counter == rest2) {
        return this->GetStripByAget_raw(icobo, iasad, AGET_idx,
                                        raw_channel_idx);
      }
      counter++;
    }
  }
  return NULL; // ERROR
}

StripTPC *GeometryTPC::GetStripByDir(int dir, int section,
                           int num) { // valid range [0-2][0-2][1-1024]
  return this->GetStripByGlobal(Global_strip2normal(dir, section, num));
}
//StripTPC *GeometryTPC::GetStripByDir(
//    int dir, int num) { // legacy for section=0, valid range [0-2][1-1024]
//  return GetStripByDir(dir, 0, num);
//}

int GeometryTPC::Aget_normal2raw(int channel_idx) { // valid range [0-63]
  if (/*!IsOK() || */ channel_idx < 0 || channel_idx >= AGET_Nchan)
    return ERROR;
  int raw_channel_idx = channel_idx;
  for (uint i = 0; i < FPN_chanId.size(); ++i) {
    if (FPN_chanId[i] < raw_channel_idx)
      ++raw_channel_idx;
    if (FPN_chanId[i] == raw_channel_idx)
      ++raw_channel_idx;
  }
  return raw_channel_idx;
}

int GeometryTPC::Aget_raw2normal(int raw_channel_idx) { // valid range [0-67]
  if (/*!IsOK() || */ raw_channel_idx < 0 || raw_channel_idx >= AGET_Nchan_raw)
    return ERROR;
  int channel_idx = raw_channel_idx;
  for (uint i = 0; i < FPN_chanId.size(); ++i) {
    if (FPN_chanId[i] == raw_channel_idx)
      return ERROR;
    else if (FPN_chanId[i] < raw_channel_idx)
      --channel_idx;
  }
  return channel_idx;
}

int GeometryTPC::Asad_normal2raw(
   int asad_channel_idx) { // valid range [0-255]
  int aget_raw_channel_idx = Aget_normal2raw(asad_channel_idx % AGET_Nchan);
  int AGET_idx = (int)(asad_channel_idx / AGET_Nchan); // relative AGET index
  if (aget_raw_channel_idx == ERROR) return ERROR;
  return AGET_idx * AGET_Nchan_raw + aget_raw_channel_idx;
}

int GeometryTPC::Asad_normal2raw(
    int aget_idx,
    int channel_idx) { // valid range [0-3][0-63]
  int aget_raw_channel_idx = Aget_normal2raw(channel_idx);
  if (aget_raw_channel_idx == ERROR || 
      aget_idx < 0 ||
      aget_idx >= AGET_Nchips) return ERROR;
  return aget_idx * AGET_Nchan_raw + aget_raw_channel_idx;
}

int GeometryTPC::Asad_normal2normal(
    int aget_idx,
    int channel_idx) { // valid range [0-3][0-63]
  if(/*!IsOK() || */
     aget_idx<0 || aget_idx>=AGET_Nchips ||
     channel_idx<0 || channel_idx>=AGET_Nchan ) return ERROR;
  return aget_idx * AGET_Nchan + channel_idx;
}

int GeometryTPC::Asad_raw2normal(
    int aget_idx,
    int raw_channel_idx) { // valid range [0-3][0-67]
  int aget_channel_idx = Aget_raw2normal(raw_channel_idx);
  if (aget_channel_idx == ERROR ||
      aget_idx < 0 ||
      aget_idx >= AGET_Nchips)
    return ERROR;
  return aget_idx * AGET_Nchan + aget_channel_idx;
}

int GeometryTPC::Asad_raw2normal(
    int asad_raw_channel_idx) { // valid range [0-(1023+4*4)]
  int aget_channel_idx = Aget_raw2normal(asad_raw_channel_idx % AGET_Nchan_raw);
  int AGET_idx = (int)(asad_raw_channel_idx / AGET_Nchan_raw); // relative AGET index
  if (aget_channel_idx == ERROR) return ERROR;
  return AGET_idx * AGET_Nchan + aget_channel_idx;
}

int GeometryTPC::Asad_raw2raw(
    int aget_idx,
    int raw_channel_idx) { // valid range [0-3][0-67]
  if (/*!IsOK() || */
      aget_idx < 0 || aget_idx >= AGET_Nchips ||
      raw_channel_idx < 0 || raw_channel_idx >= AGET_Nchan_raw) return ERROR;
  return aget_idx * AGET_Nchan_raw + raw_channel_idx;
}

int GeometryTPC::Global_normal2raw(
    int COBO_idx, int ASAD_idx, int aget_idx,
    int channel_idx) { // valid range [0-1][0-3][0-3][0-63]
  int glb_raw_channel_idx = Aget_normal2raw(channel_idx);
  if (glb_raw_channel_idx == ERROR || COBO_idx < 0 || COBO_idx >= COBO_N ||
      ASAD_idx < 0 || ASAD_idx > ASAD_N[COBO_idx] || aget_idx < 0 ||
      aget_idx >= AGET_Nchips)
    return ERROR;
  int ASAD_offset = 0;
  for (int icobo = 0; icobo < COBO_idx; icobo++) {
    ASAD_offset += ASAD_N[icobo];
  }
  return ((ASAD_offset + ASAD_idx) * AGET_Nchips + aget_idx) * AGET_Nchan_raw +
         glb_raw_channel_idx;
}

int GeometryTPC::Global_raw2normal(
    int COBO_idx, int ASAD_idx, int aget_idx,
    int raw_channel_idx) { // valid range [0-1][0-3][0-3][0-67]
  int glb_channel_idx = Aget_raw2normal(raw_channel_idx);
  if (glb_channel_idx == ERROR || COBO_idx < 0 || COBO_idx >= COBO_N ||
      ASAD_idx < 0 || ASAD_idx > ASAD_N[COBO_idx] || aget_idx < 0 ||
      aget_idx >= AGET_Nchips)
    return ERROR;
  int ASAD_offset = 0;
  for (int icobo = 0; icobo < COBO_idx; icobo++) {
    ASAD_offset += ASAD_N[icobo];
  }
  return ((ASAD_offset + ASAD_idx) * AGET_Nchips + aget_idx) * AGET_Nchan +
         glb_channel_idx;
}

int GeometryTPC::Global_fpn2raw(
    int COBO_idx, int ASAD_idx, int aget_idx,
    int FPN_idx) { // valid range [0-1][0-3][0-3][0-3]
  if (/*!IsOK() || */ COBO_idx < 0 || COBO_idx >= COBO_N || ASAD_idx < 0 ||
      ASAD_idx > ASAD_N[COBO_idx] || aget_idx < 0 || aget_idx >= AGET_Nchips ||
      FPN_idx < 0 || FPN_idx >= stripN[FPN_CH])
    return ERROR;
  int ASAD_offset = 0;
  for (int icobo = 0; icobo < COBO_idx; icobo++) {
    ASAD_offset += ASAD_N[icobo];
  }
  return ((ASAD_offset + ASAD_idx) * AGET_Nchips + aget_idx) * AGET_Nchan_raw +
         FPN_chanId[FPN_idx];
}
int GeometryTPC::Aget_fpn2raw(int FPN_idx) { // valid range [0-3]
  if (/*!IsOK() || */ FPN_idx < 0 || FPN_idx >= stripN[FPN_CH])
    return ERROR;
  return FPN_chanId.at(FPN_idx);
}

int GeometryTPC::Global_normal2raw(
    int glb_channel_idx) { // valid range [0-1023]
  int glb_raw_channel_idx = Aget_normal2raw(glb_channel_idx % AGET_Nchan);
  int AGET_idx = (int)(glb_channel_idx / AGET_Nchan); // relative AGET index
  int ASAD_idx = (int)(AGET_idx / AGET_Nchips);       // relative ASAD index
  int counter = 0;
  for (int icobo = 0; icobo < COBO_N; icobo++)
    for (int iasad = 0; iasad < ASAD_N[icobo]; iasad++) {
      if (counter == ASAD_idx && glb_raw_channel_idx != ERROR)
        return AGET_idx * AGET_Nchan_raw + glb_raw_channel_idx;
      counter++;
    }
  return ERROR;
}

int GeometryTPC::Global_normal2normal(
    int COBO_idx, int ASAD_idx, int aget_idx,
    int channel_idx) { // valid range [0-1][0-3][0-3][0-63]
  // if(/*!IsOK() || */COBO_idx<0 || COBO_idx>=COBO_N || ASAD_idx<0 ||
  // ASAD_idx>ASAD_N[COBO_idx] || aget_idx<0 || aget_idx>=AGET_Nchips ||
  //    channel_idx<0 || channel_idx>=AGET_Nchan ) return ERROR;
  int ASAD_offset = 0;
  for (int icobo = 0; icobo < COBO_idx; icobo++) {
    ASAD_offset += ASAD_N[icobo];
  }

  return ((ASAD_offset + ASAD_idx) * AGET_Nchips + aget_idx) * AGET_Nchan +
         channel_idx;
}

int GeometryTPC::Global_raw2normal(
    int glb_raw_channel_idx) { // valid range [0-(1023+ASAD_N[0]+...)]
  int glb_channel_idx = Aget_raw2normal(glb_raw_channel_idx % AGET_Nchan_raw);
  int AGET_idx = (int)(glb_channel_idx / AGET_Nchan); // relative AGET index
  int ASAD_idx = (int)(AGET_idx / AGET_Nchips);       // relative ASAD index
  int counter = 0;
  for (int icobo = 0; icobo < COBO_N; icobo++)
    for (int iasad = 0; iasad < ASAD_N[icobo]; iasad++) {
      if (counter == ASAD_idx && glb_channel_idx != ERROR)
        return AGET_idx * AGET_Nchan + glb_channel_idx;
      counter++;
    }
  return ERROR;
}

int GeometryTPC::Global_raw2raw(
    int COBO_idx, int ASAD_idx, int aget_idx,
    int raw_channel_idx) { // valid range [0-1][0-3][0-3][0-67]
  if (/*!IsOK() || */ COBO_idx < 0 || COBO_idx >= COBO_N || ASAD_idx < 0 ||
      ASAD_idx > ASAD_N[COBO_idx] || aget_idx < 0 || aget_idx >= AGET_Nchips ||
      raw_channel_idx < 0 || raw_channel_idx >= AGET_Nchan_raw)
    return ERROR;
  int ASAD_offset = 0;
  for (int icobo = 0; icobo < COBO_idx; icobo++) {
    ASAD_offset += ASAD_N[icobo];
  }
  return ((ASAD_offset + ASAD_idx) * AGET_Nchips + aget_idx) * AGET_Nchan_raw +
         raw_channel_idx;
}

int GeometryTPC::Global_strip2normal(StripTPC *s) {
  return Global_strip2normal(s->Dir(), s->Section(), s->Num());
}

int GeometryTPC::Global_strip2normal(int dir, int section,
                                     int num) { // valid range [0-2][0-2][1-92]
  std::map<MultiKey3, StripTPC *>::iterator it =
      mapByStrip.find(MultiKey3(dir, section, num));
  if (it != mapByStrip.end() && it->second)
    return (it->second)->GlobalCh();
  return ERROR;
}
//int GeometryTPC::Global_strip2normal(
//    int dir, int num) { // legacy for section=0, valid range [0-2][1-92]
//  return Global_strip2normal(dir, 0, num);
//}

int GeometryTPC::Global_strip2raw(StripTPC *s) {
  return Global_strip2raw(s->Dir(), s->Section(), s->Num());
}

int GeometryTPC::Global_strip2raw(int dir, int section,
                                  int num) { // valid range [0-2][0-2][1-92]
  std::map<MultiKey3, StripTPC *>::iterator it =
      mapByStrip.find(MultiKey3(dir, section, num));
  if (it != mapByStrip.end() && it->second)
    return (it->second)->GlobalCh_raw();
  return ERROR;
}
//int GeometryTPC::Global_strip2raw(
//    int dir, int num) { // legacy for section=0, valid range [0-2][1-92]
//  return Global_strip2raw(dir, 0, num);
//}

// Checks if 2 strips (single electronic channel) are crossing inside the active area of TPC,
// on success (true) also returns the calculated offset vector wrt ORIGIN POINT (0,0)
bool GeometryTPC::GetCrossPoint(StripTPC *strip1, StripTPC *strip2,
                                TVector2 &point) {
  if (!strip1 || !strip2)
    return false;
  const double u1[2] = {strip1->Unit().X(), strip1->Unit().Y()};
  const double u2[2] = {strip2->Unit().X(), strip2->Unit().Y()};
  double W = -u1[0] * u2[1] + u1[1] * u2[0];
  if (fabs(W) < NUM_TOLERANCE)
    return false;
  const double offset[2] = {strip2->Offset().X() - strip1->Offset().X(),
                            strip2->Offset().Y() - strip1->Offset().Y()};
  double W1 = -offset[0] * u2[1] + offset[1] * u2[0];
  double W2 = u1[0] * offset[1] - u1[1] * offset[0];
  double len1 = W1 / W;
  double len2 = W2 / W;
  double residual = 0.5 * pad_pitch;
  if (len1 < -residual - NUM_TOLERANCE || len2 < -residual - NUM_TOLERANCE ||
      len1 > strip1->Length() + NUM_TOLERANCE ||
      len2 > strip2->Length() + NUM_TOLERANCE)
    return false;
  point.Set(strip1->Offset().X() + len1 * strip1->Unit().X(),
            strip1->Offset().Y() + len1 * strip1->Unit().Y());
  point = point + reference_point;
  return true;
}

// Finds 2D crossing point of two lines defined by 2 of 3 redundant UVW coordinates expressed in millimiters
// (e.g. calculated by Strip2posUVW), on success (true) calculates offset vector wrt ORIGIN POINT (0,0)
bool GeometryTPC::GetUVWCrossPointInMM(int dir1, double UVW_pos1, int dir2, double UVW_pos2, TVector2 &point) {
  const TVector2 unit_vec[2] = { GetStripUnitVector(dir1), GetStripUnitVector(dir2) };
  const TVector2 offset_vec[2] = { GetStripPitchVector(dir1)*UVW_pos1, GetStripPitchVector(dir2)*UVW_pos2 };

  // sanity check (not parallel AND not empty)
  double W = -unit_vec[0].X() * unit_vec[1].Y() + unit_vec[0].Y() * unit_vec[1].X();  
  if (fabs(W) < NUM_TOLERANCE)
    return false;
  
  const double offset[2] = {offset_vec[1].X() - offset_vec[0].X(),
			    offset_vec[1].Y() - offset_vec[0].Y()};  
  double W1 = -offset[0] * unit_vec[1].Y() + offset[1] * unit_vec[1].X();
  // double W2 = unit_vec[0].X() * offset[1] - unit_vec[0].Y() * offset[0]; // not needed
  double len1 = W1 / W; 
  //  double len2 = W2 / W; // not needed
  point = offset_vec[0] + unit_vec[0]*len1; // postion wrt ORIGIN POINT (0,0)
  return true;
}

// Checks if 3 strips (single electronic channel) are crossing inside the active area of TPC within a given
// tolerance (radius in [mm]), on success (true) also returns the average
// calculated offset vector wrt ORIGIN POINT (0,0)
bool GeometryTPC::MatchCrossPoint(StripTPC *strip1, StripTPC *strip2,
                                  StripTPC *strip3, double radius,
                                  TVector2 &point) {
  TVector2 points[3];
  if (!this->GetCrossPoint(strip1, strip2, points[0]) ||
      !this->GetCrossPoint(strip2, strip3, points[1]) ||
      !this->GetCrossPoint(strip3, strip1, points[2]))
    return false;
  const double rad2 = radius * radius;
  if ((points[0] - points[1]).Mod2() > rad2 ||
      (points[1] - points[2]).Mod2() > rad2 ||
      (points[2] - points[0]).Mod2() > rad2)
    return false;
  point = (points[0] + points[1] + points[2]) /
          3.0; // return average position wrt ORIGIN POINT (0,0)
  return true;
}

TVector2 GeometryTPC::GetStripUnitVector(int dir) { // XY ([mm],[mm])
  const TVector2 empty(0, 0);
  if (!IsOK())
    return empty; // ERROR
  switch (dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W:
    return strip_unit_vec.at(dir);
  };
  return empty; // ERROR
}

TVector2 GeometryTPC::GetStripPitchVector(int dir) { // XY ([mm],[mm])
  
  if (!IsOK()){
    return empty; // ERROR
  }
  switch (dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W:
    return pitch_unit_vec.at(dir);
  };
  return empty; // ERROR
}

TVector3 GeometryTPC::GetStripPitchVector3D(int dir) {
  return TVector3(GetStripPitchVector(dir).X(), GetStripPitchVector(dir).Y(), 0);
}

// Calculates (signed) distance in [mm] of projection of point (X=0,Y=0) from
// projection of the central axis of the existing strip (DIR, SECTION, NUM) on
// the strip pitch axis for a given DIR family.
// The "err_flag" is set to TRUE if:
// - geometry has not been initialized properly, or
// - input strip DIR and/or NUM are invalid
double GeometryTPC::Strip2posUVW(int dir, int section, int num, bool &err_flag) { // (per section)
                                 
  return this->Strip2posUVW(this->GetStripByDir(dir, section, num), err_flag);
}
double GeometryTPC::Strip2posUVW(int dir, int num, bool &err_flag) { // (all sections)
  double x=0.0;
  for(unsigned int isec=0; isec<this->GetDirSectionIndexList(dir).size(); isec++) {
    x = Strip2posUVW(dir, this->GetDirSectionIndexList(dir).at(isec), num, err_flag);
    /////// DEBUG
    //      if(dir==DIR_U) std::cout << "Strip2pos: dir=" << dir << ", strip=" << num << ", section=" << this->GetDirSectionIndexList(dir).at(isec)
    //			       << ": X[mm]=" << x << ", err_flag=" << err_flag << std::endl;
    /////// DEBUG
    if(!err_flag) return x; // find first section that contains the given strip number
  }
  err_flag = true;
  return 0.0; // ERROR
}

// Calculates (signed) distance in [mm] of projection of point (X=0,Y=0) from
// projection of the central axis of the existing strip (StripTPC object) on
// the strip pitch axis for a given strip family.
// The "err_flag" is set to TRUE if:
// - geometry has not been initialized properly, or
// - input StripTPC object is invalid
double GeometryTPC::Strip2posUVW(StripTPC *strip, bool &err_flag) {
  err_flag = true;
  if (!strip)
    return 0.0; // ERROR
  switch (strip->Dir()) {
  case DIR_U:
  case DIR_V:
  case DIR_W:
    err_flag = false; // valid strip
    return (reference_point + strip->Offset()) *
           pitch_unit_vec[strip->Dir()]; // [mm]
  };
  return 0.0; // ERROR
}

// Calculates (signed) distance of projection of (X=0, Y=0) point from
// projection of a given (X,Y) point on the strip pitch axis
// for a given DIR family.
// The "err_flag" is set to TRUE if:
// - geometry has not been initialized properly, or
// - input strip DIR is invalid
double GeometryTPC::Cartesian2posUVW(double x, double y, int dir,
                                     bool &err_flag) {
  return this->Cartesian2posUVW(TVector2(x, y), dir, err_flag); // [mm]
}

// Calculates (signed) distance of projection of (X=0, Y=0) point from
// projection of a given (X,Y) point on the strip pitch axis
// for a given DIR family.
double GeometryTPC::Cartesian2posUVW(TVector2 pos, int dir, bool &err_flag) {
  err_flag = true;
  switch (dir) {
  case DIR_U:
  case DIR_V:
  case DIR_W:
    err_flag = false;                 // valid DIR
    return pos * pitch_unit_vec[dir]; // [mm]
  };
  return 0.0; // ERROR
}

// Calculates position in [mm] along Z-axis corresponding to
// the given (fractional) time cell number [0-511]
// The "err_flag" is set to TRUE if:
// - geometry has not been initialized properly, or
// - input time-cell is outside of the valid range [0-511]
double GeometryTPC::Timecell2pos(double position_in_cells, bool &err_flag) {
  err_flag = true;
  if (!IsOK())
    return 0.0; // ERROR
  if (position_in_cells >= 0.0 && position_in_cells <= AGET_Ntimecells)
    err_flag = false; // check range
  return (position_in_cells - AGET_Ntimecells)*GetTimeBinWidth() +
    GetTriggerDelay() * GetDriftVelocity() * 10.0; // [mm]
}

// Calculates (fractional) time cell number [0-511] corresponding to
// the given Z-coordinate in [mm]
// The "err_flag" is set to TRUE if:
// - geometry has not been initialized properly, or
// - resulting time-cell is outside of the valid range [0-511]
double GeometryTPC::Pos2timecell(double z, bool &err_flag) {
  err_flag = true;
  const double position_in_cells =
      AGET_Ntimecells +
    GetSamplingRate() * (z / GetDriftVelocity() / 10.0 - GetTriggerDelay()); // [cells]
  if (position_in_cells >= 0.0 && position_in_cells <= AGET_Ntimecells)
    err_flag = false; // check range
  return position_in_cells;
}

// min/max X [mm] cartesian coordinates covered by UVW active area
std::tuple<double, double> GeometryTPC::rangeX() {
  if(!isOK_TH2Poly) return std::tuple<double, double>(0.0, 0.0); // ERROR
  return std::tuple<double, double>(tp->GetXaxis()->GetXmin(),
				    tp->GetXaxis()->GetXmax());
}

// min/max Y [mm] cartesian coordinates covered by UVW active area
std::tuple<double, double> GeometryTPC::rangeY() {
  if(!isOK_TH2Poly) return std::tuple<double, double>(0.0, 0.0); // ERROR
  return std::tuple<double, double>(tp->GetYaxis()->GetXmin(),
				    tp->GetYaxis()->GetXmax());
}

// min/max Z [mm] cartesian coordinates covered by drift cage
std::tuple<double, double> GeometryTPC::rangeZ() {
  return std::tuple<double, double>(drift_zmin, drift_zmax);
}

// min/max X and Y [mm] cartesian coordinates covered by UVW active area
std::tuple<double, double, double, double> GeometryTPC::rangeXY() {
  if(!isOK_TH2Poly) return std::tuple<double, double, double, double>(0.0, 0.0, 0.0, 0.0); // ERROR
  return std::tuple<double, double, double, double>(tp->GetXaxis()->GetXmin(),
						    tp->GetXaxis()->GetXmax(),
						    tp->GetYaxis()->GetXmin(),
						    tp->GetYaxis()->GetXmax());
}

// min/max X and Y [mm] cartesian coordinates covered by UVW active area
// and min/max Z [mm] cartesian coordinates covered by drift cage
std::tuple<double, double, double, double, double, double> GeometryTPC::rangeXYZ() {
  if(!isOK_TH2Poly) return std::tuple<double, double, double, double, double, double>(0.0, 0.0, 0.0, 0.0, drift_zmin, drift_zmax); // ERROR
  return std::tuple<double, double, double, double, double, double>
    (tp->GetXaxis()->GetXmin(),
     tp->GetXaxis()->GetXmax(),
     tp->GetYaxis()->GetXmin(),
     tp->GetYaxis()->GetXmax(),
     drift_zmin, drift_zmax);
}

// min/max (signed) distance [mm] between projection of outermost strip's central axis and
// projection of the origin (X=0,Y=0) point on the U/V/W pitch axis for a given direction (per section)
std::tuple<double, double> GeometryTPC::rangeStripSectionInMM(int dir, int section) {
  double minStripInMM=1E30;
  double maxStripInMM=-1E30;

  double distance1, distance2; 
  StripTPC *firstStrip = GetStripByDir(dir, section,
				       GetDirMinStrip(dir, section));
  StripTPC *lastStrip = GetStripByDir(dir, section,
				      GetDirMaxStrip(dir, section));
  if(firstStrip && lastStrip) {
    distance1 = (firstStrip->Offset() + GetReferencePoint())*GetStripPitchVector(dir);
    distance2 = (lastStrip->Offset() + GetReferencePoint())*GetStripPitchVector(dir);
    if(distance1>maxStripInMM) maxStripInMM=distance1;
    if(distance1<minStripInMM) minStripInMM=distance1;
    if(distance2>maxStripInMM) maxStripInMM=distance2;
    if(distance2<minStripInMM) minStripInMM=distance2;
  }
  
  //////// DEBUG
  //  std::cout << "rangeStripSectionInMM: DIR=" << GetDirName(dir) << ", Section=" << section << ", range[mm]=[" << minStripInMM << ", " << maxStripInMM << "]" << std::endl;
  //////// DEBUG
  return std::tuple<double, double>( minStripInMM, maxStripInMM );
}

// min/max (signed) distance [mm] between projection of outermost strip's central axis and
// projection of the origin (X=0,Y=0) point on the U/V/W pitch axis for a given direction (all sections)
std::tuple<double, double> GeometryTPC::rangeStripDirInMM(int dir) {
  double minStripInMM=1E30;
  double maxStripInMM=-1E30;

  // - loop over all sections in a given strip direction
  for(auto &i : GetDirSectionIndexList(dir)) {
    double distance1, distance2;
    std::tie(distance1, distance2) = rangeStripSectionInMM(dir, i);
    if(distance1>maxStripInMM) maxStripInMM=distance1;
    if(distance1<minStripInMM) minStripInMM=distance1;
    if(distance2>maxStripInMM) maxStripInMM=distance2;
    if(distance2<minStripInMM) minStripInMM=distance2;
  }
  //////// DEBUG
  //  std::cout << "rangeStripDirInMM: DIR=" << GetDirName(dir) << ", range[mm]=[" << minStripInMM << ", " << maxStripInMM << "]" << std::endl;
  //////// DEBUG
  return std::tuple<double, double>( minStripInMM, maxStripInMM );
}

void GeometryTPC::Debug() {
  for (auto &i : mapByStrip) {
    auto s = i.second;
    auto p = s->Offset() + reference_point;
    double x = p.X();
    double y = p.Y();
    // auto p=s->Offset()+reference_point;
    std::cout << p.X() << ", " << p.Y() << s->Section() << std::endl;
    tp->Fill(x, y, s->Section() + 1);
  }
}

// ClassImp(StripTPC)
// ClassImp(GeometryTPC)

