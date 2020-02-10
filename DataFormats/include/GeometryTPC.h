#ifndef __GEOMETRYTPC_H__
#define __GEOMETRYTPC_H__

// TPC geometry class.
// VERSION: 05 May 2018

#include <cstdlib>
#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <iostream> // for: cout, cerr, endl
#include <sstream>
#include <iterator>
#include <fstream>
#include <utility>
#include <numeric>
#include <functional>
#include <mutex>
#include <sstream>
#include <tuple>

#include "TROOT.h"
#include "TMath.h"
#include "TVector2.h"
#include "TGraph.h"
#include "TH2Poly.h"
#include "CommonDefinitions.h"
 
constexpr auto ERROR = -1;   // error result indicator
constexpr auto NUM_TOLERANCE = 1e-6;

class Geometry_Strip;

class GeometryTPC;

GeometryTPC& Geometry(std::string fname = "", bool debug = false);

auto find_line = [](std::vector<std::string>& vec, std::string str) {
    return std::find_if(/*std::execution::par, */vec.begin(), vec.end(), [&](std::string& str_) { return str_.find(str) != std::string::npos; }); //C++17
};

// UVW strip geometry defined as a class

class GeometryTPC {

 private:

  int COBO_N;                           // total # of COBO boards in the system
  int AGET_Nchips;                      // # of AGET chips per ASAD board
  int AGET_Nchan;                       // # of channels in AGET chip (without FPN channels)
  int AGET_Nchan_fpn;                   // # of FPN channels in AGET chip
  int AGET_Nchan_raw;                   // # of total channels in AGET chip (including FPN channels)
  int AGET_Ntimecells;                  // # of time cells (buckets) in AGET chip
  int ASAD_Nboards;                     // total # of ASAD boards
  std::map<direction, int>         stripN{ {direction::U, 0},{direction::V, 0},{direction::W, 0} };    // pair=(directory_idx, number_of_strips=xxx,xxx,xxx,4*ASAD_N*COBO_N)                          
  std::map<direction, std::string> dir2name;  // pair=(directory_idx, group_name="U","V","W","FPN")
  std::map<std::string, direction> name2dir;  // pair=(group_name="U","V","W","FPN", directory_idx)
  std::map<std::tuple<int, int, int, int>, std::shared_ptr<Geometry_Strip>> arrayByAget;/*channel_idx[0-63] AGET_idx[0-3] ASAD_idx[0-3] COBO_idx[0-1]*/
  std::map<std::tuple<direction, int> ,std::shared_ptr<Geometry_Strip>> stripArray; // key = {U,V,W}, STRIP_NUMBER [1-1024]
  std::vector<int> ASAD_N;       // pair=(COBO_idx, number of ASAD boards)
  std::vector<int> FPN_chanId;     // FPN channels in AGET chips
  double pad_size;                 // in [mm]
  double pad_pitch;                // in [mm]
  double strip_pitch;              // in [mm]
  TVector2 reference_point;        // XY offset in [mm] of the REFERENCE POINT used to define the geometry
  std::map<direction, TVector2> strip_unit_vec; // XY unit vector in [mm] for a given family of strips pointing towards ascending pad numbers of the strip
  std::map<direction, TVector2> pitch_unit_vec; // XY unit vector in [mm] for a given family of strips pointing towards ascending strip numbers 
  double vdrift;                   // electron drift velocity in [cm / micosecond]
  double sampling_rate;            // electronics sampling rate in [MHz]
  double trigger_delay;            // delay in [microseconds] of the external "t0" trigger signal (for accelerator beam) 
  double drift_zmin;               // lower drift cage acceptance limit along Z-axis [mm] (closest to readout PCB)
  double drift_zmax;               // upper drift cage acceptance limit along Z-axis [mm] (farthest from readout PCB)
  const bool _debug;                     // debug/verbose info flag
     
  // Setter methods 
  
  bool Load(std::string fname);                 // loads geometry from TXT config file
  
  GeometryTPC(std::string  fname, bool debug=false);

 public:

  // Getter methods  
  int GetDirNstrips(direction dir) const;

  inline int GetAgetNchips() { return AGET_Nchips; }
  inline int GetAgetNchannels() { return AGET_Nchan; }
  inline int GetAgetNchannels_raw() { return AGET_Nchan_raw; }
  inline int GetAgetNchannels_fpn() { return AGET_Nchan_fpn; }
  inline int GetAgetNtimecells() { return AGET_Ntimecells; }
  inline int GetAsadNboards() { return ASAD_Nboards; }
  inline int GetCoboNboards() { return COBO_N; }

  std::string GetDirName(direction dir);

  std::shared_ptr<Geometry_Strip> GetStripByAget(int COBO_idx, int ASAD_idx, int AGET_idx, int channel_idx) const;         // valid range [0-1][0-3][0-3][0-63]
  std::shared_ptr<Geometry_Strip> GetStripByDir(direction dir, int num) const;                                                   // valid range [0-2][1-1024]
  std::vector<std::shared_ptr<Geometry_Strip>> GetStrips() const;

  // various helper functions for calculating local/global normal/raw channel index
  int Aget_normal2raw(int channel_idx);                      // valid range [0-63]
  int Aget_fpn2raw(int FPN_idx);                             // valid range [0-3]

  int Global_normal2normal(int COBO_idx, int ASAD_idx, int aget_idx, int channel_idx);   // valid range [0-1][0-3][0-3][0-63]

  bool GetCrossPoint(std::shared_ptr<Geometry_Strip> strip1, std::shared_ptr<Geometry_Strip> strip2, TVector2 &point);
  bool MatchCrossPoint(std::array<int, 3> strip_nums, double radius, TVector2 &point);

  inline double GetPadPitch() { return pad_pitch; } // [mm]
  inline double GetStripPitch() { return strip_pitch; } // [mm]
  inline TVector2 GetReferencePoint() { return reference_point; } // XY ([mm],[mm])
  TVector2 GetStripPitchVector(direction dir); // XY ([mm],[mm])

  double Strip2posUVW(direction dir, int number); // [mm] (signed) distance of direction of (X=0, Y=0) point from direction of the central line of the (existing) strip on the strip pitch axis for a given direction
  double Strip2posUVW(std::shared_ptr<Geometry_Strip> strip); // [mm] (signed) distance of direction of (X=0, Y=0) point from direction of the central line of the (existing) strip on the strip pitch axis for a strip given direction

  double Timecell2pos(double position_in_cells); // [mm] output: position along Z-axis

  std::tuple<double, double, double, double> rangeXY(); //min/max X Y cartesian coordinates covered by strips in any direction
  
  friend GeometryTPC& Geometry(std::string fname, bool debug);
};

#include "Geometry_Strip.h"

#endif
